#pragma once

#include "Common.h"
#include "DatabaseWorkerPool.h"
#include <vector>
#include <tuple>

#include "ObjectGuid.h"

class Player;

struct SkillValue
{
    float rating;
    float rd;
    uint64 lastRating;
    float GetRatingFactor() const;
};

class GAME_API PvPData
{
    ObjectGuid pguid;
    uint32 rank;
    uint32 cp;
    uint32 rp;
    uint32 cpLastWeek;
    uint32 standing;
    SkillValue skill;
    // arena data
    uint32 arenaRating;
    SkillValue arenaSkill;
    uint32 seasonGameCount;
    uint32 weeklyGameCount;
    static std::map<ObjectGuid, std::map<ObjectGuid, uint32>> dmData;
    static void CalculateRankUpdate(SQLTransaction &trans, uint32 faction);
public:
    PvPData();
    void LoadFromDB(SharedPreparedQueryResult result);
    void SaveToDB(SQLTransaction &trans, ObjectGuid guid);
    void RewardContributionPoints(uint32 points, ObjectGuid lowguid = ObjectGuid::Empty);
    void RewardArenaBonusCP(uint32 matchCount);
    uint32 GetContributionPoints() const { return cp; }
    uint32 GetRank() const { return rank; }
    bool HasStanding() { return standing > 0; }
    void SendToPlayer(Player* plr) const;
    static void CalculateRankUpdate();

    static const uint32 rankCP[15];

    static void SkillHelper(SkillValue &rating, std::vector<std::pair<SkillValue, float>> &matches, float weight = 1.f);
    SkillValue& GetRating() { return skill; }

    SkillValue& GetArenaSkillValues();
    void AddArenaGame();
    uint32 const& GetArenaRating() const { return arenaRating; }
    uint32& GetArenaRating() { return arenaRating; }

    bool operator<(PvPData const& other) const { return cp < other.cp; }
};
