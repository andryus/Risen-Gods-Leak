#pragma once
#include "ScriptParams.h"
#include "ScriptUndoHandler.h"

namespace script
{

    /*************************************
    * InvokationInterface
    **************************************/

    template<class Me>
    struct StoredTarget;

    struct InvokationInterface
    {
        InvokationInterface() = default;
        virtual ~InvokationInterface() = default;

        virtual bool Invoke(ScriptLogger* logger) = 0;
        virtual std::unique_ptr<InvokationInterface> Clone() const = 0;
    };

    using InvocationInterfacePtr = std::unique_ptr<InvokationInterface>;

    template<class Functor, class Context>
    struct StoredInvokation :
        public InvokationInterface
    {
        using Me = typename Context::Target;
        using Param = typename Context::Param;

        StoredInvokation(Functor functor, Me& me, Param param, uint64 stateId) :
            functor(std::move(functor)), me(me), param(param), stateId(stateId) { }

        bool Invoke(ScriptLogger* logger) override
        {
            const auto block = startLoggerBlock(logger);
            auto params = makeParams(me.Load(logger), param.Load(logger), logger);

            const StateBlock state = ApplyState(params.target);
            decltype(auto) result = functor.Call(params);

            constexpr auto flatten = [](auto&& result)
            {
                return bool(result);
            };

            return applyForReturnValue<bool>(result, flatten);
        }

        template<class ContextT>
        decltype(auto) Call(Params<ContextT> params)
        {
            if constexpr(std::is_same_v<Param, NoParam>)
            {
                auto callParams = makeParams(me.Load(params.logger), params.param, params);
                const StateBlock state = ApplyState(callParams.target);

                return functor.Call(callParams);
            }
            else
            {
                auto callParams = makeParams(me.Load(params.logger), param.Load(params.logger), params);
                const StateBlock state = ApplyState(callParams.target);

                return functor.Call(callParams);
            }
        }

        std::unique_ptr<InvokationInterface> Clone() const override
        {
            return std::make_unique<StoredInvokation<Functor, Context>>(*this);
        }

    protected:

        Functor functor;

    private:

        StateBlock ApplyState(Me* me) const
        {
            if (auto scriptable = script::castSourceTo<Scriptable>(me))
            {
                const uint64 oldStateId = UndoHandler::GetStateId(*scriptable);

                if (UndoHandler::ApplyState(*scriptable, stateId))
                    return StateBlock::Store(*scriptable, oldStateId);
            }
            
            return {};
        }

        StoredParam<Me&> me;
        StoredParam<Param> param;
        uint64 stateId;
    };
}
