#pragma once
#include "Units.h"
#include "CreatureOwner.h"
#include "CreatureHooks.h"
#include "Entities/CreatureScript.h"

class Creature;

#define CREATURE_SCRIPT(NAME) USER_SCRIPT_BASE(Creature, Npc, NAME)
