#include "CombatScript.h"
#include "Creature.h"

#include "BaseScript.h"
#include "StateScript.h"
#include "TimedEventScript.h"
#include "RegularEventScript.h"
#include "UnitHooks.h"
#include "UnitScript.h"
#include "CreatureScript.h"
#include "SpellScript.h"
#include "MovementScript.h"
#include "Grids/Notifiers/GridNotifiers.h"

SCRIPT_MODULE_STATE_IMPL(Combat) 
{
    bool canAttack = true;
    bool canAttackRanged = false;

    SCRIPT_MODULE_PRINT(Combat) 
    { 
        return "AutoAttack: " + script::print(canAttack) + ", RangedAttack: " + script::print(canAttackRanged);
    }
};

SCRIPT_FILTER_EX_INLINE(CanAttackWithStyle, Creature, AttackType, type)
{
    bool canAttack = true;
    if (me.HasScript())
    {
        Scripts::Combat& module = Module<Scripts::Combat>();

        canAttack &= (type == AttackType::Ranged) ? module.canAttackRanged : module.canAttack;
    }

    if (type == AttackType::OffHand)
        canAttack &= me.haveOffhandWeapon();

    return canAttack;
}

SCRIPT_FILTER_EX1_INLINE(InAttackRangeFor, Creature, AttackType, type, Unit&, target)
{
    const bool inMeleeRange = me <<= If::WithinMeleeRange.Bind(target);

    return type == AttackType::Ranged ? !inMeleeRange : inMeleeRange;
}

SCRIPT_FILTER1_INLINE(CanEngage, Unit, Unit&, target)
{
    if (Creature* creature = script::castSourceTo<Creature>(me))
        return creature->CanStartAttack(&target, false);

    return true;
}

SCRIPT_ACTION_EX_INLINE(ResetAttackTimer, Unit, AttackType, attack)
{
    me.resetAttackTimer((WeaponAttackType)attack);
}

SCRIPT_ACTION_EX1_INLINE(ExecuteAttack, Unit, AttackType, attack, Unit&, target)
{
    constexpr bool ignoreLOS = false;

    if (attack == AttackType::Ranged)
        me <<= Do::CastOn(6660).Bind(target);
    else
        me.AttackerStateUpdate(&target, (WeaponAttackType)attack, false, ignoreLOS);
}

SCRIPT_ACTION_EX_INLINE(StartAttack, Unit, AttackType, type)
{
    me <<= If::CanAttackWithStyle(type) 
        |= Do::ResetAttackTimer(type);
}

///////////////////////////////////////
/// Combat
///////////////////////////////////////

SCRIPT_ACTION_INLINE(Evade, Unit)
{
    if (me.IsInCombat())
        me.CombatStop(true);

    me.GetThreatManager().ClearAllThreat(); // @todo: hack, should be removed (fix threat)

    if (!me.GetVehicle()) // otherwise me will be in evade mode forever
        me <<= Exec::StateEvent(On::ReachedHome) |= Do::MakeEvade;
    else
        ASSERT(false);
}

SCRIPT_ACTION_INLINE(CheckTargetsOnSpawn, Unit)
{
    Trinity::AllWorldObjectsInRange rangedVisitor(&me, me.GetSightRange());

    std::vector<Unit*> units;
    me.VisitAnyNearbyObject<Unit, Trinity::ContainerAction>(me.GetSightRange(), units);

    std::sort(units.begin(), units.end(), [&me = me](const Unit* a, const Unit* b) { return me.GetDistance(a) < me.GetDistance(b); });

    bool success = false;
    for (Unit* unit : units)
        success |= Trinity::onNoticed(me, *unit);

    SetSuccess(success);
}

SCRIPT_ACTION_INLINE(QueryVictim, Creature)
{
    VictimAction action = VictimAction::None;
    
    Unit* victim = me.QueryVictim(action);

    if (action == VictimAction::Select)
    {
        if (victim != me.GetVictim())
            me <<= Do::AttackTarget.Bind(*victim);
        else
            SetSuccess(false);
    }
    else if(action != VictimAction::None)
        me <<= Fire::Evade;
    else
        SetSuccess(false);
}

void registerAutoAttacks(script::ConstructionCtx<Unit> me)
{
    for (uint8 i = 0; i < (uint8)AttackType::Count; i++)
    {    
        const AttackType type = (AttackType)i;

        me <<= On::AttackTimer(type)
            |= If::InCombat |= If::CanAttackWithStyle(type)
                |= Exec::UntilSuccess 
                    |= Get::Victim 
                    |= If::ParamValid && If::CanAttackWithStyle(type) && If::InAttackRangeFor(type) |= BeginBlock
                        += Do::ExecuteAttack(type)
                        += Do::ResetAttackTimer(type)
                    EndBlock;
    }
}

SCRIPT_MODULE_IMPL(Combat)
{
    // 1s after spawn, check for any nearby targets
    // delay guarantees subtype initialisation is done, and adds a (blizzlike) feel of spawn sickness
    me <<= On::InitWorld
        |= Exec::After(1s) |= Do::CheckTargetsOnSpawn;

    me <<= On::Noticed
        |= !If::InCombat && If::HostileTo && If::CanEngage
            |= Do::EngageWith;

    // Attack damage-causer, and add state to revert everything on a reset
    me <<= On::EnterCombat |= BeginBlock
            += Do::PushStateEvent(On::LeaveCombat)
            += Do::AttackTarget
            += Do::StartAttack::Multi(
                AttackType::MainHand,
                AttackType::OffHand,
                AttackType::Ranged)
    EndBlock;

    registerAutoAttacks(me);

    me <<= On::CheckThreat
        |= If::InCombat |= Do::QueryVictim;

    // restore health to full, once it reached its spawn-point
    me <<= On::ReachedHome
        |= Do::RestoreHealth;

    me <<= On::Visible
        |= Get::NewVictim |= Do::EngageWith;

    // revert all changes done during the fight
    me <<= On::Evade
        |= Do::Evade;
}

/************************************
* ACTIONS
************************************/

SCRIPT_ACTION_IMPL(EngageWith)
{
    me.EngageWithTarget(&target);
}

SCRIPT_ACTION_IMPL(AttackTarget)
{
    if (me.Attack(&victim, true))
    {
        // Clear distracted state on attacking
        if (me.HasUnitState(UNIT_STATE_DISTRACTED))
            me.ClearUnitState(UNIT_STATE_DISTRACTED);

        me <<= Do::MoveChase.Bind(victim);
    }
    else
        SetSuccess(false);
}

SCRIPT_ACTION_IMPL(RestoreHealth)
{
    me.SetFullHealth();
}

SCRIPT_ACTION_IMPL(MakeUnkillable)
{
    me.SetUnkillable(true);
}


SCRIPT_ACTION_IMPL(MakeAttackable)
{
    me.SetImmuneToAll(false);

    AddUndoable(Do::MakeUnattackable);
}

SCRIPT_ACTION_IMPL(MakeUnattackable)
{
    me.SetImmuneToAll(true);

    AddUndoable(Do::MakeAttackable);
}

SCRIPT_ACTION_INLINE(EnableAutoAttack, Scripts::Combat)
{
    me.canAttack = true;
}

SCRIPT_ACTION_IMPL(DisableAutoAttack)
{
    me.canAttack = false;

    AddUndoable(Do::EnableAutoAttack);
}

SCRIPT_ACTION_INLINE(DisableRangedAttack, Scripts::Combat)
{
    me.canAttackRanged = false;
}

SCRIPT_ACTION_IMPL(EnableRangedAttack)
{
    if (!me.canAttackRanged)
    {
        me.canAttackRanged = true;

        // TODO: what if we are in-combat?
        //me <<= If::InCombat | Do::Cast(6660);
    }
}

SCRIPT_ACTION_IMPL(RemoveFromFight)
{
    me.CombatStop(true);
    me.GetThreatManager().ClearAllThreat();
}

SCRIPT_ACTION_IMPL(AssistAttack)
{
    if (Unit* victim = (target <<= Get::Victim))
        me <<= Do::EngageWith.Bind(*victim);
    else
        SetSuccess(false);
}

/************************************
* FILTER
************************************/

SCRIPT_FILTER_IMPL(InCombat)
{
    return me.IsInCombat();
}

SCRIPT_FILTER_IMPL(WithinMeleeRange)
{
    return me.IsWithinMeleeRange(&victim);
}

SCRIPT_FILTER_IMPL(HostileTo)
{
    return (me <<= If::Aggressive) && me.CanStartAttack(&victim, false);
}

SCRIPT_FILTER_IMPL(Dead)
{
    return me.isDead();
}

SCRIPT_FILTER_IMPL(IsAttackable)
{
    return !me.IsImmuneToAll();
}

/************************************
* GETTER
************************************/

SCRIPT_GETTER_IMPL(Health)
{
    return { me.GetHealth(), me.GetMaxHealth() };
}

SCRIPT_GETTER_IMPL(HealthAfterDmg)
{
    const auto health = me <<= Get::Health;

    return ResourceParam((health.value - dmg.value), health.max);
}

SCRIPT_GETTER_IMPL(Victim)
{
    return me.GetVictim();
}

SCRIPT_GETTER_IMPL(NewVictim)
{
    VictimAction action;
    return me.QueryVictim(action);
}
