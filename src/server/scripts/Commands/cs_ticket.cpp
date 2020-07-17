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

/* ScriptData
Name: ticket_commandscript
%Complete: 100
Comment: All ticket related commands
Category: commandscripts
EndScriptData */

#include "Accounts/AccountMgr.h"
#include "Entities/Player/PlayerCache.hpp"
#include "Globals/ObjectMgr.h"
#include "Miscellaneous/Language.h"
#include "Scripting/ScriptMgr.h"
#include "Support/TicketMgr.hpp"

#include "Chat.h"
#include "Chat/Command/Commands/LegacyCommand.hpp"
#include "Chat/Command/Commands/SimpleCommand.hpp"
#include "Chat/Command/GenericCommandScriptLoader.hpp"
#include "WorldPacket.h"

namespace chat::command::handler
{

    bool HandleToggleGMTicketSystem(ChatHandler* handler, char const* /*args*/)
    {
        support::sTicketMgr.withLock([&](support::TicketMgr& ticketMgr) {
            const bool status = ticketMgr.IsEnabled();
            ticketMgr.SetEnabled(!status);
            handler->PSendSysMessage((!status) ? LANG_ALLOW_TICKETS : LANG_DISALLOW_TICKETS);
        });
        return true;
    }

    class TicketEscalateCommand : public SimpleCommand
    {
    public:
        explicit TicketEscalateCommand(CommandDefinition definition) : SimpleCommand(std::move(definition), true) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
            if (args.size() < 1)
                return ExecutionResult::FAILURE_MISSING_ARGUMENT;

            auto ticketIdOpt = args[0].GetUInt32();
            if (!ticketIdOpt)
                return ExecutionResult::FAILURE_INVALID_ARGUMENT;
            support::Ticket::Id ticketId = ticketIdOpt.value();

            return support::sTicketMgr.withLock([&](support::TicketMgr& ticketMgr)
            {
                auto ticket = ticketMgr.GetOpenTicket(ticketId);
                if (!ticket || ticket->IsCompleted() || ticket->IsEscalated())
                    return SendError(context, LANG_COMMAND_TICKETNOTEXIST);

                if (ticket->IsAssigned())
                {
                    if (*(ticket->GetAssigneeId()) != context.GetAccountId())
                        return SendError(context, LANG_COMMAND_TICKETALREADYASSIGNED, ticket->GetId());

                    ticket->Unassign(context.GetAccountId(), false);
                }

                ticket->Escalate(context.GetAccountId());

                return ExecutionResult::SUCCESS;
            });
        }
    };

    class TicketCommentCommand : public SimpleCommand
    {
    public:
        explicit TicketCommentCommand(CommandDefinition definition) : SimpleCommand(std::move(definition), true) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
            if (args.size() < 2)
                return ExecutionResult::FAILURE_MISSING_ARGUMENT;

            auto ticketIdOpt = args[0].GetUInt32();
            if (!ticketIdOpt)
                return ExecutionResult::FAILURE_INVALID_ARGUMENT;
            support::Ticket::Id ticketId = ticketIdOpt.value();

            std::string comment = args[1].GetString();

            return support::sTicketMgr.withLock([&](support::TicketMgr& ticketMgr)
            {
                auto ticket = ticketMgr.GetOpenTicket(ticketId);
                if (!ticket)
                    return SendError(context, LANG_COMMAND_TICKETNOTEXIST);

                if (ticket->IsAssigned() && *(ticket->GetAssigneeId()) != context.GetAccountId())
                    return SendError(context, LANG_COMMAND_TICKETALREADYASSIGNED, ticket->GetId());

                ticket->SetComment(std::move(comment), context.GetAccountId());

                return ExecutionResult::SUCCESS;
            });
        }
    };

    class TicketAssignCommand : public SimpleCommand
    {
    public:
        explicit TicketAssignCommand(CommandDefinition definition) : SimpleCommand(std::move(definition), true) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
            if (args.size() < 1)
                return ExecutionResult::FAILURE_MISSING_ARGUMENT;

            auto ticketIdOpt = args[0].GetUInt32();
            if (!ticketIdOpt)
                return ExecutionResult::FAILURE_INVALID_ARGUMENT;
            support::Ticket::Id ticketId = ticketIdOpt.value();

            AccountId target = context.GetAccountId(); // target to assign the ticket to

            if (args.size() > 1)
            {
                auto targetIdOpt = args[1].GetUInt32();
                if (!ticketIdOpt)
                    return ExecutionResult::FAILURE_INVALID_ARGUMENT;
                target = *targetIdOpt;
            }

            if (!AccountMgr::HasPermission(target, rbac::RBAC_PERM_COMMANDS_BE_ASSIGNED_TICKET, realm.Id.Realm))
                return SendError(context, LANG_COMMAND_TICKETASSIGNERROR_A);

            return support::sTicketMgr.withLock([&](support::TicketMgr& ticketMgr)
            {
                auto ticket = ticketMgr.GetOpenTicket(ticketId);
                if (!ticket)
                    return SendError(context, LANG_COMMAND_TICKETNOTEXIST);

                if (ticket->IsAssigned())
                {
                    if (*(ticket->GetAssigneeId()) == target)
                        return SendError(context, LANG_COMMAND_TICKETASSIGNERROR_B, ticket->GetId());

                    // to prevent simulatnious assignments to overwrite each other, only allow assigning a ticket if it is currently assigned to you (or unassigned)
                    if (*(ticket->GetAssigneeId()) != context.GetAccountId())
                        return SendError(context, LANG_COMMAND_TICKETALREADYASSIGNED, ticket->GetId());
                }

                ticket->AssignTo(target, context.GetAccountId());

                return ExecutionResult::SUCCESS;
            });
        }
    };

    class TicketUnassignCommand : public SimpleCommand
    {
    public:
        explicit TicketUnassignCommand(CommandDefinition definition) : SimpleCommand(std::move(definition), true) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
            if (args.size() < 1)
                return ExecutionResult::FAILURE_MISSING_ARGUMENT;

            auto ticketIdOpt = args[0].GetUInt32();
            if (!ticketIdOpt)
                return ExecutionResult::FAILURE_INVALID_ARGUMENT;
            support::Ticket::Id ticketId = ticketIdOpt.value();

            return support::sTicketMgr.withLock([&](support::TicketMgr& ticketMgr)
            {
                auto ticket = ticketMgr.GetOpenTicket(ticketId);
                if (!ticket)
                    return SendError(context, LANG_COMMAND_TICKETNOTEXIST);

                if (!ticket->IsAssigned())
                    return SendError(context, LANG_COMMAND_TICKETNOTASSIGNED, ticket->GetId());

                if (AccountMgr::GetSecurity(*ticket->GetAssigneeId(), realm.Id.Realm) > context.GetSecurity())
                    return SendError(context, LANG_COMMAND_TICKETUNASSIGNSECURITY);

                ticket->Unassign(context.GetAccountId());

                return ExecutionResult::SUCCESS;
            });
        }
    };

    template<bool newline>
    class TicketResponseAppendCommand : public SimpleCommand
    {
    public:
        explicit TicketResponseAppendCommand(CommandDefinition definition) : SimpleCommand(std::move(definition), true) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
            if (args.size() < 2)
                return ExecutionResult::FAILURE_MISSING_ARGUMENT;

            auto ticketIdOpt = args[0].GetUInt32();
            if (!ticketIdOpt)
                return ExecutionResult::FAILURE_INVALID_ARGUMENT;
            support::Ticket::Id ticketId = ticketIdOpt.value();

            std::string response = args[1].GetString();
            if constexpr (newline)
                response += "\n";

            return support::sTicketMgr.withLock([&](support::TicketMgr& ticketMgr)
            {
                auto ticket = ticketMgr.GetOpenTicket(ticketId);
                if (!ticket)
                    return SendError(context, LANG_COMMAND_TICKETNOTEXIST);

                if (ticket->IsAssigned() && *(ticket->GetAssigneeId()) != context.GetAccountId())
                    return SendError(context, LANG_COMMAND_TICKETALREADYASSIGNED, ticket->GetId());

                ticket->AppendResponse(std::move(response), context.GetAccountId());

                return ExecutionResult::SUCCESS;
            });
        }
    };

    class TicketCompleteCommand : public SimpleCommand
    {
    public:
        explicit TicketCompleteCommand(CommandDefinition definition) : SimpleCommand(std::move(definition), true) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
            if (args.size() < 1)
                return ExecutionResult::FAILURE_MISSING_ARGUMENT;

            auto ticketIdOpt = args[0].GetUInt32();
            if (!ticketIdOpt)
                return ExecutionResult::FAILURE_INVALID_ARGUMENT;
            support::Ticket::Id ticketId = ticketIdOpt.value();

            return support::sTicketMgr.withLock([&](support::TicketMgr& ticketMgr)
            {
                auto ticket = ticketMgr.GetOpenTicket(ticketId);
                if (!ticket || ticket->IsCompleted())
                    return SendError(context, LANG_COMMAND_TICKETNOTEXIST);

                if (ticket->IsAssigned() && *(ticket->GetAssigneeId()) != context.GetAccountId())
                    return SendError(context, LANG_COMMAND_TICKETALREADYASSIGNED, ticket->GetId());

                ticket->Complete(context.GetAccountId());

                return ExecutionResult::SUCCESS;
            });
        }
    };

    class TicketCloseCommand : public SimpleCommand
    {
    public:
        explicit TicketCloseCommand(CommandDefinition definition) : SimpleCommand(std::move(definition), true) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
            if (args.size() < 1)
                return ExecutionResult::FAILURE_MISSING_ARGUMENT;

            auto ticketIdOpt = args[0].GetUInt32();
            if (!ticketIdOpt)
                return ExecutionResult::FAILURE_INVALID_ARGUMENT;
            support::Ticket::Id ticketId = ticketIdOpt.value();

            AccountId closer;
            if (context.GetSource() == ExecutionContext::Source::CONSOLE)
            {
                if (args.size() < 2)
                    return ExecutionResult::FAILURE_MISSING_ARGUMENT;

                auto closerOpt = args[1].GetUInt32();
                if (!closerOpt)
                    return ExecutionResult::FAILURE_INVALID_ARGUMENT;
                closer = *closerOpt;
            }
            else
                closer = context.GetAccountId();

            return support::sTicketMgr.withLock([&](support::TicketMgr& ticketMgr)
            {
                auto ticket = ticketMgr.GetOpenTicket(ticketId);
                if (!ticket)
                    return SendError(context, LANG_COMMAND_TICKETNOTEXIST);

                if (ticket->IsAssigned() && *(ticket->GetAssigneeId()) != closer)
                    return SendError(context, LANG_COMMAND_TICKETCANNOTCLOSE, ticket->GetId());

                ticket->Close(closer);

                return ExecutionResult::SUCCESS;
            });
        }
    };

}

void AddSC_ticket_commandscript()
{
    using namespace chat::command;
    using namespace chat::command::handler;

    new LegacyCommandScript("cmd_ticket_togglesystem", HandleToggleGMTicketSystem, true);

    new GenericCommandScriptLoader<TicketEscalateCommand>("cmd_ticket_escalate");
    new GenericCommandScriptLoader<TicketCommentCommand>("cmd_ticket_comment");
    new GenericCommandScriptLoader<TicketAssignCommand>("cmd_ticket_assign");
    new GenericCommandScriptLoader<TicketUnassignCommand>("cmd_ticket_unassign");

    new GenericCommandScriptLoader<TicketResponseAppendCommand<false>>("cmd_ticket_response_append");
    new GenericCommandScriptLoader<TicketResponseAppendCommand<true>>("cmd_ticket_response_appendln");
    new GenericCommandScriptLoader<TicketCompleteCommand>("cmd_ticket_complete");

    new GenericCommandScriptLoader<TicketCloseCommand>("cmd_ticket_close");
}
