#include "Chat/Command/Commands/LegacyCommand.hpp"

#include "Language.h"

#include "Chat/Command/ExecutionContext.hpp"
#include "Chat/Command/Util/Detail.hpp"
#include "Chat/Command/Util/PrintCommands.hpp"

namespace chat {
    namespace command {

        LegacyCommand::LegacyCommand(CommandDefinition _definition, Handler _handler, bool _availableOnConsole) :
            MetaCommand(std::move(_definition)),
            handler(_handler),
            availableOnConsole(_availableOnConsole)
        {}

        bool LegacyCommand::IsAvailableForExecution(const ExecutionContext& context) const
        {
            return Command::IsAvailable(context) && (availableOnConsole || context.GetSource() != ExecutionContext::Source::CONSOLE);
        }

        bool LegacyCommand::IsAvailable(const ExecutionContext& context) const
        {
            return IsAvailableForExecution(context) || MetaCommand::IsAvailable(context);
        }

        void LegacyCommand::Handle_OnNoCommandNameFound(const CommandString& _command, ExecutionContext& _context)
        {
            ExecuteHandler(_command, _context);
        }

        void LegacyCommand::Handle_OnNoSubCommandFound(const CommandString& _fullCommand, const CommandString& /*_commandName*/, ExecutionContext& _context)
        {
            ExecuteHandler(_fullCommand, _context);
        }

        void LegacyCommand::PrintHelp(ExecutionContext& _context) const
        {
            if (IsAvailableForExecution(_context))
            {
                _context.SendSystemMessage(
                    LANG_COMMAND_HELP_TEXT,
                    GetFullName(),
                    _context.GetString(GetSyntax()),
                    _context.GetString(GetDescription())
                );
            }

            auto _availableSubCommands = GetAvailableSubCommands(_context);
            auto _numAvailableSubCommands = boost::distance(_availableSubCommands);
            if (_numAvailableSubCommands > 0)
            {
                _context.SendSystemMessage(LANG_COMMAND_AVAILABLE_SUBCOMMANDS);
                util::PrintCommands(_availableSubCommands, _context, _context.GetString(LANG_COMMAND_LIST_FORMAT));
            }
        }

        void LegacyCommand::ExecuteHandler(const CommandString& _command, ExecutionContext& _context)
        {
            if (!IsAvailableForExecution(_context))
            {
                _context.SendSystemMessage(LANG_COMMAND_NOT_AVAILABLE);
                return;
            }

            bool success = handler(&_context.GetChatHandler(), std::string(_command).c_str());

            if (!success && !_context.GetChatHandler().HasSentErrorMessage())
            {
                _context.SendSystemMessage(LANG_COMMAND_EXECUTION_FAILED);
                PrintHelp(_context);
            }

            if (success)
                Log(_command, _context);
        }

        LegacyCommandScript::LegacyCommandScript(const char* _name, LegacyCommand::Handler _handler, bool _availableOnConsole) :
            CommandScriptLoader(_name),
            handler(_handler),
            availableOnConsole(_availableOnConsole)
        {}

        std::unique_ptr<Command> LegacyCommandScript::CreateCommand(CommandDefinition definition) const
        {
            return std::make_unique<LegacyCommand>(std::move(definition), handler, availableOnConsole);
        }

    }
}
