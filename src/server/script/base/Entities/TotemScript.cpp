#include "TotemScript.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "PlayerScript.h"

#include "AbilityScript.h"
#include "CombatScript.h"
#include "MovementScript.h"
#include "CreatureScript.h"

SCRIPT_MODULE_STATE_IMPL(Totem) 
{
    SCRIPT_MODULE_PRINT_NONE
};

SCRIPT_ACTION_INLINE(SetupTotemAbilities, Creature)
{
    auto spell1 = me.m_spells[0];
    if (const SpellInfo* firstSpell = sSpellMgr->GetSpellInfo(spell1))
    {
        if (firstSpell->CalcCastTime())
        {
            me <<= Do::OverrideSightRange(firstSpell->GetMaxRange());
            me <<= Do::AddAbility({spell1});

            if (firstSpell->Effects[0].TargetA.GetReferenceType() == SpellTargetReferenceTypes::TARGET_REFERENCE_TYPE_TARGET)
                me.SetReactState(REACT_AGGRESSIVE);
        }
        else
            me <<= Do::ExecuteAbility({spell1});
    }

    auto spell2 = me.m_spells[1];
    if (const SpellInfo* passiveSpell = sSpellMgr->GetSpellInfo(spell2))
        me <<= Do::ExecuteAbility({spell2});
}

SCRIPT_MODULE_IMPL(Totem)
{
    me <<= On::InitWorld |= BeginBlock
        += Do::DisableAutoAttack
        += Do::DisableMovement
        += Do::SetupTotemAbilities
    EndBlock;

    me <<= On::SummonedBy |= If::Aggressive |= BeginBlock
        += *If::InCombat |= Do::AssistAttack
        += ~(!If::IsPlayer 
            |= *On::EnterCombat |= Do::EngageWith)
        += *On::EnterCombat |= Do::EngageWith
    EndBlock;
}
