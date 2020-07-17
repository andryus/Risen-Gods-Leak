#pragma once
#include "WorldObjectHooks.h"
#include "InstanceHooks.h"
#include "EncounterOwner.h"


SCRIPT_EVENT_PARAM(AddToEncounter, Encounter, Unit*, unit)
SCRIPT_EVENT_PARAM(RegisterEncounter, WorldObject, Encounter*, encounter)

SCRIPT_EVENT(EncounterStarted, Encounter);
SCRIPT_EVENT(EncounterEnded, Encounter);
SCRIPT_EVENT(EncounterFinished, Encounter);
