#include "CombatFormulas.h"

#include <algorithm>
#include "ICombatant.h"

namespace combat
{

    BaseTable Formulas::CreateBaseTable(AttackerParams attacker, VictimParams victim)
    {
        const int32 skillDifference = std::min<int32>(victim.defenseSkill, victim.level * 5) - attacker.weaponSkill;
        const AttackParams params = { attacker, victim };

        auto baseTable = BuildAttackTableBase(params, skillDifference);
        FinalizeAttackTable(params, baseTable, skillDifference, victim.level);

        return baseTable;
    }

    BaseTable Formulas::BuildAttackTableBase(AttackParams params, int32 skillDifference)
    {
        const AttackerParams attacker = params.attacker;
        const VictimParams victim = params.victim;

        // Attack table
        // basic modifiers/values

        AttackModTable mods;
        // attacker base values
        mods[AttackTableEntry::Miss] = victim.miss - attacker.hit;
        mods[AttackTableEntry::Crit] = attacker.crit;

        // melee
        mods[AttackTableEntry::Parry] = victim.parry;
        mods[AttackTableEntry::Dodge] = victim.dodge;
        mods[AttackTableEntry::Block] = victim.block;

        // spell/ranged
        mods[AttackTableEntry::Resist] = victim.resist;
        mods[AttackTableEntry::Deflect] = victim.deflect;

        // apply expertise
        mods[AttackTableEntry::Parry] -= attacker.expertise;
        mods[AttackTableEntry::Dodge] -= attacker.expertise;

        const auto canDefend = !(victim.cantDefend || victim.isAttackedFromBehind);

        // todo: better way to check if parry/block/dodge is allowed

        AttackCheckTable checks;
        checks[AttackTableEntry::Miss] = true;
        checks[AttackTableEntry::Resist] = true;
        checks[AttackTableEntry::Deflect] = attacker.isRanged && !victim.isAttackedFromBehind;
        checks[AttackTableEntry::Dodge] = victim.dodge > 0.0f && !victim.cantDefend;
        checks[AttackTableEntry::Parry] = victim.parry > 0.0f && canDefend;
        checks[AttackTableEntry::Glancing] = !attacker.isRanged;
        checks[AttackTableEntry::Block] = victim.block > 0.0f && canDefend;
        checks[AttackTableEntry::Crit] = true;
        checks[AttackTableEntry::Crushing] = true;


        return { mods, checks };
    }

    void Formulas::FinalizeAttackTable(AttackParams params, BaseTable& table, int32 skillDifference, uint8 victimLevel)
    {
        params.attacker.attacker.EvaluateAttackerTable(table.checks, params.attacker);
        params.victim.victim.EvaluateVictimTable(table.checks, params.victim);

        const int32 skillDiff2 = params.victim.defenseSkill - params.attacker.weaponSkill;

        ApplySkillMods(table.mods, params.attacker.isPlayer, params.victim.isPlayer, skillDifference, skillDiff2);
    }

    void Formulas::ApplySkillMods(AttackModTable& mods, bool attackerIsPlayer, bool victimIsPlayer, int32 skillDifference, int32 skillDiff2)
    {
        AttackModTable skillMods;
        auto& missModifier = skillMods[AttackTableEntry::Miss];
        auto& parryModifier = skillMods[AttackTableEntry::Parry];
        auto& dodgeModifier = skillMods[AttackTableEntry::Dodge];
        auto& blockModifier = skillMods[AttackTableEntry::Block];
        auto& critModifier = skillMods[AttackTableEntry::Crit];

        constexpr float PctPerSkill = 0.04f;

        if (attackerIsPlayer && !victimIsPlayer && skillDifference >= 0)
        {
            constexpr float HighPctPerSkill = 0.1f;
            constexpr int32 SkillPerLevel = 5;
            constexpr int32 SkillBreakpoint = SkillPerLevel * 2;

            if (skillDifference <= SkillBreakpoint)
            {
                missModifier = skillDifference * HighPctPerSkill;
                parryModifier = skillDifference * HighPctPerSkill;
            }
            else // when skill-diff is larger then 10, the scaling increases drastically
            {
                const int32 modifiedSkillDiff = skillDifference - SkillBreakpoint;

                missModifier = 1.0f + modifiedSkillDiff * 0.4f;

                if (skillDifference >= SkillBreakpoint + SkillPerLevel) // at 3 levels+, parry chance explodes by 9%
                    parryModifier = 9.0f + PctPerSkill * (modifiedSkillDiff - SkillPerLevel); // we add 0.04% per point exceeding 15 - this is not confirmed
                else
                    parryModifier = skillDifference * HighPctPerSkill;
            }

            dodgeModifier = blockModifier = skillDifference * HighPctPerSkill;
        }
        else
            missModifier = dodgeModifier = parryModifier = blockModifier = skillDifference * PctPerSkill;

        if (skillDifference >= 0)
            mods[AttackTableEntry::Glancing] = std::min((10 + skillDifference) * 0.96f, 40.0f); // 0.96f is for clamping glance to 24% vs boss-mobs
        if (skillDifference <= -20)
            mods[AttackTableEntry::Crushing] = -skillDifference * 2.0f - 15.0f;

        critModifier = -skillDiff2 * PctPerSkill;

        constexpr AttackTableEntry skillEntries[] =
        {
            AttackTableEntry::Miss,
            AttackTableEntry::Parry,
            AttackTableEntry::Dodge,
            AttackTableEntry::Block,
            AttackTableEntry::Crit,
        };

        for (const auto entry : skillEntries)
            mods[entry] += skillMods[entry];
    }

}
