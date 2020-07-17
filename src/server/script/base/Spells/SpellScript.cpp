#include "Spells/SpellScript.h"
#include "UnitScript.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "Player.h"
#include "SpellHistory.h"

/************************************
* ACTION
************************************/

SCRIPT_ACTION_IMPL(Cast)
{
    me.CastSpell(nullptr, spellId.id);
}

SCRIPT_ACTION_IMPL(CastOn)
{
    me.CastSpell(&target, spellId.id);
}

SCRIPT_ACTION_IMPL(CastGO)
{
    me.CastSpell(nullptr, spellId.id);
}

SCRIPT_ACTION_IMPL(Interrupt)
{
    me.InterruptNonMeleeSpells(false);
}

SCRIPT_ACTION_IMPL(RemoveSpellCooldown)
{
    if (const SpellInfo* spell = sSpellMgr->GetSpellInfo(spellId.id))
    {
        if (uint32 category = spell->GetCategory())
        {
            me.GetSpellHistory()->ResetCooldowns([category](SpellHistory::CooldownStorageType::iterator itr) -> bool
            {
                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itr->first);
                return spellInfo && spellInfo->GetCategory() == category;
            }, true);
        }
        else
            me.GetSpellHistory()->ResetCooldown(spellId.id, true);
    }
}

/************************************
* FILTER
************************************/

SCRIPT_FILTER_IMPL(Casting)
{
    return me.IsNonMeleeSpellCast(false);
}

SCRIPT_FILTER_IMPL(CanCastSpell)
{
    if (const SpellInfo* spell = sSpellMgr->GetSpellInfo(spellId.id))
    {
        if (me <<= !If::Casting)
            return true;

        if (me <<= If::Pacified)
            return false;

        return spell->HasAttribute(SPELL_ATTR4_CAN_CAST_WHILE_CASTING);
    }
    else
        return false;
}

/************************************
* SELECTOR
************************************/

SCRIPT_GETTER_IMPL(CreatureSpell)
{
    if (id < CREATURE_MAX_SPELLS)
        return me.m_spells[id];
    else
        return 0;
}


SCRIPT_SELECTOR_IMPL(SpellCaster)
{
    return me.GetCaster();
}


SCRIPT_SELECTOR_IMPL(SpellTarget)
{
    return me.GetUnitTarget();
}
