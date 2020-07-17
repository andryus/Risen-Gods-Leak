#include "CommandMgr.hpp"

#include <boost/optional.hpp>
#include <unordered_set>

#include "Define.h"

#include "Configuration/Config.h"
#include "Locale/Locale.hpp"
#include "Locale/LocalizedString.hpp"
#include "Scripting/ScriptMgr.h"

#include "Chat/Chat.h"
#include "Chat/Command/ExecutionContext.hpp"
#include "Chat/Command/Commands/RootCommand.hpp"

namespace chat {
    namespace command {

        std::unique_ptr<RootCommand> CommandMgr::rootCommand;
        CommandMgr::config_t CommandMgr::config;

        void CommandMgr::LoadConfig()
        {
            config.firstFit = sConfigMgr->GetBoolDefault("Chat.Command.FirstFit", false);
        }

        void CommandMgr::LoadCommands()
        {
            struct ExtendedCommandDefinition
            {
                CommandDefinition definition;
                uint32 parentId;
                std::string scriptName;
            };

            TC_LOG_INFO("server.loading", "Loading chat commands ...");
            const uint32 startTime = getMSTime();

            // temporary storage to collect all needed infos
            std::unordered_map<uint32, ExtendedCommandDefinition> definitionStorage;

            // fetch definition
            {
                PreparedQueryResult result = WorldDatabase.Query(WorldDatabase.GetPreparedStatement(WORLD_SEL_COMMANDS));
                if (result)
                {
                    TC_LOG_INFO("server.loading", "Found " UI64FMTD " chat command definitions.", result->GetRowCount());

                    do {
                        Field* fields = result->Fetch();
                        const uint32 id = fields[0].GetUInt32();

                        if (id == 0)
                        {
                            TC_LOG_ERROR("commands", "Found command with id 0. There should not be a command with id 0. Skipping.");
                            continue;
                        }

                        ExtendedCommandDefinition exDef;

                        exDef.definition.id = id;
                        exDef.definition.sortingKey = fields[1].GetInt32();
                        exDef.definition.name = fields[2].GetString();
                        exDef.parentId = fields[3].GetUInt32();
                        exDef.definition.permission = fields[4].GetUInt32();
                        exDef.definition.mask = CommandMask(fields[5].GetUInt32());
                        exDef.scriptName = fields[6].GetString();

                        definitionStorage[id] = std::move(exDef);
                    } while (result->NextRow());
                }
                else
                    TC_LOG_INFO("server.loading", "Found 0 chat command definitions.");
            }

            // fetch help texts
            {
                PreparedQueryResult result = WorldDatabase.Query(WorldDatabase.GetPreparedStatement(WORLD_SEL_COMMAND_HELP));
                if (result)
                {
                    TC_LOG_INFO("server.loading", "Found " UI64FMTD " chat command help definitions.", result->GetRowCount());

                    do {
                        Field* fields = result->Fetch();

                        const uint32 commandId = fields[0].GetUInt32();

                        if (commandId == 0)
                        {
                            TC_LOG_ERROR("commands", "Found help definition for command with id 0. There should not be a command with id 0. Skipping.");
                            continue;
                        }

                        ExtendedCommandDefinition& exDef = definitionStorage.at(commandId); // must exist because of foreign key in db
                        const locale::Locale locale = locale::GetLocale(fields[1].GetUInt8());

                        exDef.definition.syntax.Set(locale, fields[2].GetString());
                        exDef.definition.description.Set(locale, fields[3].GetString());
                    } while (result->NextRow());
                }
                else
                    TC_LOG_INFO("server.loading", "Found 0 chat command help definitions.");
            }

            // temporary storage for commands used to build the command tree
            std::unordered_map<uint32, Command*> commandStorage;

            // create Command objects using the CommandScripts
            for (auto& exDefEntry : definitionStorage)
            {
                const uint32 id = exDefEntry.first;
                const auto& exDef = exDefEntry.second;

                auto scriptOpt = sScriptMgr->GetCommandScriptLoader(exDef.scriptName);

                if (!scriptOpt)
                {
                    TC_LOG_ERROR("commands", "Command script \"%s\" missing. Skipping command %u.", exDef.scriptName.c_str(), id);
                    continue;
                }

                // important note: definition might be invalid afterwards -> don't use it anymore
                Command* command = scriptOpt->CreateCommand(std::move(exDef.definition)).release();

                if (!command)
                {
                    TC_LOG_ERROR("commands", "Command script \"%s\" returned no command. Skipping command %u.", exDef.scriptName.c_str(), id);
                    continue;
                }

                commandStorage[id] = command;
            }

            // initialize root command
            rootCommand.reset(new RootCommand({}));
            commandStorage[0] = rootCommand.get(); // needs to be stored to build the tree

            // build command tree
            std::unordered_set<Command*> unusedCommands;
            for (auto& commandEntry : commandStorage)
            {
                if (commandEntry.first == 0)
                    continue; // root command has no parent

                const uint32 id = commandEntry.first;
                Command* command = commandEntry.second;
                const uint32 parentId = definitionStorage.at(id).parentId;

                auto parentItr = commandStorage.find(parentId);
                if (parentItr == commandStorage.end())
                {
                    // Parent command is guaranteed to exist in the database (-> foreign key), but it's initialization via the CommandScriptLoader
                    // may have failed and the command object therefore doesn't exist. In this case we don't need to log an error as the
                    // failed initialization has already done so, but a debug message may be useful for future problems.
                    TC_LOG_DEBUG("commands", "Parent command %u missing. Skipping command %u and all of its subcommands.", parentId, id);
                    unusedCommands.insert(command); // store for later cleanup. delay cleanup to ensure validity of all pointers within commandStorage
                    continue;
                }

                auto parentCommandOpt = parentItr->second->ToMetaCommand();
                if (!parentCommandOpt)
                {
                    TC_LOG_ERROR(
                        "commands", "Parent command %u exists but is not of type MetaCommand and therefore cannot have subcommands. "
                        "Skipping command %u and all of its subcommands.", parentId, id
                    );
                    unusedCommands.insert(command);
                    continue;
                }

                // transfairing "ownership" (or the responsibility to clean it up) here but still holding a normal pointer
                // (within commandStorage) to add subcommands to command later on
                parentCommandOpt->AddSubCommand(std::unique_ptr<Command>{ command });
            }

            rootCommand->Initialize();

            // cleanup all unused commands (with a missing or invalid parent command)
            // this does also automatically delete all subcommands via the destructor
            for (auto unusedCommand : unusedCommands)
            {
                delete unusedCommand;
            }

            TC_LOG_INFO("server.loading", ">> Loaded chat commands in %u ms.", GetMSTimeDiffToNow(startTime));
        }

        RootCommand& CommandMgr::GetRootCommand()
        {
            ASSERT(rootCommand);
            return *rootCommand;
        }

        bool CommandMgr::IsChatCommand(const CommandString& message)
        {
            if (message.size() < 2)
                return false;

            return message[0] == '.' && message[1] != '.' && message[1] != ' ';
        }

        void CommandMgr::HandleChat(const CommandString& _command, ChatHandler& _chatHandler)
        {
            if (_command.empty())
                return;

            ASSERT(_chatHandler.GetSession());

            ExecutionContext _context = ExecutionContext::ForChat(_chatHandler, *_chatHandler.GetSession());
            rootCommand->Handle(_command.substr(1), _context); // substr(1) to remove the leading dot
        }

        void CommandMgr::HandleConsole(const CommandString& _command, ChatHandler& _chatHandler)
        {
            if (_command.empty())
                return;

            ExecutionContext _context = ExecutionContext::ForConsole(_chatHandler);
            rootCommand->Handle(_command, _context);
        }

        void CommandMgr::HandleHelp(const CommandString& command, ExecutionContext& context)
        {
            rootCommand->HandleHelp(command, context);
        }

    }
}
