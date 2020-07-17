#pragma once
#include "BaseScript.h"
#include "WorldObjectOwner.h"

/************************************
* ACTION
************************************/

SCRIPT_ACTION(MakeAlwaysActive, WorldObject)

/************************************
* MODIFIER
************************************/

SCRIPT_GETTER(Location, WorldObject, ::Position)
