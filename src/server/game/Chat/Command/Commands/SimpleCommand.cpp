#include "Chat/Command/Commands/SimpleCommand.hpp"

#include "Language.h"

#include "Chat/Command/ExecutionContext.hpp"
#include "Chat/Command/Util/Argument.hpp"

namespace chat {
    namespace command {
        SimpleCommand::SimpleCommand(CommandDefinition _definition, bool _availableOnConsole) :
            Command(std::move(_definition)),
            availableOnConsole(_availableOnConsole)
        {}

        bool SimpleCommand::IsAvailable(const ExecutionContext& context) const
        {
            return Command::IsAvailable(context) && (availableOnConsole || context.GetSource() != ExecutionContext::Source::CONSOLE);
        }

        void SimpleCommand::Handle(const CommandString& _command, ExecutionContext& _context)
        {
            if (!IsAvailable(_context))
            {
                _context.SendSystemMessage(LANG_COMMAND_NOT_AVAILABLE);
                return;
            }

            ExecutionResult result = this->Execute(_command, _context);

            switch (result)
            {
                case ExecutionResult::SUCCESS:
                {
                    Log(_command, _context);
                    break;
                }

                case ExecutionResult::FAILURE:
                {
                    _context.SendSystemMessage(LANG_COMMAND_EXECUTION_FAILED);
                    break;
                }

                case ExecutionResult::FAILURE_ERROR_MESSAGE_ALREADY_SENT:
                {
                    break;
                }

                case ExecutionResult::FAILURE_NOT_ALLOWED:
                {
                    _context.SendSystemMessage(LANG_COMMAND_NOT_ALLOWED);
                    break;
                }

                case ExecutionResult::FAILURE_MISSING_ARGUMENT:
                {
                    _context.SendSystemMessage(LANG_COMMAND_MISSING_ARGUMENT);
                    PrintHelp(_context);
                    break;
                }

                case ExecutionResult::FAILURE_INVALID_ARGUMENT:
                {
                    _context.SendSystemMessage(LANG_COMMAND_INVALID_ARGUMENT);
                    PrintHelp(_context);
                    break;
                }
            }
        }

        SimpleCommand::ExecutionResult SimpleCommand::Execute(const CommandString& command, ExecutionContext& context)
        {
            return Execute(util::ArgumentParser(command).Parse(), context);
        }
    }
}
