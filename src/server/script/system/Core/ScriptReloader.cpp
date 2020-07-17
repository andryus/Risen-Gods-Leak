#include "ScriptReloader.h"

#ifdef ENABLE_SCRIPT_RELOAD
#include <string>
#include <boost/filesystem.hpp>
#include <efsw/efsw.hpp>
#include "Log.h"
#include "Timer.h"
#include "ScriptAccessor.h"
#include "GitRevision.h"
#include "Duration.h"

#ifdef WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

namespace
{
#if WIN32
    using HandleType = HMODULE;
#else // Posix
    using HandleType = void*;
#endif

    // Returns "dll" on Windows, "dylib" on OS X, and "so" on posix.
    constexpr std::string_view getDllExtention()
    {
#if WIN32
        return ".dll";
#else // Posix
        return ".so";
#endif
    }

    template<typename Fn>
    Fn getFunctionFromDLL(HandleType handle, std::string_view name)
    {
#if WIN32
        return reinterpret_cast<Fn>(GetProcAddress(handle, name.data()));
#else // Posix
        return reinterpret_cast<Fn>(dlsym(handle, name.data()));
#endif
    }

    void removeBrackets(std::string& string)
    {
        string.erase(std::remove(string.begin(), string.end(), '('));
        string.erase(std::remove(string.begin(), string.end(), ')'));
    }

    using RegisterFunc = void(*)();
}

struct DllHandle
{
    DllHandle(HandleType handle, std::string&& path) :
        handle(handle), path(std::move(path)) {}

    ~DllHandle()
    {

#if WIN32
        const bool success = (FreeLibrary(handle) != 0);
#else
        const bool success = (dlclose(handle) == 0);
#endif

        if (!success)
            TC_LOG_ERROR("server.script", "Failed to unload Shared Library %s", path);
    }

private:

    HandleType handle;
    std::string path;
};


namespace script
{
    struct ContentFileWatcher final :
        public efsw::FileWatchListener
    {
        void handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename) override
        {
            const std::string extention = boost::filesystem::extension(filename);

            if (extention == ".cpp")
                sScriptReloader.Reload("script_content");
        }
    };

    ScriptReloader::ScriptReloader()
    {
    }

    ScriptReloader::~ScriptReloader() = default;

    ScriptReloader& ScriptReloader::instance()
    {
        static ScriptReloader loader;

        return loader;
    }

    void ScriptReloader::Init()
    {
        TC_LOG_ERROR("server.script", "Initializing Script Reloader...");

        fileWatcher = std::make_unique<efsw::FileWatcher>();

        std::string watchDirectory = GitRevision::GetSourceDirectory();
        removeBrackets(watchDirectory);
        watchDirectory += "/src/server/script/content/";

        fileWatcher->addWatch(watchDirectory, new ContentFileWatcher, true);
        fileWatcher->watch();
    }

    void ScriptReloader::Load(std::string_view path)
    {
        const uint32 startTime = getMSTime();
        TC_LOG_ERROR("server.script", "Loading Scripts from DLL %s...", path.data());

        if (!Stage(path))
        {
            // @todo: fallback to loading regular DLL
            TC_LOG_ERROR("server.script", "Failed to load Script Content due to Staging Error.");

            return;
        }

        std::string fullPath = boost::filesystem::temp_directory_path().string() + std::string(path) + "_staged";
        fullPath += getDllExtention();

#if WIN32
        HandleType handle = LoadLibrary(fullPath.c_str());
#else // Posix
        HandleType handle = dlopen(path.data(), RTLD_LAZY);
#endif
        if (!handle)
        {
            TC_LOG_ERROR("server.script", "Failed to open Script DLL %s.", fullPath.c_str());
            return;
        }

        const RegisterFunc func = getFunctionFromDLL<RegisterFunc>(handle, "registerScripts");
        if (!func)
        {
            TC_LOG_ERROR("server.script", "DLL is missing 'registerScripts'-function, from %s.", fullPath.c_str());
            return;
        }

        func();

        activeDll = std::make_unique<DllHandle>(handle, std::move(fullPath));

        uint32 uPathLoadTime = getMSTimeDiff(startTime, getMSTime());
        TC_LOG_ERROR("server.script", "Loading Scripts in %i ms", uPathLoadTime);
    }

    void ScriptReloader::Reload(std::string_view path)
    {
        if (isReloading)
            return;

        isReloading = true;

        ScriptAccessor::UnloadScripts(true);
        activeDll = nullptr;

        Compile();

        Load(path);
        ScriptAccessor::ReloadScripts();

        isReloading = false;
    }

    void ScriptReloader::Unload()
    {
        ScriptAccessor::UnloadScripts(false);

        activeDll = nullptr;
    }

    void ScriptReloader::Compile()
    {
        TC_LOG_INFO("server.script", "Building Script Content...");

        // Without binding stdin
        std::string build = "--build " ; 
        build += GitRevision::GetBuildDirectory();
        removeBrackets(build);

        std::string config = "--config ";
        config += GitRevision::GetBuildDirective();

        // cmake command
        std::string command = "\"";
        command += GitRevision::GetCMakeCommand();
        command += "\" ";
        removeBrackets(command);
        
        // params
        command += build + ' ';
        command += "--target script_content ";
        command += config + ' ';

        system(command.c_str());
    }

    bool ScriptReloader::Stage(std::string_view path)
    {
        _stageScripts = std::async(std::launch::async, [fileName = std::string(path)]()
        {            
            TC_LOG_INFO("server.script", "Attempting to stage Script Library %s....", fileName.c_str());

            const std::string file = fileName + ".dll";
            const std::string targetName = boost::filesystem::temp_directory_path().string() + fileName;
            const std::string target = targetName + "_staged.dll";

            try
            {
                boost::filesystem::copy_file(file, target, boost::filesystem::copy_option::overwrite_if_exists);
            }
            catch (boost::filesystem::filesystem_error)
            {
                TC_LOG_FATAL("server.script", "Failed to stage Script Library %s. Script are not being loaded.", target.c_str());

                return false;
            }

            const std::string debugSymbols = fileName + ".pdb";
            const std::string debugSymbolsTarget = targetName + "_staged.pdb";

            try
            {
                boost::filesystem::copy_file(debugSymbols, debugSymbolsTarget, boost::filesystem::copy_option::overwrite_if_exists);
            }
            catch (boost::filesystem::filesystem_error)
            {
                TC_LOG_ERROR("server.script", "Failed to stage Debug Symbols for Script Library %s. Debugging Scripts is not possible.", debugSymbolsTarget.c_str());
            }

            TC_LOG_INFO("server.script", "Successfully Staged Script Library %s.", fileName.c_str());

            return true;
        });


        return _stageScripts.wait_for(10s) != std::future_status::timeout && _stageScripts.get();
    }

}

#endif
