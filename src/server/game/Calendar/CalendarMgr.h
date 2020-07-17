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

#ifndef TRINITY_CALENDARMGR_H
#define TRINITY_CALENDARMGR_H

#include "Common.h"
#include "DatabaseEnv.h"
#include "ObjectGuid.h"
#include "Player.h"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/tag.hpp>

enum CalendarMailAnswers
{
    // else
    CALENDAR_EVENT_REMOVED_MAIL_SUBJECT     = 0,
    // if ( *(_DWORD *)(a1 + 8292) & 0x100 )
    CALENDAR_INVITE_REMOVED_MAIL_SUBJECT    = 0x100
};

enum CalendarFlags
{
    CALENDAR_FLAG_ALL_ALLOWED       = 0x001,
    CALENDAR_FLAG_INVITES_LOCKED    = 0x010,
    CALENDAR_FLAG_WITHOUT_INVITES   = 0x040,
    CALENDAR_FLAG_GUILD_EVENT       = 0x400
};

enum CalendarModerationRank
{
    CALENDAR_RANK_PLAYER            = 0,
    CALENDAR_RANK_MODERATOR         = 1,
    CALENDAR_RANK_OWNER             = 2
};

enum CalendarSendEventType
{
    CALENDAR_SENDTYPE_GET           = 0,
    CALENDAR_SENDTYPE_ADD           = 1,
    CALENDAR_SENDTYPE_COPY          = 2
};

enum CalendarEventType : uint8
{
    CALENDAR_TYPE_RAID              = 0,
    CALENDAR_TYPE_DUNGEON           = 1,
    CALENDAR_TYPE_PVP               = 2,
    CALENDAR_TYPE_MEETING           = 3,
    CALENDAR_TYPE_OTHER             = 4
};

enum CalendarRepeatType
{
    CALENDAR_REPEAT_NEVER           = 0,
    CALENDAR_REPEAT_WEEKLY          = 1,
    CALENDAR_REPEAT_BIWEEKLY        = 2,
    CALENDAR_REPEAT_MONTHLY         = 3
};

enum CalendarInviteStatus
{
    CALENDAR_STATUS_INVITED         = 0,
    CALENDAR_STATUS_ACCEPTED        = 1,
    CALENDAR_STATUS_DECLINED        = 2,
    CALENDAR_STATUS_CONFIRMED       = 3,
    CALENDAR_STATUS_OUT             = 4,
    CALENDAR_STATUS_STANDBY         = 5,
    CALENDAR_STATUS_SIGNED_UP       = 6,
    CALENDAR_STATUS_NOT_SIGNED_UP   = 7,
    CALENDAR_STATUS_TENTATIVE       = 8,
    CALENDAR_STATUS_REMOVED         = 9     // correct name?
};

enum CalendarError
{
    CALENDAR_OK                                 = 0,
    CALENDAR_ERROR_GUILD_EVENTS_EXCEEDED        = 1,
    CALENDAR_ERROR_EVENTS_EXCEEDED              = 2,
    CALENDAR_ERROR_SELF_INVITES_EXCEEDED        = 3,
    CALENDAR_ERROR_OTHER_INVITES_EXCEEDED       = 4,
    CALENDAR_ERROR_PERMISSIONS                  = 5,
    CALENDAR_ERROR_EVENT_INVALID                = 6,
    CALENDAR_ERROR_NOT_INVITED                  = 7,
    CALENDAR_ERROR_INTERNAL                     = 8,
    CALENDAR_ERROR_GUILD_PLAYER_NOT_IN_GUILD    = 9,
    CALENDAR_ERROR_ALREADY_INVITED_TO_EVENT_S   = 10,
    CALENDAR_ERROR_PLAYER_NOT_FOUND             = 11,
    CALENDAR_ERROR_NOT_ALLIED                   = 12,
    CALENDAR_ERROR_IGNORING_YOU_S               = 13,
    CALENDAR_ERROR_INVITES_EXCEEDED             = 14,
    CALENDAR_ERROR_INVALID_DATE                 = 16,
    CALENDAR_ERROR_INVALID_TIME                 = 17,

    CALENDAR_ERROR_NEEDS_TITLE                  = 19,
    CALENDAR_ERROR_EVENT_PASSED                 = 20,
    CALENDAR_ERROR_EVENT_LOCKED                 = 21,
    CALENDAR_ERROR_DELETE_CREATOR_FAILED        = 22,
    CALENDAR_ERROR_SYSTEM_DISABLED              = 24,
    CALENDAR_ERROR_RESTRICTED_ACCOUNT           = 25,
    CALENDAR_ERROR_ARENA_EVENTS_EXCEEDED        = 26,
    CALENDAR_ERROR_RESTRICTED_LEVEL             = 27,
    CALENDAR_ERROR_USER_SQUELCHED               = 28,
    CALENDAR_ERROR_NO_INVITE                    = 29,

    CALENDAR_ERROR_EVENT_WRONG_SERVER           = 36,
    CALENDAR_ERROR_INVITE_WRONG_SERVER          = 37,
    CALENDAR_ERROR_NO_GUILD_INVITES             = 38,
    CALENDAR_ERROR_INVALID_SIGNUP               = 39,
    CALENDAR_ERROR_NO_MODERATOR                 = 40
};

#define CALENDAR_MAX_EVENTS         30
#define CALENDAR_MAX_GUILD_EVENTS   100
#define CALENDAR_MAX_INVITES        100

struct CalendarEventData
{
    ObjectGuid creatorGUID;
    uint32 guildId;
    CalendarEventType type;
    int32 dungeonId;
    time_t eventTime;
    uint32 flags;
    time_t timezoneTime;
    std::string title;
    std::string description;

    struct Invite
    {
        ObjectGuid invitee;
        uint8 status;
        uint8 rank;
    };

    std::vector<Invite> invitations;
};

struct CalendarInvite
{
    struct id{};
    struct event_id{};
    struct invitee_guid{};

    public:
        CalendarInvite(CalendarInvite const& calendarInvite, uint64 inviteId, uint64 eventId)
        {
            _inviteId = inviteId;
            _eventId = eventId;
            _invitee = calendarInvite.GetInviteeGUID();
            _senderGUID = calendarInvite.GetSenderGUID();
            _statusTime = calendarInvite.GetStatusTime();
            _status = calendarInvite.GetStatus();
            _rank = calendarInvite.GetRank();
            _text = calendarInvite.GetText();
        }

        CalendarInvite() : _inviteId(1), _eventId(0), _invitee(), _senderGUID(), _statusTime(time(NULL)),
            _status(CALENDAR_STATUS_INVITED), _rank(CALENDAR_RANK_PLAYER), _text("") { }

        CalendarInvite(uint64 inviteId, uint64 eventId, ObjectGuid invitee, ObjectGuid senderGUID, time_t statusTime,
            CalendarInviteStatus status, CalendarModerationRank rank, std::string text) :
            _inviteId(inviteId), _eventId(eventId), _invitee(invitee), _senderGUID(senderGUID), _statusTime(statusTime),
            _status(status), _rank(rank), _text(text) { }
        
        uint64 GetInviteId() const { return _inviteId; }

        uint64 GetEventId() const { return _eventId; }

        void SetSenderGUID(ObjectGuid guid) { _senderGUID = guid; }
        ObjectGuid GetSenderGUID() const { return _senderGUID; }

        ObjectGuid GetInviteeGUID() const { return _invitee; }

        void SetStatusTime(time_t statusTime) { _statusTime = statusTime; }
        time_t GetStatusTime() const { return _statusTime; }

        void SetText(const std::string& text) { _text = text; }
        std::string GetText() const { return _text; }

        void SetStatus(CalendarInviteStatus status) { _status = status; }
        CalendarInviteStatus GetStatus() const { return _status; }

        void SetRank(CalendarModerationRank rank) { _rank = rank; }
        CalendarModerationRank GetRank() const { return _rank; }

    private:
        uint64 _inviteId;
        uint64 _eventId;
        ObjectGuid _invitee;
        ObjectGuid _senderGUID;
        time_t _statusTime;
        CalendarInviteStatus _status;
        CalendarModerationRank _rank;
        std::string _text;
};

struct CalendarEvent
{
    struct id{};
    struct creator_guid{};
    struct guild_id{};

    public:
        CalendarEvent(CalendarEvent const& calendarEvent, uint64 eventId)
        {
            _eventId = eventId;
            _creatorGUID = calendarEvent.GetCreatorGUID();
            _guildId = calendarEvent.GetGuildId();
            _type = calendarEvent.GetType();
            _dungeonId = calendarEvent.GetDungeonId();
            _eventTime = calendarEvent.GetEventTime();
            _flags = calendarEvent.GetFlags();
            _timezoneTime = calendarEvent.GetTimeZoneTime();
            _title = calendarEvent.GetTitle();
            _description = calendarEvent.GetDescription();
        }

        CalendarEvent(uint64 eventId, ObjectGuid creatorGUID, uint32 guildId, CalendarEventType type, int32 dungeonId,
            time_t eventTime, uint32 flags, time_t timezoneTime, std::string title, std::string description) :
            _eventId(eventId), _creatorGUID(creatorGUID), _guildId(guildId), _type(type), _dungeonId(dungeonId),
            _eventTime(eventTime), _flags(flags), _timezoneTime(timezoneTime), _title(title),
            _description(description) { }

        CalendarEvent(CalendarEventData const& data, uint64 eventId) : _eventId(eventId), _creatorGUID(data.creatorGUID), 
            _guildId(data.guildId), _type(data.type), _dungeonId(data.dungeonId), _eventTime(data.eventTime), _flags(data.flags), 
            _timezoneTime(data.timezoneTime), _title(data.title), _description(data.description) { }

        CalendarEvent() : _eventId(1), _creatorGUID(), _guildId(0), _type(CALENDAR_TYPE_OTHER), _dungeonId(-1), _eventTime(0),
            _flags(0), _timezoneTime(0), _title(""), _description("") { }

        uint64 GetEventId() const { return _eventId; }

        ObjectGuid GetCreatorGUID() const { return _creatorGUID; }

        ///! MUST NOT be called after event is registered in CalendarMgr, otherwise lookup index gets corrupted
        void SetGuildId(uint32 guildId) { _guildId = guildId; }
        uint32 GetGuildId() const { return _guildId; }

        void SetTitle(const std::string& title) { _title = title; }
        std::string GetTitle() const { return _title; }

        void SetDescription(const std::string& description) { _description = description; }
        std::string GetDescription() const { return _description; }

        void SetType(CalendarEventType type) { _type = type; }
        CalendarEventType GetType() const { return _type; }

        void SetDungeonId(int32 dungeonId) { _dungeonId = dungeonId; }
        int32 GetDungeonId() const { return _dungeonId; }

        void SetEventTime(time_t eventTime) { _eventTime = eventTime; }
        time_t GetEventTime() const { return _eventTime; }

        void SetFlags(uint32 flags) { _flags = flags; }
        uint32 GetFlags() const { return _flags; }

        void SetTimeZoneTime(time_t timezoneTime) { _timezoneTime = timezoneTime; }
        time_t GetTimeZoneTime() const { return _timezoneTime; }

        bool IsGuildEvent() const { return (_flags & CALENDAR_FLAG_GUILD_EVENT) != 0; }
        bool IsGuildAnnouncement() const { return (_flags & CALENDAR_FLAG_WITHOUT_INVITES) != 0; }

        std::string BuildCalendarMailSubject(ObjectGuid remover) const;
        std::string BuildCalendarMailBody() const;

    private:
        uint64 _eventId;
        ObjectGuid _creatorGUID;
        uint32 _guildId;
        CalendarEventType _type;
        int32 _dungeonId;
        time_t _eventTime;
        uint32 _flags;
        time_t _timezoneTime;
        std::string _title;
        std::string _description;
};

typedef boost::multi_index::multi_index_container<
            CalendarEvent*,
            boost::multi_index::indexed_by<
                boost::multi_index::ordered_unique<
                    boost::multi_index::tag<CalendarEvent>,
                    boost::multi_index::identity<CalendarEvent*>
                >,
                boost::multi_index::ordered_unique<
                    boost::multi_index::tag<CalendarEvent::id>,
                    boost::multi_index::const_mem_fun<CalendarEvent, uint64, &CalendarEvent::GetEventId>
                >,
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::tag<CalendarEvent::creator_guid>,
                    boost::multi_index::const_mem_fun<CalendarEvent, ObjectGuid, &CalendarEvent::GetCreatorGUID>
                >,
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::tag<CalendarEvent::guild_id>,
                    boost::multi_index::const_mem_fun<CalendarEvent, uint32, &CalendarEvent::GetGuildId>
                >
            >
        > CalendarEventStore;

typedef boost::multi_index::multi_index_container<
            CalendarInvite*,
            boost::multi_index::indexed_by<
                boost::multi_index::ordered_unique<
                    boost::multi_index::tag<CalendarInvite>,
                    boost::multi_index::identity<CalendarInvite*>
                >,
                boost::multi_index::ordered_unique<
                    boost::multi_index::tag<CalendarInvite::id>,
                    boost::multi_index::const_mem_fun<CalendarInvite, uint64, &CalendarInvite::GetInviteId>
                >,
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::tag<CalendarInvite::event_id>,
                    boost::multi_index::const_mem_fun<CalendarInvite, uint64, &CalendarInvite::GetEventId>
                >,
                boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<CalendarInvite::invitee_guid>,
                    boost::multi_index::const_mem_fun<CalendarInvite, ObjectGuid, &CalendarInvite::GetInviteeGUID>
                >
            >
        > CalendarInviteStore;

class GAME_API CalendarMgr
{
    private:
        using Mutex = std::mutex;
        using Lock = std::lock_guard<Mutex>;

        CalendarMgr();
        ~CalendarMgr();

        CalendarEventStore _events;
        CalendarInviteStore _invites;

        std::deque<uint64> _freeEventIds;
        std::deque<uint64> _freeInviteIds;
        uint64 _maxEventId;
        uint64 _maxInviteId;

        Mutex _mutex;

    public:
        static CalendarMgr* instance();

        void LoadFromDB();

        void AddEvent(Player& player, CalendarEventData const& data);
        void UpdateEvent(Player& player, uint64 eventId, CalendarEventData const& data);
        void CopyEvent(Player& player, uint64 eventId, time_t eventTime);
        void RemoveEvent(Player& player, uint64 eventId, uint64 inviteId);

        void RemoveAllPlayerEventsAndInvites(ObjectGuid guid);
        void RemovePlayerGuildEventsAndSignups(ObjectGuid guid, uint32 guildId);

        void AddInvite(Player& player, uint64 eventId, uint32 inviteId, ObjectGuid inviteeGuid, uint32 inviteeGuildId, bool isPreInvite, bool isGuildEvent);
        void EventSignup(Player& player, uint64 eventId, bool tentative);
        void EventSetStatus(Player& player, uint64 eventId, uint64 inviteId, uint32 status);
        void EventSetModeratorStatus(Player& player, uint64 eventId, uint64 senderInviteId, uint64 inviteId, uint8 rank);
        void RemoveInvite(Player& player, uint64 eventId, uint64 senderInviteId, uint64 inviteId, ObjectGuid invitee);

        void SendEventData(Player& player, uint64 eventId);
        static void SendCalendarCommandResult(Player& player, CalendarError err, char const* param = NULL);

        void WritePlayerData(ObjectGuid guid, ByteBuffer& buffer);
        uint32 GetPlayerNumPending(ObjectGuid guid);

    private:
        void FreeEventIdUnsafe(uint64 id);
        void FreeInviteIdUnsafe(uint64 id);

        CalendarEvent* GetEventUnsafe(uint64 eventId) const;
        CalendarEventStore GetPlayerEventsUnsafe(ObjectGuid guid);

        CalendarInvite* GetInviteUnsafe(uint64 inviteId) const;
        CalendarInviteStore GetEventInvitesUnsafe(uint64 eventId);
        CalendarInviteStore GetPlayerInvitesUnsafe(ObjectGuid guid);

        uint64 GetFreeEventIdUnsafe();
        uint64 GetFreeInviteIdUnsafe();

        void AddEventUnsafe(CalendarEvent* calendarEvent, SQLTransaction& trans);
        void UpdateEventInDBUnsafe(CalendarEvent* calendarEvent, SQLTransaction& trans);

        void AddInviteUnsafe(CalendarEvent* calendarEvent, CalendarInvite* invite);
        void AddInviteUnsafe(CalendarEvent* calendarEvent, CalendarInvite* invite, SQLTransaction& trans);
        void RemoveInviteUnsafe(uint64 inviteId, uint64 eventId);
        void UpdateInviteInDBUnsafe(CalendarInvite* invite);
        void UpdateInviteInDBUnsafe(CalendarInvite* invite, SQLTransaction& trans);

        void SendCalendarEventUnsafe(Player& player, CalendarEvent const& calendarEvent, CalendarSendEventType sendType);
        void SendCalendarEventInviteUnsafe(Player& player, CalendarInvite const& invite);
        void SendCalendarEventInviteUnsafe(CalendarEvent* calendarEvent, CalendarInvite const& invite);
        static void WriteCalendarEventInviteUnsafe(ByteBuffer& buffer, bool hasStatusTime, CalendarInvite const& invite);
        void SendCalendarEventInviteAlertUnsafe(CalendarEvent const& calendarEvent, CalendarInvite const& invite);
        void SendCalendarEventInviteRemoveUnsafe(CalendarEvent const& calendarEvent, CalendarInvite const& invite, uint32 flags);
        void SendCalendarEventInviteRemoveAlertUnsafe(ObjectGuid guid, CalendarEvent const& calendarEvent, CalendarInviteStatus status);
        void SendCalendarEventUpdateAlertUnsafe(CalendarEvent const& calendarEvent, time_t oldEventTime);
        void SendCalendarEventStatusUnsafe(CalendarEvent const& calendarEvent, CalendarInvite const& invite);
        void SendCalendarEventRemovedAlertUnsafe(CalendarEvent const& calendarEvent);
        void SendCalendarEventModeratorStatusAlertUnsafe(CalendarEvent const& calendarEvent, CalendarInvite const& invite);
        static void SendCalendarClearPendingActionUnsafe(Player& player);

        void SendPacketToAllEventRelativesUnsafe(const WorldPacket& packet, CalendarEvent const& calendarEvent);

        void RemoveEventUnsafe(ObjectGuid guid, uint64 eventId);

        bool HasPermissionUnsafe(ObjectGuid guid, uint32 inviteId, CalendarModerationRank minRank);
};

#define sCalendarMgr CalendarMgr::instance()

#endif
