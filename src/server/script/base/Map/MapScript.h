#pragma once
#include "API/ModuleScript.h"

#include "MapOwner.h"
#include "CreatureOwner.h"
#include "GameObjectOwner.h"

SCRIPT_ACTION_EX1_RET(SpawnCreature, Map, Creature*, CreatureEntry, entry, Position, pos)
SCRIPT_ACTION_EX1_RET(SpawnGO, Map, GameObject*, GameObjectEntry, entry, Position, pos)
