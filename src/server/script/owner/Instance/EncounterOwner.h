#pragma once
#include "BaseOwner.h"

class Map;
class Encounter;

SCRIPT_OWNER_SHARED(Encounter, script::Scriptable)

SCRIPT_COMPONENT(Encounter, Map)
