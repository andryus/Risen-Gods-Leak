#pragma once
#include "BaseHooks.h"
#include "GameEventOwner.h"
#include "MapOwner.h"

SCRIPT_EVENT_EX(GameEventActivate, Map, uint32, eventId)
SCRIPT_EVENT_EX(GameEventDeactivate, Map, uint32, eventId)