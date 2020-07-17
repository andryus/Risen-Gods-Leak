#include "ArenaReplaySystem.h"

#include "Chat.h"
#include "Language.h"
#include "Player.h"
#include "Battleground.h"
#include "Map.h"
#include "ArenaTeam.h"
#include "Opcodes.h"
#include "LFGMgr.h"
#include "BattlegroundMgr.h"
#include "Entities/Player/PlayerCache.hpp"
#include "Config.h"
#include "DBCStores.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>

const uint32 MAX_CACHED_REPLAYS = 20;

const std::map<uint32, StartingPosition> startingPositions =
{
    { 559, { 998, 929, 936 } },
    { 562, { 971, 939, 940 } },
    { 572, { 1260, 1258, 1259 } },
    { 617, { 1361, 1362, 1363 } },
    { 618, { 1366, 1364, 1365 } }
};

bool Player::IsWatchingReplay() const
{
    if (IsInWorld() && GetMap()->ToArenaReplayMap())
        return true;
    return false;
}

bool ArenaReplayMgr::ShouldRecordReplay(Battleground* bg)
{
    if (!IsEnabled() || !bg->isArena())
        return false;

    if (!bg->isRated() && RecordRatedMatchesOnly())
        return false;

    return true;
}

const ReplayEntry* ArenaReplayMgr::GetReplayUnsafe(uint32 replayId) const
{
    auto itr = replayStorage.find(replayId);
    if (itr != replayStorage.end())
        return &(itr->second);
    return nullptr;
}

void ArenaReplayMgr::BattlegroundEnded(Battleground* bg, ArenaTeam* team1, ArenaTeam* team2, bool store)
{
    if (!ShouldRecordReplay(bg))
        return;

    TC_LOG_INFO("arena.replay", "ArenaReplayMgr: BattlegroundEnded(): bg: %u, winner: %u", bg->GetInstanceID(), bg->GetWinner());
    std::shared_ptr<ReplayData> rd = bg->GetBgMap()->GetReplayData();

    if (rd && store)
    {
        rd->GetEntry().SetMatchTime(time(nullptr));
        sWorld->lowPrioThreadPool.add([rd]() mutable { rd->SaveToFile(); });
        rd->GetEntry().SaveToDB(bg, team1, team2);
    }
}

uint32 ArenaReplayMgr::GenerateNewReplayIdUnsafe()
{
    if (!++maxId)
    {
        TC_LOG_ERROR("arena.replay", "ArenaReplayMgr: Replay ID overflow!");
        ASSERT(false);
    }
    return maxId;
}

bool ArenaReplayMgr::WatchRandomReplay(Player& player)
{
    Lock lock(mutex);

    uint32 id = urand(0, maxId);

    auto itr = replayStorage.lower_bound(id);

    if (itr != replayStorage.end())
        return WatchReplayUnsafe(player, itr->first, 0);

    return false;
}

bool ArenaReplayMgr::WatchReplay(Player& player, uint32 replayId, uint8 team)
{
    Lock lock(mutex);
    return WatchReplayUnsafe(player, replayId, team);
}

bool ArenaReplayMgr::WatchReplayUnsafe(Player& player, uint32 replayId, uint8 team)
{
    if (!IsEnabled())
    {
        ChatHandler(player.GetSession()).SendSysMessage(LANG_ARENA_REPLAY_DISABLED);
        return false;
    }

    if (player.IsWatchingReplay())
    {
        ChatHandler(player.GetSession()).SendSysMessage(LANG_ARENA_REPLAY_INTERNAL_ERROR);
        return false;
    }

    for (int i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
    {
        if (BattlegroundQueueTypeId bgQueueTypeId = player.GetBattlegroundQueueTypeId(i))
        {
            player.RemoveBattlegroundQueueId(bgQueueTypeId);
            BattlegroundQueue& queue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
            queue.RemovePlayer(player.GetGUID(), true);
        }
    }

    const ReplayEntry* replayData = GetReplayUnsafe(replayId);
    
    if (replayData)
    {
        if (!AccountMgr::IsStaffAccount(player.GetSession()->GetSecurity()) && replayData->GetMatchTime() + sWorld->getIntConfig(CONFIG_ARENA_REPLAY_DELAY) > time(nullptr))
        {
            ChatHandler(player.GetSession()).SendSysMessage(LANG_ARENA_REPLAY_NOT_YET_AVAILABLE);
            return false;
        }

        if (replayData->GetPlayerTeams().find(player.GetGUID()) != replayData->GetPlayerTeams().end())
        {
            ChatHandler(player.GetSession()).SendSysMessage(LANG_ARENA_REPLAY_CANT_WATCH_OWN_MATCHES);
            return false;
        }

        auto it = startingPositions.find(replayData->GetMapId());
        if (it != startingPositions.end())
        {
            uint32 locEntry = team == 0 ? it->second.team0 : (team == 1 ? it->second.team1 : it->second.team2);
            WorldSafeLocsEntry const* loc = sWorldSafeLocsStore.LookupEntry(locEntry);
            if (loc)
            {
                pendingTeleports[player.GetGUID()] = replayId;
                player.SetBattlegroundEntryPoint();
                player.TeleportTo(replayData->GetMapId(), loc->x, loc->y, loc->z, 1.0f, TELE_TO_ARENA_REPLAY);
                return true;
            }
            else
                TC_LOG_ERROR("arena.replay", "ArenaReplayMgr: Unknown WorldSafeLoc. Replay ID: %u", replayId);
        }
        else
            TC_LOG_ERROR("arena.replay", "ArenaReplayMgr: Unknown start position. Replay ID: %u", replayId);
    }
    else
    {
        ChatHandler(player.GetSession()).PSendSysMessage(LANG_ARENA_REPLAY_NOT_FOUND, replayId);
        TC_LOG_ERROR("arena.replay", "ArenaReplayMgr: Could not find replay data. ID: %u", replayId);
    }
    return false;
}

void ArenaReplayMgr::StartReplay(Player& player)
{
    Lock lock(mutex);

    if (player.FindMap() && player.GetMap()->ToArenaReplayMap())
    {
        uint32 replayId = 0;
        auto it = pendingTeleports.find(player.GetGUID());
        if (it != pendingTeleports.end())
        {
            replayId = it->second;
            pendingTeleports.erase(it);
        }
        else
            TC_LOG_ERROR("arena.replay", "ArenaReplayMgr: Player %u not in player map", player.GetGUID().GetCounter());

        auto itr = replayStorage.find(replayId);

        if (itr != replayStorage.end())
        {
            ReplayEntry& replay = itr->second;
            
            if (!replay.IsLoaded() && !replay.IsLoading())
            {
                replay.SetLoading();
                sWorld->lowPrioThreadPool.add([&replay]() mutable { replay.LoadFromFile(); });
            }
            
            ArenaReplayMap* map = player.GetMap()->ToArenaReplayMap();
            map->SetReplay(&itr->second);
        }
        else
            player.TeleportToBGEntryPoint();
    }
}

void ArenaReplayMgr::Initialize()
{
    path = sConfigMgr->GetStringDefault("ArenaReplay.Path", "Replays");
    boost::filesystem::create_directory(boost::filesystem::path(path));

    maxId = 0;
    QueryResult r = CharacterDatabase.PQuery("SELECT MAX(`id`) FROM `arena_replay_data`");
    if (r)
    {
        Field* f = r->Fetch();
        if (!f[0].IsNull())
            maxId = f[0].GetUInt32();
    }

    QueryResult result = CharacterDatabase.PQuery("SELECT id, map, time, team1, team2 FROM `arena_replay_data`");
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            
            uint32 id = fields[0].GetUInt32();

            replayStorage[id].Initialize(fields);
        } while (result->NextRow());
    }

    result = CharacterDatabase.PQuery("SELECT id, guid, team FROM `arena_replay_char_data`");
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 id = fields[0].GetUInt32();

            auto itr = replayStorage.find(id);
            if (itr == replayStorage.end())
                continue;

            itr->second.LoadPlayerFromDB(fields);
        } while (result->NextRow());
    }
}

void ArenaReplayMap::SetReplay(ReplayEntry* entry)
{
    replayEntry = entry;
}

void ArenaReplayMap::Update(const uint32 diff)
{
    Map::Update(diff);

    if (closeTimer)
    {
        if (closeTimer <= diff)
        {
            closeTimer = 0;
            SetUnload();
            auto const &l = GetPlayers().copy();
            for (auto plr : l)
                plr.second->TeleportToBGEntryPoint();
        }
        else
            closeTimer -= diff;
    }
    else if (!m_replaying && replayEntry)
    {
        if (replayEntry->IsLoaded())
        {
            if (replayEntry->IsInvalid())
            {
                Map::PlayerList const &l = GetPlayers();
                for (Map::PlayerList::const_iterator itr = l.begin(); itr != l.end(); ++itr)
                    ChatHandler(itr->GetSession()).SendSysMessage(LANG_ARENA_REPLAY_INTERNAL_ERROR);
                closeTimer = 1;
            }
            else
                StartReplay();
        }
    }

    if (!m_replaying || replayData == nullptr)
        return;

    m_curReplayTime += diff * m_replaySpeed;
    while (replayItr != replayData->GetPackets().end() && replayItr->first <= m_curReplayTime)
    {
        WorldPacket const& packet = replayItr->second;
        TC_LOG_INFO("arena.replay", "ArenaReplayMap: Replaying opcode: %s, curReplayTime: %u, pdiff: %u", GetOpcodeNameForLogging(packet.GetOpcode()).c_str(), m_curReplayTime, replayItr->first);
        Map::PlayerList const &l = GetPlayers();
        for (Map::PlayerList::const_iterator itr = l.begin(); itr != l.end(); ++itr)
        {
            itr->SendDirectMessage(packet);
        }
        replayItr++;
    }

    if (replayItr == replayData->GetPackets().end())
    {
        closeTimer = 2 * MINUTE * IN_MILLISECONDS;
        replayData = nullptr;
        m_replaying = false;
        m_curReplayTime = 0;
    }
}

void ArenaReplayMap::StartReplay()
{
    TC_LOG_INFO("arena.replay", "ArenaReplayMap: Start replaying");

    replayData = replayEntry->GetReplayData();
    if (!replayData)
        return;

    m_replaying = true;
    replayItr = replayData->GetPackets().begin();
    m_curReplayTime = 0;
    auto itr = replayItr;
    // select first non-zero timestamp as start time
    while (itr != replayData->GetPackets().end())
    {
        if (itr->first)
        {
            m_curReplayTime = itr->first;
            break;
        }
        else
            itr++;
    }
}

std::shared_ptr<ReplayData> ArenaReplayMgr::CreateNewRecord(uint32 mapId)
{
    Lock lock(mutex);
    uint32 id = GenerateNewReplayIdUnsafe();
    ReplayEntry& replay = replayStorage[id];
    replay.Initialize(id, mapId);
    return replay.StartRecord();
}

std::vector<std::pair<uint32, std::string>> ArenaReplayMgr::GetLastReplayList(ObjectGuid guid) const
{
    Lock lock(mutex);
    std::vector<std::pair<uint32, std::string>> result;

    time_t now = time(nullptr);

    uint32 count = 0;
    auto const& replays = replayStorage;
    for (auto itr = replays.rbegin(); itr != replays.rend() && count < 10; itr++)
    {
        auto const& replay = itr->second;
        auto& playerData = replay.GetPlayerTeams();
        // skip replays with current player
        if (replay.IsInvalid() || replay.GetMatchTime() + sWorld->getIntConfig(CONFIG_ARENA_REPLAY_DELAY) > now || playerData.find(guid) != playerData.end())
            continue;

        std::stringstream ss;
        ss << "Replay #" << replay.GetId() << " - ";
        GuidVector teams[2];
        for (auto kvp : playerData)
            if (kvp.second == 0)
                teams[0].push_back(kvp.first);
            else
                teams[1].push_back(kvp.first);

        if (!replay.GetTeamName(0).empty())
            ss << replay.GetTeamName(0);
        else if (!teams[0].empty())
        {
            auto itr = teams[0].begin();
            ss << player::PlayerCache::GetName(*itr);
            itr++;
            while (itr != teams[0].end())
            {
                ss << ", " << player::PlayerCache::GetName(*itr);
                itr++;
            }
        }

        if ((!replay.GetTeamName(0).empty() || !teams[0].empty()) && 
            (!replay.GetTeamName(1).empty() || !teams[1].empty()))
            ss << " vs. ";

        if (!replay.GetTeamName(1).empty())
            ss << replay.GetTeamName(1);
        else if (!teams[1].empty())
        {
            auto itr = teams[1].begin();
            ss << player::PlayerCache::GetName(*itr);
            itr++;
            while (itr != teams[1].end())
            {
                ss << ", " << player::PlayerCache::GetName(*itr);
                itr++;
            }
        }

        result.push_back(std::make_pair(replay.GetId(), ss.str()));
        count++;
    }

    return result;
}

std::shared_ptr<ReplayData> ArenaReplayMgr::GetCachedReplayData(uint32 replayId)
{
    std::shared_ptr<ReplayData> result = replayDataMap[replayId].lock();
    return result;
}

void ArenaReplayMgr::AddCacheReplayData(std::shared_ptr<ReplayData> data)
{
    replayDataMap[data->GetId()] = std::weak_ptr<ReplayData>(data);
    for (auto itr = cachedReplays.begin(); itr != cachedReplays.end(); itr++)
    {
        if ((*itr)->GetId() == data->GetId())
        {
            cachedReplays.erase(itr);
            break;
        }
    }

    cachedReplays.push_back(data);

    if (cachedReplays.size() > MAX_CACHED_REPLAYS)
        cachedReplays.pop_front();
}

ReplayEntry::ReplayEntry() : state(LoadingState::NOT_LOADED), isLoading(false), matchTime(0) {}

ReplayData::ReplayData(ReplayEntry& _entry) : recording(false), entry(_entry) { }

void ReplayEntry::Initialize(uint32 _replayId, uint32 _mapId) 
{ 
    mapId = _mapId; 
    id = _replayId;
    initialized = true;
}

void ReplayEntry::Initialize(Field* fields)
{
    id = fields[0].GetUInt32();
    mapId = fields[1].GetUInt32();
    matchTime = fields[2].GetUInt64();
    if (!fields[3].IsNull())
        teamNames[0] = fields[3].GetString();
    if (!fields[4].IsNull())
        teamNames[1] = fields[4].GetString();
    initialized = true;
}

void ReplayData::Update(uint32 diff)
{
    ASSERT(recording);
    recordTime += diff;
}

std::shared_ptr<ReplayData> ReplayEntry::StartRecord()
{
    ASSERT(initialized);
    std::shared_ptr<ReplayData> result = std::make_shared<ReplayData>(*this);
    result->StartRecord();
    data = std::weak_ptr<ReplayData>(result);
    state = ReplayEntry::LoadingState::INVALID;
    return result;
}

void ReplayData::StartRecord()
{
    recordTime = 0;
    recording = true;
}

void ReplayEntry::EndRecord()
{
    state = ReplayEntry::LoadingState::LOADED;
}

void ReplayData::EndRecord()
{
    entry.EndRecord();
    recording = false;
}

void ReplayData::AddPacket(WorldPacket const& pck)
{
    if (recording)
        packets.push_back(std::make_pair(recordTime, pck));
}

const std::string_view HEADER("REPLAY");

void ReplayEntry::LoadFromFile()
{
    ASSERT(initialized);
    ASSERT(state == LoadingState::NOT_LOADED);

    std::shared_ptr<ReplayData> result = std::make_shared<ReplayData>(*this);
    bool success = result->LoadFromFile();

    state = success ? LoadingState::LOADED : LoadingState::INVALID;
    isLoading = false;

    data = std::weak_ptr<ReplayData>(result);
    sArenaReplayMgr->AddCacheReplayData(result);
}

bool ReplayData::LoadFromFile()
{
    std::stringstream ss;
    ss << sArenaReplayMgr->GetReplayPath() << "/" << entry.GetId() << ".replay";
    boost::filesystem::path path(ss.str());

    TC_LOG_INFO("arena.replay", "ArenaReplayMgr: Loading Replay, id: %u", entry.GetId());

    bool error = false;

    boost::filesystem::ifstream rawStream(path, boost::filesystem::ifstream::binary);
    boost::iostreams::filtering_istream ifs;
    ifs.push(boost::iostreams::zlib_decompressor());
    ifs.push(rawStream);
    try
    {
        if (ifs.good())
        {
            char* HeaderData = new char[HEADER.size()];
            ifs.read(HeaderData, HEADER.size());
            if (std::string_view(HeaderData, HEADER.size()) == HEADER)
            {
                delete[] HeaderData;
                ifs.read(reinterpret_cast<char*>(&matchStart), sizeof(uint32));
                size_t playerCount, packetCount, mirrorImageOwnerCount;
                ifs.read(reinterpret_cast<char*>(&playerCount), sizeof(size_t));
                TC_LOG_DEBUG("arena.replay", "ArenaReplayMgr: Player Count: " UI64FMTD, playerCount);
                for (size_t i = 0; i < playerCount; ++i)
                {
                    uint64 guidRaw;
                    size_t bufferSize;
                    ifs.read(reinterpret_cast<char*>(&guidRaw), sizeof(uint64));
                    ifs.read(reinterpret_cast<char*>(&bufferSize), sizeof(size_t));

                    if (bufferSize > 0xFFFF)
                        throw std::runtime_error("Arena Replay: parsing error");

                    ObjectGuid guid(guidRaw);
                    ByteBuffer& buffer = mirrorImageData[guid];
                    buffer.resize(bufferSize);
                    ifs.read(reinterpret_cast<char*>(buffer.contents()), bufferSize);

                    if (ifs.eof())
                        throw std::runtime_error("Arena Replay: end of file");
                }

                ifs.read(reinterpret_cast<char*>(&mirrorImageOwnerCount), sizeof(size_t));
                TC_LOG_DEBUG("arena.replay", "ArenaReplayMgr: Mirror Image Count: " UI64FMTD, mirrorImageOwnerCount);
                for (size_t i = 0; i < mirrorImageOwnerCount; ++i)
                {
                    uint64 imageGuid, ownerGuid;

                    ifs.read(reinterpret_cast<char*>(&imageGuid), sizeof(uint64));
                    ifs.read(reinterpret_cast<char*>(&ownerGuid), sizeof(uint64));

                    mirrorImageOwners[ObjectGuid(imageGuid)] = ObjectGuid(ownerGuid);

                    if (ifs.eof())
                        throw std::runtime_error("Arena Replay: end of file");
                }

                ifs.read(reinterpret_cast<char*>(&packetCount), sizeof(size_t));
                TC_LOG_DEBUG("arena.replay", "ArenaReplayMgr: Packet Count: " UI64FMTD, packetCount);
                for (size_t i = 0; i < packetCount; ++i)
                {
                    uint32 timestamp;
                    size_t size;
                    uint16 opcode;
                    ifs.read(reinterpret_cast<char*>(&timestamp), sizeof(uint32));
                    ifs.read(reinterpret_cast<char*>(&size), sizeof(size_t));
                    ifs.read(reinterpret_cast<char*>(&opcode), sizeof(uint16));

                    if (size > 0xFFFF)
                        throw std::runtime_error("Arena Replay: parsing error");

                    WorldPacket packet(opcode, size);
                    TC_LOG_DEBUG("arena.replay", "ArenaReplayMgr: timestamp: %u, size: " UI64FMTD ", opcode: %s", timestamp, size, GetOpcodeNameForLogging(opcode).c_str());
                    packet.resize(size);
                    ifs.read(reinterpret_cast<char*>(packet.contents()), size);

                    packets.push_back(std::make_pair(timestamp, packet));

                    if (ifs.eof())
                        throw std::runtime_error("Arena Replay: parsing error");
                }
            }
            else
            {
                delete[] HeaderData;
                TC_LOG_ERROR("arena.replay", "ArenaReplayMgr: Invalid replay data from file %s", ss.str().c_str());
                error = true;
            }
        }
        else
            TC_LOG_ERROR("arena.replay", "ArenaReplayMgr: Could not read replay data from file %s", ss.str().c_str());
    }
    catch (...)
    {
        TC_LOG_ERROR("arena.replay", "ArenaReplayMgr: Could not read replay data from file %s", ss.str().c_str());
        error = true;
    }

    error = error || !ifs.good();

    return !error;
}

void ReplayData::SaveToFile()
{
    std::stringstream ss;
    ss << sArenaReplayMgr->GetReplayPath() << "/" << entry.GetId() << ".replay";
    boost::filesystem::path path(ss.str());
    TC_LOG_INFO("arena.replay", "ArenaReplayMgr: Writing Replay, id: %u", entry.GetId());

    boost::filesystem::ofstream rawStream(path, boost::filesystem::ofstream::binary);
    boost::iostreams::filtering_ostream ofs;
    ofs.push(boost::iostreams::zlib_compressor());
    ofs.push(rawStream);
    if (ofs.good())
    {
        ofs.write(HEADER.data(), HEADER.size());
        ofs.write(reinterpret_cast<char*>(&matchStart), sizeof(uint32));
        size_t mirrorImageDataSize = mirrorImageData.size();
        TC_LOG_DEBUG("arena.replay", "ArenaReplayMgr: Player Count: " UI64FMTD, mirrorImageDataSize);
        ofs.write(reinterpret_cast<char*>(&mirrorImageDataSize), sizeof(size_t));
        for (auto& itr : mirrorImageData)
        {
            uint64 guid = itr.first.GetRawValue();
            size_t size = itr.second.size();
            ofs.write(reinterpret_cast<char*>(&guid), sizeof(uint64));
            ofs.write(reinterpret_cast<char*>(&size), sizeof(size_t));
            ofs.write(reinterpret_cast<char*>(itr.second.contents()), itr.second.size());
        }

        size_t mirrorImageOwnerSize = mirrorImageOwners.size();
        TC_LOG_DEBUG("arena.replay", "ArenaReplayMgr: Mirror Image Count: " UI64FMTD, mirrorImageOwnerSize);
        ofs.write(reinterpret_cast<char*>(&mirrorImageOwnerSize), sizeof(size_t));
        for (auto& itr : mirrorImageOwners)
        {
            uint64 imageGuid = itr.first.GetRawValue();
            uint64 ownerGuid = itr.second.GetRawValue();
            ofs.write(reinterpret_cast<char*>(&imageGuid), sizeof(uint64));
            ofs.write(reinterpret_cast<char*>(&ownerGuid), sizeof(uint64));
        }

        size_t packetCount = GetPackets().size();
        TC_LOG_DEBUG("arena.replay", "ArenaReplayMgr: Packet Count: " UI64FMTD, packetCount);
        ofs.write(reinterpret_cast<char*>(&packetCount), sizeof(size_t));
        for (auto& kvp : packets)
        {
            uint32 timestamp = kvp.first;
            WorldPacket& packet = kvp.second;
            size_t size = packet.size();
            uint16 opcode = packet.GetOpcode();
            TC_LOG_DEBUG("arena.replay", "ArenaReplayMgr: timestamp: %u, size: " UI64FMTD ", opcode: %s", timestamp, size, GetOpcodeNameForLogging(opcode).c_str());
            ofs.write(reinterpret_cast<char*>(&timestamp), sizeof(uint32));
            ofs.write(reinterpret_cast<char*>(&size), sizeof(size_t));
            ofs.write(reinterpret_cast<char*>(&opcode), sizeof(uint16));
            ofs.write(reinterpret_cast<char*>(packet.contents()), size);
        }
    }
    else
        TC_LOG_ERROR("arena.replay", "ArenaReplayMgr: Could not write replay data to file %s", ss.str().c_str());
}

void ReplayEntry::SaveToDB(Battleground* bg, ArenaTeam* team1, ArenaTeam* team2)
{
    std::string name1 = (team1 ? team1->GetName().c_str() : "");
    std::string name2 = (team2 ? team2->GetName().c_str() : "");
    uint8 winner = 2;
    if (bg->GetWinner() != WINNER_NONE)
        winner = ((bg->GetWinner() == WINNER_ALLIANCE) ? 0 : 1);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    PreparedStatement* dStmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_ARENAREPLAY_DATA);
    dStmt->setUInt32(0, id);
    dStmt->setUInt32(1, mapId);
    if (team1 && team2)
    {
        dStmt->setString(2, team1->GetName().c_str());
        dStmt->setString(3, team2->GetName().c_str());
    }
    else
    {
        dStmt->setNull(2);
        dStmt->setNull(3);
    }
    dStmt->setUInt8(4, bg->GetArenaType());
    dStmt->setUInt8(5, winner);
    dStmt->setUInt8(6, uint8(bg->isRated()));
    dStmt->setUInt64(7, matchTime);
    trans->Append(dStmt);

    for (auto& player : playerTeams)
    {
        PreparedStatement* dStmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_ARENAREPLAY_CHAR_DATA);
        dStmt->setUInt32(0, id);
        dStmt->setUInt32(1, player.first.GetCounter());
        dStmt->setUInt8(2, player.second);
        dStmt->setString(3, player::PlayerCache::GetName(player.first));
        trans->Append(dStmt);
    }

    CharacterDatabase.CommitTransaction(trans);
}

void ReplayEntry::LoadPlayerFromDB(Field* fields)
{
    uint32 guid = fields[1].GetUInt32();
    uint8 team = fields[2].GetUInt8();

    playerTeams[ObjectGuid(HighGuid::Player, guid)] = team;
}

void ReplayData::OnPlayerAdded(Player* player, uint32 team)
{
    entry.playerTeams[player->GetGUID()] = team;

    ByteBuffer& data = mirrorImageData[player->GetGUID()];

    data << uint32(player->GetDisplayId());
    data << uint8(player->getRace());
    data << uint8(player->getGender());
    data << uint8(player->getClass());

    data << uint8(player->GetByteValue(PLAYER_BYTES, 0));   // skin
    data << uint8(player->GetByteValue(PLAYER_BYTES, 1));   // face
    data << uint8(player->GetByteValue(PLAYER_BYTES, 2));   // hair
    data << uint8(player->GetByteValue(PLAYER_BYTES, 3));   // haircolor
    data << uint8(player->GetByteValue(PLAYER_BYTES_2, 0)); // facialhair
    data << uint32(player->GetGuildId());                   // guild

    static EquipmentSlots const itemSlots[] =
    {
        EQUIPMENT_SLOT_HEAD,
        EQUIPMENT_SLOT_SHOULDERS,
        EQUIPMENT_SLOT_BODY,
        EQUIPMENT_SLOT_CHEST,
        EQUIPMENT_SLOT_WAIST,
        EQUIPMENT_SLOT_LEGS,
        EQUIPMENT_SLOT_FEET,
        EQUIPMENT_SLOT_WRISTS,
        EQUIPMENT_SLOT_HANDS,
        EQUIPMENT_SLOT_BACK,
        EQUIPMENT_SLOT_TABARD,
        EQUIPMENT_SLOT_END
    };

    // Display items in visible slots
    for (EquipmentSlots const* itr = &itemSlots[0]; *itr != EQUIPMENT_SLOT_END; ++itr)
    {
        if (*itr == EQUIPMENT_SLOT_HEAD && player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_HELM))
            data << uint32(0);
        else if (*itr == EQUIPMENT_SLOT_BACK && player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_CLOAK))
            data << uint32(0);
        else if (Item const* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, *itr))
            data << uint32(item->GetTemplate()->DisplayInfoID);
        else
            data << uint32(0);
    }
}

void ReplayData::AddMirrorImage(ObjectGuid image, ObjectGuid caster)
{
    mirrorImageOwners[image] = caster;
}

ByteBuffer const* ReplayData::GetMirrorImageData(ObjectGuid guid) const
{
    auto itr = mirrorImageOwners.find(guid);
    if (itr == mirrorImageOwners.end())
        return nullptr;
    auto itr2 = mirrorImageData.find(itr->second);
    if (itr2 != mirrorImageData.end())
        return &(itr2->second);
    return nullptr;
}

void ReplayEntry::OnUnload()
{
    if (state == LoadingState::LOADED)
        state = LoadingState::NOT_LOADED;
}

ReplayData::~ReplayData()
{
    entry.OnUnload();
}
