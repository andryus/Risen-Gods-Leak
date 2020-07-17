#pragma once
#include "BaseHooks.h"
#include "SpellOwner.h"
#include "AuraOwner.h"

/************************************
* EVENTS
************************************/

SCRIPT_EVENT_EX(EffectPeriodic, Aura, uint8, effectId)

SCRIPT_EVENT_PARAM(AppliedOn, Aura, Unit*, target)
SCRIPT_EVENT_EX_PARAM(AuraApplied, Unit, SpellId, spellId, Aura*, aura)
SCRIPT_EVENT_PARAM(GainedStack, Aura, uint8, numStacks)

SCRIPT_EVENT_EX_PARAM(ReceivedAura, Unit, SpellId, spellId, Aura*, aura)
SCRIPT_EVENT_EX_PARAM(LostAura, Unit, SpellId, spellId, Aura*, aura)
