#pragma once
#include <functional>
#include <map>
#include <typeindex>

#include "ScriptModuleFactory.h"
#include "ScriptUtil.h"

namespace script
{
    class InvokationCtx;
    class Scriptable;

    template<class Type>
    struct OwnerTypeId {};

    class SCRIPT_API ScriptRegistry
    {
    public:
        using ScriptLoadFunction = std::function<StateContainer(InvokationCtx&, Scriptable&)>;
        using ScriptLoadMap = std::map<std::string_view, ScriptLoadFunction, ScriptNamePredicate>;
        
        template<class Me>
        ScriptLoadMap& GetContainer();

    private:

        ScriptLoadMap& GetContainerImpl(uint64 id);

        using ScriptTypeMap = std::unordered_map<uint64, ScriptLoadMap>;
        ScriptTypeMap scripts;
    };

    template<class Me>
    ScriptRegistry::ScriptLoadMap& ScriptRegistry::GetContainer()
    {
        return GetContainerImpl(OwnerTypeId<Me>()());
    }

#ifdef SCRIPT_DYNAMIC_RELOAD
    SCRIPT_API extern ScriptRegistry _scriptRegistry;
#endif

    SCRIPT_API ScriptRegistry& scriptRegistry();

}
