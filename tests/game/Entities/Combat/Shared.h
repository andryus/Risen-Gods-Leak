#pragma once
#include "Combat/ICombatant.h"
#include "Combat/CombatDefines.h"
#include "Combat/CombatFormulas.h"

using namespace combat;

// players can't crush or be glanced, dodge is position-dependant.
class PlayerMock final : public ICombatant
{
    void EvaluateAttackerTable(AttackCheckTable& table, const AttackerParams& attacker) const override
    {
        table[AttackTableEntry::Crushing] = false;
    }

    void EvaluateVictimTable(AttackCheckTable& table, const VictimParams& victim) const override
    {
        table[AttackTableEntry::Dodge] &= !victim.isAttackedFromBehind;
        table[AttackTableEntry::Glancing] = false;
    }
};

// does nothing special w/o game-supplied data
class BossMock final : public ICombatant
{
    void EvaluateAttackerTable(AttackCheckTable& table, const AttackerParams& attacker) const override
    {
        table[AttackTableEntry::Glancing] = false;
    }

    void EvaluateVictimTable(AttackCheckTable& table, const VictimParams& victim) const override
    {
    }
};

const PlayerMock player;
const BossMock boss;

inline const ICombatant& selectCombatant(bool isPlayer)
{
    return isPlayer ? (const ICombatant&)player : (const ICombatant&)boss;
}

inline AttackerParams createAttacker(bool isPlayer, uint8 level)
{
    AttackerParams attacker = { selectCombatant(isPlayer) };
    attacker.isPlayer = isPlayer;
    attacker.crit = 5.0f;
    attacker.hit = 0.0f;
    attacker.expertise = 0.0f;
    attacker.isRanged = false;
    attacker.weaponSkill = level * 5;

    return attacker;
}

inline VictimParams createVictim(bool isPlayer, uint8 level)
{
    VictimParams victim = { selectCombatant(isPlayer) };
    victim.isPlayer = isPlayer;
    victim.miss = 5.0f;
    victim.block = 5.0f;
    victim.dodge = 5.0f;
    victim.parry = 5.0f;
    victim.isAttackedFromBehind = false;
    victim.defenseSkill = level * 5;
    victim.cantDefend = false;
    victim.level = level;
    victim.resist = 0.0f;
    victim.deflect = 0.0f;

    return victim;
}
