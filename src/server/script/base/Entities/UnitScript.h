#pragma once
#include "API/OwnerScript.h"
#include "UnitOwner.h"
#include "PlayerOwner.h"

/************************************
* ACTIONS
************************************/

SCRIPT_ACTION_EX(Talk, Unit, TextLine, text);
SCRIPT_ACTION_EX1(TalkTo, Unit, TextLine, text, Unit&, target);
SCRIPT_ACTION1(FaceTo, Unit, WorldObject&, target)
SCRIPT_ACTION_EX(FaceDirection, Unit, float, orientation)


SCRIPT_ACTION(Show, Unit)
SCRIPT_ACTION(Hide, Unit)
SCRIPT_ACTION(HideModel, Unit)
SCRIPT_ACTION_EX(Morph, Unit, uint32, modelId);
SCRIPT_ACTION_EX(ChangeFaction, Unit, FactionType, faction);

SCRIPT_ACTION(Die, Unit)
SCRIPT_ACTION(MakeSelectable, Unit)
SCRIPT_ACTION(MakeUnselectable, Unit)
SCRIPT_ACTION(MakeEvade, Unit)

SCRIPT_ACTION(DisablePlayerInteraction, Unit)
SCRIPT_ACTION(DisableGossip, Unit)
SCRIPT_ACTION(MakeQuestGiver, Unit)
SCRIPT_ACTION(RemoveQuestGiver, Unit)
SCRIPT_ACTION(Kneel, Unit)
SCRIPT_ACTION(StandUp, Unit)
SCRIPT_ACTION(Pacify, Unit)
SCRIPT_ACTION(Dismount, Unit)
SCRIPT_ACTION(StopEmote, Unit)
SCRIPT_ACTION_EX(MakeSheath, Unit, bool, ranged)

SCRIPT_ACTION_EX(TeleportTo, Unit, Position, position)

/************************************
* FILTER
************************************/

SCRIPT_FILTER(Visible, Unit)
SCRIPT_FILTER(Sitting, Unit)
SCRIPT_FILTER(Alive, Unit)
SCRIPT_FILTER(Pacified, Unit)
SCRIPT_FILTER(IsNPC, Unit)
SCRIPT_FILTER_EX(IsOfFaction, Unit, FactionType, faction)
SCRIPT_FILTER1(PrimaryTargetOf, Unit, Unit&, attacker)
SCRIPT_FILTER_EX1(WithinRange, Unit, float, range, Unit&, target)

/************************************
* MODIFIER
************************************/

SCRIPT_GETTER(ForwardDirection, Unit, Position)
SCRIPT_GETTER1(DirectionTo, Unit, Position, Unit&, target)
SCRIPT_GETTER(UnitsInCombatWith, Unit, script::List<Unit*>)

SCRIPT_SELECTOR(Owner, Unit, Unit*)
