#pragma once

#include <memory>

#include "Chat/Command/CommandDefines.hpp"

class ChatHandler;
class WorldSession;

namespace chat {
    namespace command {

        class RootCommand;
        class ExecutionContext;

        class GAME_API CommandMgr
        {
        private:
            static std::unique_ptr<RootCommand> rootCommand;

        public:
            static struct config_t {
                bool firstFit = false;
            } config;

        public:
            static void LoadConfig();
            static void LoadCommands();

            static RootCommand& GetRootCommand();

            static bool IsChatCommand(const CommandString& message);
            static void HandleChat(const CommandString& command, ChatHandler& chatHandler);
            static void HandleConsole(const CommandString& command, ChatHandler& chatHandler);

            static void HandleHelp(const CommandString& command, ExecutionContext& context);
        };

    }
}
