#pragma once
#include "BaseOwner.h"

class Map;
class WorldObject;

struct MapHandle
{
    using Stored = Map;

    MapHandle();
    ~MapHandle();
    MapHandle(Stored* map);
    MapHandle(Stored& map);

    Stored* Load() const;

    std::string Print() const;

    static Stored* FromString(std::string_view string, script::Scriptable&);

private:

    Map* map;
};

SCRIPT_OWNER_HANDLE(Map, script::Scriptable, MapHandle)
