#pragma once
#include <chrono>
#include "API/ModuleScript.h"

#include "UnitOwner.h"
#include "SpellOwner.h"

enum class AbilityTarget : uint8
{
    AUTO,
    ME,
    TARGET,
    RANDOM
};

struct SpellAbility
{
    constexpr SpellAbility() = default;
    constexpr SpellAbility(SpellId spellId, AbilityTarget target = {}) :
        spellId(spellId), target(target) {}
    constexpr SpellAbility(SpellId spellId, TextLine say, AbilityTarget target = {}) :
        spellId(spellId), say(say), target(target) {}
    constexpr SpellAbility(SpellId spellId, GameDuration cooldown, AbilityTarget target) :
        spellId(spellId), cooldown(cooldown), initialDelay(cooldown), target(target) {}
    constexpr SpellAbility(SpellId spellId, GameDuration cooldown, TextLine say = {}, AbilityTarget target = {}) :
        spellId(spellId), cooldown(cooldown), initialDelay(cooldown), say(say), target(target)  {}
    constexpr SpellAbility(SpellId spellId, GameDuration initialDelay, GameDuration cooldown, AbilityTarget target) :
        spellId(spellId), initialDelay(initialDelay), cooldown(cooldown), target(target) {}
    constexpr SpellAbility(SpellId spellId, GameDuration initialDelay, GameDuration cooldown, TextLine say ={}, AbilityTarget target = {}) :
        spellId(spellId), initialDelay(initialDelay), cooldown(cooldown), say(say), target(target) {}

    operator SpellId() const { return spellId; }

    SpellId spellId = 0;
    GameDuration cooldown = {};
    GameDuration initialDelay = {};
    TextLine say = {};
    AbilityTarget target = AbilityTarget::AUTO;
};

SCRIPT_PRINTER(SpellAbility);

///////////////////////////////////////
/// Abilities
///////////////////////////////////////
     
SCRIPT_MODULE(Ability, Unit);

/************************************
* ACTIONS
************************************/

SCRIPT_ACTION_EX(AddAbility,             Scripts::Ability, SpellAbility, ability)
SCRIPT_ACTION_EX(RemoveAbility,          Scripts::Ability, SpellAbility, ability)
SCRIPT_ACTION_EX(ExecuteAbility,         Unit, SpellAbility, ability)

SCRIPT_ACTION(DisableAbilities, Scripts::Ability)
