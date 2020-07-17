#pragma once

/**************************************
* CONTEXT
**************************************/

#define _SCRIPT_TEMPLATE_CTX(NAMESPACE, RETURN, NAME, TEMPL, DATA_T, DATA_N, PARAM_N)         \
                                                                                              \
    namespace NAMESPACE::Bind                                                                 \
    {                                                                                         \
        template<class TEMPL>                                                                 \
        struct NAME;                                                                          \
    }                                                                                         \
                                                                                              \
    namespace NAMESPACE::Ctx                                                                  \
    {                                                                                         \
        template<class TEMPL, class MeT, class ParamT>                                        \
        struct NAME                                                                           \
        {                                                                                     \
                                                                                              \
            static constexpr auto DataName = #DATA_N;                                         \
            static constexpr auto ParamName = #PARAM_N;                                       \
                                                                                              \
            using Me = MeT;                                                                   \
            using Param = ParamT;                                                             \
            using Return = RETURN;                                                            \
            using Data = DATA_T;                                                              \
            using State = script::NoState;                                                    \
                                                                                              \
            bool HadSuccess() const { return true; }                                          \
                                                                                              \
            template<class Binding>                                                           \
            NAME(Me& me, Param PARAM_N, Binding& binding, script::ScriptLogger* logger) :     \
                me(me), DATA_N(binding.data), _binding(binding), PARAM_N(PARAM_N) {}          \
                                                                                              \
            Return operator()();                                                              \
                                                                                              \
            bool _canUndo = true;                                                             \
            [[maybe_unused]] Bind::NAME<TEMPL>& _binding;                                     \
        private:                                                                              \
                                                                                              \
            [[maybe_unused]] Me& me;                                                          \
            [[maybe_unused]] Param PARAM_N;                                                   \
            [[maybe_unused]] const DATA_T DATA_N;                                             \
        };                                                                                    \
    };                                                                                        \

/**************************************
* COMPOSITE
**************************************/


#define _SCRIPT_TEMPLATE_COMP(GENERATOR, NAMESPACE, NAME, TEMPL, HAS_ME, DATA_T, HAS_PARAM, PARAM_N)     \
                                                                                                         \
    namespace NAMESPACE::Bind                                                                            \
    {                                                                                                    \
        template<class TEMPL>                                                                            \
        struct NAME                                                                                      \
        {                                                                                                \
            using Me = script::Unbound;                                                                  \
            using Param = script::NoParam;                                                               \
            using Data = DATA_T;                                                                         \
            using State = script::NoState;                                                               \
                                                                                                         \
            constexpr NAME(DATA_T data = DATA_T{}) :                                                     \
                data(data) {}                                                                            \
                                                                                                         \
            struct CtxT                                                                                  \
            {                                                                                            \
                static constexpr auto ParamName = #PARAM_N;                                              \
            };                                                                                           \
                                                                                                         \
            static constexpr auto ModuleName = #NAMESPACE;                                               \
            static constexpr auto CompositeName = #NAME;                                                 \
                                                                                                         \
            auto ToGenerator() const                                                                     \
            {                                                                                            \
                return MakeGenerator(*this);                                                             \
            }                                                                                            \
                                                                                                         \
            template<class Other>                                                                        \
            static script::GENERATOR<Other> MakeGenerator(Other other)                                   \
            {                                                                                            \
                return script::makeGenerator<script::GENERATOR>(other);                                  \
            }                                                                                            \
                                                                                                         \
            template<class Context>                                                                      \
            auto Call(script::Params<Context> params)                                                    \
            {                                                                                            \
                using Me = std::conditional_t<HAS_ME, typename Context::Target, script::Unbound>;        \
                using Param = std::conditional_t<HAS_PARAM, typename Context::Param, script::NoParam>;   \
                                                                                                         \
                using Ctx = Ctx::NAME<TEMPL, Me, Param>;                                                 \
                return script::doBindingCall<NAME, Ctx, script::GENERATOR>(*this, params);               \
            }                                                                                            \
                                                                                                         \
            void Print(script::ScriptLogger& logger) const                                               \
            {                                                                                            \
                script::printComposite<script::GENERATOR>(#NAMESPACE, #NAME, *this, logger);             \
            }                                                                                            \
                                                                                                         \
            DATA_T data;                                                                                 \
            State state;                                                                                 \
            bool canUndo = true;                                                                         \
            bool expected = true;                                                                        \
        };                                                                                               \
    }                                                                                                    \
                                                                                                         \

/***********************************
* DECLARATION
***********************************/

#define _SCRIPT_TEMPLATE_BASE(GENERATOR, NAMESPACE, RETURN, NAME, TEMPL, HAS_ME, DATA_T, DATA_N, HAS_PARAM, PARAM_N)  \
    _SCRIPT_TEMPLATE_CTX(NAMESPACE, RETURN, NAME, TEMPL, DATA_T, DATA_N, PARAM_N)                                     \
    _SCRIPT_TEMPLATE_COMP(GENERATOR, NAMESPACE, NAME, TEMPL, HAS_ME, DATA_T, HAS_PARAM, PARAM_N)                      \

/***********************************
* CREATE
***********************************/

#define _SCRIPT_TEMPLATE_FACTORY_FUNC(GENERATOR, NAMESPACE, NAME, TEMPL, DATA_T, DATA_N)  \
    namespace NAMESPACE                                                                   \
    {                                                                                     \
        template<class TEMPL>                                                             \
        constexpr auto NAME(DATA_T DATA_N)                                                \
        {                                                                                 \
            return script::GENERATOR<Bind::NAME<TEMPL>>(DATA_N);                          \
        }                                                                                 \
    }                                                                                     \

#define _SCRIPT_TEMPLATE_FACTORY_VAR(GENERATOR, NAMESPACE, NAME, BIND_NAME)   \
    namespace NAMESPACE                                                       \
    {                                                                         \
        constexpr auto NAME = script::GENERATOR<Bind::BIND_NAME<void>>();     \
    }                                                                         \

/***********************************
* IMPLEMENTATION
***********************************/

#define _SCRIPT_TEMPLATE_IMPL(NAMESPACE, NAME, TEMPL) template<class TEMPL, class Me, class Param> \
    inline typename NAMESPACE::Ctx::NAME<TEMPL, Me, Param>::Return NAMESPACE::Ctx::NAME<TEMPL, Me, Param>::operator()()

/***********************************
* DEFINITION
***********************************/

#define SCRIPT_TEMPLATE_DATA(GENERATOR, NAMESPACE, RETURN, NAME, TEMPL, DATA_T, DATA_N)                                   \
    _SCRIPT_TEMPLATE_BASE(GENERATOR, NAMESPACE, RETURN, NAME, TEMPL, false, DATA_T, DATA_N, false, _noParam)              \
    _SCRIPT_TEMPLATE_FACTORY_FUNC(GENERATOR, NAMESPACE, NAME, TEMPL, DATA_T, DATA_N)                                      \
    _SCRIPT_TEMPLATE_IMPL(NAMESPACE, NAME, TEMPL)                                                                         \

#define SCRIPT_TEMPLATE_PARAM(GENERATOR, NAMESPACE, RETURN, NAME, BIND_NAME)                                              \
    _SCRIPT_TEMPLATE_BASE(GENERATOR, NAMESPACE, RETURN, BIND_NAME, _t, false, script::NoData, _noData, true, param)       \
    _SCRIPT_TEMPLATE_FACTORY_VAR(GENERATOR, NAMESPACE, NAME, BIND_NAME)                                                   \
    _SCRIPT_TEMPLATE_IMPL(NAMESPACE, BIND_NAME, TEMPL)                                                                    \

#define SCRIPT_TEMPLATE_DATA_PARAM(GENERATOR, NAMESPACE, NAME, TEMPL, DATA_T, DATA_N, PARAM_T, PARAM_N)                   \
    _SCRIPT_TEMPLATE_BASE(GENERATOR, NAMESPACE, TEMPL, NAME, TEMPL, false, DATA_T, DATA_N, true, PARAM_N)                 \
    _SCRIPT_TEMPLATE_FACTORY_FUNC(GENERATOR, NAMESPACE, NAME, TEMPL, DATA_T, DATA_N)                                      \
    _SCRIPT_TEMPLATE_IMPL(NAMESPACE, NAME, TEMPL)                                                                         \



#define SCRIPT_TEMPLATE_ME(GENERATOR, NAMESPACE, RETURN, NAME, BIND_NAME)                                                 \
    _SCRIPT_TEMPLATE_BASE(GENERATOR, NAMESPACE, RETURN, BIND_NAME, _t, true, script::NoData, _noData, false, _noParam)    \
    _SCRIPT_TEMPLATE_FACTORY_VAR(GENERATOR, NAMESPACE, NAME, BIND_NAME)                                                   \
    _SCRIPT_TEMPLATE_IMPL(NAMESPACE, BIND_NAME, TEMPL)                                                                    \

#define SCRIPT_TEMPLATE_ME_PARAM(GENERATOR, NAMESPACE, RETURN, NAME, BIND_NAME)                                           \
    _SCRIPT_TEMPLATE_BASE(GENERATOR, NAMESPACE, RETURN, BIND_NAME, _t, true, script::NoData, _noData, true, param)        \
    _SCRIPT_TEMPLATE_FACTORY_VAR(GENERATOR, NAMESPACE, NAME, BIND_NAME)                                                   \
    _SCRIPT_TEMPLATE_IMPL(NAMESPACE, BIND_NAME, TEMPL)                                                                    \

#define SCRIPT_TEMPLATE_ME_DATA(GENERATOR, NAMESPACE, RETURN, NAME, TEMPL, DATA_T, DATA_N)                                \
    _SCRIPT_TEMPLATE_BASE(GENERATOR, NAMESPACE, RETURN, NAME, TEMPL, true, DATA_T, DATA_N, false, _noParam)               \
    _SCRIPT_TEMPLATE_FACTORY_FUNC(GENERATOR, NAMESPACE, NAME, TEMPL, DATA_T, DATA_N)                                      \
    _SCRIPT_TEMPLATE_IMPL(NAMESPACE, NAME, TEMPL)                                                                         \

#define SCRIPT_TEMPLATE_FULL(GENERATOR, NAMESPACE, RETURN, NAME, TEMPL, DATA_T, DATA_N, PARAM_T, PARAM_N)   \
    _SCRIPT_TEMPLATE_BASE(GENERATOR, NAMESPACE, RETURN, NAME, TEMPL, true, DATA_T, DATA_N, true, PARAM_N)   \
    _SCRIPT_TEMPLATE_FACTORY_FUNC(GENERATOR, NAMESPACE, NAME, TEMPL, DATA_T, DATA_N)                        \
    _SCRIPT_TEMPLATE_IMPL(NAMESPACE, NAME, TEMPL)                                                           \

    