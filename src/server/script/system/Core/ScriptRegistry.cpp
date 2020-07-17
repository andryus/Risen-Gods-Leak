#include "ScriptRegistry.h"
#include "ScriptState.h"

namespace script
{
#ifdef SCRIPT_DYNAMIC_RELOAD
    SCRIPT_API ScriptRegistry _scriptRegistry;
#endif


    ScriptRegistry::ScriptLoadMap& ScriptRegistry::GetContainerImpl(uint64 id)
    {
        return scripts[id];
    }

    ScriptRegistry& scriptRegistry()
    {
#ifdef SCRIPT_DYNAMIC_RELOAD
       return _scriptRegistry;
#else
       static ScriptRegistry registry;

       return registry;
#endif
    }

}
