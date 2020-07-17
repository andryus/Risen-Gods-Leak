#include "Chat/Command/Commands/SimpleCommand.hpp"
#include "Chat/Command/GenericCommandScriptLoader.hpp"
#include "Debug/ScriptLogFile.h"
#include "ScriptCommands.h"
#include "Language.h"
#include "GameObject.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "ScriptAccessor.h"
#include "ScriptReloader.h"

#include "BaseHooks.h"

namespace chat::command
{
    static script::Scriptable* parseScriptTarget(ExecutionContext& context, std::string_view target)
    {
        if (target.empty())
        {
            if (WorldObject* object = context.GetChatHandler().getSelectedObject())
                return object;
            else
                return context.GetSession()->GetPlayer();
        }

        if (target.front() != '@')
            return nullptr;

        target = target.substr(1);

        if (target.empty() || target == "target")
        {
            if (WorldObject* object = context.GetChatHandler().getSelectedObject())
                return object;
        }
        else if (target == "player")
            return context.GetSession()->GetPlayer();
        else if (target == "tot")
        {
            if (Unit* unit = context.GetChatHandler().getSelectedUnit())
                return ObjectAccessor::GetWorldObject(*unit, unit->GetTarget());
        }
        else if (target == "go")
            return context.GetChatHandler().GetNearbyGameObject();

        return nullptr;
    }

    class ScriptLogCommand final : public SimpleCommand
    {
    public:

        explicit ScriptLogCommand(const CommandDefinition& definition) : SimpleCommand(std::move(definition), false) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
            if (Unit* unit = context.GetChatHandler().getSelectedUnit())
            {
                if (script::ScriptLogFile* scriptLogFile = unit->GetScriptLogFile())
                {
                    script::LogType log;
                    if (args.empty())
                        log = script::LogType::NORMAL;
                    else
                    {
                        const auto level = args.front();
                        if (level == "off")
                            log = script::LogType::NONE;
                        else if (level == "on")
                            log = script::LogType::NORMAL;
                        else if(level == "verbose")
                            log = script::LogType::VERBOSE;
                        else
                            return ExecutionResult::FAILURE_INVALID_ARGUMENT;
                    }

                    scriptLogFile->ChangeLogging(log);

                    return ExecutionResult::SUCCESS;
                }
                else
                    context.SendSystemMessage(LANG_COMMAND_SCRIPT_MISSING);
            }

            context.SendSystemMessage(LANG_SELECT_CREATURE);

            return ExecutionResult::FAILURE_ERROR_MESSAGE_ALREADY_SENT;
        }
    };

    class ScriptCallBase : public SimpleCommand
    {
    public:

        explicit ScriptCallBase(std::string_view module, const CommandDefinition& definition) :
            SimpleCommand(std::move(definition), false), module(module) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override final
        {
            if (args.empty())
                return ExecutionResult::FAILURE_MISSING_ARGUMENT;

            auto argList = make_span(args);
            
            script::Scriptable* target = nullptr;

            if (argList.front().GetStringView().front() == '@')
            {
                target = parseScriptTarget(context, argList.front().GetStringView());

                argList.pop_front();
            }
            else
                target = parseScriptTarget(context, "@");

            if (!target)
            {
                context.SendSystemMessage(LANG_SELECT_CHAR_OR_CREATURE);
                return ExecutionResult::FAILURE_ERROR_MESSAGE_ALREADY_SENT;
            }
            else if(argList.empty())
                return ExecutionResult::FAILURE_MISSING_ARGUMENT;
            
            std::string_view command = argList.front().GetStringView();
            argList.pop_front();

            std::vector<std::string> targets;
            std::vector<std::string_view> input;
            input.reserve(argList.size());

            for (const util::Argument arg : argList)
            {
                std::string_view str = arg.GetStringView();

                if (str.front() == '@')
                {
                    std::string targetGuid;

                    if (script::Scriptable* scriptable = parseScriptTarget(context, str))
                        if (WorldObject* object = dynamic_cast<WorldObject*>(scriptable))
                            targetGuid = std::to_string(object->GetGUID().GetRawValue());

                    str = targets.emplace_back(std::move(targetGuid));
                }

                input.push_back(str);
            }       

            const std::string name = script::CommandRegistry::FormatName(module, command);

            script::CommandRegistry& registry = script::commandRegistry();
            if (!registry.HasCommand(name))
            {
                const auto list = registry.GetList(name);

                std::string strOutput;
                if (!list.empty())
                    strOutput += " Did you mean:\n";

                for (const std::string_view command : list)
                {
                    strOutput += "- ";
                    strOutput += command;
                    strOutput += "\n";
                }

                context.SendSystemMessage("No command \"%s\" found in module \"%s.\"%s", command.data(), module.data(), strOutput.data());
            }
            else  
            {
                std::string outName = std::string(registry.GetName(name));

                if (const auto result = registry.Call(name, *target, input))
                {
                    context.SendSystemMessage("%s: %s", outName.data(), result->data());

                    return ExecutionResult::SUCCESS;
                }
                else
                    context.SendSystemMessage("%s failed to execute.", outName.data());
            }

            return ExecutionResult::FAILURE_ERROR_MESSAGE_ALREADY_SENT;
        }

        std::string_view module;

    };

    class ScriptCallCommand : public ScriptCallBase
    {
    public:
        explicit ScriptCallCommand(const CommandDefinition& definition) :
            ScriptCallBase("Do", definition) {}
    };

    class ScriptCallFilter : public ScriptCallBase
    {
    public:
        explicit ScriptCallFilter(const CommandDefinition& definition) :
            ScriptCallBase("If", definition) {}
    };

    class ScriptCallGetter : public ScriptCallBase
    {
    public:
        explicit ScriptCallGetter(const CommandDefinition& definition) :
            ScriptCallBase("Get", definition) {}
    };


    /**********************************************
    * cmd_script_reload
    ***********************************************/

    class ScriptReloadCommand final : public SimpleCommand
    {
    public:
        explicit ScriptReloadCommand(const CommandDefinition& definition) : SimpleCommand(std::move(definition), false) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
#ifdef ENABLE_SCRIPT_RELOAD
            sScriptReloader.Reload("script_content");

            return ExecutionResult::SUCCESS;
#else
            return ExecutionResult::FAILURE;
#endif
        }
    };

    /**********************************************
    * cmd_script_timers
    ***********************************************/

    class ScriptTimerCommand final : public SimpleCommand
    {
    public:

        explicit ScriptTimerCommand(const CommandDefinition& definition) : SimpleCommand(std::move(definition), false) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
            if (WorldObject* unit = context.GetChatHandler().getSelectedObject())
            {
                if (unit->HasScript())
                {
                    ((script::Scriptable&)*unit) <<= Fire::DebugFinishTimers;

                    return ExecutionResult::SUCCESS;
                }
            }

            return ExecutionResult::FAILURE;
        }
    };

    /**********************************************
    * cmd_script_info
    ***********************************************/

    class ScriptInfoCommand final : public SimpleCommand
    {
    public:

        explicit ScriptInfoCommand(const CommandDefinition& definition) : SimpleCommand(std::move(definition), false) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
            script::Scriptable* scriptable = parseScriptTarget(context, args.empty() ? "" : args.front().GetStringView());

            if (scriptable)
            {
                std::string info = "FScript Info for:\n  ";
                info += scriptable->FullName();

                info += "\nActive FScript: ";

                if (scriptable->HasScript())
                {
                    info += "true";

                    info += "\nModules:\n";

                    for (auto& [id, module] : scriptable->ScriptModules())
                    {
                        const auto content = module->Print();

                        if (!content.empty())
                            info += "- " + content + '\n';
                    }
                }
                else
                    info += "false";

                context.SendSystemMessage(info);

                return ExecutionResult::SUCCESS;
            }
            else
                context.SendSystemMessage(LANG_SELECT_CREATURE);

            return ExecutionResult::FAILURE_ERROR_MESSAGE_ALREADY_SENT;
        }
    };

}

GAME_API void AddSC_Scripting()
{
    using namespace chat;
    using namespace chat::command;

    new GenericCommandScriptLoader<ScriptLogCommand>("cmd_script_log");

    new GenericCommandScriptLoader<ScriptCallCommand>("cmd_call_do");
    new GenericCommandScriptLoader<ScriptCallFilter>("cmd_call_if");
    new GenericCommandScriptLoader<ScriptCallGetter>("cmd_call_get");

    new GenericCommandScriptLoader<ScriptReloadCommand>("cmd_script_reload");
    new GenericCommandScriptLoader<ScriptTimerCommand>("cmd_script_timers");
    new GenericCommandScriptLoader<ScriptInfoCommand>("cmd_script_info");
}
