#include "AchievementScript.h"
#include "AchievementCriteria.h"
#include "AchievementMgr.h"

#include "AchievementHooks.h"

#include "EncounterOwner.h"

SCRIPT_MODULE_STATE_IMPL(Achievement)
{
    std::vector<std::shared_ptr<AchievementCriteria>> achievements;
    
    SCRIPT_MODULE_PRINT(Achievement)
    {
        return std::to_string(achievements.size()) + " Active Achievement(s)";
    }
};

SCRIPT_GETTER1_INLINE(CriteriaState, Scripts::Achievement, bool, AchievementCriteria&, achievement)
{
    return achievement.IsCompleted();
}

SCRIPT_MODULE_IMPL(Achievement)
{
}

SCRIPT_ACTION1_INLINE(RemoveAchievement, Scripts::Achievement, AchievementCriteria&, achievement)
{
    for (auto itr = me.achievements.begin(), end = me.achievements.end(); itr != end;)
    {
        if (itr->get() == &achievement)
        {
            me.achievements.erase(itr);
            return;
        }
        else
            ++itr;
    }

    SetSuccess(false);
}


SCRIPT_ACTION_IMPL(StartAchievement)
{
    const auto scriptName = sAchievementMgr->GetAchievementCriteriaFScript(criteria.id);
    if (scriptName.empty())
        return SetSuccess(false);

    auto achievement = std::make_shared<AchievementCriteria>(scriptName, true);
    achievement->Init(*script::castSourceTo<Encounter>(me));

    me.achievements.push_back(achievement);

    me.GetBase() <<= Respond::CriteriaCompleted(criteria.id)
        |= Get::CriteriaState.Bind(*achievement);

    AddUndoableWith(Do::RemoveAchievement, achievement.get());
}

SCRIPT_ACTION_IMPL(CriteriaFulfilled)
{
    me.FullFill();
}

SCRIPT_ACTION_IMPL(CriteriaFailed)
{
    me.Fail();
}
