/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ObjectDefines.h"
#include "Containers.h"
#include "DBCStructure.h"
#include "DBCStores.h"
#include "Group.h"
#include "LFGQueue.h"
#include "LFGMgr.h"
#include "Log.h"
#include "Player.h"
#include "SocialMgr.h"
#include "ObjectAccessor.h"

namespace lfg
{

// Adds a new premade to queue, old data will be overwritten
void LFGQueue::AddNewToQueue(ObjectGuid guid, time_t joinTime, TimedDungeonSet const& dungeonTimes, LfgRolesMap const& roles, bool isLfgGroup)
{
    assert(!_isUpdating); // race condition when this is called while we are updating

    LfgDungeonSet dungeons;
    auto now = time(nullptr);

    for (auto itr = dungeonTimes.begin(); itr != dungeonTimes.end(); itr++)
    {
        if (itr->second <= now)
            dungeons.insert(itr->first);
        else
        {
            DelayedDungeonEntry delayed;
            delayed.premade = guid;
            delayed.dungeon = itr->first;
            delayed.timeToAdd = itr->second;
            delayed.joinTime = joinTime;
            timedDungeonEntries.insert(delayed);
        }
    }

    QueueDataContainer::iterator myData;

    QueueDataContainer::iterator it = queueDataStore.find(guid);
    if (it == queueDataStore.end())
        myData = queueDataStore.insert(std::make_pair(guid, QueueData(joinTime, dungeons, roles, isLfgGroup))).first;
    else
    {
        it->second = QueueData(joinTime, dungeons, roles, isLfgGroup);
        myData = it;
    }

    Premade newPremade = Premade(*myData);

    if (IsQueued(newPremade))
    {
        RemoveFromQueue(newPremade);
        TC_LOG_ERROR("lfg.queue", "%s was already in queue when tried to be added!", guid.ToString().c_str());
    }

    newToQueue.insert(newPremade);

    RefreshWaitTime(newPremade);
}

// Re-adds premade to queue using old data
void LFGQueue::AddOldToQueue(ObjectGuid guid)
{
    assert(!_isUpdating); // race condition when this is called while we are updating

    QueueDataContainer::iterator myData = queueDataStore.find(guid);

    if (myData == queueDataStore.end())
    {
        QueueDataContainer::iterator it = queueDataCellar.find(guid);
        if (it == queueDataCellar.end())
        {
            TC_LOG_ERROR("lfg.queue", "Tried to readd %s to queue but queueData is nowhere to be found! Will now be stuck in queue forever.", guid.ToString().c_str());
            return;
        }

        TC_LOG_ERROR("lfg.queue", "Tried to readd %s but queueData should have been deleted! Now using archived data.", guid.ToString().c_str());
        myData = queueDataStore.insert(*it).first;
    }

    Premade newPremade = Premade(*myData);

    if (IsQueued(newPremade))
    {
        RemoveFromQueue(newPremade);
        TC_LOG_ERROR("lfg.queue", "%s was already in queue when tried to be (re-)added!", guid.ToString().c_str());
    }
    newToQueue.insert(newPremade);

    RefreshWaitTime(newPremade);
}

// Removes premade from queue but keeps data
void LFGQueue::RemoveFromQueue(ObjectGuid guid)
{
    QueueDataContainer::iterator it = queueDataStore.find(guid);
    if (it != queueDataStore.end()) // If there is no queueData guid cannot be in queue
        RemoveFromQueue(Premade(*it));
}

void LFGQueue::RemoveFromQueue(Premade premade)
{
    newToQueue.erase(premade);
    queue.erase(premade);
    for (CombinationContainer::iterator it = allCombinations.begin(); it != allCombinations.end();)
        if (it->premades.find(premade) != it->premades.end())
            it = allCombinations.erase(it);
        else
            ++it;
    combinationsUpdated = true;
}

// Removes multiple from queue
void LFGQueue::RemoveFromQueue(PremadeSet guids)
{
    for (PremadeSet::const_iterator it = guids.begin(); it != guids.end(); ++it)
        RemoveFromQueue(*it);
}

// Removes premade from queue and data store
// FIXME: To work around a bug in LFGMgr we preserve queueData
void LFGQueue::RemoveFromSystem(ObjectGuid guid)
{
    assert(!_isUpdating); // race condition when this is called while we are updating

    RemoveFromQueue(guid);
    QueueDataContainer::const_iterator it = queueDataStore.find(guid);
    if (it != queueDataStore.end())
    {
        QueueDataContainer::const_iterator cIt = queueDataCellar.find(guid);
        if (cIt != queueDataCellar.end())
            queueDataCellar.at(guid) = it->second;
        else
            queueDataCellar.insert(*it);

        queueDataStore.erase(it);
    }
}

void LFGQueue::SetDebugUpdating(bool isUpdating)
{
    _isUpdating = isUpdating;
}

// Initializes group search
void LFGQueue::FindGroups()
{
    auto now = time(NULL);

    while (!timedDungeonEntries.empty() && timedDungeonEntries.begin()->timeToAdd <= now)
    {
        auto guid = timedDungeonEntries.begin()->premade;
        auto itr = queueDataStore.find(guid);
        if (itr != queueDataStore.end() && itr->second.joinTime == timedDungeonEntries.begin()->joinTime)
        {
            itr->second.dungeons.insert(timedDungeonEntries.begin()->dungeon);
            if (queue.find(Premade(*itr)) != queue.end())
            {
                RemoveFromQueue(Premade(*itr));
                AddOldToQueue(guid);
            }
        }
        timedDungeonEntries.erase(timedDungeonEntries.begin());
    }

    if (newToQueue.empty())
        return;

    // Only search for a single premade to spread runtime across world updates
    FindGroup(*newToQueue.begin());
    newToQueue.erase(newToQueue.begin());

    combinationsUpdated = true;
}

// Evaluates a given combination of premades
CombinationInfo LFGQueue::BuildCombinationInfo(PremadeSet const& premades)
{
    // -> Get RolesMap for all players and check group size

    LfgRolesMap roles;

    for (PremadeSet::const_iterator pIt = premades.begin(); pIt != premades.end(); ++pIt)
        for (LfgRolesMap::const_iterator rIt = pIt->data.roles.begin(); rIt != pIt->data.roles.end(); ++rIt)
            roles.insert(*rIt);

    if (roles.size() > MAXGROUPSIZE)
        return CombinationInfo();

    // -> Check common dungeons

    LfgDungeonSet commonDungeons = premades.begin()->data.dungeons;

    for (PremadeSet::const_iterator pIt = premades.begin(); pIt != premades.end(); ++pIt)
    {
        LfgDungeonSet intersect;
        std::set_intersection(pIt->data.dungeons.begin(), pIt->data.dungeons.end(), commonDungeons.begin(), commonDungeons.end(), std::inserter(intersect, intersect.end()));
        commonDungeons = intersect;
    }

    if (commonDungeons.empty())
        return CombinationInfo();

    // -> Check roles

    bool full = roles.size() == MAXGROUPSIZE;

    LfgRolesMapStore armts = CheckRoles(roles, full);
    if (armts.empty())
        return CombinationInfo();

    // -> Only 1 group can queue from dungeon

    ObjectGuid groupFromDungeon = ObjectGuid::Empty;

    for (PremadeSet::const_iterator pIt = premades.begin(); pIt != premades.end(); ++pIt)
    {
        if (pIt->data.isLfgGroup)
        {
            if (groupFromDungeon)
                return CombinationInfo();
            else
                groupFromDungeon = pIt->guid;
        }
    }

    // -> Check ignores

    for (PremadeSet::const_iterator gIt = premades.begin(); gIt != premades.end(); ++gIt)
        for (PremadeSet::const_iterator ggIt = premades.begin(); ggIt != premades.end(); ++ggIt)
        {
            if (gIt->guid == ggIt->guid) // Allow for same group
                continue;

            for (LfgRolesMap::const_iterator pIt = gIt->data.roles.begin(); pIt != gIt->data.roles.end(); ++pIt)
                for (LfgRolesMap::const_iterator ppIt = ggIt->data.roles.begin(); ppIt != ggIt->data.roles.end(); ++ppIt)
                {
                    Player* p = ObjectAccessor::FindPlayer(pIt->first);
                    if (p && p->GetSocial()->HasIgnore(ppIt->first))
                        return CombinationInfo();
                }
        }

    // -> Premades are compatible at this point

    // For full groups we calculate all possible arrangements so roles can be distributed fairly
    // Incomplete groups will get an arrangement with as many tanks and then as many healers as possible
    LfgRolesMap arrangement = Trinity::Containers::SelectRandomContainerElement(armts);

    if (!full)
        return CombinationInfo(premades, arrangement);

    return CombinationInfo(premades, arrangement, commonDungeons, SelectLeader(roles), groupFromDungeon);
}

// Builds combination information to store
Combination LFGQueue::ToCombination(CombinationInfo const& combination)
{
    uint64 sumJoinTime = 0;

    for (PremadeSet::const_iterator it = combination.premades.begin(); it != combination.premades.end(); ++it)
        sumJoinTime += it->data.joinTime * it->data.roles.size();

    LfgRolesNeededMap rolesNeeded = NEEDED_ALL_ROLES;
    for (LfgRolesMap::const_iterator it = combination.arrangement.begin(); it != combination.arrangement.end(); ++it)
        rolesNeeded[LfgRoles(it->second)]--;

    return Combination{ combination.premades, static_cast<time_t>(sumJoinTime / combination.arrangement.size()), rolesNeeded };
}

// Introduces a new premade to the queue and tries to form a group
bool LFGQueue::FindGroup(Premade premade)
{
    CombinationContainer newCombinations;

    // Check whether premade is full valid group
    CombinationInfo info = BuildCombinationInfo({ premade });
    if (info.state == COMBINATION_INVALID)
    {
        TC_LOG_ERROR("lfg.queue", "Received incompatible data for %s!", premade.guid.ToString().c_str());
        return false;
    }

    if (info.state == COMBINATION_COMPLETE)
    {
        LfgProposal p = BuildProposal(info);
        sLFGMgr->AddProposal(p);
        return true;
    }
    else // COMBINATION_INCOMPLETE
        newCombinations.insert(ToCombination(info));

    // Check existing groups
    for (CombinationContainer::const_iterator cIt = allCombinations.begin(); cIt != allCombinations.end(); ++cIt)
    {
        PremadeSet cGuids = cIt->premades;
        cGuids.insert(premade);
        info = BuildCombinationInfo(cGuids);


        if (info.state == COMBINATION_INVALID)
            continue;

        if (info.state == COMBINATION_COMPLETE) // The first group we find is the combination with the longest average wait time
        {
            LfgProposal p = BuildProposal(info);
            RemoveFromQueue(cIt->premades);
            sLFGMgr->AddProposal(p);
            return true;
        }
        else // COMBINATION_INCOMPLETE
            newCombinations.insert(ToCombination(info));
    }
    
    queue.insert(premade);
    allCombinations.insert(newCombinations.begin(), newCombinations.end());
    return false;
}

LfgRolesMapStore LFGQueue::CheckRoles(LfgRolesMap rolesMap, bool full)
{
    LfgRolesNeededMap needed = NEEDED_ALL_ROLES;
    LfgRolesMap interim;
    LfgRolesMapStore result;

    CheckRoles(rolesMap, needed, interim, result, full);

    return result;
}

void LFGQueue::CheckRoles(LfgRolesMap players, LfgRolesNeededMap const needed, LfgRolesMap const interim, LfgRolesMapStore& result, bool full)
{
    if (!full && !result.empty()) // If only 1 arrangement is needed we can stop the recursion
        return;

    if (players.empty()) // Found a role for every player
    {
        result.push_back(interim);
        return;
    }

    std::pair<ObjectGuid, uint8> nextPlayer = *players.begin();
    players.erase(players.begin());

    for (LfgRolesNeededMap::const_iterator it = needed.begin(); it != needed.end(); ++it)
    {
        if (it->second == 0 || !(it->first & nextPlayer.second)) // Role needed and player can fill
            continue;

        LfgRolesMap newInterim = interim;
        newInterim.insert(std::make_pair(nextPlayer.first, it->first));

        LfgRolesNeededMap newNeeded = needed;
        --newNeeded.at(it->first);
        
        CheckRoles(players, newNeeded, newInterim, result, full);
    }
}

// Selects leader for given rolesMap (must not be empty)
ObjectGuid LFGQueue::SelectLeader(LfgRolesMap const& roles)
{
    std::set<ObjectGuid> playersWithLeaderFlag;
    std::set<ObjectGuid> allPlayers;

    for (LfgRolesMap::const_iterator it = roles.begin(); it != roles.end(); ++it)
    {
        if (it->second & PLAYER_ROLE_LEADER)
            playersWithLeaderFlag.insert(it->first);
        allPlayers.insert(it->first);
    }

    return Trinity::Containers::SelectRandomContainerElement(!playersWithLeaderFlag.empty() ? playersWithLeaderFlag : allPlayers);
}

LfgProposal LFGQueue::BuildProposal(CombinationInfo const& combinationInfo)
{
    LfgProposal proposal;

    proposal.state = LFG_PROPOSAL_INITIATING;

    GuidList asList;
    for (PremadeSet::const_iterator it = combinationInfo.premades.begin(); it != combinationInfo.premades.end(); ++it)
        asList.push_back(it->guid);

    proposal.queues = asList;

    proposal.leader = combinationInfo.leaderGuid;

    proposal.group = combinationInfo.groupFromDungeon;
    proposal.isNew = !(proposal.group && sLFGMgr->GetOldState(proposal.group) == LFG_STATE_DUNGEON);

    proposal.dungeonId = Trinity::Containers::SelectRandomContainerElement(combinationInfo.commonDungeons);
    proposal.cancelTime = time(NULL) + LFG_TIME_PROPOSAL;

    LfgProposalPlayerContainer proposalPlayers;

    for (LfgRolesMap::const_iterator rIt = combinationInfo.arrangement.begin(); rIt != combinationInfo.arrangement.end(); ++rIt)
    {
        LfgProposalPlayer pPlayer;
        pPlayer.role = rIt->second;

        pPlayer.group = sLFGMgr->GetGroup(rIt->first);

        pPlayer.accept = !proposal.isNew && pPlayer.group && pPlayer.group == proposal.group ? LFG_ANSWER_AGREE : LFG_ANSWER_PENDING;
        proposalPlayers.insert(std::make_pair(rIt->first, pPlayer));
    }

    proposal.players = proposalPlayers;

    proposal.commonDungeons = combinationInfo.commonDungeons;

    return proposal;
}

uint8 LFGQueue::GetSumNeeded(LfgRolesNeededMap const& map)
{
    uint8 result = 0;
    for (LfgRolesNeededMap::const_iterator it = map.begin(); it != map.end(); ++it)
        result += it->second;
    return result;
}

// Refreshes best arrangements if there have been changes to combination list since last update
void LFGQueue::RefreshBestArrangements()
{
    if (!combinationsUpdated)
        return;

    // Reset values
    for (PremadeSet::const_iterator it = queue.begin(); it != queue.end(); ++it)
        it->data.rolesNeeded = NEEDED_ALL_ROLES;

    for (CombinationContainer::const_iterator cIt = allCombinations.begin(); cIt != allCombinations.end(); ++cIt)
    {
        bool isValid = true; // Only if combination is preferable for all premades, otherwise it is unlikely to be used for new group
        for (PremadeSet::const_iterator pIt = cIt->premades.begin(); pIt != cIt->premades.end(); ++pIt)
            if (GetSumNeeded(cIt->rolesNeeded) >= GetSumNeeded(pIt->data.rolesNeeded))
            {
                isValid = false;
                break;
            }

        if (isValid)
            for (PremadeSet::const_iterator pIt = cIt->premades.begin(); pIt != cIt->premades.end(); ++pIt)
                pIt->data.rolesNeeded = cIt->rolesNeeded;
    }

    combinationsUpdated = false;
    queueDataUpdated = true;
}

// Refreshes average wait time for a single premade
void LFGQueue::RefreshWaitTime(Premade const& premade)
{
    QueueData& myData = premade.data;

    uint8 roles = 0;
    for (LfgRolesMap::const_iterator rIt = myData.roles.begin(); rIt != myData.roles.end(); ++rIt)
        roles |= rIt->second;
    roles &= ~PLAYER_ROLE_LEADER;

    int32 bestWaitTime = -1;

    // Searching for minimum queue time of all times of dungeons we are queued and roles someone in the group can fill
    for (LfgDungeonSet::const_iterator dIt = myData.dungeons.begin(); dIt != myData.dungeons.end(); ++dIt)
    {
        WaitTimesContainer::const_iterator wtIt = waitTimes.find(*dIt);
        if (wtIt == waitTimes.end())
            continue;

        for (WaitTime::const_iterator xIt = wtIt->second.begin(); xIt != wtIt->second.end(); ++xIt)
            if (xIt->first & roles && (xIt->second < bestWaitTime || bestWaitTime == -1))
                bestWaitTime = xIt->second;
    }

    myData.avgWaitTime = bestWaitTime;
}

// Refreshes average wait times for all players if wait time data has been added since last update
void LFGQueue::RefreshWaitTimes()
{
    if (!timesUpdated)
        return;

    for (PremadeSet::const_iterator itQueue = queue.begin(); itQueue != queue.end(); ++itQueue)
        RefreshWaitTime(*itQueue);

    timesUpdated = false;
    queueDataUpdated = true;
}

// Called by LFGMgr with a set of waittimes when a completed group enters a dungeon
void LFGQueue::UpdateWaitTime(LfgDungeonSet const& dungeons, LfgWaitTimesUpdate const& times)
{
    // Get mean from raw data
    WaitTime wt;
    for (std::multimap<LfgRoles, int32>::const_iterator it = times.begin(); it != times.end(); ++it)
    {
        WaitTime::iterator wIt = wt.find(it->first);
        if (wIt == wt.end())
            wt.insert(*it);
        else
            wIt->second += it->second;
    }

    for (WaitTime::iterator it = wt.begin(); it != wt.end(); ++it)
        it->second /= NEEDED_ALL_ROLES.at(it->first);

    // Update waitTime store
    for (LfgDungeonSet::const_iterator it = dungeons.begin(); it != dungeons.end(); ++it)
    {
        WaitTimesContainer::iterator cIt = waitTimes.find(*it);
        if (cIt == waitTimes.end())
            waitTimes.insert(std::make_pair(*it, wt));
        else
            for (WaitTime::const_iterator wIt = wt.begin(); wIt != wt.end(); ++wIt)
            {
                WaitTime::iterator xIt = cIt->second.find(wIt->first);
                if (xIt == cIt->second.end())
                    cIt->second.insert(*wIt);
                else
                    xIt->second = (xIt->second + wIt->second) / 2; // New data should weigh more
            }
    }
    
    timesUpdated = true;
}

// Updates queue data for all players if changed since last update
void LFGQueue::SendQueueData(time_t currTime)
{
    if (!queueDataUpdated)
        return;

    for (PremadeSet::const_iterator itQueue = queue.begin(); itQueue != queue.end(); ++itQueue)
    {
        QueueData& queueinfo = itQueue->data;

        uint32 queuedTime = uint32(currTime - queueinfo.joinTime);

        LfgQueueStatusData queueData(queueinfo.avgWaitTime, queuedTime, queueinfo.rolesNeeded.at(PLAYER_ROLE_TANK), queueinfo.rolesNeeded.at(PLAYER_ROLE_HEALER), queueinfo.rolesNeeded.at(PLAYER_ROLE_DAMAGE));

        for (LfgRolesMap::const_iterator itPlayer = queueinfo.roles.begin(); itPlayer != queueinfo.roles.end(); ++itPlayer)
            LFGMgr::SendLfgQueueStatus(itPlayer->first, queueData);
    }

    queueDataUpdated = false;
}

time_t LFGQueue::GetJoinTime(ObjectGuid guid)
{
    QueueDataContainer::const_iterator it = queueDataStore.find(guid);
    if (it == queueDataStore.end())
    {
        QueueDataContainer::const_iterator cit = queueDataCellar.find(guid);
        if (cit == queueDataCellar.end())
        {
            TC_LOG_ERROR("lfg.queue", "Join time requested for %s but queueData is nowhere to be found! Will return dummy value", guid.ToString().c_str());
            return time(NULL) - MINUTE;
        }

        TC_LOG_ERROR("lfg.queue", "Join time requested for %s but queueData should have been deleted! Now using archived data.", guid.ToString().c_str());
        return cit->second.joinTime;
    }

    return it->second.joinTime;
}

std::string LFGQueue::DumpQueueInfo() const
{
    std::ostringstream o;

    o << "Number of premades queued: " << queue.size();

    for (PremadeSet::const_iterator it = queue.begin(); it != queue.end(); ++it)
    {
        ObjectGuid guid = it->guid;
            if (guid.IsGroup())
        {
            o << "\nGroup " << guid.ToString() << ": ";
            LfgRolesMap roles = it->data.roles;
            for (LfgRolesMap::const_iterator it2 = roles.begin(); it2 != roles.end(); ++it2)
                o << it2->first.GetRawValue() << ", ";
        }
        else
            o << "\nPlayer " << guid.GetRawValue();
    }

    return o.str();
}

std::string LFGQueue::DumpCompatibleInfo(bool full) const
{
    std::ostringstream o;

    o << "\nNumber of combinations queued: " << allCombinations.size();
    if (full)
        for (CombinationContainer::const_iterator it = allCombinations.begin(); it != allCombinations.end(); ++it)
        {
            o << "\n";
            for (PremadeSet::const_iterator it2 = it->premades.begin(); it2 != it->premades.end(); ++it2)
            {
                ObjectGuid guid = it2->guid;
                if (guid.IsGroup())
                    o << "Group " << guid.ToString() << ", ";
                else
                    o << "Player " << guid.ToString() << ", ";
            }
        }

    return o.str();
}

} // namespace lfg
