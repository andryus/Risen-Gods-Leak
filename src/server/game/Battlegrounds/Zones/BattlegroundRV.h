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
#ifndef __BATTLEGROUNDRV_H
#define __BATTLEGROUNDRV_H

#include "Battleground.h"
#include "CreatureAIImpl.h"

#define PILLAR_COLLISION_COUNT 2

enum BattlegroundRVObjectTypes
{
    BG_RV_OBJECT_BUFF_1,
    BG_RV_OBJECT_BUFF_2,
    BG_RV_OBJECT_FIRE_1,
    BG_RV_OBJECT_FIRE_2,
    BG_RV_OBJECT_FIREDOOR_1,
    BG_RV_OBJECT_FIREDOOR_2,

    // 1 and 3 belong together
    BG_RV_OBJECT_PILAR_1,
    BG_RV_OBJECT_PILAR_3,
    BG_RV_OBJECT_PILAR_COLLISION_1_1,
    BG_RV_OBJECT_PILAR_COLLISION_1_2,
    BG_RV_OBJECT_PILAR_COLLISION_3_1,
    BG_RV_OBJECT_PILAR_COLLISION_3_2,

    // 2 and 4 belong together
    BG_RV_OBJECT_PILAR_2,
    BG_RV_OBJECT_PILAR_4,
    BG_RV_OBJECT_PILAR_COLLISION_2_1,
    BG_RV_OBJECT_PILAR_COLLISION_2_2,
    BG_RV_OBJECT_PILAR_COLLISION_4_1,
    BG_RV_OBJECT_PILAR_COLLISION_4_2,

    // belongs to pillar 1 and 3
    BG_RV_OBJECT_GEAR_1,
    BG_RV_OBJECT_GEAR_2,
    // belongs to pillar 2 and 4
    BG_RV_OBJECT_PULLEY_1,
    BG_RV_OBJECT_PULLEY_2,

    BG_RV_OBJECT_ELEVATOR_1,
    BG_RV_OBJECT_ELEVATOR_2,
    BG_RV_OBJECT_FENCE_1,
    BG_RV_OBJECT_FENCE_2,

    BG_RV_READY_MARKER_H,
    BG_RV_READY_MARKER_A,
    BG_RV_OBJECT_MAX
};

enum BattlegroundRVObjects
{
    BG_RV_OBJECT_TYPE_BUFF_1                     = 184663,
    BG_RV_OBJECT_TYPE_BUFF_2                     = 184664,

    BG_RV_OBJECT_TYPE_FIRE_1                     = 192704,
    BG_RV_OBJECT_TYPE_FIRE_2                     = 192705,
    BG_RV_OBJECT_TYPE_FIREDOOR_2                 = 192387,
    BG_RV_OBJECT_TYPE_FIREDOOR_1                 = 192388,

    BG_RV_OBJECT_TYPE_PULLEY_1                   = 192389, //! visual GOs
    BG_RV_OBJECT_TYPE_PULLEY_2                   = 192390,
    BG_RV_OBJECT_TYPE_GEAR_1                     = 192393,
    BG_RV_OBJECT_TYPE_GEAR_2                     = 192394,

    BG_RV_OBJECT_TYPE_FENCE_1                    = 192391, //! fence around the elevators before the start
    BG_RV_OBJECT_TYPE_FENCE_2                    = 192392,
    BG_RV_OBJECT_TYPE_ELEVATOR_1                 = 194582,
    BG_RV_OBJECT_TYPE_ELEVATOR_2                 = 194586,

    BG_RV_OBJECT_TYPE_PILAR_COLLISION_1          = 194580, // axe
    BG_RV_OBJECT_TYPE_PILAR_COLLISION_2          = 194579, // arena
    BG_RV_OBJECT_TYPE_PILAR_COLLISION_3          = 194581, // lightning
    BG_RV_OBJECT_TYPE_PILAR_COLLISION_4          = 194578, // ivory

    BG_RV_OBJECT_TYPE_PILAR_1                    = 194583, // axe
    BG_RV_OBJECT_TYPE_PILAR_2                    = 194584, // arena
    BG_RV_OBJECT_TYPE_PILAR_3                    = 194585, // lightning
    BG_RV_OBJECT_TYPE_PILAR_4                    = 194587  // ivory
};

enum BattlegroundRVData
{
    // Events
    EVENT_BG_RV_TOGGLE_FIRST_PILLAR         = 1,
    EVENT_BG_RV_TOGGLE_NEXT_PILLAR,
    EVENT_BG_RV_CLOSE_FIRE_DOOR,
    EVENT_BG_RV_REMOVE_TRANSPORT_FLAG,
    EVENT_BG_RV_ADD_TRANSPORT_FLAG,

    // Timer
    TIMER_BG_RV_TOGGLE_FIRST_PILLAR         = 10000,
    TIMER_BG_RV_EVENT_TOGGLE_NEXT_PILLAR    = 10000,
    TIMER_BG_RV_CLOSE_FIRE_DOOR             = 5000,
    TIMER_BG_RV_REMOVE_TRANSPORT_FLAG       = 2000,
    TIMER_BG_RV_ADD_TRANSPORT_FLAG          = 9000,

    //! worldstates
    BG_RV_WORLD_STATE_A                     = 0xe10,
    BG_RV_WORLD_STATE_H                     = 0xe11,
    BG_RV_WORLD_STATE                       = 0xe1a,
};

class BattlegroundRV : public Battleground
{
    public:
        BattlegroundRV();

        /* inherited from BattlegroundClass */
        void AddPlayer(Player* player);
        void StartingEventOpenDoors();
        void FillInitialWorldStates(WorldPacket &d);

        void RemovePlayer(Player* player, ObjectGuid guid, uint32 team);
        void HandleAreaTrigger(Player* Source, uint32 Trigger);
        bool SetupBattleground();
        void HandleKillPlayer(Player* player, Player* killer);
        bool HandlePlayerUnderMap(Player* player);
        bool IsPlayerUnderMap(float z);
        void DoAction(uint32 action, ObjectGuid var);

    private:
        EventMap _events;
        uint8 loweredPillar;

        void PostUpdateImpl(uint32 diff);

    protected:
        void TogglePillar(uint8 pillar, bool movePlayers);
        void UseGameObject(uint32 type);
        void GetPillarTypes(uint8 pillar, uint8& pillarType, uint8& collType, uint8& accType);
        void RemoveTransportFlag(uint8 pillar);
        void AddTransportFlag(uint8 pillar);
        void GameObjectSendUpdateToPlayers(uint32 type);
};

#endif
