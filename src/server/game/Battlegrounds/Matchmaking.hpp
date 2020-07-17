#pragma once

#include "ArenaTeam.h"
#include "SharedDefines.h"

class Battleground;
struct GroupQueueInfo;

namespace Matchmaking
{
    /// Internal ids used for matchmaking - never send to client
    enum class BracketId : uint32
    {
        Level10_19      = 0,
        Level20_29      = 1,
        Level30_39      = 2,
        Level40_49      = 3,
        Level50_59      = 4,
        Level60_69      = 5,
        Level70_79      = 6,
        Level80         = 7,
        End,
    };

    using GroupsQueueType = std::list<GroupQueueInfo*>;
    
    struct Proposal
    {
        Proposal(const GroupsQueueType *baseQueue);
    
        std::vector<int32> assigment;
        double quality;
        int32 unassignedCounter;
        const GroupsQueueType *baseQueue;
        void Clear();
        void UpdateSearchRange();
        bool HasPlayersAssignedToBothTeams() const;
    };
    
    class GAME_API BattlegroundMatchmakingData
    {
        public:
            BattlegroundMatchmakingData();
            void FindProposal(GroupsQueueType &queuedGroups);
            const Proposal& GetBestProposal() const { return best; }
            void AddPlayerData(Player* plr, GroupQueueInfo* groupInfo);
            void RemovePlayerData(ObjectGuid guid);
            void SendDebugInfo(Player* plr);
            void SetConfig(uint32 maxPlayers);
        private:
            struct PlayerData
            {
                double rating;
                bool crossfactionPlayer;
                bool isHeal;
                float avgItemLevel;
                uint32 premadeSize;
            };
    
            void FindProposal(Proposal &current, int depth, GroupsQueueType::const_iterator itr, size_t index);
    
            double CalculateProposalQuality(Proposal &prop, Player* debugPlr = nullptr);
            Proposal best;
            uint32 maxPlayers;
            uint32 maxPlayersPerTeam;
            uint32 timestamp;
            std::map<ObjectGuid, PlayerData> playerData[BG_TEAMS_COUNT];
            uint32 totalPremadeSize[BG_TEAMS_COUNT];
            double totalRating[BG_TEAMS_COUNT];
            float totalItemLevel[BG_TEAMS_COUNT];
            uint32 totalHealerCount[BG_TEAMS_COUNT];
            uint32 totalCrossfactionCount;
    };

    constexpr ArenaTeamTypes SOLO_ARENA_TEAM_TYPE = ARENA_TEAM_3v3;
    constexpr BattlegroundQueueTypeId SOLO_ARENA_QUEUE_TYPE = BATTLEGROUND_QUEUE_3v3;

    template<ArenaTeamTypes SIZE>
    class ArenaMatchmakingData
    {
        public:
            ArenaMatchmakingData();
            void FindProposal(GroupsQueueType &queuedGroups);
            const Proposal& GetBestProposal() const { return best; }
        private:
            void FindProposal(Proposal &current, int depth, GroupsQueueType::const_iterator itr, size_t index);

            double CalculateProposalQuality(Proposal &prop, bool strict, Player* debugPlr = nullptr);

            Proposal best;
            uint32 timestamp;
    };
}