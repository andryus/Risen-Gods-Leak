#pragma once
#include "Base.h"
#include "AchievementOwner.h"
#include "AchievementScript.h"

#define ACHIEVEMENT_SCRIPT(NAME) \
    USER_SCRIPT_BASE(AchievementCriteria, Achievement, NAME)
