#pragma once
#include "BaseOwner.h"

class Encounter;
class AchievementCriteria;

struct AchievementCriteriaId 
{
    AchievementCriteriaId() = default;
    constexpr AchievementCriteriaId(uint32 id) :
        id(id) {}

    operator uint32() const { return id; } 

    uint32 id;
};

SCRIPT_OWNER_SHARED(AchievementCriteria, script::Scriptable)

SCRIPT_COMPONENT(AchievementCriteria, Encounter)
