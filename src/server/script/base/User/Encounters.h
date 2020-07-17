#pragma once
#include "Instances.h"
#include "EncounterOwner.h"
#include "EncounterHooks.h"
#include "AchievementScript.h"

#include "EncounterScript.h"

#define ENCOUNTER_SCRIPT(NAME) USER_SCRIPT(Encounter, NAME)

template<>
struct script::OwnerBundle<Encounter> :
    public script::ScriptBundle<Encounter, Scripts::Encounter, Scripts::Achievement> {};
