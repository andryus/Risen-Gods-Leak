#include "GameEventScript.h"
#include "GameEventMgr.h"

SCRIPT_FILTER_IMPL(GameEventActive)
{
    return sGameEventMgr->IsActiveEvent(id.id);
}
