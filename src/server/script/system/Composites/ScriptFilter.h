#pragma once
#include "ScriptGenerator.h"
#include "ScriptBinding.h"
#include "ScriptModifier.h"

namespace script
{

    /******************************
    * Filter
    *******************************/

    template<class FilterT>
    struct Filter : public Generator<FilterT>
    {
        using Generator<FilterT>::Generator;

        auto operator!() const
        {
            return Not();
        }

        auto Not() const
        {
            FilterT filter = Generator<FilterT>::GetFunctor();
            filter.expected = !filter.expected;

            return Filter(std::move(filter));
        }

        auto ToParam() const
        {
            return makeDecorator<Getter>(Generator<FilterT>::GetFunctor());
        }

        template<class Ctx, class Context>
        static bool ValidateResult(bool result, const Ctx& filter, Params<Context> params)
        {
            return result == filter._binding.expected;
        }

        template<class Ctx, class Context>
        static bool ResultForFailure(Params<Context>)
        {
            return false;
        }
    };

    template<>
    struct GeneratorPrint<Filter>
    {
        template<class Type>
        std::string operator()(std::string_view in, const Type& type) const
        {
            if (!type.expected)
                return '!' + std::string(in); 
            else
                return std::string(in);
        }
    };

#define _EXPORT_FILTER_GET(NAME) extern template struct script::CompositeDecorator<script::Getter, If::Bind::NAME>;

#define SCRIPT_FILTER(NAME, ME) \
    SCRIPT_COMPOSITE(Filter, If, bool, NAME, ME, _SCRIPT_NO_INLINE,) \
    _EXPORT_FILTER_GET(NAME)
#define SCRIPT_FILTER_INLINE(NAME, ME) \
    SCRIPT_COMPOSITE(Filter, If, bool, NAME, ME, _SCRIPT_INLINE,)

#define SCRIPT_FILTER1(NAME, ME, PARAM_T, PARAM_N) \
    SCRIPT_COMPOSITE1(Filter, If, bool, NAME, ME, PARAM_T, PARAM_N, _SCRIPT_NO_INLINE,) \
    _EXPORT_FILTER_GET(NAME)
#define SCRIPT_FILTER1_INLINE(NAME, ME, PARAM_T, PARAM_N) \
    SCRIPT_COMPOSITE1(Filter, If, bool, NAME, ME, PARAM_T, PARAM_N, _SCRIPT_INLINE,)

#define SCRIPT_FILTER_EX(NAME, ME, DATA_T, DATA_N) \
    SCRIPT_COMPOSITE_EX(Filter, If, bool, NAME, ME, DATA_T, DATA_N, _SCRIPT_NO_INLINE,) \
    _EXPORT_FILTER_GET(NAME)
#define SCRIPT_FILTER_EX_INLINE(NAME, ME, DATA_T, DATA_N) \
    SCRIPT_COMPOSITE_EX(Filter, If, bool, NAME, ME, DATA_T, DATA_N, _SCRIPT_INLINE,)

#define SCRIPT_FILTER_EX1(NAME, ME, DATA_T, DATA_N, PARAM_T, PARAM_N) \
    SCRIPT_COMPOSITE_EX1(Filter, If, bool, NAME, ME, DATA_T, DATA_N, PARAM_T, PARAM_N, _SCRIPT_NO_INLINE,) \
    _EXPORT_FILTER_GET(NAME)
#define SCRIPT_FILTER_EX1_INLINE(NAME, ME, DATA_T, DATA_N, PARAM_T, PARAM_N) \
    SCRIPT_COMPOSITE_EX1(Filter, If, bool, NAME, ME, DATA_T, DATA_N, PARAM_T, PARAM_N, _SCRIPT_INLINE,)

#define SCRIPT_FILTER_IMPL(NAME) \
    template struct script::Getter<If::Bind::NAME>; \
    SCRIPT_REGISTER_COMMAND(If, NAME) \
    SCRIPT_COMPOSITE_IMPL(Filter, If, NAME)

}
