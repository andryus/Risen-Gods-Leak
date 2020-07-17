#pragma once
#include "Instance/InstanceScript.h"
#include "EncounterOwner.h"
#include "CreatureOwner.h"

SCRIPT_MODULE(Encounter, ::Encounter)

/*********************************
* ACTIONS
**********************************/

SCRIPT_ACTION1(StartEncounter, Scripts::Encounter, Creature*, boss)
SCRIPT_ACTION(EndEncounter, Scripts::Encounter)
SCRIPT_ACTION(FinishEncounter, Scripts::Encounter)

/*********************************
* MODIFIERS
**********************************/

SCRIPT_SELECTOR(Boss, Scripts::Encounter, Creature*)

SCRIPT_SELECTOR(Instance, Encounter, InstanceMap*)
