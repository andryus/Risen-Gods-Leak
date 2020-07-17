#pragma once
#include "API/OwnerScript.h"
#include "AuraOwner.h"
#include "SpellOwner.h"
#include "UnitOwner.h"

/************************************
* ACTIONS
************************************/

SCRIPT_ACTION_EX(AddAura, Unit, SpellId, spellId)
SCRIPT_ACTION1(RemoveAura, Unit, Aura&, aura)
SCRIPT_ACTION_EX(RemoveAuraBySpellId, Unit, SpellId, spellId)
SCRIPT_ACTION(DeleteAura, Aura)

/************************************
* FILTER
************************************/

SCRIPT_FILTER_EX(HasAura, Unit, SpellId, spellId)

/************************************
* MODIFIER
************************************/

SCRIPT_GETTER_EX(NumAuraStacks, Unit, uint8, SpellId, spellId)

SCRIPT_SELECTOR_EX(GetAura, Unit, Aura*, SpellId, spellId)
SCRIPT_SELECTOR(AuraCaster, Aura, Unit*)
SCRIPT_SELECTOR(EffectTarget, Aura, Unit*)
