#include "InstanceOwner.h"
#include "InstanceMap.h"

SCRIPT_OWNER_IMPL(Instance, Map)
{
    return dynamic_cast<Instance*>(&base);
}
