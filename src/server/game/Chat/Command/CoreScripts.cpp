#include "Miscellaneous/Language.h"
#include "Scripting/ScriptMgr.h"

#include "Chat/Command/CommandMgr.hpp"
#include "Chat/Command/Commands/MetaCommand.hpp"
#include "Chat/Command/Commands/SimpleCommand.hpp"

#include "Chat/Command/GenericCommandScriptLoader.hpp"
#include "Chat/Command/ExecutionContext.hpp"

namespace chat { namespace command {

    class HelpCommand : public SimpleCommand
    {
    public:
        explicit HelpCommand(CommandDefinition definition) : SimpleCommand(std::move(definition), true) {}

        ExecutionResult Execute(const CommandString& command, ExecutionContext& context) override
        {
            CommandMgr::HandleHelp(command, context);
            return ExecutionResult::SUCCESS;
        }
    };

}}

class chat_command_world_script : public WorldScript
{
public:
    chat_command_world_script() : WorldScript("chat_command_world_script") {}

    void OnLoad() override
    {
        chat::command::CommandMgr::LoadCommands();
    }

    void OnConfigLoad(bool /*reload*/) override
    {
        chat::command::CommandMgr::LoadConfig();
    }
};

GAME_API void AddSC_command_scripts()
{
    using namespace chat::command;

    new GenericCommandScriptLoader<MetaCommand>("cmd_meta");
    new GenericCommandScriptLoader<HelpCommand>("cmd_help");

    new chat_command_world_script();
}
