/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _LFGQUEUE_H
#define _LFGQUEUE_H

#include "LFG.h"
#include "LFGMgr.h"

namespace lfg
{

/// Stores player or group queue info
struct QueueData
{
    QueueData(time_t joinTime, LfgDungeonSet const& dungeons, LfgRolesMap const& roles, bool isLfgGroup) :
        joinTime(joinTime), dungeons(dungeons), roles(roles), avgWaitTime(-1), rolesNeeded(NEEDED_ALL_ROLES), isLfgGroup(isLfgGroup) {}

    time_t joinTime;
    LfgDungeonSet dungeons;
    LfgRolesMap roles;                           // Player guid -> All selected roles
    bool isLfgGroup;
    int32 avgWaitTime;
    LfgRolesNeededMap rolesNeeded;               // What roles are missing (to be displayed in client)
};

typedef std::map<ObjectGuid, QueueData> QueueDataContainer;              // Group or player guid -> data

struct Premade                                                       // Used to couple player / group guids with their data so we don't need to do searches
{
    ObjectGuid const guid;
    QueueData& data;

    Premade(std::pair<ObjectGuid const, QueueData> &value) : guid(value.first), data(value.second) {}

    bool operator<(Premade const& other) const {
        if (guid == other.guid)
            return false;
        if (data.joinTime == other.data.joinTime)
            return guid < other.guid;
        return data.joinTime < other.data.joinTime;
    }
    
};

typedef std::set<Premade> PremadeSet;

enum CombinationInfoResult
{
    COMBINATION_INVALID,                         // Combination not possible for some reason
    COMBINATION_INCOMPLETE,                      // Combination possible but more players needed
    COMBINATION_COMPLETE                         // Combination is full valid group
};

struct CombinationInfo                                               // Used during group search
{
    CombinationInfoResult state;

    PremadeSet premades;                         // Guids from QueueDataContainer
    LfgRolesMap arrangement;                     // Wich player would take what role
    LfgDungeonSet commonDungeons;                // Dungeons all premades of this combination have selected
    ObjectGuid leaderGuid;                           // Needed for proposal
    ObjectGuid groupFromDungeon;                     // Guid of the group that queues from a dungeon (can only be 1) or 0

    CombinationInfo() : state(COMBINATION_INVALID) {}
    CombinationInfo(PremadeSet const& premades, LfgRolesMap const& arrangement) : state(COMBINATION_INCOMPLETE), premades(premades), arrangement(arrangement) {}
    CombinationInfo(PremadeSet const& premades, LfgRolesMap const& arrangement, LfgDungeonSet const& commonDungeons, ObjectGuid leaderGuid, ObjectGuid groupFromDungeon) :
        state(COMBINATION_COMPLETE), premades(premades), arrangement(arrangement), commonDungeons(commonDungeons), leaderGuid(leaderGuid), groupFromDungeon(groupFromDungeon) {}
};

struct Combination                                                   // Used to store possible combination for later
{
    PremadeSet premades;
    time_t joinTimeAvg;                          // Used to prioritize combinations
    LfgRolesNeededMap rolesNeeded;               // Data for QueueData.rolesNeeded

    bool operator<(Combination const& other) const { return joinTimeAvg < other.joinTimeAvg; }
};

typedef std::multiset<Combination> CombinationContainer;             // Ordered by joinTimeAvg

struct DelayedDungeonEntry                                             // Dungeon entry that will be added to a premade group info
{
    uint32 dungeon;
    time_t timeToAdd;
    time_t joinTime;
    ObjectGuid premade;

    bool operator<(DelayedDungeonEntry const& other) const { return timeToAdd < other.timeToAdd; }
};

typedef std::set<DelayedDungeonEntry> DelayedDungeonSet;

typedef std::map<LfgRoles, int32> WaitTime;
typedef std::map<uint32, WaitTime> WaitTimesContainer;               // DungeonId -> (role -> time)

/**
    Stores all data related to queue
*/
class LFGQueue
{
    public:

        // Add/Remove from queue
        void AddNewToQueue(ObjectGuid guid, time_t joinTime, TimedDungeonSet const& dungeons, LfgRolesMap const& roles, bool isLfgGroup);
        void AddOldToQueue(ObjectGuid guid);

        void RemoveFromSystem(ObjectGuid guid);
        void SetDebugUpdating(bool isUpdating);

        bool HasData(ObjectGuid guid) const { return queueDataStore.find(guid) != queueDataStore.end(); }

        bool NeedsUpdate() const { return !newToQueue.empty() || combinationsUpdated || timesUpdated || 
            (!timedDungeonEntries.empty() && timedDungeonEntries.begin()->timeToAdd < time(NULL)); }

        void RefreshBestArrangements();
        void RefreshWaitTimes();

        // Update Timers (when proposal success)
        void UpdateWaitTime(LfgDungeonSet const& dungeons, LfgWaitTimesUpdate const& times);

        // Update Queue timers
        void SendQueueData(time_t currTime);
        time_t GetJoinTime(ObjectGuid guid);

        // Find new groups
        void FindGroups();

        // Just for debugging purposes
        std::string DumpQueueInfo() const;
        std::string DumpCompatibleInfo(bool full) const;
    private:
        void RemoveFromQueue(ObjectGuid guid);
        void RemoveFromQueue(Premade premade);
        void RemoveFromQueue(PremadeSet premades);

        bool IsQueued(Premade const& premade) const { return queue.find(premade) != queue.end() || newToQueue.find(premade) != newToQueue.end(); }

        bool FindGroup(Premade premade);

        void RefreshWaitTime(Premade const& premade);

        CombinationInfo BuildCombinationInfo(PremadeSet const& premades);
        Combination ToCombination(CombinationInfo const& combination);

        LfgRolesMapStore CheckRoles(LfgRolesMap rolesMap, bool full);
        void CheckRoles(LfgRolesMap players, LfgRolesNeededMap const needed, LfgRolesMap const interim, LfgRolesMapStore& result, bool full);

        ObjectGuid SelectLeader(LfgRolesMap const& roles);

        LfgProposal BuildProposal(CombinationInfo const& combinationInfo);

        uint8 GetSumNeeded(LfgRolesNeededMap const& map);

        // Queue
        QueueDataContainer queueDataStore;                           // Raw data for all groups and players.
        QueueDataContainer queueDataCellar;                          // FIXME: Workaround for a bug where LFGMgr wants to remove queueData but requests it later!
        PremadeSet queue;                                            // Players / groups waiting in DB (just a helper, allCombinations is the real queue)
        PremadeSet newToQueue;                                       // Players / groups to be introduced to the queue on next update

        CombinationContainer allCombinations;                        // All combinations of queued premades that could form a group with more players

        WaitTimesContainer waitTimes;

        bool combinationsUpdated = false;                            // Whether there have been additions / removals to the queue since last rolesNeeded and timers update
        bool timesUpdated = false;                                   // Whether queue times have been updated by LFGMgr

        bool queueDataUpdated = false;                               // Whether any queue data has been updated and can be sent to clients
        bool _isUpdating = false;

        DelayedDungeonSet timedDungeonEntries;
};

} // namespace lfg

#endif
