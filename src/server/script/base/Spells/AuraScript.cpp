#include "AuraScript.h"
#include "SpellAuras.h"

/************************************
* ACTIONS
************************************/

SCRIPT_ACTION_IMPL(AddAura)
{
    if (Aura* aura = me.AddAura(spellId.id, &me))
       AddUndoableWith(Do::RemoveAura, aura);
    else
        SetSuccess(false);
}

SCRIPT_ACTION_IMPL(RemoveAura)
{
    me.RemoveAura(&aura);
}

SCRIPT_ACTION_IMPL(RemoveAuraBySpellId)
{
    me.RemoveAurasDueToSpell(spellId.id);
}

SCRIPT_ACTION_IMPL(DeleteAura) 
{
    me.Remove();
}

/************************************
* FILTER
************************************/

SCRIPT_FILTER_IMPL(HasAura)
{
    return me.HasAura(spellId.id);
}

/************************************
* MODIFIER
************************************/

SCRIPT_GETTER_IMPL(NumAuraStacks)
{
    if (Aura* aura = me.GetAura(spellId.id))
        return aura->GetStackAmount();
    else
        return 0;
}

SCRIPT_SELECTOR_IMPL(GetAura) 
{
    return me.GetAura(spellId.id);
}

SCRIPT_SELECTOR_IMPL(AuraCaster)
{
    return me.GetCaster();
}

SCRIPT_SELECTOR_IMPL(EffectTarget)
{
    return me.GetApplicationMap().begin()->second->GetTarget();
}
