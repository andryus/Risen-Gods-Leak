#include "MapScript.h"
#include "Map.h"
#include "Position.h"
#include "TemporarySummon.h"

SCRIPT_ACTION_RET_IMPL(SpawnCreature)
{
    return me.SummonCreature(entry.entry, pos);
}

SCRIPT_ACTION_RET_IMPL(SpawnGO)
{
    return me.SummonGameObject(entry.id, pos, PHASEMASK_NORMAL, entry.rotation0, entry.rotation1, entry.rotation2, entry.rotation3);
}
