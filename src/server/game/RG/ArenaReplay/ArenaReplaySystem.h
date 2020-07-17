#pragma once

#include "World.h"
#include "WorldPacket.h"
#include <map>
#include <mutex>
#include <atomic>
#include <memory>

class Battleground;
class ArenaTeam;
class ReplayData;

struct StartingPosition
{
    uint32 team0;
    uint32 team1;
    uint32 team2;
};

class ReplayEntry
{
        friend class ReplayData;
    private:
        enum class LoadingState
        {
            NOT_LOADED,
            LOADED,
            INVALID
        };

    public:
        ReplayEntry(ReplayEntry const&) = delete;
        ReplayEntry(ReplayEntry&&) = delete;
        ReplayEntry();
        void Initialize(Field* fields);
        void Initialize(uint32 replayId, uint32 mapId);

        std::unordered_map<ObjectGuid, uint32> const& GetPlayerTeams() const { return playerTeams; }
        uint32 GetId() const { return id; }
        uint32 GetMapId() const { return mapId; }
        std::string const& GetTeamName(uint32 teamId) const { return teamNames[teamId]; }
        bool IsLoaded() const { return state == LoadingState::LOADED || state == LoadingState::INVALID; }
        bool IsLoading() const { return isLoading; }
        bool IsInvalid() const { return state == LoadingState::INVALID; }
        time_t GetMatchTime() const { return matchTime; }
        void SetMatchTime(time_t time) { matchTime = time; }

        std::shared_ptr<ReplayData> StartRecord();
        void EndRecord();

        void SaveToDB(Battleground* bg, ArenaTeam* team1, ArenaTeam* team2);
        void LoadPlayerFromDB(Field* fields);

        void LoadFromFile();

        void SetLoading() { isLoading = true; }
        std::shared_ptr<ReplayData> GetReplayData() { return data.lock(); }

    private:
        void OnUnload();

        bool initialized;
        uint32 id;
        uint32 mapId;
        time_t matchTime;

        bool isLoading;
        LoadingState state;

        std::weak_ptr<ReplayData> data;

        std::array<std::string, 2> teamNames;
        std::unordered_map<ObjectGuid, uint32> playerTeams;
};

class ReplayData
{
    public:
        ReplayData(ReplayEntry& entry);
        ~ReplayData();

        void AddPacket(WorldPacket const& pck);
        void Update(uint32 diff);
        void OnPlayerAdded(Player* player, uint32 team);
        void AddMirrorImage(ObjectGuid image, ObjectGuid caster);

        std::vector<std::pair<uint32, WorldPacket>> const& GetPackets() const { return packets; }
        ByteBuffer const* GetMirrorImageData(ObjectGuid guid) const;

        bool IsRecording() const { return recording; }

        void StartRecord();
        void EndRecord();
        void SetMatchStart() { matchStart = recordTime; }

        bool LoadFromFile();
        void SaveToFile();

        uint32 GetId() const { return entry.id; }
        ReplayEntry& GetEntry() { return entry; }

    private:
        ReplayEntry& entry;

        bool recording;
        uint32 recordTime;

        uint32 matchStart;
        std::vector<std::pair<uint32, WorldPacket>> packets;
        std::unordered_map<ObjectGuid, ByteBuffer> mirrorImageData;
        std::unordered_map<ObjectGuid, ObjectGuid> mirrorImageOwners;
};

class GAME_API ArenaReplayMgr
{
        using Mutex = std::mutex;
        using Lock = std::lock_guard<Mutex>;

    public:
        static ArenaReplayMgr* instance()
        {
            static ArenaReplayMgr instance;
            return &instance;
        }

        void Initialize();
        std::shared_ptr<ReplayData> CreateNewRecord(uint32 mapId);
        void BattlegroundEnded(Battleground* bg, ArenaTeam* team1, ArenaTeam* team2, bool store = true);
        bool WatchReplay(Player& player, uint32 replayId, uint8 team = 0);
        bool WatchRandomReplay(Player& player);
        void StartReplay(Player& player);
        static bool ShouldRecordReplay(Battleground* bg);
        bool HasPendingReplayTeleport(ObjectGuid player) const { Lock lock(mutex); return pendingTeleports.find(player) != pendingTeleports.end(); }
        static bool RecordRatedMatchesOnly() { return sWorld->getBoolConfig(CONFIG_ARENA_REPLAY_RATED_ONLY); }
        static bool IsEnabled() { return sWorld->getBoolConfig(CONFIG_ARENA_REPLAY_ENABLED); }

        void AddCacheReplayData(std::shared_ptr<ReplayData> data);
        std::shared_ptr<ReplayData> GetCachedReplayData(uint32 replayId);

        std::vector<std::pair<uint32, std::string>> GetLastReplayList(ObjectGuid guid) const;
        
        std::string const& GetReplayPath() { return path; }

    private:
        ArenaReplayMgr() { }
        ~ArenaReplayMgr() { }

        const ReplayEntry* GetReplayUnsafe(uint32 replayId) const;
        bool WatchReplayUnsafe(Player& player, uint32 replayId, uint8 team);

        mutable Mutex mutex;

        std::string path;

        uint32 maxId;
        uint32 GenerateNewReplayIdUnsafe();

        std::list<std::shared_ptr<ReplayData>> cachedReplays;
        std::unordered_map<uint32, std::weak_ptr<ReplayData>> replayDataMap;

        std::map<uint32, ReplayEntry> replayStorage;
        std::unordered_map<ObjectGuid, uint32> pendingTeleports;
};
#define sArenaReplayMgr ArenaReplayMgr::instance()
