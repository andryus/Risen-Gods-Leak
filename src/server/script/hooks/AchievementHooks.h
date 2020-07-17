#pragma once
#include "BaseHooks.h"
#include "AchievementOwner.h"

SCRIPT_QUERY_EX(CriteriaCompleted, script::Scriptable, bool, AchievementCriteriaId, criteriaId) { return false; }
