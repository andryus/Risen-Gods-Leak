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

#ifndef TRINITYCORE_CHAT_H
#define TRINITYCORE_CHAT_H

#include "SharedDefines.h"
#include "StringFormat.h"
#include "WorldSession.h"
#include "RBAC.h"

#include <vector>

class ChatHandler;
class Creature;
class Group;
class Player;
class Unit;
class WorldSession;
class WorldObject;

struct GameTele;

class GAME_API ChatHandler
{
    public:
        WorldSession* GetSession() { return m_session; }
        explicit ChatHandler(WorldSession* session) : m_session(session), sentErrorMessage(false) { }
        virtual ~ChatHandler() { }

        // Builds chat packet and returns receiver guid position in the packet to substitute in whisper builders
        static size_t BuildChatPacket(WorldPacket& data, ChatMsg chatType, Language language, ObjectGuid senderGUID, ObjectGuid receiverGUID, std::string const& message, uint8 chatTag,
                                    std::string const& senderName = "", std::string const& receiverName = "",
                                    uint32 achievementId = 0, bool gmMessage = false, std::string const& channelName = "");

        // Builds chat packet and returns receiver guid position in the packet to substitute in whisper builders
        static size_t BuildChatPacket(WorldPacket& data, ChatMsg chatType, Language language, WorldObject const* sender, WorldObject const* receiver, std::string const& message, uint32 achievementId = 0, std::string const& channelName = "", LocaleConstant locale = DEFAULT_LOCALE);

        static char* LineFromMessage(char*& pos) { char* start = strtok(pos, "\n"); pos = NULL; return start; }

        // function with different implementation for chat/console
        virtual const char *GetTrinityString(int32 entry) const;

        virtual void SendSysMessage(const char *str) const;
        void SendSysMessage(int32 entry) const;

        template<typename... Args>
        void PSendSysMessage(const char* fmt, Args&&... args) const
        {
            SendSysMessage(Trinity::StringFormat(fmt, std::forward<Args>(args)...).c_str());
        }

        template<typename... Args>
        void PSendSysMessage(uint32 entry, Args&&... args) const
        {
            SendSysMessage(PGetParseString(entry, std::forward<Args>(args)...).c_str());
        }

        template<typename... Args>
        std::string PGetParseString(uint32 entry, Args&&... args) const
        {
            return Trinity::StringFormat(GetTrinityString(entry), std::forward<Args>(args)...);
        }

        bool isValidChatMessage(const char* msg) const;
        void SendGlobalSysMessage(const char *str);

        // function with different implementation for chat/console
        virtual bool HasPermission(uint32 permission) const { return m_session->HasPermission(permission); }
        virtual std::string GetNameLink() const { return GetNameLink(m_session->GetPlayer()); }
        virtual bool needReportToTarget(Player* chr) const;
        virtual LocaleConstant GetSessionDbcLocale() const;
        virtual int GetSessionDbLocaleIndex() const;

        bool HasLowerSecurity(Player* target, ObjectGuid guid, bool strong = false);
        bool HasLowerSecurityAccount(WorldSession* target, uint32 account, bool strong = false);

        void SendGlobalGMSysMessage(const char *str) const;
        Player*   getSelectedPlayer() const;
        Creature* getSelectedCreature() const;
        Unit*     getSelectedUnit() const;
        WorldObject* getSelectedObject();
        // Returns either the selected player or self if there is no selected player
        Player*   getSelectedPlayerOrSelf() const;

        char*     extractKeyFromLink(char* text, char const* linkType, char** something1 = NULL) const;
        char*     extractKeyFromLink(char* text, char const* const* linkTypes, int* found_idx, char** something1 = NULL) const;

        // if args have single value then it return in arg2 and arg1 == NULL
        void      extractOptFirstArg(char* args, char** arg1, char** arg2);
        char*     extractQuotedArg(char* args);

        uint32    extractSpellIdFromLink(char* text) const;
        ObjectGuid extractGuidFromLink(char* text) const;
        GameTele const* extractGameTeleFromLink(char* text, bool exactName = false) const;
        bool GetPlayerGroupAndGUIDByName(const char* cname, Player*& player, Group*& group, ObjectGuid& guid, bool offline = false);
        std::string extractPlayerNameFromLink(char* text) const;
        // select by arg (name/link) or in-game selection online/offline player or self if a creature is selected
        bool extractPlayerTarget(char* args, Player** player, ObjectGuid* player_guid = NULL, std::string* player_name = NULL);

        std::string playerLink(std::string const& name) const { return m_session ? "|cffffffff|Hplayer:"+name+"|h["+name+"]|h|r" : name; }
        std::string GetNameLink(Player* chr) const;

        GameObject* GetNearbyGameObject() const;
        GameObject* GetObjectGlobalyWithGuidOrNearWithDbGuid(uint32 lowguid, uint32 entry) const;
        bool HasSentErrorMessage() const { return sentErrorMessage; }
        void SetSentErrorMessage(bool val){ sentErrorMessage = val; }

    protected:
        explicit ChatHandler() : m_session(nullptr), sentErrorMessage(false) { }     // for CLI subclass

    private:
        WorldSession* m_session;                           // != NULL for chat command call and NULL for CLI command

        // common global flag
        bool sentErrorMessage;
};

class CliHandler : public ChatHandler
{
    public:
        typedef void Print(void*, char const*);
        explicit CliHandler(void* callbackArg, Print* zprint, uint32 accountId = 0, std::string accountName = "") : m_callbackArg(callbackArg), m_print(zprint) 
        { 
            m_accountId = accountId;
            m_accountName = accountName;
        }

        LocaleConstant GetSessionDbcLocale() const override;
        int GetSessionDbLocaleIndex() const override;

        const char *GetTrinityString(int32 entry) const override;
        void SendSysMessage(const char *str) const override;

        bool HasPermission(uint32 permission) const override;
        uint32 getAccountId() const { return m_accountId; }
        const std::string& getAccountName() const { return m_accountName; }
        std::string GetNameLink() const override;

        bool needReportToTarget(Player* chr) const override;

    private:
        void* m_callbackArg;
        Print* m_print;
        uint32 m_accountId;
        std::string m_accountName;
};

#endif
