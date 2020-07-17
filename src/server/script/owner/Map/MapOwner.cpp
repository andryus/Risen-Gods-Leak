#include "MapOwner.h"
#include "WorldObject.h"
#include "Map.h"

MapHandle::MapHandle() = default;
MapHandle::~MapHandle() = default;

MapHandle::MapHandle(Stored* map) :
    map(map) {}

MapHandle::MapHandle(Stored& map) :
    MapHandle(&map) {}

MapHandle::Stored* MapHandle::Load() const
{
    return map;
}

MapHandle::Stored* MapHandle::FromString(std::string_view string, script::Scriptable &)
{
    return nullptr;
}

std::string MapHandle::Print() const
{
    return "Map ???";
}

SCRIPT_OWNER_IMPL(Map, script::Scriptable)
{
    return dynamic_cast<Map*>(&base);
}
