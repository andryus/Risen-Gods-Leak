#include <algorithm>
#include <iterator>

#include <boost/range/algorithm.hpp>

#include "util/adaptor/Adaptors.hpp"

#include "Entities/Player/Player.h"
#include "Scripting/ScriptMgr.h"
#include "Support/Ticket.hpp"
#include "Support/TicketMgr.hpp"

#include "API/Addon/Endpoint.hpp"
#include "API/ResponseBuilder.hpp"

#include "API/Handler/Includes.hpp" // NOTE: this must always be the last include

folly::Future<json> HandleTicketShow(std::shared_ptr<const Request> request)
{
    const json& params = request->parameters;

    const uint32 id = Get<uint32>(params, "id");

    return providers::Ticket(ExtractRequestedValues(request), request)(id)
        .then([](providers::Ticket::ResultType result) -> json
        {
            if (!result)
                throw APIException("ticket not found");

            return std::move(*result);
        });
}

folly::Future<json> HandleTicketList(std::shared_ptr<const Request> request)
{
    const json& params = request->parameters;

    ResponseBuilder response(ExtractRequestedValues(request), request);

    const size_t maxCount = GetOptional<size_t>(params, "maxCount", -1);

    boost::optional<bool> filterViewed    = boost::none;
    boost::optional<bool> filterEscalated = boost::none;
    boost::optional<bool> filterAssigned  = boost::none;
    boost::optional<bool> filterCompleted = boost::none;
    boost::optional<bool> filterClosed    = boost::none;
    boost::optional<bool> filterOnline    = boost::none;
    if (params.has("filter"))
    {
        const json& filter = params["filter"];

        filterViewed    = GetOptional<bool>(filter, "is_viewed");
        filterEscalated = GetOptional<bool>(filter, "is_escalated");
        filterAssigned  = GetOptional<bool>(filter, "is_assigned");
        filterCompleted = GetOptional<bool>(filter, "is_completed");
        filterClosed    = GetOptional<bool>(filter, "is_closed");
        filterOnline    = GetOptional<bool>(filter, "is_author_online");
    }

    std::vector<support::Ticket::Id> ticketIds{};

    support::sTicketMgr.withLock([&](support::TicketMgr& ticketMgr)
    {
        ticketMgr.ForEachTicket([&](support::Ticket& ticket)
        {
            if (filterViewed != boost::none && *filterViewed != ticket.IsViewed())
                return true;

            if (filterEscalated != boost::none && *filterEscalated != ticket.IsEscalated())
                return true;

            if (filterAssigned != boost::none && *filterAssigned != ticket.IsAssigned())
                return true;

            if (filterCompleted != boost::none && *filterCompleted != ticket.IsCompleted())
                return true;

            if (filterClosed != boost::none && *filterClosed != ticket.IsClosed())
                return true;

            if (filterOnline != boost::none && *filterOnline != static_cast<bool>(ticket.GetAuthor()))
                return true;

            ticketIds.push_back(ticket.GetId());

            return (ticketIds.size() < maxCount);
        });
    });

    std::vector<providers::Ticket::FutureResultType> ticketFutures{};
    ticketFutures.reserve(ticketIds.size());
    const auto provider = providers::MakeProvider<providers::Ticket>(ExtractRequestedValues(request), "tickets", request);

    std::transform(
        ticketIds.begin(), ticketIds.end(),
        std::back_inserter(ticketFutures),
        provider
    );

    return folly::collect(ticketFutures)
        .then([response = std::move(response)](std::vector<providers::Ticket::ResultType> tickets) mutable -> json
        {
            response.setWith("tickets", [&]() -> json
            {
                json arr = json::array();

                boost::range::copy(
                    tickets | ::util::adaptor::FilterAndUnwrapOptional,
                    std::back_inserter(arr)
                );

                return arr;
            });

            return response.build();
        });
}

class api_ticket_support_script : public SupportScript
{
public:
    api_ticket_support_script() : SupportScript("api_ticket_support_script") {}

    static void SendCreate(support::Ticket::Id id, json&& payload = json::object())
    {
        payload["id"] = id;
        addon::Endpoint::notifications.Send("ticket/create", std::move(payload));
    }

    static void SendUpdate(support::Ticket::Id id, const std::string& type, AccountId causer, json&& payload = json::object())
    {
        payload["id"] = id;
        payload["type"] = type;
        payload["causer"] = causer;
        addon::Endpoint::notifications.Send("ticket/update", std::move(payload));
    }

    static void SendClose(support::Ticket::Id id, AccountId causer, json&& payload = json::object())
    {
        payload["id"] = id;
        payload["causer"] = causer;
        addon::Endpoint::notifications.Send("ticket/close", std::move(payload));
    }


    void OnTicketCreate(support::Ticket& ticket) override
    {
        SendCreate(ticket.GetId());
    }

    void OnTicketFirstViewed(support::Ticket& ticket, AccountId causer) override
    {
        SendUpdate(ticket.GetId(), "view", causer);
    }

    void OnTicketComplete(support::Ticket& ticket, AccountId causer) override
    {
        SendUpdate(ticket.GetId(), "complete", causer);
    }

    void OnTicketEscalate(support::Ticket& ticket, AccountId causer) override
    {
        SendUpdate(ticket.GetId(), "escalate", causer);
    }

    void OnTicketAssign(support::Ticket& ticket, AccountId causer) override
    {
        SendUpdate(ticket.GetId(), "assign", causer, {{ "assignee_id", ticket.GetAssigneeId().value() }});
    }

    void OnTicketUnassign(support::Ticket& ticket, AccountId causer) override
    {
        SendUpdate(ticket.GetId(), "unassign", causer);
    }

    void OnTicketUpdateMessage(support::Ticket& ticket, AccountId causer) override
    {
        SendUpdate(ticket.GetId(), "message", causer);
    }

    void OnTicketUpdateComment(support::Ticket& ticket, AccountId causer) override
    {
        SendUpdate(ticket.GetId(), "comment", causer);
    }

    void OnTicketUpdateResponse(support::Ticket& ticket, AccountId causer) override
    {
        SendUpdate(ticket.GetId(), "response", causer);
    }

    void OnTicketClose(support::Ticket& ticket, AccountId causer) override
    {
        SendClose(ticket.GetId(), causer);
    }
};

class api_ticket_player_script : public PlayerScript
{
public:
    explicit api_ticket_player_script() : PlayerScript("api_ticket_player_script") {}


    static void SendUpdate(support::Ticket::Id id, json&& payload = json::object())
    {
        payload["id"] = id;
        payload["type"] = "online";
        addon::Endpoint::notifications.Send("ticket/update", std::move(payload));
    }

    void OnLogin(Player* player, bool /* firstLogin */) override
    {
        support::sTicketMgr.withLock([guid = player->GetGUID()](support::TicketMgr& ticketMgr)
        {
            if (auto ticket = ticketMgr.GetOpenTicketByAuthor(guid))
                SendUpdate(ticket->GetId(), {{ "is_author_online", true }});
        });
    }

    void OnLogout(Player* player) override
    {
        support::sTicketMgr.withLock([guid = player->GetGUID()](support::TicketMgr& ticketMgr)
        {
            if (auto ticket = ticketMgr.GetOpenTicketByAuthor(guid))
                SendUpdate(ticket->GetId(), {{ "is_author_online", false }});
        });
    }
};

void RegisterTicketHandlers()
{
    Dispatcher::RegisterHandler("ticket/show", HandleTicketShow, rbac::PERM_API_TICKET_SHOW);
    Dispatcher::RegisterHandler("ticket/list", HandleTicketList, rbac::PERM_API_TICKET_LIST);

    addon::Endpoint::notifications.Create("ticket/create", rbac::PERM_API_TICKET_EVENT_CREATE);
    addon::Endpoint::notifications.Create("ticket/update", rbac::PERM_API_TICKET_EVENT_UPDATE);
    addon::Endpoint::notifications.Create("ticket/close", rbac::PERM_API_TICKET_EVENT_CLOSE);

    new api_ticket_support_script();
    new api_ticket_player_script();
}
