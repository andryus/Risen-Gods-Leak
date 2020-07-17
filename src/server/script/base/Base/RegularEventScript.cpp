#include "RegularEventScript.h"
#include "BaseHooks.h"

SCRIPT_MODULE_STATE_IMPL(RegularEvent) 
{
    uint64 Add(script::InvocationInterfacePtr&& invocation, bool expected)
    {
        return events.emplace_back(std::move(invocation), eventId++, expected).eventId;
    }

    void Remove(uint64 eventId)
    {
        if (activeId == eventId)
        {
            wasActiveDeleted = true;
            return;
        }

        for (auto itr = events.begin(); itr != events.end();)
        {
            if (itr->eventId == eventId)
            {
                events.erase(itr);
                return;
            }

            ++itr;
        }
    }

    bool Process(script::ScriptLogger* logger)
    {
        bool wasCalled = false;

        if (events.empty())
            return false;

        uint32 index = events.size();
        while (index && !events.empty())
        {
            auto itr = events.begin() + (index - 1);

            auto event = std::move(*itr);
            events.erase(itr);

            if(logger)
            {
                logger->AddModule("Event");
                logger->AddData(std::to_string(event.eventId));
            }

            activeId = event.eventId;

            if (event.invocation->Invoke(logger) == event.expected)
                wasCalled = true;
            else if (!wasActiveDeleted)
                events.push_back(std::move(event));

            activeId = -1;
            wasActiveDeleted = false;

            index = std::min<uint32>(index - 1, events.size());
        }

        return wasCalled;
    }

private:

    struct EventEntry
    {
        EventEntry(script::InvocationInterfacePtr&& invocation, uint64 eventId, bool expected) :
            invocation(std::move(invocation)), eventId(eventId), expected(expected) { }

        script::InvocationInterfacePtr invocation;
        uint64 eventId;
        bool expected;
    };

    SCRIPT_MODULE_PRINT(RegularEvents) 
    { 
        return std::to_string(events.size()) + " Event(s), MaxID: " + std::to_string(eventId);
    }

    std::vector<EventEntry> events;
    uint64 eventId = 0;
    uint64 activeId = -1;
    bool wasActiveDeleted = false;
};

SCRIPT_ACTION1_INLINE(ProcessRegularEvents, Scripts::RegularEvent, DiffParam, tick)
{
    const bool result = me.Process(logger);

    SetSuccess(result);
}

SCRIPT_MODULE_IMPL(RegularEvent)
{
    me <<= On::ProcessEvents
        |= Do::ProcessRegularEvents;
}


/************************************
* EXECUTORS
************************************/

SCRIPT_ACTION_EX_INLINE(RemoveRegularEvent, Scripts::RegularEvent, uint64, id)
{
    me.Remove(id);
}

SCRIPT_EXEC_IMPL(UntilSuccess)
{
    const uint64 id = me.Add(ExecuteDelayed(), true);

    if (logger)
        logger->AddParam("ID", std::to_string(id));

    AddUndoable(Do::RemoveRegularEvent(id));
}

SCRIPT_EXEC_IMPL(UntilFailure)
{
    const uint64 id = me.Add(ExecuteDelayed(), false);

    if (logger)
        logger->AddParam("ID", std::to_string(id));

    AddUndoable(Do::RemoveRegularEvent(id));
}
