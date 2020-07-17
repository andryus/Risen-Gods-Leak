#pragma once
#include "API/ModuleScript.h"
#include "CreatureScript.h"

SCRIPT_MODULE(Boss, Creature)

struct EncounterTexts
{
    TextLine aggro, kill, death;
};

SCRIPT_ACTION_EX(AddEncounterTexts, Creature, EncounterTexts, texts)
