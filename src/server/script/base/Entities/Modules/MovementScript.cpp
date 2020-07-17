#include "MovementScript.h"

#include "TimedEventScript.h"
#include "Unit.h"
#include "UnitHooks.h"
#include "AuraScript.h"
#include "Creature.h"
#include "WorldObjectOwner.h"
#include "Position.h"
#include "CreatureGroups.h"

SCRIPT_MODULE_STATE_IMPL(Move)
{
    bool moveCommandAction = false;

    SCRIPT_MODULE_PRINT(Move) 
    {
        return "ActiveCommand: " + script::print(moveCommandAction); 
    }
};

SCRIPT_ACTION_INLINE(FinishMove, Scripts::Move)
{
    me.moveCommandAction = false;
}

SCRIPT_FILTER_IMPL(HasMoveCommand)
{
    return me.moveCommandAction;
}

SCRIPT_ACTION_INLINE(StartMove, Scripts::Move)
{
    me.moveCommandAction = true;

    AddUndoable(Do::FinishMove);
}

SCRIPT_MODULE_IMPL(Move)
{
    me <<= On::Evade
        |= Exec::After(1s) |= Do::MoveHome;

    me <<= On::PointReached
        |= Do::FinishMove;
    me <<= On::PathEnded
        |= Do::FinishMove;
}



/************************************
* ACTIONS
************************************/

constexpr SpellId AURA_ROOT = 23973;

SCRIPT_ACTION_IMPL(MakeMoveIdleRandom)
{
    me.SetRespawnRadius(distance);
    me.SetDefaultMovementType(MovementGeneratorType::RANDOM_MOTION_TYPE);

    me.GetMotionMaster()->Initialize();
}

SCRIPT_ACTION_INLINE(EnableMovement, Unit)
{
    me.RemoveAurasDueToSpell(AURA_ROOT.id, ObjectGuid::Empty, 0, AuraRemoveMode::AURA_REMOVE_BY_CANCEL);
}

SCRIPT_ACTION_IMPL(DisableMovement)
{
    me <<= Do::AddAura(AURA_ROOT);

    me.GetMotionMaster()->MoveIdle();
}

SCRIPT_ACTION_IMPL(StopMovement)
{
    me.GetMotionMaster()->MoveIdle();
}

SCRIPT_ACTION_IMPL(StopActiveMovement) 
{
    me.GetMotionMaster()->PauseActiveMovement();
}


SCRIPT_ACTION_IMPL(MoveHome)
{
    if (me.CanFreeMove())
        me.GetMotionMaster()->MoveTargetedHome();
    else
        me.ClearUnitState(UNIT_STATE_EVADE);
}

SCRIPT_ACTION_IMPL(MoveFollow)
{
    me <<= Do::StartMove;

    me.GetMotionMaster()->MoveFollow(&target, data.dist, data.angle, MOTION_SLOT_ACTIVE);
}

SCRIPT_ACTION_IMPL(MoveChase)
{
    me.GetMotionMaster()->MoveChase(&victim);
}

SCRIPT_ACTION_IMPL(MoveNear) 
{
    me <<= Do::StartMove;
    
    me.GetMotionMaster()->MoveCloserAndStop(0, &target, data.distance, data.height);
}

SCRIPT_ACTION_IMPL(MovePath)
{
    me <<= Do::StartMove;

    uint32 pathId;
    if (data.useGUID)
        pathId = me.GetGUID().GetCounter() * 100;
    else
        pathId = me.GetEntry() * 100;

    me <<= Do::MoveExplicitPath(data.repeatable).Bind(pathId);
}


SCRIPT_ACTION_IMPL(MoveExplicitPath)
{
    me.GetMotionMaster()->MoveScriptPath(pathId, repeatable, false);
}

SCRIPT_ACTION_IMPL(MoveRandom)
{
    me.GetMotionMaster()->MoveRandom(radius);
}

SCRIPT_ACTION_IMPL(MoveRandomOffset)
{
    const float angle = frand(0.0f, float(M_PI) * 2.0f);

    Position target = me.MovePositionToFirstCollision2D(me.GetPosition(), distance, angle);

    me.GetMotionMaster()->MovePoint(0, target);
}

SCRIPT_ACTION_IMPL(MakeWalk)
{
    me.SetWalk(true);

    AddUndoable(Do::MakeRun);
}

SCRIPT_ACTION_IMPL(MakeRun)
{
    me.SetWalk(false);

    AddUndoable(Do::MakeWalk);
}

SCRIPT_ACTION_IMPL(MovePos) 
{
    me <<= Do::StartMove;

    me.GetMotionMaster()->MovePoint(0, pos, true);
}

SCRIPT_ACTION_IMPL(MoveJump) 
{
    me <<= Do::StartMove;

    me.GetMotionMaster()->MoveJump(jumpposition.targetpos, jumpposition.speedXY, jumpposition.speedZ);
}

SCRIPT_ACTION_IMPL(MakeFormationLeader)
{
    const bool success = me <<= Do::AddToFormation.Bind(me);

    SetSuccess(success);
}

SCRIPT_ACTION_IMPL(AddToFormation)
{
    const uint64 formationId = leader.GetSpawnId();

    const bool success = sFormationMgr->AddCreatureToGroup(formationId, &me);

    SetSuccess(success);
}

SCRIPT_FILTER_IMPL(IsMoving)
{
    return me.isMoving();
}
