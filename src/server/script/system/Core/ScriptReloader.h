#pragma once

#ifdef ENABLE_SCRIPT_RELOAD

#include <memory>
#include <string_view>
#include <future>
#include "API.h"

struct DllHandle;

namespace efsw { class FileWatcher; }

namespace script
{

    class SCRIPT_API ScriptReloader
    {
    private:
        ScriptReloader();
        ~ScriptReloader();
    public:

        static ScriptReloader& instance();

        void Init();
        void Load(std::string_view path);
        void Reload(std::string_view path);
        void Unload();

    private:

        void Compile();
        bool Stage(std::string_view path);

        std::unique_ptr<DllHandle> activeDll;
        std::unique_ptr<efsw::FileWatcher> fileWatcher;

        std::future<bool> _stageScripts;
        bool isReloading = false;
    };

#define sScriptReloader script::ScriptReloader::instance()

}

#endif
