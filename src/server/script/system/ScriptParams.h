#pragma once
#include <tuple>
#include <vector>
#include "ScriptTarget.h"
#include "ScriptLogger.h"
#include "Utilities/Span.h"

namespace script
{
    class BaseTarget;
    class ScriptLogger;

    // TODO: belongs to ScriptHook.h, but include-mess
    struct DefaultHook
    {
        template<class Composite>
        void operator()(const Composite& composite) const {}
    };

    /*************************************
    * Params
    **************************************/

    struct NoParam 
    {
        NoParam() = default;

        template<class Any>
        NoParam(Any&& any) {}
    };

    template<class ContextT>
    constexpr bool HasParam = !std::is_same_v<std::decay_t<typename ContextT::Param>, NoParam>;

    template<class TargetT, class ParamT, class DebugHook = DefaultHook>
    struct Context
    {
        using Target = TargetT;
        using Param = ParamT;
        using HookT = DebugHook;
    };

    template<class ContextT>
    struct Params
    {
        using Target = typename ContextT::Target;
        using Param = typename ContextT::Param;
        using HookT = typename ContextT::HookT;

        using Context = ContextT;

        explicit operator bool() const { return target; }

        Target* target;
        Param param;
        HookT hook;
        ScriptLogger* logger;
    };
    
    class InvokationCtx;

    template<class Target, class ParamT, class HookT = DefaultHook>
    auto makeParams(Target* target, ParamT param, ScriptLogger* logger = nullptr, HookT hook = HookT())
    {
        using ContextT = Context<std::decay_t<Target>, std::decay_t<ParamT>, std::decay_t<HookT>>;

        static_assert(!std::is_same_v<std::decay_t<Target>, InvokationCtx>, "How?");

        if (!logger && target)
            logger = getLoggerFromTarget(target);

        return Params<ContextT>{target, std::forward<ParamT>(param), hook, logger};
    }

    template<class Target, class ParamT, class Context>
    auto makeParams(Target* target, ParamT param, Params<Context> params)
    {
        if constexpr(std::is_same_v<std::decay_t<ParamT>, NoParam>)
            return makeParams(target, params.param, params.logger, params.hook);
        else
            return makeParams(target, param, params.logger, params.hook);
    }

    /*************************************
    * StoredParam
    **************************************/
    template<class Target>
    struct StoredTarget;

    template<class Param>
    struct StoredParam
    {
        StoredParam() = default;
        StoredParam(Param param) : 
            param(param) {}

        decltype(auto) Load(ScriptLogger* logger) const { return param; }

    private:

        Param param;
    };

    template<class Param>
    struct StoredParam<Param*>
    {
        StoredParam() = default;
        StoredParam(Param* param) : 
            target(param) {}

        decltype(auto) Load(ScriptLogger* logger) const 
        {
            if (auto out = target.Load())
                return out;
            else if(logger)
                logger->AddError("Stored param doesn't exist: " + target.Print());

            return decltype(target.Load())();
        }

    private:
        StoredTarget<Param> target;
    };

    template<class Param>
    struct StoredParam<Param&> : 
        public StoredParam<Param*>
    {
        StoredParam() = default;
        StoredParam(Param& param) :
            StoredParam<Param*>(&param) {}

        using StoredParam<Param*>::Load;
    };

    /*************************************
    * List
    **************************************/

    template<class Param>
    using List = std::vector<Param>;
    template<class Param>
    using StaticList = span<Param>;
    template<class Param, size_t Size>
    using Array = std::array<Param, Size>;
    template<class Param, size_t Size>
    using ArrayRef = const Param(&)[Size];

    template<class Param>
    struct ElementType
    {
        using type = Param;
    };

    template<class Param>
    struct ElementType<Param&>
    {
        using type = std::reference_wrapper<Param>;
    };

    template<class Param>
    constexpr bool IsList = false;
    template<class Param>
    constexpr bool IsList<List<Param>> = true;
    template<class Param>
    constexpr bool IsList<StaticList<Param>> = true;
    template<class Param, size_t Size>
    constexpr bool IsList<Array<Param, Size>> = true;
    template<class Param, size_t Size>
    constexpr bool IsList<ArrayRef<Param, Size>> = true;

    template<class Param>
    constexpr uint32 ArraySize = 0;
    template<class Param, size_t Size>
    constexpr uint32 ArraySize<Array<Param, Size>> = Size;
    template<class Param, size_t Size>
    constexpr uint32 ArraySize<ArrayRef<Param, Size>> = Size;

    template<class Target, class Type, class Functor>
    decltype(auto) applyForReturnValue(Type returnValue, Functor functor)
    {
        if constexpr(IsList<Type> && !IsList<Target>)
        {
            constexpr uint32 arraySize = ArraySize<Type>;

            using Output = decltype(functor(returnValue[0]));

            // @todo: configurable - DynamicBinding etc.. should not accumulate, but return array of results
            if constexpr(std::is_same_v<Output, bool>)
            {
                bool accumulated = true;

                for (const auto& element : returnValue)
                {
                    decltype(auto) result = functor(element);

                    accumulated &= static_cast<bool>(result);
                }

                return accumulated;
            }
            else if constexpr(arraySize != 0)
            {
                Array<Output, arraySize> output;

                for (uint32 i = 0; i < arraySize; i++)
                {
                    output[i] = functor(returnValue[i]);
                }

                return output;
            }
            else
            {
                for (const auto& element : returnValue)
                {
                    functor(element);
                }

                return Output();
            }
        }
        else
            return functor(returnValue);
    }

    /*************************************
    * String-conversion
    **************************************/

    template<class Data, typename Enable = void>
    struct FromString
    {
        Data operator()(std::string_view string, script::Scriptable&) const
        {
            return {};
        }
    };

    template<class Data>
    auto fromString(std::string_view string, script::Scriptable& ctx)
    {
        return FromString<Data>()(string, ctx);
    }

    template<class Data>
    struct FromString<Data, std::enable_if_t<std::is_arithmetic_v<Data>>>
    {
        Data operator()(std::string_view string, script::Scriptable&) const
        {
            return std::strtol(string.data(), nullptr, 10);
        }
    };

}
