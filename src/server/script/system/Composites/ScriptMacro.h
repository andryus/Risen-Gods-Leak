#pragma once
#include "ScriptGenerator.h"

namespace script
{

    template<class MacroT>
    class Macro :
        public Generator<MacroT>
    { 
        using Generator<MacroT>::Generator;
    };

/******************************
* Base
******************************/

#define _SCRIPT_MACRO_BASE(NAMESPACE, NAME, COMPOSITE_N, DATA, TEMPL)       \
    namespace NAMESPACE::Bind                                               \
    {                                                                       \
        TEMPL                                                               \
        class NAME                                                          \
        {                                                                   \
        public:                                                             \
                                                                            \
            auto ToGenerator() const                                        \
            {                                                               \
                return script::Macro<NAME>(*this);                          \
            }                                                               \
                                                                            \
            template<class Composite>                                       \
            auto operator()(Composite&& COMPOSITE_N) const;                 \
                                                                            \
            DATA                                                            \
        };                                                                  \
    }                                                                       \

#define _SCRIPT_MACRO_IMPL(NAMESPACE, NAME, COMPOSITE_N)                    \
    template<class Composite>                                               \
    auto NAMESPACE::Bind::NAME::operator()(Composite&& COMPOSITE_N) const   \


#define SCRIPT_MACRO(NAMESPACE, NAME, COMPOSITE_N)                          \
    _SCRIPT_MACRO_BASE(NAMESPACE, NAME, COMPOSITE_N,,)                      \
                                                                            \
    namespace NAMESPACE                                                     \
    {                                                                       \
        constexpr auto NAME =                                               \
            script::makeGenerator<script::Macro>(Bind::NAME());             \
    }                                                                       \
                                                                            \
    _SCRIPT_MACRO_IMPL(NAMESPACE, NAME, COMPOSITE_N)                        \

/******************************
* Data
******************************/

#define SCRIPT_MACRO_DATA(NAMESPACE, NAME, COMPOSITE_N, DATA_T, DATA_N)     \
    _SCRIPT_MACRO_BASE(NAMESPACE, NAME, COMPOSITE_N, DATA_T DATA_N;,)       \
                                                                            \
    namespace NAMESPACE                                                     \
    {                                                                       \
        struct NAME : public script::Macro<Bind::NAME>                      \
        {                                                                   \
            using Base = script::Macro<Bind::NAME>;                         \
            constexpr NAME(DATA_T DATA_N)                                   \
                : Base(Bind::NAME{std::move(DATA_N)}) {};                   \
                                                                            \
            template<class... Values>                                       \
            static auto Multi(Values... values)                             \
            {                                                               \
                return script::makeRepeat                                   \
                     (Bind::NAME{DATA_T(values)}...);                       \
            }                                                               \
        };                                                                  \
    }                                                                       \
                                                                            \
    _SCRIPT_MACRO_IMPL(NAMESPACE, NAME, COMPOSITE_N)                        \

/******************************
* Template
******************************/

#define _SCRIPT_MACRO_TEMPL template<class Template>

#define _SCRIPT_MACRO_TEMPL_IMPL(NAMESPACE, NAME, COMPOSITE_N)                       \
    template<class Template> template<class Composite>                               \
    auto NAMESPACE::Bind::NAME<Template>::operator()(Composite&& COMPOSITE_N) const  \

#define SCRIPT_MACRO_TEMPLATE(NAMESPACE, NAME, COMPOSITE_N, DATA_N)         \
    _SCRIPT_MACRO_BASE(                                                     \
        NAMESPACE, NAME, COMPOSITE_N, Template DATA_N;, _SCRIPT_MACRO_TEMPL)\
                                                                            \
    namespace NAMESPACE                                                     \
    {                                                                       \
        template<class Template>                                            \
        auto NAME(Template DATA_N)                                          \
        {                                                                   \
            return script::Macro<Bind::NAME<Template>>({DATA_N});           \
        }                                                                   \
    }                                                                       \
                                                                            \
    _SCRIPT_MACRO_TEMPL_IMPL(NAMESPACE, NAME, COMPOSITE_N)                  \

}
