#pragma once
#include "BaseHooks.h"

#include "UnitOwner.h"
#include "SpellOwner.h"

SCRIPT_EVENT(CastStart, Spell)
SCRIPT_EVENT(CastSuccess, Spell)
SCRIPT_EVENT_EX_PARAM(SpellCast, Unit, SpellId, spellId, Spell*, spell)

SCRIPT_EVENT_PARAM(HitTarget, Spell, Unit*, target)
SCRIPT_EVENT_EX_PARAM(HitBySpell, Unit, SpellId, spellId, Spell*, spell)

SCRIPT_QUERY(CanCast, Spell, bool) { return true; }
SCRIPT_QUERY_PARAM(ValidTargetForSpell, Spell, bool, Unit*, unit) { return true; }
SCRIPT_QUERY_EX_PARAM(EffectPullTowardsDest, Spell, Position, uint8, effIndex, Position, target) { return target; }
