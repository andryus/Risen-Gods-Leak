#pragma once
#include "BaseScript.h"
#include "SpellOwner.h"
#include "CreatureOwner.h"
#include "GameObjectOwner.h"
#include "PlayerOwner.h"

/************************************
* ACTION
************************************/

// Simply casts a spell 
SCRIPT_ACTION_EX(Cast, Unit, SpellId, spellId);
SCRIPT_ACTION_EX1(CastOn, Unit, SpellId, spellId, Unit&, target);

SCRIPT_ACTION_EX(CastGO, GameObject, SpellId, spellId);

SCRIPT_ACTION(Interrupt, Unit)

SCRIPT_ACTION_EX(RemoveSpellCooldown, Player, SpellId, spellId)

/************************************
* FILTER
************************************/

SCRIPT_FILTER(Casting, Unit);
SCRIPT_FILTER_EX(CanCastSpell, Unit, SpellId, spellId)

/************************************
* SELECTOR
************************************/

SCRIPT_SELECTOR(SpellCaster, Spell, Unit*)
SCRIPT_SELECTOR(SpellTarget, Spell, Unit*)

SCRIPT_GETTER_EX(CreatureSpell, Creature, SpellId, uint8, id)
