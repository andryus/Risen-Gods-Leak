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
Name: arena_commandscript
%Complete: 100
Comment: All arena team related commands
Category: commandscripts
EndScriptData */

#include "ObjectMgr.h"
#include "Chat.h"
#include "Chat/Command/Commands/LegacyCommand.hpp"
#include "Chat/Command/Commands/SimpleCommand.hpp"
#include "Language.h"
#include "ArenaTeamMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Entities/Player/PlayerCache.hpp"
#include "RG/ArenaReplay/ArenaReplaySystem.h"

namespace chat { namespace command { namespace handler {
    static bool HandleArenaCreateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Player* target;
        if (!handler->extractPlayerTarget(*args != '"' ? (char*)args : NULL, &target))
            return false;

        char* tailStr = *args != '"' ? strtok(NULL, "") : (char*)args;
        if (!tailStr)
            return false;

        char* name = handler->extractQuotedArg(tailStr);
        if (!name)
            return false;

        char* typeStr = strtok(NULL, "");
        if (!typeStr)
            return false;

        int8 type = atoi(typeStr);
        if (sArenaTeamMgr->GetArenaTeamByName(name))
        {
            handler->PSendSysMessage(LANG_ARENA_ERROR_NAME_EXISTS, name);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (type == 2 || type == 3 || type == 5)
        {
            if (Player::GetArenaTeamIdFromDB(target->GetGUID(), type) != 0)
            {
                handler->PSendSysMessage(LANG_ARENA_ERROR_SIZE, target->GetName().c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }

            ArenaTeam* arena = new ArenaTeam();

            if (!arena->Create(target->GetGUID(), type, name, 4293102085, 101, 4293253939, 4, 4284049911))
            {
                delete arena;
                handler->SendSysMessage(LANG_BAD_VALUE);
                handler->SetSentErrorMessage(true);
                return false;
            }

            sArenaTeamMgr->AddArenaTeam(arena);
            handler->PSendSysMessage(LANG_ARENA_CREATE, arena->GetName().c_str(), arena->GetId(), arena->GetType(), arena->GetCaptain().GetCounter());
        }
        else
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }

    static bool HandleArenaDisbandCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 teamId = atoi((char*)args);
        if (!teamId)
            return false;

        ArenaTeam* arena = sArenaTeamMgr->GetArenaTeamById(teamId);

        if (!arena)
        {
            handler->PSendSysMessage(LANG_ARENA_ERROR_NOT_FOUND, teamId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (arena->IsFighting())
        {
            handler->SendSysMessage(LANG_ARENA_ERROR_COMBAT);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string name = arena->GetName();
        arena->Disband();
        if (handler->GetSession())
            TC_LOG_DEBUG("bg.arena", "GameMaster: %s [GUID: %u] disbanded arena team type: %u [Id: %u].",
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUID().GetCounter(), arena->GetType(), teamId);
        else
            TC_LOG_DEBUG("bg.arena", "Console: disbanded arena team type: %u [Id: %u].", arena->GetType(), teamId);

        delete(arena);

        handler->PSendSysMessage(LANG_ARENA_DISBAND, name.c_str(), teamId);
        return true;
    }

    static bool HandleArenaRenameCommand(ChatHandler* handler, char const* _args)
    {
        if (!*_args)
            return false;

        char* args = (char *)_args;

        char const* oldArenaStr = handler->extractQuotedArg(args);
        if (!oldArenaStr)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char const* newArenaStr = handler->extractQuotedArg(strtok(NULL, ""));
        if (!newArenaStr)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        ArenaTeam* arena = sArenaTeamMgr->GetArenaTeamByName(oldArenaStr);
        if (!arena)
        {
            handler->PSendSysMessage(LANG_AREAN_ERROR_NAME_NOT_FOUND, oldArenaStr);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (sArenaTeamMgr->GetArenaTeamByName(newArenaStr))
        {
            handler->PSendSysMessage(LANG_ARENA_ERROR_NAME_EXISTS, oldArenaStr);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (arena->IsFighting())
        {
            handler->SendSysMessage(LANG_ARENA_ERROR_COMBAT);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!arena->SetName(newArenaStr))
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage(LANG_ARENA_RENAME, arena->GetId(), oldArenaStr, newArenaStr);
        if (handler->GetSession())
            TC_LOG_DEBUG("bg.arena", "GameMaster: %s [GUID: %u] rename arena team \"%s\"[Id: %u] to \"%s\"",
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUID().GetCounter(), oldArenaStr, arena->GetId(), newArenaStr);
        else
            TC_LOG_DEBUG("bg.arena", "Console: rename arena team \"%s\"[Id: %u] to \"%s\"", oldArenaStr, arena->GetId(), newArenaStr);

        return true;
    }

    static bool HandleArenaCaptainCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* idStr;
        char* nameStr;
        handler->extractOptFirstArg((char*)args, &idStr, &nameStr);
        if (!idStr)
            return false;

        uint32 teamId = atoi(idStr);
        if (!teamId)
            return false;

        Player* target;
        ObjectGuid targetGuid;
        if (!handler->extractPlayerTarget(nameStr, &target, &targetGuid))
            return false;

        ArenaTeam* arena = sArenaTeamMgr->GetArenaTeamById(teamId);

        if (!arena)
        {
            handler->PSendSysMessage(LANG_ARENA_ERROR_NOT_FOUND, teamId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!target)
        {
            handler->PSendSysMessage(LANG_PLAYER_NOT_EXIST_OR_OFFLINE, nameStr);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (arena->IsFighting())
        {
            handler->SendSysMessage(LANG_ARENA_ERROR_COMBAT);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!arena->IsMember(targetGuid))
        {
            handler->PSendSysMessage(LANG_ARENA_ERROR_NOT_MEMBER, nameStr, arena->GetName().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (arena->GetCaptain() == targetGuid)
        {
            handler->PSendSysMessage(LANG_ARENA_ERROR_CAPTAIN, nameStr, arena->GetName().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        arena->SetCaptain(targetGuid);

        auto captain_info = player::PlayerCache::Get(arena->GetCaptain());
        if (!captain_info)
        {
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage(LANG_ARENA_CAPTAIN, arena->GetName().c_str(), arena->GetId(), captain_info.name.c_str(), target->GetName().c_str());
        if (handler->GetSession())
            TC_LOG_DEBUG("bg.arena", "GameMaster: %s [GUID: %u] promoted player: %s [GUID: %u] to leader of arena team \"%s\"[Id: %u]",
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUID().GetCounter(), target->GetName().c_str(), target->GetGUID().GetCounter(), arena->GetName().c_str(), arena->GetId());
        else
            TC_LOG_DEBUG("bg.arena", "Console: promoted player: %s [GUID: %u] to leader of arena team \"%s\"[Id: %u]",
                target->GetName().c_str(), target->GetGUID().GetCounter(), arena->GetName().c_str(), arena->GetId());

        return true;
    }

    static bool HandleArenaInfoCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 teamId = atoi((char*)args);
        if (!teamId)
            return false;

        ArenaTeam* arena = sArenaTeamMgr->GetArenaTeamById(teamId);

        if (!arena)
        {
            handler->PSendSysMessage(LANG_ARENA_ERROR_NOT_FOUND, teamId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage(LANG_ARENA_INFO_HEADER, arena->GetName().c_str(), arena->GetId(), arena->GetRating(), arena->GetType(), arena->GetType());
        for (ArenaTeam::MemberList::iterator itr = arena->m_membersBegin(); itr != arena->m_membersEnd(); ++itr)
            handler->PSendSysMessage(LANG_ARENA_INFO_MEMBERS, itr->Name.c_str(), itr->Guid.GetCounter(), itr->PersonalRating, (arena->GetCaptain() == itr->Guid ? "- Captain" : ""));

        return true;
    }

    static bool HandleArenaLookupCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        std::string namepart = args;
        std::wstring wnamepart;

        if (!Utf8toWStr(namepart, wnamepart))
            return false;

        wstrToLower(wnamepart);

        bool found = false;
        ArenaTeamMgr::ArenaTeamContainer::const_iterator i = sArenaTeamMgr->GetArenaTeamMapBegin();
        for (; i != sArenaTeamMgr->GetArenaTeamMapEnd(); ++i)
        {
            ArenaTeam* arena = i->second;

            if (Utf8FitTo(arena->GetName(), wnamepart))
            {
                if (handler->GetSession())
                {
                    handler->PSendSysMessage(LANG_ARENA_LOOKUP, arena->GetName().c_str(), arena->GetId(), arena->GetType(), arena->GetType());
                    found = true;
                    continue;
                }
            }
        }

        if (!found)
            handler->PSendSysMessage(LANG_AREAN_ERROR_NAME_NOT_FOUND, namepart.c_str());

        return true;
    }

    static bool HandleArenaRatingCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* arg1 = strtok((char*)args, " ");
        char* arg2 = strtok(NULL, " ");

        if (!arg2)
            return false;

        uint32 teamId = atoi(arg1);
        if (!teamId)
            return false;

        int16 rating = atoi(arg2);
        if (rating < 0)
            return false;

        ArenaTeam* arena = sArenaTeamMgr->GetArenaTeamById(teamId);

        if (!arena)
        {
            handler->PSendSysMessage(LANG_ARENA_ERROR_NOT_FOUND, teamId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        arena->SetRating(rating);
        arena->SaveToDB();
        arena->NotifyStatsChanged();
        return true;
    }

    static bool HandleArenaPersonalRatingCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* arg1 = strtok((char*)args, " ");
        char* arg2 = strtok(NULL, " ");
        char* arg3 = strtok(NULL, " ");

        if (!arg2 || !arg3)
            return false;

        uint32 teamId = atoi(arg2);
        if (!teamId)
            return false;

        int16 rating = atoi(arg3);
        if (rating < 0)
            return false;

        ArenaTeam* arena = sArenaTeamMgr->GetArenaTeamById(teamId);

        if (!arena)
        {
            handler->PSendSysMessage(LANG_ARENA_ERROR_NOT_FOUND, teamId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        ArenaTeamMember* member = arena->GetMember(std::string(arg1));

        if (!member)
            return false;

        member->SetPersonalRating(rating);
        arena->SaveToDB();
        arena->NotifyStatsChanged();
        return true;
    }
    
    static bool HandleArenaReplayWatchCommand(ChatHandler* handler, char const* args)
    {
        if (!sArenaReplayMgr->IsEnabled())
        {
            handler->SendSysMessage(LANG_ARENA_REPLAY_DISABLED);
            return true;
        }

        if (!*args)
            return false;

        char* arg1 = strtok((char*)args, " ");
        if (!arg1)
            return false;
        uint32 replayId = atoi(arg1);

        char* arg2 = strtok(NULL, " ");
        uint8 team = 0;
        if (arg2)
            team = atoi(arg2);

        if (!replayId)
            return false;

        Player* player = handler->GetSession()->GetPlayer();
        if (player->GetMap() && player->GetMap()->Instanceable() && player->GetMapId() != 35 /* SecretQueuing */)
            handler->PSendSysMessage("You have to leave the instance first.");
        else if (player->IsInCombat())
            handler->PSendSysMessage("You have to be out of combat.");
        else if (!sArenaReplayMgr->WatchReplay(*player, replayId, team))
            handler->PSendSysMessage("Unknown replay.");

        return true;
    }

    static bool HandleArenaReplayListCommand(ChatHandler* handler, char const* args)
    {
        if (!sArenaReplayMgr->IsEnabled())
        {
            handler->SendSysMessage(LANG_ARENA_REPLAY_DISABLED);
            return true;
        }

        auto replays = sArenaReplayMgr->GetLastReplayList(handler->GetSession()->GetPlayer()->GetGUID());

        if (replays.empty())
            handler->SendSysMessage(LANG_ARENA_REPLAY_NO_REPLAYS_FOUND);

        for (auto& kvp : replays)
            handler->SendSysMessage(kvp.second.c_str());

        return true;
    }

    static bool HandleArenaReplayBindCommand(ChatHandler* handler, char const* args)
    {
        Player* plr = handler->GetSession()->GetPlayer();
        if (!plr->IsWatchingReplay())
            return false;

        ObjectGuid target = plr->GetTarget();
        if (target && target != plr->GetGUID())
        {
            plr->SetClientControl(plr, false);
            plr->SetGuidValue(PLAYER_FARSIGHT, target);
            return true;
        }
        else
        {
            plr->SetGuidValue(PLAYER_FARSIGHT, ObjectGuid::Empty);
            plr->SetClientControl(plr, true);
            return true;
        }
    }

    static bool HandleArenaReplaySpeedCommand(ChatHandler* handler, char const* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player->IsWatchingReplay())
            return false;

        if (!*args)
            return false;

        float speed = atof(args);

        if (speed < 0.01f || speed > 5)
        {
            handler->PSendSysMessage("The value has to be in [0.01, 5.0].");
            return false;
        }

        ArenaReplayMap* map;
        if (player->GetMap() && (map = player->GetMap()->ToArenaReplayMap()))
        {
            map->SetReplaySpeed(speed);
            handler->PSendSysMessage("Set playback speed to %f.", speed);
        }

        return true;
    }
}}}

void AddSC_arena_commandscript()
{
    using namespace chat::command;
    using namespace chat::command::handler;

    new LegacyCommandScript("cmd_arena_create",             HandleArenaCreateCommand,           true);
    new LegacyCommandScript("cmd_arena_disband",            HandleArenaDisbandCommand,          true);
    new LegacyCommandScript("cmd_arena_rename",             HandleArenaRenameCommand,           true);
    new LegacyCommandScript("cmd_arena_captain",            HandleArenaCaptainCommand,          false);
    new LegacyCommandScript("cmd_arena_info",               HandleArenaInfoCommand,             true);
    new LegacyCommandScript("cmd_arena_lookup",             HandleArenaLookupCommand,           false);
    new LegacyCommandScript("cmd_arena_rating",             HandleArenaRatingCommand,           false);
    new LegacyCommandScript("cmd_arena_personalrating",     HandleArenaPersonalRatingCommand,   false);
    new LegacyCommandScript("cmd_arena_replay_watch",       HandleArenaReplayWatchCommand,      false);
    new LegacyCommandScript("cmd_arena_replay_list",        HandleArenaReplayListCommand,       false);
    new LegacyCommandScript("cmd_arena_replay_bind",        HandleArenaReplayBindCommand,       false);
    new LegacyCommandScript("cmd_arena_replay_speed",       HandleArenaReplaySpeedCommand,      false);
}
