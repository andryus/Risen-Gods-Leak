#pragma once
#include "BaseHooks.h"
#include "WorldObjectOwner.h"
#include "UnitOwner.h"

SCRIPT_EVENT(InitWorld, WorldObject)
SCRIPT_EVENT(UnInitWorld, WorldObject)

SCRIPT_EVENT_PARAM(SummonedBy, WorldObject, Unit*, summoner);
