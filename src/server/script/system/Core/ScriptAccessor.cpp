#include "ScriptAccessor.h"
#include "Errors.h"
#include "Scriptable.h"

namespace script
{

    uint64 ScriptAccessor::AddScriptable(Scriptable& scriptable)
    {
        std::lock_guard _guard(_lock);

        const uint64 id = scriptId++;
        scriptables.emplace(id, &scriptable);

        return id;
    }

    Scriptable* ScriptAccessor::GetScriptable(uint64 id)
    {
        std::lock_guard _guard(_lock);

        auto itr = scriptables.find(id);
        if (itr != scriptables.end())
            return itr->second;
        
        return nullptr;
    }

    void ScriptAccessor::RemoveScriptable(uint64 id)
    {
        std::lock_guard _guard(_lock);

        ASSERT(scriptables.count(id));
        scriptables.erase(id);
    }

    void ScriptAccessor::UnloadScripts(bool forReload)
    {
        std::lock_guard _guard(_lock);

        for (auto [id, scriptable] : scriptables)
            scriptable->ClearScript(true);

        if (!forReload)
            scriptables.clear();
    }

    void ScriptAccessor::ReloadScripts()
    {
        std::lock_guard _guard(_lock);

        for (auto [id, scriptable] : scriptables)
        {
            scriptable->ReloadScript();
        }
    }

    std::mutex ScriptAccessor::_lock;
    uint64 ScriptAccessor::scriptId = 0;
    ScriptAccessor::ScriptableMap ScriptAccessor::scriptables;

}
 