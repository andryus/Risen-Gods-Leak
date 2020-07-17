#include "Matchmaking.hpp"

#include "BattlegroundQueue.h"
#include "Player.h"
#include "World.h"
#include "BattlegroundMgr.h"
#include "Group.h"
#include "DisableMgr.h"
#include "Chat.h"
#include "Language.h"
#include "GameEventMgr.h"
#include "WorldPacket.h"
#include "DBCStores.h"

using namespace Matchmaking;

std::unordered_map<ObjectGuid, std::unordered_map<ObjectGuid, uint32>> recentOpponents;
std::unordered_map<ObjectGuid, std::unordered_map<ObjectGuid, uint32>> recentAllies;

void StoreRecentOpponents(Proposal const& proposal)
{
    uint32 timestamp = getMSTime();
    auto itr = proposal.baseQueue->begin();
    for (size_t index = 0; index < proposal.assigment.size(); index++)
    {
        auto group = *itr;
        itr++;

        int32 team = proposal.assigment[index];
        if (proposal.assigment[index] < BG_TEAMS_COUNT)
            for (auto& player : group->Players)
            {
                auto itr2 = proposal.baseQueue->begin();
                for (size_t index2 = 0; index2 < proposal.assigment.size(); index2++)
                {
                    auto otherGroup = *itr2;
                    itr2++;

                    if (index == index2 || proposal.assigment[index2] >= BG_TEAMS_COUNT)
                        continue;

                    for (auto& otherPlayer : otherGroup->Players)
                    {
                        if (proposal.assigment[index2] == team)
                            recentAllies[player.first][otherPlayer.first] = timestamp;
                        else
                            recentOpponents[player.first][otherPlayer.first] = timestamp;
                    }
                }
            }
    }
}

void BattlegroundQueue::ArenaMatchmaking(BattlegroundBracketId bracket_id)
{
    Battleground* bg_template = sBattlegroundMgr->GetBattlegroundTemplate(BATTLEGROUND_AA);
    if (!bg_template)
    {
        TC_LOG_ERROR("bg.battleground", "Battleground: Update: bg template not found for %u", BATTLEGROUND_AA);
        return;
    }

    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketById(bg_template->GetMapId(), bracket_id);
    if (!bracketEntry)
    {
        TC_LOG_ERROR("bg.battleground", "Battleground: Update: bg bracket entry not found for map %u bracket id %u", bg_template->GetMapId(), bracket_id);
        return;
    }

    GroupsQueueType groups;

    std::copy_if(m_QueuedGroups[bracket_id][BG_QUEUE_MIXED].begin(), m_QueuedGroups[bracket_id][BG_QUEUE_MIXED].end(), std::back_inserter(groups),
        [&](GroupQueueInfo* g) { return (!g->IsInvitedToBGInstanceGUID); });
    
    if (groups.empty())
        return;

    // ArenaMatchmakingData<ARENA_TEAM_5v5> matchmaker5v5;
    // matchmaker5v5.FindProposal(groups);
    ArenaMatchmakingData<ARENA_TEAM_3v3> matchmaker3v3;
    matchmaker3v3.FindProposal(groups);
    // ArenaMatchmakingData<ARENA_TEAM_2v2> matchmaker2v2;
    // matchmaker2v2.FindProposal(groups);

    std::pair<ArenaTeamTypes, const Proposal*> bestProposal(ARENA_TEAM_3v3, &matchmaker3v3.GetBestProposal());

    if (bestProposal.second->quality > 0.5 || (sBattlegroundMgr->isArenaTesting() && bestProposal.second->HasPlayersAssignedToBothTeams()))
    {
        Battleground* arena = sBattlegroundMgr->CreateNewBattleground(BATTLEGROUND_AA, bracketEntry, bestProposal.first, true); //bracketEntry, arenaType, true);
        if (!arena)
        {
            TC_LOG_ERROR("bg.battleground", "BattlegroundQueue::Update couldn't create arena instance for rated arena match!");
            return;
        }
        StartProposal(arena, *bestProposal.second);
        StoreRecentOpponents(*bestProposal.second);

        arena->CalculateArenaMMRData();
        TC_LOG_DEBUG("bg.battleground", "Starting rated arena match!");
        arena->StartBattleground();
    }
}

template<ArenaTeamTypes SIZE>
ArenaMatchmakingData<SIZE>::ArenaMatchmakingData() : best(nullptr) { }

bool QueueTimeComparator(GroupQueueInfo* l, GroupQueueInfo* r)
{
    return l->JoinTime < r->JoinTime;
}

template<ArenaTeamTypes SIZE>
void ArenaMatchmakingData<SIZE>::FindProposal(GroupsQueueType &queuedGroups)
{
    queuedGroups.sort(QueueTimeComparator);

    Proposal current(&queuedGroups);
    current.quality = CalculateProposalQuality(current, false);
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

    best.quality = CalculateProposalQuality(best, true);
}

template<ArenaTeamTypes SIZE>
void ArenaMatchmakingData<SIZE>::FindProposal(Proposal &current, int depth, GroupsQueueType::const_iterator itr, size_t index)
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
            current.assigment[index] = team;

            if (depth <= 1 || current.unassignedCounter == 0)
            {
                current.quality = CalculateProposalQuality(current, false);
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

template<ArenaTeamTypes SIZE>
double CalculateRecentOpponentPenalty(std::array<std::array<ObjectGuid, SIZE>, BG_TEAMS_COUNT>& playerGuids, uint32 timestamp)
{
    double result = 1.0;
    for (uint32 team = 0; team < BG_TEAMS_COUNT; team++)
    {
        for (auto player : playerGuids[team])
            for (uint32 team2 = 0; team2 < BG_TEAMS_COUNT; team2++)
            {
                std::unordered_map<ObjectGuid, uint32>* map;
                if (team == team2)
                    map = &recentAllies[player];
                else
                    map = &recentOpponents[player];

                for (auto player2 : playerGuids[team2])
                {
                    auto itr = map->find(player2);
                    if (itr != map->end())
                    {
                        float pct = float(timestamp - itr->second) / sWorld->getFloatConfig(CONFIG_ARENA_MATCHMAKING_RECENT_OPPONENT_TIME);
                        if (pct < 1.f)
                            result = std::min(double((1.f - pct) * sWorld->getFloatConfig(CONFIG_ARENA_MATCHMAKING_RECENT_OPPONENT_PENALTY) + pct), result);
                    }
                }
            }
    }
    return result;
}

template<ArenaTeamTypes SIZE>
double ArenaMatchmakingData<SIZE>::CalculateProposalQuality(Proposal& prop, bool strict, Player* debugPlr)
{
    int32 maxPlayers = SIZE * BG_TEAMS_COUNT;

    int32 freeSlots[BG_TEAMS_COUNT];
    int32 playerCount[BG_TEAMS_COUNT];
    uint32 healerCount[BG_TEAMS_COUNT];
    double rating[BG_TEAMS_COUNT];
    double minRating = std::numeric_limits<double>::infinity();
    double maxRating = -std::numeric_limits<double>::infinity();
    double itemLevel[BG_TEAMS_COUNT];
    double premadeSize[BG_TEAMS_COUNT];
    std::array<uint32, BG_TEAMS_COUNT> teamIds;
    teamIds.fill(0);
    uint32 wait = 0;

    std::array<uint32, BG_TEAMS_COUNT> playerGuidWritePos;
    std::array<std::array<ObjectGuid, SIZE>, BG_TEAMS_COUNT> playerGuids;

    for (uint32 team = 0; team < BG_TEAMS_COUNT; team++)
    {
        freeSlots[team] = SIZE;
        playerCount[team] = 0;
        rating[team] = 0;
        premadeSize[team] = 0;
        healerCount[team] = 0;
        itemLevel[team] = 0;
        playerGuids[team].fill(ObjectGuid::Empty);
        playerGuidWritePos[team] = 0;
    }

    auto itr = prop.baseQueue->begin();
    for (size_t index = 0; index < prop.assigment.size(); index++)
    {
        auto group = *itr;
        itr++;
        int32 team = prop.assigment[index];

        if (team >= 0 && team < BG_TEAMS_COUNT)
        {
            if (teamIds[team] == 0)
                teamIds[team] = group->Team;
            // do not allow mixed teams atm
            if (teamIds[team] != group->Team)
                return -std::numeric_limits<double>::infinity();
            playerCount[team] += group->Players.size();
            premadeSize[team] += group->Players.size() * group->Players.size();
            wait += (timestamp - group->JoinTime) * group->Players.size();
            for (auto pinfo : group->Players)
            {
                if (playerGuidWritePos[team] >= SIZE)
                    return -std::numeric_limits<double>::infinity();
                playerGuids[team][playerGuidWritePos[team]++] = pinfo.first;
                rating[team] += pinfo.second->Rating;
                if (minRating > pinfo.second->Rating)
                    minRating = pinfo.second->Rating;
                if (maxRating < pinfo.second->Rating)
                    maxRating = pinfo.second->Rating;
                itemLevel[team] += pinfo.second->AvgItemLevel;
                if (pinfo.second->IsHeal)
                    healerCount[team] += 1;
            }
        }
    }

    wait /= IN_MILLISECONDS;

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

    if (strict && (freeSlots[TEAM_ALLIANCE] > 0 || freeSlots[TEAM_HORDE] > 0))
        return -std::numeric_limits<double>::infinity();

    if (std::abs(premadeSize[TEAM_ALLIANCE] - premadeSize[TEAM_HORDE]) >= sWorld->getFloatConfig(CONFIG_ARENA_MATCHMAKING_MAX_PREMADE_DIFF))
        return -std::numeric_limits<double>::infinity();

    if ((healerCount[TEAM_ALLIANCE] != healerCount[TEAM_HORDE] || healerCount[TEAM_ALLIANCE] > SIZE / 2) && premadeSize[TEAM_ALLIANCE] < SIZE)
        return -std::numeric_limits<double>::infinity();

    double players, balance, premade;
    double quality = 1.0;

    players = 0.25 + 0.75 * (playerCount[TEAM_ALLIANCE] + playerCount[TEAM_HORDE]) / double(maxPlayers);
    quality *= std::pow(players, sWorld->getFloatConfig(CONFIG_ARENA_MATCHMAKING_PLAYER_COUNT_WEIGHT));

    double maxSkillRange = std::min(1.0, double(wait) / sWorld->getFloatConfig(CONFIG_ARENA_MATCHMAKING_SKILL_RANGE_TIME));
    maxSkillRange = (1.0 - maxSkillRange) * sWorld->getFloatConfig(CONFIG_ARENA_MATCHMAKING_SKILL_RANGE_MIN)
        + maxSkillRange * sWorld->getFloatConfig(CONFIG_ARENA_MATCHMAKING_SKILL_RANGE_MAX);

    double skillRange = std::min(1.0, std::max(0.0, 1 - (maxRating - minRating) / maxSkillRange));
    quality *= std::pow(skillRange, sWorld->getFloatConfig(CONFIG_ARENA_MATCHMAKING_SKILL_RANGE_WEIGHT));

    double maxSkillDiff = std::min(1.0, double(wait) / sWorld->getFloatConfig(CONFIG_ARENA_MATCHMAKING_SKILL_DIFF_TIME));
    maxSkillDiff = (1.0 - maxSkillDiff) * sWorld->getFloatConfig(CONFIG_ARENA_MATCHMAKING_SKILL_DIFF_MIN)
        + maxSkillDiff * sWorld->getFloatConfig(CONFIG_ARENA_MATCHMAKING_SKILL_DIFF_MAX);

    balance = std::max(0.0, 1 - std::abs(rating[TEAM_ALLIANCE] - rating[TEAM_HORDE]) / maxSkillDiff);
    quality *= std::pow(balance, sWorld->getFloatConfig(CONFIG_ARENA_MATCHMAKING_SKILL_DIFF_WEIGHT));

    premade = 1.0 - std::abs(premadeSize[TEAM_ALLIANCE] - premadeSize[TEAM_HORDE]) / double(maxPlayers / 2 - 1);
    quality *= std::pow(premade, sWorld->getFloatConfig(CONFIG_ARENA_MATCHMAKING_PREMADE_WEIGHT));

    quality *= CalculateRecentOpponentPenalty<SIZE>(playerGuids, timestamp);

    if (strict && SIZE == ARENA_TEAM_2v2)
        quality *= std::min(1.f, float(wait) / sWorld->getFloatConfig(CONFIG_ARENA_MATCHMAKING_2V2_TIME));
    if (strict && SIZE == ARENA_TEAM_3v3)
        quality *= std::min(1.f, float(wait) / sWorld->getFloatConfig(CONFIG_ARENA_MATCHMAKING_3V3_TIME));

    if (debugPlr)
    {
        std::stringstream ss;
        ss << "Battleground Matchmaking Info: \n";
        ss << "Player Count: " << players << /*" Player Diff: " << playerDiff <<*/ " Balance: "
            << balance << /*" Faction: " << faction <<*/ " Premade: " << premade
            << " Heal: " << healerCount[TEAM_ALLIANCE] << /*" Item Level: " << item <<*/ " Total: " << quality;
        debugPlr->SendChatMessage(ss.str().c_str());
    }

    return quality;
}

void Player::JoinArenaSoloQueue(bool rated)
{
    if (!sWorld->getBoolConfig(CONFIG_ARENA_SOLOQUEUE_ENABLED))
    {
        GetSession()->SendNotification(LANG_ARENA_SOLO_QUEUE_DISABLED);
        return;
    }

    //check existance
    Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(BATTLEGROUND_AA);
    if (!bg)
    {
        TC_LOG_ERROR("network", "Battleground: template bg (all arenas) not found");
        return;
    }

    if (DisableMgr::IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, BATTLEGROUND_AA, NULL))
    {
        ChatHandler(GetSession()).PSendSysMessage(LANG_ARENA_DISABLED);
        return;
    }

    //CUSTOM RG: rated only between normal times
    //CUSTOM RG: event 107 is spawn event for battlemaster
    GameEventMgr::ActiveEvents const& activeEvents = sGameEventMgr->GetActiveEventList();
    if (rated && activeEvents.find(107) == activeEvents.end())
    {
        GetSession()->SendNotification("Kann nicht ausserhalb der normalen Anmeldezeiten verwendet werden!");
        return;
    }

    // expected bracket entry
    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), getLevel());
    if (!bracketEntry)
        return;

    const uint32 arenaslot = 0;
    const ArenaTeamTypes arenatype = SOLO_ARENA_TEAM_TYPE;
    const BattlegroundQueueTypeId bgQueueTypeId = SOLO_ARENA_QUEUE_TYPE;
    GroupJoinBattlegroundResult err = ERR_GROUP_JOIN_BATTLEGROUND_FAIL;

    bg->SetRated(rated);

    BattlegroundQueue &bgQueue = sBattlegroundMgr->GetBattlegroundQueue(BATTLEGROUND_QUEUE_SOLO_ARENA);

    if (!GetGroup())
    {
        // check if already in queue
        if (GetBattlegroundQueueIndex(bgQueueTypeId) < PLAYER_MAX_BATTLEGROUND_QUEUES)
            //player is already in this queue
            return;
        // check if has free queue slots
        if (!HasFreeBattlegroundQueueId())
            return;

        const float arenaRating = pvpData.GetArenaRating();
        float matchmakerRating = pvpData.GetArenaSkillValues().rating;

        GroupQueueInfo* ginfo = bgQueue.AddGroup(this, NULL, BATTLEGROUND_AA, bracketEntry, arenatype, rated, false, arenaRating, matchmakerRating, 0, true);
        uint32 avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId());
        uint32 queueSlot = AddBattlegroundQueueId(bgQueueTypeId);

        // send status packet (in queue)
        WorldPacket data = sBattlegroundMgr->BuildBattlegroundStatusPacket(bg, queueSlot, STATUS_WAIT_QUEUE, avgTime, 0, arenatype, 0);
        GetSession()->SendPacket(std::move(data));
        TC_LOG_DEBUG("bg.battleground", "Battleground: player joined queue for arena, skirmish, bg queue type %u bg type %u: GUID %u, NAME %s", bgQueueTypeId, BATTLEGROUND_AA, GetGUID().GetCounter(), GetName().c_str());
    }
    else
    {
        GetSession()->SendNotification(LANG_ARENA_SOLO_QUEUE_ERROR_IN_GROUP);
        return;
    }
}
