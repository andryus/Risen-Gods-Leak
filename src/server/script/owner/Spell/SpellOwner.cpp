#include "SpellOwner.h"
#include "UnitOwner.h"
#include "Spell.h"
#include "SpellMgr.h"

SCRIPT_OWNER_IMPL(Spell, script::Scriptable)
{
    return dynamic_cast<Spell*>(&base);
}

SCRIPT_PRINTER_IMPL(SpellId)
{
    if (const SpellInfo* info = sSpellMgr->GetSpellInfo(value.id))
        return '"' + std::string(info->SpellName[0]) + "\"(Spell: " + script::print(value.id) + ')';
    else
        return "(UNKNOWN SPELL: " + script::print(value.id) + ')';
}


SCRIPT_COMPONENT_IMPL(Spell, Map)
{
    return script::castSourceTo<Map>(owner.GetCaster());
}


SCRIPT_COMPONENT_IMPL(Spell, MapArea)
{
    return script::castSourceTo<MapArea>(owner.GetCaster());
}

SCRIPT_COMPONENT_IMPL(Spell, Encounter)
{
    Unit* caster = script::castSourceTo<Unit>(owner.GetCaster());

    return script::castSourceTo<Encounter>(caster);
}
