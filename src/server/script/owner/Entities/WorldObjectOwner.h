#pragma once
#include "BaseOwner.h"
#include "Position.h"

///////////////////////////////////////
/// WorldObject
///////////////////////////////////////

class Map;
class InstanceMap;
class Encounter;
class MapArea;
class WorldObject;

struct WorldObjectHandle
{
    using Stored = WorldObject;

    WorldObjectHandle() = default;
    WorldObjectHandle(const WorldObject& object);
    WorldObjectHandle(const WorldObject* object);

    WorldObject* Load() const;

    std::string Print() const;

    static WorldObject* FromString(std::string_view guid, script::Scriptable& ctx);
private:

    Map* map = nullptr;
    uint64 guid = 0;
};

SCRIPT_OWNER_HANDLE(WorldObject, script::Scriptable, WorldObjectHandle);

SCRIPT_COMPONENT(WorldObject, MapArea)
SCRIPT_COMPONENT(WorldObject, Map)
SCRIPT_COMPONENT(WorldObject, Encounter)

