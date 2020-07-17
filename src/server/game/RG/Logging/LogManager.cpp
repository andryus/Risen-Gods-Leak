#include "LogManager.hpp"

#include "ScriptMgr.h"
#include "Chat.h"
#include "Config.h"

namespace RG
{
namespace Logging
{
    class SetupWorldScript : public WorldScript
    {
    public:
        SetupWorldScript() : WorldScript("rg_logging_setup") {}

        void OnLoad() override
        {
            //Manager::Initialize();
        }

        void OnShutdown() override
        {
            Manager::Shutdown();
        }
    };

    class ReloadWorldScript : public WorldScript
    {
    public:
        ReloadWorldScript() : WorldScript("rg_logging_reload") {}

        void OnConfigLoad(bool reload) override
        {
            if (reload)
                Manager::LoadConfig();
        }
    };
}
}

GAME_API std::vector<std::unique_ptr<RG::Logging::LogModule>> RG::Logging::Manager::modules_(RG::Logging::MODULE_COUNT);
GAME_API bool RG::Logging::Manager::enabled_(false);
GAME_API bool RG::Logging::Manager::initDone_(false);

void RG::Logging::Manager::Initialize()
{
    AddModule<ItemLogModule>();
    AddModule<MoneyLogModule>();
    AddModule<LevelLogModule>();
    AddModule<EncounterLogModule>();
    AddModule<ServerStatisticsModule>();
    AddModule<GMLogModule>();
    AddModule<TradeLogModule>();
    AddModule<PremiumLogModule>();

    ForEach([](std::unique_ptr<LogModule>& module) { ASSERT(module.get() != nullptr); });

    LoadConfig();
}

void RG::Logging::Manager::Shutdown()
{
    ForEach([](std::unique_ptr<LogModule>& module) { module.release(); });
}

void RG::Logging::Manager::LoadConfig()
{
    bool newEnabled = sConfigMgr->GetBoolDefault("RG.Database", false) && sConfigMgr->GetBoolDefault("RG.Logging.Enabled", false);
    if (!initDone_ && newEnabled && !enabled_)
    {
        ForEach([](std::unique_ptr<LogModule>& module) { module->Initialize(); });
        initDone_ = true;
    }
    enabled_ = newEnabled;

    ForEach([](std::unique_ptr<LogModule>& module) { module->LoadConfig(); });
}

void AddSC_rg_logging()
{
    using namespace RG::Logging;

    new SetupWorldScript();
    new ReloadWorldScript();
}