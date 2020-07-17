#pragma once
#include "Define.h"
#include <boost/preprocessor/punctuation/comma.hpp>

/***********************************
* CONTEXT
***********************************/

namespace script
{

#define SCRIPT_DATA_FUNCS(ELEMENT, PRE, POST, PREFIX, FUNC)            \
    template<size_t Size>                                              \
    PRE auto PREFIX##Multi(script::ArrayRef<ELEMENT, Size> array) POST \
    {                                                                  \
        return FUNC(script::makeArray(array));                         \
    }                                                                  \
                                                                       \
    template<class... Types>                                           \
    PRE auto PREFIX##Multi(Types... values) POST                       \
    {                                                                  \
        return FUNC(script::makeArray<ELEMENT>(values...));            \
    }                                                                  \
                                                                       \
    template<size_t Size>                                              \
    PRE auto PREFIX##OneOf(script::ArrayRef<ELEMENT, Size> array) POST \
    {                                                                  \
        return FUNC(script::makeOneOf(array));                         \
    }                                                                  \
                                                                       \
    template<class... Types>                                           \
    PRE auto PREFIX##OneOf(Types... values) POST                       \
    {                                                                  \
        return FUNC(script::makeOneOf<ELEMENT>(values...));            \
    }                                                                  \
                                                                       \
    template<class Getter>                                             \
    PRE auto PREFIX##OneOfFrom(script::Generator<Getter> getter) POST  \
    {                                                                  \
        return FUNC(script::OneOfComposite(getter.Extract()));         \
    }                                                                  \

#define _SCRIPT_CTX(NAMESPACE, NAME, RETURN, ME, DATA_T, DATA_N, PARAM_T, PARAM_N, STATE_T, STATE_N, EXT)  \
    namespace NAMESPACE::Bind { struct NAME; }                                                             \
    namespace NAMESPACE::Ctx                                                                               \
    {                                                                                                      \
        struct NAME                                                                                        \
        {                                                                                                  \
            static constexpr auto DataName = #DATA_N;                                                      \
            static constexpr auto ParamName = #PARAM_N;                                                    \
            static constexpr auto StateName = #STATE_N;                                                    \
                                                                                                           \
            using Me = ME;                                                                                 \
            using Return = RETURN;                                                                         \
            using Param = PARAM_T;                                                                         \
            using Data = DATA_T;                                                                           \
            using State = STATE_T;                                                                         \
                                                                                                           \
            NAME(const NAME&) = delete;                                                                    \
            template<class Binding>                                                                        \
            NAME(Me& me, Param PARAM_N, Binding& binding, script::ScriptLogger* logger) :                  \
                me(me), DATA_N(binding.data), PARAM_N(PARAM_N), STATE_N(binding.state),                    \
                logger(logger), _binding(reinterpret_cast<Bind::NAME&>(binding)) {}                        \
                                                                                                           \
                                                                                                           \
            template<class Other>                                                                          \
            auto ConvertTo() const                                                                         \
            {                                                                                              \
                return Other(me, PARAM_N, _binding, logger);                                               \
            }                                                                                              \
                                                                                                           \
            Return operator()();                                                                           \
                                                                                                           \
            EXT                                                                                            \
                                                                                                           \
            bool _canUndo = true;                                                                          \
            Bind::NAME& _binding;                                                                          \
        private:                                                                                           \
                                                                                                           \
            template<class Module>                                                                         \
            Module& Module()                                                                               \
            {                                                                                              \
                Module* module = script::castSourceTo<Module>(me);                                         \
                ASSERT(module);                                                                            \
                                                                                                           \
                return *module;                                                                            \
            }                                                                                              \
                                                                                                           \
            template<class Composite>                                                                      \
            void AddUndoable(Composite composite)                                                          \
            {                                                                                              \
                if (_canUndo)                                                                              \
                    script::BindingUndo::Add<PARAM_T>(composite.Extract(), me, PARAM_N);                   \
            }                                                                                              \
                                                                                                           \
            template<class Composite, class Target>                                                        \
            void AddUndoableFor(Composite composite, Target& target)                                       \
            {                                                                                              \
                if (_canUndo)                                                                              \
                    script::BindingUndo::AddFor<PARAM_T>(composite.Extract(), me, target, PARAM_N);        \
            }                                                                                              \
                                                                                                           \
            template<class Composite, class Param>                                                         \
            void AddUndoableWith(Composite composite, Param param)                                         \
            {                                                                                              \
                if (_canUndo)                                                                              \
                    script::BindingUndo::Add<Param>(composite.Extract(), me, param);                       \
            }                                                                                              \
                                                                                                           \
            [[maybe_unused]] Me& me;                                                                       \
            [[maybe_unused]] Param PARAM_N;                                                                \
            [[maybe_unused]] const DATA_T DATA_N;                                                          \
            [[maybe_unused]] STATE_T STATE_N;                                                              \
            [[maybe_unused]] script::ScriptLogger* logger;                                                 \
                                                                                                           \
        };                                                                                                 \
    }                                                                                                      \

/***********************************
* COMPOSITE
***********************************/

#define _SCRIPT_COMPOSITE_BASE(GENERATOR, NAMESPACE, RETURN, NAME, ME, PARAM, DATA_T, STATE_T)             \
    namespace NAMESPACE::Bind                                                                              \
    {                                                                                                      \
        struct NAME : public script::BindingCall<NAME, script::GENERATOR>                                  \
        {                                                                                                  \
            using Return = RETURN;                                                                         \
            using Data = DATA_T;                                                                           \
            using State = STATE_T;                                                                         \
            using Me = ME;                                                                                 \
                                                                                                           \
            static constexpr auto CompositeName = #NAME;                                                   \
            static constexpr auto ModuleName = #NAMESPACE;                                                 \
                                                                                                           \
            static_assert(!script::HasState<NAME> || !std::is_same_v<Me, script::Unbound>,                 \
                "Composite with state cannot be Unbound.");                                                \
                                                                                                           \
            constexpr NAME(DATA_T data = DATA_T{}) :                                                       \
                data(data), state() {}                                                                     \
                                                                                                           \
            auto ToGenerator() const                                                                       \
            {                                                                                              \
                return MakeGenerator(*this);                                                               \
            }                                                                                              \
                                                                                                           \
            template<class Other>                                                                          \
            static script::GENERATOR<Other> MakeGenerator(Other other)                                     \
            {                                                                                              \
                return script::makeGenerator<script::GENERATOR>(other);                                    \
            }                                                                                              \
                                                                                                           \
            template<class HookT>                                                                          \
            auto Hook(HookT hook) const                                                                    \
            {                                                                                              \
                return script::makeHookedComposite(*this, hook);                                           \
            }                                                                                              \
                                                                                                           \
            using CtxT = NAMESPACE::Ctx::NAME;                                                             \
            PARAM                                                                                          \
                                                                                                           \
            void Print(script::ScriptLogger& logger) const                                                 \
            {                                                                                              \
                script::printComposite<script::GENERATOR>(#NAMESPACE, #NAME, *this, logger);               \
            }                                                                                              \
                                                                                                           \
            DATA_T data = {};                                                                              \
            STATE_T state = {};                                                                            \
                                                                                                           \
            bool expected = true;                                                                          \
            bool canUndo = true;                                                                           \
            script::InvocationMode mode = script::InvocationMode::ALL;                                     \
            const NAME& GetFunctor() const { return *this; }                                               \
        };                                                                                                 \
    }                                                                                                      \

/***********************************
* EXTENTIONS
***********************************/

    template<class Element, class Composite, class Value1, class Value2, class... Values>
    auto makeMultiVar(Composite composite, Value1 value1, Value2 value2, Values&&... values)
    {
        const Element array[] = { Element(value1), Element(value2), Element(values)... };

        return composite.BindMulti(array);
    }

#define _SCRIPT_PARAM(GENERATOR, PARAM_T, PARAM_N)                                                         \
private:                                                                                                   \
    template<class Value>                                                                                  \
    auto BindImpl(Value value) const                                                                       \
    {                                                                                                      \
        return script::makePreparedBinding(this->GetFunctor(), value);                                     \
    }                                                                                                      \
public:                                                                                                    \
    using Param = PARAM_T;                                                                                 \
    auto Bind(PARAM_T PARAM_N) const                                                                       \
    {                                                                                                      \
        return BindImpl(script::ValueComposite<PARAM_T>(std::forward<PARAM_T>(PARAM_N)));                  \
    }                                                                                                      \
                                                                                                           \
    using ElementType = script::ElementType<PARAM_T>::type;                                                \
                                                                                                           \
    SCRIPT_DATA_FUNCS(ElementType,,const, Bind, BindImpl)                                                  \
                                                                                                           \

    struct NoData 
    {
        explicit operator uint32() const { return 0; }
    };
    struct NoState {};

    template<class ContextT>
    constexpr bool HasData = !std::is_same_v<typename ContextT::Data, NoData>;
    template<class ContextT>
    constexpr bool HasState = !std::is_same_v<typename ContextT::State, NoState>;

#define _SCRIPT_NO_STATE_T script::NoState
#define _SCRIPT_NO_DATA_T script::NoData
#define _SCRIPT_NO_PARAM_T script::NoParam

#define _SCRIPT_NO_PARAM using Param = _SCRIPT_NO_PARAM_T;

/***********************************
* CREATION
***********************************/

#define _SCRIPT_GENERATOR(GENERATOR, NAME, PARAM)        \
    struct NAME :                                        \
        public script::GENERATOR<Bind::NAME>             \
    {                                                    \
        using Return = void;                             \
                                                         \
        using script::GENERATOR<Bind::NAME>::GENERATOR;  \
        using script::GENERATOR<Bind::NAME>::GetFunctor; \
                                                         \
        PARAM                                            \
    };                                                   \


#define _SCRIPT_FACTORY_VARIABLE(NAME) \
    constexpr auto NAME = Create::NAME(Bind::NAME());

#define _SCRIPT_FACTORY_TYPE(GENERATOR, NAME, DATA_T, DATA_N, STATE_T)                                \
    struct NAME : public Create::NAME                                                                 \
    {                                                                                                 \
        using Base = Create::NAME;                                                                    \
        constexpr NAME(DATA_T DATA_N)                                                                 \
            : Base(Bind::NAME(std::move(DATA_N))) {};                                                 \
                                                                                                      \
        SCRIPT_DATA_FUNCS(DATA_T,static,,, DynamicImpl)                                               \
                                                                                                      \
                                                                                                      \
    private:                                                                                          \
                                                                                                      \
        template<class Value>                                                                         \
        static auto DynamicImpl(Value value)                                                          \
        {                                                                                             \
            return script::makeDynamicBinding<Bind::NAME>(value);                                     \
        }                                                                                             \
    public:                                                                                           \
                                                                                                      \
        template<class Getter>                                                                        \
        static auto Dynamic(script::Generator<Getter> getter)                                         \
        {                                                                                             \
            return DynamicImpl(getter.Extract());                                                     \
        }                                                                                             \
    };                                                                                                \

/***********************************
* IMPLEMENTATION
***********************************/

#ifndef __INTELLISENSE__
#define SCRIPT_COMPOSITE_IMPL(GENERATOR, NAMESPACE, NAME) \
    template struct script::GENERATOR<NAMESPACE::Bind::NAME>; \
    template struct script::BindingCall<NAMESPACE::Bind::NAME, script::GENERATOR>;                         \
    template void script::printComposite<script::GENERATOR>(std::string_view, std::string_view, const NAMESPACE::Bind::NAME&, script::ScriptLogger&); \
    template NAMESPACE::Ctx::NAME::NAME(NAMESPACE::Ctx::NAME::Me&, NAMESPACE::Ctx::NAME::Param, NAMESPACE::Bind::NAME&, script::ScriptLogger*);     \
    NAMESPACE::Ctx::NAME::Return NAMESPACE::Ctx::NAME::operator()()                                                           \

#else
#define SCRIPT_COMPOSITE_IMPL(GENERATOR, NAMESPACE, NAME) \
    NAMESPACE::Ctx::NAME::Return NAMESPACE::Ctx::NAME::operator()()
#endif

#define _SCRIPT_INLINE(GENERATOR, NAMESPACE, NAME) inline NAMESPACE::Ctx::NAME::Return NAMESPACE::Ctx::NAME::operator()()

#ifndef __INTELLISENSE__
#define _SCRIPT_NO_INLINE(GENERATOR, NAMESPACE, NAME) \
    extern template struct script::GENERATOR<NAMESPACE::Bind::NAME>;       \
    extern template struct script::BindingCall<NAMESPACE::Bind::NAME, script::GENERATOR>;   \
    extern template void script::printComposite<script::GENERATOR>(std::string_view, std::string_view, const NAMESPACE::Bind::NAME&, script::ScriptLogger&);                               \
    extern template NAMESPACE::Ctx::NAME::NAME(NAMESPACE::Ctx::NAME::Me&, NAMESPACE::Ctx::NAME::Param, NAMESPACE::Bind::NAME&, script::ScriptLogger*);                      \

#else
#define _SCRIPT_NO_INLINE(GENERATOR, NAMESPACE, NAME)
#endif

/***********************************
* DECLARATION
******************************************/


#define _SCRIPT_COMPOSITE_DEFINITION(GENERATOR, NAMESPACE, NAME, PARAM, FACTORY)    \
    namespace NAMESPACE::Create                                                     \
    {                                                                               \
        _SCRIPT_GENERATOR(GENERATOR, NAME, PARAM);                                  \
    }                                                                               \
    namespace NAMESPACE                                                             \
    {                                                                               \
        FACTORY                                                                     \
    }                                                                               \

#define _SCRIPT_COMPOSITE_DECL(GENERATOR, NAMESPACE, RETURN, NAME, ME, DATA_T, PARAM, STATE_T, FACTORY, INLINE) \
    _SCRIPT_COMPOSITE_BASE(GENERATOR, NAMESPACE, RETURN, NAME, ME, PARAM, DATA_T, STATE_T)                      \
    _SCRIPT_COMPOSITE_DEFINITION(GENERATOR, NAMESPACE, NAME, PARAM, FACTORY)                                    \
    INLINE(GENERATOR, NAMESPACE, NAME)                                                                          \

/***********************************
* DEFINITION
***********************************/


#define _SCRIPT_COMPOSITE(GENERATOR, NAMESPACE, RETURN, NAME, ME, PARAM, STATE_T, INLINE) \
    _SCRIPT_COMPOSITE_DECL(GENERATOR, NAMESPACE, RETURN, NAME, ME, _SCRIPT_NO_DATA_T, PARAM, STATE_T, _SCRIPT_FACTORY_VARIABLE(NAME), INLINE)

#define _SCRIPT_COMPOSITE_EX(GENERATOR, NAMESPACE, RETURN, NAME, ME, DATA_T, DATA_N, PARAM, STATE_T, INLINE)  \
    _SCRIPT_COMPOSITE_DECL(GENERATOR, NAMESPACE, RETURN, NAME, ME, DATA_T, PARAM, STATE_T, _SCRIPT_FACTORY_TYPE(GENERATOR, NAME, DATA_T, DATA_N, STATE_T), INLINE)


#define SCRIPT_COMPOSITE(GENERATOR, NAMESPACE, RETURN, NAME, ME,INLINE, EXT) \
    _SCRIPT_CTX(NAMESPACE, NAME, RETURN, ME, _SCRIPT_NO_DATA_T, _noData, _SCRIPT_NO_PARAM_T,_noParam, _SCRIPT_NO_STATE_T, _noState, EXT) \
    _SCRIPT_COMPOSITE(GENERATOR, NAMESPACE, RETURN, NAME, ME, _SCRIPT_NO_PARAM, _SCRIPT_NO_STATE_T, INLINE)

#define SCRIPT_COMPOSITE1(GENERATOR, NAMESPACE, RETURN, NAME, ME, PARAM_T, PARAM_N, INLINE, EXT)  \
    _SCRIPT_CTX(NAMESPACE, NAME, RETURN, ME, _SCRIPT_NO_DATA_T, _noData, PARAM_T, PARAM_N, _SCRIPT_NO_STATE_T, _noState, EXT) \
    _SCRIPT_COMPOSITE(GENERATOR, NAMESPACE, RETURN, NAME, ME, _SCRIPT_PARAM(GENERATOR, PARAM_T, PARAM_N), _SCRIPT_NO_STATE_T, INLINE)

#define SCRIPT_COMPOSITE2(GENERATOR, NAMESPACE, RETURN, NAME, ME, STATE_T, STATE_N, INLINE, EXT)    \
    _SCRIPT_CTX(NAMESPACE, NAME, RETURN, ME, _SCRIPT_NO_DATA_T, _noData, _SCRIPT_NO_PARAM_T, _noParam, STATE_T&, STATE_N, EXT) \
    _SCRIPT_COMPOSITE(GENERATOR, NAMESPACE, RETURN, NAME, ME, _SCRIPT_NO_PARAM, STATE_T, INLINE)     

#define SCRIPT_COMPOSITE3(GENERATOR, NAMESPACE, RETURN, NAME, ME, PARAM_T, PARAM_N, STATE_T, STATE_N, INLINE, EXT)  \
    _SCRIPT_CTX(NAMESPACE, NAME, RETURN, ME, _SCRIPT_NO_DATA_T, _noData, PARAM_T ,PARAM_N, STATE_T&, STATE_N, EXT) \
    _SCRIPT_COMPOSITE(GENERATOR, NAMESPACE, RETURN, NAME, ME, _SCRIPT_PARAM(GENERATOR, PARAM_T, PARAM_N), STATE_T, INLINE) 


#define SCRIPT_COMPOSITE_EX(GENERATOR, NAMESPACE, RETURN, NAME, ME, DATA_T, DATA_N, INLINE, EXT)  \
    _SCRIPT_CTX(NAMESPACE, NAME, RETURN, ME, DATA_T, DATA_N, _SCRIPT_NO_PARAM_T, _noParam, _SCRIPT_NO_STATE_T, _noState, EXT) \
    _SCRIPT_COMPOSITE_EX(GENERATOR, NAMESPACE, RETURN, NAME, ME, DATA_T, DATA_N, _SCRIPT_NO_PARAM, _SCRIPT_NO_STATE_T, INLINE)

#define SCRIPT_COMPOSITE_EX1(GENERATOR, NAMESPACE, RETURN, NAME, ME, DATA_T, DATA_N, PARAM_T, PARAM_N, INLINE, EXT)  \
    _SCRIPT_CTX(NAMESPACE, NAME, RETURN, ME, DATA_T, DATA_N, PARAM_T, PARAM_N, _SCRIPT_NO_STATE_T, _noState, EXT) \
    _SCRIPT_COMPOSITE_EX(GENERATOR, NAMESPACE, RETURN, NAME, ME, DATA_T, DATA_N, _SCRIPT_PARAM(GENERATOR, PARAM_T, PARAM_N), _SCRIPT_NO_STATE_T, INLINE)     
#define SCRIPT_COMPOSITE_EX2(GENERATOR, NAMESPACE, RETURN, NAME, ME, DATA_T, DATA_N, STATE_T, STATE_N, INLINE, EXT)  \
    _SCRIPT_CTX(NAMESPACE, NAME, RETURN, ME, DATA_T, DATA_N, _SCRIPT_NO_PARAM_T, _noParam, STATE_T&, STATE_N, EXT) \
    _SCRIPT_COMPOSITE_EX(GENERATOR, NAMESPACE, RETURN, NAME, ME, DATA_T, DATA_N, _SCRIPT_NO_PARAM, STATE_T, INLINE)    

#define SCRIPT_COMPOSITE_EX3(GENERATOR, NAMESPACE, RETURN, NAME, ME, DATA_T, DATA_N, PARAM_T, PARAM_N, STATE_T, STATE_N, INLINE, EXT)  \
    _SCRIPT_CTX(NAMESPACE, NAME, RETURN, ME, DATA_T, DATA_N, PARAM_T, PARAM_N, STATE_T&, STATE_N, EXT) \
    _SCRIPT_COMPOSITE_EX(GENERATOR, NAMESPACE, RETURN, NAME, ME, DATA_T, DATA_N, _SCRIPT_PARAM(GENERATOR, PARAM_T, PARAM_N), STATE_T, INLINE)   

}
