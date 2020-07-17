#pragma once
#include "UnitOwner.h"

class Player;

///////////////////////////////////////
/// Player
///////////////////////////////////////

struct QuestId
{
    QuestId() = default;
    constexpr QuestId(uint32 id) : id(id) {}

    uint32 id;
};

SCRIPT_PRINTER(QuestId)

SCRIPT_OWNER(Player, Unit);
