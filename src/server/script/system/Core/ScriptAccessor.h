#pragma once
#include <unordered_map>
#include <mutex>

#include "Define.h"

namespace script
{
    class Scriptable;

    class SCRIPT_API ScriptAccessor
    {
        using ScriptableMap = std::unordered_map<uint64, Scriptable*>;
    public:

        static uint64 AddScriptable(Scriptable& scriptable);
        static Scriptable* GetScriptable(uint64 id);
        static void RemoveScriptable(uint64 id);

        static void UnloadScripts(bool forReload);
        static void ReloadScripts();

    private:

        static std::mutex _lock;
        static uint64 scriptId;
        static ScriptableMap scriptables;
    };

}
