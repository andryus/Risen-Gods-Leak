#pragma once
#include "BaseOwner.h"

class Map;
class MapArea;

using Area = MapArea;

///////////////////////////////////////
/// MapArea
///////////////////////////////////////

SCRIPT_OWNER_SHARED(Area, script::Scriptable)

SCRIPT_COMPONENT(Area, Map)
