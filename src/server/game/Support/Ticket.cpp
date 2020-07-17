#include "Support/Ticket.hpp"

#include "Packets/WorldPacket.h"

#include "Chat/Chat.h"
#include "Entities/Player/PlayerCache.hpp"
#include "Globals/ObjectAccessor.h"
#include "Scripting/ScriptMgr.h"
#include "Server/WorldSession.h"
#include "Server/Protocol/Opcodes.h"

#include "Support/TicketMgr.hpp"

inline float GetAge(TimePoint t)
{
    return static_cast<float>(duration_cast<Seconds>(SystemClock::now() - t).count()) / DAY;
}

namespace support
{

    Ticket::Ticket(Id id, Type type, bool isFollowUpTicket, const WorldLocation& pos, ObjectGuid author, std::string message, TicketMgr& callbacks) :
        id(id),
        type(type),
        isFollowUpTicket(isFollowUpTicket),
        author(author),
        position(pos),
        message(std::move(message)),
        callbacks(callbacks)
    {}

    Ticket::Ticket(
        Id id, Type type, bool isFollowUpTicket, const WorldLocation& position, TimePoint creationTime, ObjectGuid author,
        bool viewed, bool completed, bool escalated, boost::optional<AccountId> assignee,
        std::string message, std::string comment, std::string response, TicketMgr& callbacks
    ) :
        id(id), type(type), isFollowUpTicket(isFollowUpTicket), position(position), creationTime(creationTime), author(author),
        viewed(viewed), completed(completed), escalated(escalated), assignee(assignee),
        message(std::move(message)), comment(std::move(comment)), response(std::move(response)), callbacks(callbacks)
    {}

    boost::optional<Player&> Ticket::GetAuthor() const
    {
        if (Player* p = ObjectAccessor::FindPlayer(GetAuthorGUID()))
            return boost::optional<Player&>(*p);
        else
            return boost::none;
    }

    void Ticket::SetViewed(AccountId causer, bool persistent)
    {
        if (IsViewed())
            return;

        viewed = true;

        OnUpdate(persistent, persistent);

        sScriptMgr->OnTicketFirstViewed(*this, causer);
    }

    void Ticket::Complete(AccountId causer, bool persistent)
    {
        if (IsCompleted())
            return;

        completed = true;

        callbacks.OnTicketCompleted(GetId());

        OnUpdate(persistent, persistent);

        sScriptMgr->OnTicketComplete(*this, causer);
    }

    void Ticket::Escalate(AccountId causer, bool persistent)
    {
        if (IsEscalated())
            return;

        escalated = true;

        OnUpdate(persistent, persistent);

        sScriptMgr->OnTicketEscalate(*this, causer);
    }

    void Ticket::AssignTo(AccountId assignee, AccountId causer, bool persistent)
    {
        if (GetAssigneeId() == assignee)
            return;

        this->assignee = assignee;

        OnUpdate(persistent, persistent);

        sScriptMgr->OnTicketAssign(*this, causer);
    }

    void Ticket::Unassign(AccountId causer, bool persistent)
    {
        if (!IsAssigned())
            return;

        assignee = boost::none;

        OnUpdate(persistent, persistent);

        sScriptMgr->OnTicketUnassign(*this, causer);
    }

    void Ticket::Close(AccountId closer, bool persistent)
    {
        if (IsClosed())
            return;

        this->closer = closer;

        OnUpdate(persistent, false);

        if (persistent)
        {
            if (const auto author = GetAuthor())
            {
                WorldPacket packet(SMSG_GMTICKET_DELETETICKET, 4);
                packet << uint32(GMTICKET_RESPONSE_TICKET_DELETED);
                author->SendDirectMessage(std::move(packet));
            }
        }

        sScriptMgr->OnTicketClose(*this, closer);

        callbacks.OnTicketClosed(GetId()); // note: the ticket is delete in here
    }

    void Ticket::SetMessage(std::string message, AccountId causer, bool persistent)
    {
        this->message = std::move(message);

        OnUpdate(persistent, false);

        sScriptMgr->OnTicketUpdateMessage(*this, causer);
    }

    void Ticket::SetComment(std::string comment, AccountId causer, bool persistent)
    {
        this->comment = std::move(comment);

        OnUpdate(persistent, false);

        sScriptMgr->OnTicketUpdateComment(*this, causer);
    }

    /*void Ticket::Respond(std::string response, AccountId causer, bool persistent)
    {
        this->response = std::move(response);

        OnUpdate(persistent, persistent);

        sScriptMgr->OnTicketRespond(*this, causer);
    }*/

    void Ticket::AppendResponse(const std::string& response, AccountId causer, bool persistent)
    {
        this->response += response;

        // temporary disabled to prevent race condition of db update queries when executing
        // ".ticket response append(ln)" and ".ticket complete" in direct succession
        // OnUpdate(persistent, persistent);

        sScriptMgr->OnTicketUpdateResponse(*this, causer);
    }

    Ticket Ticket::LoadFromDB(Field* fields, TicketMgr& callbacks)
    {
        return Ticket(
            fields[0].GetUInt32(),
            static_cast<Ticket::Type>(fields[1].GetUInt8()),
            fields[2].GetBool(),
            WorldLocation{ fields[3].GetUInt16(), fields[4].GetFloat(), fields[5].GetFloat(), fields[6].GetFloat() },
            FromUnixTime(fields[7].GetInt64()),
            ObjectGuid{ HighGuid::Player, fields[8].GetUInt32() },
            fields[9].GetBool(),
            fields[10].GetBool(),
            fields[11].GetBool(),
            fields[12].IsNull() ? boost::none : boost::optional<AccountId>(fields[12].GetUInt32()),
            fields[13].GetString(),
            fields[14].GetString(),
            fields[15].GetString(),
            callbacks
        );
    }

    void Ticket::SaveToDB(SQLTransaction trans) const
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_TICKET);

        stmt->setUInt32(0, GetId());
        stmt->setUInt8(1, static_cast<uint8>(type));
        stmt->setBool(2, IsFollowUpTicket());
        stmt->setUInt16(3, GetPosition().GetMapId());
        stmt->setFloat(4, GetPosition().GetPositionX());
        stmt->setFloat(5, GetPosition().GetPositionY());
        stmt->setFloat(6, GetPosition().GetPositionZ());
        stmt->setInt64(7, ToUnixTime(GetCreationTime()));

        stmt->setUInt32(8, GetAuthorGUID().GetCounter());
        stmt->setString(9, player::PlayerCache::GetName(GetAuthorGUID()));

        stmt->setBool(10, IsViewed());
        stmt->setBool(11, IsCompleted());
        stmt->setBool(12, IsEscalated());

        if (IsAssigned())
            stmt->setUInt32(13, *GetAssigneeId());
        else
            stmt->setNull(13);

        if (IsClosed())
            stmt->setUInt32(14, *GetCloserId());
        else
            stmt->setNull(14);

        stmt->setString(15, GetMessage());
        stmt->setString(16, GetComment());
        stmt->setString(17, GetResponse());

        CharacterDatabase.ExecuteOrAppend(trans, stmt);
    }

    void Ticket::SendNoTicketTo(WorldSession& session)
    {
        WorldPacket packet(SMSG_GMTICKET_GETTICKET, 4);
        packet << uint32(GMTICKET_STATUS_DEFAULT);
        session.SendPacket(std::move(packet));
    }

    void Ticket::SendTo(WorldSession& session) const
    {
        ASSERT(!IsClosed());

        if (IsCompleted())
        {
            WorldPacket packet(SMSG_GMRESPONSE_RECEIVED);
            packet << uint32(1); // response id
            packet << uint32(GetId());
            packet << message.c_str();

            // response
            {
                size_t len = response.size();
                const char* s = response.c_str();

                for (int i = 0; i < 4; i++)
                {
                    if (len > 0)
                    {
                        const size_t writeLen = std::min<size_t>(len, 3999);
                        packet.append(s, writeLen);

                        len -= writeLen;
                        s += writeLen;
                    }
                    
                    packet << uint8(0);
                }
            }

            session.SendPacket(std::move(packet));
        }
        else
        {
            WorldPacket packet(SMSG_GMTICKET_GETTICKET);
            packet << uint32(GMTICKET_STATUS_HASTEXT);
            packet << uint32(GetId());
            packet << message;
            packet << uint8(isFollowUpTicket);

            packet << GetAge(creationTime);
            packet << GetAge(callbacks.GetOldestTicketCreationTime());
            packet << GetAge(callbacks.GetLastGMInteractionTime());

            packet << uint8(GetEscalationStatus());

            // we check for assigned or escalated here instead of viewed as we don't want the client to show "Ticket will be serviced soon." after a GM viewed the ticket
            packet << uint8((IsAssigned() || IsEscalated()) ? GMTICKET_OPENEDBYGM_STATUS_OPENED : GMTICKET_OPENEDBYGM_STATUS_NOT_OPENED);

            session.SendPacket(std::move(packet));
        }
    }

    void Ticket::SendToAuthor() const
    {
        if (const auto author = GetAuthor())
            SendTo(*author->GetSession());
    }

    void Ticket::OnUpdate(bool saveToDB, bool sendToAuthor) const
    {
        if (saveToDB)
            SaveToDB();

        if (sendToAuthor)
            SendToAuthor();
    }
}
