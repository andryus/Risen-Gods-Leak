#pragma once
#include "WorldObjectOwner.h"

class Unit;

///////////////////////////////////////
/// Unit
///////////////////////////////////////

SCRIPT_OWNER(Unit, WorldObject);

struct DamageParams
{
    constexpr DamageParams() = default;
    DamageParams(Unit& unit, uint32 value) : unit(&unit), value(value) {}

    operator Unit*() const { return unit; }

    Unit* unit = nullptr;
    uint32 value = 0;
};

struct TextLine
{
    constexpr TextLine() = default;
    constexpr TextLine(uint8 id) :
        id(id) {}
    constexpr TextLine(uint8 id, uint8 chance) :
        id(id), chance(chance / 100.0f) {}

    uint8 id = 255;
    float chance = 1.0f;
};

SCRIPT_PRINTER(TextLine)

struct FactionType
{
    FactionType() = default;
    constexpr FactionType(uint32 id) :
        id(id) {}
    
    uint32 id = -1;
};

namespace Faction
{
    constexpr FactionType ENEMY = 14;
    constexpr FactionType FRIENDLY_AH = 35;
    constexpr FactionType ENEMY_SPAR = 2150;
}

enum class AttackType : uint8
{
    MainHand,
    OffHand,
    Ranged,
    Count
};
