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

#ifndef TRANSPORTS_H
#define TRANSPORTS_H

#include "GameObject.h"
#include "TransportMgr.h"
#include "VehicleDefines.h"
#include "TransportBase.h"

struct CreatureData;
struct SummonPropertiesEntry;

class GAME_API Transport : public GameObject, public TransportBase
{
    public:
        Transport() : GameObject() {}

        typedef std::set<WorldObject*> PassengerSet;
        virtual void AddPassenger(WorldObject* passenger) = 0;
        virtual void RemovePassenger(WorldObject* passenger) = 0;
        PassengerSet const& GetPassengers() const { return _passengers; }

        /// This method transforms supplied transport offsets into global coordinates
        void CalculatePassengerPosition(float& x, float& y, float& z, float* o = NULL) const override
        {
            TransportBase::CalculatePassengerPosition(x, y, z, o, GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());
        }

        /// This method transforms supplied global coordinates into local offsets
        void CalculatePassengerOffset(float& x, float& y, float& z, float* o = NULL) const override
        {
            TransportBase::CalculatePassengerOffset(x, y, z, o, GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());
        }

        uint32 GetPathProgress() const { return GetGOValue()->Transport.PathProgress; }
        void SetPathProgress(uint32 val) { m_goValue.Transport.PathProgress = val; }

    protected:
        PassengerSet _passengers;
};

// GO type 15 - GAMEOBJECT_TYPE_MO_TRANSPORT
class GAME_API MotionTransport : public Transport
{
        friend MotionTransport* TransportMgr::CreateTransport(uint32, uint32, Map*);

    public:
        static MotionTransport* Create(uint32 entry, uint32 mapId, Position const& pos, uint32 animprogress, uint32 guidLow = 0);

        ~MotionTransport();

    protected:
        MotionTransport();

        bool Create(uint32 guidlow, uint32 entry, uint32 mapid, float x, float y, float z, float ang, uint32 animprogress);

    public:
        void CleanupsBeforeDelete(bool finalCleanup = true) override;

        void Update(uint32 diff) override;

        void AddPassenger(WorldObject* passenger) override;
        void RemovePassenger(WorldObject* passenger) override;

        Creature* CreateNPCPassenger(uint32 guid, CreatureData const* data);
        GameObject* CreateGOPassenger(uint32 guid, GameObjectData const* data);

        /**
        * @fn bool Transport::SummonPassenger(uint64, Position const&, TempSummonType, SummonPropertiesEntry const*, uint32, Unit*, uint32, uint32)
        *
        * @brief Temporarily summons a creature as passenger on this transport.
        *
        * @param entry Id of the creature from creature_template table
        * @param pos Initial position of the creature (transport offsets)
        * @param summonType
        * @param properties
        * @param duration Determines how long the creauture will exist in world depending on @summonType (in milliseconds)
        * @param summoner Summoner of the creature (for AI purposes)
        * @param spellId
        * @param vehId If set, this value overrides vehicle id from creature_template that the creature will use
        *
        * @return Summoned creature.
        */
        TempSummon* SummonPassenger(uint32 entry, Position const& pos, TempSummonType summonType, SummonPropertiesEntry const* properties = NULL, uint32 duration = 0, Unit* summoner = NULL, uint32 spellId = 0, uint32 vehId = 0);

        uint32 GetPeriod() const { return GetUInt32Value(GAMEOBJECT_LEVEL); }
        void SetPeriod(uint32 period) { SetUInt32Value(GAMEOBJECT_LEVEL, period); }
        uint32 GetTimer() const { return GetGOValue()->Transport.PathProgress; }

        KeyFrameVec const& GetKeyFrames() const { return _transportInfo->keyFrames; }

        void UpdatePosition(float x, float y, float z, float o);

        //! Needed when transport moves from inactive to active grid
        void LoadStaticPassengers();

        //! Needed when transport enters inactive grid
        void UnloadStaticPassengers();

        void EnableMovement(bool enabled);

        TransportTemplate const* GetTransportTemplate() const { return _transportInfo; }

        VisibilityGroup GetPassengerVisibilityGroup(WorldObject* object);

    private:
        void MoveToNextWaypoint();
        float CalculateSegmentPos(float perc);
        bool TeleportTransport(uint32 newMapid, float x, float y, float z, float o);
        void UpdatePassengerPositions(PassengerSet& passengers);
        void DoEventIfAny(KeyFrame const& node, bool departure);

        //! Helpers to know if stop frame was reached
        bool IsMoving() const { return _isMoving; }
        void SetMoving(bool val) { _isMoving = val; }

        TransportTemplate const* _transportInfo;

        KeyFrameVec::const_iterator _currentFrame;
        KeyFrameVec::const_iterator _nextFrame;
        TimeTrackerSmall _positionChangeTimer;
        bool _isMoving;
        bool _pendingStop;

        //! These are needed to properly control events triggering only once for each frame
        bool _triggeredArrivalEvent;
        bool _triggeredDepartureEvent;

        PassengerSet _passengers;
        PassengerSet::iterator _passengerTeleportItr;
        PassengerSet _staticPassengers;

        VisibilityGroup _passengerVisiblityGroup;
};

// GO type 11 - GAMEOBJECT_TYPE_TRANSPORT
class StaticTransport : public Transport
{
    public:
	    StaticTransport(); //< Use GameObject::Create which automatically creates a static transport based on the entry
	    ~StaticTransport();

	    void CleanupsBeforeDelete(bool finalCleanup = true);

        void Update(uint32 diff);
	    void RelocateToProgress(uint32 progress);
        void UpdatePosition(float x, float y, float z, float o);
        void UpdatePassengerPositions();

        void AddPassenger(WorldObject* passenger) override;
        void RemovePassenger(WorldObject* passenger) override;

        uint32 GetPauseTime() const { return GetUInt32Value(GAMEOBJECT_LEVEL); }
        void SetPauseTime(uint32 val) { SetUInt32Value(GAMEOBJECT_LEVEL, val); }
        uint32 GetPeriod() const { return m_goValue.Transport.AnimationInfo ? m_goValue.Transport.AnimationInfo->TotalTime : GetPauseTime()+2; }
    private:
	    bool _needDoInitialRelocation;
};

#endif
