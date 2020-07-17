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
Name: instance_commandscript
%Complete: 100
Comment: All instance related commands
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "Chat.h"
#include "Chat/Command/Commands/LegacyCommand.hpp"
#include "Group.h"
#include "InstanceSaveMgr.h"
#include "InstanceScript.h"
#include "InstanceMap.h"
#include "MapManager.h"
#include "Player.h"
#include "Language.h"

namespace chat { namespace command { namespace handler {

    std::string GetTimeString(uint64 time)
    {
        uint64 days = time / DAY, hours = (time % DAY) / HOUR, minute = (time % HOUR) / MINUTE;
        std::ostringstream ss;
        if (days)
            ss << days << "d ";
        if (hours)
            ss << hours << "h ";
        ss << minute << 'm';
        return ss.str();
    }

    bool HandleInstanceListBindsCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->getSelectedPlayer();
        if (!player)
            player = handler->GetSession()->GetPlayer();

        uint32 counter = 0;
        for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        {
            Player::BoundInstancesMap &binds = player->GetBoundInstances(Difficulty(i));
            for (Player::BoundInstancesMap::const_iterator itr = binds.begin(); itr != binds.end(); ++itr)
            {
                InstanceSave* save = itr->second.save;
                std::string timeleft = GetTimeString(save->GetResetTime() - time(NULL));
                handler->PSendSysMessage(LANG_COMMAND_LIST_BIND_INFO, itr->first, save->GetInstanceId(), itr->second.perm ? "yes" : "no", save->GetDifficulty(), save->CanReset() ? "yes" : "no", timeleft.c_str());
                counter++;
            }
        }
        handler->PSendSysMessage(LANG_COMMAND_LIST_BIND_PLAYER_BINDS, counter);

        counter = 0;
        if (Group* group = player->GetGroup())
        {
            for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
            {
                Group::BoundInstancesMap &binds = group->GetBoundInstances(Difficulty(i));
                for (Group::BoundInstancesMap::const_iterator itr = binds.begin(); itr != binds.end(); ++itr)
                {
                    InstanceSave* save = itr->second.save;
                    std::string timeleft = GetTimeString(save->GetResetTime() - time(NULL));
                    handler->PSendSysMessage(LANG_COMMAND_LIST_BIND_INFO, itr->first, save->GetInstanceId(), itr->second.perm ? "yes" : "no", save->GetDifficulty(), save->CanReset() ? "yes" : "no", timeleft.c_str());
                    counter++;
                }
            }
        }
        handler->PSendSysMessage(LANG_COMMAND_LIST_BIND_GROUP_BINDS, counter);

        return true;
    }

    bool HandleInstanceUnbindCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Player* player = handler->getSelectedPlayer();
        if (!player)
            player = handler->GetSession()->GetPlayer();

        char* map = strtok((char*)args, " ");
        char* pDiff = strtok(NULL, " ");
        int8 diff = -1;
        if (pDiff)
            diff = atoi(pDiff);
        uint16 counter = 0;
        uint16 MapId = 0;

        if (strcmp(map, "all") != 0)
        {
            MapId = uint16(atoi(map));
            if (!MapId)
                return false;
        }

        for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        {
            Player::BoundInstancesMap &binds = player->GetBoundInstances(Difficulty(i));
            for (Player::BoundInstancesMap::iterator itr = binds.begin(); itr != binds.end();)
            {
                InstanceSave* save = itr->second.save;
                if (itr->first != player->GetMapId() && (!MapId || MapId == itr->first) && (diff == -1 || diff == save->GetDifficulty()))
                {
                    std::string timeleft = GetTimeString(save->GetResetTime() - time(NULL));
                    handler->PSendSysMessage(LANG_COMMAND_INST_UNBIND_UNBINDING, itr->first, save->GetInstanceId(), itr->second.perm ? "yes" : "no", save->GetDifficulty(), save->CanReset() ? "yes" : "no", timeleft.c_str());
                    player->UnbindInstance(itr, Difficulty(i));
                    counter++;
                }
                else
                    ++itr;
            }
        }
        handler->PSendSysMessage(LANG_COMMAND_INST_UNBIND_UNBOUND, counter);

        return true;
    }

    bool HandleInstanceStatsCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->PSendSysMessage(LANG_COMMAND_INST_STAT_LOADED_INST, sMapMgr->GetNumInstances());
        handler->PSendSysMessage(LANG_COMMAND_INST_STAT_PLAYERS_IN, sMapMgr->GetNumPlayersInInstances());
        handler->PSendSysMessage(LANG_COMMAND_INST_STAT_SAVES, sInstanceSaveMgr->GetNumInstanceSaves());
        handler->PSendSysMessage(LANG_COMMAND_INST_STAT_PLAYERSBOUND, sInstanceSaveMgr->GetNumBoundPlayersTotal());
        handler->PSendSysMessage(LANG_COMMAND_INST_STAT_GROUPSBOUND, sInstanceSaveMgr->GetNumBoundGroupsTotal());

        return true;
    }

    bool HandleInstanceSaveDataCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        Map* map = player->GetMap();
        if (!map->IsDungeon())
        {
            handler->PSendSysMessage(LANG_NOT_DUNGEON);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!((InstanceMap*)map)->GetInstanceScript())
        {
            handler->PSendSysMessage(LANG_NO_INSTANCE_DATA);
            handler->SetSentErrorMessage(true);
            return false;
        }

        ((InstanceMap*)map)->GetInstanceScript()->SaveToDB();

        return true;
    }

    bool HandleInstanceUnloadCommand(ChatHandler* handler, char const* args)
    {
        bool is_unlocked = handler->GetSession()->HasPermission(rbac::RBAC_PERM_RG_COMMAND_INSTANCE_UNLOAD_UNLOCKED);

        uint32 instance_id = 0;
        uint32 map_id = 0;

        auto send_message = [&handler, &instance_id, &map_id](TrinityStrings message)
        {
            handler->PSendSysMessage(message);

            TC_LOG_INFO("misc", "Instance: Unloaded instance %u (map: %u) by %s (GUID: %u) result %s (%u)",
                instance_id, map_id,
                handler->GetSession()->GetPlayerName().c_str(),
                handler->GetSession()->GetPlayer() ? handler->GetSession()->GetPlayer()->GetGUID().GetCounter() : 0,
                message == TrinityStrings::LANG_COMMAND_INSTANCE_UNLOAD_SUCCESSFUL ? "successful" : "failed",
                message);

            if (message != TrinityStrings::LANG_COMMAND_INSTANCE_UNLOAD_SUCCESSFUL)
            {
                handler->SetSentErrorMessage(true);
                return false;
            }
            else
                return true;
        };


        if (!args)
            return false;

        instance_id = (uint32)std::atoi(args);

        if (instance_id != 0)
        {
            bool id_found = false;

            if (!is_unlocked)
            {
                Player* player = handler->GetSession()->GetPlayer();
                if (!player)
                    return false;

                // check group lead
                if (Group* group = player->GetGroup())
                    if (!group->IsLeader(player->GetGUID()))
                        return send_message(TrinityStrings::LANG_COMMAND_INSTANCE_UNLOAD_NOT_LEAD);

                // try to find id
                for (uint8 difficulty = 0; difficulty < MAX_DIFFICULTY && !id_found; ++difficulty)
                {
                    const Player::BoundInstancesMap& binds = player->GetBoundInstances((Difficulty)difficulty);
                    for (const auto& pair : binds)
                    {
                        const InstancePlayerBind& bind = pair.second;

                        // do not respect non permanent binds
                        if (!bind.perm)
                            continue;

                        if (bind.save->GetInstanceId() != instance_id)
                            continue;

                        id_found = true;
                        map_id = bind.save->GetMapId();
                        break;
                    }
                }
            }
            else
            {
                if (InstanceSave* save = sInstanceSaveMgr->GetInstanceSave(instance_id))
                {
                    id_found = true;
                    map_id = save->GetMapId();
                }
            }

            if (!id_found)
                return send_message(TrinityStrings::LANG_COMMAND_INSTANCE_UNLOAD_ID_NOT_FOUND);

            Map* map = sMapMgr->FindMap(map_id, instance_id);
            if (!map)
                return send_message(TrinityStrings::LANG_COMMAND_INSTANCE_UNLOAD_MAP_NOT_LOADED);

            if (!map->Instanceable())
                return send_message(TrinityStrings::LANG_COMMAND_INSTANCE_UNLOAD_NOT_AN_INSTANCE);

            InstanceMap* instance = dynamic_cast<InstanceMap*>(map);
            if (!instance)
                return send_message(TrinityStrings::LANG_COMMAND_INSTANCE_UNLOAD_NOT_AN_INSTANCE);

            if (!map->GetPlayers().empty())
                return send_message(TrinityStrings::LANG_COMMAND_INSTANCE_UNLOAD_NOT_EMPTY);

            if (!is_unlocked)
            {
                if (!instance->IsRaid())
                    return send_message(TrinityStrings::LANG_COMMAND_INSTANCE_UNLOAD_NOT_RAID);

                if (!instance->CanForceUnload())
                    return send_message(TrinityStrings::LANG_COMMAND_INSTANCE_UNLOAD_COOLDOWN);
            }

            if (!instance->ForceUnload())
                return send_message(TrinityStrings::LANG_COMMAND_INSTANCE_UNLOAD_FAILED);

            return send_message(TrinityStrings::LANG_COMMAND_INSTANCE_UNLOAD_SUCCESSFUL);
        }
        else
            return false;
    }

    bool HandleInstanceSetBossStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* param1 = strtok((char*)args, " ");
        char* param2 = strtok(nullptr, " ");
        char* param3 = strtok(nullptr, " ");
        uint32 encounterId = 0;
        int32 state = 0;
        Player* player = nullptr;
        std::string playerName;

        // Character name must be provided when using this from console.
        if (!param2 || (!param3 && !handler->GetSession()))
        {
            handler->PSendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!param3)
            player = handler->GetSession()->GetPlayer();
        else
        {
            playerName = param3;
            if (normalizePlayerName(playerName))
                player = sObjectAccessor->FindPlayerByName(playerName);
        }

        if (!player)
        {
            handler->PSendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Map* map = player->GetMap();
        if (!map->IsDungeon())
        {
            handler->PSendSysMessage(LANG_NOT_DUNGEON);
            handler->SetSentErrorMessage(true);
            return false;
        }

        InstanceScript* instance = map->ToInstanceMap()->GetInstanceScript();

        if (!instance)
        {
            handler->PSendSysMessage(LANG_NO_INSTANCE_DATA);
            handler->SetSentErrorMessage(true);
            return false;
        }

        encounterId = atoi(param1);
        state = atoi(param2);

        // Reject improper values.
        if (state < 0 || state > TO_BE_DECIDED || encounterId > instance->GetEncounterCount())
        {
            handler->PSendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        instance->SetBossState(encounterId, (EncounterState)state);
        handler->PSendSysMessage(LANG_COMMAND_INST_SET_BOSS_STATE, encounterId, state);
        return true;
    }

    bool HandleInstanceGetBossStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* param1 = strtok((char*)args, " ");
        char* param2 = strtok(nullptr, " ");
        uint32 encounterId = 0;
        Player* player = nullptr;
        std::string playerName;

        // Character name must be provided when using this from console.
        if (!param1 || (!param2 && !handler->GetSession()))
        {
            handler->PSendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!param2)
            player = handler->GetSession()->GetPlayer();
        else
        {
            playerName = param2;
            if (normalizePlayerName(playerName))
                player = sObjectAccessor->FindPlayerByName(playerName);
        }

        if (!player)
        {
            handler->PSendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Map* map = player->GetMap();
        if (!map->IsDungeon())
        {
            handler->PSendSysMessage(LANG_NOT_DUNGEON);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!map->ToInstanceMap()->GetInstanceScript())
        {
            handler->PSendSysMessage(LANG_NO_INSTANCE_DATA);
            handler->SetSentErrorMessage(true);
            return false;
        }

        encounterId = atoi(param1);

        if (encounterId > map->ToInstanceMap()->GetInstanceScript()->GetEncounterCount())
        {
            handler->PSendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint8 state = map->ToInstanceMap()->GetInstanceScript()->GetBossState(encounterId);
        handler->PSendSysMessage(LANG_COMMAND_INST_GET_BOSS_STATE, encounterId, state);
        return true;
    }

}}}

void AddSC_instance_commandscript()
{
    using namespace chat::command;
    using namespace chat::command::handler;

    new LegacyCommandScript("cmd_instance_listbinds",       HandleInstanceListBindsCommand,     false);
    new LegacyCommandScript("cmd_instance_unbind",          HandleInstanceUnbindCommand,        false);
    new LegacyCommandScript("cmd_instance_stats",           HandleInstanceStatsCommand,         true);
    new LegacyCommandScript("cmd_instance_savedata",        HandleInstanceSaveDataCommand,      false);
    new LegacyCommandScript("cmd_instance_unload",          HandleInstanceUnloadCommand,        false);
    new LegacyCommandScript("cmd_instance_setbossstate",    HandleInstanceSetBossStateCommand,  true);
    new LegacyCommandScript("cmd_instance_getbossstate",    HandleInstanceGetBossStateCommand,  true);
}
