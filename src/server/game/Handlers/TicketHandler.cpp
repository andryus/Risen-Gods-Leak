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

#include "zlib.h"
#include "Common.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "Support/TicketMgr.hpp"
#include "Util.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include "RG/RGSuspiciousLog.h"

inline void SendTicketCreationFailed(WorldSession& session)
{
    WorldPacket packet(SMSG_GMTICKET_CREATE, 4);
    packet << uint32(GMTICKET_RESPONSE_CREATE_ERROR);
    session.SendPacket(std::move(packet));
}

void WorldSession::HandleGMTicketCreateOpcode(WorldPacket& recvData)
{
    support::sTicketMgr.withLock([&](support::TicketMgr& ticketMgr)
    {
        // Don't accept tickets if the ticket queue is disabled. (Ticket UI is greyed out but not fully dependable)
        if (!ticketMgr.IsEnabled())
            return;

        if (GetPlayer()->getLevel() < sWorld->getIntConfig(CONFIG_TICKET_LEVEL_REQ))
        {
            SendNotification(GetTrinityString(LANG_TICKET_REQ), sWorld->getIntConfig(CONFIG_TICKET_LEVEL_REQ));
            return;
        }

        uint32 mapId;
        float x, y, z;
        std::string message;
        support::Ticket::Type type;
        bool isFollowUpTicket;

        uint32 count;
        std::list<uint32> times;
        uint32 decompressedSize;
        std::string chatLog;

        recvData >> mapId;
        recvData >> x >> y >> z;
        recvData >> message;

        {
            uint32 typeValue;
            recvData >> typeValue;
            if (typeValue != 1 && typeValue != 17)
            {
                SendTicketCreationFailed(*this);
                return;
            }
            type = static_cast<support::Ticket::Type>(typeValue);
        }

        recvData >> isFollowUpTicket;

        recvData >> count;
        for (uint32 i = 0; i < count; i++)
        {
            uint32 time;
            recvData >> time;
            times.push_back(time);
        }

        recvData >> decompressedSize;
        if (count && decompressedSize && decompressedSize < 0xFFFF)
        {
            uint32 pos = recvData.rpos();
            ByteBuffer dest;
            dest.resize(decompressedSize);

            uLongf realSize = decompressedSize;
            if (uncompress(dest.contents(), &realSize, recvData.contents() + pos, recvData.size() - pos) == Z_OK)
            {
                dest >> chatLog;
            }
            else
            {
                TC_LOG_ERROR("network", "CMSG_GMTICKET_CREATE possibly corrupt. Uncompression failed.");
                recvData.rfinish();
                return;
            }

            recvData.rfinish(); // Will still have compressed data in buffer.
        }

        if (auto ticket = ticketMgr.GetOpenTicketByAuthor(GetPlayer()->GetGUID()))
        {
            if (isFollowUpTicket && ticket->IsCompleted())
            {
                ticket->Close(GetAccountId());
            }
            else
            {
                // multiple tickets not allowed; should be prevented by client normally
                SendTicketCreationFailed(*this);
                return;
            }
        }

        support::Ticket& ticket = ticketMgr.CreateTicket(
            type,
            *GetPlayer(),
            WorldLocation{ mapId, x, y, z },
            std::move(message),
            isFollowUpTicket
        );

        GetMonitor()->CheckConditionMeets(RG::Suspicious::CONDITION_TICKET_CREATE, ticket);

        WorldPacket packet(SMSG_GMTICKET_CREATE, 4);
        packet << uint32(GMTICKET_RESPONSE_CREATE_SUCCESS);
        SendPacket(std::move(packet));
    });
}

void WorldSession::HandleGMTicketUpdateOpcode(WorldPacket& recvData)
{
    support::sTicketMgr.withLock([&](support::TicketMgr& ticketMgr)
    {
        std::string message;
        recvData >> message;

        GMTicketResponse response = GMTICKET_RESPONSE_UPDATE_ERROR;

        if (auto ticket = ticketMgr.GetOpenTicketByAuthor(GetPlayer()->GetGUID()))
        {
            ticket->SetMessage(std::move(message), GetAccountId());

            response = GMTICKET_RESPONSE_UPDATE_SUCCESS;
        }

        WorldPacket packet(SMSG_GMTICKET_UPDATETEXT, 4);
        packet << uint32(response);
    SendPacket(std::move(packet));
    });
}

void WorldSession::HandleGMTicketDeleteOpcode(WorldPacket& /*recvData*/)
{
    support::sTicketMgr.withLock([&](support::TicketMgr& ticketMgr)
    {
        if (auto ticket = ticketMgr.GetOpenTicketByAuthor(GetPlayer()->GetGUID()))
        {
            ticket->Close(GetAccountId());
        }
    });
}

void WorldSession::HandleGMTicketGetTicketOpcode(WorldPacket& /*recvData*/)
{
    SendQueryTimeResponse();

    support::sTicketMgr.withLock([&](support::TicketMgr& ticketMgr)
    {
        if (auto ticket = ticketMgr.GetOpenTicketByAuthor(GetPlayer()->GetGUID()))
            ticket->SendTo(*this);
        else
            support::Ticket::SendNoTicketTo(*this);
    });
}

void WorldSession::HandleGMTicketSystemStatusOpcode(WorldPacket& /*recvData*/)
{
    support::sTicketMgr.withLock([&](support::TicketMgr& ticketMgr)
    {
        // Note: This only disables the ticket UI at client side and is not fully reliable
        WorldPacket packet(SMSG_GMTICKET_SYSTEMSTATUS, 4);
        packet << uint32(ticketMgr.IsEnabled() ? GMTICKET_QUEUE_STATUS_ENABLED : GMTICKET_QUEUE_STATUS_DISABLED);
        SendPacket(std::move(packet));
    });
}

void WorldSession::HandleGMSurveySubmit(WorldPacket& recvData)
{
    uint32 nextSurveyID = support::sTicketMgr.lock()->GenerateSurveyId();
    // just put the survey into the database
    uint32 mainSurvey; // GMSurveyCurrentSurvey.dbc, column 1 (all 9) ref to GMSurveySurveys.dbc
    recvData >> mainSurvey;

    std::unordered_set<uint32> surveyIds;
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    // sub_survey1, r1, comment1, sub_survey2, r2, comment2, sub_survey3, r3, comment3, sub_survey4, r4, comment4, sub_survey5, r5, comment5, sub_survey6, r6, comment6, sub_survey7, r7, comment7, sub_survey8, r8, comment8, sub_survey9, r9, comment9, sub_survey10, r10, comment10,
    for (uint8 i = 0; i < 10; i++)
    {
        uint32 subSurveyId; // ref to i'th GMSurveySurveys.dbc field (all fields in that dbc point to fields in GMSurveyQuestions.dbc)
        recvData >> subSurveyId;
        if (!subSurveyId)
            break;

        uint8 rank; // probably some sort of ref to GMSurveyAnswers.dbc
        recvData >> rank;
        std::string comment; // comment ("Usage: GMSurveyAnswerSubmit(question, rank, comment)")
        recvData >> comment;

        // make sure the same sub survey is not added to DB twice
        if (!surveyIds.insert(subSurveyId).second)
            continue;

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_TICKET_SUBSURVEY);
        stmt->setUInt32(0, nextSurveyID);
        stmt->setUInt32(1, subSurveyId);
        stmt->setUInt32(2, rank);
        stmt->setString(3, comment);
        trans->Append(stmt);
    }

    std::string comment; // just a guess
    recvData >> comment;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_TICKET_SURVEY);
    stmt->setUInt32(0, GetPlayer()->GetGUID().GetCounter());
    stmt->setUInt32(1, nextSurveyID);
    stmt->setUInt32(2, mainSurvey);
    stmt->setString(3, comment);

    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

void WorldSession::HandleReportLag(WorldPacket& recvData)
{
    // just put the lag report into the database...
    // can't think of anything else to do with it
    uint32 lagType, mapId;
    recvData >> lagType;
    recvData >> mapId;
    float x, y, z;
    recvData >> x;
    recvData >> y;
    recvData >> z;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_LAG_REPORT);
    stmt->setUInt32(0, GetPlayer()->GetGUID().GetCounter());
    stmt->setUInt8 (1, lagType);
    stmt->setUInt16(2, mapId);
    stmt->setFloat (3, x);
    stmt->setFloat (4, y);
    stmt->setFloat (5, z);
    stmt->setUInt32(6, GetLatency());
    stmt->setUInt32(7, time(nullptr));
    CharacterDatabase.Execute(stmt);
}

void WorldSession::HandleGMResponseResolve(WorldPacket& /*recvPacket*/)
{
    support::sTicketMgr.withLock([&](support::TicketMgr& ticketMgr)
    {
        // empty packet
        if (auto ticket = ticketMgr.GetOpenTicketByAuthor(GetPlayer()->GetGUID()))
        {
            uint8 getSurvey = 0;
            if (float(rand_chance()) < sWorld->getFloatConfig(CONFIG_CHANCE_OF_GM_SURVEY))
                getSurvey = 1;

            WorldPacket packet(SMSG_GMRESPONSE_STATUS_UPDATE, 4);
            packet << uint8(getSurvey);
            SendPacket(std::move(packet));

            ticket->Close(GetAccountId());
        }
    });
}
