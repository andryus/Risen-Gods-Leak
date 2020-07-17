#include "TimedEventScript.h"
#include "RegularEventScript.h"
#include "Duration.h"
#include "StateScript.h"

#include "BaseHooks.h"

SCRIPT_MODULE_STATE_IMPL(TimedEvent)
{
    uint64 Add(GameDuration duration, script::InvocationInterfacePtr&& invokation, bool repeat)
    {
        eventQueue.push_back({ duration, gameTime + duration.Get().time, std::move(invokation), repeat, currentId });

        std::sort(eventQueue.begin(), eventQueue.end());

        return currentId++;
    }

    void Remove(uint64 id)
    {
        if (activeId == id)
        {
            wasActiveDeleted = true;
            return;
        }

        for (auto itr = eventQueue.begin(); itr != eventQueue.end();)
        {
            if (itr->id == id)
            {
                eventQueue.erase(itr);
                break;
            }
            else
                ++itr;
        }
    }

    bool Process(DiffParam tick, script::ScriptLogger* logger)
    {
        gameTime += tick.time;

        bool wasCalled = false;
        while (!eventQueue.empty())
        {
            if (eventQueue.back().targetTime <= gameTime)
            {
                EventEntry entry = std::move(eventQueue.back());
                eventQueue.pop_back();
                activeId = entry.id;

                if (logger)
                {
                    logger->AddModule("Event");
                    logger->AddData(std::to_string(entry.id));
                }

                if (entry.invokation->Invoke(logger))
                    wasCalled = true;
                    
                if (entry.repeat && !wasActiveDeleted)
                {
                    const auto nextDuration = entry.duration.Get().time;
                    
                    if (nextDuration > 0ms)
                        entry.targetTime += nextDuration;
                    else
                        entry.targetTime = gameTime + nextDuration;
                        
                    eventQueue.push_back(std::move(entry));

                    std::sort(eventQueue.begin(), eventQueue.end());
                }

                activeId = -1;
                wasActiveDeleted = false;
            }
            else
                break;
        }

        return wasCalled;
    }

    void FinishTimers()
    {
        for (auto& element : eventQueue)
        {
            element.targetTime = gameTime;
        }
    }

private:

    struct EventEntry
    {
        bool operator<(const EventEntry& right) const
        {
            return targetTime > right.targetTime;
        }

        GameDuration duration;
        std::chrono::milliseconds targetTime;
        script::InvocationInterfacePtr invokation;
        bool repeat;
        uint64 id;
    };

    SCRIPT_MODULE_PRINT(TimedEvents)
    {
        return std::to_string(eventQueue.size()) + " Event(s), MaxID: " + std::to_string(currentId);
    }

    std::chrono::milliseconds gameTime = 0ms;
    std::vector<EventEntry> eventQueue;
    uint64 currentId = 0;
    uint64 activeId = -1;
    bool wasActiveDeleted = false;
};


SCRIPT_ACTION1_INLINE(ProcessTimedEvents, Scripts::TimedEvent, DiffParam, tick)
{
    const bool result = me.Process(tick, logger);

    SetSuccess(result);
}

SCRIPT_ACTION_INLINE(FinishTimers, Scripts::TimedEvent)
{
    me.FinishTimers();
}

SCRIPT_MODULE_IMPL(TimedEvent)
{
    me <<= On::ProcessEvents
        |= Do::ProcessTimedEvents;

    me <<= On::DebugFinishTimers
        |= Do::FinishTimers;
}

/************************************
* EXECUTORS
************************************/

SCRIPT_ACTION_EX_INLINE(RemoveEvent, Scripts::TimedEvent, uint64, id)
{
    me.Remove(id);
}

SCRIPT_EXEC_IMPL(Every)
{
    const uint64 id = me.Add(duration, ExecuteDelayed(), true);

    if (logger)
        logger->AddParam("ID", std::to_string(id));

    AddUndoable(Do::RemoveEvent(id));
}

SCRIPT_EXEC_IMPL(After)
{
    const uint64 id = me.Add(duration, ExecuteDelayed(), false);

    if (logger)
        logger->AddParam("ID", std::to_string(id));

    AddUndoable(Do::RemoveEvent(id));
}
