#include "CreatureOwner.h"
#include "Creature.h"
#include "ObjectMgr.h"

SCRIPT_OWNER_IMPL(Creature, Unit)
{
    return base.To<Creature>();
}

SCRIPT_PRINTER_IMPL(CreatureEntry)
{
    if (const CreatureTemplate* creature = sObjectMgr->GetCreatureTemplate(value.entry))
        return '"' + creature->Name + "\"(Creature: " + script::print(value.entry) + ')';
    else
        return "(UNKNOWN CREATURE: " + script::print(value.entry) + ')';
}
