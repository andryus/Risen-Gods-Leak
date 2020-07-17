#pragma once

#include <tuple>

/****************************************************************
* Adds functionality for compile-time reflection of function
* names/signatures
* 
* Usage example:
* 
* DEFINE_METHOD_RELECTION(Visit, true)
*
* //checks for Visitor::Visit<Creature>
* if constexpr(VisitExists<Visitor, Creature>) 
*    visitor.Visit(creature);
*
********************************************************/

#undef CONST

namespace Reflect
{
    enum class ResultMode : uint8_t
    {
        LAST,
        OR
    };

    namespace detail
    {
        template<class Return>
        struct ReturnResult
        {
            Return result = Return();

            constexpr void WriteResult(Return&& result)
            {
                this->result = std::move(result);
            }
            constexpr Return ReadResult() const
            {
                return std::move(result);
            }
        };

        template<class Return>
        struct ReturnResult<Return&>
        {
            Return* result = nullptr;

            constexpr void WriteResult(Return& result)
            {
                this->result = &result;
            }
            constexpr Return* ReadResult() const
            {
                return result;
            }
        };

        template<>
        struct ReturnResult<void>
        {
            constexpr void WriteResult(std::void_t<>)
            {
            }
            constexpr void ReadResult() const
            {
            }
        };

        template<class... CallArgs>
        struct CallTypes
        {
            template<class... BaseParams>
            struct Params
            {
            private:
                template<class Executor>
                using ReturnValue = decltype(std::declval<Executor>().template Exec<BaseParams...>(std::declval<CallArgs>()...));
                template<class Executor>
                using Result = detail::ReturnResult<ReturnValue<Executor>>;

            public:

                template<class Executor>
                struct Call :
                    public Result<Executor>
                {
                    using CallTuple = std::tuple<CallArgs...>;

                    constexpr Call(Executor& executor, CallArgs&&... args) :
                        executor(executor), args(std::move(args)...) { }

                    template<class... Types>
                    constexpr void Exec()
                    {
                        Result<Executor>::WriteResult(std::apply(executor.template Exec<Types...>, args));
                    }

                private:

                    Executor & executor;
                    CallTuple args;
                };

                template<class Executor>
                struct CallOr :
                    public Call<Executor>
                {
                    using Call<Executor>::Call;

                    template<class... Types>
                    constexpr void Exec()
                    {
                        const bool preResult = Call<Executor>::ReadResult();
                        Call<Executor>::template Exec<Types...>();

                        Call<Executor>::result |= preResult;
                    }
                };
            };
        };

        template<typename Executor, typename... BaseParams>
        struct CallTypeSelector
        {
            template<ResultMode Result, typename... CallArgs>
            struct Arguments
            {
            private:

                using Supplier = typename CallTypes<CallArgs...>::template Params<BaseParams...>;

                // clang doesn't support nested template class explicit specialization o_O
                template<class Supplier, ResultMode>
                struct Selector {  };
                template<class Supplier>
                struct Selector<Supplier, ResultMode::LAST>
                {
                    using Caller = typename Supplier::template Call<Executor>;
                };
                template<class Supplier>
                struct Selector<Supplier, ResultMode::OR>
                {
                    using Caller = typename Supplier::template CallOr<Executor>;
                };

            public:

                using Caller = typename Selector<Supplier, Result>::Caller;
            };
        };

        template<ResultMode Result = ResultMode::LAST, typename... BaseParams, class Executor, typename... CallArgs>
        constexpr auto createCall(Executor& executor, CallArgs&&... args)
        {
            using _Call = typename CallTypeSelector<Executor, BaseParams...>::template Arguments<Result, CallArgs...>::Caller;

            return _Call(executor, std::forward<CallArgs>(args)...);
        }

        template<typename Param>
        using CleanParams = std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<Param>>>;

        template<ResultMode Result, template<class> class Dispatcher, typename... Params, class Executor, typename... CallArgs>
        static constexpr decltype(auto) dispatchCall(Executor& executor, CallArgs&&... args)
        {
            auto call = createCall<Result, Params...>(executor, std::forward<CallArgs>(args)...);
            Dispatcher<CleanParams<Params>...>::Dispatch(call);
            return call.ReadResult();
        }


        template<typename Type>
        constexpr std::add_rvalue_reference_t<Type> _nullptrHack()
        {
            Type* ptr = nullptr;

            return std::move(*ptr);
        };

        template<typename Type>
        constexpr std::add_rvalue_reference_t<Type> cdeclval = _nullptrHack<Type>();

        template<typename Base, typename Target>
        struct ApplyQualifiersHelper
        {
            using type = Target;
        };

        template<typename Base, typename Target>
        using ApplyQualifiers = typename ApplyQualifiersHelper<Base, Target>::type;

        template<typename Base, typename Target>
        struct ApplyQualifiersHelper<Base*, Target>
        {
            using type = ApplyQualifiers<Base, Target>*;
        };

        template<typename Base, typename Target>
        struct ApplyQualifiersHelper<Base&, Target>
        {
            using type = ApplyQualifiers<Base, Target>&;
        };

        template<typename Base, typename Target>
        struct ApplyQualifiersHelper<const Base, Target>
        {
            using type = const ApplyQualifiers<Base, Target>;
        };

        template<template<class...> class Reflector, template<class> class Dispatcher, typename... Params>
        struct Dispatch
        {
            using Class = typename Reflector<Params...>::Class;

        private:

            template<typename... CallArgs>
            struct Executor
            {

                template<typename... Types>
                using FixedReflector = Reflector<ApplyQualifiers<Params, Types>...>;

                struct Exists
                {
                    template<class... Types>
                    static constexpr bool Exec()
                    {
                        return FixedReflector<Types...>::Exists();
                    }
                };

                struct Call
                {
                    Call(Class& object) :
                        object(object) { };

                    template<typename... Types>
                    constexpr decltype(auto) Exec(CallArgs&&... args)
                    {
                        return FixedReflector<Types...>::Call(object, std::forward<CallArgs>(args)...);
                    }

                private:

                    Class& object;
                };

                struct CallStatic
                {
                    template<class... Types>
                    static constexpr decltype(auto) Exec(CallArgs&&... args)
                    {
                        return FixedReflector<Types...>::CallStatic(std::forward<CallArgs>(args)...);
                    }
                };
            };

            template<ResultMode Result = ResultMode::LAST, class Executor, typename... CallArgs>
            static constexpr decltype(auto) DispatchCall(Executor& executor, CallArgs&&... args)
            {
                return dispatchCall<Result, Dispatcher, Params...>(executor, std::forward<CallArgs>(args)...);
            };

        public:

            static constexpr bool Exists()
            {
                constexpr typename Executor<>::Exists executor {};

                return DispatchCall<ResultMode::OR>(executor);
            }

            template<typename... CallArgs>
            static constexpr decltype(auto) Call(Class& object, CallArgs&&... args)
            {
                constexpr typename Executor<CallArgs...>::Call executor(object);

                return DispatchCall(executor, std::forward<CallArgs>(args)...);
            }

            template<ResultMode Mode = ResultMode::OR, typename... CallArgs>
            static constexpr decltype(auto) CallStatic(CallArgs&&... args)
            {
                constexpr typename Executor<CallArgs...>::CallStatic executor {};

                return DispatchCall<Mode>(executor, std::forward<CallArgs>(args)...);
            }

            template<ResultMode Mode = ResultMode::OR, typename Return, typename... CallArgs>
            static constexpr decltype(auto) CallStaticIfExists(Return&& defaultValue, CallArgs&&... args)
            {
                if constexpr(Exists())
                    return CallStatic<Mode>(std::forward<CallArgs>(args)...);
                else
                    return std::move(defaultValue);
            }
        };

    }

    template<class Reflector, typename... Args>
    struct MethodReflector
    {
        using Class = typename Reflector::Class;

        static constexpr bool Exists()
        {
            return Reflector::template Exists<Args...>();
        }

        static constexpr decltype(auto) Call(Class& object, Args&&... args)
        {
            return Reflector::Call(object, std::forward<Args>(args)...);
        }

        static constexpr decltype(auto) CallStatic(Args&&... args)
        {
            return Reflector::CallStatic(std::forward<Args>(args)...);
        }

        template<typename _Return>
        static constexpr decltype(auto) CallIfExists(Class& object, _Return&& defaultValue, Args&&... args)
        {
            if constexpr(Exists())
                return Call(object, std::forward<Args>(args)...);
            else
                return std::move(defaultValue);
        }

        template<typename _Return>
        static constexpr decltype(auto) CallStaticIfExists(_Return&& defaultValue, Args&&... args)
        {
            if constexpr(Exists())
                return CallStatic(std::forward<Args>(args)...);
            else
                return std::move(defaultValue);
        }
    };

    template<template<class...> class _Reflector, typename... Params>
    struct TemplateReflector
    {
        using Reflector = _Reflector<Params...>;
        using Class = typename Reflector::Class;

        template<typename... Args>
        static constexpr bool Exists()
        {
            return Reflector::template Exists<Args...>();
        }

        template<typename... Args>
        static constexpr decltype(auto) Call(Class& object, Args&&... args)
        {
            return Reflector::Call(object, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static constexpr decltype(auto) CallStatic(Args&&... args)
        {
            return Reflector::CallStatic(std::forward<Args>(args)...);
        }

        template<typename... Args>
        using Arguments = MethodReflector<Reflector, Args...>;
        template<template<class> class Dispatcher, typename... Args>
        using DispatchArguments = typename detail::Dispatch<Arguments, Dispatcher, Args...>;

        using Reflect = Arguments<>;
    };

    template<template<class...> class Reflector, typename... Args>
    struct ArgumentReflector
    {
        template<typename... Params>
        using Class = typename Reflector<Params...>::Class;

        template<typename... Params>
        static constexpr bool Exists()
        {
            return Reflector<Params...>::template Exists<Args...>();
        }

        template<typename... Params>
        static constexpr decltype(auto) Call(Class<Params...>& object, Args&&... args)
        {
            return Reflector<Params...>::template Call(object, std::forward<Args>(args)...);
        }

        template<typename... Params>
        static constexpr decltype(auto) CallStatic(Args&&... args)
        {
            return Reflector<Params...>::template CallStatic(std::forward<Args>(args)...);
        }

        template<typename... Params>
        using Templates = MethodReflector<Reflector<Params...>, Args...>;
        template<template<class> class Dispatcher, typename... Params>
        using DispatchTemplates = typename detail::Dispatch<Templates, Dispatcher, Params...>;

        using Reflect = Templates<>;
    };

    template<template<class, class...> class _Reflector, class Class>
    struct _ClassReflector
    {
        template<typename... Params>
        using Reflector = _Reflector<Class, Params...>;

        template<typename... Params, typename... Args>
        static constexpr bool Exists(Args&&...) // todo: hack
        {
            return Reflector<Params...>::template Exists<Args...>();
        }

        template<typename... Params, typename... Args>
        static constexpr decltype(auto) Call(Class& object, Args&&... args)
        {
            return Reflector<Params...>::template Call(object, std::forward<Args>(args)...);
        }

        template<typename... Params, typename... Args>
        static constexpr decltype(auto) CallStatic(Args&&... args)
        {
            return Reflector<Params...>::template CallStatic(std::forward<Args>(args)...);
        }

        using Reflect = MethodReflector<Reflector<>>;
        template<typename... Params>
        using Templates = TemplateReflector<Reflector, Params...>;
        template<typename... Args>
        using Arguments = ArgumentReflector<Reflector, Args...>;

        template<template<class> class Dispatcher, typename... Args>
        using DispatchArguments = typename Templates<>::template DispatchArguments<Dispatcher, Args...>;
        template<template<class> class Dispatcher, typename... Params>
        using DispatchTemplates = typename Arguments<>::template DispatchTemplates<Dispatcher, Params...>;
    };
}

#define DEFINE_METHOD_REFLECTION_NAME(NAME, METHOD, CONST) \
namespace Reflect::NAME##_ \
{ \
\
    template<class _Class, typename... Params> \
    struct BaseReflector \
    { \
        using Class = _Class; \
    private: \
\
        template<typename... Args> \
        struct ExistsHelper \
        { \
            template<class _Class2> \
            static constexpr auto ExistsImpl(uint32) -> decltype(std::declval<_Class2>().template METHOD<Params...>(std::declval<Args>()...), bool()) \
            { \
                return true; \
            } \
        \
            template<class _Class2> \
            static constexpr bool ExistsImpl(uint64) \
            { \
                return false; \
            } \
        public: \
        \
            static constexpr bool Exists() \
            { \
                return ExistsImpl<Class>(uint32()); \
            } \
        }; \
\
    public: \
\
        template<typename... Args> \
        static constexpr bool Exists() \
        { \
             return ExistsHelper<Args...>::Exists(); \
        } \
\
        template<typename... Args> \
        static constexpr decltype(auto) Call(Class& object, Args&&... args) \
        { \
            return object.template METHOD<Params...>(std::forward<Args>(args)...); \
        } \
\
        template<typename... Args> \
        static constexpr decltype(auto) CallStatic(Args&&... args) \
        { \
            return Class::template METHOD<Params...>(std::forward<Args>(args)...); \
        } \
    }; \
\
    template<class _Class> \
    struct BaseReflector<_Class> \
    { \
        using Class = _Class; \
\
    private: \
\
        template<typename... Args> \
        struct ExistsHelper \
        { \
\
            template<class _Class2> \
            static constexpr auto ExistsImpl(uint32) -> decltype(std::declval<_Class2>().METHOD(std::declval<Args>()...), bool()) \
            { \
                return true; \
            } \
\
            template<class _Class2> \
            static constexpr bool ExistsImpl(uint64) \
            { \
                return false; \
            } \
        public: \
                \
            static constexpr bool Exists(void) \
            { \
                return ExistsImpl<Class>(uint32()); \
            } \
        }; \
\
    public: \
\
        template<typename... Args> \
        static constexpr bool Exists() \
        { \
             return ExistsHelper<Args...>::Exists(); \
        } \
\
        template<typename... Args> \
        static constexpr decltype(auto) Call(Class& object, Args&&... args) \
        { \
            if constexpr(Exists<Args...>()) \
                return object.METHOD(std::forward<Args>(args)...); \
        } \
\
        template<typename... Args> \
        static constexpr decltype(auto) CallStatic(Args&&... args) \
        { \
            if constexpr(Exists<Args...>()) \
                return Class::METHOD(std::forward<Args>(args)...); \
        } \
    }; \
\
    template<class _Class> \
    using Class = _ClassReflector<BaseReflector, CONST _Class>; \
\
}

#define DEFINE_METHOD_REFLECTION(METHOD, CONST) DEFINE_METHOD_REFLECTION_NAME(METHOD, METHOD, CONST)

#define _DEFINE_METHOD_SIGNATURE(NAME, SIGNATURE) \
namespace Reflect::NAME##_ \
{ \
     template<class _Class> \
     using Signature = typename Class<_Class>::template SIGNATURE; \
}

#define DEFINE_METHOD_REFLECTION_SIGNATURE_NAME(NAME, METHOD, SIGNATURE, CONST) \
DEFINE_METHOD_REFLECTION_NAME(NAME, METHOD, CONST) \
_DEFINE_METHOD_SIGNATURE(NAME, SIGNATURE)

#define DEFINE_METHOD_REFLECTION_SIGNATURE(METHOD, SIGNATURE, CONST) DEFINE_METHOD_REFLECTION_SIGNATURE_NAME(METHOD, METHOD, SIGNATURE, CONST)

// whoever defined this...
#define CONST const