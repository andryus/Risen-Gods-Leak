#include "Chat/Command/Commands/RootCommand.hpp"

#include "Language.h"

#include "Chat/Command/ExecutionContext.hpp"
#include "Chat/Command/Util/PrintCommands.hpp"

namespace chat {
    namespace command {

        RootCommand::RootCommand(CommandDefinition _definition) : MetaCommand(std::move(_definition)) {}

        void RootCommand::Initialize(std::string _fullName)
        {
            Command::Initialize(std::move(_fullName));

            for (auto& _command : GetSubCommands())
            {
                _command.Initialize(_command.GetName());
            }
        }

        void RootCommand::Initialize()
        {
            Initialize("");
        }

        bool RootCommand::IsAvailable(const ExecutionContext& context) const
        {
            return true;
        }

        void RootCommand::PrintHelp(ExecutionContext& _context) const
        {
            auto _availableSubCommands = GetAvailableSubCommands(_context);
            _context.SendSystemMessage(LANG_COMMAND_AVAILABLE_COMMANDS);
            util::PrintCommands(_availableSubCommands, _context, _context.GetString(LANG_COMMAND_LIST_FORMAT));
        }

    }
}
