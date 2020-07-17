#pragma once
#include "ScriptParams.h"
#include "ScriptGenerator.h"

namespace script
{

    template<class ModifierT, bool isMe>
    struct Modifier : public Generator<ModifierT>
    {
        using Generator<ModifierT>::Generator;

        template<class Result, class Context, class Ctx>
        static auto ValidateResult(Result result, const Ctx& ctx, Params<Context> params)
        {
            return result;
        }

        template<class Ctx, class Context>
        static auto ResultForFailure(Params<Context> params)
        {
            return typename Ctx::Return{};
        }

        template<class Composite>
        auto PutData() const
        {
            return Composite::Dynamic(*this);
        }
    };

    template<class GetterT>
    struct Getter :
        public Modifier<GetterT, false>
    {
        using Modifier<GetterT, false>::Modifier;
    };

    /******************************
    * Getter
    *******************************/
    
#define SCRIPT_GETTER(NAME, ME, RETURN) \
    SCRIPT_COMPOSITE(Getter, Get, RETURN, NAME, ME, _SCRIPT_NO_INLINE, /*EXT*/)
#define SCRIPT_GETTER_INLINE(NAME, ME, RETURN) \
    SCRIPT_COMPOSITE(Getter, Get, RETURN, NAME, ME, _SCRIPT_INLINE, /*EXT*/)

#define SCRIPT_GETTER1(NAME, ME, RETURN, PARAM_T, PARAM_N) \
    SCRIPT_COMPOSITE1(Getter, Get, RETURN, NAME, ME, PARAM_T, PARAM_N, _SCRIPT_NO_INLINE, /*EXT*/)
#define SCRIPT_GETTER1_INLINE(NAME, ME, RETURN, PARAM_T, PARAM_N) \
    SCRIPT_COMPOSITE1(Getter, Get, RETURN, NAME, ME, PARAM_T, PARAM_N, _SCRIPT_INLINE, /*EXT*/)

#define SCRIPT_GETTER2_INLINE(NAME, ME, RETURN, STATE_T, STATE_N) \
    SCRIPT_COMPOSITE2(Getter, Get, RETURN, NAME, ME, STATE_T, STATE_N, _SCRIPT_INLINE, /*EXT*/)

#define SCRIPT_GETTER_EX(NAME, ME, RETURN, DATA_T, DATA_N) \
    SCRIPT_COMPOSITE_EX(Getter, Get, RETURN, NAME, ME, DATA_T, DATA_N, _SCRIPT_NO_INLINE, /*EXT*/)
#define SCRIPT_GETTER_EX_INLINE(NAME, ME, RETURN, DATA_T, DATA_N) \
    SCRIPT_COMPOSITE_EX(Getter, Get, RETURN, NAME, ME, DATA_T, DATA_N, _SCRIPT_INLINE, /*EXT*/)

#define SCRIPT_GETTER_EX1_INLINE(NAME, ME, RETURN, DATA_T, DATA_N, PARAM_T, PARAM_N) \
    SCRIPT_COMPOSITE_EX1(Getter, Get, RETURN, NAME, ME, DATA_T, DATA_N, PARAM_T, PARAM_N, _SCRIPT_INLINE, /*EXT*/)

#define SCRIPT_GETTER_EX2_INLINE(NAME, ME, RETURN, DATA_T, DATA_N, STATE_T, STATE_N) \
    SCRIPT_COMPOSITE_EX2(Getter, Get, RETURN, NAME, ME, DATA_T, DATA_N, STATE_T, STATE_N, _SCRIPT_INLINE, /*EXT*/)

#define SCRIPT_GETTER_IMPL(NAME) \
    SCRIPT_REGISTER_COMMAND(Get, NAME) \
    SCRIPT_COMPOSITE_IMPL(Getter, Get, NAME)

    /******************************
    * Selector
    *******************************/

    template<class SelectorT>
    struct Selector :
        public Modifier<SelectorT, true>
    {
        using Modifier<SelectorT, true>::Modifier;
    };

#define _SCRIPT_GETTER_ADAPTOR(NAME)                  \
{                                                     \
    auto ctx = ConvertTo<Select::Ctx::NAME>();        \
                                                      \
    return ctx();                                     \
}                                                     \


#define SCRIPT_SELECTOR(NAME, ME, RETURN)                                            \
    SCRIPT_COMPOSITE(Selector, Select, RETURN, NAME, ME, _SCRIPT_NO_INLINE, /*EXT*/) \
    SCRIPT_GETTER_INLINE(NAME, ME, RETURN) _SCRIPT_GETTER_ADAPTOR(NAME)              \

#define SCRIPT_SELECTOR_INLINE(NAME, ME, RETURN) \
    SCRIPT_COMPOSITE(Selector, Select, RETURN, NAME, ME, _SCRIPT_INLINE, /*EXT*/)

#define SCRIPT_SELECTOR1(NAME, ME, RETURN, PARAM_T, PARAM_N)                                            \
    SCRIPT_COMPOSITE1(Selector, Select, RETURN, NAME, ME, PARAM_T, PARAM_N, _SCRIPT_NO_INLINE, /*EXT*/) \
    SCRIPT_GETTER1_INLINE(NAME, ME, RETURN, PARAM_T, PARAM_N) _SCRIPT_GETTER_ADAPTOR(NAME)              \

#define SCRIPT_SELECTOR_EX(NAME, ME, RETURN, DATA_T, DATA_N)                                            \
    SCRIPT_COMPOSITE_EX(Selector, Select, RETURN, NAME, ME, DATA_T, DATA_N, _SCRIPT_NO_INLINE, /*EXT*/) \
    SCRIPT_GETTER_EX_INLINE(NAME, ME, RETURN, DATA_T, DATA_N) _SCRIPT_GETTER_ADAPTOR(NAME)              \

#define SCRIPT_SELECTOR_IMPL(NAME) \
    SCRIPT_COMPOSITE_IMPL(Selector, Select, NAME)

}
