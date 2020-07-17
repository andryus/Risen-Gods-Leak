#pragma once
#include "ScriptGenerator.h"
#include "ScriptEntryPoint.h"

namespace script
{
    template<bool hasData, class InvokeT, class ReturnT, class MeT, class StoredMeT, class ParamT>
    struct QueryData
    {
        static constexpr bool HasData = hasData;

        using Return = ReturnT;
        using Me = MeT;
        using StoredMe = StoredMeT;

        using Param = ParamT;
        using Invoke = InvokeT;

        uint32 data;

        static Return ReturnDefault(ParamT param)
        {
            return InvokeT::ReturnDefault(param);
        }
    };

    template<class Ctx, class StoredMe>
    using QueryDataFromCtx = QueryData<HasData<Ctx>, typename Ctx::Invoke, typename Ctx::Return, typename Ctx::Me, StoredMe, typename Ctx::Param>;

    template<class QueryT>
    struct Query : public Generator<QueryT>
    {
        using Generator<QueryT>::Generator;

        template<class Ctx, class Context>
        static auto ValidateResult(uint32 result, const Ctx& event, Params<Context> params)
        {
            return std::make_pair(QueryDataFromCtx<typename Ctx::CallType::CtxT, typename Context::Target>{ result }, params);
        }

        template<class Ctx, class Context>
        static auto ResultForFailure(Params<Context> params)
        {
            return decltype(ValidateResult<Ctx, Context>(0, std::declval<Ctx>(), params))();
        }

        USE_POSTFIX_MANIPULATORS(QueryT, Query);
    };

    struct QueryEntryPoint
    {
        template<class CallCtx, class MeT, class Composite, class Hook>
        static auto AddInvokable(ConstructionCtx<MeT> ctx, Composite composite, uint32 data, Hook hook)
        {
            const QueryDataFromCtx<CallCtx, MeT> queryData = { data };

            ctx.AddQuery(queryData, composite, hook);

            return true;
        }

        template<class QueryData, class InvocableT, class EntryT, class UndoTarget, class Me>
        static void AddInvokable(ConstructionCtx<Me> ctx, UndoTarget& target, Me& me, QueryData data, InvocableT&& invocable, EntryT hook)
        {
            const uint64 id = ctx.AddQuery(data, invocable, hook);

            addEntryPointUndoable(target, me, [data, id](auto& ctx)
            {
                ctx.RemoveQuery(data, id);
            });
        }

        template<class Composite>
        static auto MakeGenerator(Composite composite)
        {
            return makeGenerator<Action>(composite);
        }
    };

#define _SCRIPT_QUERY_TYPE(NAME, ME, PARAM_T, PARAM_N, RETURN) \
    namespace Queries::Bind                                    \
    {                                                          \
        struct NAME                                            \
        {                                                      \
            using Me = ME;                                     \
            using Param = PARAM_T;                             \
            using Return = RETURN;                             \
                                                               \
            static Return ReturnDefault(PARAM_T PARAM_N);      \
        };                                                     \
    }                                                          \
    template<>                                                 \
    struct script::InvokableId<Queries::Bind::NAME>            \
    {                                                          \
        uint64 operator()() const                              \
        {                                                      \
            return typeid(Queries::Bind::NAME).hash_code();    \
        }                                                      \
    };                                                         \
    namespace Query::Bind                                      \
    {                                                          \
        struct NAME;                                           \
    }                                                          \

#define _SCRIPT_QUERY_EXT(NAME)                          \
public:                                                  \
    static constexpr bool HadSuccess() { return true; }  \
    using Invoke = Queries::Bind::NAME;                  \
    using CallType = Query::Bind::NAME;                  \
    static constexpr bool IsAction = false;              \

#define _SCRIPT_QUERY_IMPL(NAME, PARAM_T, PARAM_N, DATA_N)                 \
    {                                                                      \
        if (auto* ctx = script::castSourceTo<script::InvokationCtx>(me))   \
        {                                                                  \
            script::QueryDataFromCtx<std::decay_t<decltype(*this)>, Me>    \
                data = {static_cast<uint32>(DATA_N)};                      \
            return ctx->Query(_binding, data, me, PARAM_N);                \
        }                                                                  \
        else                                                               \
            return Queries::Bind::NAME::ReturnDefault(PARAM_N);            \
    }                                                                      \
                                                                           \
    inline Queries::Bind::NAME::Return                                     \
        Queries::Bind::NAME::ReturnDefault(PARAM_T PARAM_N)                \


#define _SCRIPT_RESPONSE_IMPL(NAME, DATA_N)  \
    {                                        \
        return static_cast<uint32>(DATA_N);  \
    }                                        \

/**********************************
* BASE
***********************************/

#define _SCRIPT_RESPONSE_BASE(NAME, ME, PARAM_T, PARAM_N, RETURN)                                             \
    _SCRIPT_QUERY_TYPE(NAME, ME, PARAM_T, PARAM_N, RETURN)                                                    \
    _SCRIPT_CTX(Respond, NAME, uint32, ME, _SCRIPT_NO_DATA_T, _noData, _SCRIPT_NO_PARAM_T,                    \
        PARAM_N, _SCRIPT_NO_STATE_T, _noState, _SCRIPT_QUERY_EXT(NAME))                                       \
    _SCRIPT_COMPOSITE(Query, Respond, uint32, NAME, ME, _SCRIPT_NO_PARAM, _SCRIPT_NO_STATE_T, _SCRIPT_INLINE) \
    _SCRIPT_RESPONSE_IMPL(NAME, _noData);                                                                     \



#define SCRIPT_QUERY(NAME, ME, RETURN)                                                         \
    _SCRIPT_RESPONSE_BASE(NAME, ME, script::NoParam, _noParam, RETURN)                         \
    SCRIPT_COMPOSITE(Getter, Query, RETURN, NAME, ME, _SCRIPT_INLINE, _SCRIPT_QUERY_EXT(NAME)) \
    _SCRIPT_QUERY_IMPL(NAME, script::NoParam, _noParam, _noData)                               \

#define SCRIPT_QUERY_PARAM(NAME, ME, RETURN, PARAM_T, PARAM_N)                                 \
    _SCRIPT_RESPONSE_BASE(NAME, ME, PARAM_T, PARAM_N, RETURN)                                  \
    SCRIPT_COMPOSITE1(Getter, Query, RETURN, NAME, ME, PARAM_T, PARAM_N,                       \
        _SCRIPT_INLINE, _SCRIPT_QUERY_EXT(NAME))                                               \
    _SCRIPT_QUERY_IMPL(NAME, PARAM_T, PARAM_N, _noData)                                        \


/**********************************
* EX
***********************************/

#define _SCRIPT_RESPONSE_BASE_EX(NAME, ME, PARAM_T, PARAM_N, RETURN, DATA_T, DATA_N)                \
    _SCRIPT_QUERY_TYPE(NAME, ME, PARAM_T, PARAM_N, RETURN)                                          \
    _SCRIPT_CTX(Respond, NAME, uint32, ME, DATA_T, DATA_N, _SCRIPT_NO_PARAM_T,                      \
        PARAM_N, _SCRIPT_NO_STATE_T, _noState, _SCRIPT_QUERY_EXT(NAME))                             \
    _SCRIPT_COMPOSITE_EX(Query, Respond, uint32, NAME, ME, DATA_T, DATA_N,                          \
        _SCRIPT_NO_PARAM, _SCRIPT_NO_STATE_T, _SCRIPT_INLINE)                                       \
    _SCRIPT_ON_IMPL(NAME, DATA_N)                                                                   \


#define SCRIPT_QUERY_EX(NAME, ME, RETURN, DATA_T, DATA_N)                                         \
    _SCRIPT_RESPONSE_BASE_EX(NAME, ME, script::NoParam, _noParam, RETURN, DATA_T, DATA_N)         \
    SCRIPT_COMPOSITE_EX(                                                                          \
        Getter, Query, RETURN, NAME, ME, DATA_T, DATA_N, _SCRIPT_INLINE, _SCRIPT_QUERY_EXT(NAME)) \
    _SCRIPT_QUERY_IMPL(NAME, script::NoParam, _noParam, DATA_N)                                   \

#define SCRIPT_QUERY_EX_PARAM(NAME, ME, RETURN, DATA_T, DATA_N, PARAM_T, PARAM_N)                                   \
    _SCRIPT_RESPONSE_BASE_EX(NAME, ME, PARAM_T, PARAM_N, RETURN, DATA_T, DATA_N)                                    \
    SCRIPT_COMPOSITE_EX1(                                                                                           \
        Getter, Query, RETURN, NAME, ME, DATA_T, DATA_N, PARAM_T, PARAM_N, _SCRIPT_INLINE, _SCRIPT_QUERY_EXT(NAME)) \
    _SCRIPT_QUERY_IMPL(NAME, PARAM_T, PARAM_N, DATA_N)                                                              \

}
