#include "Matchmaking.hpp"

#include "BattlegroundQueue.h"
#include "BattlegroundMgr.h"
#include "DBCStores.h"
#include "World.h"
#include "Player.h"
#include <optional>

using namespace Matchmaking;

PvPDifficultyEntry const* GetBracketForBG(BattlegroundTypeId bgTypeId, BracketId matchmakingBracket)
{
    Battleground* bg_template = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);

    if (!bg_template)
        return nullptr;

    uint32 level = (static_cast<uint32>(matchmakingBracket) + 1) * 10;
    PvPDifficultyEntry const* entry = GetBattlegroundBracketByLevel(bg_template->GetMapId(), level);
    if (entry)
        return entry;
    return nullptr;
}

std::optional<BattlegroundBracketId> GetBracketIdForBG(BattlegroundTypeId bgTypeId, BracketId matchmakingBracket)
{
    auto entry = GetBracketForBG(bgTypeId, matchmakingBracket);
    if (entry)
        return std::optional<BattlegroundBracketId>(static_cast<BattlegroundBracketId>(entry->bracketId));
    return std::optional<BattlegroundBracketId>();
}

std::optional<BracketId> GetMMBracketFromBG(BattlegroundTypeId bgTypeId, BattlegroundBracketId bgBracketId)
{
    return std::optional<BracketId>();
}

bool BattlegroundQueue::BattlegroundMatchmaking(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket_id)
{
    const std::vector<BattlegroundTypeId> BattlegroundTypes = { BATTLEGROUND_AV, BATTLEGROUND_WS, BATTLEGROUND_AB, BATTLEGROUND_EY, BATTLEGROUND_SA, BATTLEGROUND_IC };

    BGFreeSlotQueueContainer& bgQueues = sBattlegroundMgr->GetBGFreeSlotQueueStore(bgTypeId);
    for (BGFreeSlotQueueContainer::iterator itr = bgQueues.begin(); itr != bgQueues.end();)
    {
        Battleground* bg = *itr; ++itr;
        if (!bg->isRated() && bg->GetTypeID() == bgTypeId && bg->GetBracketId() == bracket_id &&
            bg->GetStatus() > STATUS_WAIT_QUEUE && bg->GetStatus() < STATUS_WAIT_LEAVE)
        {
            GroupsQueueType groups;

            std::copy_if(m_QueuedGroups[bracket_id][BG_QUEUE_MIXED].begin(), m_QueuedGroups[bracket_id][BG_QUEUE_MIXED].end(), std::back_inserter(groups),
                [&](GroupQueueInfo* g) { return (!g->IsInvitedToBGInstanceGUID && g->BgTypeId == bgTypeId); });

            auto& matchmaker = bg->GetMatchmakingData();
            matchmaker.FindProposal(groups);
            StartProposal(bg, matchmaker.GetBestProposal());

            if (!bg->HasFreeSlots())
                bg->RemoveFromBGFreeSlotQueue();
        }
    }

    Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);

    if (!bg)
        return false;

    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketById(bg->GetMapId(), bracket_id);
    if (!bracketEntry)
        return false;

    uint32 lowerBound = 0;
    uint32 minPlayer[BG_TEAMS_COUNT];
    for (uint32 team = 0; team < BG_TEAMS_COUNT; team++)
        minPlayer[team] = 0;
    for (auto itr : m_QueuedGroups[bracket_id][BG_QUEUE_MIXED])
        if (!itr->IsInvitedToBGInstanceGUID && itr->BgTypeId == bgTypeId)
        {
            uint32 groupSize = itr->Players.size();
            uint8 teamId = itr->Team == ALLIANCE ? TEAM_ALLIANCE : TEAM_HORDE;
            uint8 otherTeamId = itr->Team == ALLIANCE ? TEAM_HORDE : TEAM_ALLIANCE;
            lowerBound += groupSize;
            minPlayer[teamId] += groupSize;
            if (sWorld->getBoolConfig(BATTLEGROUND_CROSSFACTION_ENABLED))
                minPlayer[otherTeamId] += groupSize;
        }

    auto minPerTeam = sBattlegroundMgr->isTesting() ? 0 : bg->GetMinPlayersPerTeam();
    auto minPlayers = sBattlegroundMgr->isTesting() ? 1 : bg->GetMinPlayers();

    if (lowerBound < minPlayers || minPlayer[TEAM_ALLIANCE] < minPerTeam || minPlayer[TEAM_HORDE] < minPerTeam)
        return false;

    // create new battleground
    Battleground* bg2 = sBattlegroundMgr->CreateNewBattleground(bgTypeId, bracketEntry, 0, false);
    if (!bg2)
    {
        TC_LOG_ERROR("bg.battleground", "BattlegroundQueue::Update - Cannot create battleground: %u", bgTypeId);
        return false;
    }

    GroupsQueueType groups;

    std::copy_if(m_QueuedGroups[bracket_id][BG_QUEUE_MIXED].begin(), m_QueuedGroups[bracket_id][BG_QUEUE_MIXED].end(), std::back_inserter(groups),
        [&](GroupQueueInfo* g) { return (!g->IsInvitedToBGInstanceGUID && g->BgTypeId == bgTypeId); });

    auto& matchmaker = bg2->GetMatchmakingData();
    matchmaker.FindProposal(groups);
    StartProposal(bg2, matchmaker.GetBestProposal());

    bg2->StartBattleground();

    return true;
}

// new queue behavior (not ready yet)
bool BattlegroundQueue::BattlegroundMatchmaking()
{
    const std::vector<BattlegroundTypeId> BattlegroundTypes = { BATTLEGROUND_AV, BATTLEGROUND_WS, BATTLEGROUND_AB, BATTLEGROUND_EY, BATTLEGROUND_SA, BATTLEGROUND_IC };

    for (uint32 bracket = 0; bracket < static_cast<uint32>(BracketId::End); ++bracket)
    {
        const BracketId mmBracketId = static_cast<BracketId>(bracket);
        GroupsQueueType& queue = m_QueuedGroups[static_cast<uint32>(mmBracketId)][BG_QUEUE_MIXED];

        if (queue.empty())
            continue;

        for (BattlegroundTypeId bgTypeId : BattlegroundTypes)
        {
            const auto bgBracket = GetBracketIdForBG(bgTypeId, mmBracketId);
            if (!bgBracket)
                continue;

            const BattlegroundBracketId bracket_id = *bgBracket;

            BGFreeSlotQueueContainer& bgQueues = sBattlegroundMgr->GetBGFreeSlotQueueStore(bgTypeId);
            for (BGFreeSlotQueueContainer::iterator itr = bgQueues.begin(); itr != bgQueues.end();)
            {
                Battleground* bg = *itr; ++itr;
                if (!bg->isRated() && bg->GetTypeID() == bgTypeId && bg->GetBracketId() == bracket_id &&
                    bg->GetStatus() > STATUS_WAIT_QUEUE && bg->GetStatus() < STATUS_WAIT_LEAVE)
                {
                    GroupsQueueType groups;

                    std::copy_if(queue.begin(), queue.end(), std::back_inserter(groups),
                        [&](GroupQueueInfo* g) { return !g->IsInvitedToBGInstanceGUID; });

                    auto& matchmaker = bg->GetMatchmakingData();
                    matchmaker.FindProposal(groups);
                    StartProposal(bg, matchmaker.GetBestProposal());

                    if (!bg->HasFreeSlots())
                        bg->RemoveFromBGFreeSlotQueue();
                }
            }
        }
    }


    for (uint32 bracket = 0; bracket < static_cast<uint32>(BracketId::End); ++bracket)
    {
        const BracketId mmBracketId = static_cast<BracketId>(bracket);
        GroupsQueueType& queue = m_QueuedGroups[static_cast<uint32>(mmBracketId)][BG_QUEUE_MIXED];

        GroupsQueueType groups;
        std::copy_if(queue.begin(), queue.end(), std::back_inserter(groups),
            [&](GroupQueueInfo* g) { return (!g->IsInvitedToBGInstanceGUID); });

        if (groups.empty())
            continue;

        float bestQuality = -std::numeric_limits<float>::infinity();
        std::optional<Proposal> bestProposal;
        BattlegroundTypeId bestTypeId = BATTLEGROUND_TYPE_NONE;

        for (BattlegroundTypeId bgTypeId : BattlegroundTypes)
        {
            Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);

            if (!bg)
                continue;

            BattlegroundMatchmakingData matchmaker;
            matchmaker.SetConfig(bg->GetMaxPlayers());
            matchmaker.FindProposal(groups);

            if (matchmaker.GetBestProposal().quality > bestQuality)
            {
                bestQuality = matchmaker.GetBestProposal().quality;
                bestProposal = std::optional<Proposal>(matchmaker.GetBestProposal());
                bestTypeId = bgTypeId;
            }
        }

        if (bestQuality >= 0.5f)
        {
            const auto bracketEntry = GetBracketForBG(bestTypeId, mmBracketId);

            if (!bracketEntry)
                continue;

            // create new battleground
            Battleground* bg2 = sBattlegroundMgr->CreateNewBattleground(bestTypeId, bracketEntry, 0, false);
            if (!bg2)
            {
                TC_LOG_ERROR("bg.battleground", "BattlegroundQueue::Update - Cannot create battleground: %u", bestTypeId);
                continue;
            }

            StartProposal(bg2, *bestProposal);
            bg2->StartBattleground();
        }
    }

    return true;
}

namespace Matchmaking
{
    static const int32 PERFORMANCE_GROUPS_PER_ITERATION = 25;

    Proposal::Proposal(const GroupsQueueType * baseQueue) : baseQueue(baseQueue),
        assigment(std::min(baseQueue ? static_cast<int32>(baseQueue->size()) : 0, PERFORMANCE_GROUPS_PER_ITERATION))
    {
        Clear();
        std::fill(assigment.begin(), assigment.end(), -1);
        unassignedCounter = assigment.size();
    }

    void Proposal::Clear()
    {
        quality = -std::numeric_limits<double>::infinity();
    }

    void Proposal::UpdateSearchRange()
    {
        int increasedSize = std::min(int32(baseQueue->size() - assigment.size()), PERFORMANCE_GROUPS_PER_ITERATION - unassignedCounter);
        assigment.resize(assigment.size() + increasedSize, -1);
        unassignedCounter += increasedSize;
    }

    bool Proposal::HasPlayersAssignedToBothTeams() const
    {
        bool assigned[BG_TEAMS_COUNT];
        for (uint32 team = 0; team < BG_TEAMS_COUNT; ++team)
            assigned[team] = false;

        for (auto itr : assigment)
            if (itr >= 0)
                assigned[itr] = true;

        for (uint32 team = 0; team < BG_TEAMS_COUNT; ++team)
            if (!assigned[team])
                return false;
        return true;
    }

    BattlegroundMatchmakingData::BattlegroundMatchmakingData() : maxPlayers(0), maxPlayersPerTeam(0), best(nullptr), totalCrossfactionCount(0), timestamp(0)
    {
        for (uint32 team = 0; team < BG_TEAMS_COUNT; team++)
        {
            totalPremadeSize[team] = 0;
            totalHealerCount[team] = 0;
            totalRating[team] = 0.0;
            totalItemLevel[team] = 0.0f;
        }
    }

    bool QueueTimeComparator(GroupQueueInfo* l, GroupQueueInfo* r)
    {
        return l->JoinTime < r->JoinTime;
    }

    void BattlegroundMatchmakingData::FindProposal(GroupsQueueType &queuedGroups)
    {
        queuedGroups.sort(QueueTimeComparator);

        Proposal current(&queuedGroups);
        current.quality = CalculateProposalQuality(current);
        best = current;

        timestamp = getMSTime();

        // Here we try to find a good match by maximazing the proposal quality.
        // We use a depth limited search algorithm which assigns 2-4 groups per iteration to a team.
        while (current.unassignedCounter > 0)
        {
            int search_depth;
            if (current.unassignedCounter <= 6)
                search_depth = 4;
            else if (current.unassignedCounter <= 12)
                search_depth = 3;
            else
                search_depth = 2;
            FindProposal(current, search_depth, current.baseQueue->cbegin(), 0);
            if (best.quality < current.quality)
                best = current;
            current.UpdateSearchRange();
        }
    }

    void BattlegroundMatchmakingData::FindProposal(Proposal &current, int depth, GroupsQueueType::const_iterator itr, size_t index)
    {
        Proposal currentBest(current.baseQueue);
        currentBest.Clear();

        const GroupsQueueType &queue = *current.baseQueue;
        current.unassignedCounter--;
        for (; index < current.assigment.size(); index++)
        {
            if (current.assigment[index] >= 0)
            {
                itr++;
                continue;
            }

            GroupQueueInfo *group = *itr;

            for (uint32 team = 0; team < BG_TEAMS_COUNT + 1; team++)
            {
                if (team < BG_TEAMS_COUNT && !group->CanJoinBGTeam(team))
                    continue;

                current.assigment[index] = team;

                if (depth <= 1 || current.unassignedCounter == 0)
                {
                    current.quality = CalculateProposalQuality(current);
                    if (current.quality > currentBest.quality)
                        currentBest = current;
                }
                else
                {
                    Proposal localMaximum = current;
                    FindProposal(localMaximum, depth - 1, itr, index);
                    if (localMaximum.quality > currentBest.quality)
                        currentBest = localMaximum;
                }
            }

            current.assigment[index] = -1;
            itr++;
        }

        current = currentBest;
    }

    void BattlegroundMatchmakingData::AddPlayerData(Player* plr, GroupQueueInfo* groupInfo)
    {
        uint32 team = TEAM_ALLIANCE;
        if (groupInfo->Team == HORDE)
            team = TEAM_HORDE;
        auto& data = playerData[team];
        const float iLvl = plr->GetPvPItemLevel();
        const bool isHeal = plr->IsHealer();
        data[plr->GetGUID()] = {
            plr->pvpData.GetRating().rating,
            groupInfo->Team != plr->GetOTeam(),
            isHeal,
            iLvl,
            uint32(groupInfo->Players.size())
        };
        if (groupInfo->Team != plr->GetOTeam())
            totalCrossfactionCount++;
        totalPremadeSize[team] += uint32(groupInfo->Players.size());
        totalRating[team] += plr->pvpData.GetRating().rating;
        totalItemLevel[team] += iLvl;
        if (isHeal)
            totalHealerCount[team] += 1;
    }

    void BattlegroundMatchmakingData::RemovePlayerData(ObjectGuid guid)
    {
        for (uint32 team = 0; team < BG_TEAMS_COUNT; team++)
        {
            auto itr = playerData[team].find(guid);
            if (itr != playerData[team].end())
            {
                if (itr->second.crossfactionPlayer)
                    totalCrossfactionCount--;
                totalPremadeSize[team] -= itr->second.premadeSize;
                totalRating[team] -= itr->second.rating;
                totalItemLevel[team] -= itr->second.avgItemLevel;
                if (itr->second.isHeal)
                    totalHealerCount[team] -= 1;
                playerData[team].erase(itr);
            }
        }
    }

    double BattlegroundMatchmakingData::CalculateProposalQuality(Proposal &prop, Player* debugPlr)
    {
        int32 freeSlots[BG_TEAMS_COUNT];
        int32 playerCount[BG_TEAMS_COUNT];
        uint32 healerCount[BG_TEAMS_COUNT];
        const uint32 TEAM_IDS[BG_TEAMS_COUNT] = { ALLIANCE, HORDE };
        double rating[BG_TEAMS_COUNT];
        double itemLevel[BG_TEAMS_COUNT];
        double premadeSize[BG_TEAMS_COUNT];

        uint32 factionChangeCount = totalCrossfactionCount;

        double wait = 0.0;

        for (uint32 team = 0; team < BG_TEAMS_COUNT; team++)
        {
            freeSlots[team] = maxPlayersPerTeam;
            playerCount[team] = playerData[team].size();
            rating[team] = totalRating[team];
            premadeSize[team] = totalPremadeSize[team];
            healerCount[team] = totalHealerCount[team];
            itemLevel[team] = totalItemLevel[team];
        }

        auto itr = prop.baseQueue->begin();
        for (size_t index = 0; index < prop.assigment.size(); index++)
        {
            auto group = *itr;
            itr++;
            int32 team = prop.assigment[index];

            if (team >= 0 && team < BG_TEAMS_COUNT)
            {
                if (group->Team != TEAM_IDS[team])
                    factionChangeCount += group->Players.size();
                playerCount[team] += group->Players.size();
                premadeSize[team] += group->Players.size() * group->Players.size();
                for (auto pinfo : group->Players)
                {
                    rating[team] += pinfo.second->Rating;
                    itemLevel[team] += pinfo.second->AvgItemLevel;
                    if (pinfo.second->IsHeal)
                        healerCount[team] += 1;
                }
            }
            else
                wait += (timestamp - group->JoinTime) * group->Players.size();
        }

        for (uint32 team = 0; team < BG_TEAMS_COUNT; team++)
        {
            if (playerCount[team])
            {
                rating[team] /= playerCount[team];
                premadeSize[team] /= playerCount[team];
                itemLevel[team] /= playerCount[team];
            }
            freeSlots[team] -= playerCount[team];
        }

        if (freeSlots[TEAM_ALLIANCE] < 0 || freeSlots[TEAM_HORDE] < 0)
            return -std::numeric_limits<double>::infinity();

        double quality = 1.0;

        double players = (playerCount[TEAM_ALLIANCE] + playerCount[TEAM_HORDE]) / double(maxPlayers);
        quality *= std::pow(players, sWorld->getFloatConfig(CONFIG_BG_MATCHMAKING_PLAYER_COUNT));

        double playerDiff = 1.0 - std::abs(freeSlots[TEAM_ALLIANCE] - freeSlots[TEAM_HORDE]) /
            (1.0 + std::max(1.0, double(playerCount[TEAM_ALLIANCE] + playerCount[TEAM_HORDE])));
        quality *= std::pow(playerDiff, sWorld->getFloatConfig(CONFIG_BG_MATCHMAKING_PLAYER_DIFF));

        if (sBattlegroundMgr->isTesting())
            return quality;

        double healerDiff = 1.0 - std::abs(double(healerCount[TEAM_ALLIANCE]) - double(healerCount[TEAM_HORDE])) /
            (1.0 + std::max(1.0, double(playerCount[TEAM_ALLIANCE] + playerCount[TEAM_HORDE])));
        quality *= std::pow(healerDiff, sWorld->getFloatConfig(CONFIG_BG_MATCHMAKING_HEALER_DIFF));

        double balance = std::max(0.1, 1 - std::abs(rating[TEAM_ALLIANCE] - rating[TEAM_HORDE]) / sWorld->getFloatConfig(CONFIG_BG_MATCHMAKING_MAX_SKILL));
        quality *= std::pow(balance, sWorld->getFloatConfig(CONFIG_BG_MATCHMAKING_SKILL));

        double item = std::max(0.1, 1 - std::abs(itemLevel[TEAM_ALLIANCE] - itemLevel[TEAM_HORDE]) / sWorld->getFloatConfig(CONFIG_BG_MATCHMAKING_MAX_ITEMLVL));
        quality *= std::pow(item, sWorld->getFloatConfig(CONFIG_BG_MATCHMAKING_ITEMLVL));

        double faction = 1.0 - double(factionChangeCount) / double(maxPlayers);
        quality *= std::pow(faction, sWorld->getFloatConfig(CONFIG_BG_MATCHMAKING_FACTION));

        double premade = 1.0 - std::abs(premadeSize[TEAM_ALLIANCE] - premadeSize[TEAM_HORDE]) / double(maxPlayers / 2 - 1);
        quality *= std::pow(premade, sWorld->getFloatConfig(CONFIG_BG_MATCHMAKING_PREMADE));

        wait = 1.0 - std::min(0.1, wait / sWorld->getFloatConfig(CONFIG_BG_MATCHMAKING_MAX_WAIT));
        quality *= std::pow(wait, sWorld->getFloatConfig(CONFIG_BG_MATCHMAKING_WAIT));

        if (debugPlr)
        {
            std::stringstream ss;
            ss << "Battleground Matchmaking Info: \n";
            ss << "Player Count: " << players << " Player Diff: " << playerDiff << " Balance: "
                << balance << " Faction: " << faction << " Premade: " << premade
                << " Heal: " << healerDiff << " Item Level: " << item << " Total: " << quality;
            debugPlr->SendChatMessage(ss.str().c_str());
        }

        return quality;
    }

    void BattlegroundMatchmakingData::SendDebugInfo(Player * plr)
    {
        GroupsQueueType noQueue;
        Proposal prop(&noQueue);
        prop.quality = CalculateProposalQuality(prop, plr);
    }

    void BattlegroundMatchmakingData::SetConfig(uint32 maxPlayers)
    {
        ASSERT(maxPlayers % 2 == 0);

        this->maxPlayers = maxPlayers;
        this->maxPlayersPerTeam = maxPlayers / 2;
    }
}
