#include "DatabaseCleaner.h"
#include "Account/AccountCleanup.h"

#include "Chat.h"
#include "ScriptMgr.h"

class RG::Cleanup::WorldReloadScript : public WorldScript
{
public:
    WorldReloadScript() : WorldScript("rg_cleanup_world") {}

    void OnConfigLoad(bool reload)
    {
        if (reload)
            OnLoad();
    }

    void OnLoad()
    {
        Account::LoadFromConfig();
    }
};

void AddSC_rg_accountcleanup_commandscript();

void AddSC_rg_cleanup()
{
    AddSC_rg_accountcleanup_commandscript();
    new RG::Cleanup::WorldReloadScript();
}