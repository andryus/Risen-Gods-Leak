#pragma once

#include <boost/optional.hpp>

#include "Define.h"
#include "Utilities/Duration.h"

#include "Database/Transaction.h"

#include "Entities/Object/ObjectGuid.h"
#include "Entities/Player/Player.h"
#include "Entities/WorldObject/WorldLocation.h"

#include "Support/Define.hpp"

class ChatHandler;
class WorldSession;

namespace support
{

    class TicketMgr;
    
    class GAME_API Ticket
    {
    public:
        using Id = uint32;

        enum class Type : uint8
        {
            BugReport = 1,
            HelpRequest = 17,
        };

    private:
        const Id id;
        const Type type;
        const bool isFollowUpTicket;
        const WorldLocation position;
        const TimePoint creationTime = SystemClock::now();
        const ObjectGuid author;

        bool viewed = false;
        bool completed = false;
        bool escalated = false;
        boost::optional<AccountId> assignee = boost::none;
        boost::optional<AccountId> closer = boost::none;

        std::string message;
        std::string comment{};
        std::string response{};

        TicketMgr& callbacks; // stored to remove the dependency on the singleton implementation

    public:
        Ticket(Id id, Type type, bool isFollowUpTicket, const WorldLocation& pos, ObjectGuid author, std::string message, TicketMgr& callbacks);
        Ticket(
            Id id, Type type, bool isFollowUpTicket, const WorldLocation& position, TimePoint creationTime, ObjectGuid author,
            bool viewed, bool completed, bool escalated, boost::optional<AccountId> assignee,
            std::string message, std::string comment, std::string response, TicketMgr& callbacks
        );

        Id GetId() const { return id; }
        Type GetType() const { return type; }
        bool IsFollowUpTicket() const { return isFollowUpTicket; }
        const WorldLocation& GetPosition() const { return position; }
        TimePoint GetCreationTime() const { return creationTime; }

        ObjectGuid GetAuthorGUID() const { return author; }
        boost::optional<Player&> GetAuthor() const;

        bool IsViewed() const { return viewed; }
        void SetViewed(AccountId causer, bool persistent = true);

        bool IsCompleted() const { return completed; }
        void Complete(AccountId causer, bool persistent = true);

        bool IsEscalated() const { return escalated; }
        void Escalate(AccountId causer, bool persistent = true);

        bool IsAssigned() const { return static_cast<bool>(assignee); }
        boost::optional<AccountId> GetAssigneeId() const { return assignee; }
        void AssignTo(AccountId assignee, AccountId causer, bool persistent = true);
        void Unassign(AccountId causer, bool persistent = true);

        bool IsClosed() const { return static_cast<bool>(closer); }
        boost::optional<AccountId> GetCloserId() const { return closer; }
        void Close(AccountId closer, bool persistent = true);

        const std::string& GetMessage() const { return message; }
        void SetMessage(std::string message, AccountId causer, bool persistent = true);

        const std::string& GetComment() const { return comment; }
        void SetComment(std::string comment, AccountId causer, bool persistent = true);

        const std::string& GetResponse() const { return response; }
        //void Respond(std::string response, AccountId causer, bool persistent = true);
        void AppendResponse(const std::string& response, AccountId causer, bool persistent = true);

        static Ticket LoadFromDB(Field* fields, TicketMgr& callbacks);
        void SaveToDB(SQLTransaction trans = nullptr) const;

        static void SendNoTicketTo(WorldSession& session);
        void SendTo(WorldSession& session) const;
        void SendToAuthor() const;

    private:
        void OnUpdate(bool saveToDB, bool sendToAuthor) const;

        GMTicketEscalationStatus GetEscalationStatus() const
        {
            if (IsAssigned())
                return TICKET_ASSIGNED;
            else
                return IsEscalated() ? TICKET_IN_ESCALATION_QUEUE : TICKET_UNASSIGNED;
        }
    };

}
