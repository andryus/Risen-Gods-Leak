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
Name: server_commandscript
%Complete: 100
Comment: All server related commands
Category: commandscripts
EndScriptData */

#include <boost/lexical_cast.hpp>

#include "Chat.h"
#include "Chat/Command/Commands/LegacyCommand.hpp"
#include "Chat/Command/Commands/SimpleCommand.hpp"
#include "Chat/Command/GenericCommandScriptLoader.hpp"
#include "Config.h"
#include "Language.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "GitRevision.h"
#include "Util.h"
#include "Player.h"
#include "Map.h"
#include "GameTime.h"

namespace chat { namespace command { namespace handler {

    bool ParseExitCode(char const* exitCodeStr, int32& exitCode)
    {
        exitCode = atoi(exitCodeStr);

        // Handle atoi() errors
        if (exitCode == 0 && (exitCodeStr[0] != '0' || exitCodeStr[1] != '\0'))
            return false;

        // Exit code should be in range of 0-125, 126-255 is used
        // in many shells for their own return codes and code > 255
        // is not supported in many others
        if (exitCode < 0 || exitCode > 125)
            return false;

        return true;
    }

    bool ShutdownServer(char const* args, uint32 shutdownMask, int32 defaultExitCode)
    {
        if (!*args)
            return false;

        if (strlen(args) > 255)
            return false;

        // #delay [#exit_code] [reason]
        int32 delay = 0;
        char* delayStr = strtok((char*)args, " ");
        if (!delayStr)
            return false;

        if (isNumeric(delayStr))
        {
            delay = atoi(delayStr);
            // Prevent interpret wrong arg value as 0 secs shutdown time
            if ((delay == 0 && (delayStr[0] != '0' || delayStr[1] != '\0')) || delay < 0)
                return false;
        }
        else
        {
            delay = TimeStringToSecs(std::string(delayStr));

            if (delay == 0)
                return false;
        }

        char* exitCodeStr = nullptr;

        char reason[256] = { 0 };

        while (char* nextToken = strtok(nullptr, " "))
        {
            if (isNumeric(nextToken))
                exitCodeStr = nextToken;
            else
            {
                strcat(reason, nextToken);
                if (char* remainingTokens = strtok(nullptr, "\0"))
                {
                    strcat(reason, " ");
                    strcat(reason, remainingTokens);
                }
                break;
            }
        }

        int32 exitCode = defaultExitCode;
        if (exitCodeStr)
            if (!ParseExitCode(exitCodeStr, exitCode))
                return false;

        sWorld->ShutdownServ(delay, shutdownMask, static_cast<uint8>(exitCode), std::string(reason));

        return true;
    }

    // Triggering corpses expire check in world
    bool HandleServerCorpsesCommand(ChatHandler* /*handler*/, char const* /*args*/)
    {
        sObjectAccessor->RemoveOldCorpses();
        return true;
    }

    bool HandleServerInfoCommand(ChatHandler* handler, char const* /*args*/)
    {
        bool isIngame = false;
        if (handler->GetSession())
            isIngame = true;

        uint32 playersNum = sWorld->GetPlayerCount();
        uint32 maxPlayersNum = sWorld->GetMaxPlayerCount();
        uint32 activeClientsNum = sWorld->GetActiveSessionCount();
        uint32 queuedClientsNum = sWorld->GetQueuedSessionCount();
        uint32 maxActiveClientsNum = sWorld->GetMaxActiveSessionCount();
        uint32 maxQueuedClientsNum = sWorld->GetMaxQueuedSessionCount();
        std::string uptime = secsToTimeString(game::time::GetUptime());
        
        handler->SendSysMessage(GitRevision::GetFullVersion(isIngame));
        handler->PSendSysMessage(isIngame ? LANG_CONNECTED_PLAYERS_COLORED : LANG_CONNECTED_PLAYERS, playersNum, maxPlayersNum);
        handler->PSendSysMessage(isIngame ? LANG_CONNECTED_USERS_COLORED : LANG_CONNECTED_USERS, activeClientsNum, maxActiveClientsNum, queuedClientsNum, maxQueuedClientsNum);
        handler->PSendSysMessage(isIngame ? LANG_UPTIME_COLORED : LANG_UPTIME, uptime.c_str());

        auto worldInfo = sWorld->GetDiffTimer().GetInfo();
        handler->PSendSysMessage(isIngame ? LANG_SERVERINFO_WORLD_DIFFTIME_COLORED : LANG_SERVERINFO_WORLD_DIFFTIME, worldInfo.maxLatency, worldInfo.avgLatency, worldInfo.maxDuration, worldInfo.avgDuration);
        if (handler->GetSession())
        {
            if (handler->GetSession()->GetPlayer())
            {
                if (handler->GetSession()->GetPlayer()->IsInWorld())
                {
                    auto mapInfo = handler->GetSession()->GetPlayer()->GetMap()->GetDiffTimer().GetInfo();
                    float visibilityDist = handler->GetSession()->GetPlayer()->GetMap()->GetDynamicVisibilityRange();
                    handler->PSendSysMessage(LANG_SERVERINFO_MAP_DIFFTIME, mapInfo.maxLatency, mapInfo.avgLatency, mapInfo.maxDuration, mapInfo.avgDuration, visibilityDist);
                }
            }
        }
        // Can't use sWorld->ShutdownMsg here in case of console command
        if (sWorld->IsShuttingDown())
            handler->PSendSysMessage(LANG_SHUTDOWN_TIMELEFT, secsToTimeString(sWorld->GetShutDownTimeLeft()).c_str());

        return true;
    }
    // Display the 'Message of the day' for the realm
    bool HandleServerMotdCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->PSendSysMessage(LANG_MOTD_CURRENT, sWorld->GetMotd());
        return true;
    }

    class ServerPlayerLimitCommand : public SimpleCommand
    {
    public:
        explicit ServerPlayerLimitCommand(CommandDefinition definition) : SimpleCommand(std::move(definition), true) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
            if (args.size() > 0) // set new limit
            {
                uint32 limit;

                if (args[0] == "reset")
                    limit = sConfigMgr->GetIntDefault("PlayerLimit", 100);
                else
                {
                    auto valueOpt = args[0].GetUInt32();
                    if (!valueOpt)
                        return ExecutionResult::FAILURE_INVALID_ARGUMENT;
                    limit = *valueOpt;
                }

                sWorld->SetPlayerAmountLimit(limit);
            }

            // show current value (may have changed above)
            context.SendSystemMessage("Server player limit: %u", sWorld->GetPlayerAmountLimit());

            return ExecutionResult::SUCCESS;
        };
    };

    class ServerSecurityLimitCommand : public SimpleCommand
    {
    public:
        explicit ServerSecurityLimitCommand(CommandDefinition definition) : SimpleCommand(std::move(definition), true) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
            if (args.size() > 0)
            {
                const util::Argument& arg = args[0];

                if (arg == "reset")
                    sWorld->LoadDBAllowedSecurityLevel();
                else
                {
                    AccountTypes limit;

                    if (arg == "player")
                        limit = SEC_PLAYER;
                    else if (arg == "moderator" || arg == "mod")
                        limit = SEC_MODERATOR;
                    else if (arg == "gamemaster" || arg == "gm")
                        limit = SEC_GAMEMASTER;
                    else if (arg == "administrator" || arg == "admin")
                        limit = SEC_ADMINISTRATOR;
                    else
                    {
                        auto valueOpt = arg.GetUInt32();
                        if (!valueOpt)
                            return ExecutionResult::FAILURE_INVALID_ARGUMENT;
                        limit = static_cast<AccountTypes>(*valueOpt);
                    }

                    sWorld->SetPlayerSecurityLimit(limit);
                }
            }

            // show current value (may have changed above)
            std::string secName;
            AccountTypes limit = sWorld->GetPlayerSecurityLimit();
            switch (limit)
            {
                case SEC_PLAYER:
                    secName = "Player";
                    break;
                case SEC_MODERATOR:
                    secName = "Moderator";
                    break;
                case SEC_GAMEMASTER:
                    secName = "Gamemaster";
                    break;
                case SEC_ADMINISTRATOR:
                    secName = "Administrator";
                    break;
                default:
                {
                    try { secName = boost::lexical_cast<std::string>(limit); }
                    catch (const boost::bad_lexical_cast&) { secName = "<unknown>"; }
                    break;
                }
            }
            context.SendSystemMessage("Server security limit: %s", secName);

            return ExecutionResult::SUCCESS;
        }
    };

    bool HandleServerShutDownCancelCommand(ChatHandler* /*handler*/, char const* /*args*/)
    {
        sWorld->ShutdownCancel();

        return true;
    }

    bool HandleServerShutDownCommand(ChatHandler* /*handler*/, char const* args)
    {
        return ShutdownServer(args, 0, SHUTDOWN_EXIT_CODE);
    }

    bool HandleServerRestartCommand(ChatHandler* /*handler*/, char const* args)
    {
        return ShutdownServer(args, SHUTDOWN_MASK_RESTART, RESTART_EXIT_CODE);
    }

    bool HandleServerIdleRestartCommand(ChatHandler* /*handler*/, char const* args)
    {
        return ShutdownServer(args, SHUTDOWN_MASK_RESTART | SHUTDOWN_MASK_IDLE, RESTART_EXIT_CODE);
    }

    bool HandleServerIdleShutDownCommand(ChatHandler* /*handler*/, char const* args)
    {
        return ShutdownServer(args, SHUTDOWN_MASK_IDLE, SHUTDOWN_EXIT_CODE);
    }

    // Exit the realm
    bool HandleServerExitCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->SendSysMessage(LANG_COMMAND_EXIT);
        World::StopNow(SHUTDOWN_EXIT_CODE);
        return true;
    }

    // Define the 'Message of the day' for the realm
    bool HandleServerSetMotdCommand(ChatHandler* handler, char const* args)
    {
        sWorld->SetMotd(args);
        handler->PSendSysMessage(LANG_MOTD_NEW, args);
        return true;
    }

    // Set whether we accept new clients
    bool HandleServerSetClosedCommand(ChatHandler* handler, char const* args)
    {
        if (strncmp(args, "on", 3) == 0)
        {
            handler->SendSysMessage(LANG_WORLD_CLOSED);
            sWorld->SetClosed(true);
            return true;
        }
        else if (strncmp(args, "off", 4) == 0)
        {
            handler->SendSysMessage(LANG_WORLD_OPENED);
            sWorld->SetClosed(false);
            return true;
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    // Set the level of logging
    bool HandleServerSetLogLevelCommand(ChatHandler* /*handler*/, char const* args)
    {
        if (!*args)
            return false;

        char* type = strtok((char*)args, " ");
        char* name = strtok(NULL, " ");
        char* level = strtok(NULL, " ");

        if (!type || !name || !level || *name == '\0' || *level == '\0' || (*type != 'a' && *type != 'l'))
            return false;

        sLog->SetLogLevel(name, level, *type == 'l');
        return true;
    }

    // set diff time record interval
    bool HandleServerSetDiffTimeCommand(ChatHandler* /*handler*/, char const* args)
    {
        if (!*args)
            return false;

        char* newTimeStr = strtok((char*)args, " ");
        if (!newTimeStr)
            return false;

        int32 newTime = atoi(newTimeStr);
        if (newTime < 0)
            return false;

        sWorld->SetRecordDiffInterval(newTime);
        printf("Record diff every %u ms\n", newTime);

        return true;
    }

    bool HandleServerCrash(ChatHandler* /*handler*/, char const* /*args*/)
    {
        ABORT();
    }

}}}

void AddSC_server_commandscript()
{
    using namespace chat::command;
    using namespace chat::command::handler;

    new GenericCommandScriptLoader<ServerPlayerLimitCommand>("cmd_server_playerlimit");
    new GenericCommandScriptLoader<ServerSecurityLimitCommand>("cmd_server_securitylimit");

    new LegacyCommandScript("cmd_server_corpses",               HandleServerCorpsesCommand,         true);
    new LegacyCommandScript("cmd_server_exit",                  HandleServerExitCommand,            true);
    new LegacyCommandScript("cmd_server_info",                  HandleServerInfoCommand,            true);
    new LegacyCommandScript("cmd_server_motd",                  HandleServerMotdCommand,            true);
    new LegacyCommandScript("cmd_server_crash",                 HandleServerCrash,                  true);

    new LegacyCommandScript("cmd_server_idlerestart",           HandleServerIdleRestartCommand,     true);
    new LegacyCommandScript("cmd_server_idlerestart_cancel",    HandleServerShutDownCancelCommand,  true);

    new LegacyCommandScript("cmd_server_idleshutdown",          HandleServerIdleShutDownCommand,    true);
    new LegacyCommandScript("cmd_server_idleshutdown_cancel",   HandleServerShutDownCancelCommand,  true);

    new LegacyCommandScript("cmd_server_restart",               HandleServerRestartCommand,         true);
    new LegacyCommandScript("cmd_server_restart_cancel",        HandleServerShutDownCancelCommand,  true);

    new LegacyCommandScript("cmd_server_shutdown",              HandleServerShutDownCommand,        true);
    new LegacyCommandScript("cmd_server_shutdown_cancel",       HandleServerShutDownCancelCommand,  true);

    new LegacyCommandScript("cmd_server_set_difftime",          HandleServerSetDiffTimeCommand,     true);
    new LegacyCommandScript("cmd_server_set_loglevel",          HandleServerSetLogLevelCommand,     true);
    new LegacyCommandScript("cmd_server_set_motd",              HandleServerSetMotdCommand,         true);
    new LegacyCommandScript("cmd_server_set_closed",            HandleServerSetClosedCommand,       true);
}
