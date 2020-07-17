#include "CreatureScript.h"
#include "Creature.h"
#include "TemporarySummon.h"
#include "Map.h"

/************************************
* ACTIONS
************************************/

SCRIPT_ACTION_IMPL(MorphCreatureEntry)
{
    const uint32 oldEntry = me.GetEntry();

    if (oldEntry != entry.entry)
    {
        me.UpdateCreatureEntry(entry.entry);

        AddUndoable(Do::MorphCreatureEntry(oldEntry));
    }
}

SCRIPT_ACTION_IMPL(MorphCreatureToModel) 
{
    const uint32 oldModel = me.GetDisplayId();
    me.SetDisplayId(modelId);

    AddUndoable(Do::MorphCreatureToModel(oldModel));
}

SCRIPT_ACTION_EX_INLINE(ChangeReactState, Creature, ReactStates, reactState)
{
    const ReactStates react = me.GetReactState();
    if (react != reactState)
    {
        me.SetReactState(reactState);

        AddUndoable(Do::ChangeReactState(react));
    }
}

SCRIPT_ACTION_IMPL(MakeAggressive)
{
    me <<= Do::ChangeReactState(REACT_AGGRESSIVE);
}

SCRIPT_ACTION_IMPL(MakeDefensive)
{
    me <<= Do::ChangeReactState(REACT_DEFENSIVE);
}

SCRIPT_ACTION_IMPL(MakePassive)
{
    me <<= Do::ChangeReactState(REACT_PASSIVE);
}

SCRIPT_ACTION_INLINE(EnableHealthReg, Creature)
{
    me.setRegeneratingHealth(true);
}

SCRIPT_ACTION_IMPL(DisableHealthReg)
{
    me.setRegeneratingHealth(false);

    AddUndoable(Do::EnableHealthReg);
}

SCRIPT_ACTION_EX_INLINE(SetHomePos, Creature, Position, pos)
{
    const Position currentHome = me.GetHomePosition();
    AddUndoable(Do::SetHomePos(currentHome));

    me.SetHomePosition(pos);

}

SCRIPT_ACTION_IMPL(MakePosToHome)
{
    const Position pos = me.GetPosition();

    me <<= Do::SetHomePos(pos);
}

SCRIPT_ACTION_IMPL(OverrideCorpseDecay)
{
    me.SetCorpseDelay(duration.count());
}

SCRIPT_ACTION_IMPL(OverrideSightRange)
{
    me.m_SightDistance = range;
    me.m_CombatDistance = range;
}

SCRIPT_ACTION_RET_IMPL(SummonCreature)
{
    return me.SummonCreature(entry.entry, pos, TempSummonType::TEMPSUMMON_DEAD_DESPAWN);
}

SCRIPT_ACTION_IMPL(DespawnCreature)
{
    me.DespawnOrUnsummon();
}

SCRIPT_ACTION_IMPL(DisappearAndDie)
{
    me.SetCorpseDelay(0);
    me.SetRespawnTime(respawnTime.count());
    me.DisappearAndDie();
}


SCRIPT_ACTION_IMPL(ChangeEquip)
{
    const uint8 currentEquip = me.GetCurrentEquipmentId();

    me.LoadEquipment(equipId, true);

    AddUndoable(Do::ChangeEquip(currentEquip));
}

/************************************
* FILTER
************************************/

SCRIPT_FILTER_IMPL(Aggressive)
{
    return me.HasReactState(REACT_AGGRESSIVE);
}

SCRIPT_FILTER_IMPL(IsCreature)
{
    if (Creature* creature = script::castSourceTo<Creature>(me))
        return creature->GetEntry() == entry.entry;
    else
        return false;
}

/************************************
* GETTER
************************************/

SCRIPT_GETTER_IMPL(SightRange)
{
    return me.GetSightRange();
}
