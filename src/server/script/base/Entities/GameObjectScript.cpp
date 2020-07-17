#include "GameObjectScript.h"
#include "GameObject.h"
#include "UnitOwner.h"


/************************************
* Actions
************************************/

SCRIPT_ACTION_RET_IMPL(SummonGO)
{
    if (GameObject* spawn = me.SummonGameObject(goEntry.id, position.m_positionX, position.m_positionY, position.m_positionZ, position.m_orientation, goEntry.rotation0, goEntry.rotation1, goEntry.rotation2, goEntry.rotation3, 0))
    {
        AddUndoableFor(Do::DespawnGO, *spawn);

        return spawn;
    }
    else
        return nullptr;
}

SCRIPT_ACTION_IMPL(UseGO)
{
    me.Use(&user);
}

SCRIPT_ACTION_IMPL(MakeInteractible)
{
    me.RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);

    AddUndoable(Do::MakeNonInteractible);
}

SCRIPT_ACTION_IMPL(MakeNonInteractible)
{
    me.SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);

    AddUndoable(Do::MakeInteractible);
}

SCRIPT_ACTION_INLINE(ResetGO, GameObject)
{
    me.ResetDoorOrButton();
}

SCRIPT_ACTION_IMPL(ActivateGO)
{
    if (me.UseDoorOrButton(0, true))
        AddUndoable(Do::ResetGO);
    else
        SetSuccess(false);
}

SCRIPT_ACTION_IMPL(DespawnGO)
{
    me.DespawnOrUnsummon();
}

SCRIPT_ACTION_IMPL(MakeGOReady)
{
    me.RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
    me.SetLootState(GO_READY);
    me.SetGoState(GO_STATE_READY);
}

/************************************
* Filter
************************************/

SCRIPT_FILTER_IMPL(IsGO)
{
    if (GameObject* go = me.To<GameObject>())
        return go->GetEntry() == goEntry.id;

    return false;
}
