#pragma once

#include <functional>

#include "ScriptMgr.h"

#include "Chat/Command/Commands/MetaCommand.hpp"

class ChatHandler;

namespace chat {
    namespace command {

        class LegacyCommand : public MetaCommand
        {
        public:
            using Handler = std::function<bool(ChatHandler*, const char*)>;

        private:
            const Handler handler;
            const bool availableOnConsole;

        public:
            LegacyCommand(CommandDefinition definition, Handler handler, bool availableOnConsole);

            bool IsAvailable(const ExecutionContext& context) const override;

            void Handle_OnNoCommandNameFound(const CommandString& command, ExecutionContext& context) override;
            void Handle_OnNoSubCommandFound(const CommandString& fullCommand, const CommandString& commandName, ExecutionContext& context) override;

            void PrintHelp(ExecutionContext& context) const override;

        private:
            bool IsAvailableForExecution(const ExecutionContext& context) const;
            void ExecuteHandler(const CommandString& command, ExecutionContext& context);
        };


        class GAME_API LegacyCommandScript : public CommandScriptLoader
        {
        private:
            const LegacyCommand::Handler handler;
            const bool availableOnConsole;

        public:
            LegacyCommandScript(const char* name, LegacyCommand::Handler handler, bool availableOnConsole = true);
            std::unique_ptr<Command> CreateCommand(CommandDefinition definition) const override;
        };

    }
}
