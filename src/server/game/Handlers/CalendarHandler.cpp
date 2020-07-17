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

/*
----- Opcodes Not Used yet -----

SMSG_CALENDAR_EVENT_INVITE_NOTES        [ packguid(Invitee), uint64(inviteId), string(Text), Boolean(Unk) ]
?CMSG_CALENDAR_EVENT_INVITE_NOTES       [ uint32(unk1), uint32(unk2), uint32(unk3), uint32(unk4), uint32(unk5) ]
SMSG_CALENDAR_EVENT_INVITE_NOTES_ALERT  [ uint64(inviteId), string(Text) ]
SMSG_CALENDAR_EVENT_INVITE_STATUS_ALERT [ uint64(eventId), uint32(eventTime), uint32(unkFlag), uint8(deletePending) ]
SMSG_CALENDAR_RAID_LOCKOUT_UPDATED      SendCalendarRaidLockoutUpdated(InstanceSave const* save)

@todo

Finish complains' handling - what to do with received complains and how to respond?
Find out what to do with all "not used yet" opcodes
Correct errors sending (event/invite not found, invites exceeded, event already passed, permissions etc.)
Fix locked events to be displayed properly and response time shouldn't be shown for people that haven't respond yet
Copied events should probably have a new owner

*/

#include "InstanceSaveMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "SocialMgr.h"
#include "CalendarMgr.h"
#include "GameEventMgr.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "DatabaseEnv.h"
#include "GuildMgr.h"
#include "ArenaTeamMgr.h"
#include "WorldSession.h"
#include "WorldPacket.h"
#include "DBCStores.h"
#include "Entities/Player/PlayerCache.hpp"

void WorldSession::HandleCalendarGetCalendar(WorldPacket& /*recvData*/)
{
    ObjectGuid guid = GetPlayer()->GetGUID();
    TC_LOG_DEBUG("network.handler.calendar", "CMSG_CALENDAR_GET_CALENDAR [" UI64FMTD "]", guid.GetRawValue());

    time_t currTime = time(NULL);

    WorldPacket data(SMSG_CALENDAR_SEND_CALENDAR, 1000); // Average size if no instance

    sCalendarMgr->WritePlayerData(guid, data);

    data << uint32(currTime);                              // server time
    data << asPackedTime(currTime);                       // zone time

    ByteBuffer dataBuffer;
    uint32 boundCounter = 0;
    for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
    {
        Player::BoundInstancesMap boundInstances = GetPlayer()->GetBoundInstances(Difficulty(i));
        for (Player::BoundInstancesMap::const_iterator itr = boundInstances.begin(); itr != boundInstances.end(); ++itr)
        {
            if (itr->second.perm)
            {
                InstanceSave const* save = itr->second.save;
                dataBuffer << uint32(save->GetMapId());
                dataBuffer << uint32(save->GetDifficulty());
                dataBuffer << uint32(save->GetResetTime() - currTime);
                dataBuffer << uint64(save->GetInstanceId());     // instance save id as unique instance copy id
                ++boundCounter;
            }
        }
    }

    data << uint32(boundCounter);
    data.append(dataBuffer);

    data << uint32(1135753200);                            // Constant date, unk (28.12.2005 07:00)

    // Reuse variables
    boundCounter = 0;
    std::set<uint32> sentMaps;
    dataBuffer.clear();

    ResetTimeByMapDifficultyMap const& resets = sInstanceSaveMgr->GetResetTimeMap();
    for (ResetTimeByMapDifficultyMap::const_iterator itr = resets.begin(); itr != resets.end(); ++itr)
    {
        uint32 mapId = PAIR32_LOPART(itr->first);
        if (sentMaps.find(mapId) != sentMaps.end())
            continue;

        MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
        if (!mapEntry || !mapEntry->IsRaid())
            continue;

        sentMaps.insert(mapId);

        dataBuffer << int32(mapId);
        dataBuffer << int32(itr->second - currTime);
        dataBuffer << int32(0); // Never seen anything else in sniffs - still unknown
        ++boundCounter;
    }

    data << uint32(boundCounter);
    data.append(dataBuffer);

    data << uint32(sGameEventMgr->modifiedHolidays.size());
    for (uint32 entry : sGameEventMgr->modifiedHolidays)
    {
        HolidaysEntry const* holiday = sHolidaysStore.LookupEntry(entry);

        data << uint32(holiday->Id);                        // m_ID
        data << uint32(holiday->Region);                    // m_region, might be looping
        data << uint32(holiday->Looping);                   // m_looping, might be region
        data << uint32(holiday->Priority);                  // m_priority
        data << uint32(holiday->CalendarFilterType);        // m_calendarFilterType

        for (uint8 j = 0; j < MAX_HOLIDAY_DATES; ++j)
            data << uint32(holiday->Date[j]);               // 26 * m_date -- WritePackedTime ?

        for (uint8 j = 0; j < MAX_HOLIDAY_DURATIONS; ++j)
            data << uint32(holiday->Duration[j]);           // 10 * m_duration

        for (uint8 j = 0; j < MAX_HOLIDAY_FLAGS; ++j)
            data << uint32(holiday->CalendarFlags[j]);      // 10 * m_calendarFlags

        data << holiday->TextureFilename;                   // m_textureFilename (holiday name)
    }

    SendPacket(std::move(data));
}

void WorldSession::HandleCalendarGetEvent(WorldPacket& recvData)
{
    uint64 eventId;
    recvData >> eventId;

    TC_LOG_DEBUG("network.handler.calendar", "CMSG_CALENDAR_GET_EVENT. Player ["
        UI64FMTD "] Event [" UI64FMTD "]", GetPlayer()->GetGUID().GetRawValue(), eventId);

    sCalendarMgr->SendEventData(*GetPlayer(), eventId);
}

void WorldSession::HandleCalendarGuildFilter(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network.handler.calendar", "CMSG_CALENDAR_GUILD_FILTER [" UI64FMTD "]", GetPlayer()->GetGUID().GetRawValue());

    uint32 minLevel;
    uint32 maxLevel;
    uint32 minRank;

    recvData >> minLevel >> maxLevel >> minRank;

    if (auto guild = sGuildMgr->GetGuildById(GetPlayer()->GetGuildId()))
        guild->MassInviteToEvent(this, minLevel, maxLevel, minRank);

    TC_LOG_DEBUG("network.handler.calendar", "CMSG_CALENDAR_GUILD_FILTER: Min level [%d], Max level [%d], Min rank [%d]", minLevel, maxLevel, minRank);
}

void WorldSession::HandleCalendarArenaTeam(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network.handler.calendar", "CMSG_CALENDAR_ARENA_TEAM [" UI64FMTD "]", GetPlayer()->GetGUID().GetRawValue());

    uint32 arenaTeamId;
    recvData >> arenaTeamId;

    if (ArenaTeam* team = sArenaTeamMgr->GetArenaTeamById(arenaTeamId))
        team->MassInviteToEvent(this);
}

void WorldSession::HandleCalendarAddEvent(WorldPacket& recvData)
{
    ObjectGuid guid = GetPlayer()->GetGUID();

    uint8 repeatable;
    uint32 maxInvites;

    CalendarEventData eventData;

    eventData.creatorGUID = guid;

    recvData >> eventData.title >> eventData.description >> eventData.type >> repeatable >> maxInvites >> eventData.dungeonId;
    recvData >> asPackedTime(eventData.eventTime) >> asPackedTime(eventData.timezoneTime);
    recvData >> eventData.flags;

    if ((eventData.flags & CALENDAR_FLAG_GUILD_EVENT) != 0 || (eventData.flags & CALENDAR_FLAG_WITHOUT_INVITES) != 0)
        eventData.guildId = _player->GetGuildId();

    // prevent events in the past
    // To Do: properly handle timezones and remove the "- time_t(86400L)" hack
    if (eventData.eventTime < (time(NULL) - time_t(86400L)))
    {
        recvData.rfinish();
        return;
    }

    if ((eventData.flags & CALENDAR_FLAG_WITHOUT_INVITES) == 0)
    {
        // client limits the amount of players to be invited to 100
        const uint32 MaxPlayerInvites = 100;

        uint32 inviteCount;

        recvData >> inviteCount;

        for (uint32 i = 0; i < inviteCount && i < MaxPlayerInvites; ++i)
        {
            CalendarEventData::Invite invite;
            recvData >> invite.invitee.ReadAsPacked();
            recvData >> invite.status >> invite.rank;
            eventData.invitations.push_back(invite);
        }
    }

    sCalendarMgr->AddEvent(*_player, eventData);
}

void WorldSession::HandleCalendarUpdateEvent(WorldPacket& recvData)
{
    ObjectGuid guid = GetPlayer()->GetGUID();

    uint64 eventId;
    uint64 inviteId;
    uint8 repetitionType;
    uint32 maxInvites;
    uint32 flags;

    CalendarEventData eventData;

    recvData >> eventId >> inviteId >> eventData.title >> eventData.description >> eventData.type >> repetitionType >> maxInvites >> eventData.dungeonId;
    recvData >> asPackedTime(eventData.eventTime) >> asPackedTime(eventData.timezoneTime);
    recvData >> eventData.flags;

    // prevent events in the past
    // To Do: properly handle timezones and remove the "- time_t(86400L)" hack
    if (eventData.eventTime < (time(NULL) - time_t(86400L)))
    {
        recvData.rfinish();
        return;
    }

    TC_LOG_DEBUG("network.handler.calendar", "CMSG_CALENDAR_UPDATE_EVENT [" UI64FMTD "] EventId [" UI64FMTD
        "], InviteId [" UI64FMTD "] Title %s, Description %s, type %u "
        "Repeatable %u, MaxInvites %u, Dungeon ID %d, Time %ld "
        "Time2 %ld, Flags %u", guid.GetRawValue(), eventId, inviteId, eventData.title.c_str(),
        eventData.description.c_str(), eventData.type, repetitionType, maxInvites, eventData.dungeonId,
        eventData.eventTime, eventData.timezoneTime, flags);

    sCalendarMgr->UpdateEvent(*_player, eventId, eventData);
}

void WorldSession::HandleCalendarRemoveEvent(WorldPacket& recvData)
{
    ObjectGuid guid = GetPlayer()->GetGUID();
    uint64 eventId;
    uint64 inviteId;

    recvData >> eventId;
    recvData >> inviteId;
    recvData.rfinish(); // Skip flags, we don't use them

    sCalendarMgr->RemoveEvent(*_player, eventId, inviteId);
}

void WorldSession::HandleCalendarCopyEvent(WorldPacket& recvData)
{
    ObjectGuid guid = GetPlayer()->GetGUID();
    uint64 eventId;
    uint64 inviteId;
    time_t eventTime = 0;

    recvData >> eventId >> inviteId;
    recvData >> asPackedTime(eventTime);
    TC_LOG_DEBUG("network.handler.calendar", "CMSG_CALENDAR_COPY_EVENT [" UI64FMTD "], EventId [" UI64FMTD
        "] inviteId [" UI64FMTD "] Time: %ld", guid.GetRawValue(), eventId, inviteId, eventTime);

    // prevent events in the past
    // To Do: properly handle timezones and remove the "- time_t(86400L)" hack
    if (eventTime < (time(NULL) - time_t(86400L)))
    {
        recvData.rfinish();
        return;
    }

    sCalendarMgr->CopyEvent(*_player, eventId, eventTime);
}

void WorldSession::HandleCalendarEventInvite(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network.handler.calendar", "CMSG_CALENDAR_EVENT_INVITE");

    ObjectGuid playerGuid = GetPlayer()->GetGUID();

    uint64 eventId;
    uint64 inviteId;
    std::string name;
    bool isPreInvite;
    bool isGuildEvent;

    ObjectGuid inviteeGuid;
    uint32 inviteeTeam = 0;
    uint32 inviteeGuildId = 0;

    recvData >> eventId >> inviteId >> name >> isPreInvite >> isGuildEvent;

    {
        auto pCacheEntry = player::PlayerCache::Get(name);
        if (!pCacheEntry)
        {
            CalendarMgr::SendCalendarCommandResult(*_player, CALENDAR_ERROR_PLAYER_NOT_FOUND);
            return;
        }
        inviteeGuid = pCacheEntry.guid;
        inviteeTeam = Player::TeamForRace(pCacheEntry.race);
        inviteeGuildId = pCacheEntry.guild;
        // unlock player cache
    }

    if (GetPlayer()->GetTeam() != inviteeTeam && !sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_CALENDAR))
    {
        CalendarMgr::SendCalendarCommandResult(*_player, CALENDAR_ERROR_NOT_ALLIED);
        return;
    }

    if (QueryResult result = CharacterDatabase.PQuery("SELECT flags FROM character_social WHERE guid = " UI64FMTD " AND friend = " UI64FMTD, inviteeGuid.GetCounter(), playerGuid.GetCounter()))
    {
        Field* fields = result->Fetch();
        if (fields[0].GetUInt8() & SOCIAL_FLAG_IGNORED)
        {
            CalendarMgr::SendCalendarCommandResult(*_player, CALENDAR_ERROR_IGNORING_YOU_S, name.c_str());
            return;
        }
    }

    sCalendarMgr->AddInvite(*_player, eventId, inviteId, inviteeGuid, inviteeGuildId, isPreInvite, isGuildEvent);
}

void WorldSession::HandleCalendarEventSignup(WorldPacket& recvData)
{
    ObjectGuid guid = GetPlayer()->GetGUID();
    uint64 eventId;
    bool tentative;

    recvData >> eventId >> tentative;
    TC_LOG_DEBUG("network.handler.calendar", "CMSG_CALENDAR_EVENT_SIGNUP [" UI64FMTD "] EventId [" UI64FMTD "] Tentative %u", guid.GetRawValue(), eventId, tentative);

    sCalendarMgr->EventSignup(*_player, eventId, tentative);
}

void WorldSession::HandleCalendarEventRsvp(WorldPacket& recvData)
{
    uint64 eventId;
    uint64 inviteId;
    uint32 status;

    recvData >> eventId >> inviteId >> status;
    TC_LOG_DEBUG("network.handler.calendar", "CMSG_CALENDAR_EVENT_RSVP [" UI64FMTD "] EventId ["
        UI64FMTD "], InviteId [" UI64FMTD "], status %u", _player->GetGUID().GetRawValue(), eventId,
        inviteId, status);

    sCalendarMgr->EventSetStatus(*_player, eventId, inviteId, status);
}

void WorldSession::HandleCalendarEventRemoveInvite(WorldPacket& recvData)
{
    ObjectGuid guid = GetPlayer()->GetGUID();
    ObjectGuid invitee;
    uint64 eventId;
    uint64 senderInviteId;
    uint64 inviteId;

    recvData >> invitee.ReadAsPacked();
    recvData >> inviteId >> senderInviteId >> eventId;

    TC_LOG_DEBUG("network.handler.calendar", "CMSG_CALENDAR_EVENT_REMOVE_INVITE [%s] EventId [" UI64FMTD "], senderInviteId ["
        UI64FMTD "], Invitee ([" UI64FMTD "] id: [" UI64FMTD "])",
        guid.ToString().c_str(), eventId, senderInviteId, invitee.GetRawValue(), inviteId);

    if (!senderInviteId)
        senderInviteId = inviteId;

    sCalendarMgr->RemoveInvite(*_player, eventId, senderInviteId, inviteId, invitee);
}

void WorldSession::HandleCalendarEventStatus(WorldPacket& recvData)
{
    ObjectGuid guid = GetPlayer()->GetGUID();
    ObjectGuid invitee;
    uint64 eventId;
    uint64 inviteId;
    uint64 ownerInviteId; // isn't it sender's inviteId?
    uint8 status;

    recvData >> invitee.ReadAsPacked();
    recvData >> eventId >> inviteId >> ownerInviteId >> status;
    TC_LOG_DEBUG("network.handler.calendar", "CMSG_CALENDAR_EVENT_STATUS [%s] EventId ["
        UI64FMTD "] ownerInviteId [" UI64FMTD "], Invitee ([" UI64FMTD "] id: ["
        UI64FMTD "], status %u", guid.ToString().c_str(), eventId, ownerInviteId, invitee.GetRawValue(), inviteId, status);

    sCalendarMgr->EventSetStatus(*_player, eventId, inviteId, status);
}

void WorldSession::HandleCalendarEventModeratorStatus(WorldPacket& recvData)
{
    ObjectGuid guid = GetPlayer()->GetGUID();
    ObjectGuid invitee;
    uint64 eventId;
    uint64 inviteId;
    uint64 senderInviteId;
    uint8 rank;

    recvData >> invitee.ReadAsPacked();
    recvData >> eventId >> inviteId >> senderInviteId >> rank;
    TC_LOG_DEBUG("network.handler.calendar", "CMSG_CALENDAR_EVENT_MODERATOR_STATUS [" UI64FMTD "] EventId ["
        UI64FMTD "] senderInviteId [" UI64FMTD "], Invitee ([" UI64FMTD "] id: ["
        UI64FMTD "], rank %u", guid.GetRawValue(), eventId, senderInviteId, invitee.GetRawValue(), inviteId, rank);

    sCalendarMgr->EventSetModeratorStatus(*_player, eventId, senderInviteId, inviteId, rank);
}

void WorldSession::HandleCalendarComplain(WorldPacket& recvData)
{
    ObjectGuid guid = GetPlayer()->GetGUID();
    uint64 eventId;
    ObjectGuid complainGUID;

    recvData >> eventId >> complainGUID;
    TC_LOG_DEBUG("network.handler.calendar", "CMSG_CALENDAR_COMPLAIN [" UI64FMTD "] EventId ["
        UI64FMTD "] guid [%s]", guid.GetRawValue(), eventId, complainGUID.ToString().c_str());

    // what to do with complains?
}

void WorldSession::HandleCalendarGetNumPending(WorldPacket& /*recvData*/)
{
    ObjectGuid guid = GetPlayer()->GetGUID();
    uint32 pending = sCalendarMgr->GetPlayerNumPending(guid);

    TC_LOG_DEBUG("network.handler.calendar", "CMSG_CALENDAR_GET_NUM_PENDING: [%s] Pending: %u",
                 guid.ToString().c_str(), pending);

    WorldPacket data(SMSG_CALENDAR_SEND_NUM_PENDING, 4);
    data << uint32(pending);
    SendPacket(std::move(data));
}

void WorldSession::HandleSetSavedInstanceExtend(WorldPacket& recvData)
{
    uint32 mapId, difficulty;
    uint8 toggleExtend;
    recvData >> mapId >> difficulty>> toggleExtend;
    TC_LOG_DEBUG("network.handler.calendar", "CMSG_SET_SAVED_INSTANCE_EXTEND - MapId: %u, Difficulty: %u, ToggleExtend: %s", mapId, difficulty, toggleExtend ? "On" : "Off");

    /*
    InstancePlayerBind* instanceBind = GetPlayer()->GetBoundInstance(mapId, Difficulty(difficulty));
    if (!instanceBind || !instanceBind->save)
        return;

    InstanceSave* save = instanceBind->save;
    // http://www.wowwiki.com/Instance_Lock_Extension
    // SendCalendarRaidLockoutUpdated(save);
    */
}

// ----------------------------------- SEND ------------------------------------

void WorldSession::SendCalendarRaidLockout(InstanceSave const* save, bool add)
{
    TC_LOG_DEBUG("network.handler.calendar", "%s", add ? "SMSG_CALENDAR_RAID_LOCKOUT_ADDED" : "SMSG_CALENDAR_RAID_LOCKOUT_REMOVED");
    time_t currTime = time(nullptr);

    WorldPacket data(SMSG_CALENDAR_RAID_LOCKOUT_REMOVED, (add ? 4 : 0) + 4 + 4 + 4 + 8);
    if (add)
    {
        data.SetOpcode(SMSG_CALENDAR_RAID_LOCKOUT_ADDED);
        data << asPackedTime(currTime);
    }

    data << uint32(save->GetMapId());
    data << uint32(save->GetDifficulty());
    data << uint32(save->GetResetTime() - currTime);
    data << uint64(save->GetInstanceId());
    SendPacket(std::move(data));
}

void WorldSession::SendCalendarRaidLockoutUpdated(InstanceSave const* save)
{
    if (!save)
        return;

    const ObjectGuid guid = GetPlayer()->GetGUID();
    TC_LOG_DEBUG("network.handler.calendar", "SMSG_CALENDAR_RAID_LOCKOUT_UPDATED [%s] Map: %u, Difficulty %u",
                 guid.ToString().c_str(), save->GetMapId(), save->GetDifficulty());

    const time_t currTime = time(nullptr);

    WorldPacket data(SMSG_CALENDAR_RAID_LOCKOUT_UPDATED, 4 + 4 + 4 + 4 + 8);
    data << asPackedTime(currTime);
    data << uint32(save->GetMapId());
    data << uint32(save->GetDifficulty());
    data << uint32(0); // Amount of seconds that has changed to the reset time
    data << uint32(save->GetResetTime() - currTime);
    SendPacket(std::move(data));
}
