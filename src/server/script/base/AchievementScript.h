#pragma once
#include "API/ModuleScript.h"

#include "AchievementOwner.h"

SCRIPT_MODULE(Achievement, script::Scriptable)

/**************************************
* ACTIONS
**************************************/

SCRIPT_ACTION_EX(StartAchievement, Scripts::Achievement, AchievementCriteriaId, criteria)

SCRIPT_ACTION(CriteriaFulfilled, AchievementCriteria)
SCRIPT_ACTION(CriteriaFailed, AchievementCriteria)
