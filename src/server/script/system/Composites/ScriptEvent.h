#pragma once
#include "ScriptGenerator.h"
#include "ScriptTarget.h"
#include "ScriptEntryPoint.h"
#include "ScriptSerializer.h"
#include "InvokationCtx.h"

namespace script
{

    template<bool callOnAdd, bool hasData, class InvokeT, class MeT, class ParamT>
    struct InvocationData
    {
        static constexpr bool CallOnAdd = callOnAdd;
        static constexpr bool HasData = hasData;

        using Me = MeT;
        using Param = ParamT;
        using Invoke = InvokeT;

        uint32 data;
    };

    template<bool CallOnAdd, class Ctx>
    using InvokeDataFromCtx = InvocationData<CallOnAdd, HasData<Ctx>, typename Ctx::Invoke, typename Ctx::Me, typename Ctx::Param>;

    template<class Event>
    struct FireEventOnAdd
    {
        constexpr bool operator()() const
        {
            return false;
        }
    };

    /************************************************
    * EntryPoint
    ************************************************/

    template<class Me>
    struct ConstructionCtx;

    struct EventEntryPoint
    {
        template<class CallCtx, class Me, class Composite, class Hook>
        static auto AddInvokable(ConstructionCtx<Me> ctx, Composite composite, uint32 data, Hook hook)
        {
            const InvokeDataFromCtx<false, CallCtx> invokeData = { data };

            ctx.AddInvokable(invokeData, composite, hook);
        }

        template<class InvocationData, class InvocableT, class Hook, class UndoTarget, class Me>
        static void AddInvokable(ConstructionCtx<Me> ctx, UndoTarget& target, Me& me, InvocationData data, InvocableT&& invocable, Hook hook)
        {
            uint64 id;
            if constexpr(InvocationData::CallOnAdd)
                id = ctx.AddInvokableAndCall(data, std::forward<InvocableT>(invocable), hook, me);
            else
                id = ctx.AddInvokable(data, std::forward<InvocableT>(invocable), hook);

            addEntryPointUndoable(target, me, [data, id](auto& ctx)
            {
                ctx.Remove(data, id);
            });
        }

        template<class Composite>
        static auto MakeGenerator(Composite composite)
        {
            return makeGenerator<Action>(composite);
        }
    };

    /*************************************************
    * Event - Generator
    *************************************************/

    template<class EventT>
    struct Event : public Generator<EventT>
    {
        using Generator<EventT>::Generator;
        static constexpr bool DefaultReturn = true;

        // TODO: only for events with data
        static auto Any() 
        {
            EventT event = static_cast<typename EventT::Data>(InvokationCtx::ANY_CALL);

            return Event<EventT>(std::move(event));
        }

        template<class Ctx, class Context>
        static auto ValidateResult(uint32 result, const Ctx& event, Params<Context> params)
        { 
            constexpr bool FireOnAdd = FireEventOnAdd<EventT>()();

            return std::make_pair(InvokeDataFromCtx<FireOnAdd, typename Ctx::CallType::CtxT>{ result }, params);
        }

        template<class Ctx, class Context>
        static auto ResultForFailure(Params<Context> params)
        {
            return decltype(ValidateResult<Ctx, Context>(0, std::declval<Ctx>(), params))();
        }

        USE_POSTFIX_MANIPULATORS(EventT, Event)
    };

    template<class EventT>
    struct EventFire : public Action<EventT>
    {
        using Action<EventT>::Action;

        auto Random() const
        {
            EventT copy = Action<EventT>::GetFunctor();
            copy.mode = InvocationMode::RANDOM;

            return EventFire(copy);
        }
    };


#define _SCRIPT_EVENT_TYPE(NAME, ME, PARAM_T)                          \
    namespace Events::Bind                                             \
    {                                                                  \
        struct NAME                                                    \
        {                                                              \
            using Me = ME;                                             \
            using Param = PARAM_T;                                     \
        };                                                             \
    }                                                                  \
                                                                       \
    template<>                                                         \
    struct script::InvokableId<Events::Bind::NAME>                     \
    {                                                                  \
        uint64 operator()() const                                      \
        {                                                              \
            return typeid(Events::Bind::NAME).hash_code();             \
        }                                                              \
    };                                                                 \
                                                                       \
    namespace Fire::Bind                                               \
    {                                                                  \
        struct NAME;                                                   \
    }                                                                  \
                                                                       \
    namespace Events::Serialize                                        \
    {                                                                  \
        const auto NAME =                                              \
            script::sScriptSerializer.RegisterEvent<Bind::NAME>(#NAME);\
    }                                                                  \

#define _SCRIPT_EVENT_IMPL(NAME, PARAM, DATA_N)                                          \
    {                                                                                    \
        if (auto* ctx = script::castSourceTo<script::InvokationCtx>(me))                 \
        {                                                                                \
            constexpr bool FireOnAdd = script::FireEventOnAdd<On::Bind::NAME>()();       \
                                                                                         \
            On::Bind::NAME proxy = DATA_N;                                               \
                                                                                         \
            script::InvokeDataFromCtx<FireOnAdd, std::decay_t<decltype(*this)>>          \
                data = {static_cast<uint32>(DATA_N)};                                    \
                                                                                         \
            return ctx->Call(proxy, data, me, PARAM, _binding.mode);                     \
        }                                                                                \
        else                                                                             \
            return false;                                                                \
    }                                                                                    \

#define _SCRIPT_ON_IMPL(NAME, DATA_N)        \
    {                                        \
        return static_cast<uint32>(DATA_N);  \
    }                                        \

#define _SCRIPT_EVENT_EXT(NAME)                                      \
public:                                                              \
    static constexpr bool HadSuccess() { return true; }              \
    using Invoke = Events::Bind::NAME;                               \
    using CallType = Fire::Bind::NAME;                               \
    static constexpr bool IsAction = true;                           \


#define _SCRIPT_EVENT_FIRE_ADD(NAME)                             \
    namespace On::Bind { struct NAME; }                          \
    template<>                                                   \
    struct script::FireEventOnAdd<On::Bind::NAME>                \
    {                                                            \
        constexpr bool operator()() const                        \
        {                                                        \
            return true;                                         \
        }                                                        \
    };                                                           \


/**********************************
* BASE
***********************************/

#define _SCRIPT_EVENT_BASE(NAME, ME, PARAM_T, PARAM_N)                                                   \
    _SCRIPT_EVENT_TYPE(NAME, ME, PARAM_T)                                                                \
    _SCRIPT_CTX(On, NAME, uint32, script::Unbound, _SCRIPT_NO_DATA_T, _noData, _SCRIPT_NO_PARAM_T,       \
        PARAM_N, _SCRIPT_NO_STATE_T, _noState, _SCRIPT_EVENT_EXT(NAME))                                  \
    _SCRIPT_COMPOSITE(Event, On, uint32, NAME, script::Unbound, _SCRIPT_NO_PARAM, _SCRIPT_NO_STATE_T, _SCRIPT_INLINE) \
    _SCRIPT_ON_IMPL(NAME, _noData);                                                                       \



#define SCRIPT_EVENT(NAME, ME)                                                               \
    _SCRIPT_EVENT_BASE(NAME, ME, script::NoParam, _noParam)                                  \
    SCRIPT_COMPOSITE(EventFire, Fire, bool, NAME, ME, _SCRIPT_INLINE, _SCRIPT_EVENT_EXT(NAME))  \
    _SCRIPT_EVENT_IMPL(NAME, {}, _noData)                                                    \

#define SCRIPT_EVENT_PERM(NAME, ME) \
    _SCRIPT_EVENT_FIRE_ADD(NAME)    \
    SCRIPT_EVENT(NAME, ME)          \

#define SCRIPT_EVENT_PARAM(NAME, ME, PARAM_T, PARAM_N)                                                         \
    _SCRIPT_EVENT_BASE(NAME, ME, script::NoParam, PARAM_N)                                                     \
    SCRIPT_COMPOSITE1(EventFire, Fire, bool, NAME, ME, PARAM_T, PARAM_N, _SCRIPT_INLINE, _SCRIPT_EVENT_EXT(NAME)) \
    _SCRIPT_EVENT_IMPL(NAME, PARAM_N, _noData)                                                                 \

/**********************************
* EX
***********************************/

#define _SCRIPT_EVENT_BASE_EX(NAME, ME, PARAM_T, PARAM_N, DATA_T, DATA_N)                      \
    _SCRIPT_EVENT_TYPE(NAME, ME, PARAM_T)                                                      \
    _SCRIPT_CTX(On, NAME, uint32, script::Unbound, DATA_T, DATA_N, _SCRIPT_NO_PARAM_T,         \
        PARAM_N, _SCRIPT_NO_STATE_T, _noState, _SCRIPT_EVENT_EXT(NAME))                        \
    _SCRIPT_COMPOSITE_EX(Event, On, uint32, NAME, script::Unbound, DATA_T, DATA_N,             \
        _SCRIPT_NO_PARAM, _SCRIPT_NO_STATE_T, _SCRIPT_INLINE)                                  \
    _SCRIPT_ON_IMPL(NAME, DATA_N)                                                              \



#define SCRIPT_EVENT_EX(NAME, ME, DATA_T, DATA_N)                                              \
    _SCRIPT_EVENT_BASE_EX(NAME, ME, script::NoParam, _noParam, DATA_T, DATA_N)                 \
    SCRIPT_COMPOSITE_EX(                                                                       \
        EventFire, Fire, bool, NAME, ME, DATA_T, DATA_N, _SCRIPT_INLINE, _SCRIPT_EVENT_EXT(NAME)) \
    _SCRIPT_EVENT_IMPL(NAME, {}, DATA_N)                                                       \

#define SCRIPT_EVENT_EX_PERM(NAME, ME, DATA_T, DATA_N) \
    _SCRIPT_EVENT_FIRE_ADD(NAME)                       \
    SCRIPT_EVENT_EX(NAME, ME, DATA_T, DATA_N)          \

#define SCRIPT_EVENT_EX_PARAM(NAME, ME, DATA_T, DATA_N, PARAM_T, PARAM_N)                                           \
    _SCRIPT_EVENT_BASE_EX(NAME, ME, PARAM_T, PARAM_N, DATA_T, DATA_N)                                               \
    SCRIPT_COMPOSITE_EX1(                                                                                           \
        EventFire, Fire, bool, NAME, ME, DATA_T, DATA_N, PARAM_T, PARAM_N, _SCRIPT_INLINE, _SCRIPT_EVENT_EXT(NAME))    \
    _SCRIPT_EVENT_IMPL(NAME, PARAM_N, DATA_N)                                                                       \

}
