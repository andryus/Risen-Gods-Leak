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
Name: gm_commandscript
%Complete: 100
Comment: All gm related commands
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "Chat.h"
#include "Chat/Command/Commands/LegacyCommand.hpp"
#include "AccountMgr.h"
#include "Language.h"
#include "World.h"
#include "Player.h"
#include "Opcodes.h"
#include "WorldPacket.h"

namespace chat { namespace command { namespace handler {

    // Enables or disables hiding of the staff badge
    bool HandleGMChatCommand(ChatHandler* handler, char const* args)
    {
        WorldSession* session = handler->GetSession();
        Player* _player = session->GetPlayer();

        if (!*args)
        {
            if (session->HasPermission(rbac::RBAC_PERM_CHAT_USE_STAFF_BADGE) && _player->isGMChat())
                session->SendNotification(LANG_GM_CHAT_ON);
            else
                session->SendNotification(LANG_GM_CHAT_OFF);

            // Return false to prevent logging of an nothing changing command.
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string param = (char*)args;

        if (param == "on")
        {
            if (!_player->isGMChat())
            {
                _player->SetGMChat(true);
                session->SendNotification(LANG_GM_CHAT_ON);
                return true;
            }
            else
            {
                // nothing to do. GMs chat is already on. Return false to prevent logging of an nothing changing command.
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        if (param == "off")
        {
            if (_player->isGMChat())
            {
                _player->SetGMChat(false);
                session->SendNotification(LANG_GM_CHAT_OFF);
                return true;
            }
            else
            {
                // nothing to do. GMs chat is already on. Return false to prevent logging of an nothing changing command.
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    bool HandleGMFlyCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Player* target = handler->getSelectedPlayer();
        if (!target)
            target = handler->GetSession()->GetPlayer();

        WorldPacket data(12);
        if (strncmp(args, "on", 3) == 0)
            data.SetOpcode(SMSG_MOVE_SET_CAN_FLY);
        else if (strncmp(args, "off", 4) == 0)
            data.SetOpcode(SMSG_MOVE_UNSET_CAN_FLY);
        else
        {
            handler->SendSysMessage(LANG_USE_BOL);
            return false;
        }
        data << target->GetPackGUID();
        data << uint32(0);                                      // unknown
        target->SendMessageToSet(data, true);
        handler->PSendSysMessage(LANG_COMMAND_FLYMODE_STATUS, handler->GetNameLink(target).c_str(), args);
        return true;
    }

    bool HandleGMListIngameCommand(ChatHandler* handler, char const* /*args*/)
    {
        bool first = true;
        bool footer = false;

        boost::shared_lock<boost::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
        HashMapHolder<Player>::MapType const& m = sObjectAccessor->GetPlayers();
        for (HashMapHolder<Player>::MapType::const_iterator itr = m.begin(); itr != m.end(); ++itr)
        {
            uint32 itrSec = itr->second->GetSession()->GetSecurity();
            if ((itr->second->IsGameMaster() ||
                (itr->second->GetSession()->HasPermission(rbac::RBAC_PERM_COMMANDS_APPEAR_IN_GM_LIST) &&
                    itrSec <= sWorld->getIntConfig(CONFIG_GM_LEVEL_IN_GM_LIST))) &&
                    (!handler->GetSession() || itr->second->IsVisibleGloballyFor(handler->GetSession()->GetPlayer())))
            {
                if (first)
                {
                    first = false;
                    footer = true;
                    handler->SendSysMessage(LANG_GMS_ON_SRV);
                    handler->SendSysMessage("========================");
                }
                std::string const& name = itr->second->GetName();
                uint8 size = name.size();
                uint8 security = itrSec;
                uint8 max = ((16 - size) / 2);
                uint8 max2 = max;
                if ((max + max2 + size) == 16)
                    max2 = max - 1;
                if (handler->GetSession())
                    handler->PSendSysMessage("|    %s GMLevel %u", name.c_str(), security);
                else
                    handler->PSendSysMessage("|%*s%s%*s|   %u  |", max, " ", name.c_str(), max2, " ", security);
            }
        }
        if (footer)
            handler->SendSysMessage("========================");
        if (first)
            handler->SendSysMessage(LANG_GMS_NOT_LOGGED);
        return true;
    }

    /// Display the list of GMs
    bool HandleGMListFullCommand(ChatHandler* handler, char const* /*args*/)
    {
        ///- Get the accounts with GM Level >0
        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_GM_ACCOUNTS);
        stmt->setUInt8(0, uint8(SEC_MODERATOR));
        stmt->setInt32(1, int32(realm.Id.Realm));
        PreparedQueryResult result = LoginDatabase.Query(stmt);

        if (result)
        {
            handler->SendSysMessage(LANG_GMLIST);
            handler->SendSysMessage("========================");
            ///- Cycle through them. Display username and GM level
            do
            {
                Field* fields = result->Fetch();
                char const* name = fields[0].GetCString();
                uint8 security = fields[1].GetUInt8();
                uint8 max = (16 - strlen(name)) / 2;
                uint8 max2 = max;
                if ((max + max2 + strlen(name)) == 16)
                    max2 = max - 1;
                if (handler->GetSession())
                    handler->PSendSysMessage("|    %s GMLevel %u", name, security);
                else
                    handler->PSendSysMessage("|%*s%s%*s|   %u  |", max, " ", name, max2, " ", security);
            } while (result->NextRow());
            handler->SendSysMessage("========================");
        }
        else
            handler->PSendSysMessage(LANG_GMLIST_EMPTY);
        return true;
    }

    //Enable\Disable Invisible mode
    bool HandleGMVisibleCommand(ChatHandler* handler, char const* args)
    {
        Player* _player = handler->GetSession()->GetPlayer();

        if (!*args)
        {
            handler->PSendSysMessage(LANG_YOU_ARE, _player->isGMVisible() ? handler->GetTrinityString(LANG_VISIBLE) : handler->GetTrinityString(LANG_INVISIBLE));
            // Return false to prevent logging of an nothing changing command.
            handler->SetSentErrorMessage(true);
            return false;
        }

        const uint32 VISUAL_AURA = 37800;
        std::string param = (char*)args;

        if (param == "on")
        {
            if (!_player->isGMVisible())
            {
                if (_player->HasAura(VISUAL_AURA))
                    _player->RemoveAurasDueToSpell(VISUAL_AURA);

                _player->SetGMVisible(true);
                _player->UpdateObjectVisibility();
                handler->GetSession()->SendNotification(LANG_INVISIBLE_VISIBLE);
                return true;
            }
            else
            {
                // nothing to do. GMs is already visible. Return false to prevent logging of an nothing changing command.
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        if (param == "off")
        {
            if (_player->isGMVisible() || (_player->isGMVisibleToOtherGMs() && !_player->isGMVisible()))
            {
                _player->AddAura(VISUAL_AURA, _player);
                _player->SetGMVisible(false);
                _player->SetGMVisibleToOtherGMs(false);
                _player->UpdateObjectVisibility();
                handler->GetSession()->SendNotification(LANG_INVISIBLE_INVISIBLE);
                return true;
            }
            else
            {
                // nothing to do. GMs is already invinsible. Return false to prevent logging of an nothing changing command.
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        if (param == "gm")
        {
            if (_player->isGMVisible() || !_player->isGMVisibleToOtherGMs())
            {
                _player->AddAura(VISUAL_AURA, _player);
                _player->SetGMVisible(false);
                _player->SetGMVisibleToOtherGMs(true);
                _player->UpdateObjectVisibility();
                handler->GetSession()->SendNotification(LANG_GM_ONLY_VISIBLE_FOR_GMS);
                return true;
            }
            else
            {
                // nothing to do. GMs is already only visible to other GMs. Return false to prevent logging of an nothing changing command.
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    //Enable\Disable GM Mode
    bool HandleGMCommand(ChatHandler* handler, char const* args)
    {
        Player* _player = handler->GetSession()->GetPlayer();

        if (!*args)
        {
            handler->GetSession()->SendNotification(_player->IsGameMaster() ? LANG_GM_ON : LANG_GM_OFF);
            // Return false to prevent logging of an nothing changing command.
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string param = (char*)args;

        if (param == "on")
        {
            if (!_player->IsGameMaster())
            {
                _player->SetGameMaster(true);
                handler->GetSession()->SendNotification(LANG_GM_ON);
                _player->UpdateTriggerVisibility();
#ifdef _DEBUG_VMAPS
                VMAP::IVMapManager* vMapManager = VMAP::VMapFactory::createOrGetVMapManager();
                vMapManager->processCommand("stoplog");
#endif
                return true;
            }
            else
            {
                // nothing to do. GMs mode is already on. Return false to prevent logging of an nothing changing command.
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        if (param == "off")
        {
            if (_player->IsGameMaster())
            {
                _player->SetGameMaster(false);
                handler->GetSession()->SendNotification(LANG_GM_OFF);
                _player->UpdateTriggerVisibility();
#ifdef _DEBUG_VMAPS
                VMAP::IVMapManager* vMapManager = VMAP::VMapFactory::createOrGetVMapManager();
                vMapManager->processCommand("startlog");
#endif
                return true;
            }
            else
            {
                // nothing to do. GMs mode is already on. Return false to prevent logging of an nothing changing command.
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

}}}

void AddSC_gm_commandscript()
{
    using namespace chat::command;
    using namespace chat::command::handler;

    new LegacyCommandScript("cmd_gm",           HandleGMCommand,            false);
    new LegacyCommandScript("cmd_gm_chat",      HandleGMChatCommand,        false);
    new LegacyCommandScript("cmd_gm_fly",       HandleGMFlyCommand,         false);
    new LegacyCommandScript("cmd_gm_ingame",    HandleGMListIngameCommand,  true);
    new LegacyCommandScript("cmd_gm_list",      HandleGMListFullCommand,    true);
    new LegacyCommandScript("cmd_gm_visible",   HandleGMVisibleCommand,     false);
}
