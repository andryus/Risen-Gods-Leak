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

#include "Support/TicketMgr.hpp"

#include "Common.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Language.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Chat.h"
#include "World.h"
#include "Player.h"
#include "Opcodes.h"
#include "Scripting/ScriptMgr.h"

namespace support
{
    folly::Synchronized<TicketMgr, std::mutex>& TicketMgr::instance()
    {
        static folly::Synchronized<TicketMgr, std::mutex> instance{ folly::in_place };
        return instance;
    }

    void TicketMgr::Initialize()
    {
        SetEnabled(sWorld->getBoolConfig(CONFIG_ALLOW_TICKETS));
        LoadTickets();
        LoadSurveys();
    }

    void TicketMgr::LoadTickets()
    {
        TC_LOG_INFO("server.loading", "Loading GM tickets...");
        const uint32 oldMSTime = getMSTime();

        tickets.clear();

        {
            const auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAX_TICKET_ID);
            const auto result = CharacterDatabase.Query(stmt);
            if (!result)
            {
                TC_LOG_INFO("server.loading", ">> Loaded 0 GM tickets. DB table `gm_tickets` is empty!");
                return;
            }

            lastTicketId = (*result)[0].GetUInt32();
        }

        const auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_OPEN_TICKETS);
        if (const auto result = CharacterDatabase.Query(stmt))
        {
            auto& index = tickets.get<id_tag>();

            do
            {
                index.emplace(Ticket::LoadFromDB(result->Fetch(), *this));
            } while (result->NextRow());

            UpdateOldestTicketCreationTime();

            TC_LOG_INFO("server.loading", ">> Loaded %zu GM tickets in %u ms", tickets.size(), GetMSTimeDiffToNow(oldMSTime));
        }
        else
            TC_LOG_INFO("server.loading", ">> Loaded 0 GM tickets. No open tickets found.");
    }

    void TicketMgr::LoadSurveys()
    {
        TC_LOG_INFO("server.loading", "Loading GM surveys...");
        const uint32 oldMSTime = getMSTime();

        const auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAX_SURVEY_ID);
        if (const auto result = CharacterDatabase.Query(stmt))
            lastSurveyId = (*result)[0].GetUInt32();

        TC_LOG_INFO("server.loading", ">> Loaded GM Survey count from database in %u ms", GetMSTimeDiffToNow(oldMSTime));
    }

    boost::optional<Ticket&> TicketMgr::GetOpenTicket(Ticket::Id ticketId) const
    {
        auto& index = tickets.get<id_tag>();
        const auto itr = index.find(ticketId);
        if (itr != index.end())
            return const_cast<Ticket&>(*itr); // does not violate container constraints as all indexed members cannot be modified

        return boost::none;
    }

    boost::optional<Ticket&> TicketMgr::GetOpenTicketByAuthor(ObjectGuid authorGuid) const
    {
        auto& index = tickets.get<author_tag>();
        const auto itr = index.find(authorGuid);
        if (itr != index.end())
            return const_cast<Ticket&>(*itr); // does not violate container constraints as all indexed members cannot be modified

        return boost::none;
    }

    Ticket& TicketMgr::CreateTicket(Ticket::Type type, Player& author, const WorldLocation& pos, std::string message, bool isFollowUpTicket)
    {
        const Ticket::Id id = GenerateTicketId();

        auto [itr, successful] = tickets.get<id_tag>().emplace(Ticket{
            id, type, isFollowUpTicket, pos, author.GetGUID(), std::move(message), *this
        });
        ASSERT(successful, "duplicated ticket id");

        Ticket& ticket = const_cast<Ticket&>(*itr);
        ticket.SaveToDB();

        UpdateOldestTicketCreationTime();

        sScriptMgr->OnTicketCreate(ticket);

        return ticket;
    }

    uint32 TicketMgr::GenerateTicketId()
    {
        ++lastTicketId;
        return lastTicketId;
    }

    uint32 TicketMgr::GenerateSurveyId()
    {
        ++lastSurveyId;
        return lastSurveyId;
    }

    void TicketMgr::UpdateOldestTicketCreationTime()
    {
        auto& index = tickets.get<id_tag>();

        bool updated = false;
        for (const Ticket& ticket : index)
        {
            if (!ticket.IsClosed() && !ticket.IsCompleted())
            {
                updated = true;
                oldestTicketCreationTime = ticket.GetCreationTime();
                break;
            }
        }

        if (!updated)
            oldestTicketCreationTime = TimePoint{};
    }

    void TicketMgr::OnTicketClosed(Ticket::Id id)
    {
        auto& index = tickets.get<id_tag>();
        index.erase(id);

        UpdateOldestTicketCreationTime();
    }

    void TicketMgr::OnTicketCompleted(Ticket::Id /* id */)
    {
        lastGMInteractionTime = SystemClock::now();

        UpdateOldestTicketCreationTime();
    }

}
