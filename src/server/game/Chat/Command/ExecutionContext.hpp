#pragma once

#include <boost/optional.hpp>

#include "Chat/Chat.h"

class WorldSession;

namespace locale {
    class LocalizedString;
}

namespace chat {
    namespace command {

        class GAME_API ExecutionContext
        {
        public:
            enum class Source
            {
                CHAT,
                CONSOLE,
            };

        private:
            Source source;
            ChatHandler& chatHandler;
            boost::optional<WorldSession&> session;

        public:
            static ExecutionContext ForChat(ChatHandler& chatHandler, WorldSession& session);
            static ExecutionContext ForConsole(ChatHandler& chatHandler);

        public:
            ExecutionContext(Source source, ChatHandler& chatHandler, boost::optional<WorldSession&> session);

            Source GetSource() const;
            ChatHandler& GetChatHandler();
            const ChatHandler& GetChatHandler() const;
            boost::optional<WorldSession&> GetSession();
            boost::optional<const WorldSession&> GetSession() const;

            AccountId GetAccountId() const;

            bool HasPermission(uint32 permissionId) const;
            uint32 GetSecurity() const;

            void SendSystemMessage(uint32 trinityStringEntry) const;

            template<typename... Args>
            void SendSystemMessage(uint32 trinityStringEntry, Args&&... args) const
            {
                GetChatHandler().PSendSysMessage(trinityStringEntry, std::forward<Args>(args)...);
            }

            template<typename... Args>
            void SendSystemMessage(const std::string& format, Args&&... args) const
            {
                GetChatHandler().PSendSysMessage(format.c_str(), std::forward<Args>(args)...);
            }

            const char* GetString(uint32 trinityStringId) const; // TODO -> const std::string&

            const std::string& GetString(const locale::LocalizedString& string) const;

            const std::string& GetExecutorName() const;
        };

    }
}
