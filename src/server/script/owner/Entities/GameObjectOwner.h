#pragma once
#include "WorldObjectOwner.h"

///////////////////////////////////////
/// GameObject
///////////////////////////////////////

class GameObject;


struct GameObjectEntry
{
    constexpr GameObjectEntry() = default;
    constexpr GameObjectEntry(uint32 id) : id(id) {}
    constexpr GameObjectEntry(uint32 id, float rotation0, float rotation1, float rotation2, float rotation3) :
        id(id), rotation0(rotation0), rotation1(rotation1), rotation2(rotation2), rotation3(rotation3) {}

    uint32 id = 0;
    float rotation0 = 0.0f;
    float rotation1 = 0.0f;
    float rotation2 = 0.0f;
    float rotation3 = 0.0f;
};

SCRIPT_PRINTER(GameObjectEntry);


SCRIPT_OWNER(GameObject, WorldObject);
