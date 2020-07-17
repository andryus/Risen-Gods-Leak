#pragma once
#include "API/ModuleScript.h"
#include "InstanceHooks.h"

SCRIPT_MODULE(Instance, InstanceMap);

SCRIPT_FILTER(InHeroicMode, InstanceMap)
SCRIPT_GETTER(ActiveFactionId, InstanceMap, uint8)
