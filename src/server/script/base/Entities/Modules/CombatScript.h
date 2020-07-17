#pragma once
#include "API/ModuleScript.h"

#include "BaseScript.h"
#include "UnitHooks.h"
#include "CreatureOwner.h"


///////////////////////////////////////
/// Combat
///////////////////////////////////////

SCRIPT_MODULE(Combat, Unit)

/************************************
* ACTIONS
************************************/
                             
SCRIPT_ACTION1(EngageWith,      Unit,     Unit&, target);
SCRIPT_ACTION1(AttackTarget,    Unit,     Unit&, victim);
SCRIPT_ACTION_EX1(Attack,       Unit,     AttackType, atk, Unit&, victim);

SCRIPT_ACTION(MakeUnkillable,   Unit);
SCRIPT_ACTION(MakeAttackable, Unit);
SCRIPT_ACTION(MakeUnattackable, Unit);
SCRIPT_ACTION(DisableAutoAttack, Scripts::Combat);
SCRIPT_ACTION(EnableRangedAttack, Scripts::Combat);

SCRIPT_ACTION(RestoreHealth,    Unit);    
SCRIPT_ACTION(RemoveFromFight, Creature);

SCRIPT_ACTION1(AssistAttack, Unit, Unit&, target)
                                              
/************************************
* FILTER
************************************/

SCRIPT_FILTER(InCombat,         Unit);
SCRIPT_FILTER1(WithinMeleeRange,Unit,     Unit&, victim);
SCRIPT_FILTER1(HostileTo,       Creature, Unit&, victim);
SCRIPT_FILTER(Dead, Unit)
SCRIPT_FILTER(IsAttackable, Unit)

/************************************
* GETTER
************************************/
                            
SCRIPT_GETTER(Health,         Unit,     ResourceParam);
SCRIPT_GETTER1(HealthAfterDmg, Unit,    ResourceParam, DamageParams, dmg);
SCRIPT_GETTER(Victim,         Unit, Unit*);
SCRIPT_GETTER(NewVictim,      Creature, Unit*);

/************************************
* MACROS
************************************/

SCRIPT_FILTER_EX_INLINE(HealthBelowPct, Unit, uint8, pct)
{
    const ResourceParam health = me <<= Get::Health;
    
    return me <<= If::BelowPct(pct).Bind(health);
}

SCRIPT_FILTER_EX1_INLINE(HealthBeingReducedBelowPct, Unit, uint8, pct, DamageParams, damage)
{
    const bool healthBelowPct = me <<= If::HealthBelowPct(pct);

    return !healthBelowPct && (me <<= (Get::HealthAfterDmg.Bind(damage) |= If::BelowPct(pct)));
}

SCRIPT_MACRO_DATA(On, HealthReducedBelow, CALL, uint8, pct)
{
    return On::DamageTaken
        |= If::HealthBeingReducedBelowPct(pct)
            |= Exec::Once
                |= CALL;
}

SCRIPT_MACRO(On, FatalDamage, CALL)
{
    return On::DamageTaken
        |= Get::HealthAfterDmg |= If::Below(1) 
            |= Exec::Once 
                |= CALL;
}
