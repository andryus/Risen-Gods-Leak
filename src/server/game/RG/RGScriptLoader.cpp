#include "API.h"

void AddSC_rg_suspicious();
void AddSC_rg_cleanup();

void AddSC_rg_logging();

namespace RG
{
    namespace Command
    {
        void AddSC_rg_misc_command();
    }
    namespace Rebase
    {
        void AddSC_rg_rebase_item();
    }
    namespace Logging
    {
        void AddSC_rg_logging_spell_timing();
    }
}

GAME_API void AddSC_rg_scripts()
{
    AddSC_rg_suspicious();
    AddSC_rg_cleanup();

    AddSC_rg_logging();

    RG::Command::AddSC_rg_misc_command();
    RG::Rebase::AddSC_rg_rebase_item();
    RG::Logging::AddSC_rg_logging_spell_timing();
}
