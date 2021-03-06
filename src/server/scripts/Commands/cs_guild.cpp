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
Name: guild_commandscript
%Complete: 100
Comment: All guild related commands
Category: commandscripts
EndScriptData */

#include "Chat.h"
#include "Chat/Command/Commands/LegacyCommand.hpp"
#include "Language.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "Entities/Player/PlayerCache.hpp"

namespace chat { namespace command { namespace handler {

    /** \brief GM command level 3 - Create a guild.
    *
    * This command allows a GM (level 3) to create a guild.
    *
    * The "args" parameter contains the name of the guild leader
    * and then the name of the guild.
    *
    */
    bool HandleGuildCreateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        // if not guild name only (in "") then player name
        Player* target;
        if (!handler->extractPlayerTarget(*args != '"' ? (char*)args : NULL, &target))
            return false;

        char* tailStr = *args != '"' ? strtok(NULL, "") : (char*)args;
        if (!tailStr)
            return false;

        char* guildStr = handler->extractQuotedArg(tailStr);
        if (!guildStr)
            return false;

        std::string guildName = guildStr;

        if (target->GetGuildId())
        {
            handler->SendSysMessage(LANG_PLAYER_IN_GUILD);
            return true;
        }

        auto trans = CharacterDatabase.BeginTransaction();
        if(!sGuildMgr->CreateGuild(trans, target, guildName))
        {
            handler->SendSysMessage(LANG_GUILD_NOT_CREATED);
            handler->SetSentErrorMessage(true);
            return false;
        }
        CharacterDatabase.CommitTransaction(trans);

        return true;
    }

    bool HandleGuildDeleteCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* guildStr = handler->extractQuotedArg((char*)args);
        if (!guildStr)
            return false;

        std::string guildName = guildStr;

        auto targetGuild = sGuildMgr->GetGuildByName(guildName);
        if (!targetGuild)
            return false;

        auto trans = CharacterDatabase.BeginTransaction();
        targetGuild->Disband(trans);
        CharacterDatabase.CommitTransaction(trans);

        return true;
    }

    bool HandleGuildInviteCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        // if not guild name only (in "") then player name
        ObjectGuid targetGuid;
        if (!handler->extractPlayerTarget(*args != '"' ? (char*)args : NULL, NULL, &targetGuid))
            return false;

        char* tailStr = *args != '"' ? strtok(NULL, "") : (char*)args;
        if (!tailStr)
            return false;

        char* guildStr = handler->extractQuotedArg(tailStr);
        if (!guildStr)
            return false;

        std::string guildName = guildStr;
        auto targetGuild = sGuildMgr->GetGuildByName(guildName);
        if (!targetGuild)
            return false;

        // player's guild membership checked in AddMember before add
        auto trans = CharacterDatabase.BeginTransaction();
        bool success = targetGuild->AddMember(trans, targetGuid);
        CharacterDatabase.CommitTransaction(trans);
        return success;
    }

    bool HandleGuildUninviteCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        ObjectGuid targetGuid;
        if (!handler->extractPlayerTarget((char*)args, &target, &targetGuid))
            return false;

        uint32 guildId = target ? target->GetGuildId() : player::PlayerCache::GetGuildId(targetGuid);
        if (!guildId)
            return false;

        auto targetGuild = sGuildMgr->GetGuildById(guildId);
        if (!targetGuild)
            return false;

        auto trans = CharacterDatabase.BeginTransaction();
        targetGuild->DeleteMember(trans, targetGuid, false, true);
        CharacterDatabase.CommitTransaction(trans);
        return true;
    }

    bool HandleGuildRankCommand(ChatHandler* handler, char const* args)
    {
        char* nameStr;
        char* rankStr;
        handler->extractOptFirstArg((char*)args, &nameStr, &rankStr);
        if (!rankStr)
            return false;

        Player* target;
        ObjectGuid targetGuid;
        std::string target_name;
        if (!handler->extractPlayerTarget(nameStr, &target, &targetGuid, &target_name))
            return false;

        uint32 guildId = target ? target->GetGuildId() : player::PlayerCache::GetGuildId(targetGuid);
        if (!guildId)
            return false;

        auto targetGuild = sGuildMgr->GetGuildById(guildId);
        if (!targetGuild)
            return false;

        uint8 newRank = uint8(atoi(rankStr));
        auto trans = CharacterDatabase.BeginTransaction();
        bool success = targetGuild->ChangeMemberRank(trans, targetGuid, newRank);
        CharacterDatabase.CommitTransaction(trans);
        return success;
    }

    bool HandleGuildRenameCommand(ChatHandler* handler, char const* _args)
    {
        if (!*_args)
            return false;

        char *args = (char *)_args;

        char const* oldGuildStr = handler->extractQuotedArg(args);
        if (!oldGuildStr)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char const* newGuildStr = handler->extractQuotedArg(strtok(NULL, ""));
        if (!newGuildStr)
        {
            handler->SendSysMessage(LANG_INSERT_GUILD_NAME);
            handler->SetSentErrorMessage(true);
            return false;
        }

        auto guild = sGuildMgr->GetGuildByName(oldGuildStr);
        if (!guild)
        {
            handler->PSendSysMessage(LANG_COMMAND_COULDNOTFIND, oldGuildStr);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (sGuildMgr->GetGuildByName(newGuildStr))
        {
            handler->PSendSysMessage(LANG_GUILD_RENAME_ALREADY_EXISTS, newGuildStr);
            handler->SetSentErrorMessage(true);
            return false;
        }

        auto trans = CharacterDatabase.BeginTransaction();
        if (!guild->SetName(trans, newGuildStr))
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }
        CharacterDatabase.CommitTransaction(trans);

        handler->PSendSysMessage(LANG_GUILD_RENAME_DONE, oldGuildStr, newGuildStr);
        return true;
    }

    bool HandleJoinStartGuildCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        auto targetGuild = sGuildMgr->GetGuildByName(sWorld->GetStartguildName(player->GetOTeamId()));

        if (!targetGuild)
            return false;

        // player's guild membership checked in AddMember before add
        auto trans = CharacterDatabase.BeginTransaction();
        bool success = targetGuild->AddMember(trans, player->GetGUID());
        CharacterDatabase.CommitTransaction(trans);
        return success;
    }

    bool HandleGuildInfoCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        uint32 guildId;
        std::string guildName;
        std::string guildMasterName;
        Guild::GuildPtr guild;

        if (!*args)
        {
            // Look for the guild of the selected player or ourselves
            if ((target = handler->getSelectedPlayerOrSelf()))
                guild = target->GetGuild();
            else
                // getSelectedPlayerOrSelf will return null if there is no session
                // so target becomes nullptr if the command is ran through console
                // without specifying args.
                return false;
        }
        else if ((guildId = atoi(args))) // Try searching by Id
            guild = sGuildMgr->GetGuildById(guildId);
        else
        {
            // Try to extract a guild name
            char* tailStr = *args != '"' ? strtok(NULL, "") : (char*)args;
            if (!tailStr)
                return false;

            char* guildStr = handler->extractQuotedArg((char*)args);
            if (!guildStr)
                return false;

            guildName = guildStr;
            guild = sGuildMgr->GetGuildByName(guildName);
        }

        if (!guild)
            return false;

        // Display Guild Information
        handler->PSendSysMessage(LANG_GUILD_INFO_NAME, guild->GetName().c_str(), guild->GetId()); // Guild Id + Name
        guildMasterName = player::PlayerCache::GetName(guild->GetLeaderGUID());
        handler->PSendSysMessage(LANG_GUILD_INFO_GUILD_MASTER, guildMasterName.c_str(), guild->GetLeaderGUID().GetCounter()); // Guild Master

                                                                                                                    // Format creation date
        char createdDateStr[20];
        time_t createdDate = guild->GetCreatedDate();
        tm localTm;
        strftime(createdDateStr, 20, "%Y-%m-%d %H:%M:%S", localtime_r(&createdDate, &localTm));

        handler->PSendSysMessage(LANG_GUILD_INFO_CREATION_DATE, createdDateStr); // Creation Date
        handler->PSendSysMessage(LANG_GUILD_INFO_MEMBER_COUNT, guild->GetMemberCount()); // Number of Members
        handler->PSendSysMessage(LANG_GUILD_INFO_BANK_GOLD, guild->GetBankMoney() / 100 / 100); // Bank Gold (in gold coins)
        handler->PSendSysMessage(LANG_GUILD_INFO_MOTD, guild->GetMOTD().c_str()); // Message of the Day
        handler->PSendSysMessage(LANG_GUILD_INFO_EXTRA_INFO, guild->GetInfo().c_str()); // Extra Information
        return true;
    }

}}}

void AddSC_guild_commandscript()
{
    using namespace chat::command;
    using namespace chat::command::handler;

    new LegacyCommandScript("cmd_guild_create",     HandleGuildCreateCommand,       true);
    new LegacyCommandScript("cmd_guild_delete",     HandleGuildDeleteCommand,       true);
    new LegacyCommandScript("cmd_guild_invite",     HandleGuildInviteCommand,       true);
    new LegacyCommandScript("cmd_guild_uninvite",   HandleGuildUninviteCommand,     true);
    new LegacyCommandScript("cmd_guild_rank",       HandleGuildRankCommand,         true);
    new LegacyCommandScript("cmd_guild_rename",     HandleGuildRenameCommand,       true);
    new LegacyCommandScript("cmd_guild_info",       HandleGuildInfoCommand,         true);

    new LegacyCommandScript("cmd_joinstartguild",   HandleJoinStartGuildCommand,    false);
}
