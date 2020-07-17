#include "GameObjectOwner.h"
#include "GameObject.h"
#include "UnitOwner.h"
#include "ObjectMgr.h"

SCRIPT_OWNER_IMPL(GameObject, WorldObject)
{
    return base.To<GameObject>();
}

SCRIPT_PRINTER_IMPL(GameObjectEntry)
{
    if (const CreatureTemplate* go = sObjectMgr->GetCreatureTemplate(value.id))
        return '"' + go->Name + "\"(GameObject: " + script::print(value.id) + ')';
    else
        return "(UNKNOWN CREATURE: " + script::print(value.id) + ')';
}
