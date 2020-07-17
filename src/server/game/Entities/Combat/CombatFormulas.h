#pragma once
#include "CombatDefines.h"

namespace combat
{
    class ICombatant;

    struct AttackerParams
    {
        AttackerParams(const ICombatant& attacker) : attacker(attacker) {}

        const ICombatant& attacker;
        bool isPlayer;
        bool isRanged;
        float hit;
        float crit;
        float expertise;
        uint32 weaponSkill;
    };

    struct VictimParams
    {
        VictimParams(const ICombatant& victim) : victim(victim) {}

        const ICombatant& victim;
        bool isPlayer;
        bool isAttackedFromBehind;
        bool cantDefend;
        uint8 level;
        float miss;
        float dodge;
        float parry;
        float block;
        float resist;
        float deflect;
        uint32 defenseSkill;
    };

    class Formulas
    {
    public:

        static BaseTable CreateBaseTable(AttackerParams attacker, VictimParams victim);

    private:

        struct AttackParams
        {
            AttackerParams attacker;
            VictimParams victim;
        };

        static BaseTable BuildAttackTableBase(AttackParams params, int32 skillDifference);
        static void FinalizeAttackTable(AttackParams params, BaseTable& table, int32 skillDifference, uint8 victimLevel);

        static void ApplySkillMods(AttackModTable& mods, bool attackerIsPlayer, bool victimIsPlayer, int32 skillDifference, int32 skillDiff2);

    };


}
