#pragma once
#include "API/ModuleScript.h"
#include "CreatureOwner.h"
#include "RegularEventScript.h"
#include "Position.h"

struct JumpPosition 
{
    const Position targetpos;
    float speedXY;
    float speedZ;
    bool hasOrientation = false;
};

struct MoveNearData 
{ 
    float distance;
    float height;
};

struct MoveFollowData 
{
    MoveFollowData() = default;
    constexpr MoveFollowData(float dist) :
        dist(dist) {}
    constexpr MoveFollowData(float dist, float angle) :
        dist(dist), angle(angle) {}
    float dist = 1.0f;
    float angle = static_cast<float>(M_PI_2);
};

struct MovePathData
{
    MovePathData() = default;
    constexpr MovePathData(bool repeatable) :
        MovePathData(repeatable, false) {}
    constexpr MovePathData(bool repeatable, bool useGUID) :
        repeatable(repeatable), useGUID(useGUID) {}
    bool repeatable = false;
    bool useGUID = false;
};

///////////////////////////////////////
/// Movement
///////////////////////////////////////

SCRIPT_MODULE(Move, Unit)

/************************************
* ACTIONS
************************************/

SCRIPT_ACTION_EX(MakeMoveIdleRandom, Creature, float, distance)

SCRIPT_ACTION(DisableMovement, Unit)
SCRIPT_ACTION(StopMovement, Unit)
SCRIPT_ACTION(StopActiveMovement, Unit)

SCRIPT_ACTION(MoveHome, Unit)
SCRIPT_ACTION_EX(MovePos, Unit, Position, pos)
SCRIPT_ACTION_EX1(MoveFollow, Unit, MoveFollowData, data, Unit&, target)
SCRIPT_ACTION1(MoveChase, Unit, Unit&, victim)
SCRIPT_ACTION_EX1(MoveNear, Unit, MoveNearData, data, Unit&, target)
SCRIPT_ACTION_EX(MoveRandom, Unit, float, radius)
SCRIPT_ACTION_EX(MoveRandomOffset, Unit, float, distance)
SCRIPT_ACTION_EX(MovePath, Unit, MovePathData, data)
SCRIPT_ACTION_EX1(MoveExplicitPath, Unit, bool, repeatable, uint32, pathId)
SCRIPT_ACTION_EX(MoveJump, Unit, JumpPosition, jumpposition)

SCRIPT_ACTION(MakeWalk, Unit)
SCRIPT_ACTION(MakeRun, Unit)

SCRIPT_ACTION(MakeFormationLeader, Creature)
SCRIPT_ACTION1(AddToFormation, Creature, Creature&, leader)

/************************************
* FILTER
************************************/

SCRIPT_FILTER(HasMoveCommand, Scripts::Move)
SCRIPT_FILTER(IsMoving, Unit)

SCRIPT_MACRO_DATA(Exec, MovePos, AFTER, Position, pos)
{
    return BeginBlock
        += Do::MovePos(pos) 
        += Exec::UntilSuccess 
            |= !If::HasMoveCommand |= AFTER
    EndBlock;
}

SCRIPT_MACRO_DATA(Exec, MoveJump, AFTER, JumpPosition, pos)
{
    return BeginBlock
        += Do::MoveJump(pos)
        += Exec::UntilSuccess 
            |= !If::HasMoveCommand |= AFTER
    EndBlock;
}

SCRIPT_MACRO_DATA(Exec, MoveFollow, AFTER, float, distance)
{
    return BeginBlock
        += Do::MoveFollow(distance)
        += Exec::UntilSuccess
           |= !If::HasMoveCommand |= AFTER
    EndBlock;
}

SCRIPT_MACRO_DATA(Exec, MovePath, AFTER, bool, useGUID)
{
    return BeginBlock
        += Do::MovePath({ false, useGUID })
        += Exec::UntilSuccess
            |= !If::HasMoveCommand |= AFTER
    EndBlock;
}

SCRIPT_MACRO_DATA(Exec, MoveNear, AFTER, MoveNearData, data)
{
    return BeginBlock
        += Do::MoveNear(data)
        += Exec::UntilSuccess
            |= !If::HasMoveCommand |= AFTER
    EndBlock;
}




