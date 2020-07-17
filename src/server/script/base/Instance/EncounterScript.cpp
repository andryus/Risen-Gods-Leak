#include "EncounterScript.h"
#include "StateScript.h"
#include "EncounterHooks.h"
#include "Entities/Player/Player.h"
#include "Instances/Encounter.h"
#include "Maps/InstanceMap.h"

SCRIPT_MODULE_STATE_IMPL(Encounter)
{
    script::StoredParam<Creature*> boss;

    SCRIPT_MODULE_PRINT_NONE
};

SCRIPT_MODULE_IMPL(Encounter)
{
    me <<= On::EncounterStarted 
        |= Do::PushState;

    me <<= On::EncounterEnded
        |= Do::PopState;
}

SCRIPT_ACTION_INLINE(StartEncounterImpl, Encounter)
{
    me.Start();
}

SCRIPT_ACTION_INLINE(EndEncounterImpl, Encounter)
{
    me.End();
}

SCRIPT_ACTION_INLINE(FinishImpl, Encounter)
{
    me.Finish();
}

/*********************************
* ACTIONS
**********************************/

SCRIPT_ACTION_INLINE(ClearEncounter, Encounter)
{
    me.Clear();
}

SCRIPT_ACTION_IMPL(StartEncounter)
{
    me.boss = boss;
    me <<= Do::StartEncounterImpl;
    me <<= Fire::EncounterStarted;

    AddUndoable(Do::ClearEncounter);
}

SCRIPT_ACTION_IMPL(EndEncounter)
{
    me.boss = nullptr;
    me <<= Do::EndEncounterImpl;
}

SCRIPT_ACTION_IMPL(FinishEncounter)
{
    me <<= Do::FinishImpl;
}

/*********************************
* MODIFIERS
**********************************/

SCRIPT_SELECTOR_IMPL(Boss)
{
    return me.boss.Load(logger);
}

SCRIPT_SELECTOR_IMPL(Instance)
{
    return &me.GetInstance();
}
