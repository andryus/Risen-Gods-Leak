#include "AreaOwner.h"
#include "MapArea.h"

SCRIPT_OWNER_SHARED_IMPL(Area, script::Scriptable)
{
    return dynamic_cast<Area*>(&base);
}

SCRIPT_COMPONENT_IMPL(Area, Map)
{
    return &owner.GetMap();
}
