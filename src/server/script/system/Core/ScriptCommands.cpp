#include "ScriptCommands.h"
#include <cctype>
#include <algorithm>

namespace script
{
#ifdef SCRIPT_DYNAMIC_RELOAD
    SCRIPT_API CommandRegistry _commandRegistry;
#endif

    CommandRegistry& commandRegistry()
    {
#ifdef SCRIPT_DYNAMIC_RELOAD
        return _commandRegistry;
#else
        static CommandRegistry registry;
        
        return registry;
#endif
    }


    CommandRegistry::StoredCommand::StoredCommand(std::string_view command) :
        command(command) {}

    CommandRegistry::StoredCommand::~StoredCommand()
    {
        commandRegistry().Remove(command);
    }

    void CommandRegistry::Remove(std::string_view command)
    {
        commands.erase(command);
    }

    CommandRegistry::StoredCommand CommandRegistry::Store(std::string_view command, CommandFunction function)
    {
        commands.emplace(command, function);

        return StoredCommand(command);
    }

    CommandRegistry::CallResult CommandRegistry::Call(std::string_view command, Scriptable& script, ArgList args)
    {
        auto itr = commands.find(command);
        if (itr != commands.end())
            return itr->second(script, args);

        return std::nullopt;
    }

    std::string CommandRegistry::FormatName(std::string_view module, std::string_view command)
    {
        std::string out = std::string(module);
        out += '.';
        out += command;

        return out;
    }

    bool CommandRegistry::HasCommand(std::string_view command) const
    {
        return commands.count(command) != 0;
    }

    std::string_view CommandRegistry::GetName(std::string_view command) const
    {
        return commands.find(command)->first;
    }

    std::vector<std::string_view> CommandRegistry::GetList(std::string_view command) const
    {
        auto range = commands.lower_bound((std::string(command) + "a"));

        auto upper = commands.upper_bound((std::string(command) + "z"));

        std::vector<std::string_view> list;
        for (auto itr = range; itr != upper; ++itr)
        {
            list.emplace_back(itr->first);
        }

        return list;
    }

}
