/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

/** \file
    \ingroup u2w
*/

#include "WorldSocket.h"
#include "Config.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "AccountMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Player.h"
#include "Vehicle.h"
#include "ObjectMgr.h"
#include "GuildMgr.h"
#include "Group.h"
#include "Guild.h"
#include "World.h"
#include "BattlegroundMgr.h"
#include "OutdoorPvPMgr.h"
#include "SocialMgr.h"
#include "zlib.h"
#include "ScriptMgr.h"
#include "WardenWin.h"
#include "MoveSpline.h"
#include "RGSuspiciousLog.h"
#include "Monitoring/Module/Network.hpp"
#include "Monitoring/WorldMonitoring.h"

namespace {

const std::string DefaultPlayerName = "<none>";

} // namespace

bool MapSessionFilter::Process(WorldPacket* packet)
{
    OpcodeHandler const& opHandle = opcodeTable[packet->GetOpcode()];

    if (opHandle.packetProcessing == PROCESS_INPLACE)
        return true;

    //we do not process thread-unsafe packets
    if (opHandle.packetProcessing != PROCESS_THREADSAFE_MAP)
        return false;

    Player* player = m_pSession->GetPlayer();
    if (!player)
        return false;

    return player->IsInWorld();
}

//we should process ALL packets when player is not in world/logged in
//OR packet handler is not thread-safe!
bool WorldSessionFilter::Process(WorldPacket* packet)
{
    if(m_pSession->IsCommandExecuting())
        return false;

    OpcodeHandler const& opHandle = opcodeTable[packet->GetOpcode()];
    //check if packet handler is supposed to be safe
    if (opHandle.packetProcessing == PROCESS_INPLACE)
        return true;

    //thread-unsafe packets should be processed in World::UpdateSessions()
    if (opHandle.packetProcessing == PROCESS_THREADUNSAFE)
        return true;

    //no player attached? -> our client! ^^
    Player* player = m_pSession->GetPlayer();
    if (!player)
        return true;

    //lets process all packets for non-in-the-world player
    return (player->IsInWorld() == false);
}

/// WorldSession constructor
WorldSession::WorldSession(uint32 id, std::string&& name, std::shared_ptr<WorldSocket> sock, AccountTypes sec, uint8 expansion, LocaleConstant locale, uint32 recruiter, bool isARecruiter):
    m_timeOutTime(0),
    m_GUIDLow(0),
    _player(NULL),
    m_Socket(sock),
    _security(sec),
    _accountId(id),
    _accountName(std::move(name)),
    m_expansion(expansion),
    _warden(NULL),
    _logoutTime(0),
    m_inQueue(false),
    m_playerLoading(false),
    m_playerLogout(false),
    m_playerRecentlyLogout(false),
    m_playerSave(false),
    m_sessionDbcLocale(sWorld->GetAvailableDbcLocale(locale)),
    m_sessionDbLocaleIndex(locale),
    m_latency(0),
    m_clientTimeDelay(0),
    m_TutorialsChanged(false),
    recruiterId(recruiter),
    isRecruiter(isARecruiter),
    timeLastWhoCommand(0),
    _RBACData(NULL),
    _dropIncoming(false),
    _monitor(this),
    expireTime(60000), // 1 min after socket loss, session is deleted
    forceExit(false),
    m_currentBankerGUID(),
    userRequestedLogout(false),
    muteMgr(*this)
{
    memset(m_Tutorials, 0, sizeof(m_Tutorials));
    memset(_lastAuctionhouseQueries, 0, sizeof(_lastAuctionhouseQueries));

    if (sock)
    {
        m_Address = sock->GetRemoteIpAddress().to_string();
        ResetTimeOutTime();
        LoginDatabase.PExecute("UPDATE account SET online = 1 WHERE id = %u;", GetAccountId());     // One-time query

        MONITOR_WORLD(ReportSession(true));
    }
}

/// WorldSession destructor
WorldSession::~WorldSession()
{
    ///- unload player if not unloaded
    if (GetPlayer())
        LogoutPlayer (true);

    /// - If have unclosed socket, close it
    if (m_Socket)
    {
        m_Socket->CloseSocket();
        m_Socket = nullptr;
    }

    delete _warden;
    delete _RBACData;

    ///- empty incoming packet queue
    _recvQueue.clear();

    LoginDatabase.PExecute("UPDATE account SET online = 0 WHERE id = %u;", GetAccountId());     // One-time query

    MONITOR_WORLD(ReportSession(false));
}

std::string const & WorldSession::GetPlayerName() const
{
    return GetPlayer() != nullptr ? GetPlayer()->GetName() : DefaultPlayerName;
}

std::string WorldSession::GetPlayerInfo() const
{
    std::ostringstream ss;

    ss << "[Player: ";
    if (!m_playerLoading && _player)
        ss << _player->GetName() << ' ' << _player->GetGUID().ToString() << ", ";

    ss << "Account: " << GetAccountId() << "]";

    return ss.str();
}

/// Get player guid if available. Use for logging purposes only
uint32 WorldSession::GetGuidLow() const
{
    return GetPlayer() ? GetPlayer()->GetGUID().GetCounter() : 0;
}

/// Send a packet to the client
// const* overload is for tickets that are reusable
// todo: Should be reference instead
void WorldSession::SendPacket(const WorldPacket& packet)
{
    if (!m_Socket)
        return;

    SendPacket(WorldPacket(packet));
}

void WorldSession::SendPacket(WorldPacket&& packet)
{
    if (!m_Socket)
        return;

#ifdef TRINITY_DEBUG
    // Code for network use statistic
    static uint64 sendPacketCount = 0;
    static uint64 sendPacketBytes = 0;

    static time_t firstTime = time(NULL);
    static time_t lastTime = firstTime;                     // next 60 secs start time

    static uint64 sendLastPacketCount = 0;
    static uint64 sendLastPacketBytes = 0;

    time_t cur_time = time(NULL);

    if ((cur_time - lastTime) < 60)
    {
        sendPacketCount += 1;
        sendPacketBytes += packet.size();

        sendLastPacketCount += 1;
        sendLastPacketBytes += packet.size();
    }
    else
    {
        uint64 minTime = uint64(cur_time - lastTime);
        uint64 fullTime = uint64(lastTime - firstTime);
        TC_LOG_DEBUG("misc", "Send all time packets count: " UI64FMTD " bytes: " UI64FMTD " avr.count/sec: %f avr.bytes/sec: %f time: %u", sendPacketCount, sendPacketBytes, float(sendPacketCount) / fullTime, float(sendPacketBytes) / fullTime, uint32(fullTime));
        TC_LOG_DEBUG("misc", "Send last min packets count: " UI64FMTD " bytes: " UI64FMTD " avr.count/sec: %f avr.bytes/sec: %f", sendLastPacketCount, sendLastPacketBytes, float(sendLastPacketCount) / minTime, float(sendLastPacketBytes) / minTime);

        lastTime = cur_time;
        sendLastPacketCount = 1;
        sendLastPacketBytes = packet.wpos();               // wpos is real written size
    }
#endif                                                      // !TRINITY_DEBUG

    MONITOR_NETWORK(ReportPacketSend(packet.size()));

    if (_monitor.IsLoggingActive())
        _monitor.GetLogger()->LogPacket(RG::Suspicious::PacketLogger::CLIENT_TO_SERVER, packet);

    TC_LOG_TRACE("network.opcode", "S->C: %s %s", GetPlayerInfo().c_str(), GetOpcodeNameForLogging(packet.GetOpcode()).c_str());
    m_Socket->SendPacket(std::move(packet));
}

/// Add an incoming packet to the queue
void WorldSession::QueuePacket(WorldPacket&& new_packet)
{
    if (World::IsAntifloodActive())
    {
//        _recvQueue.lock();
        if (_dropIncoming)
        {
            if (_recvQueue.size() <= World::GetAntifloodMinActive())
                _dropIncoming = false;
            else
            {
                MONITOR_NETWORK(ReportPacketDropped(new_packet.size()));
//                _recvQueue.unlock();
                return;
            }
        }
        else
        {
            if (_recvQueue.size() >= World::GetAntifloodMaxActive())
                _dropIncoming = true;
        }
    }

    if (new_packet.GetOpcode() >= NUM_MSG_TYPES)
    {
        TC_LOG_ERROR("rg.exploit", "Received non-existed opcode %s from %s", GetOpcodeNameForLogging(new_packet.GetOpcode()).c_str(), GetPlayerInfo().c_str());
        MONITOR_NETWORK(ReportPacketDropped(new_packet.size()));
        return;
    }

    MONITOR_NETWORK(ReportPacketReceived(new_packet.size()));
    _recvQueue.add(std::move(new_packet));
}

/// Logging helper for unexpected opcodes
void WorldSession::LogUnexpectedOpcode(WorldPacket* packet, const char* status, const char *reason)
{
    TC_LOG_ERROR("network.opcode", "Received unexpected opcode %s Status: %s Reason: %s from %s",
        GetOpcodeNameForLogging(packet->GetOpcode()).c_str(), status, reason, GetPlayerInfo().c_str());

    _monitor.CheckConditionMeets(RG::Suspicious::Condition::CONDITION_NETWORK_EXCEPTION, RG::Suspicious::NetworkExceptionType::UNEXPECTED_OPCODE);
}

/// Logging helper for unexpected opcodes
void WorldSession::LogUnprocessedTail(WorldPacket* packet)
{
    if (packet->rpos() >= packet->wpos())
        return;

    _monitor.CheckConditionMeets(RG::Suspicious::Condition::CONDITION_NETWORK_EXCEPTION, RG::Suspicious::NetworkExceptionType::UNPROCESSED_TAIL);

    if (!sLog->ShouldLog("network.opcode", LOG_LEVEL_TRACE))
        return;

    TC_LOG_TRACE("network.opcode", "Unprocessed tail data (read stop at %u from %u) Opcode %s from %s",
        uint32(packet->rpos()), uint32(packet->wpos()), GetOpcodeNameForLogging(packet->GetOpcode()).c_str(), GetPlayerInfo().c_str());
    packet->print_storage();
}

void WorldSession::HandlePackets(PacketFilter& updater)
{
    uint32 handled = World::IsAntifloodActive() ? World::GetAntifloodMaxProcess() : -1;

    ///- Retrieve packets from the receive queue and call the appropriate handlers
    /// not process packets if socket already closed
    //! Delete packet after processing by default
    std::vector<WorldPacket> requeuePackets;

    while (m_Socket && handled-- > 0)
    {
        WorldPacket packet(0);
        if (!_recvQueue.next(packet))
            break;

        if (!updater.Process(&packet))
        {
            requeuePackets.push_back(std::move(packet));
            continue;
        }

        OpcodeHandler const& opHandle = opcodeTable[packet.GetOpcode()];
        _monitor.CheckConditionMeets(RG::Suspicious::Condition::CONDITION_NETWORK_EXCEPTION, RG::Suspicious::NetworkExceptionType::OPCODE_UNKNOWN);
        uint32 startTime = 0;
        if (sWorld->getBoolConfig(CONFIG_LAG_REPORT_ENABLED))
            startTime = getMSTime();

        bool reAdd = false;
        try
        {
            switch (opHandle.status)
            {
            case STATUS_LOGGEDIN:
                if (!GetPlayer())
                {
                    // skip STATUS_LOGGEDIN opcode unexpected errors if player logout sometime ago - this can be network lag delayed packets
                    //! If player didn't log out a while ago, it means packets are being sent while the server does not recognize
                    //! the client to be in world yet. We will re-add the packets to the bottom of the queue and process them later.
                    if (!m_playerRecentlyLogout)
                    {
                        reAdd = true;
                        TC_LOG_DEBUG("network", "Re-enqueueing packet with opcode %s with with status STATUS_LOGGEDIN. "
                            "Player is currently not in world yet.", GetOpcodeNameForLogging(packet.GetOpcode()).c_str());
                    }
                }
                else if (GetPlayer()->IsInWorld())
                {
                    (this->*opHandle.handler)(packet);
                    LogUnprocessedTail(&packet);
                }
                // lag can cause STATUS_LOGGEDIN opcodes to arrive after the player started a transfer
                break;
            case STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT:
                if (!GetPlayer() && !m_playerRecentlyLogout && !m_playerLogout) // There's a short delay between GetPlayer() = null and mGetPlayer()RecentlyLogout = true during logout
                    LogUnexpectedOpcode(&packet, "STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT",
                        "the player has not logged in yet and not recently logout");
                else
                {
                    // not expected _player or must checked in packet hanlder
                    (this->*opHandle.handler)(packet);
                    LogUnprocessedTail(&packet);
                }
                break;
            case STATUS_TRANSFER:
                if (!GetPlayer())
                    LogUnexpectedOpcode(&packet, "STATUS_TRANSFER", "the player has not logged in yet");
                else if (GetPlayer()->IsInWorld())
                    LogUnexpectedOpcode(&packet, "STATUS_TRANSFER", "the player is still in world");
                else
                {
                    (this->*opHandle.handler)(packet);
                    LogUnprocessedTail(&packet);
                }
                break;
            case STATUS_AUTHED:
                // prevent cheating with skip queue wait
                if (m_inQueue)
                {
                    LogUnexpectedOpcode(&packet, "STATUS_AUTHED", "the player not pass queue yet");
                    break;
                }

                // some auth opcodes can be recieved before STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT opcodes
                // however when we recieve CMSG_CHAR_ENUM we are surely no longer during the logout process.
                if (packet.GetOpcode() == CMSG_CHAR_ENUM)
                    m_playerRecentlyLogout = false;

                (this->*opHandle.handler)(packet);
                LogUnprocessedTail(&packet);
                break;
            case STATUS_NEVER:
                TC_LOG_ERROR("network.opcode", "Received not allowed opcode %s from %s", GetOpcodeNameForLogging(packet.GetOpcode()).c_str()
                    , GetPlayerInfo().c_str());
                KickPlayer();
                break;
            case STATUS_UNHANDLED:
                TC_LOG_DEBUG("network.opcode", "Received not handled opcode %s from %s", GetOpcodeNameForLogging(packet.GetOpcode()).c_str()
                    , GetPlayerInfo().c_str());
                _monitor.CheckConditionMeets(RG::Suspicious::Condition::CONDITION_NETWORK_EXCEPTION, RG::Suspicious::NetworkExceptionType::OPCODE_UNHANDLED);
                break;
            }
        }
        catch (ByteBufferException const&)
        {
            TC_LOG_ERROR("network", "WorldSession::Update ByteBufferException occured while parsing a packet (opcode: %u) from client %s, accountid=%i. Skipped packet.",
                packet.GetOpcode(), GetRemoteAddress().c_str(), GetAccountId());
            packet.hexlike();

            _monitor.CheckConditionMeets(RG::Suspicious::Condition::CONDITION_NETWORK_EXCEPTION, RG::Suspicious::NetworkExceptionType::EXCEPTION);

            KickPlayer();
        }

        if (GetPlayer() && sWorld->getBoolConfig(CONFIG_LAG_REPORT_ENABLED) && (getMSTime() - startTime) > sWorld->getIntConfig(CONFIG_LAG_REPORT_MAX_DIFF_OPCODES))
            TC_LOG_WARN("rg.lagreport", "[Opcode Lag Report] Diff: %u, Player: %s, OpCodeName: %s", (getMSTime() - startTime), GetPlayer()->GetName().c_str(), opHandle.name);

        if (_monitor.IsLoggingActive())
            _monitor.GetLogger()->LogPacket(RG::Suspicious::PacketLogger::CLIENT_TO_SERVER, packet);

        if(reAdd)
            requeuePackets.push_back(std::move(packet));
    }

    _recvQueue.readd_move(requeuePackets.begin(), requeuePackets.end());
}

/// Update the WorldSession (triggered by World update)
bool WorldSession::Update(uint32 diff)
{
    /// Update Timeout timer.
    UpdateTimeOutTime(diff);

    _monitor.Update(diff);

    ///- Before we process anything:
    /// If necessary, kick the player from the character select screen
    if (IsConnectionIdle())
        m_Socket->CloseSocket();

    if (m_Socket && m_Socket->IsOpen() && _warden)
        _warden->Update();

    ExecutionContext::Run();

    time_t currTime = time(NULL);
    ///- If necessary, log the player out
    if (ShouldLogOut(currTime) && !m_playerLoading)
        LogoutPlayer(true);

    if (m_Socket && GetPlayer() && _warden)
        _warden->Update();

    ///- Cleanup socket pointer if need
    if (m_Socket && !m_Socket->IsOpen())
    {
        expireTime -= expireTime > diff ? diff : expireTime;
        if (expireTime < diff || forceExit || !GetPlayer())
        {
            m_Socket = nullptr;
        }
    }

    GetAddonAPIEndpoint().CancelExpiredRequests();

    if (!m_Socket)
        return false;                                       //Will remove this session from the world session map

    return true;
}

/// %Log the player out
void WorldSession::LogoutPlayer(bool save)
{
    // finish pending transfers before starting the logout
    while (GetPlayer() && GetPlayer()->IsBeingTeleportedFar())
        HandleMoveWorldportAckOpcode();

    m_playerLogout = true;
    m_playerSave = save;

    if (GetPlayer())
    {
        if (ObjectGuid lguid = GetPlayer()->GetLootGUID())
            DoLootRelease(lguid);

        ///- If the player just died before logging out, make him appear as a ghost
        if (GetPlayer()->GetDeathTimer())
        {
            GetPlayer()->getHostileRefManager().deleteReferences();
            GetPlayer()->BuildPlayerRepop();
            GetPlayer()->RepopAtGraveyard();
        }
        else if (GetPlayer()->HasAuraType(SPELL_AURA_SPIRIT_OF_REDEMPTION))
        {
            // this will kill character by SPELL_AURA_SPIRIT_OF_REDEMPTION
            GetPlayer()->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);
            GetPlayer()->KillPlayer();
            GetPlayer()->BuildPlayerRepop();
            GetPlayer()->RepopAtGraveyard();
        }
        else if (GetPlayer()->HasPendingBind())
        {
            GetPlayer()->RepopAtGraveyard();
            GetPlayer()->SetPendingBind(0, 0);
        }

        //drop a flag if player is carrying it
        if (Battleground* bg = GetPlayer()->GetBattleground())
            bg->EventPlayerLoggedOut(GetPlayer());

        // remove player from the group if he is:
        // a) in group; b) not in raid group; c) logging out normally (not being kicked or disconnected); d) not on map 35
        if (_player->GetGroup() && !_player->GetGroup()->isRaidGroup() && (m_Socket && userRequestedLogout) && _player->GetMapId() != 35)
        {
            _player->RemoveFromGroup();
            _player->m_InstanceValid = false;
        }

        ///- Teleport to home if the player is in an invalid instance
        if (!GetPlayer()->m_InstanceValid && !GetPlayer()->IsGameMaster() && GetPlayer()->GetMap()->IsDungeon())
        {
            if (GetPlayer()->GetMapId() == 35) // Verstecktes Anmelden
                GetPlayer()->TeleportTo(571, 5807.75f, 588.346985f, 660.939026f, 1.663f); //Dalaran
            else
                GetPlayer()->TeleportTo(GetPlayer()->m_homebindMapId, GetPlayer()->m_homebindX, GetPlayer()->m_homebindY, GetPlayer()->m_homebindZ, GetPlayer()->GetOrientation());
        }

        if (GetPlayer()->IsWatchingReplay())
            GetPlayer()->TeleportToBGEntryPoint();

        sOutdoorPvPMgr->HandlePlayerLeaveZone(GetPlayer(), GetPlayer()->GetZoneId());

        for (int i=0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
        {
            if (BattlegroundQueueTypeId bgQueueTypeId = GetPlayer()->GetBattlegroundQueueTypeId(i))
            {
                GetPlayer()->RemoveBattlegroundQueueId(bgQueueTypeId);
                BattlegroundQueue& queue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
                queue.RemovePlayer(GetPlayer()->GetGUID(), true);
            }
        }

        // Repop at GraveYard or other player far teleport will prevent saving player because of not present map
        // Teleport player immediately for correct player save
        while (GetPlayer()->IsBeingTeleportedFar())
            HandleMoveWorldportAckOpcode();

        ///- If the player is in a guild, update the guild roster and broadcast a logout message to other guild members
        if (auto guild = sGuildMgr->GetGuildById(GetPlayer()->GetGuildId()))
            guild->HandleMemberLogout(this);

        ///- Remove pet
        GetPlayer()->RemovePet(NULL, PET_SAVE_AS_CURRENT, true);

        ///- Clear whisper whitelist
        GetPlayer()->ClearWhisperWhiteList();

        ///- empty buyback items and save the player in the database
        // some save parts only correctly work in case player present in map/player_lists (pets, etc)
        if (save)
        {
            uint32 eslot;
            for (int j = BUYBACK_SLOT_START; j < BUYBACK_SLOT_END; ++j)
            {
                eslot = j - BUYBACK_SLOT_START;
                GetPlayer()->SetGuidValue(PLAYER_FIELD_VENDORBUYBACK_SLOT_1 + (eslot * 2), ObjectGuid::Empty);
                GetPlayer()->SetUInt32Value(PLAYER_FIELD_BUYBACK_PRICE_1 + eslot, 0);
                GetPlayer()->SetUInt32Value(PLAYER_FIELD_BUYBACK_TIMESTAMP_1 + eslot, 0);
            }
            GetPlayer()->SaveToDB(false, true);
        }

        ///- Leave all channels before player delete...
        GetPlayer()->CleanupChannels();

        ///- If the player is in a group (or invited), remove him. If the group if then only 1 person, disband the group.
        _player->UninviteFromGroup();

        //! Send update to group and reset stored max enchanting level
        if (_player->GetGroup())
        {
            _player->GetGroup()->SendUpdate();
            _player->GetGroup()->ResetMaxEnchantingLevel();
        }

        //! Broadcast a logout message to the player's friends
        sSocialMgr->SendFriendStatus(GetPlayer(), FRIEND_OFFLINE, GetPlayer()->GetGUID(), true);
        sSocialMgr->RemovePlayerSocial(GetPlayer()->GetGUID());

        //! Call script hook before deletion
        sScriptMgr->OnPlayerLogout(GetPlayer());

        uint32 playerGUID = GetPlayer()->GetGUID().GetCounter();

        //! Remove the player from the world
        // the player may not be in the world when logging out
        // e.g if he got disconnected during a transfer to another map
        // calls to GetMap in this case may cause crashes
        GetPlayer()->CleanupsBeforeDelete();
        TC_LOG_INFO("entities.player.character", "Account: %d (IP: %s) Logout Character:[%s] (GUID: %u) Level: %d",
            GetAccountId(), GetRemoteAddress().c_str(), GetPlayer()->GetName().c_str(), GetPlayer()->GetGUID().GetCounter(), GetPlayer()->getLevel());
        if (Map* _map = GetPlayer()->FindMap())
            _map->RemovePlayerFromMap(_player);

        // UUACP - Log logouts
        PreparedStatement* stmt2 = LoginDatabase.GetPreparedStatement(LOGIN_INS_UUACP_LOG);
        stmt2->setUInt32(0, realm.Id.Realm);
        stmt2->setUInt32(1, GetAccountId());
        stmt2->setUInt32(2, playerGUID);
        stmt2->setString(3, GetRemoteAddress().c_str());
        stmt2->setString(4, "logout");
        LoginDatabase.Execute(stmt2);

        SetPlayer(NULL); //! Pointer already deleted during RemovePlayerFromMap

        //! Send the 'logout complete' packet to the client
        //! Client will respond by sending 3x CMSG_CANCEL_TRADE, which we currently dont handle
        WorldPacket data(SMSG_LOGOUT_COMPLETE, 0);
        SendPacket(std::move(data));
        TC_LOG_DEBUG("network", "SESSION: Sent SMSG_LOGOUT_COMPLETE Message");

        //! Since each account can only have one online character at any given time, ensure all characters for active account are marked as offline
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ACCOUNT_ONLINE);
        stmt->setUInt32(0, GetAccountId());
        CharacterDatabase.Execute(stmt);

        MONITOR_WORLD(ReportPlayer(false));
    }

    m_playerLogout = false;
    m_playerSave = false;
    m_playerRecentlyLogout = true;
    LogoutRequest(0);
}

/// Kick a player out of the World
void WorldSession::KickPlayer()
{
    if (m_Socket)
    {
        m_Socket->CloseSocket();
        forceExit = true;
    }
}

void WorldSession::SendNotification(const char *format, ...)
{
    if (format)
    {
        va_list ap;
        char szStr[1024];
        szStr[0] = '\0';
        va_start(ap, format);
        vsnprintf(szStr, 1024, format, ap);
        va_end(ap);

        WorldPacket data(SMSG_NOTIFICATION, (strlen(szStr) + 1));
        data << szStr;
        SendPacket(std::move(data));
    }
}

void WorldSession::SendNotification(uint32 string_id, ...)
{
    char const* format = GetTrinityString(string_id);
    if (format)
    {
        va_list ap;
        char szStr[1024];
        szStr[0] = '\0';
        va_start(ap, string_id);
        vsnprintf(szStr, 1024, format, ap);
        va_end(ap);

        WorldPacket data(SMSG_NOTIFICATION, (strlen(szStr) + 1));
        data << szStr;
        SendPacket(std::move(data));
    }
}

const char *WorldSession::GetTrinityString(int32 entry) const
{
    return sObjectMgr->GetTrinityString(entry, GetSessionDbLocaleIndex());
}

void WorldSession::Handle_NULL(WorldPacket& recvPacket)
{
    TC_LOG_ERROR("network.opcode", "Received unhandled opcode %s from %s"
        , GetOpcodeNameForLogging(recvPacket.GetOpcode()).c_str(), GetPlayerInfo().c_str());
}

void WorldSession::Handle_EarlyProccess(WorldPacket& recvPacket)
{
    TC_LOG_ERROR("network.opcode", "Received opcode %s that must be processed in WorldSocket::OnRead from %s"
        , GetOpcodeNameForLogging(recvPacket.GetOpcode()).c_str(), GetPlayerInfo().c_str());
}

void WorldSession::Handle_ServerSide(WorldPacket& recvPacket)
{
    TC_LOG_ERROR("network.opcode", "Received server-side opcode %s from %s"
        , GetOpcodeNameForLogging(recvPacket.GetOpcode()).c_str(), GetPlayerInfo().c_str());
}

void WorldSession::Handle_Deprecated(WorldPacket& recvPacket)
{
    TC_LOG_ERROR("network.opcode", "Received deprecated opcode %s from %s"
        , GetOpcodeNameForLogging(recvPacket.GetOpcode()).c_str(), GetPlayerInfo().c_str());
}

void WorldSession::SendAuthWaitQue(uint32 position)
{
    if (position == 0)
    {
        WorldPacket packet(SMSG_AUTH_RESPONSE, 1);
        packet << uint8(AUTH_OK);
        SendPacket(std::move(packet));
    }
    else
    {
        WorldPacket packet(SMSG_AUTH_RESPONSE, 6);
        packet << uint8(AUTH_WAIT_QUEUE);
        packet << uint32(position);
        packet << uint8(0);                                 // unk
        SendPacket(std::move(packet));
    }
}

void WorldSession::LoadAccountData(PreparedQueryResult result, uint32 mask)
{
    for (uint32 i = 0; i < NUM_ACCOUNT_DATA_TYPES; ++i)
        if (mask & (1 << i))
            m_accountData[i] = AccountData();

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 type = fields[0].GetUInt8();
        if (type >= NUM_ACCOUNT_DATA_TYPES)
        {
            TC_LOG_ERROR("misc", "Table `%s` have invalid account data type (%u), ignore.",
                mask == GLOBAL_CACHE_MASK ? "account_data" : "character_account_data", type);
            continue;
        }

        if ((mask & (1 << type)) == 0)
        {
            TC_LOG_ERROR("misc", "Table `%s` have non appropriate for table  account data type (%u), ignore.",
                mask == GLOBAL_CACHE_MASK ? "account_data" : "character_account_data", type);
            continue;
        }

        m_accountData[type].Time = time_t(fields[1].GetUInt32());
        m_accountData[type].Data = fields[2].GetString();
    }
    while (result->NextRow());
}

void WorldSession::SetAccountData(AccountDataType type, time_t tm, std::string const& data)
{
    uint32 id = 0;
    CharacterDatabaseStatements index;
    if ((1 << type) & GLOBAL_CACHE_MASK)
    {
        id = GetAccountId();
        index = CHAR_REP_ACCOUNT_DATA;
    }
    else
    {
        // GetPlayer() can be NULL and packet received after logout but m_GUID still store correct guid
        if (!m_GUIDLow)
            return;

        id = m_GUIDLow;
        index = CHAR_REP_PLAYER_ACCOUNT_DATA;
    }

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(index);
    stmt->setUInt32(0, id);
    stmt->setUInt8 (1, type);
    stmt->setUInt32(2, uint32(tm));
    stmt->setString(3, data);
    CharacterDatabase.Execute(stmt);

    m_accountData[type].Time = tm;
    m_accountData[type].Data = data;
}

void WorldSession::SendAccountDataTimes(uint32 mask)
{
    WorldPacket data(SMSG_ACCOUNT_DATA_TIMES, 4 + 1 + 4 + NUM_ACCOUNT_DATA_TYPES * 4);
    data << uint32(time(NULL));                             // Server time
    data << uint8(1);
    data << uint32(mask);                                   // type mask
    for (uint32 i = 0; i < NUM_ACCOUNT_DATA_TYPES; ++i)
        if (mask & (1 << i))
            data << uint32(GetAccountData(AccountDataType(i))->Time);// also unix time
    SendPacket(std::move(data));
}

void WorldSession::LoadTutorialsData(PreparedQueryResult result)
{
    memset(m_Tutorials, 0, sizeof(uint32) * MAX_ACCOUNT_TUTORIAL_VALUES);

    if (result)
        for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
            m_Tutorials[i] = (*result)[i].GetUInt32();

    m_TutorialsChanged = false;
}

void WorldSession::SendTutorialsData()
{
    WorldPacket data(SMSG_TUTORIAL_FLAGS, 4 * MAX_ACCOUNT_TUTORIAL_VALUES);
    for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
        data << m_Tutorials[i];
    SendPacket(std::move(data));
}

void WorldSession::SaveTutorialsData(SQLTransaction &trans)
{
    if (!m_TutorialsChanged)
        return;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_HAS_TUTORIALS);
    stmt->setUInt32(0, GetAccountId());
    bool hasTutorials = bool(CharacterDatabase.Query(stmt));
    // Modify data in DB
    stmt = CharacterDatabase.GetPreparedStatement(hasTutorials ? CHAR_UPD_TUTORIALS : CHAR_INS_TUTORIALS);
    for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
        stmt->setUInt32(i, m_Tutorials[i]);
    stmt->setUInt32(MAX_ACCOUNT_TUTORIAL_VALUES, GetAccountId());
    trans->Append(stmt);

    m_TutorialsChanged = false;
}

void WorldSession::ReadMovementInfo(WorldPacket &data, MovementInfo* mi)
{
    data >> mi->flags;
    data >> mi->flags2;
    data >> mi->time;
    data >> mi->pos;

    if (mi->HasMovementFlag(MOVEMENTFLAG_ONTRANSPORT))
    {
        data >> mi->transport.guid.ReadAsPacked();
        data >> mi->transport.pos;
        data >> mi->transport.time;
        data >> mi->transport.seat;

        if (mi->HasExtraMovementFlag(MOVEMENTFLAG2_INTERPOLATED_MOVEMENT))
            data >> mi->transport.time2;
    }

    if (mi->HasMovementFlag(MovementFlags(MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) || (mi->HasExtraMovementFlag(MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING)))
        data >> mi->pitch;

    data >> mi->fallTime;

    if (mi->HasMovementFlag(MOVEMENTFLAG_FALLING))
    {
        data >> mi->jump.zspeed;
        data >> mi->jump.sinAngle;
        data >> mi->jump.cosAngle;
        data >> mi->jump.xyspeed;
    }

    if (mi->HasMovementFlag(MOVEMENTFLAG_SPLINE_ELEVATION))
        data >> mi->splineElevation;

    //! Anti-cheat checks. Please keep them in seperate if () blocks to maintain a clear overview.
    //! Might be subject to latency, so just remove improper flags.
    #ifdef TRINITY_DEBUG
    #define REMOVE_VIOLATING_FLAGS(check, maskToRemove) \
    { \
        if (check) \
        { \
            TC_LOG_DEBUG("entities.unit", "WorldSession::ReadMovementInfo: Violation of MovementFlags found (%s). " \
                "MovementFlags: %u, MovementFlags2: %u for player GUID: %u. Mask %u will be removed.", \
                STRINGIZE(check), mi->GetMovementFlags(), mi->GetExtraMovementFlags(), GetPlayer()->GetGUID().GetCounter(), maskToRemove); \
            mi->RemoveMovementFlag((maskToRemove)); \
        } \
    }
    #else
    #define REMOVE_VIOLATING_FLAGS(check, maskToRemove) \
        if (check) \
            mi->RemoveMovementFlag((maskToRemove));
    #endif

    /*! This must be a packet spoofing attempt. MOVEMENTFLAG_ROOT sent from the client is not valid
        in conjunction with any of the moving movement flags such as MOVEMENTFLAG_FORWARD.
        It will freeze clients that receive this player's movement info.
    */
    // Only adjust movement flag removal for vehicles with the VEHICLE_FLAG_FIXED_POSITION flag, or the hard coded exceptions below:
    //  30236 | Argent Cannon
    //  39759 | Tankbuster Cannon
    if (GetPlayer()->GetVehicleBase() && ((GetPlayer()->GetVehicle()->GetVehicleInfo()->m_flags & VEHICLE_FLAG_FIXED_POSITION) || GetPlayer()->GetVehicleBase()->GetEntry() == 30236 || GetPlayer()->GetVehicleBase()->GetEntry() == 39759))
    {
        // Actually players in rooted vehicles still send commands, don't clear root for these!
        // Check specifically for the following conditions:
        // MOVEMENTFLAG_ROOT + no other flags          (0x800)
        // MOVEMENTFLAG_ROOT + MOVEMENTFLAG_LEFT       (0x810)
        // MOVEMENTFLAG_ROOT + MOVEMENTFLAG_RIGHT      (0x820)
        // MOVEMENTFLAG_ROOT + MOVEMENTFLAG_PITCH_UP   (0x840)
        // MOVEMENTFLAG_ROOT + MOVEMENTFLAG_PITCH_DOWN (0x880)
        // If none of these are true, clear the root
        if (mi->HasMovementFlag(MOVEMENTFLAG_ROOT) && mi->HasMovementFlag(MOVEMENTFLAG_LEFT | MOVEMENTFLAG_RIGHT | MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN))
            REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_ROOT),
                MOVEMENTFLAG_MASK_MOVING);
    }
    else
    {
        // Only remove here for non vehicles
        REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_ROOT),
            MOVEMENTFLAG_ROOT);
    }

    //! Cannot hover without SPELL_AURA_HOVER
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_HOVER) && !GetPlayer()->HasAuraType(SPELL_AURA_HOVER),
        MOVEMENTFLAG_HOVER);

    //! Cannot ascend and descend at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_ASCENDING) && mi->HasMovementFlag(MOVEMENTFLAG_DESCENDING),
        MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING);

    //! Cannot move left and right at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_LEFT) && mi->HasMovementFlag(MOVEMENTFLAG_RIGHT),
        MOVEMENTFLAG_LEFT | MOVEMENTFLAG_RIGHT);

    //! Cannot strafe left and right at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_STRAFE_LEFT) && mi->HasMovementFlag(MOVEMENTFLAG_STRAFE_RIGHT),
        MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT);

    //! Cannot pitch up and down at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_PITCH_UP) && mi->HasMovementFlag(MOVEMENTFLAG_PITCH_DOWN),
        MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN);

    //! Cannot move forwards and backwards at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_FORWARD) && mi->HasMovementFlag(MOVEMENTFLAG_BACKWARD),
        MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD);

    //! Cannot walk on water without SPELL_AURA_WATER_WALK
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_WATERWALKING) && !GetPlayer()->HasAuraType(SPELL_AURA_WATER_WALK),
        MOVEMENTFLAG_WATERWALKING);

    //! Cannot feather fall without SPELL_AURA_FEATHER_FALL
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_FALLING_SLOW) && !GetPlayer()->HasAuraType(SPELL_AURA_FEATHER_FALL),
        MOVEMENTFLAG_FALLING_SLOW);

    /*! Cannot fly if no fly auras present. Exception is being a GM.
        Note that we check for account level instead of Player::IsGameMaster() because in some
        situations it may be feasable to use .gm fly on as a GM without having .gm on,
        e.g. aerial combat.
    */

    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_FLYING | MOVEMENTFLAG_CAN_FLY) && AccountMgr::IsPlayerAccount(GetSecurity()) &&
        !GetPlayer()->m_mover->HasAuraType(SPELL_AURA_FLY) &&
        !GetPlayer()->m_mover->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED),
        MOVEMENTFLAG_FLYING | MOVEMENTFLAG_CAN_FLY);

    //! Cannot fly and fall at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_DISABLE_GRAVITY) && mi->HasMovementFlag(MOVEMENTFLAG_FALLING),
        MOVEMENTFLAG_FALLING);

    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_SPLINE_ENABLED) &&
        (!GetPlayer()->movespline->Initialized() || GetPlayer()->movespline->Finalized()), MOVEMENTFLAG_SPLINE_ENABLED);

    #undef REMOVE_VIOLATING_FLAGS
}

void WorldSession::WriteMovementInfo(WorldPacket* data, MovementInfo* mi)
{
    *data << mi->guid.WriteAsPacked();
    *data << mi->flags;
    *data << mi->flags2;
    *data << mi->time;
    *data << mi->pos;

    if (mi->HasMovementFlag(MOVEMENTFLAG_ONTRANSPORT))
    {
       *data << mi->transport.guid.WriteAsPacked();
       *data << mi->transport.pos;
       *data << mi->transport.time;
       *data << mi->transport.seat;

       if (mi->HasExtraMovementFlag(MOVEMENTFLAG2_INTERPOLATED_MOVEMENT))
           *data << mi->transport.time2;
    }

    if (mi->HasMovementFlag(MovementFlags(MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) || mi->HasExtraMovementFlag(MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING))
        *data << mi->pitch;

    *data << mi->fallTime;

    if (mi->HasMovementFlag(MOVEMENTFLAG_FALLING))
    {
        *data << mi->jump.zspeed;
        *data << mi->jump.sinAngle;
        *data << mi->jump.cosAngle;
        *data << mi->jump.xyspeed;
    }

    if (mi->HasMovementFlag(MOVEMENTFLAG_SPLINE_ELEVATION))
        *data << mi->splineElevation;
}

void WorldSession::ReadAddonsInfo(ByteBuffer &data)
{
    if (data.rpos() + 4 > data.size())
        return;

    uint32 size;
    data >> size;

    if (!size)
        return;

    if (size > 0xFFFFF)
    {
        TC_LOG_DEBUG("addon", "WorldSession::ReadAddonsInfo: AddOnInfo too big, size %u", size);
        return;
    }

    uLongf uSize = size;

    uint32 pos = data.rpos();

    ByteBuffer addonInfo;
    addonInfo.resize(size);

    if (uncompress(addonInfo.contents(), &uSize, data.contents() + pos, data.size() - pos) == Z_OK)
    {
        uint32 addonsCount;
        addonInfo >> addonsCount;                         // addons count

        for (uint32 i = 0; i < addonsCount; ++i)
        {
            std::string addonName;
            uint8 enabled;
            uint32 crc, unk1;

            // check next addon data format correctness
            if (addonInfo.rpos() + 1 > addonInfo.size())
                return;

            addonInfo >> addonName;

            addonInfo >> enabled >> crc >> unk1;

            TC_LOG_DEBUG("addon", "AddOn: %s (CRC: 0x%x) - enabled: 0x%x - Unknown2: 0x%x", addonName.c_str(), crc, enabled, unk1);

            AddonInfo addon(addonName, enabled, crc, 2, true);

            SavedAddon const* savedAddon = AddonMgr::GetAddonInfo(addonName);
            if (savedAddon)
            {
                if (addon.CRC != savedAddon->CRC)
                    TC_LOG_WARN("addon", " Addon: %s: modified (CRC: 0x%x) - accountID %d)", addon.Name.c_str(), savedAddon->CRC, GetAccountId());
                else
                    TC_LOG_DEBUG("addon", "Addon: %s: validated (CRC: 0x%x) - accountID %d", addon.Name.c_str(), savedAddon->CRC, GetAccountId());
            }
            else
            {
                AddonMgr::SaveAddon(addon);
                TC_LOG_WARN("addon", "Addon: %s: unknown (CRC: 0x%x) - accountId %d (storing addon name and checksum to database)", addon.Name.c_str(), addon.CRC, GetAccountId());
            }

            /// @todo Find out when to not use CRC/pubkey, and other possible states.
            m_addonsList.push_back(addon);
        }

        uint32 currentTime;
        addonInfo >> currentTime;
        TC_LOG_DEBUG("addon", "AddOn: CurrentTime: %u", currentTime);
    }
    else
        TC_LOG_DEBUG("addon", "AddOn: Addon packet uncompress error!");
}

void WorldSession::SendAddonsInfo()
{
    constexpr uint8 addonPublicKey[256] =
    {
        0xC3, 0x5B, 0x50, 0x84, 0xB9, 0x3E, 0x32, 0x42, 0x8C, 0xD0, 0xC7, 0x48, 0xFA, 0x0E, 0x5D, 0x54,
        0x5A, 0xA3, 0x0E, 0x14, 0xBA, 0x9E, 0x0D, 0xB9, 0x5D, 0x8B, 0xEE, 0xB6, 0x84, 0x93, 0x45, 0x75,
        0xFF, 0x31, 0xFE, 0x2F, 0x64, 0x3F, 0x3D, 0x6D, 0x07, 0xD9, 0x44, 0x9B, 0x40, 0x85, 0x59, 0x34,
        0x4E, 0x10, 0xE1, 0xE7, 0x43, 0x69, 0xEF, 0x7C, 0x16, 0xFC, 0xB4, 0xED, 0x1B, 0x95, 0x28, 0xA8,
        0x23, 0x76, 0x51, 0x31, 0x57, 0x30, 0x2B, 0x79, 0x08, 0x50, 0x10, 0x1C, 0x4A, 0x1A, 0x2C, 0xC8,
        0x8B, 0x8F, 0x05, 0x2D, 0x22, 0x3D, 0xDB, 0x5A, 0x24, 0x7A, 0x0F, 0x13, 0x50, 0x37, 0x8F, 0x5A,
        0xCC, 0x9E, 0x04, 0x44, 0x0E, 0x87, 0x01, 0xD4, 0xA3, 0x15, 0x94, 0x16, 0x34, 0xC6, 0xC2, 0xC3,
        0xFB, 0x49, 0xFE, 0xE1, 0xF9, 0xDA, 0x8C, 0x50, 0x3C, 0xBE, 0x2C, 0xBB, 0x57, 0xED, 0x46, 0xB9,
        0xAD, 0x8B, 0xC6, 0xDF, 0x0E, 0xD6, 0x0F, 0xBE, 0x80, 0xB3, 0x8B, 0x1E, 0x77, 0xCF, 0xAD, 0x22,
        0xCF, 0xB7, 0x4B, 0xCF, 0xFB, 0xF0, 0x6B, 0x11, 0x45, 0x2D, 0x7A, 0x81, 0x18, 0xF2, 0x92, 0x7E,
        0x98, 0x56, 0x5D, 0x5E, 0x69, 0x72, 0x0A, 0x0D, 0x03, 0x0A, 0x85, 0xA2, 0x85, 0x9C, 0xCB, 0xFB,
        0x56, 0x6E, 0x8F, 0x44, 0xBB, 0x8F, 0x02, 0x22, 0x68, 0x63, 0x97, 0xBC, 0x85, 0xBA, 0xA8, 0xF7,
        0xB5, 0x40, 0x68, 0x3C, 0x77, 0x86, 0x6F, 0x4B, 0xD7, 0x88, 0xCA, 0x8A, 0xD7, 0xCE, 0x36, 0xF0,
        0x45, 0x6E, 0xD5, 0x64, 0x79, 0x0F, 0x17, 0xFC, 0x64, 0xDD, 0x10, 0x6F, 0xF3, 0xF5, 0xE0, 0xA6,
        0xC3, 0xFB, 0x1B, 0x8C, 0x29, 0xEF, 0x8E, 0xE5, 0x34, 0xCB, 0xD1, 0x2A, 0xCE, 0x79, 0xC3, 0x9A,
        0x0D, 0x36, 0xEA, 0x01, 0xE0, 0xAA, 0x91, 0x20, 0x54, 0xF0, 0x72, 0xD8, 0x1E, 0xC7, 0x89, 0xD2
    };

    WorldPacket data(SMSG_ADDON_INFO, 4);

    for (AddonsList::iterator itr = m_addonsList.begin(); itr != m_addonsList.end(); ++itr)
    {
        data << uint8(itr->State);

        uint8 crcpub = itr->UsePublicKeyOrCRC;
        data << uint8(crcpub);
        if (crcpub)
        {
            uint8 usepk = (itr->CRC != STANDARD_ADDON_CRC); // If addon is Standard addon CRC
            data << uint8(usepk);
            if (usepk)                                      // if CRC is wrong, add public key (client need it)
            {
                TC_LOG_DEBUG("addon", "AddOn: %s: CRC checksum mismatch: got 0x%x - expected 0x%x - sending pubkey to accountID %d",
                    itr->Name.c_str(), itr->CRC, STANDARD_ADDON_CRC, GetAccountId());

                data.append(addonPublicKey, sizeof(addonPublicKey));
            }

            data << uint32(0);                              /// @todo Find out the meaning of this.
        }

        data << uint8(0);       // uses URL
        //if (usesURL)
        //    data << uint8(0); // URL
    }

    m_addonsList.clear();

    AddonMgr::BannedAddonList const* bannedAddons = AddonMgr::GetBannedAddons();
    data << uint32(bannedAddons->size());
    for (AddonMgr::BannedAddonList::const_iterator itr = bannedAddons->begin(); itr != bannedAddons->end(); ++itr)
    {
        data << uint32(itr->Id);
        data.append(itr->NameMD5, sizeof(itr->NameMD5));
        data.append(itr->VersionMD5, sizeof(itr->VersionMD5));
        data << uint32(itr->Timestamp);
        data << uint32(1);  // IsBanned
    }

    SendPacket(std::move(data));
}

void WorldSession::SetPlayer(std::shared_ptr<Player>&& player)
{
    _player = std::move(player);

    // set m_GUID that can be used while player loggined and later until mGetPlayer()RecentlyLogout not reset
    if (GetPlayer())
        m_GUIDLow = GetPlayer()->GetGUID().GetCounter();

    _monitor.SetPlayer(GetPlayer());
}

void WorldSession::InitWarden(BigNumber* k, std::string const& os)
{
    if (os == "Win")
    {
        _warden = new WardenWin();
        _warden->Init(this, k);
    }
    else if (os == "OSX")
    {
        // Disabled as it is causing the client to crash
        // _warden = new WardenMac();
        // _warden->Init(this, k);
    }
}

void WorldSession::LoadPermissions()
{
    uint32 id = GetAccountId();
    uint8 secLevel = GetSecurity();

    _RBACData = new rbac::RBACData(id, _accountName, realm.Id.Realm, secLevel);
    _RBACData->LoadFromDB();
}

PreparedQueryResultFuture WorldSession::LoadPermissionsAsync()
{
    uint32 id = GetAccountId();
    uint8 secLevel = GetSecurity();

    TC_LOG_DEBUG("rbac", "WorldSession::LoadPermissions [AccountId: %u, Name: %s, realmId: %d, secLevel: %u]",
        id, _accountName.c_str(), realm.Id.Realm, secLevel);

    _monitor.Reload();

    _RBACData = new rbac::RBACData(id, _accountName, realm.Id.Realm, secLevel);
    return _RBACData->LoadFromDBAsync();
}

class AccountInfoQueryHolderPerRealm : public SQLQueryHolder
{
public:
    enum
    {
        GLOBAL_ACCOUNT_DATA = 0,
        TUTORIALS,

        MAX_QUERIES
    };

    AccountInfoQueryHolderPerRealm() { SetSize(MAX_QUERIES); }

    bool Initialize(uint32 accountId)
    {
        bool ok = true;

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_DATA);
        stmt->setUInt32(0, accountId);
        ok = SetPreparedQuery(GLOBAL_ACCOUNT_DATA, stmt) && ok;

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_TUTORIALS);
        stmt->setUInt32(0, accountId);
        ok = SetPreparedQuery(TUTORIALS, stmt) && ok;

        return ok;
    }
};

void WorldSession::InitializeSession()
{
    AccountInfoQueryHolderPerRealm* realmHolder = new AccountInfoQueryHolderPerRealm();
    if (!realmHolder->Initialize(GetAccountId()))
    {
        delete realmHolder;
        SendAuthResponse(AUTH_SYSTEM_ERROR, false);
        return;
    }

    CharacterDatabase.DelayQueryHolder(realmHolder)
            .via(GetExecutor()).then([this](QueryResultHolder result) { InitializeSessionCallback(std::move(result)); });
}

void WorldSession::InitializeSessionCallback(QueryResultHolder realmHolder)
{
    LoadAccountData(realmHolder->GetPreparedResult(AccountInfoQueryHolderPerRealm::GLOBAL_ACCOUNT_DATA), GLOBAL_CACHE_MASK);
    LoadTutorialsData(realmHolder->GetPreparedResult(AccountInfoQueryHolderPerRealm::TUTORIALS));

    if (!m_inQueue)
        SendAuthResponse(AUTH_OK, true);
    else
        SendAuthWaitQue(0);

    SetInQueue(false);
    ResetTimeOutTime();

    SendAddonsInfo();
    SendClientCacheVersion(sWorld->getIntConfig(CONFIG_CLIENTCACHE_VERSION));
    SendTutorialsData();
}

rbac::RBACData* WorldSession::GetRBACData()
{
    return _RBACData;
}

bool WorldSession::HasPermission(uint32 permission)
{
    if (!_RBACData)
        LoadPermissions();

    bool hasPermission = _RBACData->HasPermission(permission);
    TC_LOG_DEBUG("rbac", "WorldSession::HasPermission [AccountId: %u, Name: %s, realmId: %d]",
                   _RBACData->GetId(), _RBACData->GetName().c_str(), realm.Id.Realm);

    return hasPermission;
}

void WorldSession::InvalidateRBACData()
{
    TC_LOG_DEBUG("rbac", "WorldSession::Invalidaterbac::RBACData [AccountId: %u, Name: %s, realmId: %d]",
                   _RBACData->GetId(), _RBACData->GetName().c_str(), realm.Id.Realm);
    delete _RBACData;
    _RBACData = NULL;
}

const MuteMgr& WorldSession::GetMuteMgr() const
{
    return muteMgr;
}

MuteMgr& WorldSession::GetMuteMgr()
{
    return muteMgr;
}
