#include "GameEventOwner.h"
#include "GameEventMgr.h"

SCRIPT_PRINTER_IMPL(GameEventId)
{
    const auto& events = sGameEventMgr->GetEventMap();

    if (events.size() < value.id)
        return events[value.id].description;
    else
        return "<INVALID>";
}
