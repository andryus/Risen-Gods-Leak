#include "EncounterOwner.h"
#include "Encounter.h"
#include "InstanceOwner.h"

SCRIPT_OWNER_SHARED_IMPL(Encounter, script::Scriptable)
{
    return dynamic_cast<Encounter*>(&base);
}

SCRIPT_COMPONENT_IMPL(Encounter, Map)
{
    return script::castSourceTo<Map>(owner.GetInstance());
}