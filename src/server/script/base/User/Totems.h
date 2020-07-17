#pragma once
#include "Creatures.h"
#include "TotemScript.h"

#define TOTEM_SCRIPT(NAME) USER_SCRIPT_BASE(Creature, Npc, NAME, Scripts::Totem)

inline const auto registry = script::Registration<Creature>::CreateDefault<Scripts::Totem>("totem_base");
