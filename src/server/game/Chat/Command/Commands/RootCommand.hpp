#pragma once

#include "Chat/Command/Commands/MetaCommand.hpp"

namespace chat {
    namespace command {

        class RootCommand : public MetaCommand
        {
        public:
            explicit RootCommand(CommandDefinition definition);
            void Initialize(std::string fullName) override;
            void Initialize();

            bool IsAvailable(const ExecutionContext& context) const override;

            void PrintHelp(ExecutionContext& context) const override;
        };

    }
}
