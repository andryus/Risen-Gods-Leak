#pragma once
#include "WorldObjectHooks.h"
#include "CreatureOwner.h"

class WorldObject;
class GameObject;
class Player;

// base
SCRIPT_EVENT(Visible, Unit);
SCRIPT_EVENT(Invisible, Unit);

SCRIPT_EVENT(ReachedHome, Unit);
SCRIPT_EVENT_PARAM(Noticed, Unit, Unit*, target);

// combat
SCRIPT_EVENT_PARAM(EnterCombat, Unit,     Unit*, initiator);
SCRIPT_EVENT_PARAM(Death,       Unit,     Unit*, killer);
SCRIPT_EVENT_PARAM(Kill,        Unit,     Unit*, victim);
SCRIPT_EVENT(LeaveCombat,       Unit);    
SCRIPT_EVENT(Evade,             Unit);
SCRIPT_EVENT(CheckThreat,       Unit);

SCRIPT_EVENT_EX(AttackTimer,    Unit,     AttackType, attack);
SCRIPT_EVENT_PARAM(DamageTaken, Unit,     DamageParams, damage);


// summoning
SCRIPT_EVENT_PARAM(SummonGO, Unit, GameObject*, go);
SCRIPT_EVENT_EX_PARAM(SummonCreature, Unit, CreatureEntry, entry, Creature*, summon);

// quest
SCRIPT_EVENT_EX_PARAM(GossipOption, Unit, uint8, option, Player*, player)
SCRIPT_EVENT_EX(QuestReward, Unit, uint32, questId)
SCRIPT_EVENT_PARAM(SpellClick, Unit, Unit*, clicker)

// movement
SCRIPT_EVENT_EX(WaypointReached, Unit, uint8, pointId)
SCRIPT_EVENT(PathEnded, Unit)
SCRIPT_EVENT(PointReached, Unit)


SCRIPT_QUERY_PARAM(DamageTakenFromMultiplier, Unit, float, Unit*, unit) { return 1.0f;}
SCRIPT_QUERY_PARAM(DamageDealtToMultiplier, Unit, float, Unit*, target) { return 1.0f; }

SCRIPT_QUERY_EX_PARAM(CanSeeGossipOption, Unit, bool, uint8, gossipOption, Player*, player) { return true;}
