#include "API/Provider/Providers.hpp"

#include "Support/Ticket.hpp"
#include "Support/TicketMgr.hpp"

#include "API/ResponseBuilder.hpp"
#include "API/JsonSerializers.hpp"

namespace api::providers
{

    Ticket::FutureResultType Ticket::Execute(support::Ticket::Id id, std::shared_ptr<const json> requestedValues, std::shared_ptr<const Request> request) const
    {
        ResponseBuilder response(requestedValues, request);

        ObjectGuid authorGUID;
        boost::optional<AccountId> assigneeId;
        boost::optional<AccountId> closerId;

        const bool found = support::sTicketMgr.withLock([&](support::TicketMgr& ticketMgr) -> bool
        {
            const auto ticket = ticketMgr.GetOpenTicket(id);
            if (!ticket)
                return false;

            response.set("id",              id,                                     rbac::PERM_API_TICKET_BASIC);
            response.set("type",            ticket->GetType(),                      rbac::PERM_API_TICKET_BASIC);
            response.set("is_follow_up",    ticket->IsFollowUpTicket(),             rbac::PERM_API_TICKET_BASIC);
            response.set("position",        ticket->GetPosition(),                  rbac::PERM_API_TICKET_BASIC);
            response.set("creation_time",   ToUnixTime(ticket->GetCreationTime()),  rbac::PERM_API_TICKET_BASIC);

            response.set("is_viewed",       ticket->IsViewed(),     rbac::PERM_API_TICKET_BASIC);
            response.set("is_completed",    ticket->IsCompleted(),  rbac::PERM_API_TICKET_BASIC);
            response.set("is_escalated",    ticket->IsEscalated(),  rbac::PERM_API_TICKET_BASIC);
            response.set("is_assigned",     ticket->IsAssigned(),   rbac::PERM_API_TICKET_BASIC);
            response.set("is_closed",       ticket->IsClosed(),     rbac::PERM_API_TICKET_BASIC);

            response.set("message",     ticket->GetMessage(),   rbac::PERM_API_TICKET_BASIC);
            response.set("comment",     ticket->GetComment(),   rbac::PERM_API_TICKET_BASIC);
            response.set("response",    ticket->GetResponse(),  rbac::PERM_API_TICKET_BASIC);

            authorGUID = ticket->GetAuthorGUID();
            assigneeId = ticket->GetAssigneeId();
            closerId = ticket->GetCloserId();

            return true;
        });

        if (!found)
            return boost::none;

        auto author = MakeProvider<Character>(requestedValues, "author", request)(authorGUID);
        auto assignee = MakeProvider<Account>(requestedValues, "assignee", request)(assigneeId);
        auto closer = MakeProvider<Account>(requestedValues, "closer", request)(closerId);

        return folly::collect(author, assignee, closer)
            .then([response = std::move(response)](std::tuple<Character::ResultType, Character::ResultType, Character::ResultType> t) mutable->ResultType
            {
                auto&[author, assignee, closer] = t;

                response.setOptional("author",      std::move(author),      rbac::PERM_API_TICKET_BASIC);
                response.setOptional("assignee",    std::move(assignee),    rbac::PERM_API_TICKET_BASIC);
                response.setOptional("closer",      std::move(closer),      rbac::PERM_API_TICKET_BASIC);

                return response.build();
            });
    }
    
}
