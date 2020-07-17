#include "AchievementOwner.h"
#include "AchievementCriteria.h"

SCRIPT_OWNER_SHARED_IMPL(AchievementCriteria, script::Scriptable)
{
    return dynamic_cast<AchievementCriteria*>(&base);
}

SCRIPT_COMPONENT_IMPL(AchievementCriteria, Encounter)
{
    return owner.GetEncounter();
}
