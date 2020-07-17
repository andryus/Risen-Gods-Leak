#pragma once

#include <mutex>

#include <boost/optional.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <folly/Synchronized.h>

#include "Duration.h"

#include "Support/Define.hpp"
#include "Support/Ticket.hpp"

class ChatHandler;

namespace support
{

    class GAME_API TicketMgr
    {
        friend class Ticket;

    private:
        struct id_tag {};
        struct author_tag {};

        using Container = boost::multi_index_container<
            Ticket,
            boost::multi_index::indexed_by<
                boost::multi_index::ordered_unique<
                    boost::multi_index::tag<id_tag>,
                    boost::multi_index::const_mem_fun<Ticket, Ticket::Id, &Ticket::GetId>
                >,
                boost::multi_index::hashed_unique<
                    boost::multi_index::tag<author_tag>,
                    boost::multi_index::const_mem_fun<Ticket, ObjectGuid, &Ticket::GetAuthorGUID>
                >
            >
        >;

    private:
        Container tickets{};

        bool isEnabled = true;
        uint32 lastTicketId = 0;
        uint32 lastSurveyId = 0;
        TimePoint lastGMInteractionTime{};
        TimePoint oldestTicketCreationTime{};

    public:
        TicketMgr() = default;
        TicketMgr(const TicketMgr&) = delete;

        static folly::Synchronized<TicketMgr, std::mutex>& instance();

        bool IsEnabled() const { return isEnabled; }
        void SetEnabled(bool status) { isEnabled = status; }

        void Initialize();

        void LoadTickets();
        void LoadSurveys();

        boost::optional<Ticket&> GetOpenTicket(Ticket::Id ticketId) const;
        boost::optional<Ticket&> GetOpenTicketByAuthor(ObjectGuid authorGuid) const;

        Ticket& CreateTicket(Ticket::Type type, Player& author, const WorldLocation& pos, std::string message, bool isFollowUpTicket);

        TimePoint GetOldestTicketCreationTime() const { return oldestTicketCreationTime; }
        TimePoint GetLastGMInteractionTime() const { return lastGMInteractionTime; }

        uint32 GenerateTicketId();
        uint32 GenerateSurveyId();

        template<typename FunctionT>
        void ForEachTicket(FunctionT&& func)
        {
            for (const Ticket& ticket : tickets.get<id_tag>())
                if (!func(const_cast<Ticket&>(ticket)))
                    break;
        }

        template<typename FunctionT>
        void ForEachTicket(FunctionT&& func) const
        {
            for (const Ticket& ticket : tickets.get<id_tag>())
                if (!func(ticket))
                    break;
        }

    private:
        void UpdateOldestTicketCreationTime();

        void OnTicketClosed(Ticket::Id id);
        void OnTicketCompleted(Ticket::Id id);
    };

}

#define sTicketMgr TicketMgr::instance()
