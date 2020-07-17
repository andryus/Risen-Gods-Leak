#pragma once
#include "Creatures.h"
#include "BossScript.h"

#define BOSS_SCRIPT(NAME) USER_SCRIPT_BASE(Creature, Boss, NAME, Scripts::Boss)
