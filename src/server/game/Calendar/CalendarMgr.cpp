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

#include "CalendarMgr.h"
#include "QueryResult.h"
#include "Log.h"
#include "GuildMgr.h"
#include "ObjectAccessor.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "Entities/Player/PlayerCache.hpp"

CalendarMgr::CalendarMgr() : _maxEventId(0), _maxInviteId(0) { }

CalendarMgr::~CalendarMgr()
{
    for (CalendarEventStore::iterator itr = _events.begin(); itr != _events.end(); ++itr)
        delete *itr;

    for (CalendarInviteStore::iterator itr = _invites.begin(); itr != _invites.end(); ++itr)
        delete *itr;
}

CalendarMgr* CalendarMgr::instance()
{
    static CalendarMgr instance;
    return &instance;
}

void CalendarMgr::LoadFromDB()
{
    uint32 count = 0;
    _maxEventId = 0;
    _maxInviteId = 0;

    //                                                       0   1        2      3            4     5        6          7      8
    if (QueryResult result = CharacterDatabase.Query("SELECT id, creator, title, description, type, dungeon, eventtime, flags, time2 FROM calendar_events"))
        do
        {
            Field* fields = result->Fetch();

            uint64 eventId          = fields[0].GetUInt64();
            ObjectGuid creatorGUID  = ObjectGuid(HighGuid::Player, fields[1].GetUInt32());
            std::string title       = fields[2].GetString();
            std::string description = fields[3].GetString();
            CalendarEventType type  = CalendarEventType(fields[4].GetUInt8());
            int32 dungeonId         = fields[5].GetInt32();
            uint32 eventTime        = fields[6].GetUInt32();
            uint32 flags            = fields[7].GetUInt32();
            uint32 timezoneTime     = fields[8].GetUInt32();
            uint32 guildId = 0;

            if (flags & CALENDAR_FLAG_GUILD_EVENT || flags & CALENDAR_FLAG_WITHOUT_INVITES)
                guildId = player::PlayerCache::GetGuildId(creatorGUID);

            CalendarEvent* calendarEvent = new CalendarEvent(eventId, creatorGUID, guildId, type, dungeonId, time_t(eventTime), flags, time_t(timezoneTime), title, description);
            _events.insert(calendarEvent);

            _maxEventId = std::max(_maxEventId, eventId);

            ++count;
        }
        while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u calendar events", count);
    count = 0;

    //                                                       0   1      2        3       4       5           6     7
    if (QueryResult result = CharacterDatabase.Query("SELECT id, event, invitee, sender, status, statustime, rank, text FROM calendar_invites"))
        do
        {
            Field* fields = result->Fetch();

            uint64 inviteId             = fields[0].GetUInt64();
            uint64 eventId              = fields[1].GetUInt64();
            ObjectGuid invitee          = ObjectGuid(HighGuid::Player, fields[2].GetUInt32());
            ObjectGuid senderGUID       = ObjectGuid(HighGuid::Player, fields[3].GetUInt32());
            CalendarInviteStatus status = CalendarInviteStatus(fields[4].GetUInt8());
            uint32 statusTime           = fields[5].GetUInt32();
            CalendarModerationRank rank = CalendarModerationRank(fields[6].GetUInt8());
            std::string text            = fields[7].GetString();

            CalendarInvite* invite = new CalendarInvite(inviteId, eventId, invitee, senderGUID, time_t(statusTime), status, rank, text);
            _invites.insert(invite);

            _maxInviteId = std::max(_maxInviteId, inviteId);

            ++count;
        }
        while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u calendar invites", count);

    for (uint64 i = 1; i < _maxEventId; ++i)
        if (!GetEventUnsafe(i))
            _freeEventIds.push_back(i);

    for (uint64 i = 1; i < _maxInviteId; ++i)
        if (!GetInviteUnsafe(i))
            _freeInviteIds.push_back(i);
}

void CalendarMgr::AddEventUnsafe(CalendarEvent* calendarEvent, SQLTransaction& trans)
{
    _events.insert(calendarEvent);
    UpdateEventInDBUnsafe(calendarEvent, trans);
}

void CalendarMgr::AddInviteUnsafe(CalendarEvent* calendarEvent, CalendarInvite* invite)
{
    SQLTransaction dummy;
    AddInviteUnsafe(calendarEvent, invite, dummy);
}

void CalendarMgr::AddInviteUnsafe(CalendarEvent* calendarEvent, CalendarInvite* invite, SQLTransaction& trans)
{
    if (!calendarEvent->IsGuildAnnouncement())
        SendCalendarEventInviteUnsafe(calendarEvent, *invite);

    if (!calendarEvent->IsGuildEvent() || invite->GetInviteeGUID() == calendarEvent->GetCreatorGUID())
        SendCalendarEventInviteAlertUnsafe(*calendarEvent, *invite);

    if (!calendarEvent->IsGuildAnnouncement())
    {
        _invites.insert(invite);
        UpdateInviteInDBUnsafe(invite, trans);
    }
}

void CalendarMgr::RemoveInviteUnsafe(uint64 inviteId, uint64 eventId)
{
    CalendarEvent* calendarEvent = GetEventUnsafe(eventId);

    if (!calendarEvent)
        return;

    auto& index = _invites.get<CalendarInvite::id>();
    auto itr = index.find(inviteId);
    if (itr == index.end() || (*itr)->GetEventId() != eventId)
        return;

    CalendarInvite* invite = (*itr);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CALENDAR_INVITE);
    stmt->setUInt64(0, invite->GetInviteId());
    trans->Append(stmt);
    CharacterDatabase.CommitTransaction(trans);

    if (!calendarEvent->IsGuildEvent())
        SendCalendarEventInviteRemoveAlertUnsafe(invite->GetInviteeGUID(), *calendarEvent, CALENDAR_STATUS_REMOVED);

    SendCalendarEventInviteRemoveUnsafe(*calendarEvent, *invite, calendarEvent->GetFlags());

    // we need to find out how to use CALENDAR_INVITE_REMOVED_MAIL_SUBJECT to force client to display different mail
    //if ((*itr)->GetInviteeGUID() != remover)
    //    MailDraft(calendarEvent->BuildCalendarMailSubject(remover), calendarEvent->BuildCalendarMailBody())
    //        .SendMailTo(trans, MailReceiver((*itr)->GetInvitee()), calendarEvent, MAIL_CHECK_MASK_COPIED);

    index.erase(itr);
    FreeInviteIdUnsafe(invite->GetInviteId());
    delete invite;
}

void CalendarMgr::UpdateEventInDBUnsafe(CalendarEvent* calendarEvent, SQLTransaction& trans)
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_CALENDAR_EVENT);
    stmt->setUInt64(0, calendarEvent->GetEventId());
    stmt->setUInt32(1, calendarEvent->GetCreatorGUID().GetCounter());
    stmt->setString(2, calendarEvent->GetTitle());
    stmt->setString(3, calendarEvent->GetDescription());
    stmt->setUInt8(4, calendarEvent->GetType());
    stmt->setInt32(5, calendarEvent->GetDungeonId());
    stmt->setUInt32(6, uint32(calendarEvent->GetEventTime()));
    stmt->setUInt32(7, calendarEvent->GetFlags());
    stmt->setUInt32(8, calendarEvent->GetTimeZoneTime()); // correct?
    trans->Append(stmt);
}

void CalendarMgr::UpdateInviteInDBUnsafe(CalendarInvite* invite)
{
    SQLTransaction dummy;
    UpdateInviteInDBUnsafe(invite, dummy);
}

void CalendarMgr::UpdateInviteInDBUnsafe(CalendarInvite* invite, SQLTransaction& trans)
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_CALENDAR_INVITE);
    stmt->setUInt64(0, invite->GetInviteId());
    stmt->setUInt64(1, invite->GetEventId());
    stmt->setUInt32(2, invite->GetInviteeGUID().GetCounter());
    stmt->setUInt32(3, invite->GetSenderGUID().GetCounter());
    stmt->setUInt8(4, invite->GetStatus());
    stmt->setUInt32(5, uint32(invite->GetStatusTime()));
    stmt->setUInt8(6, invite->GetRank());
    stmt->setString(7, invite->GetText());
    CharacterDatabase.ExecuteOrAppend(trans, stmt);
}

CalendarEvent* CalendarMgr::GetEventUnsafe(uint64 eventId) const
{
    auto& index = _events.get<CalendarEvent::id>();
    auto itr = index.find(eventId);
    if (itr != index.end())
        return *itr;

    TC_LOG_DEBUG("calendar", "CalendarMgr::GetEvent: [" UI64FMTD "] not found!", eventId);
    return NULL;
}

CalendarInvite* CalendarMgr::GetInviteUnsafe(uint64 inviteId) const
{
    auto& index = _invites.get<CalendarInvite::id>();
    auto itr = index.find(inviteId);
    if (itr != index.end())
        return *itr;
    
    TC_LOG_DEBUG("calendar", "CalendarMgr::GetInviteUnsafe: [" UI64FMTD "] not found!", inviteId);
    return NULL;
}

void CalendarMgr::FreeEventIdUnsafe(uint64 id)
{
    if (id == _maxEventId)
        --_maxEventId;
    else
        _freeEventIds.push_back(id);
}

uint64 CalendarMgr::GetFreeEventIdUnsafe()
{
    if (_freeEventIds.empty())
        return ++_maxEventId;

    uint64 eventId = _freeEventIds.front();
    _freeEventIds.pop_front();
    return eventId;
}

void CalendarMgr::FreeInviteIdUnsafe(uint64 id)
{
    if (id == _maxInviteId)
        --_maxInviteId;
    else
        _freeInviteIds.push_back(id);
}

uint64 CalendarMgr::GetFreeInviteIdUnsafe()
{
    if (_freeInviteIds.empty())
        return ++_maxInviteId;

    uint64 inviteId = _freeInviteIds.front();
    _freeInviteIds.pop_front();
    return inviteId;
}

CalendarEventStore CalendarMgr::GetPlayerEventsUnsafe(ObjectGuid guid)
{
    CalendarEventStore events;

    {
        auto& index = _invites.get<CalendarInvite::invitee_guid>();
        auto idx_invitee = index.find(guid);
        if (idx_invitee != index.end())
        {
            auto range = index.equal_range(index.key_extractor()(*idx_invitee));
            for (auto itr = range.first; itr != range.second; ++itr)
                if (CalendarEvent* event = GetEventUnsafe((*itr)->GetEventId())) // NULL check added as attempt to fix #11512
                    events.insert(event);
        }
    }

    if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
    {
        if (player->GetGuildId())
        {
            auto& index = _events.get<CalendarEvent::guild_id>();
            auto itr_guid = index.find(player->GetGuildId());
            if (itr_guid != index.end())
            {
                auto range = index.equal_range(index.key_extractor()(*itr_guid));
                for (auto itr_event = range.first; itr_event != range.second; ++itr_event)
                    events.insert(*itr_event);
            }
        }
    }

    return events;
}

CalendarInviteStore CalendarMgr::GetEventInvitesUnsafe(uint64 eventId)
{
    CalendarInviteStore invites;

    auto& index = _invites.get<CalendarInvite::event_id>();
    auto idx_invitee = index.find(eventId);
    if (idx_invitee != index.end())
    {
        auto range = index.equal_range(index.key_extractor()(*idx_invitee));
        for (auto itr = range.first; itr != range.second; ++itr)
            invites.insert(*itr);
    }

    return invites;
}

CalendarInviteStore CalendarMgr::GetPlayerInvitesUnsafe(ObjectGuid guid)
{
    CalendarInviteStore invites;

    auto& index = _invites.get<CalendarInvite::invitee_guid>();
    auto idx_invitee = index.find(guid);
    if (idx_invitee != index.end())
    {
        auto range = index.equal_range(index.key_extractor()(*idx_invitee));
        for (auto itr = range.first; itr != range.second; ++itr)
            invites.insert(*itr);
    }

    return invites;
}

std::string CalendarEvent::BuildCalendarMailSubject(ObjectGuid remover) const
{
    std::ostringstream strm;
    strm << remover.GetRawValue() << ':' << _title;
    return strm.str();
}

std::string CalendarEvent::BuildCalendarMailBody() const
{
    std::ostringstream strm;

    strm << createPackedTime(_eventTime);
    return strm.str();
}

void CalendarMgr::SendCalendarEventInviteUnsafe(Player& player, CalendarInvite const& invite)
{
    time_t statusTime = invite.GetStatusTime();
    bool hasStatusTime = statusTime != 946684800;   // 01/01/2000 00:00:00

    WorldPacket data(SMSG_CALENDAR_EVENT_INVITE, 8 + 8 + 8 + 1 + 1 + 1 + (hasStatusTime ? 4 : 0) + 1);
    WriteCalendarEventInviteUnsafe(data, hasStatusTime, invite);
    player.SendDirectMessage(std::move(data));
}

void CalendarMgr::SendCalendarEventInviteUnsafe(CalendarEvent* calendarEvent, CalendarInvite const& invite)
{
    time_t statusTime = invite.GetStatusTime();
    bool hasStatusTime = statusTime != 946684800;   // 01/01/2000 00:00:00

    WorldPacket data(SMSG_CALENDAR_EVENT_INVITE, 8 + 8 + 8 + 1 + 1 + 1 + (hasStatusTime ? 4 : 0) + 1);
    WriteCalendarEventInviteUnsafe(data, hasStatusTime, invite);
    SendPacketToAllEventRelativesUnsafe(data, *calendarEvent);
}

void CalendarMgr::WriteCalendarEventInviteUnsafe(ByteBuffer& data, bool hasStatusTime, CalendarInvite const& invite)
{
    ObjectGuid invitee = invite.GetInviteeGUID();
    uint8 level = player::PlayerCache::GetLevel(invitee);

    data << invitee.WriteAsPacked();
    data << uint64(invite.GetEventId());
    data << uint64(invite.GetInviteId());
    data << uint8(level);
    data << uint8(invite.GetStatus());
    data << uint8(hasStatusTime);
    if (hasStatusTime)
        data << asPackedTime(invite.GetStatusTime());
    data << uint8(invite.GetSenderGUID() != invite.GetInviteeGUID()); // false only if the invite is sign-up
}

void CalendarMgr::SendCalendarEventUpdateAlertUnsafe(CalendarEvent const& calendarEvent, time_t oldEventTime)
{
    WorldPacket data(SMSG_CALENDAR_EVENT_UPDATED_ALERT, 1 + 8 + 4 + 4 + 4 + 1 + 4 +
        calendarEvent.GetTitle().size() + calendarEvent.GetDescription().size() + 1 + 4 + 4);
    data << uint8(1);       // unk
    data << uint64(calendarEvent.GetEventId());
    data << asPackedTime(oldEventTime);
    data << uint32(calendarEvent.GetFlags());
    data << asPackedTime(calendarEvent.GetEventTime());
    data << uint8(calendarEvent.GetType());
    data << int32(calendarEvent.GetDungeonId());
    data << calendarEvent.GetTitle();
    data << calendarEvent.GetDescription();
    data << uint8(CALENDAR_REPEAT_NEVER);   // repeatable
    data << uint32(CALENDAR_MAX_INVITES);
    data << uint32(0);      // unk

    SendPacketToAllEventRelativesUnsafe(data, calendarEvent);
}

void CalendarMgr::SendCalendarEventStatusUnsafe(CalendarEvent const& calendarEvent, CalendarInvite const& invite)
{
    WorldPacket data(SMSG_CALENDAR_EVENT_STATUS, 8 + 8 + 4 + 4 + 1 + 1 + 4);
    data << invite.GetInviteeGUID().WriteAsPacked();
    data << uint64(calendarEvent.GetEventId());
    data << asPackedTime(calendarEvent.GetEventTime());
    data << uint32(calendarEvent.GetFlags());
    data << uint8(invite.GetStatus());
    data << uint8(invite.GetRank());
    data << asPackedTime(invite.GetStatusTime());

    SendPacketToAllEventRelativesUnsafe(data, calendarEvent);
}

void CalendarMgr::SendCalendarEventRemovedAlertUnsafe(CalendarEvent const& calendarEvent)
{
    WorldPacket data(SMSG_CALENDAR_EVENT_REMOVED_ALERT, 1 + 8 + 1);
    data << uint8(1); // FIXME: If true does not SignalEvent(EVENT_CALENDAR_ACTION_PENDING)
    data << uint64(calendarEvent.GetEventId());
    data << asPackedTime(calendarEvent.GetEventTime());

    SendPacketToAllEventRelativesUnsafe(data, calendarEvent);
}

void CalendarMgr::SendCalendarEventInviteRemoveUnsafe(CalendarEvent const& calendarEvent, CalendarInvite const& invite, uint32 flags)
{
    WorldPacket data(SMSG_CALENDAR_EVENT_INVITE_REMOVED, 8 + 4 + 4 + 1);
    data << invite.GetInviteeGUID().WriteAsPacked();
    data << uint64(invite.GetEventId());
    data << uint32(flags);
    data << uint8(1); // FIXME

    SendPacketToAllEventRelativesUnsafe(data, calendarEvent);
}

void CalendarMgr::SendCalendarEventModeratorStatusAlertUnsafe(CalendarEvent const& calendarEvent, CalendarInvite const& invite)
{
    WorldPacket data(SMSG_CALENDAR_EVENT_MODERATOR_STATUS_ALERT, 8 + 8 + 1 + 1);
    data << invite.GetInviteeGUID().WriteAsPacked();
    data << uint64(invite.GetEventId());
    data << uint8(invite.GetRank());
    data << uint8(1); // Unk boolean - Display to client?

    SendPacketToAllEventRelativesUnsafe(data, calendarEvent);
}

void CalendarMgr::SendCalendarEventInviteAlertUnsafe(CalendarEvent const& calendarEvent, CalendarInvite const& invite)
{
    WorldPacket data(SMSG_CALENDAR_EVENT_INVITE_ALERT);
    data << uint64(calendarEvent.GetEventId());
    data << calendarEvent.GetTitle();
    data << asPackedTime(calendarEvent.GetEventTime());
    data << uint32(calendarEvent.GetFlags());
    data << uint32(calendarEvent.GetType());
    data << int32(calendarEvent.GetDungeonId());
    data << uint64(invite.GetInviteId());
    data << uint8(invite.GetStatus());
    data << uint8(invite.GetRank());
    data << calendarEvent.GetCreatorGUID().WriteAsPacked();
    data << invite.GetSenderGUID().WriteAsPacked();

    if (calendarEvent.IsGuildEvent() || calendarEvent.IsGuildAnnouncement())
    {
        if (auto guild = sGuildMgr->GetGuildById(calendarEvent.GetGuildId()))
            guild->BroadcastPacket(data);
    }
    else if (Player* player = ObjectAccessor::FindConnectedPlayer(invite.GetInviteeGUID()))
        player->SendDirectMessage(std::move(data));
}

void CalendarMgr::SendCalendarEventUnsafe(Player& player, CalendarEvent const& calendarEvent, CalendarSendEventType sendType)
{
    std::vector<CalendarInvite*> invites;
    auto& index = _invites.get<CalendarInvite::event_id>();
    auto idx_invitee = index.find(calendarEvent.GetEventId());
    if (idx_invitee != index.end())
    {
        auto range = index.equal_range(index.key_extractor()(*idx_invitee));
        for (auto itr = range.first; itr != range.second; ++itr)
            invites.push_back(*itr);
    }

    WorldPacket data(SMSG_CALENDAR_SEND_EVENT, 60 + invites.size() * 32);
    data << uint8(sendType);
    data << calendarEvent.GetCreatorGUID().WriteAsPacked();
    data << uint64(calendarEvent.GetEventId());
    data << calendarEvent.GetTitle();
    data << calendarEvent.GetDescription();
    data << uint8(calendarEvent.GetType());
    data << uint8(CALENDAR_REPEAT_NEVER);   // repeatable
    data << uint32(CALENDAR_MAX_INVITES);
    data << int32(calendarEvent.GetDungeonId());
    data << uint32(calendarEvent.GetFlags());
    data << asPackedTime(calendarEvent.GetEventTime());
    data << asPackedTime(calendarEvent.GetTimeZoneTime());
    data << uint32(calendarEvent.GetGuildId());

    data << uint32(invites.size());
    for (auto itr = invites.begin(); itr != invites.end(); ++itr)
    {
        CalendarInvite const* calendarInvite = (*itr);
        ObjectGuid inviteeGuid = calendarInvite->GetInviteeGUID();
        Player* invitee = ObjectAccessor::FindConnectedPlayer(inviteeGuid);

        uint8 inviteeLevel = invitee ? invitee->getLevel() : player::PlayerCache::GetLevel(inviteeGuid);
        uint32 inviteeGuildId = invitee ? invitee->GetGuildId() : player::PlayerCache::GetGuildId(inviteeGuid);

        data << inviteeGuid.WriteAsPacked();
        data << uint8(inviteeLevel);
        data << uint8(calendarInvite->GetStatus());
        data << uint8(calendarInvite->GetRank());
        data << uint8(calendarEvent.IsGuildEvent() && calendarEvent.GetGuildId() == inviteeGuildId);
        data << uint64(calendarInvite->GetInviteId());
        data << asPackedTime(calendarInvite->GetStatusTime());
        data << calendarInvite->GetText();
    }

    player.SendDirectMessage(std::move(data));
}

void CalendarMgr::SendCalendarEventInviteRemoveAlertUnsafe(ObjectGuid guid, CalendarEvent const& calendarEvent, CalendarInviteStatus status)
{
    if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
    {
        WorldPacket data(SMSG_CALENDAR_EVENT_INVITE_REMOVED_ALERT, 8 + 4 + 4 + 1);
        data << uint64(calendarEvent.GetEventId());
        data << asPackedTime(calendarEvent.GetEventTime());
        data << uint32(calendarEvent.GetFlags());
        data << uint8(status);

        player->SendDirectMessage(std::move(data));
    }
}

void CalendarMgr::SendCalendarClearPendingActionUnsafe(Player& player)
{
    WorldPacket data(SMSG_CALENDAR_CLEAR_PENDING_ACTION, 0);
    player.SendDirectMessage(std::move(data));
}

void CalendarMgr::SendCalendarCommandResult(Player& player, CalendarError err, char const* param /*= NULL*/)
{
    WorldPacket data(SMSG_CALENDAR_COMMAND_RESULT, 0);
    data << uint32(0);
    data << uint8(0);
    switch (err)
    {
        case CALENDAR_ERROR_OTHER_INVITES_EXCEEDED:
        case CALENDAR_ERROR_ALREADY_INVITED_TO_EVENT_S:
        case CALENDAR_ERROR_IGNORING_YOU_S:
            data << param;
            break;
        default:
            data << uint8(0);
            break;
    }

    data << uint32(err);

    player.SendDirectMessage(std::move(data));
}

void CalendarMgr::SendPacketToAllEventRelativesUnsafe(const WorldPacket& packet, CalendarEvent const& calendarEvent)
{
    // Send packet to all guild members
    if (calendarEvent.IsGuildEvent() || calendarEvent.IsGuildAnnouncement())
        if (auto guild = sGuildMgr->GetGuildById(calendarEvent.GetGuildId()))
            guild->BroadcastPacket(packet);

    // Send packet to all invitees if event is non-guild, in other case only to non-guild invitees (packet was broadcasted for them)
    auto& index = _invites.get<CalendarInvite::event_id>();
    auto idx_invitee = index.find(calendarEvent.GetEventId());
    if (idx_invitee != index.end())
    {
        auto range = index.equal_range(index.key_extractor()(*idx_invitee));
        for (auto itr = range.first; itr != range.second; ++itr)
            if (Player* player = ObjectAccessor::FindConnectedPlayer((*itr)->GetInviteeGUID()))
                if (!calendarEvent.IsGuildEvent() || (calendarEvent.IsGuildEvent() && player->GetGuildId() != calendarEvent.GetGuildId()))
                    player->SendDirectMessage(packet);
    }
}

void CalendarMgr::RemoveEventUnsafe(ObjectGuid guid, uint64 eventId)
{
    CalendarEvent* calendarEvent = GetEventUnsafe(eventId);

    if (!calendarEvent)
    {
        if (guid)
            if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
                SendCalendarCommandResult(*player, CALENDAR_ERROR_EVENT_INVALID);
        return;
    }

    if(!guid)
        guid = calendarEvent->GetCreatorGUID();

    SendCalendarEventRemovedAlertUnsafe(*calendarEvent);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    PreparedStatement* stmt;
    MailDraft mail(calendarEvent->BuildCalendarMailSubject(guid), calendarEvent->BuildCalendarMailBody());

    auto& index = _invites.get<CalendarInvite::event_id>();
    auto itr_event = index.find(eventId);
    if (itr_event != index.end())
    {
        auto range = index.equal_range(index.key_extractor()(*itr_event));
        for (auto itr = range.first; itr != range.second; ++itr)
        {
            CalendarInvite* invite = *itr;
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CALENDAR_INVITE);
            stmt->setUInt64(0, invite->GetInviteId());
            trans->Append(stmt);

            // guild events only? check invite status here?
            // When an event is deleted, all invited (accepted/declined? - verify) guildies are notified via in-game mail. (wowwiki)
            if (invite->GetInviteeGUID() != guid)
                mail.SendMailTo(trans, MailReceiver(invite->GetInviteeGUID()), calendarEvent, MAIL_CHECK_MASK_COPIED);

            FreeInviteIdUnsafe(invite->GetInviteId());
            delete invite;
        }
        index.erase(range.first, range.second);
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CALENDAR_EVENT);
    stmt->setUInt64(0, eventId);
    trans->Append(stmt);
    CharacterDatabase.CommitTransaction(trans);

    _events.get<CalendarEvent>().erase(calendarEvent);
    FreeEventIdUnsafe(calendarEvent->GetEventId());
    delete calendarEvent;
}

bool CalendarMgr::HasPermissionUnsafe(ObjectGuid guid, uint32 inviteId, CalendarModerationRank minRank)
{
    if (CalendarInvite* invite = GetInviteUnsafe(inviteId))
        if (invite->GetInviteeGUID() == guid && invite->GetRank() >= minRank)
            return true;
    return false;
}

// Public Methods

void CalendarMgr::WritePlayerData(ObjectGuid guid, ByteBuffer& data)
{
    Lock lock(_mutex);

    CalendarInviteStore invites = GetPlayerInvitesUnsafe(guid);
    data << uint32(invites.size());
    for (CalendarInviteStore::const_iterator itr = invites.begin(); itr != invites.end(); ++itr)
    {
        data << uint64((*itr)->GetEventId());
        data << uint64((*itr)->GetInviteId());
        data << uint8((*itr)->GetStatus());
        data << uint8((*itr)->GetRank());

        if (CalendarEvent* calendarEvent = GetEventUnsafe((*itr)->GetEventId()))
        {
            data << uint8(calendarEvent->IsGuildEvent());
            data << calendarEvent->GetCreatorGUID().WriteAsPacked();
        }
        else
        {
            data << uint8(0);
            data << (*itr)->GetSenderGUID().WriteAsPacked();
        }
    }

    CalendarEventStore playerEvents = GetPlayerEventsUnsafe(guid);
    data << uint32(playerEvents.size());
    for (CalendarEventStore::const_iterator itr = playerEvents.begin(); itr != playerEvents.end(); ++itr)
    {
        CalendarEvent* calendarEvent = *itr;

        data << uint64(calendarEvent->GetEventId());
        data << calendarEvent->GetTitle();
        data << uint32(calendarEvent->GetType());
        data << asPackedTime(calendarEvent->GetEventTime());
        data << uint32(calendarEvent->GetFlags());
        data << int32(calendarEvent->GetDungeonId());
        data << calendarEvent->GetCreatorGUID().WriteAsPacked();
    }
}

void CalendarMgr::SendEventData(Player& player, uint64 eventId)
{
    Lock lock(_mutex);

    if (CalendarEvent* calendarEvent = GetEventUnsafe(eventId))
        SendCalendarEventUnsafe(player, *calendarEvent, CALENDAR_SENDTYPE_GET);
    else
        SendCalendarCommandResult(player, CALENDAR_ERROR_EVENT_INVALID);
}

void CalendarMgr::AddEvent(Player& player, CalendarEventData const& data)
{
    Lock lock(_mutex);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    CalendarEvent* calendarEvent = new CalendarEvent(data, GetFreeEventIdUnsafe());
    AddEventUnsafe(calendarEvent, trans);

    for (auto& itr : data.invitations)
    {
        CalendarInvite* invite = new CalendarInvite(GetFreeInviteIdUnsafe(), calendarEvent->GetEventId(), itr.invitee, player.GetGUID(), 946684800,
            CalendarInviteStatus(itr.status), CalendarModerationRank(itr.rank), "");
        AddInviteUnsafe(calendarEvent, invite, trans);
    }

    // notify guild
    if (calendarEvent->IsGuildAnnouncement())
    {
        CalendarInvite dummy(0, calendarEvent->GetEventId(), ObjectGuid::Empty, player.GetGUID(), 946684800, CALENDAR_STATUS_NOT_SIGNED_UP, CALENDAR_RANK_PLAYER, "");
        SendCalendarEventInviteAlertUnsafe(*calendarEvent, dummy);
    }

    SendCalendarEventUnsafe(player, *calendarEvent, CALENDAR_SENDTYPE_ADD);

    CharacterDatabase.CommitTransaction(trans);
}

void CalendarMgr::UpdateEvent(Player& player, uint64 eventId, CalendarEventData const& data)
{
    Lock lock(_mutex);

    if (CalendarEvent* calendarEvent = GetEventUnsafe(eventId))
    {
        time_t oldEventTime = calendarEvent->GetEventTime();

        calendarEvent->SetType(CalendarEventType(data.type));
        calendarEvent->SetFlags(data.flags);
        calendarEvent->SetEventTime(data.eventTime);
        calendarEvent->SetTimeZoneTime(data.timezoneTime); // Not sure, seems constant from the little sniffs we have
        calendarEvent->SetDungeonId(data.dungeonId);
        calendarEvent->SetTitle(data.title);
        calendarEvent->SetDescription(data.description);

        SQLTransaction trans = CharacterDatabase.BeginTransaction();

        UpdateEventInDBUnsafe(calendarEvent, trans);
        SendCalendarEventUpdateAlertUnsafe(*calendarEvent, oldEventTime);

        CharacterDatabase.CommitTransaction(trans);
    }
    else
        SendCalendarCommandResult(player, CALENDAR_ERROR_EVENT_INVALID);
}

void CalendarMgr::RemoveEvent(Player& player, uint64 eventId, uint64 inviteId)
{
    Lock lock(_mutex);

    CalendarEvent* calendarEvent = GetEventUnsafe(eventId);

    if (!calendarEvent)
    {
        SendCalendarCommandResult(player, CALENDAR_ERROR_EVENT_INVALID);
        return;
    }

    bool hasGuildPermission = calendarEvent->IsGuildAnnouncement() || calendarEvent->IsGuildEvent();
    if (hasGuildPermission)
        hasGuildPermission = calendarEvent->GetGuildId() == player.GetGuildId();

    if (!hasGuildPermission && !HasPermissionUnsafe(player.GetGUID(), inviteId, CALENDAR_RANK_MODERATOR))
    {
        SendCalendarCommandResult(player, CALENDAR_ERROR_PERMISSIONS);
        return;
    }

    RemoveEventUnsafe(player.GetGUID(), eventId);
}

void CalendarMgr::CopyEvent(Player& player, uint64 eventId, time_t eventTime)
{
    Lock lock(_mutex);

    if (CalendarEvent* oldEvent = GetEventUnsafe(eventId))
    {
        SQLTransaction trans = CharacterDatabase.BeginTransaction();

        CalendarEvent* newEvent = new CalendarEvent(*oldEvent, GetFreeEventIdUnsafe());
        newEvent->SetEventTime(time_t(eventTime));
        AddEventUnsafe(newEvent, trans);

        CalendarInviteStore invites = GetEventInvitesUnsafe(eventId);

        for (CalendarInviteStore::const_iterator itr = invites.begin(); itr != invites.end(); ++itr)
            AddInviteUnsafe(newEvent, new CalendarInvite(**itr, GetFreeInviteIdUnsafe(), newEvent->GetEventId()), trans);

        // notify guild
        if (newEvent->IsGuildAnnouncement())
        {
            CalendarInvite dummy(0, newEvent->GetEventId(), ObjectGuid::Empty, player.GetGUID(), 946684800, CALENDAR_STATUS_NOT_SIGNED_UP, CALENDAR_RANK_PLAYER, "");
            SendCalendarEventInviteAlertUnsafe(*newEvent, dummy);
        }

        SendCalendarEventUnsafe(player, *newEvent, CALENDAR_SENDTYPE_COPY);

        CharacterDatabase.CommitTransaction(trans);
        // should we change owner when somebody makes a copy of event owned by another person?
    }
    else
        SendCalendarCommandResult(player, CALENDAR_ERROR_EVENT_INVALID);
}

void CalendarMgr::AddInvite(Player& player, uint64 eventId, uint32 inviteId, ObjectGuid inviteeGuid, uint32 inviteeGuildId, bool isPreInvite, bool isGuildEvent)
{
    Lock lock(_mutex);

    if (!isPreInvite)
    {
        if (CalendarEvent* calendarEvent = GetEventUnsafe(eventId))
        {
            if (!HasPermissionUnsafe(player.GetGUID(), inviteId, CALENDAR_RANK_MODERATOR))
            {
                SendCalendarCommandResult(player, CALENDAR_ERROR_PERMISSIONS);
                return;
            }

            if (calendarEvent->IsGuildEvent() && calendarEvent->GetGuildId() == inviteeGuildId)
            {
                // we can't invite guild members to guild events
                SendCalendarCommandResult(player, CALENDAR_ERROR_NO_GUILD_INVITES);
                return;
            }

            // 946684800 is 01/01/2000 00:00:00 - default response time
            CalendarInvite* invite = new CalendarInvite(GetFreeInviteIdUnsafe(), eventId, inviteeGuid, player.GetGUID(), 946684800, CALENDAR_STATUS_INVITED, CALENDAR_RANK_PLAYER, "");
            AddInviteUnsafe(calendarEvent, invite);
        }
        else
            SendCalendarCommandResult(player, CALENDAR_ERROR_EVENT_INVALID);
    }
    else
    {
        if (isGuildEvent && inviteeGuildId == player.GetGuildId())
        {
            SendCalendarCommandResult(player, CALENDAR_ERROR_NO_GUILD_INVITES);
            return;
        }

        // 946684800 is 01/01/2000 00:00:00 - default response time
        CalendarInvite invite(inviteId, 0, inviteeGuid, player.GetGUID(), 946684800, CALENDAR_STATUS_INVITED, CALENDAR_RANK_PLAYER, "");
        SendCalendarEventInviteUnsafe(player, invite);
    }
}

void CalendarMgr::EventSignup(Player& player, uint64 eventId, bool tentative)
{
    Lock lock(_mutex);

    if (CalendarEvent* calendarEvent = GetEventUnsafe(eventId))
    {
        if (!calendarEvent->IsGuildEvent() || calendarEvent->GetGuildId() != player.GetGuildId())
        {
            SendCalendarCommandResult(player, CALENDAR_ERROR_GUILD_PLAYER_NOT_IN_GUILD);
            return;
        }

        CalendarInviteStatus status = tentative ? CALENDAR_STATUS_TENTATIVE : CALENDAR_STATUS_SIGNED_UP;
        CalendarInvite* invite = new CalendarInvite(GetFreeInviteIdUnsafe(), eventId, player.GetGUID(), player.GetGUID(), time(NULL), status, CALENDAR_RANK_PLAYER, "");
        AddInviteUnsafe(calendarEvent, invite);
        SendCalendarClearPendingActionUnsafe(player);
    }
    else
        SendCalendarCommandResult(player, CALENDAR_ERROR_EVENT_INVALID);
}

void CalendarMgr::EventSetStatus(Player& player, uint64 eventId, uint64 inviteId, uint32 status)
{
    Lock lock(_mutex);

    if (CalendarEvent* calendarEvent = GetEventUnsafe(eventId))
    {
        // i think we still should be able to remove self from locked events
        if (status != CALENDAR_STATUS_REMOVED && calendarEvent->GetFlags() & CALENDAR_FLAG_INVITES_LOCKED)
        {
            SendCalendarCommandResult(player, CALENDAR_ERROR_EVENT_LOCKED);
            return;
        }

        if (CalendarInvite* invite = GetInviteUnsafe(inviteId))
        {
            invite->SetStatus(CalendarInviteStatus(status));
            invite->SetStatusTime(time(NULL));

            UpdateInviteInDBUnsafe(invite);
            SendCalendarEventStatusUnsafe(*calendarEvent, *invite);
            SendCalendarClearPendingActionUnsafe(player);
        }
        else
            SendCalendarCommandResult(player, CALENDAR_ERROR_NO_INVITE); // correct?
    }
    else
        SendCalendarCommandResult(player, CALENDAR_ERROR_EVENT_INVALID);
}

void CalendarMgr::EventSetModeratorStatus(Player& player, uint64 eventId, uint64 senderInviteId, uint64 inviteId, uint8 rank)
{
    Lock lock(_mutex);

    if (!HasPermissionUnsafe(player.GetGUID(), senderInviteId, CALENDAR_RANK_MODERATOR))
    {
        SendCalendarCommandResult(player, CALENDAR_ERROR_PERMISSIONS);
        return;
    }

    if (CalendarEvent* calendarEvent = GetEventUnsafe(eventId))
    {
        if (CalendarInvite* invite = GetInviteUnsafe(inviteId))
        {
            invite->SetRank(CalendarModerationRank(rank));
            UpdateInviteInDBUnsafe(invite);
            SendCalendarEventModeratorStatusAlertUnsafe(*calendarEvent, *invite);
        }
        else
            SendCalendarCommandResult(player, CALENDAR_ERROR_NO_INVITE); // correct?
    }
    else
        SendCalendarCommandResult(player, CALENDAR_ERROR_EVENT_INVALID);
}

void CalendarMgr::RemoveInvite(Player& player, uint64 eventId, uint64 senderInviteId, uint64 inviteId, ObjectGuid invitee)
{
    Lock lock(_mutex);

    if (CalendarEvent* calendarEvent = GetEventUnsafe(eventId))
    {
        if (!HasPermissionUnsafe(player.GetGUID(), senderInviteId, (senderInviteId == inviteId ? CALENDAR_RANK_PLAYER : CALENDAR_RANK_MODERATOR)))
        {
            SendCalendarCommandResult(player, CALENDAR_ERROR_PERMISSIONS);
            return;
        }

        if (calendarEvent->GetCreatorGUID() == invitee)
        {
            SendCalendarCommandResult(player, CALENDAR_ERROR_DELETE_CREATOR_FAILED);
            return;
        }

        RemoveInviteUnsafe(inviteId, eventId);
    }
    else
        SendCalendarCommandResult(player, CALENDAR_ERROR_NO_INVITE);
}

uint32 CalendarMgr::GetPlayerNumPending(ObjectGuid guid)
{
    Lock lock(_mutex);

    CalendarInviteStore const& invites = GetPlayerInvitesUnsafe(guid);

    time_t now = time(nullptr);

    uint32 pendingNum = 0;
    for (CalendarInviteStore::const_iterator itr = invites.begin(); itr != invites.end(); ++itr)
    {
        if (CalendarEvent* calendarEvent = GetEventUnsafe((*itr)->GetEventId()))
        {
            if (calendarEvent->GetEventTime() < now)
                continue;

            switch ((*itr)->GetStatus())
            {
                case CALENDAR_STATUS_INVITED:
                case CALENDAR_STATUS_TENTATIVE:
                case CALENDAR_STATUS_NOT_SIGNED_UP:
                    ++pendingNum;
                    break;
                default:
                    break;
            }
        }
    }

    return pendingNum;
}

void CalendarMgr::RemovePlayerGuildEventsAndSignups(ObjectGuid guid, uint32 guildId)
{
    Lock lock(_mutex);

    {
        auto& index = _events.get<CalendarEvent::creator_guid>();
        auto itr_guid = index.find(guid);
        if (itr_guid != index.end())
        {
            std::list<uint32> eventsToRemove;

            auto range = index.equal_range(index.key_extractor()(*itr_guid));
            for (auto itr_event = range.first; itr_event != range.second; ++itr_event)
                if ((*itr_event)->IsGuildEvent() || (*itr_event)->IsGuildAnnouncement())
                    eventsToRemove.push_back((*itr_event)->GetEventId());

            for(uint32 eventId : eventsToRemove)
                RemoveEventUnsafe(guid, eventId);
        }
    }

    CalendarInviteStore playerInvites = GetPlayerInvitesUnsafe(guid);
    for (CalendarInviteStore::const_iterator itr = playerInvites.begin(); itr != playerInvites.end(); ++itr)
        if (CalendarEvent* calendarEvent = GetEventUnsafe((*itr)->GetEventId()))
            if (calendarEvent->IsGuildEvent() && calendarEvent->GetGuildId() == guildId)
                RemoveInviteUnsafe((*itr)->GetInviteId(), (*itr)->GetEventId());
}

void CalendarMgr::RemoveAllPlayerEventsAndInvites(ObjectGuid guid)
{
    // delete events
    {
        auto& index = _events.get<CalendarEvent::creator_guid>();
        auto itr_guid = index.find(guid);
        if (itr_guid != index.end())
        {
            auto range = index.equal_range(index.key_extractor()(*itr_guid));
            for (auto itr_event = range.first; itr_event != range.second; ++itr_event)
                RemoveEventUnsafe(ObjectGuid(), (*itr_event)->GetEventId()); // don't send mail if removing a character
        }
    }

    CalendarInviteStore playerInvites = GetPlayerInvitesUnsafe(guid);
    for (CalendarInviteStore::const_iterator itr = playerInvites.begin(); itr != playerInvites.end(); ++itr)
        RemoveInviteUnsafe((*itr)->GetInviteId(), (*itr)->GetEventId());
}
