#include "PvPData.h"
#include "DatabaseEnv.h"
#include "SharedDefines.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "World.h"


std::map<ObjectGuid, std::map<ObjectGuid, uint32>> PvPData::dmData;
const uint32 PvPData::rankCP[15] = { 198, 198, 210, 221, 233, 246, 260, 274, 289, 305, 321, 339, 357, 377, 398 };

PvPData::PvPData() : pguid(), rank(0), cp(0), rp(0), cpLastWeek(0), standing(0), arenaRating(0), seasonGameCount(0),
                     weeklyGameCount(0) {}

void PvPData::LoadFromDB(SharedPreparedQueryResult result)
{
    if (result)
    {
        Field* fields = result->Fetch();
        pguid = ObjectGuid(HighGuid::Player, fields[0].GetUInt32());
        rank = fields[1].GetUInt8();
        cp = fields[2].GetUInt32();
        rp = fields[3].GetUInt32();
        cpLastWeek = fields[4].GetUInt32();
        standing = fields[5].GetUInt32();
        skill.rating = fields[6].GetFloat();
        skill.rd = fields[7].GetFloat();
        skill.lastRating = uint64(fields[8].GetUInt32());
        arenaRating = fields[9].GetUInt32();
        arenaSkill.rating = fields[10].GetFloat();
        arenaSkill.rd = fields[11].GetFloat();
        arenaSkill.lastRating = uint64(fields[12].GetUInt32());
        seasonGameCount = fields[13].GetUInt32();
        weeklyGameCount = fields[14].GetUInt32();
    }
    else
    {
        pguid.Clear();
        rank = 0;
        cp = 0;
        rp = 0;
        cpLastWeek = 0;
        standing = 0;
        skill.rating = 1500.f;
        skill.rd = 350.f;
        skill.lastRating = time(nullptr);
        arenaRating = 0;
        arenaSkill.rating = 1500.f;
        arenaSkill.rd = 350.f;
        arenaSkill.lastRating = time(nullptr);
        seasonGameCount = 0;
        weeklyGameCount = 0;
    }
}

void PvPData::SaveToDB(SQLTransaction& trans, ObjectGuid guid)
{
    if ((!guid && !pguid) || (!rank && !cp && !cpLastWeek))
        return;

    if (!pguid)
        pguid = guid;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_PVP_RANK);
    stmt->setUInt32(0, pguid.GetCounter());
    stmt->setUInt8(1, rank);
    stmt->setUInt32(2, cp);
    stmt->setUInt32(3, rp);
    stmt->setUInt32(4, cpLastWeek);
    stmt->setUInt32(5, standing);
    stmt->setFloat(6, skill.rating);
    stmt->setFloat(7, skill.rd);
    stmt->setUInt32(8, skill.lastRating);
    stmt->setUInt32(9, arenaRating);
    stmt->setFloat(10, arenaSkill.rating);
    stmt->setFloat(11, arenaSkill.rd);
    stmt->setUInt32(12, arenaSkill.lastRating);
    stmt->setUInt32(13, seasonGameCount);
    stmt->setUInt32(14, weeklyGameCount);
    trans->Append(stmt);
}

void PvPData::RewardContributionPoints(uint32 points, ObjectGuid guid)
{
    if (guid && pguid)
    {
        uint32 diminishingReturns = 0;
        auto map = &(dmData[pguid]);
        auto itr = map->find(guid);
        if (itr != map->end())
            diminishingReturns = itr->second;
        ApplyPct(points, 100 - 10 * diminishingReturns);
        (*map)[guid] = std::min(diminishingReturns + 1, 10u);
    }
    cp += points;
}

void PvPData::RewardArenaBonusCP(uint32 matchCount)
{
    float points = floor(sWorld->getRate(RATE_PVP_CP_ARENA_BONUS) * PvPData::rankCP[0] * matchCount / (matchCount + sWorld->getIntConfig(CONFIG_PVP_CP_ARENA_BONUS_MATCHES))) 
        - floor(sWorld->getRate(RATE_PVP_CP_ARENA_BONUS) * PvPData::rankCP[0] * (matchCount - 1) / (matchCount - 1 + sWorld->getIntConfig(CONFIG_PVP_CP_ARENA_BONUS_MATCHES)));
    ASSERT(points >= 0.f);
    RewardContributionPoints(uint32(points));
}

uint32 CalculateRank(uint32 RP)
{
    if (!RP)
        return 0;
    if (RP < 2000)
        return 1;
    return std::min(2 + RP / 5000, 14u);
}

float CalculateRpDecay(float rpEarning, float RP)
{
    float Decay = round(0.2f * RP);
    float Delta = rpEarning - Decay;

    if (Delta < 0)
        Delta = Delta / 2;

    if (Delta < -2500)
        Delta = -2500;

    TC_LOG_TRACE("pvp.rank", "Calculate RP: earning=%f decay=%f delta=%f", rpEarning, Decay, Delta);

    return RP + Delta;
}

void PvPData::SendToPlayer(Player * plr) const
{
    plr->SendUpdateWorldState(10000, rank);
    if (rank >= 14 || rank == 0)
        plr->SendUpdateWorldState(10001, 0);
    else if (rank == 1)
        plr->SendUpdateWorldState(10001, uint32(100.f * float(rp) / 2000.f));
    else if (rank == 2)
        plr->SendUpdateWorldState(10001, uint32(100.f * float(rp - 2000) / 5000.f));
    else
        plr->SendUpdateWorldState(10001, uint32(100.f * float(rp - (rank - 2) * 5000) / 5000.f));
    plr->SendUpdateWorldState(10002, cp);
    plr->SendUpdateWorldState(10003, standing);
    plr->SendUpdateWorldState(10004, cpLastWeek);
}

void PvPData::CalculateRankUpdate()
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    CalculateRankUpdate(trans, ALLIANCE);
    CalculateRankUpdate(trans, HORDE);
    CharacterDatabase.CommitTransaction(trans);
}

struct HonorScores
{
    float FX[15];
    float FY[15];
    float BRK[14];
};

float CalculateRpEarning(float CP, HonorScores &sc)
{
    // search the function for the two points that bound the given CP
    uint8 i = 0;
    while (i < 14 && sc.BRK[i] > 0 && sc.FX[i] <= CP)
        i++;

    // we now have i such that FX[i] > CP >= FX[i-1]
    // so interpolate
    return (sc.FY[i] - sc.FY[i - 1]) * (CP - sc.FX[i - 1]) / (sc.FX[i] - sc.FX[i - 1]) + sc.FY[i - 1];
}

HonorScores GenerateScores(std::multiset<PvPData> &standingList)
{
    HonorScores sc;

    // initialize the breakpoint values
    sc.BRK[13] = 0.002f;
    sc.BRK[12] = 0.007f;
    sc.BRK[11] = 0.017f;
    sc.BRK[10] = 0.037f;
    sc.BRK[9] = 0.077f;
    sc.BRK[8] = 0.137f;
    sc.BRK[7] = 0.207f;
    sc.BRK[6] = 0.287f;
    sc.BRK[5] = 0.377f;
    sc.BRK[4] = 0.477f;
    sc.BRK[3] = 0.587f;
    sc.BRK[2] = 0.715f;
    sc.BRK[1] = 0.858f;
    sc.BRK[0] = 1.000f;

    // get the WS scores at the top of each break point
    for (uint8 group = 0; group < 14; group++)
        sc.BRK[group] = std::max(std::floor((sc.BRK[group] * standingList.size()) + 0.5f), 14.f - (float)group);

    // initialize RP array
    // set the low point
    sc.FY[0] = 0;

    // the Y values for each breakpoint are fixed
    sc.FY[1] = 400;
    for (uint8 i = 2; i <= 13; i++)
    {
        sc.FY[i] = (i - 1) * 1000;
    }

    // and finally
    sc.FY[14] = 13000;   // ... gets 13000 RP

                         // the X values for each breakpoint are found from the CP scores
                         // of the players around that point in the WS scores
    float honor;

    // initialize CP array
    sc.FX[0] = 0;

    for (uint8 i = 1; i <= 13; i++)
    {
        honor = 0.0f;
        if (standingList.size() > std::size_t(sc.BRK[i]))
            honor += std::next(standingList.rbegin(), int(sc.BRK[i]))->GetContributionPoints();
        if (standingList.size() > std::size_t(sc.BRK[i] + 1))
            honor += std::next(standingList.rbegin(), int(sc.BRK[i] + 1))->GetContributionPoints();

        sc.FX[i] = honor ? honor / 2 : 0;
    }

    // set the high point if FX full filled before
    sc.FX[14] = sc.FX[13] ? standingList.rbegin()->GetContributionPoints() : 0;   // top scorer

    return sc;
}

void PvPData::CalculateRankUpdate(SQLTransaction &trans, uint32 faction)
{
    std::multiset<PvPData> all;

    PreparedStatement* stmt;
    switch (faction)
    {
        case ALLIANCE: stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PVP_RANK_ALLIANCE); break;
        case HORDE: stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PVP_RANK_HORDE); break;
        default: return;
    }
    SharedPreparedQueryResult result = CharacterDatabase.Query(stmt);
    
    if (!result)
        return;

    PvPData next;
    do
    {
        next.LoadFromDB(result);
        if (next.pguid)
            all.insert(next);
    } while (result->NextRow());

    auto itr = all.begin();

    // process all players with less than minimum cp
    while (itr != all.end() && itr->cp < sWorld->getIntConfig(CONFIG_PVP_CP_MINIMUM))
    {
        next = *itr;
        next.rp = uint32(CalculateRpDecay(0.f, itr->rp));
        next.cpLastWeek = next.cp;
        next.standing = 0;
        next.rank = CalculateRank(next.rp);
        next.cp = 0;
        next.SaveToDB(trans, ObjectGuid::Empty);
        if (auto player = sObjectAccessor->FindPlayer(next.pguid))
            player->pvpData = next;
        
        itr = all.erase(itr);
    }

    uint32 playerCount = all.size();

    if (!playerCount)
        return;

    HonorScores data = GenerateScores(all);
    for (; itr != all.end(); itr++)
    {
        next = *itr;
        float earning = CalculateRpEarning(itr->cp, data);
        next.rp = uint32(CalculateRpDecay(earning, next.rp));
        next.cpLastWeek = next.cp;
        next.standing = playerCount--;
        next.rank = CalculateRank(next.rp);
        next.cp = 0;
        next.SaveToDB(trans, ObjectGuid::Empty);
        if (auto player = sObjectAccessor->FindPlayer(next.pguid))
            player->pvpData = next;
    }

    return;
}

const float q = log(10.f) / 400.f;

float ExpectedOutcome(SkillValue &a, SkillValue &b)
{
    return 1.f / (1.f + pow(10.f, b.GetRatingFactor() * (a.rating - b.rating) / -400.f));
}

void PvPData::SkillHelper(SkillValue &rating, std::vector<std::pair<SkillValue, float>> &matches, float weight)
{
    float newRD = sqrt(pow(rating.rd, 2.f) + pow(0.25f, 2.f) * (time(nullptr) - rating.lastRating));
    rating.rd = std::min(350.f, newRD);

    float ratingChanceFactor = 0.f;
    for (auto match : matches)
    {
        float e = ExpectedOutcome(rating, match.first);
        ratingChanceFactor += pow(match.first.GetRatingFactor(), 2.f) * e * (1.f - e);
    }
    float recipCertainty = ratingChanceFactor * q * q; // 1/Certainty
    ratingChanceFactor = q / (1.f / pow(rating.rd, 2.f) + recipCertainty);

    float ratingDiff = 0.f;
    for (auto match : matches)
    {
        float e = ExpectedOutcome(rating, match.first);
        ratingDiff += pow(match.first.GetRatingFactor(), 2.f) * (match.second - e);
    }

    rating.rd = rating.rd * (1.f - weight) + sqrt(1.f / (1.f / (rating.rd * rating.rd) + recipCertainty)) * weight;
    rating.rating += ratingDiff * ratingChanceFactor * weight;
    rating.lastRating = time(nullptr);
}

float SkillValue::GetRatingFactor() const
{
    return 1.f / sqrt(1.f + (3.f * q * q * rd * rd) / (M_PI * M_PI));
}

SkillValue& PvPData::GetArenaSkillValues()
{
    if (arenaSkill.rating == 0)
        arenaSkill.rating = std::min(std::max(skill.rating, 1000.f), 3000.f);
    return arenaSkill;
}

void PvPData::AddArenaGame()
{
    seasonGameCount++;
    weeklyGameCount++;
}
