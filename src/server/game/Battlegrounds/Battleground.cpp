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

#include "ArenaTeam.h"
#include "ArenaTeamMgr.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "Creature.h"
#include "CreatureTextMgr.h"
#include "Chat.h"
#include "Formulas.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ReputationMgr.h"
#include "SpellAuras.h"
#include "Util.h"
#include "World.h"
#include "WorldPacket.h"
#include "LFGMgr.h"
#include "Pet.h"
#include "GameEventMgr.h"
#include "Transport.h"
#include "ArenaReplaySystem.h"
#include "GameTime.h"
#include "DBCStores.h"
#include "Packets/WorldPacket.h"

#include "ObjectGuid.h"

namespace Trinity
{
    class BattlegroundChatBuilder
    {
        public:
            BattlegroundChatBuilder(ChatMsg msgtype, int32 textId, Player const* source, va_list* args = NULL)
                : _msgtype(msgtype), _textId(textId), _source(source), _args(args) { }

            void operator()(WorldPacket& data, LocaleConstant loc_idx)
            {
                char const* text = sObjectMgr->GetTrinityString(_textId, loc_idx);
                if (_args)
                {
                    // we need copy va_list before use or original va_list will corrupted
                    va_list ap;
                    va_copy(ap, *_args);

                    char str[2048];
                    vsnprintf(str, 2048, text, ap);
                    va_end(ap);

                    do_helper(data, &str[0]);
                }
                else
                    do_helper(data, text);
            }

        private:
            void do_helper(WorldPacket& data, char const* text)
            {
                ChatHandler::BuildChatPacket(data, _msgtype, LANG_UNIVERSAL, _source, _source, text);
            }

            ChatMsg _msgtype;
            int32 _textId;
            Player const* _source;
            va_list* _args;
    };

    class Battleground2ChatBuilder
    {
        public:
            Battleground2ChatBuilder(ChatMsg msgtype, int32 textId, Player const* source, int32 arg1, int32 arg2)
                : _msgtype(msgtype), _textId(textId), _source(source), _arg1(arg1), _arg2(arg2) { }

            void operator()(WorldPacket& data, LocaleConstant loc_idx)
            {
                char const* text = sObjectMgr->GetTrinityString(_textId, loc_idx);
                char const* arg1str = _arg1 ? sObjectMgr->GetTrinityString(_arg1, loc_idx) : "";
                char const* arg2str = _arg2 ? sObjectMgr->GetTrinityString(_arg2, loc_idx) : "";

                char str[2048];
                snprintf(str, 2048, text, arg1str, arg2str);

                ChatHandler::BuildChatPacket(data, _msgtype, LANG_UNIVERSAL, _source, _source, str);
            }

        private:
            ChatMsg _msgtype;
            int32 _textId;
            Player const* _source;
            int32 _arg1;
            int32 _arg2;
    };
} // namespace Trinity

template<class Do>
void Battleground::BroadcastWorker(Do& _do)
{
    for (BattlegroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
        if (Player* player = _GetPlayer(itr, "BroadcastWorker"))
            _do(player);
}

Battleground::Battleground() : arenaHealReduceLevel(0), isSoloQueueArenaMatch(false)
{
    m_TypeID            = BATTLEGROUND_TYPE_NONE;
    m_RandomTypeID      = BATTLEGROUND_TYPE_NONE;
    m_InstanceID        = 0;
    m_Status            = STATUS_NONE;
    m_ClientInstanceID  = 0;
    m_EndTime           = 0;
    m_BracketId         = BG_BRACKET_ID_FIRST;
    m_InvitedAlliance   = 0;
    m_InvitedHorde      = 0;
    m_ArenaType         = 0;
    m_IsArena           = false;
    m_Winner            = 2;
    m_StartTime         = 0;
    m_ResetStatTimer    = 0;
    m_ValidStartPositionTimer = 0;
    m_Events            = 0;
    m_StartDelayTime    = 0;
    m_IsRated           = false;
    m_BuffChange        = false;
    m_IsRandom          = false;
    m_LevelMin          = 0;
    m_LevelMax          = 0;
    m_InBGFreeSlotQueue = false;
    m_SetDeleteThis     = false;

    m_MaxPlayersPerTeam = 0;
    m_MaxPlayers        = 0;
    m_MinPlayersPerTeam = 0;
    m_MinPlayers        = 0;

    m_MapId             = 0;
    m_Map               = NULL;
    m_StartMaxDist      = 0.0f;
    ScriptId            = 0;

    m_TeamStartLocX[TEAM_ALLIANCE]   = 0;
    m_TeamStartLocX[TEAM_HORDE]      = 0;

    m_TeamStartLocY[TEAM_ALLIANCE]   = 0;
    m_TeamStartLocY[TEAM_HORDE]      = 0;

    m_TeamStartLocZ[TEAM_ALLIANCE]   = 0;
    m_TeamStartLocZ[TEAM_HORDE]      = 0;

    m_TeamStartLocO[TEAM_ALLIANCE]   = 0;
    m_TeamStartLocO[TEAM_HORDE]      = 0;

    m_ArenaTeamIds[TEAM_ALLIANCE]   = 0;
    m_ArenaTeamIds[TEAM_HORDE]      = 0;

    m_ArenaTeamRatingChanges[TEAM_ALLIANCE]   = 0;
    m_ArenaTeamRatingChanges[TEAM_HORDE]      = 0;

    m_ArenaTeamMMR[TEAM_ALLIANCE]   = 0;
    m_ArenaTeamMMR[TEAM_HORDE]      = 0;

    m_BgRaids[TEAM_ALLIANCE]         = NULL;
    m_BgRaids[TEAM_HORDE]            = NULL;

    m_PlayersCount[TEAM_ALLIANCE]    = 0;
    m_PlayersCount[TEAM_HORDE]       = 0;

    m_TeamScores[TEAM_ALLIANCE]      = 0;
    m_TeamScores[TEAM_HORDE]         = 0;

    m_PrematureCountDown = false;
    m_PrematureCountDownTimer = 0;

    m_HonorMode = BG_NORMAL;

    m_ReadyMarkerList.clear();

    StartDelayTimes[BG_STARTING_EVENT_FIRST]  = BG_START_DELAY_2M;
    StartDelayTimes[BG_STARTING_EVENT_SECOND] = BG_START_DELAY_1M;
    StartDelayTimes[BG_STARTING_EVENT_THIRD]  = BG_START_DELAY_30S;
    StartDelayTimes[BG_STARTING_EVENT_FOURTH] = BG_START_DELAY_NONE;
    //we must set to some default existing values
    StartMessageIds[BG_STARTING_EVENT_FIRST]  = LANG_BG_WS_START_TWO_MINUTES;
    StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_WS_START_ONE_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_WS_START_HALF_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_WS_HAS_BEGUN;

    bgTeamRating[TEAM_ALLIANCE] = 0.f;
    bgTeamRating[TEAM_HORDE] = 0.f;
    bgTeamRD[TEAM_ALLIANCE] = 0.f;
    bgTeamRD[TEAM_HORDE] = 0.f;
    ratingSampleSize = 0;
    ratingSampleTimer = 30 * IN_MILLISECONDS;
}

Battleground::~Battleground()
{
    // remove objects and creatures
    // (this is done automatically in mapmanager update, when the instance is reset after the reset time)
    uint32 size = uint32(BgCreatures.size());
    for (uint32 i = 0; i < size; ++i)
        DelCreature(i);

    size = uint32(BgObjects.size());
    for (uint32 i = 0; i < size; ++i)
        DelObject(i);

    sBattlegroundMgr->RemoveBattleground(GetTypeID(), GetInstanceID());
    // unload map
    if (m_Map)
    {
        m_Map->SetUnload();
        //unlink to prevent crash, always unlink all pointer reference before destruction
        m_Map->SetBG(NULL);
        m_Map = NULL;
    }
    // remove from bg free slot queue
    RemoveFromBGFreeSlotQueue();

    for (BattlegroundScoreMap::const_iterator itr = PlayerScores.begin(); itr != PlayerScores.end(); ++itr)
        delete itr->second;
}

void Battleground::Update(uint32 diff)
{
    if (!PreUpdateImpl(diff))
        return;
    
    if (!GetPlayersSize())
    {
        //BG is empty
        // if there are no players invited, delete BG
        // this will delete arena or bg object, where any player entered
        // [[   but if you use battleground object again (more battles possible to be played on 1 instance)
        //      then this condition should be removed and code:
        //      if (!GetInvitedCount(HORDE) && !GetInvitedCount(ALLIANCE))
        //          this->AddToFreeBGObjectsQueue(); // not yet implemented
        //      should be used instead of current
        // ]]
        // Battleground Template instance cannot be updated, because it would be deleted
        if (!GetInvitedCount(HORDE) && !GetInvitedCount(ALLIANCE))
        {
            sArenaReplayMgr->BattlegroundEnded(this, NULL, NULL, false);
            m_SetDeleteThis = true;
        }
        return;
    }

    switch (GetStatus())
    {
        case STATUS_WAIT_JOIN:
            if (GetPlayersSize())
            {
                _ProcessJoin(diff);
                _CheckSafePositions(diff);
            }
            break;
        case STATUS_IN_PROGRESS:
            _ProcessOfflineQueue();
            // after 47 minutes without one team losing, the arena closes with no winner and no rating change
            if (isArena())
            {
                UpdateHealReduceLevel();
                if (GetStartTime() >= 47 * MINUTE*IN_MILLISECONDS)
                {
                    UpdateArenaWorldState();
                    CheckArenaAfterTimerConditions();
                    return;
                }
            }
            else
            {
                for (auto itr = m_LastResurrectTime.begin(); itr != m_LastResurrectTime.end(); ++itr)
                    itr->second = itr->second + diff;
                if (sBattlegroundMgr->GetPrematureFinishTime() && (GetPlayersCountByTeam(ALLIANCE) < GetMinPlayersPerTeam() || GetPlayersCountByTeam(HORDE) < GetMinPlayersPerTeam()))
                    _ProcessProgress(diff);
                else if (m_PrematureCountDown)
                    m_PrematureCountDown = false;
                UpdateRatingTimer(diff);
            }
            break;
        case STATUS_WAIT_LEAVE:
            _ProcessLeave(diff);
            break;
        default:
            break;
    }

    // Update start time and reset stats timer
    m_StartTime += diff;
    m_ResetStatTimer += diff;

    PostUpdateImpl(diff);
}

inline void Battleground::_CheckSafePositions(uint32 diff)
{
    float maxDist = GetStartMaxDist();
    if (!maxDist)
        return;

    m_ValidStartPositionTimer += diff;
    if (m_ValidStartPositionTimer >= CHECK_PLAYER_POSITION_INVERVAL)
    {
        m_ValidStartPositionTimer = 0;

        Position pos;
        float x, y, z, o;
        for (BattlegroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
            if (Player* player = ObjectAccessor::FindPlayer(itr->first))
            {
                player->GetPosition(&pos);
                GetTeamStartLoc(player->GetTeam(), x, y, z, o);
                if (!player->isDeveloper() && pos.GetExactDistSq(x, y, z) > maxDist)
                {
                    TC_LOG_DEBUG("bg.battleground", "BATTLEGROUND: Sending %s back to start location (map: %u) (possible exploit)", player->GetName().c_str(), GetMapId());
                    player->TeleportTo(GetMapId(), x, y, z, o);
                }
            }
    }
}

inline void Battleground::_ProcessOfflineQueue()
{
    // remove offline players from bg after 5 minutes
    if (!m_OfflineQueue.empty())
    {
        BattlegroundPlayerMap::iterator itr = m_Players.find(*(m_OfflineQueue.begin()));
        if (itr != m_Players.end())
        {
            if (itr->second.OfflineRemoveTime <= game::time::GetGameTime())
            {
                RemovePlayerAtLeave(itr->first, true, true);// remove player from BG
                m_OfflineQueue.pop_front();                 // remove from offline queue
                //do not use itr for anything, because it is erased in RemovePlayerAtLeave()
            }
        }
    }
}

uint32 Battleground::GetPrematureWinner()
{
    uint32 winner = 0;
    if (GetPlayersCountByTeam(ALLIANCE) >= GetMinPlayersPerTeam())
        winner = ALLIANCE;
    else if (GetPlayersCountByTeam(HORDE) >= GetMinPlayersPerTeam())
        winner = HORDE;

    return winner;
}

inline void Battleground::_ProcessProgress(uint32 diff)
{
    // *********************************************************
    // ***           BATTLEGROUND BALLANCE SYSTEM            ***
    // *********************************************************
    // if less then minimum players are in on one side, then start premature finish timer
    if (!m_PrematureCountDown)
    {
        m_PrematureCountDown = true;
        m_PrematureCountDownTimer = sBattlegroundMgr->GetPrematureFinishTime();
    }
    else if (m_PrematureCountDownTimer < diff)
    {
        // time's up!
        EndBattleground(GetPrematureWinner());
        m_PrematureCountDown = false;
    }
    else if (!sBattlegroundMgr->isTesting())
    {
        uint32 newtime = m_PrematureCountDownTimer - diff;
        // announce every minute
        if (newtime > (MINUTE * IN_MILLISECONDS))
        {
            if (newtime / (MINUTE * IN_MILLISECONDS) != m_PrematureCountDownTimer / (MINUTE * IN_MILLISECONDS))
                PSendMessageToAll(LANG_BATTLEGROUND_PREMATURE_FINISH_WARNING, CHAT_MSG_SYSTEM, NULL, (uint32)(m_PrematureCountDownTimer / (MINUTE * IN_MILLISECONDS)));
        }
        else
        {
            //announce every 15 seconds
            if (newtime / (15 * IN_MILLISECONDS) != m_PrematureCountDownTimer / (15 * IN_MILLISECONDS))
                PSendMessageToAll(LANG_BATTLEGROUND_PREMATURE_FINISH_WARNING_SECS, CHAT_MSG_SYSTEM, NULL, (uint32)(m_PrematureCountDownTimer / IN_MILLISECONDS));
        }
        m_PrematureCountDownTimer = newtime;
    }
}

inline void Battleground::_ProcessJoin(uint32 diff)
{
    // *********************************************************
    // ***           BATTLEGROUND STARTING SYSTEM            ***
    // *********************************************************
    ModifyStartDelayTime(diff);

    if (m_ResetStatTimer > 5000)
    {
        m_ResetStatTimer = 0;
        for (BattlegroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
            if (Player* player = ObjectAccessor::GetPlayer(*m_Map, itr->first))
            {
                player->ResetAllPowers();
                if (Pet* pet = player->GetPet())
                    pet->SetHealth(pet->GetMaxHealth());
            }
    }

    if (!(m_Events & BG_STARTING_EVENT_1))
    {
        m_Events |= BG_STARTING_EVENT_1;

        if (!FindBgMap())
        {
            TC_LOG_ERROR("bg.battleground", "Battleground::_ProcessJoin: map (map id: %u, instance id: %u) is not created!", m_MapId, m_InstanceID);
            EndNow();
            return;
        }

        // Setup here, only when at least one player has ported to the map
        if (!SetupBattleground())
        {
            EndNow();
            return;
        }

        StartingEventCloseDoors();
        SetStartDelayTime(StartDelayTimes[BG_STARTING_EVENT_FIRST]);
        // First start warning - 2 or 1 minute
        SendMessageToAll(StartMessageIds[BG_STARTING_EVENT_FIRST], CHAT_MSG_BG_SYSTEM_NEUTRAL);

        uint32 minlevel = GetMinLevel();
        uint32 maxlevel = GetMaxLevel();

        // replace hardcoded max level by player max level for nice output
        if (maxlevel > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
            maxlevel = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);

        // Announce BG starting
        if (isBattleground() && sWorld->getBoolConfig(CONFIG_BATTLEGROUND_QUEUE_ANNOUNCER_ENABLE))
            sWorld->SendWorldText(LANG_BG_STARTED_ANNOUNCE_WORLD, minlevel, maxlevel);
    }
    // After 1 minute or 30 seconds, warning is signaled
    else if (GetStartDelayTime() <= StartDelayTimes[BG_STARTING_EVENT_SECOND] && !(m_Events & BG_STARTING_EVENT_2))
    {
        m_Events |= BG_STARTING_EVENT_2;
        SendMessageToAll(StartMessageIds[BG_STARTING_EVENT_SECOND], CHAT_MSG_BG_SYSTEM_NEUTRAL);
    }
    // After 30 or 15 seconds, warning is signaled
    else if (GetStartDelayTime() <= StartDelayTimes[BG_STARTING_EVENT_THIRD] && !(m_Events & BG_STARTING_EVENT_3))
    {
        m_Events |= BG_STARTING_EVENT_3;
        SendMessageToAll(StartMessageIds[BG_STARTING_EVENT_THIRD], CHAT_MSG_BG_SYSTEM_NEUTRAL);
    }
    // Delay expired (after 2 or 1 minute)
    else if (GetStartDelayTime() <= 0 && !(m_Events & BG_STARTING_EVENT_4))
    {
        m_Events |= BG_STARTING_EVENT_4;

        StartingEventOpenDoors();

        SendWarningToAll(StartMessageIds[BG_STARTING_EVENT_FOURTH]);
        SetStatus(STATUS_IN_PROGRESS);
        SetStartDelayTime(StartDelayTimes[BG_STARTING_EVENT_FOURTH]);

        if (auto replay = GetBgMap()->GetReplayData())
            replay->SetMatchStart();

        // Remove preparation
        if (isArena())
        {
            /// @todo add arena sound PlaySoundToAll(SOUND_ARENA_START);
            for (BattlegroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
                if (Player* player = ObjectAccessor::GetPlayer(*m_Map, itr->first))
                {
                    // BG Status packet
                    BattlegroundQueueTypeId bgQueueTypeId = sBattlegroundMgr->BGQueueTypeId(m_TypeID, GetArenaType());
                    uint32 queueSlot = player->GetBattlegroundQueueIndex(bgQueueTypeId);
                    WorldPacket status = sBattlegroundMgr->BuildBattlegroundStatusPacket(this, queueSlot, STATUS_IN_PROGRESS, 0, GetStartTime(), GetArenaType(), player->GetTeam());
                    player->SendDirectMessage(std::move(status));

                    if (GameObject* table = player->FindNearestGameObject(ARENA_MAGE_TABLE, 1000.0f))
                    {
                        table->Delete();
                    }

                    
                    if (GameObject* soulwell = player->FindNearestGameObject(ARENA_WARLOCK_SOULWELL, 1000.0f))
                    {
                        soulwell->Delete();
                    }

                    if (GameObject* soulwell2 = player->FindNearestGameObject(ARENA_WARLOCK_SOULWELL_TWO, 1000.0f))
                    {
                        soulwell2->Delete();
                    }

                    if (GameObject* soulwell3 = player->FindNearestGameObject(ARENA_WARLOCK_SOULWELL_THREE, 1000.0f))
                    {
                        soulwell3->Delete();
                    }


                    player->RemoveAurasDueToSpell(SPELL_ARENA_PREPARATION);
                    player->ResetAllPowers();
                    player->SetCommandStatusOff(CHEAT_CASTTIME);
                    if (!player->IsGameMaster())
                    {
                        // remove auras with duration lower than 30s
                        Unit::AuraApplicationMap & auraMap = player->GetAppliedAuras();
                        for (Unit::AuraApplicationMap::iterator iter = auraMap.begin(); iter != auraMap.end();)
                        {
                            AuraApplication * aurApp = iter->second;
                            Aura* aura = aurApp->GetBase();
                            if (!aura->IsPermanent()
                                && aura->GetDuration() <= 30*IN_MILLISECONDS
                                && aurApp->IsPositive()
                                && (!aura->GetSpellInfo()->HasAttribute(SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY))
                                && (!aura->HasEffectType(SPELL_AURA_MOD_INVISIBILITY)))
                                player->RemoveAura(iter);
                            else
                                ++iter;
                        }
                    }
                }

            CheckArenaWinConditions();
        }
        else
        {
            PlaySoundToAll(SOUND_BG_START);

            for (BattlegroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
                if (Player* player = ObjectAccessor::GetPlayer(*m_Map, itr->first))
                {
                    player->RemoveAurasDueToSpell(SPELL_PREPARATION);
                    player->ResetAllPowers();
                }
        }
    }
    if (GetStartDelayTime() <= 5 * IN_MILLISECONDS && (m_Events & BG_STARTING_EVENT_3))
    {
        for (BattlegroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
            if (Player* player = ObjectAccessor::GetPlayer(*m_Map, itr->first))
                player->SetCommandStatusOff(CHEAT_CASTTIME);
    }
}

inline void Battleground::_ProcessLeave(uint32 diff)
{
    // *********************************************************
    // ***           BATTLEGROUND ENDING SYSTEM              ***
    // *********************************************************
    // remove all players from battleground after 2 minutes
    m_EndTime -= diff;
    if (m_EndTime <= 0)
    {
        m_EndTime = 0;
        BattlegroundPlayerMap::iterator itr, next;
        for (itr = m_Players.begin(); itr != m_Players.end(); itr = next)
        {
            next = itr;
            ++next;
            //itr is erased here!
            RemovePlayerAtLeave(itr->first, true, true);// remove player from BG
            // do not change any battleground's private variables
        }
    }
}

inline Player* Battleground::_GetPlayer(ObjectGuid guid, bool offlineRemove, char const* context) const
{
    Player* player = NULL;
    if (!offlineRemove)
    {
        // should this be ObjectAccessor::FindConnectedPlayer() to return players teleporting ?
        player = ObjectAccessor::GetPlayer(*GetBgMap(), guid);
        if (!player)
            TC_LOG_ERROR("bg.battleground", "Battleground::%s: player (GUID: %u) not found for BG (map: %u, instance id: %u)!",
                context, guid.GetCounter(), m_MapId, m_InstanceID);
    }
    return player;
}

inline Player* Battleground::_GetPlayer(BattlegroundPlayerMap::iterator itr, char const* context)
{
    return _GetPlayer(itr->first, itr->second.OfflineRemoveTime, context);
}

inline Player* Battleground::_GetPlayer(BattlegroundPlayerMap::const_iterator itr, char const* context) const
{
    return _GetPlayer(itr->first, itr->second.OfflineRemoveTime, context);
}

inline Player* Battleground::_GetPlayerForTeam(uint32 teamId, BattlegroundPlayerMap::const_iterator itr, char const* context) const
{
    Player* player = _GetPlayer(itr, context);
    if (player)
    {
        uint32 team = itr->second.Team;
        if (!team)
            team = player->GetTeam();
        if (team != teamId)
            player = NULL;
    }
    return player;
}

void Battleground::SetTeamStartLoc(uint32 TeamID, float X, float Y, float Z, float O)
{
    TeamId idx = GetTeamIndexByTeamId(TeamID);
    m_TeamStartLocX[idx] = X;
    m_TeamStartLocY[idx] = Y;
    m_TeamStartLocZ[idx] = Z;
    m_TeamStartLocO[idx] = O;
}

void Battleground::SendPacketToAll(const WorldPacket& packet)
{
    for (BattlegroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
        if (Player* player = _GetPlayer(itr, "SendPacketToAll"))
            player->SendDirectMessage(packet);

    if (auto replay = GetBgMap()->GetReplayData())
        replay->AddPacket(packet);
}

void Battleground::SendPacketToTeam(uint32 TeamID, const WorldPacket& packet, Player* sender, bool self)
{
    for (BattlegroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        if (Player* player = _GetPlayerForTeam(TeamID, itr, "SendPacketToTeam"))
        {
            if (self || sender != player)
                player->SendDirectMessage(packet);
        }
    }
}

void Battleground::SendChatMessage(Creature* source, uint8 textId, WorldObject* target /*= NULL*/)
{
    sCreatureTextMgr->SendChat(source, textId, target);
}

void Battleground::PlaySoundToAll(uint32 SoundID)
{
    WorldPacket data = sBattlegroundMgr->BuildPlaySoundPacket(SoundID);
    SendPacketToAll(data);
}

void Battleground::PlaySoundToTeam(uint32 SoundID, uint32 TeamID)
{
    WorldPacket data = sBattlegroundMgr->BuildPlaySoundPacket(SoundID);
    for (BattlegroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
        if (Player* player = _GetPlayerForTeam(TeamID, itr, "PlaySoundToTeam"))
            player->SendDirectMessage(data);
}

void Battleground::CastSpellOnTeam(uint32 SpellID, uint32 TeamID)
{
    for (BattlegroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
        if (Player* player = _GetPlayerForTeam(TeamID, itr, "CastSpellOnTeam"))
            player->CastSpell(player, SpellID, true);
}

void Battleground::RemoveAuraOnTeam(uint32 SpellID, uint32 TeamID)
{
    for (BattlegroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
        if (Player* player = _GetPlayerForTeam(TeamID, itr, "RemoveAuraOnTeam"))
            player->RemoveAura(SpellID);
}

void Battleground::RewardHonorToTeam(uint32 Honor, uint32 TeamID)
{
    for (BattlegroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
        if (Player* player = _GetPlayerForTeam(TeamID, itr, "RewardHonorToTeam"))
            UpdatePlayerScore(player, SCORE_BONUS_HONOR, Honor);

    RewardExperienceToTeam(float(Honor) / float(GetBonusHonorFromKill(1)), TeamID);
}

void Battleground::RewardExperienceToTeam(float multiplier, uint32 TeamID)
{
    for (BattlegroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
        if (Player* player = _GetPlayerForTeam(TeamID, itr, "RewardExperienceToTeam"))
        {
            float percent;
            if (player->getLevel() < 60)
                percent = 5.f - 2.5f * (float(player->getLevel() - 10) / 50.f);
            else if (player->getLevel() < 70)
                percent = 2.5f - 1.5f * (float(player->getLevel() - 60) / 10.f);
            else
                percent = 1.f;
            uint32 xp = uint32(multiplier * percent / 100.f * player->GetUInt32Value(PLAYER_NEXT_LEVEL_XP));
            player->GiveXP(xp, nullptr);
        }
}

void Battleground::RewardReputationToTeam(uint32 a_faction_id, uint32 h_faction_id, uint32 Reputation, uint32 teamId)
{
    FactionEntry const* a_factionEntry = sFactionStore.LookupEntry(a_faction_id);
    FactionEntry const* h_factionEntry = sFactionStore.LookupEntry(h_faction_id);

    if (!a_factionEntry || !h_factionEntry)
        return;

    for (BattlegroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        if (itr->second.OfflineRemoveTime)
            continue;

        Player* plr = ObjectAccessor::GetPlayer(*m_Map, itr->first);

        if (!plr)
        {
            TC_LOG_ERROR("bg.battleground", "BattleGround:RewardReputationToTeam: %s not found!", itr->first.ToString().c_str());
            continue;
        }            

        uint32 team = plr->GetTeam();

        if (team == teamId)
        {
            uint32 repGain = Reputation;
            uint32 faction_id = plr->GetOTeam() == ALLIANCE ? a_faction_id : h_faction_id;
            auto factionEntry = plr->GetOTeam() == ALLIANCE ? a_factionEntry : h_factionEntry;
            AddPct(repGain, plr->GetTotalAuraModifier(SPELL_AURA_MOD_REPUTATION_GAIN));
            AddPct(repGain, plr->GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_FACTION_REPUTATION_GAIN, faction_id));
            plr->GetReputationMgr().ModifyReputation(factionEntry, repGain);
        }
    }
}

void Battleground::UpdateWorldState(uint32 Field, uint32 Value)
{
    WorldPacket data = sBattlegroundMgr->BuildUpdateWorldStatePacket(Field, Value);
    SendPacketToAll(data);
}

void Battleground::UpdateWorldStateForPlayer(uint32 field, uint32 value, Player* player)
{
    WorldPacket data = sBattlegroundMgr->BuildUpdateWorldStatePacket(field, value);
    player->SendDirectMessage(std::move(data));
}

const uint32 HEAL_REDUCE_SPELL = 38770;
const uint32 HEAL_REDUCE_MAX_LEVEL = 15;
const uint32 HEAL_REDUCE_INITIAL_TIME = 15 * MINUTE * IN_MILLISECONDS;
const uint32 HEAL_REDUCE_INTERVAL = 100 * IN_MILLISECONDS;

void Battleground::UpdateHealReduceLevel()
{
    if (!isArena())
        return;

    if (GetStartTime() >= HEAL_REDUCE_INITIAL_TIME + HEAL_REDUCE_INTERVAL * arenaHealReduceLevel)
    {
        arenaHealReduceLevel = std::min(arenaHealReduceLevel + 1, HEAL_REDUCE_MAX_LEVEL);
        for (BattlegroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
        {
            Player* player = ObjectAccessor::GetPlayer(*m_Map, itr->first);
            if (player)
            {
                if (auto aura = player->AddAura(HEAL_REDUCE_SPELL, player))
                {
                    aura->SetStackAmount(arenaHealReduceLevel);
                    aura->SetMaxDuration(3 * MINUTE * IN_MILLISECONDS);
                    aura->RefreshDuration();
                }
            }
        }
    }
}

void Battleground::EndBattleground(uint32 winner)
{
    RemoveFromBGFreeSlotQueue();

    ArenaTeam* winnerArenaTeam = NULL;
    ArenaTeam* loserArenaTeam = NULL;

    uint32 loserTeamRating = 0;
    uint32 loserMatchmakerRating = 0;
    int32  loserChange = 0;
    int32  loserMatchmakerChange = 0;
    uint32 winnerTeamRating = 0;
    uint32 winnerMatchmakerRating = 0;
    int32  winnerChange = 0;
    int32  winnerMatchmakerChange = 0;

    int32 winmsg_id = 0;

    if (winner == ALLIANCE)
    {
        winmsg_id = isBattleground() ? LANG_BG_A_WINS : LANG_ARENA_GOLD_WINS;

        PlaySoundToAll(SOUND_ALLIANCE_WINS);                // alliance wins sound

        SetWinner(WINNER_ALLIANCE);
    }
    else if (winner == HORDE)
    {
        winmsg_id = isBattleground() ? LANG_BG_H_WINS : LANG_ARENA_GREEN_WINS;

        PlaySoundToAll(SOUND_HORDE_WINS);                   // horde wins sound

        SetWinner(WINNER_HORDE);
    }
    else
    {
        SetWinner(3);
    }

    SQLTransaction pvpStatsTransaction;
    uint64 pvpStatsId = 0;
    if (isBattleground() && sWorld->getBoolConfig(CONFIG_BATTLEGROUND_STORE_STATISTICS_ENABLE))
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PVPSTATS_MAXID);

        if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
        {
            Field* fields = result->Fetch();
            pvpStatsId = fields[0].GetUInt64() + 1;
        }

        pvpStatsTransaction = CharacterDatabase.BeginTransaction();
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PVPSTATS_BATTLEGROUND_WITH_ID);
        stmt->setUInt64(0, pvpStatsId);
        stmt->setUInt8(1, GetWinner());
        stmt->setUInt8(2, GetUniqueBracketId());
        stmt->setUInt8(3, GetTypeID(true));
        pvpStatsTransaction->Append(stmt);
    }


    SetStatus(STATUS_WAIT_LEAVE);
    //we must set it this way, because end time is sent in packet!
    m_EndTime = TIME_TO_AUTOREMOVE;

    // arena rating calculation
    if (isArena() && isRated())
    {
        if (isSoloQueueArenaMatch)
            UpdateArenaRating(winner);
        else
        {
        winnerArenaTeam = sArenaTeamMgr->GetArenaTeamById(GetArenaTeamIdForTeam(winner));
        loserArenaTeam = sArenaTeamMgr->GetArenaTeamById(GetArenaTeamIdForTeam(GetOtherTeam(winner)));

        if (winnerArenaTeam && loserArenaTeam && winnerArenaTeam != loserArenaTeam)
        {
            if (winner != WINNER_NONE)
            {
                loserTeamRating = loserArenaTeam->GetRating();
                loserMatchmakerRating = GetArenaMatchmakerRating(GetOtherTeam(winner));
                winnerTeamRating = winnerArenaTeam->GetRating();
                winnerMatchmakerRating = GetArenaMatchmakerRating(winner);
                winnerMatchmakerChange = winnerArenaTeam->WonAgainst(winnerMatchmakerRating, loserMatchmakerRating, winnerChange);
                loserMatchmakerChange = loserArenaTeam->LostAgainst(loserMatchmakerRating, winnerMatchmakerRating, loserChange);
                TC_LOG_DEBUG("bg.arena", "match Type: %u --- Winner: old rating: %u, rating gain: %d, old MMR: %u, MMR gain: %d --- Loser: old rating: %u, rating loss: %d, old MMR: %u, MMR loss: %d ---", m_ArenaType, winnerTeamRating, winnerChange, winnerMatchmakerRating,
                    winnerMatchmakerChange, loserTeamRating, loserChange, loserMatchmakerRating, loserMatchmakerChange);
                SetArenaMatchmakerRating(winner, winnerMatchmakerRating + winnerMatchmakerChange);
                SetArenaMatchmakerRating(GetOtherTeam(winner), loserMatchmakerRating + loserMatchmakerChange);
                SetArenaTeamRatingChangeForTeam(winner, winnerChange);
                SetArenaTeamRatingChangeForTeam(GetOtherTeam(winner), loserChange);
                TC_LOG_DEBUG("bg.arena", "Arena match Type: %u for Team1Id: %u - Team2Id: %u ended. WinnerTeamId: %u. Winner rating: +%d, Loser rating: %d", m_ArenaType, m_ArenaTeamIds[TEAM_ALLIANCE], m_ArenaTeamIds[TEAM_HORDE], winnerArenaTeam->GetId(), winnerChange, loserChange);

                /** World of Warcraft Armory **/
                uint32 gameID = 0;
                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ARMORY_ARENA_GAMES_MAXID);

                if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
                {
                    Field* fields = result->Fetch();
                    gameID = fields[0].GetUInt32() + 1;
                }

                SQLTransaction armoryArenaTransaction = CharacterDatabase.BeginTransaction();

                for (BattlegroundScoreMap::const_iterator itr = PlayerScores.begin(); itr != PlayerScores.end(); ++itr)
                {
                    const auto player = ObjectAccessor::FindConnectedPlayer(itr->first);
                    if (!player)
                        continue;

                    uint32 playerTeamId = player->GetArenaTeamId(winnerArenaTeam->GetSlot());

                    uint8 changeType = 2; // Loss
                    uint32 resultRating = loserTeamRating;
                    uint32 resultTeamID = loserArenaTeam->GetId();
                    int32 ratingChange = loserChange;

                    if (playerTeamId == winnerArenaTeam->GetId())
                    {
                        changeType = 1; // win
                        resultRating = winnerTeamRating;
                        resultTeamID = playerTeamId;
                        ratingChange = winnerChange;
                    }

                    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_ARMORY_ARENA_GAMES);
                    stmt->setUInt32(0, gameID);
                    stmt->setUInt32(1, resultTeamID);
                    stmt->setUInt64(2, player->GetGUID().GetRawValue());
                    stmt->setUInt8(3, changeType);
                    stmt->setInt32(4, ratingChange);
                    stmt->setUInt32(5, resultRating);
                    stmt->setUInt32(6, itr->second->GetDamageDone());
                    stmt->setUInt32(7, itr->second->GetDeaths());
                    stmt->setUInt32(8, itr->second->GetHealingDone());
                    stmt->setUInt32(9, itr->second->GetDamageTaken());
                    stmt->setUInt32(10, itr->second->GetHealingTaken());
                    stmt->setUInt32(11, itr->second->GetKillingBlows());
                    stmt->setUInt32(12, m_MapId);
                    stmt->setUInt16(13, m_StartTime / IN_MILLISECONDS);

                    armoryArenaTransaction->Append(stmt);
                }
                CharacterDatabase.CommitTransaction(armoryArenaTransaction);
                /** World of Warcraft Armory **/

                if (sWorld->getBoolConfig(CONFIG_ARENA_LOG_EXTENDED_INFO))
                    for (Battleground::BattlegroundScoreMap::const_iterator itr = GetPlayerScoresBegin(); itr != GetPlayerScoresEnd(); ++itr)
                        if (Player* player = ObjectAccessor::FindConnectedPlayer(itr->first))
                        {
                            TC_LOG_DEBUG("bg.arena", "Statistics match Type: %u for %s (GUID: %s, Team: %d, IP: %s): %u damage, %u healing, %u killing blows",
                                m_ArenaType, player->GetName().c_str(), itr->first.ToString().c_str(), player->GetArenaTeamId(m_ArenaType == 5 ? 2 : m_ArenaType == 3),
                                player->GetSession()->GetRemoteAddress().c_str(), itr->second->DamageDone, itr->second->HealingDone,
                                itr->second->KillingBlows);
                        }
            }
            // Deduct 16 points from each teams arena-rating if there are no winners after 45+2 minutes
            else
            {
                SetArenaTeamRatingChangeForTeam(ALLIANCE, ARENA_TIMELIMIT_POINTS_LOSS);
                SetArenaTeamRatingChangeForTeam(HORDE, ARENA_TIMELIMIT_POINTS_LOSS);
                winnerArenaTeam->FinishGame(ARENA_TIMELIMIT_POINTS_LOSS);
                loserArenaTeam->FinishGame(ARENA_TIMELIMIT_POINTS_LOSS);
            }
        }
        else
        {
            SetArenaTeamRatingChangeForTeam(ALLIANCE, 0);
            SetArenaTeamRatingChangeForTeam(HORDE, 0);
        }
        }
    }

    WorldPacket pvpLogData = sBattlegroundMgr->BuildPvpLogDataPacket(this);

    if (isArena())
    {
        if (auto replay = GetBgMap()->GetReplayData())
        {
            replay->AddPacket(pvpLogData);
            replay->EndRecord();
            sArenaReplayMgr->AddCacheReplayData(replay);

            for (BattlegroundPlayerMap::iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
                if (Player* player = _GetPlayer(itr, "EndBattleground"))
                    ChatHandler(player->GetSession()).PSendSysMessage(LANG_ARENA_REPLAY_SAVED, replay->GetId());
        }
        ArenaTeam* team1 = NULL;
        ArenaTeam* team2 = NULL;
        if (isRated())
        {
            team1 = sArenaTeamMgr->GetArenaTeamById(GetArenaTeamIdForTeam(ALLIANCE));
            team2 = sArenaTeamMgr->GetArenaTeamById(GetArenaTeamIdForTeam(HORDE));
        }
        sArenaReplayMgr->BattlegroundEnded(this, team1, team2);
        GetBgMap()->SetReplayData(nullptr);
        RemoveAuraOnTeam(HEAL_REDUCE_SPELL, TEAM_ALLIANCE);
        RemoveAuraOnTeam(HEAL_REDUCE_SPELL, TEAM_HORDE);
    }

    BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(GetTypeID(), GetArenaType());

    uint8 aliveWinners = GetAlivePlayersCountByTeam(winner);
    for (BattlegroundPlayerMap::iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        uint32 team = itr->second.Team;

        if (itr->second.OfflineRemoveTime)
        {
            //if rated arena match - make member lost!
            if (isArena() && isRated() && winnerArenaTeam && loserArenaTeam && winnerArenaTeam != loserArenaTeam)
            {
                if (team == winner)
                    winnerArenaTeam->OfflineMemberLost(itr->first, loserMatchmakerRating, winnerMatchmakerChange);
                else
                    loserArenaTeam->OfflineMemberLost(itr->first, winnerMatchmakerRating, loserMatchmakerChange);
            }
            continue;
        }

        Player* player = _GetPlayer(itr, "EndBattleground");
        if (!player)
            continue;

        // should remove spirit of redemption
        if (player->HasAuraType(SPELL_AURA_SPIRIT_OF_REDEMPTION))
            player->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);

        // Last standing - Rated 5v5 arena & be solely alive player
        if (team == winner && isArena() && isRated() && GetArenaType() == ARENA_TYPE_5v5 && aliveWinners == 1 && player->IsAlive())
            player->CastSpell(player, SPELL_THE_LAST_STANDING, true);

        if (!player->IsAlive())
        {
            player->ResurrectPlayer(1.0f);
            player->SpawnCorpseBones();
        }
        else
        {
            //needed cause else in av some creatures will kill the players at the end
            player->CombatStop();
            player->getHostileRefManager().deleteReferences();
        }

        // per player calculation
        if (isArena() && isRated() && winnerArenaTeam && loserArenaTeam && winnerArenaTeam != loserArenaTeam)
        {
            if (team == winner)
            {
                // update achievement BEFORE personal rating update
                uint32 rating = player->GetArenaPersonalRating(winnerArenaTeam->GetSlot());
                player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA, rating ? rating : 1);
                player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA, GetMapId());

                winnerArenaTeam->MemberWon(player, loserMatchmakerRating, winnerMatchmakerChange);
            }
            else
            {
                loserArenaTeam->MemberLost(player, winnerMatchmakerRating, loserMatchmakerChange);

                // Arena lost => reset the win_rated_arena having the "no_lose" condition
                player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA, ACHIEVEMENT_CRITERIA_CONDITION_NO_LOSE);
            }
        }

        uint32 winner_kills = player->GetRandomWinner() ? sWorld->getIntConfig(CONFIG_BG_REWARD_WINNER_HONOR_LAST) : sWorld->getIntConfig(CONFIG_BG_REWARD_WINNER_HONOR_FIRST);
        uint32 loser_kills = player->GetRandomWinner() ? sWorld->getIntConfig(CONFIG_BG_REWARD_LOSER_HONOR_LAST) : sWorld->getIntConfig(CONFIG_BG_REWARD_LOSER_HONOR_FIRST);
        uint32 winner_arena = player->GetRandomWinner() ? sWorld->getIntConfig(CONFIG_BG_REWARD_WINNER_ARENA_LAST) : sWorld->getIntConfig(CONFIG_BG_REWARD_WINNER_ARENA_FIRST);

        if (isBattleground() && sWorld->getBoolConfig(CONFIG_BATTLEGROUND_STORE_STATISTICS_ENABLE))
        {
            BattlegroundScoreMap::const_iterator score = PlayerScores.find(player->GetGUID());
            if (score == PlayerScores.end())
                continue;
            // battleground_id, character_guid, score_killing_blows, score_deaths, score_honorable_kills, score_bonus_honor, score_damage_done, score_healing_done

            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PVPSTATS_PLAYER);
            stmt->setUInt32(0, pvpStatsId);
            stmt->setUInt32(1, player->GetGUID().GetCounter());
            stmt->setUInt32(2, score->second->GetKillingBlows());
            stmt->setUInt32(3, score->second->GetDeaths());
            stmt->setUInt32(4, score->second->GetHonorableKills());
            stmt->setUInt32(5, score->second->GetBonusHonor());
            stmt->setUInt32(6, score->second->GetDamageDone());
            stmt->setUInt32(7, score->second->GetHealingDone());
            stmt->setUInt32(8, score->second->GetAttr1());
            stmt->setUInt32(9, score->second->GetAttr2());
            stmt->setUInt32(10, score->second->GetAttr3());
            stmt->setUInt32(11, score->second->GetAttr4());
            stmt->setUInt32(12, score->second->GetAttr5());
            pvpStatsTransaction->Append(stmt);
        }

        // Reward winner team
        if (team == winner)
        {
            if (IsRandom() || BattlegroundMgr::IsBGWeekend(GetTypeID()))
            {
                UpdatePlayerScore(player, SCORE_BONUS_HONOR, GetBonusHonorFromKill(winner_kills));
                if (CanAwardArenaPoints())
                    player->ModifyArenaPoints(winner_arena);
                if (!player->GetRandomWinner())
                    player->SetRandomWinner(true);
            }

            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WIN_BG, 1);

            if (isBattleground() || isRated())
                player->pvpData.RewardContributionPoints(uint32(sWorld->getRate(isBattleground() ? RATE_PVP_CP_BG_WIN : RATE_PVP_CP_ARENA_WIN) * PvPData::rankCP[0]));
        }
        else
        {
            if (IsRandom() || BattlegroundMgr::IsBGWeekend(GetTypeID()))
                UpdatePlayerScore(player, SCORE_BONUS_HONOR, GetBonusHonorFromKill(loser_kills));

            if (isBattleground() || isRated())
                player->pvpData.RewardContributionPoints(uint32(sWorld->getRate(isBattleground() ? RATE_PVP_CP_BG_LOSE : RATE_PVP_CP_ARENA_LOSE) * PvPData::rankCP[0]));
        }

        player->ResetAllPowers();
        player->CombatStopWithPets(true);

        // Handle PvP-Quests - RG-Quest Costum Killcredit
        if (player && isArena() && isRated() && team == winner)
        {
            switch (GetArenaType())
            {
                case ARENA_TYPE_2v2:
                    player->KilledMonsterCredit(1010715); // win 2v2
                    break;
                case ARENA_TYPE_3v3:
                    if (IsEventActive(EVENT_QUEST_3V3))
                        player->KilledMonsterCredit(1010717); // win 3v3
                    break;
            }
        }

        BlockMovement(player);

        player->SendDirectMessage(pvpLogData);

        WorldPacket data = sBattlegroundMgr->BuildBattlegroundStatusPacket(this, player->GetBattlegroundQueueIndex(bgQueueTypeId), STATUS_IN_PROGRESS, TIME_TO_AUTOREMOVE, GetStartTime(), GetArenaType(), player->GetTeam());
        player->SendDirectMessage(std::move(data));
        player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND, 1);
    }

    if (isArena() && isRated() && winnerArenaTeam && loserArenaTeam && winnerArenaTeam != loserArenaTeam)
    {
        // save the stat changes
        winnerArenaTeam->SaveToDB();
        loserArenaTeam->SaveToDB();
        // send updated arena team stats to players
        // this way all arena team members will get notified, not only the ones who participated in this match
        winnerArenaTeam->NotifyStatsChanged();
        loserArenaTeam->NotifyStatsChanged();
    }

    if (winmsg_id)
        SendMessageToAll(winmsg_id, CHAT_MSG_BG_SYSTEM_NEUTRAL);

    if (isBattleground() && sWorld->getBoolConfig(CONFIG_BATTLEGROUND_STORE_STATISTICS_ENABLE))
        CharacterDatabase.CommitTransaction(pvpStatsTransaction);

    EndRatingUpdate();
}

uint32 Battleground::GetBonusHonorFromKill(uint32 kills) const
{
    //variable kills means how many honorable kills you scored (so we need kills * honor_for_one_kill)
    uint32 maxLevel = std::min<uint32>(GetMaxLevel(), 80U);
    return Trinity::Honor::hk_honor_at_level(maxLevel, float(kills));
}

void Battleground::BlockMovement(Player* player)
{
    player->SetClientControl(player, 0);                          // movement disabled NOTE: the effect will be automatically removed by client when the player is teleported from the battleground, so no need to send with uint8(1) in RemovePlayerAtLeave()
}

void Battleground::RemovePlayerAtLeave(ObjectGuid guid, bool Transport, bool SendPacket)
{
    Player* player = ObjectAccessor::GetPlayer(*m_Map, guid);
    uint32 team = GetPlayerTeam(guid);

    RemovePlayer(player, guid, team);                           // BG subclass specific code

    if (player && isArena() && isRated())
    {
        switch (GetArenaType())
        {
            case ARENA_TYPE_2v2:
                player->KilledMonsterCredit(1010705); // pass 2v2
                break;
            case ARENA_TYPE_3v3:
                if (IsEventActive(EVENT_QUEST_3V3))
                    player->KilledMonsterCredit(1010716); // pass 3v3
                break;
        }
    }

    bool participant = false;
    // Remove from lists/maps
    BattlegroundPlayerMap::iterator itr = m_Players.find(guid);
    if (itr != m_Players.end())
    {
        UpdatePlayersCountByTeam(team, true);               // -1 player
        m_Players.erase(itr);
        // check if the player was a participant of the match, or only entered through gm command (goname)
        participant = true;
    }

    BattlegroundScoreMap::iterator itr2 = PlayerScores.find(guid);
    if (itr2 != PlayerScores.end())
    {
        delete itr2->second;                                // delete player's score
        PlayerScores.erase(itr2);
    }

    RemovePlayerFromResurrectQueue(guid);

    // should remove spirit of redemption
    if (player)
    {
        if (player->HasAuraType(SPELL_AURA_SPIRIT_OF_REDEMPTION))
            player->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);

        if (!player->IsAlive())                              // resurrect on exit
        {
            player->ResurrectPlayer(1.0f);
            player->SpawnCorpseBones();
        }

        player->SetCommandStatusOff(CHEAT_CASTTIME);
    }
    else // try to resurrect the offline player. If he is alive nothing will happen
        sObjectAccessor->ConvertCorpseForPlayer(guid);

    if (participant) // if the player was a match participant, remove auras, calc rating, update queue
    {
        BattlegroundTypeId bgTypeId = GetTypeID();
        BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(GetTypeID(), GetArenaType());
        if (player)
        {
            player->ClearAfkReports();

            if (!team) team = player->GetTeam();

            // if arena, remove the specific arena auras
            if (isArena())
            {
                bgTypeId=BATTLEGROUND_AA;                   // set the bg type to all arenas (it will be used for queue refreshing)

                // unsummon current and summon old pet if there was one and there isn't a current pet
                player->RemovePet(NULL, PET_SAVE_NOT_IN_SLOT);
                player->ResummonPetTemporaryUnSummonedIfAny();

                if (isRated() && GetStatus() == STATUS_IN_PROGRESS)
                {
                    //left a rated match while the encounter was in progress, consider as loser
                    ArenaTeam* winnerArenaTeam = sArenaTeamMgr->GetArenaTeamById(GetArenaTeamIdForTeam(GetOtherTeam(team)));
                    ArenaTeam* loserArenaTeam = sArenaTeamMgr->GetArenaTeamById(GetArenaTeamIdForTeam(team));
                    if (winnerArenaTeam && loserArenaTeam && winnerArenaTeam != loserArenaTeam)
                        loserArenaTeam->MemberLost(player, GetArenaMatchmakerRating(GetOtherTeam(team)));
                }
            }
            if (SendPacket)
            {
                WorldPacket data = sBattlegroundMgr->BuildBattlegroundStatusPacket(this, player->GetBattlegroundQueueIndex(bgQueueTypeId), STATUS_NONE, 0, 0, 0, 0);
                player->SendDirectMessage(std::move(data));
            }

            // this call is important, because player, when joins to battleground, this method is not called, so it must be called when leaving bg
            player->RemoveBattlegroundQueueId(bgQueueTypeId);
        }
        else
        // removing offline participant
        {
            if (isRated() && GetStatus() == STATUS_IN_PROGRESS)
            {
                //left a rated match while the encounter was in progress, consider as loser
                ArenaTeam* others_arena_team = sArenaTeamMgr->GetArenaTeamById(GetArenaTeamIdForTeam(GetOtherTeam(team)));
                ArenaTeam* players_arena_team = sArenaTeamMgr->GetArenaTeamById(GetArenaTeamIdForTeam(team));
                if (others_arena_team && players_arena_team)
                    players_arena_team->OfflineMemberLost(guid, GetArenaMatchmakerRating(GetOtherTeam(team)));
            }
        }

        // remove from raid group if player is member
        if (Group* group = GetBgRaid(team))
        {
            if (!group->RemoveMember(guid))                // group was disbanded
            {
                SetBgRaid(team, NULL);
            }
        }
        DecreaseInvitedCount(team);
        GetMatchmakingData().RemovePlayerData(guid);
        //we should update battleground queue, but only if bg isn't ending
        if (isBattleground() && GetStatus() < STATUS_WAIT_LEAVE)
        {
            // a player has left the battleground, so there are free slots -> add to queue
            AddToBGFreeSlotQueue();
            sBattlegroundMgr->ScheduleQueueUpdate(0, 0, bgQueueTypeId, bgTypeId, GetBracketId());
        }
        // Let others know

        WorldPacket data = sBattlegroundMgr->BuildPlayerLeftBattlegroundPacket(guid);
        SendPacketToTeam(team, data, player, false);
    }

    if (player)
    {
        player->FitPlayerInTeam(false, this);
        // Do next only if found in battleground
        player->SetBattlegroundId(0, BATTLEGROUND_TYPE_NONE);  // We're not in BG.
        // reset destination bg team
        player->SetBGTeam(0);

        if (Transport)
            player->TeleportToBGEntryPoint();

        TC_LOG_DEBUG("bg.battleground", "Removed player %s from Battleground.", player->GetName().c_str());
    }

    RemovePlayer(player, guid, team);
    //battleground object will be deleted next Battleground::Update() call
}

void Battleground::RemovePlayer(Player* player, ObjectGuid /*guid*/, uint32 /*team*/)
{
    if (!player)
        return;

    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WIN_BG, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
}

// this method is called when no players remains in battleground
void Battleground::Reset()
{
    SetWinner(WINNER_NONE);
    SetStatus(STATUS_WAIT_QUEUE);
    SetStartTime(0);
    SetEndTime(0);

    m_LastResurrectTime.clear();
    m_Events = 0;

    if (m_InvitedAlliance > 0 || m_InvitedHorde > 0)
        TC_LOG_ERROR("bg.battleground", "Battleground::Reset: one of the counters is not 0 (alliance: %u, horde: %u) for BG (map: %u, instance id: %u)!",
            m_InvitedAlliance, m_InvitedHorde, m_MapId, m_InstanceID);

    m_InvitedAlliance = 0;
    m_InvitedHorde = 0;
    m_InBGFreeSlotQueue = false;

    m_Players.clear();

    for (BattlegroundScoreMap::const_iterator itr = PlayerScores.begin(); itr != PlayerScores.end(); ++itr)
        delete itr->second;
    PlayerScores.clear();

    ResetBGSubclass();
}

void Battleground::StartBattleground()
{
    SetStartTime(0);
    // add BG to free slot queue
    AddToBGFreeSlotQueue();

    // add bg to update list
    // This must be done here, because we need to have already invited some players when first BG::Update() method is executed
    // and it doesn't matter if we call StartBattleground() more times, because m_Battlegrounds is a map and instance id never changes
    sBattlegroundMgr->AddBattleground(this);

    if (m_IsRated)
        TC_LOG_DEBUG("bg.arena", "Arena match type: %u for Team1Id: %u - Team2Id: %u started.", m_ArenaType, m_ArenaTeamIds[TEAM_ALLIANCE], m_ArenaTeamIds[TEAM_HORDE]);
    if (isBattleground())
        TC_LOG_INFO("pvp.bg.queue", "Starting battleground '%s' (%u) (%u: %u-%u) with %u alliance and %u horde players (invited)",
                    GetName(), GetTypeID(), GetBracketId(), GetMinLevel(), GetMaxLevel(), GetInvitedCount(ALLIANCE), GetInvitedCount(HORDE));
}

void Battleground::AddPlayer(Player* player)
{
    // remove afk from player
    if (player->isAFK())
        player->ToggleAFK();

    // score struct must be created in inherited class

    uint32 team = player->GetTeam();

    BattlegroundPlayer bp;
    bp.OfflineRemoveTime = 0;
    bp.Team = team;

    // Add to list/maps
    m_Players[player->GetGUID()] = bp;

    UpdatePlayersCountByTeam(team, false);                  // +1 player

    const WorldPacket data = sBattlegroundMgr->BuildPlayerJoinedBattlegroundPacket(player);
    SendPacketToTeam(team, data, player, false);

    player->RemoveAurasByType(SPELL_AURA_MOUNTED);

    // RG CUSTOM - use lfg and random bg at the same time
    sLFGMgr->LeaveLfg(player->GetGUID());

    // add arena specific auras
    if (isArena())
    {
        if (auto replay = GetBgMap()->GetReplayData())
            replay->OnPlayerAdded(player, player->GetTeamId());

        player->RemoveArenaEnchantments(TEMP_ENCHANTMENT_SLOT);
        if (team == ALLIANCE)                                // gold
        {
            if (player->GetTeam() == HORDE)
                player->CastSpell(player, SPELL_HORDE_GOLD_FLAG, true);
            else
                player->CastSpell(player, SPELL_ALLIANCE_GOLD_FLAG, true);
        }
        else                                                // green
        {
            if (player->GetTeam() == HORDE)
                player->CastSpell(player, SPELL_HORDE_GREEN_FLAG, true);
            else
                player->CastSpell(player, SPELL_ALLIANCE_GREEN_FLAG, true);
        }

        player->DestroyConjuredItems(true);
        player->UnsummonPetTemporaryIfAny();

        if (GetStatus() == STATUS_WAIT_JOIN)                 // not started yet
        {
            player->CastSpell(player, SPELL_ARENA_PREPARATION, true);
            player->ResetAllPowers();
            player->SetCommandStatusOn(CHEAT_CASTTIME);

            if(player->getClass() == CLASS_WARLOCK)
               player->CastSpell(player, COSTUM_SOULWELL, false);
            
            if(player->getClass() == CLASS_MAGE)
            player->CastSpell(player, COSTUM_TABLE, false);
        }
    }
    else
    {
        if (GetStatus() == STATUS_WAIT_JOIN)                 // not started yet
            player->CastSpell(player, SPELL_PREPARATION, true);   // reduces all mana cost of spells.
    }

    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WIN_BG, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);
    player->ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL, ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP, GetMapId(), true);

    // setup BG group membership
    PlayerAddedToBGCheckIfBGIsRunning(player);
    AddOrSetPlayerToCorrectBgGroup(player, team);

    player->FitPlayerInTeam(true, this);
}

// this method adds player to his team's bg group, or sets his correct group if player is already in bg group
void Battleground::AddOrSetPlayerToCorrectBgGroup(Player* player, uint32 team)
{
    ObjectGuid playerGuid = player->GetGUID();
    Group* group = GetBgRaid(team);
    if (!group)                                      // first player joined
    {
        group = new Group;
        SetBgRaid(team, group);
        group->Create(player);
    }
    else                                            // raid already exist
    {
        if (group->IsMember(playerGuid))
        {
            uint8 subgroup = group->GetMemberGroup(playerGuid);
            player->SetBattlegroundOrBattlefieldRaid(group, subgroup);
        }
        else
        {
            group->AddMember(player);
            if (Group* originalGroup = player->GetOriginalGroup())
                if (originalGroup->IsLeader(playerGuid))
                {
                    group->ChangeLeader(playerGuid);
                    group->SendUpdate();
                }
        }
    }
}

// This method should be called when player logs into running battleground
void Battleground::EventPlayerLoggedIn(Player* player)
{
    ObjectGuid guid = player->GetGUID();
    // player is correct pointer
    for (GuidDeque::iterator itr = m_OfflineQueue.begin(); itr != m_OfflineQueue.end(); ++itr)
    {
        if (*itr == guid)
        {
            m_OfflineQueue.erase(itr);
            break;
        }
    }
    m_Players[guid].OfflineRemoveTime = 0;
    PlayerAddedToBGCheckIfBGIsRunning(player);
    // if battleground is starting, then add preparation aura
    // we don't have to do that, because preparation aura isn't removed when player logs out
}

// This method should be called when player logs out from running battleground
void Battleground::EventPlayerLoggedOut(Player* player)
{
    ObjectGuid guid = player->GetGUID();
    if (!IsPlayerInBattleground(guid))  // Check if this player really is in battleground (might be a GM who teleported inside)
        return;

    // player is correct pointer, it is checked in WorldSession::LogoutPlayer()
    m_OfflineQueue.push_back(player->GetGUID());
    if (isArena())
        m_Players[guid].OfflineRemoveTime = game::time::GetGameTime() + MAX_OFFLINE_TIME_IN_ARENA;
    else
        m_Players[guid].OfflineRemoveTime = game::time::GetGameTime() + MAX_OFFLINE_TIME_IN_BG;
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        // drop flag and handle other cleanups
        RemovePlayer(player, guid, GetPlayerTeam(guid));

        // 1 player is logging out, if it is the last, then end arena!
        if (isArena())
            if (GetAlivePlayersCountByTeam(player->GetTeam()) <= 1 && GetPlayersCountByTeam(GetOtherTeam(player->GetTeam())))
                EndBattleground(GetOtherTeam(player->GetTeam()));
    }
}

// This method should be called if player clicks on ready-marker
void Battleground::EventPlayerUsedReadyMarker(Player* player)
{
    // make sure arena has not started yet and is not starting soon
    if (!isArena() || GetStatus() != STATUS_WAIT_JOIN || m_Events & BG_STARTING_EVENT_3 || m_Events & BG_STARTING_EVENT_4)
        return;

    // make sure every player can only use the ready-marker once
    if (m_ReadyMarkerList.find(player->GetGUID()) != m_ReadyMarkerList.end())
        return;

    // save player guids in a set
    m_ReadyMarkerList.insert(player->GetGUID());

    // if all player in the arena used the ready marker, start arena soon
    if (m_ReadyMarkerList.size() == (sBattlegroundMgr->isArenaTesting() ? 1 : GetMaxPlayers()))
    {
        SetStartDelayTime(BG_START_DELAY_15S);
        m_Events = BG_STARTING_EVENT_1 | BG_STARTING_EVENT_2 | BG_STARTING_EVENT_3;
        SendMessageToAll(StartMessageIds[BG_STARTING_EVENT_THIRD], CHAT_MSG_BG_SYSTEM_NEUTRAL);
    }    
}

// This method should be called only once ... it adds pointer to queue
void Battleground::AddToBGFreeSlotQueue()
{
    if (!m_InBGFreeSlotQueue && isBattleground())
    {
        sBattlegroundMgr->AddToBGFreeSlotQueue(m_TypeID, this);
        m_InBGFreeSlotQueue = true;
    }
}

// This method removes this battleground from free queue - it must be called when deleting battleground
void Battleground::RemoveFromBGFreeSlotQueue()
{
    if (m_InBGFreeSlotQueue)
    {
        sBattlegroundMgr->RemoveFromBGFreeSlotQueue(m_TypeID, m_InstanceID);
        m_InBGFreeSlotQueue = false;
    }
}

// get the number of free slots for team
// returns the number how many players can join battleground to MaxPlayersPerTeam
uint32 Battleground::GetFreeSlotsForTeam(uint32 Team) const
{
    // if BG is starting ... invite anyone
    if (GetStatus() == STATUS_WAIT_JOIN)
        return (GetInvitedCount(Team) < GetMaxPlayersPerTeam()) ? GetMaxPlayersPerTeam() - GetInvitedCount(Team) : 0;
    // if BG is already started .. do not allow to join too much players of one faction
    uint32 otherTeam;
    uint32 otherIn;
    if (Team == ALLIANCE)
    {
        otherTeam = GetInvitedCount(HORDE);
        otherIn = GetPlayersCountByTeam(HORDE);
    }
    else
    {
        otherTeam = GetInvitedCount(ALLIANCE);
        otherIn = GetPlayersCountByTeam(ALLIANCE);
    }
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        // difference based on ppl invited (not necessarily entered battle)
        // default: allow 0
        uint32 diff = 0;
        // allow join one person if the sides are equal (to fill up bg to minplayersperteam)
        if (otherTeam == GetInvitedCount(Team))
            diff = 1;
        // allow join more ppl if the other side has more players
        else if (otherTeam > GetInvitedCount(Team))
            diff = otherTeam - GetInvitedCount(Team);

        // difference based on max players per team (don't allow inviting more)
        uint32 diff2 = (GetInvitedCount(Team) < GetMaxPlayersPerTeam()) ? GetMaxPlayersPerTeam() - GetInvitedCount(Team) : 0;
        // difference based on players who already entered
        // default: allow 0
        uint32 diff3 = 0;
        // allow join one person if the sides are equal (to fill up bg minplayersperteam)
        if (otherIn == GetPlayersCountByTeam(Team))
            diff3 = 1;
        // allow join more ppl if the other side has more players
        else if (otherIn > GetPlayersCountByTeam(Team))
            diff3 = otherIn - GetPlayersCountByTeam(Team);
        // or other side has less than minPlayersPerTeam
        else if (GetInvitedCount(Team) <= GetMinPlayersPerTeam())
            diff3 = GetMinPlayersPerTeam() - GetInvitedCount(Team) + 1;

        // return the minimum of the 3 differences

        // min of diff and diff 2
        diff = std::min(diff, diff2);
        // min of diff, diff2 and diff3
        return std::min(diff, diff3);
    }
    return 0;
}

bool Battleground::HasFreeSlots() const
{
    return GetPlayersSize() < GetMaxPlayers();
}

void Battleground::UpdatePlayerScore(Player* Source, uint32 type, uint32 value, bool doAddHonor)
{
    //this procedure is called from virtual function implemented in bg subclass
    BattlegroundScoreMap::const_iterator itr = PlayerScores.find(Source->GetGUID());
    if (itr == PlayerScores.end())                         // player not found...
        return;

    switch (type)
    {
        case SCORE_KILLING_BLOWS:                           // Killing blows
            itr->second->KillingBlows += value;
            break;
        case SCORE_DEATHS:                                  // Deaths
            itr->second->Deaths += value;
            break;
        case SCORE_HONORABLE_KILLS:                         // Honorable kills
            itr->second->HonorableKills += value;
            break;
        case SCORE_BONUS_HONOR:                             // Honor bonus
            // do not add honor in arenas
            if (isBattleground())
            {
                // reward honor instantly
                if (doAddHonor)
                    Source->RewardHonor(NULL, 1, value);    // RewardHonor calls UpdatePlayerScore with doAddHonor = false
                else
                    itr->second->BonusHonor += value;
            }
            break;
            // used only in EY, but in MSG_PVP_LOG_DATA opcode
        case SCORE_DAMAGE_DONE:                             // Damage Done
            itr->second->DamageDone += value;
            break;
        case SCORE_HEALING_DONE:                            // Healing Done
            itr->second->HealingDone += value;
            break;
         /** World of Warcraft Armory **/
        case SCORE_DAMAGE_TAKEN:
            itr->second->DamageTaken += value;              // Damage Taken
            break;
        case SCORE_HEALING_TAKEN:
            itr->second->HealingTaken += value;             // Healing Taken
            break;
        /** World of Warcraft Armory **/
        default:
            TC_LOG_ERROR("bg.battleground", "Battleground::UpdatePlayerScore: unknown score type (%u) for BG (map: %u, instance id: %u)!",
                type, m_MapId, m_InstanceID);
            break;
    }
}

uint32 Battleground::GetPlayerScore(Player* Source, ScoreType type)
{
    BattlegroundScoreMap::const_iterator itr = PlayerScores.find(Source->GetGUID());
    if (itr == PlayerScores.end())
        return 0;

    switch (type)
    {
        case SCORE_KILLING_BLOWS:                           // Killing blows
            return itr->second->KillingBlows;
        case SCORE_DEATHS:                                  // Deaths
            return itr->second->Deaths;
        case SCORE_HONORABLE_KILLS:                         // Honorable kills
            return itr->second->HonorableKills;
        case SCORE_DAMAGE_DONE:                             // Damage Done
            return itr->second->DamageDone;
        case SCORE_HEALING_DONE:                            // Healing Done
            return itr->second->HealingDone;
        case SCORE_DAMAGE_TAKEN:
            return itr->second->DamageTaken;                // Damage Taken
        case SCORE_HEALING_TAKEN:
            return itr->second->HealingTaken;               // Healing Taken
        default:
            return 0;
    }
}

void Battleground::AddPlayerToResurrectQueue(ObjectGuid player_guid)
{
    Player* player = ObjectAccessor::GetPlayer(*m_Map, player_guid);
    if (!player)
        return;

    player->CastSpell(player, SPELL_WAITING_FOR_RESURRECT, true);
}

void Battleground::RemovePlayerFromResurrectQueue(ObjectGuid player_guid)
{
    if (Player* player = ObjectAccessor::GetPlayer(*m_Map, player_guid))
        player->RemoveAurasDueToSpell(SPELL_WAITING_FOR_RESURRECT);
}

void Battleground::SendAreaSpiritHealerQueryOpcode(Player* player, ObjectGuid guid)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    WorldPacket data(SMSG_AREA_SPIRIT_HEALER_TIME, 12);
    auto itr = m_LastResurrectTime.find(guid);
    if (itr != m_LastResurrectTime.end())
    {
        uint32 time = itr->second;
        if (time >= RESURRECTION_INTERVAL)
            time = 0;
        else
            time = RESURRECTION_INTERVAL - time;
        data << guid << time;
        player->SendDirectMessage(std::move(data));
    }
}

void Battleground::RelocateDeadPlayers(ObjectGuid guideGuid)
{
    if (Unit* spiritGuide = ObjectAccessor::GetUnit(*GetBgMap(), guideGuid))
    {
        TeamId teamId = spiritGuide->GetEntry() == BG_CREATURE_ENTRY_A_SPIRITGUIDE ? TEAM_ALLIANCE : TEAM_HORDE;
        WorldSafeLocsEntry const* closestGrave = NULL;
        for (auto itr = GetPlayers().cbegin(); itr != GetPlayers().cend(); ++itr)
        {
            if (Player* player = ObjectAccessor::GetPlayer(*m_Map, itr->first))
            {
                if (player->GetTeamId() == teamId && player->HasAuraType(SPELL_AURA_GHOST))
                {
                    if (player->IsBeingTeleported())
                    {
                        if (spiritGuide->GetDistance(player->GetTeleportDest()) > 20)
                            continue;
                    }
                    else if (!player->IsInRange(spiritGuide, 0, 20))
                        continue;

                    if (!closestGrave)
                        closestGrave = GetClosestGraveYard(player);

                    if (closestGrave)
                        player->TeleportTo(GetMapId(), closestGrave->x, closestGrave->y, closestGrave->z, player->GetOrientation());
                }
            }
        }
    }
}

bool Battleground::AddObject(uint32 type, uint32 entry, float x, float y, float z, float o, float rotation0, float rotation1, float rotation2, float rotation3, uint32 /*respawnTime*/)
{
    // If the assert is called, means that BgObjects must be resized!
    ASSERT(type < BgObjects.size());

    Map* map = FindBgMap();
    if (!map)
        return false;

    G3D::Quat rot(rotation0, rotation1, rotation2, rotation3);
    // Temporally add safety check for bad spawns and send log (object rotations need to be rechecked in sniff)
    if (!rotation0 && !rotation1 && !rotation2 && !rotation3)
    {
        TC_LOG_DEBUG("bg.battleground", "Battleground::AddObject: gameoobject [entry: %u, object type: %u] for BG (map: %u) has zeroed rotation fields, "
            "orientation used temporally, but please fix the spawn", entry, type, m_MapId);

        rot = G3D::Matrix3::fromEulerAnglesZYX(o, 0.f, 0.f);
    }

    // Must be created this way, adding to godatamap would add it to the base map of the instance
    // and when loading it (in go::LoadFromDB()), a new guid would be assigned to the object, and a new object would be created
    // So we must create it specific for this instance
    GameObject* go = GameObject::Create(entry, *map, PHASEMASK_NORMAL, Position(x, y, z, o), rot, 100);
    if (!go)
    {
        TC_LOG_ERROR("bg.battleground", "Battleground::AddObject: cannot create gameobject (entry: %u) for BG (map: %u, instance id: %u)!",
                entry, m_MapId, m_InstanceID);
        return false;
    }
/*
    uint32 guid = go->GetGUID().GetCounter();

    // without this, UseButtonOrDoor caused the crash, since it tried to get go info from godata
    // iirc that was changed, so adding to go data map is no longer required if that was the only function using godata from GameObject without checking if it existed
    GameObjectData& data = sObjectMgr->NewGOData(guid);

    data.id             = entry;
    data.mapid          = GetMapId();
    data.posX           = x;
    data.posY           = y;
    data.posZ           = z;
    data.orientation    = o;
    data.rotation0      = rotation0;
    data.rotation1      = rotation1;
    data.rotation2      = rotation2;
    data.rotation3      = rotation3;
    data.spawntimesecs  = respawnTime;
    data.spawnMask      = 1;
    data.animprogress   = 100;
    data.go_state       = 1;
*/
    // Add to world, so it can be later looked up from HashMapHolder
    if (!map->AddToMap(go))
    {
        delete go;
        return false;
    }
    BgObjects[type] = go->GetGUID();
    return true;
}

bool Battleground::AddObject(uint32 type, uint32 entry, Position const& pos, float rotation0, float rotation1, float rotation2, float rotation3, uint32 respawnTime /*= 0*/)
{
    return AddObject(type, entry, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), rotation0, rotation1, rotation2, rotation3, respawnTime);
}

// Some doors aren't despawned so we cannot handle their closing in gameobject::update()
// It would be nice to correctly implement GO_ACTIVATED state and open/close doors in gameobject code
void Battleground::DoorClose(uint32 type)
{
    if (GameObject* obj = GetBgMap()->GetGameObject(BgObjects[type]))
    {
        // If doors are open, close it
        if (obj->getLootState() == GO_ACTIVATED && obj->GetGoState() != GO_STATE_READY)
        {
            obj->SetLootState(GO_READY);
            obj->SetGoState(GO_STATE_READY);
        }
    }
    else
        TC_LOG_ERROR("bg.battleground", "Battleground::DoorClose: door gameobject (type: %u, GUID: %u) not found for BG (map: %u, instance id: %u)!",
            type, BgObjects[type].GetCounter(), m_MapId, m_InstanceID);
}

void Battleground::DoorOpen(uint32 type)
{
    if (GameObject* obj = GetBgMap()->GetGameObject(BgObjects[type]))
    {
        obj->SetLootState(GO_ACTIVATED);
        obj->SetGoState(GO_STATE_ACTIVE);
    }
    else
        TC_LOG_ERROR("bg.battleground", "Battleground::DoorOpen: door gameobject (type: %u, GUID: %u) not found for BG (map: %u, instance id: %u)!",
            type, BgObjects[type].GetCounter(), m_MapId, m_InstanceID);
}

void Battleground::DoorDelete(uint32 type)
{
    if (GameObject* obj = GetBgMap()->GetGameObject(BgObjects[type]))
    {
        obj->RemoveFromWorld();
    }
    else
        TC_LOG_ERROR("bg.battleground", "Battleground::DoorDelete: door gameobject (type: %u, GUID: %u) not found for BG (map: %u, instance id: %u)!",
        type, BgObjects[type].GetCounter(), m_MapId, m_InstanceID);
}

GameObject* Battleground::GetBGObject(uint32 type, bool logError)
{
    GameObject* obj = GetBgMap()->GetGameObject(BgObjects[type]);
    if (!obj)
    {
        if (logError)
            TC_LOG_ERROR("bg.battleground", "Battleground::GetBGObject: gameobject (type: %u, GUID: %u) not found for BG (map: %u, instance id: %u)!",
                type, BgObjects[type].GetCounter(), m_MapId, m_InstanceID);
        else
            TC_LOG_INFO("bg.battleground", "Battleground::GetBGObject: gameobject (type: %u, GUID: %u) not found for BG (map: %u, instance id: %u)!",
                type, BgObjects[type].GetCounter(), m_MapId, m_InstanceID);
    }
    return obj;
}

Creature* Battleground::GetBGCreature(uint32 type, bool logError)
{
    Creature* creature = GetBgMap()->GetCreature(BgCreatures[type]);
    if (!creature)
    {
        if (logError)
            TC_LOG_ERROR("bg.battleground", "Battleground::GetBGCreature: creature (type: %u, GUID: %u) not found for BG (map: %u, instance id: %u)!",
                type, BgCreatures[type].GetCounter(), m_MapId, m_InstanceID);
        else
            TC_LOG_INFO("bg.battleground", "Battleground::GetBGCreature: creature (type: %u, GUID: %u) not found for BG (map: %u, instance id: %u)!",
                type, BgCreatures[type].GetCounter(), m_MapId, m_InstanceID);
    }
    return creature;
}

void Battleground::SetBgMap(BattlegroundMap* map)
{
    m_Map = map;
    if (sArenaReplayMgr->ShouldRecordReplay(this))
    {
        auto replay = sArenaReplayMgr->CreateNewRecord(GetMapId());
        GetBgMap()->SetReplayData(replay);
        
        const WorldPacket data = sBattlegroundMgr->BuildBattlegroundStatusPacket(this, 0, STATUS_IN_PROGRESS, GetEndTime(), GetStartTime(), GetArenaType(), TEAM_OTHER);
        replay->AddPacket(data);
    }
}

void Battleground::SpawnBGObject(uint32 type, uint32 respawntime)
{
    if (Map* map = FindBgMap())
        if (GameObject* obj = map->GetGameObject(BgObjects[type]))
        {
            if (respawntime)
                obj->SetLootState(GO_JUST_DEACTIVATED);
            else
                if (obj->getLootState() == GO_JUST_DEACTIVATED)
                    // Change state from GO_JUST_DEACTIVATED to GO_READY in case battleground is starting again
                    obj->SetLootState(GO_READY);
            obj->SetRespawnTime(respawntime);
            map->AddToMap(obj);
        }
}

Creature* Battleground::AddCreature(uint32 entry, uint32 type, float x, float y, float z, float o, TeamId /*teamId = TEAM_NEUTRAL*/, uint32 respawntime /*= 0*/)
{
    // If the assert is called, means that BgCreatures must be resized!
    ASSERT(type < BgCreatures.size());

    Map* map = FindBgMap();
    if (!map)
        return nullptr;

    Creature* creature = Creature::Create(entry, map, Position(x, y, z, o), PHASEMASK_NORMAL);
    if (!creature)
        return nullptr;

    creature->SetHomePosition(x, y, z, o);

    if (!map->AddToMap(creature))
    {
        delete creature;
        return nullptr;
    }

    BgCreatures[type] = creature->GetGUID();

    if (respawntime)
        creature->SetRespawnDelay(respawntime);

    return creature;
}

Creature* Battleground::AddCreature(uint32 entry, uint32 type, Position const& pos, TeamId teamId /*= TEAM_NEUTRAL*/, uint32 respawntime /*= 0*/)
{
    return AddCreature(entry, type, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), teamId, respawntime);
}

bool Battleground::DelCreature(uint32 type)
{
    if (!BgCreatures[type])
        return true;

    if (Creature* creature = GetBgMap()->GetCreature(BgCreatures[type]))
    {
        if (creature->IsSpiritService())
            m_LastResurrectTime.erase(BgCreatures[type]);
        creature->AddObjectToRemoveList();
        BgCreatures[type].Clear();
        return true;
    }

    TC_LOG_ERROR("bg.battleground", "Battleground::DelCreature: creature (type: %u, GUID: %u) not found for BG (map: %u, instance id: %u)!",
        type, BgCreatures[type].GetCounter(), m_MapId, m_InstanceID);
    BgCreatures[type].Clear();
    return false;
}

bool Battleground::DelObject(uint32 type)
{
    if (!BgObjects[type])
        return true;

    if (GameObject* obj = GetBgMap()->GetGameObject(BgObjects[type]))
    {
        obj->SetRespawnTime(0);                                 // not save respawn time
        obj->Delete();
        BgObjects[type].Clear();
        return true;
    }
    TC_LOG_ERROR("bg.battleground", "Battleground::DelObject: gameobject (type: %u, GUID: %u) not found for BG (map: %u, instance id: %u)!",
        type, BgObjects[type].GetCounter(), m_MapId, m_InstanceID);
    BgObjects[type].Clear();
    return false;
}

bool Battleground::AddSpiritGuide(uint32 type, float x, float y, float z, float o, TeamId teamId /*= TEAM_NEUTRAL*/)
{
    uint32 entry = (teamId == TEAM_ALLIANCE) ? BG_CREATURE_ENTRY_A_SPIRITGUIDE : BG_CREATURE_ENTRY_H_SPIRITGUIDE;

    if (Creature* creature = AddCreature(entry, type, x, y, z, o, teamId))
    {
        m_LastResurrectTime[creature->GetGUID()] = 0;
        creature->setDeathState(DEAD);
        // correct cast speed
        creature->SetFloatValue(UNIT_MOD_CAST_SPEED, 1.0f);
        return true;
    }
    TC_LOG_ERROR("bg.battleground", "Battleground::AddSpiritGuide: cannot create spirit guide (type: %u, entry: %u) for BG (map: %u, instance id: %u)!",
        type, entry, m_MapId, m_InstanceID);
    EndNow();
    return false;
}

bool Battleground::AddSpiritGuide(uint32 type, Position const& pos, TeamId teamId /*= TEAM_NEUTRAL*/)
{
    return AddSpiritGuide(type, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), teamId);
}

void Battleground::SendMessageToAll(int32 entry, ChatMsg type, Player const* source)
{
    if (!entry)
        return;

    Trinity::BattlegroundChatBuilder bg_builder(type, entry, source);
    Trinity::LocalizedPacketDo<Trinity::BattlegroundChatBuilder> bg_do(bg_builder);
    BroadcastWorker(bg_do);
}

void Battleground::PSendMessageToAll(int32 entry, ChatMsg type, Player const* source, ...)
{
    if (!entry)
        return;

    va_list ap;
    va_start(ap, source);

    Trinity::BattlegroundChatBuilder bg_builder(type, entry, source, &ap);
    Trinity::LocalizedPacketDo<Trinity::BattlegroundChatBuilder> bg_do(bg_builder);
    BroadcastWorker(bg_do);

    va_end(ap);
}

void Battleground::SendWarningToAll(int32 entry, ...)
{
    if (!entry)
        return;

    std::map<uint32, WorldPacket> localizedPackets;
    for (BattlegroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
        if (Player* player = _GetPlayer(itr, "SendWarningToAll"))
        {
            if (localizedPackets.find(player->GetSession()->GetSessionDbLocaleIndex()) == localizedPackets.end())
            {
                char const* format = sObjectMgr->GetTrinityString(entry, player->GetSession()->GetSessionDbLocaleIndex());

                char str[1024];
                va_list ap;
                va_start(ap, entry);
                vsnprintf(str, 1024, format, ap);
                va_end(ap);

                ChatHandler::BuildChatPacket(localizedPackets[player->GetSession()->GetSessionDbLocaleIndex()], CHAT_MSG_RAID_BOSS_EMOTE, LANG_UNIVERSAL, NULL, NULL, str);
            }

            player->SendDirectMessage(localizedPackets[player->GetSession()->GetSessionDbLocaleIndex()]);
        }
}

void Battleground::SendMessage2ToAll(int32 entry, ChatMsg type, Player const* source, int32 arg1, int32 arg2)
{
    Trinity::Battleground2ChatBuilder bg_builder(type, entry, source, arg1, arg2);
    Trinity::LocalizedPacketDo<Trinity::Battleground2ChatBuilder> bg_do(bg_builder);
    BroadcastWorker(bg_do);
}

void Battleground::EndNow()
{
    sArenaReplayMgr->BattlegroundEnded(this, NULL, NULL, false);
    RemoveFromBGFreeSlotQueue();
    SetStatus(STATUS_WAIT_LEAVE);
    SetEndTime(0);
}

// To be removed
char const* Battleground::GetTrinityString(int32 entry)
{
    // FIXME: now we have different DBC locales and need localized message for each target client
    return sObjectMgr->GetTrinityStringForDBCLocale(entry);
}

// IMPORTANT NOTICE:
// buffs aren't spawned/despawned when players captures anything
// buffs are in their positions when battleground starts
void Battleground::HandleTriggerBuff(ObjectGuid go_guid)
{
    GameObject* obj = GetBgMap()->GetGameObject(go_guid);
    if (!obj || obj->GetGoType() != GAMEOBJECT_TYPE_TRAP || !obj->isSpawned())
        return;

    // Change buff type, when buff is used:
    int32 index = BgObjects.size() - 1;
    while (index >= 0 && BgObjects[index] != go_guid)
        index--;
    if (index < 0)
    {
        TC_LOG_ERROR("bg.battleground", "Battleground::HandleTriggerBuff: cannot find buff gameobject (GUID: %u, entry: %u, type: %u) in internal data for BG (map: %u, instance id: %u)!",
            go_guid.GetCounter(), obj->GetEntry(), obj->GetGoType(), m_MapId, m_InstanceID);
        return;
    }

    // Randomly select new buff
    uint8 buff = urand(0, 2);
    uint32 entry = obj->GetEntry();
    if (m_BuffChange && entry != Buff_Entries[buff])
    {
        // Despawn current buff
        SpawnBGObject(index, RESPAWN_ONE_DAY);
        // Set index for new one
        for (uint8 currBuffTypeIndex = 0; currBuffTypeIndex < 3; ++currBuffTypeIndex)
            if (entry == Buff_Entries[currBuffTypeIndex])
            {
                index -= currBuffTypeIndex;
                index += buff;
            }
    }

    SpawnBGObject(index, BUFF_RESPAWN_TIME);
}

void Battleground::HandleKillPlayer(Player* victim, Player* killer)
{
    // Keep in mind that for arena this will have to be changed a bit

    // Add +1 deaths
    UpdatePlayerScore(victim, SCORE_DEATHS, 1);
    // Add +1 kills to group and +1 killing_blows to killer
    if (killer)
    {
        // Don't reward credit for killing ourselves, like fall damage of hellfire (warlock)
        if (killer == victim)
            return;

        UpdatePlayerScore(killer, SCORE_HONORABLE_KILLS, 1);
        UpdatePlayerScore(killer, SCORE_KILLING_BLOWS, 1);

        for (BattlegroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
        {
            Player* creditedPlayer = ObjectAccessor::GetPlayer(*m_Map, itr->first);
            if (!creditedPlayer || creditedPlayer == killer)
                continue;

            if (creditedPlayer->GetTeam() == killer->GetTeam() && creditedPlayer->IsAtGroupRewardDistance(victim))
                UpdatePlayerScore(creditedPlayer, SCORE_HONORABLE_KILLS, 1);
        }
    }

    if (!isArena())
    {
        // To be able to remove insignia -- ONLY IN Battlegrounds
        victim->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);
        RewardXPAtKill(killer, victim);
    }
}

// Return the player's team based on battlegroundplayer info
// Used in same faction arena matches mainly
uint32 Battleground::GetPlayerTeam(ObjectGuid guid) const
{
    BattlegroundPlayerMap::const_iterator itr = m_Players.find(guid);
    if (itr != m_Players.end())
        return itr->second.Team;
    return 0;
}

uint32 Battleground::GetOtherTeam(uint32 teamId) const
{
    return teamId ? ((teamId == ALLIANCE) ? HORDE : ALLIANCE) : 0;
}

bool Battleground::IsPlayerInBattleground(ObjectGuid guid) const
{
    BattlegroundPlayerMap::const_iterator itr = m_Players.find(guid);
    if (itr != m_Players.end())
        return true;
    return false;
}

void Battleground::PlayerAddedToBGCheckIfBGIsRunning(Player* player)
{
    if (GetStatus() != STATUS_WAIT_LEAVE)
        return;

    BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(GetTypeID(), GetArenaType());

    BlockMovement(player);

    WorldPacket data = sBattlegroundMgr->BuildPvpLogDataPacket(this);
    player->SendDirectMessage(std::move(data));

    data = sBattlegroundMgr->BuildBattlegroundStatusPacket(this, player->GetBattlegroundQueueIndex(bgQueueTypeId), STATUS_IN_PROGRESS, GetEndTime(), GetStartTime(), GetArenaType(), player->GetTeam());
    player->SendDirectMessage(std::move(data));
}

uint32 Battleground::GetAlivePlayersCountByTeam(uint32 Team) const
{
    int count = 0;
    for (BattlegroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        if (itr->second.Team == Team)
        {
            Player* player = ObjectAccessor::GetPlayer(*m_Map, itr->first);
            if (player && player->IsAlive() && !player->HasByteFlag(UNIT_FIELD_BYTES_2, 3, FORM_SPIRITOFREDEMPTION))
                ++count;
        }
    }
    return count;
}

void Battleground::SetHoliday(bool is_holiday)
{
    m_HonorMode = is_holiday ? BG_HOLIDAY : BG_NORMAL;
}

int32 Battleground::GetObjectType(ObjectGuid guid)
{
    for (uint32 i = 0; i < BgObjects.size(); ++i)
        if (BgObjects[i] == guid)
            return i;
    TC_LOG_ERROR("bg.battleground", "Battleground::GetObjectType: player used gameobject (GUID: %u) which is not in internal data for BG (map: %u, instance id: %u), cheating?",
        guid.GetCounter(), m_MapId, m_InstanceID);
    return -1;
}

void Battleground::HandleKillUnit(Creature* /*victim*/, Player* /*killer*/) { }

void Battleground::CheckArenaAfterTimerConditions()
{
    EndBattleground(WINNER_NONE);
}

void Battleground::CheckArenaWinConditions()
{
    if (!GetAlivePlayersCountByTeam(ALLIANCE) && GetPlayersCountByTeam(HORDE))
        EndBattleground(HORDE);
    else if (GetPlayersCountByTeam(ALLIANCE) && !GetAlivePlayersCountByTeam(HORDE))
        EndBattleground(ALLIANCE);
}

void Battleground::UpdateArenaWorldState()
{
    UpdateWorldState(0xe10, GetAlivePlayersCountByTeam(HORDE));
    UpdateWorldState(0xe11, GetAlivePlayersCountByTeam(ALLIANCE));
}

void Battleground::SetBgRaid(uint32 TeamID, Group* bg_raid)
{
    Group*& old_raid = TeamID == ALLIANCE ? m_BgRaids[TEAM_ALLIANCE] : m_BgRaids[TEAM_HORDE];
    if (old_raid)
        old_raid->SetBattlegroundGroup(NULL);
    if (bg_raid)
        bg_raid->SetBattlegroundGroup(this);
    old_raid = bg_raid;
}

WorldSafeLocsEntry const* Battleground::GetClosestGraveYard(Player* player)
{
    return sObjectMgr->GetClosestGraveYard(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetMapId(), player->GetTeam());
}

void Battleground::StartTimedAchievement(AchievementCriteriaTimedTypes type, uint32 entry)
{
    for (BattlegroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
        if (Player* player = ObjectAccessor::GetPlayer(*m_Map, itr->first))
            player->StartTimedAchievement(type, entry);
}

void Battleground::SetBracket(PvPDifficultyEntry const* bracketEntry)
{
    m_BracketId = bracketEntry->GetBracketId();
    SetLevelRange(bracketEntry->minLevel, bracketEntry->maxLevel);
}

void Battleground::RewardXPAtKill(Player* killer, Player* victim)
{
    if (sWorld->getBoolConfig(CONFIG_BG_XP_FOR_KILL) && killer && victim)
        killer->RewardPlayerAndGroupAtKill(victim, true);
}

uint32 Battleground::GetTeamScore(uint32 teamId) const
{
    if (teamId == TEAM_ALLIANCE || teamId == TEAM_HORDE)
        return m_TeamScores[teamId];

    TC_LOG_ERROR("bg.battleground", "GetTeamScore with wrong Team %u for BG %u", teamId, GetTypeID());
    return 0;
}

void Battleground::HandleAreaTrigger(Player* player, uint32 trigger)
{
    TC_LOG_DEBUG("bg.battleground", "Unhandled AreaTrigger %u in Battleground %u. Player coords (x: %f, y: %f, z: %f)",
                   trigger, player->GetMapId(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ());
}

bool Battleground::CheckAchievementCriteriaMeet(uint32 criteriaId, Player const* /*source*/, Unit const* /*target*/, uint32 /*miscvalue1*/)
{
    TC_LOG_ERROR("bg.battleground", "Battleground::CheckAchievementCriteriaMeet: No implementation for criteria %u", criteriaId);
    return false;
}

uint8 Battleground::GetUniqueBracketId() const
{
    return GetMinLevel() / 10;
}

void Battleground::UpdateRatingTimer(uint32 diff)
{
    if (ratingSampleTimer <= diff)
    {
        ratingSampleTimer = 30 * IN_MILLISECONDS;

        const uint32 bgTeams[BG_TEAMS_COUNT] = { HORDE, ALLIANCE };
        float currentTeamRating[BG_TEAMS_COUNT];
        float currentTeamRD[BG_TEAMS_COUNT];
        int count[BG_TEAMS_COUNT];

        for (uint32 i = 0; i < BG_TEAMS_COUNT; i++)
        {
            currentTeamRating[i] = 0;
            currentTeamRD[i] = 0;
            count[i] = 0;
            for (BattlegroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
            {
                if (itr->second.Team == bgTeams[i])
                {
                    Player* player = ObjectAccessor::GetPlayer(*m_Map, itr->first);
                    if (player)
                    {
                        if (playerSamples.find(player->GetGUID()) == playerSamples.end())
                        {
                            playerSamples[player->GetGUID()] = 0;
                            playerTeam[player->GetGUID()] = i;
                        }
                        playerSamples[player->GetGUID()]++;
                        currentTeamRating[i] += player->pvpData.GetRating().rating;
                        currentTeamRD[i] += player->pvpData.GetRating().rd;
                        ++count[i];
                    }
                }
            }
        }

        int maxcount = std::max(count[0], count[1]);

        if (!maxcount)
            return;

        for (uint32 i = 0; i < BG_TEAMS_COUNT; i++)
        {
            currentTeamRating[i] /= maxcount;
            currentTeamRD[i] /= maxcount;

            bgTeamRating[i] = (ratingSampleSize * bgTeamRating[i] + currentTeamRating[i]) / (ratingSampleSize + 1);
            bgTeamRD[i] = (ratingSampleSize * bgTeamRD[i] + currentTeamRD[i]) / (ratingSampleSize + 1);
        }

        ratingSampleSize++;
    }
    else
        ratingSampleTimer -= diff;
}

void Battleground::EndRatingUpdate()
{
    if (!ratingSampleSize)
        return;

    for (auto itr = playerSamples.begin(); itr != playerSamples.end(); itr++)
    {
        auto team = playerTeam.find(itr->first);

        if (team == playerTeam.end())
            continue;

        uint32 playerTeam = team->second;
        uint32 otherTeam = (playerTeam + 1) % 2;

        float matchResult = 0.5f;
        if (GetWinner() == playerTeam)
            matchResult = 1.f;
        else if (GetWinner() == otherTeam)
            matchResult = 0.f;

        Player* player = ObjectAccessor::GetPlayer(*m_Map, itr->first);
        if (player)
        {
            SkillValue &playerRating = player->pvpData.GetRating();
            SkillValue opponent;
            opponent.rating = playerRating.rating;
            opponent.rating += bgTeamRating[otherTeam] - bgTeamRating[playerTeam];
            opponent.rd = playerRating.rd;
            opponent.rd *= bgTeamRD[otherTeam] / bgTeamRD[playerTeam];

            std::vector<std::pair<SkillValue, float>> matches;
            matches.push_back(std::pair<SkillValue, float>(opponent, matchResult));

            float weight = float(itr->second) / float(ratingSampleSize);

            PvPData::SkillHelper(playerRating, matches, weight);
        }
    }
}

void Battleground::AddArenaMMRData(ObjectGuid guid, uint32 team, SkillValue mmr, uint32 rating)
{
    ratedArenaData[GetTeamIndexByTeamId(team)][guid] = std::make_pair(mmr, rating);
}

uint32 RatingHelper(uint32 oldValue, float mmr, float delta)
{
    delta = std::min(std::max(delta, -32.f), 32.f);
    mmr = std::max(mmr, 1.f);
    if (delta < 0)
        return uint32(std::max(std::round(oldValue + (delta * float(oldValue) / mmr)), 0.f));
    else
    {
        float mod = std::sqrt(std::abs(float(oldValue) - (mmr - delta)));
        if (float(oldValue) > (mmr - delta))
            return uint32(std::max(std::round(float(oldValue) + delta - mod), 0.f));
        else
            return uint32(std::max(std::round(float(oldValue) + delta + mod), 0.f));
    }
}

// precalculate mmr results for each team winning in case a player disconnects 
void Battleground::CalculateArenaMMRData()
{
    isSoloQueueArenaMatch = true;

    for (uint32 winnerTeam = 0; winnerTeam < 2; winnerTeam++)
    {
        auto& result = ratedArenaResultData[winnerTeam];
        auto& ratingChange = ratedArenaRatingChange[winnerTeam];

        uint32 loserTeam = (winnerTeam + 1) % 2;

        std::vector<std::pair<SkillValue, float>> matchWon;
        std::vector<std::pair<SkillValue, float>> matchLost;
        for (auto kvp : ratedArenaData[loserTeam])
            matchWon.push_back(std::make_pair(kvp.second.first, 1.f));

        ratingChange[0] = 0;
        ratingChange[1] = 0;

        for (auto kvp : ratedArenaData[winnerTeam])
        {
            SkillValue skillValue = kvp.second.first;
            PvPData::SkillHelper(skillValue, matchWon, 1.f / float(matchWon.size()));
            matchLost.push_back(std::make_pair(kvp.second.first, 0.f));
            uint32 newRating = RatingHelper(kvp.second.second, skillValue.rating, skillValue.rating - kvp.second.first.rating);
            result[winnerTeam][kvp.first] = std::make_pair(skillValue, newRating);
            ratingChange[winnerTeam] += int32(newRating) - int32(kvp.second.second);
        }

        for (auto kvp : ratedArenaData[loserTeam])
        {
            SkillValue skillValue = kvp.second.first;
            PvPData::SkillHelper(skillValue, matchLost, 1.f / float(matchLost.size()));
            uint32 newRating = RatingHelper(kvp.second.second, skillValue.rating, skillValue.rating - kvp.second.first.rating);
            result[loserTeam][kvp.first] = std::make_pair(skillValue, newRating);
            ratingChange[loserTeam] += int32(newRating) - int32(kvp.second.second);
        }
        
        ratingChange[winnerTeam] /= int32(ratedArenaData[winnerTeam].size());
        ratingChange[loserTeam] /= int32(ratedArenaData[loserTeam].size());
    }
}

void Battleground::UpdateArenaRating(uint32 winner)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    auto& matchResult = ratedArenaResultData[GetTeamIndexByTeamId(winner)];
    auto& ratingChange = ratedArenaRatingChange[GetTeamIndexByTeamId(winner)];

    SetArenaTeamRatingChangeForTeam(ALLIANCE, ratingChange[TEAM_ALLIANCE]);
    SetArenaTeamRatingChangeForTeam(HORDE, ratingChange[TEAM_HORDE]);

    for (uint32 team = 0; team < BG_TEAMS_COUNT; team++)
    {
        auto& teamResult = matchResult[team];
        for (auto& member : teamResult)
        {
            Player* player = ObjectAccessor::FindConnectedPlayer(member.first);
            
            if (player)
            {
                player->pvpData.GetArenaSkillValues() = member.second.first;
                player->pvpData.GetArenaRating() = member.second.second;
                player->pvpData.AddArenaGame();
                player->pvpData.SaveToDB(trans, member.first);
                player->UpdatePersonalArenaRating();
            }
            else
            {
                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_PVP_ARENA_STATS);
                stmt->setUInt32(0, member.second.second);
                stmt->setFloat(1, member.second.first.rating);
                stmt->setFloat(2, member.second.first.rd);
                stmt->setUInt32(3, member.second.first.lastRating);
                stmt->setUInt32(4, member.first.GetCounter());
                trans->Append(stmt);
            }
        }
    }

    CharacterDatabase.CommitTransaction(trans);
}
