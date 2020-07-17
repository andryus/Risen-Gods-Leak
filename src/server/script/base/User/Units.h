#pragma once
#include "WorldObjects.h"
#include "UnitOwner.h"
#include "UnitHooks.h"
#include "UnitScript.h"

#include "CombatScript.h"
#include "AbilityScript.h"
#include "MovementScript.h"
#include "SummonerScript.h"
#include "SpellTargetScript.h"

template<>
struct script::OwnerBundle<Unit> : public script::ScriptBundle<Unit, Scripts::Combat, Scripts::Ability, Scripts::Move, Scripts::Summoner, Scripts::SpellTarget> {};
