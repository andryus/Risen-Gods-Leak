#include "StateScript.h"
#include <stack>
#include "BaseHooks.h"

#include "Errors.h"
#include "ScriptUndoable.h"

SCRIPT_MODULE_STATE_IMPL(States)
{
    using UndoableStack = std::stack<script::InvocationInterfacePtr, std::vector<script::InvocationInterfacePtr>>;

    struct StateEntry
    {
        StateEntry() : id(-1) {}
        StateEntry(uint64 id) : id(id) {}

        uint64 id;
        UndoableStack undoables;
    };

    using StateStack = std::vector<StateEntry>;

    States() = default;
    States(const States& states) = delete;
    void operator=(const States& states) = delete;

    uint64 Push()
    {
        activeId = states.emplace_back(stateId++).id;

        return activeId;
    }

    bool ApplyState(uint64 id)
    {
        for (auto& [stateId, undoables] : states)
        {
            if (stateId == id)
            {
                activeId = id;
                return true;
            }
        }
        
        return false;
    }

    uint64 StateID()
    {
        return activeId;
    }

    bool Pop(script::ScriptLogger* logger)
    {
        ASSERT(!states.empty());

        auto itr = states.begin();
        for (; itr != states.end(); ++itr)
        {
            if (itr->id == activeId)
            {
                StateEntry undoState = std::move(*itr);
                states.erase(itr);

                OnStateRemoved(undoState.id);
                UndoState(undoState.undoables, logger);

                return true;
            }
        }

        return false;
    }

    bool Store()
    {
        if (states.empty())
            return false;

        auto itr = states.begin();
        for(; itr != states.end(); ++itr)
        {
            if (itr->id == activeId)
                break;
        }

        if (itr != states.end())
        {
            auto state = std::move(*itr);
            states.erase(itr);
            states.emplace(states.begin(), std::move(state));

            OnStateRemoved(states.front().id);

            return true;
        }

        return false;
    }

    bool RemoveState(uint64 id, script::ScriptLogger* logger)
    {
        for (auto itr = states.begin(); itr != states.end();)
        {
            if (itr->id == id)
            {
                UndoableStack undoables = std::move(itr->undoables);
                states.erase(itr);

                OnStateRemoved(id);
                UndoState(undoables, logger);

                return true;
            }
            else
                ++itr;
        }

        return false;
    }

    bool AddUndoable(script::InvocationInterfacePtr&& undoable)
    {
        return AddUndoable(activeId, std::move(undoable));
    }

    bool AddUndoable(uint64 stateId, script::InvocationInterfacePtr&& undoable)
    {
        for (auto itr = states.rbegin(); itr != states.rend(); ++itr)
        {
            if (itr->id == stateId)
            {
                itr->undoables.push(std::move(undoable));
                return true;
            }
        }

        undoable->Invoke(nullptr);

        return false;
    }

private:

    void OnStateRemoved(uint64 id)
    {
        if (activeId == id)
            ApplyState(states.empty() ? -1 : states.back().id);
    }

    static void UndoState(UndoableStack& undoState, script::ScriptLogger* logger)
    {
        const auto logLevel = script::applyLogLevel(logger, true);

        while (!undoState.empty())
        {
            undoState.top()->Invoke(logger);
            undoState.pop();
        }
    }

    SCRIPT_MODULE_PRINT(States) 
    {
        return std::to_string(states.size()) + " State(s), ActiveID: " + std::to_string(activeId) + ", MaxID: " + std::to_string(stateId);
    }

    StateStack states;
    uint64 activeId = -1;
    uint64 stateId = 0;
};

namespace script
{
    void UndoHandler::AddUndoable(Scriptable& base, InvocationInterfacePtr&& undoable)
    {
        if (Scripts::States* states = base.GetScriptState<Scripts::States>())
            states->AddUndoable(std::move(undoable));
    }

    void UndoHandler::AddUndoable(uint64 stateId, Scriptable& base, InvocationInterfacePtr&& undoable)
    {
        if (Scripts::States* states = base.GetScriptState<Scripts::States>())
            states->AddUndoable(stateId, std::move(undoable));
    }

    uint64 UndoHandler::GetStateId(Scriptable& base)
    {
        if (Scripts::States* states = base.GetScriptState<Scripts::States>())
            return states->StateID();
        else
            return -1;
    }

    bool UndoHandler::ApplyState(Scriptable& base, uint64 stateId)
    {
        if (Scripts::States* states = base.GetScriptState<Scripts::States>(); stateId != -1 && states)
            return states->ApplyState(stateId);
        
        return false;
    }
}

SCRIPT_MODULE_IMPL(States)
{
    me <<= On::Init
        |= Do::PushStateEvent(On::UnInit);
}


/************************************
* ACTION
************************************/

SCRIPT_ACTION_IMPL(PushState)
{
    const uint64 stateId = me.StateID();
    AddUndoable(Do::RemoveState.Bind(stateId));

    if (logger)
        logger->AddParam("ID", std::to_string(stateId));

    me.Push();
}

SCRIPT_ACTION_IMPL(StoreState)
{
    const uint64 stateId = me.StateID();
    const bool success = me.Store();

    if (logger)
        logger->AddParam("ID", std::to_string(stateId));

    SetSuccess(success);
}

SCRIPT_ACTION_IMPL(PopState)
{
    const uint64 stateId = me.StateID();
    const bool success = me.Pop(logger);

    if (logger)
        logger->AddParam("ID", std::to_string(stateId));

    SetSuccess(success);
}

SCRIPT_ACTION_IMPL(RemoveState)
{
    const bool success = me.RemoveState(stateID, logger);

    SetSuccess(success);
}

SCRIPT_GETTER_IMPL(StateID)
{
    return me.StateID();
}
