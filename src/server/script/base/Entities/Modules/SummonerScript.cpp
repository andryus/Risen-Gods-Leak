#include "SummonerScript.h"
#include "CreatureHooks.h"
#include "GameObjectHooks.h"
#include "CreatureScript.h"
#include "GameObjectScript.h"

SCRIPT_MODULE_STATE_IMPL(Summoner) 
{
    SCRIPT_MODULE_PRINT_NONE
};

///////////////////////////////////////
/// Summoner
///////////////////////////////////////

SCRIPT_ACTION1_INLINE(StoreSummon, Unit, Creature&, summon)
{
    AddUndoableFor(Do::DespawnCreature, summon);
}

SCRIPT_ACTION1_INLINE(StoreSummonGO, Unit, GameObject&, go)
{
    AddUndoableFor(Do::DespawnGO, go);
}

SCRIPT_MODULE_IMPL(Summoner)
{
    me <<= On::SummonCreature::Any()
        |= Do::StoreSummon;

    me  <<= On::SummonGO
        |= Do::StoreSummonGO;
}
