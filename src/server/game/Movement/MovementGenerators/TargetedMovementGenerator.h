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

#ifndef TRINITY_TARGETEDMOVEMENTGENERATOR_H
#define TRINITY_TARGETEDMOVEMENTGENERATOR_H

#include "MovementGenerator.h"
#include "FollowerReference.h"
#include "Timer.h"
#include "Unit.h"
#include "PathGenerator.h"

class TargetedMovementGeneratorBase
{
    public:
        TargetedMovementGeneratorBase(Unit* target) : canReachTarget(true) { i_target.link(target, this); }
        void stopFollowing() { }
    protected:
        bool canReachTarget;
        FollowerReference i_target;
};

template<class T, typename D>
class TargetedMovementGeneratorMedium : public MovementGeneratorMedium< T, D >, public TargetedMovementGeneratorBase
{
    protected:
        TargetedMovementGeneratorMedium(Unit* target, float offset, float angle) :
            TargetedMovementGeneratorBase(target), i_recheckDistance(0), i_offset(offset), i_angle(angle), 
            i_recalculateTravel(false), i_targetReached(false), i_isPetFollow(false), i_repositionTimer(0),
            i_repositionState(RepositionState::READY), i_relocateBackwards(false), i_targetState(TargetState::MOVE),
            i_isPetChase(false)
        { }
        ~TargetedMovementGeneratorMedium() { }

    public:
        bool DoUpdate(T*, uint32);
        Unit* GetTarget() const { return i_target.getTarget(); }

        void unitSpeedChanged() override { i_recalculateTravel = true; }

        bool CanReachTarget() const override { return canReachTarget; }
    protected:
        /**
         * Launches a new movement spline for the owner (if no error occurs during path generation). If updateDestination 
         * parameter is set, a new target position will be calculated by predicting the target's movement. Otherwise
         * the last spline will be relaunched (This is used for owner speed changes.)
         * @param updateDestination if true a new spline will be generated and a new target position is selected
         */
        void _SetTargetLocation(T* owner, bool updateDestination);

        enum class TargetState : uint8
        {
            MOVE,
            JUST_STOPPED,
            STOPPED,
        };

        enum class RepositionState : uint8
        {
            READY = 1,
            CHECK = 2,
            TIMER = 3
        };

        std::unique_ptr<PathGenerator> i_path;
        TimeTrackerSmall i_recheckDistance;
        float i_offset;
        float i_angle;
        bool i_isPetFollow;
        bool i_isPetChase;
        bool i_relocateBackwards;
        int32 i_repositionTimer;
        RepositionState i_repositionState;
        TargetState i_targetState;
        bool i_recalculateTravel;
        bool i_targetReached;

        void StopMoving(T* owner);
        bool IsTargetWithinRange(T* owner) const;
    private:
        /**
         * Creates or updates the PathGenerator-Object owned by this MovementGenerator. Then the pathfinding algorithm
         * will be used to find a path from our owner to the target position. After this method is called, i_path is a
         * valid pointer to a PathGenerator-Object, which contains a point-path to the target position or an 
         * incomplete path.
         * @param target Pathfinding target
         * @return whether or not the pathfinding algorithm was successful.
         */
        bool _UpdatePath(T* owner, Position target);
};

template<class T>
class ChaseMovementGenerator : public TargetedMovementGeneratorMedium<T, ChaseMovementGenerator<T> >
{
    public:
        ChaseMovementGenerator(Unit* target)
            : TargetedMovementGeneratorMedium<T, ChaseMovementGenerator<T> >(target) { }
        ChaseMovementGenerator(Unit* target, float offset, float angle)
            : TargetedMovementGeneratorMedium<T, ChaseMovementGenerator<T> >(target, offset, angle) { }
        ~ChaseMovementGenerator() { }

        MovementGeneratorType GetMovementGeneratorType() const override { return CHASE_MOTION_TYPE; }

        void DoInitialize(T*);
        void DoFinalize(T*);
        void DoReset(T*);
        void MovementInform(T*);

        static void _clearUnitStateMove(T* u) { u->ClearUnitState(UNIT_STATE_CHASE_MOVE); }
        static void _addUnitStateMove(T* u)  { u->AddUnitState(UNIT_STATE_CHASE_MOVE); }
        bool EnableWalking() const { return false;}
        bool _lostTarget(T* u) const { return u->GetVictim() != this->GetTarget(); }
        void _reachTarget(T*);
};

template<class T>
class FollowMovementGenerator : public TargetedMovementGeneratorMedium<T, FollowMovementGenerator<T> >
{
    public:
        FollowMovementGenerator(Unit* target)
            : TargetedMovementGeneratorMedium<T, FollowMovementGenerator<T> >(target){ }
        FollowMovementGenerator(Unit* target, float offset, float angle)
            : TargetedMovementGeneratorMedium<T, FollowMovementGenerator<T> >(target, offset, angle) { }
        ~FollowMovementGenerator() { }

        MovementGeneratorType GetMovementGeneratorType() const override { return FOLLOW_MOTION_TYPE; }

        void DoInitialize(T*);
        void DoFinalize(T*);
        void DoReset(T*);
        void MovementInform(T*);

        static void _clearUnitStateMove(T* u) { u->ClearUnitState(UNIT_STATE_FOLLOW_MOVE); }
        static void _addUnitStateMove(T* u)  { u->AddUnitState(UNIT_STATE_FOLLOW_MOVE); }
        bool EnableWalking() const;
        bool _lostTarget(T*) const { return false; }
        void _reachTarget(T*) { }
    private:
        void _updateSpeed(T* owner);
};

#endif
