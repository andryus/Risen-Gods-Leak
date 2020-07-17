#pragma once
#include "Define.h"
#include "EnumArray.h"

namespace combat
{
    enum class AttackTableEntry : uint8
    {
        Miss,
        Resist,
        Deflect,
        Dodge,
        Parry,
        Glancing,
        Block,
        Crit,
        Crushing,
        SIZE,
    };

    using AttackModTable = EnumArray<float, AttackTableEntry>;
    using AttackCheckTable = EnumArray<bool, AttackTableEntry>;

    struct BaseTable
    {
        AttackModTable mods;
        AttackCheckTable checks;
    };

}
