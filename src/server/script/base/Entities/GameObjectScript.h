#pragma once
#include "API/OwnerScript.h"
#include "GameObjectOwner.h"

class Unit;


/************************************
* ACTIONS
************************************/

SCRIPT_ACTION_EX1_RET(SummonGO, WorldObject, GameObject*, GameObjectEntry, goEntry, Position, position);
SCRIPT_ACTION1(UseGO, GameObject, Unit&, user);

SCRIPT_ACTION(MakeInteractible, GameObject);
SCRIPT_ACTION(MakeNonInteractible, GameObject);
SCRIPT_ACTION(ActivateGO, GameObject);
SCRIPT_ACTION(MakeGOReady, GameObject);

SCRIPT_ACTION(DespawnGO, GameObject);

/************************************
* FILTER
************************************/

SCRIPT_FILTER_EX(IsGO, WorldObject, GameObjectEntry, goEntry)
