#pragma once
#include "UnitOwner.h"

///////////////////////////////////////
/// Creature
///////////////////////////////////////

class Creature;

struct CreatureEntry
{
    CreatureEntry() = default;
    constexpr CreatureEntry(uint32 entry) : entry(entry) {}

    explicit operator uint32() const { return entry; }

    uint32 entry = 0;
};

SCRIPT_PRINTER(CreatureEntry);

SCRIPT_OWNER(Creature, Unit)
