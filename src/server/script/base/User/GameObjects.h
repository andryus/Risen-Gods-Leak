#pragma once
#include "WorldObjects.h"
#include "GameObjectOwner.h"
#include "GameObjectHooks.h"
#include "Entities/GameObjectScript.h"

#define GO_SCRIPT(NAME) USER_SCRIPT_BASE(GameObject, Go, NAME)
