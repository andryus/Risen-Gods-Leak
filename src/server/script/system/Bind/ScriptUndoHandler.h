#pragma once
#include <memory>
#include "Define.h"
#include "API.h"

namespace script
{
    class Scriptable;
    struct InvokationInterface;

    template<class Target, class Source>
    Target* castSourceTo(Source& source);

    struct UndoHandler
    {
        static void AddUndoable(Scriptable& me, std::unique_ptr<InvokationInterface>&& undoable);
        static void AddUndoable(uint64 stateId, Scriptable& me, std::unique_ptr<InvokationInterface>&& undoable);
        static uint64 GetStateId(Scriptable& base);
        static bool ApplyState(Scriptable& base, uint64 stateId);

        template<class Me>
        static uint64 GetStateId(Me& me)
        {
            if (Scriptable* scriptable = castSourceTo<Scriptable>(me))
                return GetStateId(*scriptable);
            else
                return -1;
        }
    };


    struct StateBlock
    {
        StateBlock() = default;
        ~StateBlock()
        {
            const bool isValid = oldState != (uint64)-1;
            
            if (isValid && scriptable && newState == UndoHandler::GetStateId(*scriptable))
                UndoHandler::ApplyState(*scriptable, oldState);
        }

        static StateBlock Store(Scriptable& scriptable, uint64 stateId)
        {
            const uint64 newState = UndoHandler::GetStateId(scriptable);

            return StateBlock(scriptable, stateId, newState);
        }

    private:
        StateBlock(Scriptable& scriptable, uint64 oldState, uint64 newState) :
            scriptable(&scriptable), oldState(oldState), newState(newState) {}

        Scriptable* scriptable = nullptr;
        uint64 oldState = -1;
        uint64 newState = -1;
    };

}