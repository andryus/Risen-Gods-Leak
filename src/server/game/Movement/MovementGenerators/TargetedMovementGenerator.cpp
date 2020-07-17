/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TargetedMovementGenerator.h"
#include "Errors.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "World.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "Player.h"
#include "Spell.h"
#include "Transport.h"

#include "UnitHooks.h"

uint32 PET_POSITION_CHECK_INTERVAL = 100;
uint32 POSITION_CHECK_INTERVAL = 500;
uint32 MOVEMENT_PREDICTION_MS_TIME = 1000;

template<class T, typename D>
bool TargetedMovementGeneratorMedium<T, D>::_UpdatePath(T* owner, Position target)
{
    if (i_path && i_path->GetTransport() != owner->GetTransport())
        i_path = nullptr;

    if (!i_path)
        i_path = std::make_unique<PathGenerator>(owner);

    // allow pets to use shortcut if no path found when following their master
    bool forceDest = i_isPetFollow || owner->HasUnitState(UNIT_STATE_ALWAYS_REACH_TARGET);

    bool result = i_path->CalculatePath(target.GetPositionX(), target.GetPositionY(), target.GetPositionZ(), forceDest);

    if (i_path->GetPathType() & (PATHFIND_INCOMPLETE | PATHFIND_NOPATH))
        TC_LOG_INFO("entities.unit.evade", "Unit %s cannot find path to target %s. PathType: %u Start: %s End: %s", owner->GetGUID().ToString().c_str(), 
            i_target->GetGUID().ToString().c_str(), i_path->GetPathType(), i_path->GetStartPosition().toString().c_str(), i_path->GetEndPosition().toString().c_str());

    return result;
}

template <class T, typename D>
void TargetedMovementGeneratorMedium<T, D>::StopMoving(T* owner)
{
    Movement::MoveSplineInit init(owner);
    if (i_angle == 0.f)
        init.SetFacing(i_target.getTarget());
    init.Stop();
}

template <class T, typename D>
bool TargetedMovementGeneratorMedium<T, D>::IsTargetWithinRange(T* owner) const
{
    return i_offset ? owner->IsWithinCombatRange(i_target.getTarget(), i_offset) : owner->IsWithinMeleeRange(i_target.getTarget());
}

template<class T, typename D>
void TargetedMovementGeneratorMedium<T, D>::_SetTargetLocation(T* owner, bool updateDestination)
{
    if (!i_target.isValid() || !i_target->IsInWorld())
        return;

    if (owner->HasUnitState(UNIT_STATE_NOT_MOVE))
        return;

    if (owner->HasUnitState(UNIT_STATE_CASTING) && !owner->CanMoveDuringChannel())
        return;

    if (owner->GetTypeId() == TYPEID_UNIT)
    {
        if (owner->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (!i_target->isInAccessiblePlaceFor(owner->ToCreature()))
            return;
    }
    
    // allow pets to use shortcut if no path found when following their master
    bool forceDest = i_isPetFollow || owner->HasUnitState(UNIT_STATE_ALWAYS_REACH_TARGET);

    if (updateDestination || !i_path)
    {
        float x, y, z;

        if (!i_isPetFollow)
        {
            auto targetPos = i_target->CalculateExpectedPosition(MOVEMENT_PREDICTION_MS_TIME);

            // Pathfinder expects local transport coords
            if (Transport* transport = owner->GetTransport())
            {
                targetPos.GetPosition(x, y, z);
                transport->CalculatePassengerOffset(x, y, z);
                targetPos.Relocate(x, y, z);
            }

            const bool pathfindingResult = _UpdatePath(owner, targetPos);

            if (!pathfindingResult || (i_path->GetPathType() & PATHFIND_NOPATH && !forceDest))
            {
                if (owner->GetTypeId() == TYPEID_UNIT)
                    canReachTarget = false;
                // Cant reach target
                i_recalculateTravel = true;
                return;
            }

            float maxDistance = i_offset ? i_offset : owner->GetMeleeRange(i_target.getTarget()) * 0.95f;
            Position pathfinderEndPos(i_path->GetActualEndPosition().x, i_path->GetActualEndPosition().y, i_path->GetActualEndPosition().z);
            const float reduceDist = maxDistance - pathfinderEndPos.GetExactDist(&targetPos);
            if (reduceDist > 0.f)
            {
                i_path->ReducePathLenghtByDist(reduceDist);
                Position pathfinderReducedEndPos(i_path->GetActualEndPosition().x, i_path->GetActualEndPosition().y, i_path->GetActualEndPosition().z);
                float preferedAngle = pathfinderEndPos.GetAngle(&pathfinderReducedEndPos);
                if (i_isPetChase)
                    preferedAngle = i_target->GetOrientation() - M_PI;
                // Calculate target Position in global coords
                if (Transport* transport = owner->GetTransport())
                {
                    pathfinderEndPos.GetPosition(x, y, z);
                    transport->CalculatePassengerPosition(x, y, z, &preferedAngle);
                    pathfinderEndPos.Relocate(x, y, z);
                }
                Position target = owner->GetFollowPosition(pathfinderEndPos, preferedAngle, reduceDist, reduceDist / 2.f);

                // Back to local transport coords
                if (Transport* transport = owner->GetTransport())
                {
                    target.GetPosition(x, y, z);
                    transport->CalculatePassengerOffset(x, y, z);
                    target.Relocate(x, y, z);
                }

                // if we want to back away from the target, check if we actually move further away
                if (i_relocateBackwards && target.GetExactDist(&targetPos) < maxDistance / 2.f)
                {
                    i_relocateBackwards = false;
                    return;
                }

                i_path->OverrideEndPosition(target);
            }
        }
        else
        {
            i_target->GetPetFollowPosition(owner, x, y, z, i_offset, i_angle);

            // Pathfinder expects local transport coords
            if (Transport* transport = owner->GetTransport())
                transport->CalculatePassengerOffset(x, y, z);

            _UpdatePath(owner, Position(x, y, z));
        }
    }
    else
    {
        // the destination has not changed, we just need to refresh the path (usually speed change)
        if (i_path->GetPathType() & PATHFIND_NOPATH && !forceDest)
        {
            StopMoving(owner);
            return;
        }
    }

    if (owner->GetTypeId() == TYPEID_UNIT)
    {
        if (i_path->GetPathType() & PATHFIND_INCOMPLETE && !owner->IsControlledByPlayer())
        {
            canReachTarget = false;
            auto targetPos = i_path->GetActualEndPosition();
            if (!i_target.getTarget()->IsWithinLOS(targetPos[0], targetPos[1], targetPos[2]))
            {
                i_recalculateTravel = true;
                return;
            }
        }
        else
            canReachTarget = true;
    }

    D::_addUnitStateMove(owner);
    i_targetReached = false;
    i_recalculateTravel = false;
    owner->AddUnitState(UNIT_STATE_CHASE);

    Movement::MoveSplineInit init(owner);
    init.MovebyPath(*i_path);
    init.SetWalk(((D*)this)->EnableWalking());
    // Using the same condition for facing target as the one that is used for SetInFront on movement end
    // - applies to ChaseMovementGenerator mostly
    if (i_angle == 0.f)
        init.SetFacing(i_target.getTarget());
    else if (i_isPetFollow)
        init.SetFacing(i_target->GetTransport() ? i_target->m_movementInfo.transport.pos.GetOrientation() : i_target->GetOrientation());

    // @todo check when this is used
    // if (i_relocateBackwards)
    //     init.SetOrientationInversed();

    i_relocateBackwards = false;

    if (i_isPetFollow)
        init.SetPetMovement();

    init.Launch();
}

template<class T, typename D>
bool TargetedMovementGeneratorMedium<T, D>::DoUpdate(T* owner, uint32 time_diff)
{
    if (!i_target.isValid() || !i_target->IsInWorld())
        return false;

    if (!owner || !owner->IsAlive())
        return false;

    if (owner->HasUnitState(UNIT_STATE_NOT_MOVE))
    {
        canReachTarget = true;
        D::_clearUnitStateMove(owner);
        return true;
    }

    // prevent movement while casting spells with cast time or channel time
    Spell* spell = owner->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
    if (owner->HasUnitState(UNIT_STATE_CASTING) && (spell ? (spell->GetSpellInfo()->ChannelInterruptFlags & (AURA_INTERRUPT_FLAG_MOVE | AURA_INTERRUPT_FLAG_TURNING)) : true) && !owner->CanMoveDuringChannel())
    {
        canReachTarget = true;
        if (owner->isMoving())
            owner->StopMoving();
        // recalculate movement spline on next update
        i_recheckDistance.Reset(0);
        return true;
    }

    // prevent crash after creature killed pet
    if (static_cast<D*>(this)->_lostTarget(owner))
    {
        D::_clearUnitStateMove(owner);
        return true;
    }

    bool targetMoved = false;
    float distance = -1.f;

    if (owner->HasUnitState(UNIT_STATE_FOLLOW) && owner->GetTransport() != i_target->GetTransport())
    {
        if (auto oldTransport = owner->GetTransport())
            oldTransport->RemovePassenger(owner);
        owner->NearTeleportTo(i_target->GetPositionX(), i_target->GetPositionY(), i_target->GetPositionZ(), i_target->GetOrientation());
        if (auto newTransport = i_target->GetTransport())
        {
            newTransport->AddPassenger(owner);
            owner->m_movementInfo.transport.pos.Relocate(i_target->m_movementInfo.transport.pos);
        }
        targetMoved = true;
    }

    i_recheckDistance.Update(time_diff);
    if (i_recheckDistance.Passed())
    {
        i_recheckDistance.Reset(i_isPetFollow ? PET_POSITION_CHECK_INTERVAL : POSITION_CHECK_INTERVAL);

        //More distance let have better performance, less distance let have more sensitive reaction at target move.
        float allowed_dist;

        if (i_isPetFollow)
            allowed_dist = (i_target->GetUnitMovementFlags() & MOVEMENTFLAG_MASK_MOVING  & ~MOVEMENTFLAG_FALLING) ? 2.f : 10.0f; // pet following owner
        else
            allowed_dist = i_target->GetMeleeRange(owner) * 0.99f;

        G3D::Vector3 dest; 
        if (owner->movespline->Finalized())
            owner->GetPosition(dest.x, dest.y, dest.z);
        else
        {
            dest = owner->movespline->FinalDestination();
            if (owner->movespline->onTransport)
                if (TransportBase* transport = owner->GetDirectTransport())
                    transport->CalculatePassengerPosition(dest.x, dest.y, dest.z);
        }

        // First check distance
        if (i_isPetFollow)
        {
            float x, y, z;
            i_target->GetPetFollowPosition(owner, x, y, z, i_offset, i_angle);
            Position loc(x, y, z);
            distance = loc.GetExactDist(dest.x, dest.y, dest.z);
            targetMoved = distance >= allowed_dist;
        }
        else
        {
            const Position currentPos = i_target->CalculateExpectedPosition(0); // current position based on latest recieved movement packet
            const Position predictedPos = i_target->CalculateExpectedPosition(MOVEMENT_PREDICTION_MS_TIME);
            targetMoved = !Position(dest.x, dest.y, dest.z).IsInBetween3d(currentPos, predictedPos, allowed_dist);

            // special case: our target is moving though us, just interrupt movement and wait
            if (targetMoved && owner->GetPosition().IsInBetween3d(currentPos, predictedPos, allowed_dist))
            {
                if (!owner->movespline->Finalized())
                {
                    Movement::MoveSplineInit init(owner);
                    init.Stop();
                }
                targetMoved = false;
            }
        }

        if (targetMoved && i_isPetFollow)
            i_recheckDistance.Reset(400);

        if (targetMoved && (i_target->GetUnitMovementFlags() & MOVEMENTFLAG_MASK_MOVING))
            i_targetState = TargetState::MOVE;
        else if (!targetMoved && i_isPetChase && i_target->GetVictim() != owner)
        {
            if (i_target->HasInArc(static_cast<float>(M_PI + M_PI_2), owner))
            {
                i_recheckDistance.Reset(400);
                targetMoved = true;
            }
        }

        // then, if the target is in range, check also Line of Sight.
        if (!targetMoved)
            targetMoved = !i_target->IsWithinLOS(dest.x, dest.y, dest.z);

        // perform a single position update when target stopped
        if (!i_isPetFollow && i_targetState == TargetState::MOVE && ((i_target->GetUnitMovementFlags() & MOVEMENTFLAG_MASK_MOVING) == 0))
        {
            i_targetState = TargetState::JUST_STOPPED;

            const float destinationAngle = i_target->GetAngle(dest.x, dest.y);
            const float currentAngle = i_target->GetAngle(owner);
            float diff = (destinationAngle > currentAngle ? 1.f : -1.f) * (destinationAngle - currentAngle);
            diff = std::min(2 * float(M_PI) - diff, diff);
            // if our current destination is located behind the target, recalculate our destination
            if (diff >= float(M_PI / 2.0))
                targetMoved = true;
        }
    }

    /* Relocation Movement:
     * Occures if owner of pet moves far OR when target is too close for chase movement

     * Reposition:
     * States:
     *  RepositionStates::READY set in constuctor and at the end of an repositining event
     *  RepositionStates::CHECK set if target is to far(pet) or to close(repos creature)
     *  RepositionStates::TIMER set if no other movement is in progress check was successful
     */
    if (targetMoved || (!i_isPetFollow && i_repositionState == RepositionState::READY))
        i_repositionState = RepositionState::CHECK;
    if (i_repositionState == RepositionState::TIMER)
    {
        if (i_repositionTimer <= int32(time_diff))
        {
            if (i_isPetFollow)
                targetMoved = true;
            else if ((i_target->GetUnitMovementFlags() & MOVEMENTFLAG_MASK_MOVING) == 0)
            {
                if (distance < 0.f)
                    distance = i_target->GetExactDist(owner);

                if (distance < std::max(DEFAULT_COMBAT_REACH, owner->GetCombatReach() * 0.15f))
                {
                    targetMoved = true;
                    i_relocateBackwards = true;
                }
            }

            i_repositionState = RepositionState::READY;
        }
        else
            i_repositionTimer -= time_diff;
    }

    if (i_recalculateTravel || targetMoved)
        _SetTargetLocation(owner, targetMoved);
    else if (owner->GetTypeId() == TYPEID_UNIT && owner->movespline->Finalized() && !(i_target->GetUnitMovementFlags() & MOVEMENTFLAG_MASK_MOVING))
        if (IsTargetWithinRange(owner))
            canReachTarget = true;

    if (i_targetState == TargetState::JUST_STOPPED)
        i_targetState = TargetState::STOPPED;

    if (owner->movespline->Finalized())
    {
        static_cast<D*>(this)->MovementInform(owner);
        if (i_angle == 0.f && !owner->HasInArc(0.01f, i_target.getTarget()))
            owner->SetInFront(i_target.getTarget());

        if (i_repositionState == RepositionState::CHECK)
        {
            i_repositionState = RepositionState::TIMER;
            i_repositionTimer = 2000;
        }

        if (!i_targetReached)
        {
            i_targetReached = true;
            static_cast<D*>(this)->_reachTarget(owner);
        }
    }

    return true;
}

//-----------------------------------------------//
template<class T>
void ChaseMovementGenerator<T>::_reachTarget(T* owner)
{
    if (owner->IsWithinMeleeRange(this->i_target.getTarget()))
        owner->Attack(this->i_target.getTarget(), true);

    owner->chaseOriginalAngle = this->i_target.getTarget()->GetAngle(owner);
    owner->chaseRepositionCounter = 0;
}

template<>
void ChaseMovementGenerator<Player>::DoInitialize(Player* owner)
{
    owner->AddUnitState(UNIT_STATE_CHASE | UNIT_STATE_CHASE_MOVE);
    if (IsTargetWithinRange(owner))
        StopMoving(owner);
    else
        _SetTargetLocation(owner, true);
}

template<>
void ChaseMovementGenerator<Creature>::DoInitialize(Creature* owner)
{
    owner->SetWalk(false);
    owner->AddUnitState(UNIT_STATE_CHASE | UNIT_STATE_CHASE_MOVE);
    if (IsTargetWithinRange(owner))
        StopMoving(owner);
    else
        _SetTargetLocation(owner, true);

    if (owner->IsPet() && i_target.isValid() && i_target->GetTypeId() == TYPEID_UNIT)
        i_isPetChase = true;
}

template<class T>
void ChaseMovementGenerator<T>::DoFinalize(T* owner)
{
    owner->ClearUnitState(UNIT_STATE_CHASE | UNIT_STATE_CHASE_MOVE);
}

template<class T>
void ChaseMovementGenerator<T>::DoReset(T* owner)
{
    DoInitialize(owner);
}

template<class T>
void ChaseMovementGenerator<T>::MovementInform(T* /*unit*/) { }

template<>
void ChaseMovementGenerator<Creature>::MovementInform(Creature* unit)
{
    // Pass back the GUIDLow of the target. If it is pet's owner then PetAI will handle
    if (unit->AI())
        unit->AI()->MovementInform(CHASE_MOTION_TYPE, i_target.getTarget()->GetGUID().GetCounter());
}

//-----------------------------------------------//
template<>
bool FollowMovementGenerator<Creature>::EnableWalking() const
{
    return i_target.isValid() && i_target->IsWalking();
}

template<>
bool FollowMovementGenerator<Player>::EnableWalking() const
{
    return false;
}

template<>
void FollowMovementGenerator<Player>::_updateSpeed(Player* /*owner*/)
{
    // nothing to do for Player
}

template<>
void FollowMovementGenerator<Creature>::_updateSpeed(Creature* owner)
{
    // pet only sync speed with owner
    /// Make sure we are not in the process of a map change (IsInWorld)
    if (!owner->IsPet() || !owner->IsInWorld() || !i_target.isValid() || i_target->GetGUID() != owner->GetOwnerGUID())
        return;

    owner->UpdateSpeed(MOVE_RUN);
    owner->UpdateSpeed(MOVE_WALK);
    owner->UpdateSpeed(MOVE_SWIM);
}

template<>
void FollowMovementGenerator<Player>::DoInitialize(Player* owner)
{
    owner->AddUnitState(UNIT_STATE_FOLLOW | UNIT_STATE_FOLLOW_MOVE);
    _updateSpeed(owner);
    _SetTargetLocation(owner, true);
}

template<>
void FollowMovementGenerator<Creature>::DoInitialize(Creature* owner)
{
    if (owner->IsPet() && owner->IsInWorld() && i_target.isValid() && i_target->GetGUID() == owner->GetOwnerGUID())
        i_isPetFollow = true;

    owner->AddUnitState(UNIT_STATE_FOLLOW | UNIT_STATE_FOLLOW_MOVE);
    _updateSpeed(owner);
    _SetTargetLocation(owner, true);
}

template<class T>
void FollowMovementGenerator<T>::DoFinalize(T* owner)
{
    owner->ClearUnitState(UNIT_STATE_FOLLOW | UNIT_STATE_FOLLOW_MOVE);
    _updateSpeed(owner);
}

template<class T>
void FollowMovementGenerator<T>::DoReset(T* owner)
{
    DoInitialize(owner);
}

template<class T>
void FollowMovementGenerator<T>::MovementInform(T* /*unit*/) { }

template<>
void FollowMovementGenerator<Creature>::MovementInform(Creature* unit)
{
    *unit <<= Fire::PointReached;

    // Pass back the GUIDLow of the target. If it is pet's owner then PetAI will handle
    if (unit->AI())
        unit->AI()->MovementInform(FOLLOW_MOTION_TYPE, i_target.getTarget()->GetGUID().GetCounter());
}

//-----------------------------------------------//
template void TargetedMovementGeneratorMedium<Player, ChaseMovementGenerator<Player> >::_SetTargetLocation(Player*, bool);
template void TargetedMovementGeneratorMedium<Player, FollowMovementGenerator<Player> >::_SetTargetLocation(Player*, bool);
template void TargetedMovementGeneratorMedium<Creature, ChaseMovementGenerator<Creature> >::_SetTargetLocation(Creature*, bool);
template void TargetedMovementGeneratorMedium<Creature, FollowMovementGenerator<Creature> >::_SetTargetLocation(Creature*, bool);
template bool TargetedMovementGeneratorMedium<Player, ChaseMovementGenerator<Player> >::DoUpdate(Player*, uint32);
template bool TargetedMovementGeneratorMedium<Player, FollowMovementGenerator<Player> >::DoUpdate(Player*, uint32);
template bool TargetedMovementGeneratorMedium<Creature, ChaseMovementGenerator<Creature> >::DoUpdate(Creature*, uint32);
template bool TargetedMovementGeneratorMedium<Creature, FollowMovementGenerator<Creature> >::DoUpdate(Creature*, uint32);

template void ChaseMovementGenerator<Player>::_reachTarget(Player*);
template void ChaseMovementGenerator<Creature>::_reachTarget(Creature*);
template void ChaseMovementGenerator<Player>::DoFinalize(Player*);
template void ChaseMovementGenerator<Creature>::DoFinalize(Creature*);
template void ChaseMovementGenerator<Player>::DoReset(Player*);
template void ChaseMovementGenerator<Creature>::DoReset(Creature*);
template void ChaseMovementGenerator<Player>::MovementInform(Player*);

template void FollowMovementGenerator<Player>::DoFinalize(Player*);
template void FollowMovementGenerator<Creature>::DoFinalize(Creature*);
template void FollowMovementGenerator<Player>::DoReset(Player*);
template void FollowMovementGenerator<Creature>::DoReset(Creature*);
template void FollowMovementGenerator<Player>::MovementInform(Player*);
