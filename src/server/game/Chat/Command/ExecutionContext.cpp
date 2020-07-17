#include "Chat/Command/ExecutionContext.hpp"

#include "Common.h"

#include "Locale/Locale.hpp"
#include "Locale/LocalizedString.hpp"

#include "Entities/Player/Player.h"

namespace chat {
    namespace command {
        
        ExecutionContext ExecutionContext::ForChat(ChatHandler& chatHandler, WorldSession& session)
        {
            return ExecutionContext(Source::CHAT, chatHandler, session);
        }

        ExecutionContext ExecutionContext::ForConsole(ChatHandler& chatHandler)
        {
            return ExecutionContext(Source::CONSOLE, chatHandler, boost::none);
        }

        ExecutionContext::ExecutionContext(Source _source, ChatHandler& _chatHandler, boost::optional<WorldSession&> _session) :
            source(_source),
            chatHandler(_chatHandler),
            session(_session)
        {}

        boost::optional<WorldSession&> ExecutionContext::GetSession()
        {
            return boost::optional<WorldSession&>{ session };
        }

        boost::optional<const WorldSession&> ExecutionContext::GetSession() const
        {
            return boost::optional<const WorldSession&>{ session };
        }

        AccountId ExecutionContext::GetAccountId() const
        {
            return GetSource() == Source::CHAT ? GetSession()->GetAccountId() : 0;
        }

        ExecutionContext::Source ExecutionContext::GetSource() const
        {
            return source;
        }

        ChatHandler& ExecutionContext::GetChatHandler()
        {
            return chatHandler;
        }

        const ChatHandler& ExecutionContext::GetChatHandler() const
        {
            return chatHandler;
        }

        bool ExecutionContext::HasPermission(uint32 permissionId) const
        {
            return GetChatHandler().HasPermission(permissionId);
        }

        uint32 ExecutionContext::GetSecurity() const
        {
            return (GetSource() == Source::CHAT) ? GetSession()->GetSecurity() : SEC_CONSOLE;
        }

        void ExecutionContext::SendSystemMessage(uint32 trinityStringEntry) const
        {
            GetChatHandler().SendSysMessage(trinityStringEntry);
        }

        const char* ExecutionContext::GetString(uint32 trinityStringId) const
        {
            return GetChatHandler().GetTrinityString(trinityStringId);
        }

        const std::string& ExecutionContext::GetString(const locale::LocalizedString& string) const
        {
            locale::Locale loc = locale::GetLocale(GetChatHandler().GetSessionDbLocaleIndex());
            return string.In(loc);
        }

        const std::string& ExecutionContext::GetExecutorName() const
        {
            static std::string CONSOLE = "Console";
            return (GetSource() == Source::CHAT) ? GetSession()->GetPlayer()->GetName() : CONSOLE;
        }
    }
}
