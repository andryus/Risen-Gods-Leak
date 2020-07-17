#pragma once
#include "BaseOwner.h"

struct GameEventId
{
    GameEventId() = default;
    constexpr GameEventId(uint32 id) : 
        id(id) {}

    uint32 id = 0;
};

SCRIPT_PRINTER(GameEventId)
