#include "BossScript.h"
#include "UnitScript.h"
#include "EncounterScript.h"
#include "CreatureHooks.h"
#include "EncounterHooks.h"
#include "CombatScript.h"

SCRIPT_MODULE_STATE_IMPL(Boss)
{
    SCRIPT_MODULE_PRINT_NONE
};

SCRIPT_MODULE_IMPL(Boss)
{
    me <<= On::RegisterEncounter
        |= *On::AddToEncounter
                |= If::HostileTo |= Do::EngageWith;

    me <<= On::EnterCombat |= &Do::StartEncounter;
    me <<= On::Evade |= Do::EndEncounter;
    me <<= On::Death |= Do::FinishEncounter;
}

SCRIPT_ACTION_IMPL(AddEncounterTexts)
{
    me <<= On::EnterCombat |= Do::Talk(texts.aggro);
    me <<= On::Kill |= Do::Talk(texts.kill);
    me <<= On::Death |= Do::Talk(texts.death);
}
