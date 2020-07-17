#pragma once
#include "Core/Scriptable.h"

class Encounter;

class GAME_API AchievementCriteria :
    public script::Scriptable,
    public std::enable_shared_from_this<AchievementCriteria>
{
public:

    AchievementCriteria(std::string_view scriptName, bool defaultState);
    ~AchievementCriteria();

    void Init(Encounter& encounter);

    void FullFill();
    void Fail();

    bool IsCompleted() const;
    Encounter* GetEncounter() const;

private:

    std::string scriptName;
    bool isCompleted;

    std::string OnQueryOwnerName() const override;
    bool OnInitScript(std::string_view entry) override;

    std::weak_ptr<Encounter> encounter;
};
