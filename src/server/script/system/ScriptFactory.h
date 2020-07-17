#pragma once

#include <map>
#include <string_view>
#include <optional>
#include <memory>
#include <functional>
#include <cctype>

#include "ScriptModuleFactory.h"
#include "ScriptRegistry.h"
#include "ScriptUtil.h"
#include "Define.h"
#include "Errors.h"

namespace script
{

    /******************************
    * TypeFactory
    ******************************/

    class Scriptable;

    class InvokationCtx;
    template<class Me>
    struct ConstructionCtx;

    template<class Me, class... Modules>
    struct ScriptBundle;

    template<class Me>
    struct ModuleFactory;
    template<class Me>
    using ScriptFactoryFunction = void(*)(ConstructionCtx<Me>);

    template<class Me>
    class Factory
    {
    public:

        static auto CreateScript(Me& me, std::string_view entry)
        {
            auto ctx = std::make_unique<InvokationCtx>(entry);

            auto itr = GetStoredScripts().find(entry);
            if (itr != GetStoredScripts().end())
            {
                auto states = itr->second(*ctx, me);

                return std::make_pair(std::move(ctx), std::move(states));
            }
            else
                return std::make_pair(std::move(ctx), StateContainer());
        }

        template<class... AddModules>
        static void RegisterScript(ScriptFactoryFunction<Me> construct, std::string_view name)
        {
            auto func = [script = std::move(construct), name](InvokationCtx& ctx, Scriptable& me)
            {
                auto states = CreateState<AddModules...>(ctx, me);

                script(ConstructionCtx<Me>(ctx, name));

                return states;
            };

            GetStoredScripts().emplace(name, std::move(func));
        }

        template<class... AddModules>
        static void RegisterDefaultScript(std::string_view name)
        {
            auto func = [](InvokationCtx& ctx, Scriptable& me)
            {
                return CreateState<AddModules...>(ctx, me);
            };

            GetStoredScripts().emplace(name, std::move(func));
        }

        static void UnregisterScript(std::string_view name)
        {
            ASSERT(GetStoredScripts().count(name));

            GetStoredScripts().erase(name);
        }

    private:

        static auto& GetStoredScripts()
        {
            static auto& map = scriptRegistry().GetContainer<Me>();

            return map;
        }

        template<class... AddModules>
        static auto CreateState(InvokationCtx& ctx, Scriptable& me)
        {
            auto states = ModuleFactory<Me>::CreateScriptState(ctx);

            ScriptBundle<Me, AddModules...>::Create(states, ctx);

            for (auto& [id, state] : states)
            {
                state->SetBase(me);
            }

            return states;
        }
    };

    template<class Me>
    struct Registration
    {
        ~Registration()
        {
            Factory<Me>::UnregisterScript(name);
        }

        template<class... AddModules>
        static auto Create(ScriptFactoryFunction<Me> script, std::string_view name)
        {
            Factory<Me>::template RegisterScript<AddModules...>(script, name);

            return Registration(name);
        }

        template<class... AddModules>
        static auto CreateDefault(std::string_view name)
        {
            Factory<Me>::template RegisterDefaultScript<AddModules...>(name);

            return Registration(name);
        }

    private:

        Registration(std::string_view name) :
            name(name) {}

        std::string_view name;
    };

}
