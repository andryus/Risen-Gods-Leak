#pragma once
#include "API/OwnerScript.h"
#include "CreatureOwner.h"

/************************************
* ACTIONS
************************************/

SCRIPT_ACTION_EX(MorphCreatureEntry, Creature, CreatureEntry, entry);
SCRIPT_ACTION_EX(MorphCreatureToModel, Creature, uint32, modelId);

SCRIPT_ACTION(MakeAggressive, Creature);
SCRIPT_ACTION(MakeDefensive, Creature);
SCRIPT_ACTION(MakePassive, Creature);
SCRIPT_ACTION(DisableHealthReg, Creature)
SCRIPT_ACTION(MakePosToHome, Creature)

SCRIPT_ACTION_EX(OverrideCorpseDecay, Creature, std::chrono::seconds, duration)
SCRIPT_ACTION_EX(OverrideSightRange, Creature, float, range)

SCRIPT_ACTION_EX1_RET(SummonCreature, Unit, Creature*, CreatureEntry, entry, Position, pos)
SCRIPT_ACTION_EX(DisappearAndDie, Creature, std::chrono::seconds, respawnTime)
SCRIPT_ACTION(DespawnCreature, Creature)
SCRIPT_ACTION_EX(ChangeEquip, Creature, uint8, equipId)

/************************************
* FILTER
************************************/

SCRIPT_FILTER(Aggressive, Creature)
SCRIPT_FILTER_EX(IsCreature, WorldObject, CreatureEntry, entry)

/************************************
* MODIFIER
************************************/

SCRIPT_GETTER(SightRange, Creature, float)
