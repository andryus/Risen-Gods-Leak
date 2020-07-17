#include "AchievementCriteria.h"
#include "AchievementHooks.h"
#include "Encounter.h"
#include "API/LoadScript.h"

AchievementCriteria::AchievementCriteria(std::string_view scriptName, bool defaultState) :
    scriptName(scriptName), isCompleted(defaultState)
{
}

AchievementCriteria::~AchievementCriteria()
{
    unloadScript(*this);
}

void AchievementCriteria::FullFill()
{
    isCompleted = true;
}

void AchievementCriteria::Fail()
{
    isCompleted = false;
}

bool AchievementCriteria::IsCompleted() const
{
    return isCompleted;
}

Encounter* AchievementCriteria::GetEncounter() const
{
    return encounter.lock().get();
}

void AchievementCriteria::Init(Encounter& encounter)
{
    this->encounter = encounter.weak_from_this();
    InitScript(scriptName);
}

std::string AchievementCriteria::OnQueryOwnerName() const
{
    return "AchievementCriteria";
}

bool AchievementCriteria::OnInitScript(std::string_view entry)
{
    return loadScriptType(*this, entry);
}
