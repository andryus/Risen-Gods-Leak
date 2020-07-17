#include "Chat/Command/Commands/MetaCommand.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/indirected.hpp>

#include "Miscellaneous/Language.h"

#include "Chat/Command/CommandMgr.hpp"
#include "Chat/Command/ExecutionContext.hpp"
#include "Chat/Command/Util/Detail.hpp"
#include "Chat/Command/Util/PrintCommands.hpp"

namespace chat {
    namespace command {

        MetaCommand::MetaCommand(CommandDefinition definition) : Command(std::move(definition)) {}

        void MetaCommand::Initialize(std::string _fullName)
        {
            Command::Initialize(std::move(_fullName));

            for (auto& _command : GetSubCommands())
            {
                _command.Initialize(GetFullName() + " " + _command.GetName());
            }
        }

        bool MetaCommand::HasSubCommand(const CommandString& subCommandName) const
        {
            const auto& index = subCommands.get<name_tag>();
            return index.find(subCommandName) != index.end();
        }

        boost::optional<Command&> MetaCommand::GetSubCommand(const CommandString& subCommandName)
        {
            const auto& index = subCommands.get<name_tag>();
            auto result = index.find(subCommandName);

            if (result != index.end())
                return **result;
            else
                return boost::none;
        }

        boost::optional<const Command&> MetaCommand::GetSubCommand(const CommandString& subCommandName) const
        {
            const auto& index = subCommands.get<name_tag>();
            auto result = index.find(subCommandName);

            if (result != index.end())
                return **result;
            else
                return boost::none;
        }

        void MetaCommand::AddSubCommand(std::unique_ptr<Command> command)
        {
            subCommands.insert(std::move(command));
        }

        bool MetaCommand::HasSubCommands() const
        {
            return subCommands.size() > 0;
        }

        bool MetaCommand::HasAvailableSubCommands(const ExecutionContext& context) const
        {
            for (auto& command : GetSubCommands())
                if (command.IsAvailable(context))
                    return true;

            return false;
        }

        MetaCommand::CommandRange MetaCommand::GetSubCommands()
        {
            return subCommands | boost::adaptors::indirected;
        }

        MetaCommand::ConstCommandRange MetaCommand::GetSubCommands() const
        {
            return subCommands | boost::adaptors::indirected;
        }

        MetaCommand::CommandRange MetaCommand::GetAvailableSubCommands(const ExecutionContext& context)
        {
            // dont call GetSubCommands() here for performance
            return subCommands | boost::adaptors::indirected | boost::adaptors::filtered([&](Command& command)
            {
                return command.IsAvailable(context);
            });
        }

        MetaCommand::ConstCommandRange MetaCommand::GetAvailableSubCommands(const ExecutionContext& context) const
        {
            // dont call GetSubCommands() here for performance
            return subCommands | boost::adaptors::indirected | boost::adaptors::filtered([&](Command& command)
            {
                return command.IsAvailable(context);
            });
        }

        MetaCommand::CommandRange MetaCommand::GetAvailableSubCommandsByAbbreviation(const CommandString& abbreviation, const ExecutionContext& context)
        {
            // dont call GetSubCommands() or GetAvailableSubCommands() here for performance
            return subCommands | boost::adaptors::indirected | boost::adaptors::filtered([&](Command& command)
            {
                return command.IsAvailable(context) &&
                    boost::istarts_with(command.GetName(), abbreviation) &&
                    !(command.GetMask() & CommandFlag::HIDDEN); // abbreviations are not allowed for hidden commands
            });
        }

        MetaCommand::ConstCommandRange MetaCommand::GetAvailableSubCommandsByAbbreviation(const CommandString& abbreviation, const ExecutionContext& context) const
        {
            // dont call GetSubCommands() or GetAvailableSubCommands() here for performance
            return subCommands | boost::adaptors::indirected | boost::adaptors::filtered([&](Command& command)
            {
                return command.IsAvailable(context) &&
                    boost::istarts_with(command.GetName(), abbreviation) &&
                    !(command.GetMask() & CommandFlag::HIDDEN); // abbreviations are not allowed for hidden commands
            });
        }

        bool MetaCommand::IsAvailable(const ExecutionContext& _context) const
        {
            return HasAvailableSubCommands(_context);
        }

        void MetaCommand::Handle(const CommandString& _command, ExecutionContext& _context)
        {
            if (!IsAvailable(_context))
            {
                _context.SendSystemMessage(LANG_COMMAND_NOT_AVAILABLE);
                return;
            }

            auto _splitted = util::SplitCommandString(_command);

            if (!_splitted) // no valid command name found
            {
                Handle_OnNoCommandNameFound(_command, _context);
                return;
            }

            const CommandString& _commandName = _splitted->first;
            const CommandString& _args = _splitted->second;

            if (auto subCommand = GetSubCommand(_commandName)) // perfect match with subcommand name
            {
                subCommand->Handle(_args, _context);
                return;
            }

            // no perfect match -> find best match
            auto _availableSubCommands = GetAvailableSubCommandsByAbbreviation(_commandName, _context);
            const auto _numAvailableSubCommands = boost::distance(_availableSubCommands);

            if (_numAvailableSubCommands == 0)
            {
                Handle_OnNoSubCommandFound(_command, _commandName, _context);
            }
            else if (_numAvailableSubCommands == 1) // exactly 1 matching subcommand
            {
                _availableSubCommands.begin()->Handle(_args, _context);
            }
            else // command abbreviation is ambiguous
            {
                if (CommandMgr::config.firstFit)
                {
                    _availableSubCommands.begin()->Handle(_args, _context);
                }
                else
                {
                    _context.SendSystemMessage(LANG_COMMAND_AMBIGUOUS, (GetFullName().size() > 0 ? GetFullName() + " " : "") + std::string(_commandName));
                    util::PrintCommands(_availableSubCommands, _context, _context.GetString(LANG_COMMAND_LIST_FORMAT));
                }
            }
        }

        void MetaCommand::Handle_OnNoCommandNameFound(const CommandString& _command, ExecutionContext& _context)
        {
            PrintHelp(_context);
        }

        void MetaCommand::Handle_OnNoSubCommandFound(const CommandString& _fullCommand, const CommandString& _commandName, ExecutionContext& _context)
        {
            _context.SendSystemMessage(LANG_COMMAND_NOT_FOUND, (GetFullName().size() > 0 ? GetFullName() + " " : "") + std::string(_commandName));
        }

        void MetaCommand::HandleHelp(const CommandString& _command, ExecutionContext& _context) const
        {
            if (!IsAvailable(_context))
            {
                _context.SendSystemMessage(LANG_COMMAND_NOT_AVAILABLE);
                return;
            }

            auto _splitted = util::SplitCommandString(_command);

            if (!_splitted) // no valid command name found
            {
                PrintHelp(_context);
                return;
            }

            const CommandString& _commandName = _splitted->first;
            const CommandString& _args = _splitted->second;

            if (auto subCommand = GetSubCommand(_commandName)) // perfect match with subcommand name
            {
                subCommand->HandleHelp(_args, _context);
                return;
            }

            // no perfect match
            auto _availableSubCommands = GetAvailableSubCommandsByAbbreviation(_commandName, _context);
            const auto _numAvailableSubCommands = boost::distance(_availableSubCommands);

            if (_numAvailableSubCommands == 0)
            {
                _context.SendSystemMessage(LANG_COMMAND_NOT_FOUND, (GetFullName().size() > 0 ? GetFullName() + " " : "") + std::string(_commandName));
            }
            else if (_numAvailableSubCommands == 1) // exactly 1 matching subcommand
            {
                _availableSubCommands.begin()->HandleHelp(_args, _context);
            }
            else // command abbreviation is ambiguous
            {
                if (CommandMgr::config.firstFit)
                {
                    _availableSubCommands.begin()->HandleHelp(_args, _context);
                }
                else
                {
                    _context.SendSystemMessage(LANG_COMMAND_AMBIGUOUS, (GetFullName().size() > 0 ? GetFullName() + " " : "") + std::string(_commandName));
                    util::PrintCommands(_availableSubCommands, _context, _context.GetString(LANG_COMMAND_LIST_FORMAT));
                }
            }
        }

        void MetaCommand::PrintHelp(ExecutionContext& _context) const
        {
            auto _availableSubCommands = GetAvailableSubCommands(_context);
            _context.SendSystemMessage(LANG_COMMAND_AVAILABLE_SUBCOMMANDS_FOR, GetFullName());
            util::PrintCommands(_availableSubCommands, _context, _context.GetString(LANG_COMMAND_LIST_FORMAT));
        }

    }
}
