#pragma once
#include "Utilities/EnumArray.h"
#include "CombatDefines.h"

namespace combat
{
    // forward-declared
    struct AttackerParams;
    struct VictimParams;

    // used to hide and handle different type of combat entities (ie. Player, Creature...)
    // allows them to turn all entries of the attack table on or off
    class ICombatant
    {
    protected:
        virtual ~ICombatant(void) = 0;

    public:

        virtual void EvaluateAttackerTable(AttackCheckTable& table, const AttackerParams& attacker) const = 0;
        virtual void EvaluateVictimTable(AttackCheckTable& table, const VictimParams& victim) const = 0;

    };

    inline ICombatant::~ICombatant() = default;

}
