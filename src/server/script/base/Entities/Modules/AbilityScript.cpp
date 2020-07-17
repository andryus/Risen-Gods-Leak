#include "AbilityScript.h"

#include "RegularEventScript.h"
#include "TimedEventScript.h"
#include "UnitScript.h"
#include "UnitHooks.h"
#include "SpellScript.h"
#include "AuraScript.h"
#include "CombatScript.h"
#include "AuraHooks.h"

#include "SpellInfo.h"
#include "SpellMgr.h"

SCRIPT_PRINTER_IMPL(SpellAbility)
{
    std::string out = script::print(value.spellId);

    if (value.initialDelay && value.initialDelay != value.cooldown)
        out += ", Delay:" + script::print(value.initialDelay);

    if (value.cooldown)
        out += ", CD: " + script::print(value.cooldown);

    if (value.say.id != 255)
        out += ", Text: " + script::print(value.say);

    return out;
}

SCRIPT_MODULE_STATE_IMPL(Ability)
{
    std::vector<SpellAbility> abilities;
    bool enabled = true;

    SCRIPT_MODULE_PRINT(Ability)
    {
        return std::to_string(abilities.size()) + " Abilities(s), Enabled: " + script::print(enabled);
    }
};

SCRIPT_FILTER_EX_INLINE(CanExecuteAbilities, Scripts::Ability, bool, direct)
{
    return direct || me.enabled;
}

SCRIPT_ACTION_EX_INLINE(CastAbility, Scripts::Ability, SpellAbility, ability)
{
    if (ability.target == AbilityTarget::AUTO)
    {
        const bool success = me <<= Do::Cast(ability);

        return SetSuccess(success);
    }

    Unit* caster = script::castSourceTo<Unit>(me);
    ASSERT(caster);

    Unit* target = nullptr;

    switch(ability.target)
    {
    case AbilityTarget::ME:
        target = caster;
        break;
    case AbilityTarget::TARGET:
        target = me <<= Get::Victim;
        break;
    case AbilityTarget::RANDOM:
    {
        const auto targets = me <<= Get::UnitsInCombatWith;
        const SpellInfo* info = sSpellMgr->GetSpellInfo(ability.spellId.id);
        if (!info)
            return SetSuccess(false);

        script::List<Unit*> validTargets;
        for (Unit* target : targets)
        {
            const float distance = caster->GetDistance(target);

            if (distance >= info->GetMinRange() && distance <= info->GetMaxRange())
                validTargets.push_back(target);
        }

        if (!validTargets.empty())
            target = Trinity::Containers::SelectRandomContainerElement(validTargets);

        break;
    }
    }

    if (target)
    {
        const bool success = me <<= Do::CastOn(ability).Bind(*target);

        return SetSuccess(success);
    }
    else
        SetSuccess(false);
}

struct ExecutedAbility
{
    SpellAbility ability;
    bool direct;
};

SCRIPT_MACRO_DATA(Util, ExecuteAbilityImpl, ON_EXEC, ExecutedAbility, exec)
{
    const SpellAbility ability = exec.ability;
    const bool isAuto = ability.target == AbilityTarget::AUTO;

    return Exec::UntilSuccess
        |= If::CanCastSpell(ability.spellId) |= If::CanExecuteAbilities(exec.direct) |= BeginBlock
            += Do::CastAbility(ability) 
            += ON_EXEC
            += Do::Talk(ability.say)
        EndBlock;
}

SCRIPT_ACTION_EX_INLINE(ScheduleAbility, Scripts::Ability, ExecutedAbility, ability)
{
    me <<= Exec::After(ability.ability.cooldown) 
        |= Util::ExecuteAbilityImpl(ability) 
            |= Do::ScheduleAbility(ability);
}

SCRIPT_ACTION_EX_INLINE(InitScheduleAbility, Scripts::Ability, ExecutedAbility, ability)
{
    me <<= Exec::After(ability.ability.initialDelay)
        |= Util::ExecuteAbilityImpl(ability)
        |= Do::ScheduleAbility(ability);
}

SCRIPT_ACTION_INLINE(StartAbilities, Scripts::Ability)
{
    for(SpellAbility ability : me.abilities)
    {
        me <<= Do::InitScheduleAbility({ability, false});
    }
}

// undo

SCRIPT_ACTION1_INLINE(StoreAuraUndo, Unit, Aura&, aura)
{
    AddUndoableWith(Do::RemoveAura, &aura);
}

SCRIPT_MODULE_IMPL(Ability)
{
    me <<= On::EnterCombat
        |= Do::StartAbilities;

    me <<= On::AuraApplied::Any()
        |= Do::StoreAuraUndo;
}

/************************************
* ACTIONS
************************************/

SCRIPT_ACTION_IMPL(AddAbility)
{
    me.abilities.push_back(ability);
}

SCRIPT_ACTION_IMPL(RemoveAbility)
{
    //const auto itr = std::remove_if(abilities.begin(), abilities.end(), [ability](SpellAbility stored) { return stored.spellId == ability.spellId; });
    //abilities.erase(itr);
}

SCRIPT_ACTION_IMPL(ExecuteAbility)
{
    me <<= Util::ExecuteAbilityImpl({ability, true}) |= script::Ignore;
}

SCRIPT_ACTION_INLINE(EnableAbilities, Scripts::Ability)
{
    me.enabled = true;
}

SCRIPT_ACTION_IMPL(DisableAbilities)
{
    me.enabled = false;

    AddUndoable(Do::EnableAbilities);
}
