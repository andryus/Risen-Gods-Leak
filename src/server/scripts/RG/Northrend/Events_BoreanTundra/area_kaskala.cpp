/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * Copyright (C) 2010-2017 Rising Gods <http://www.rising-gods.de/>
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

#include "PassiveAI.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "WaypointManager.h"
#include "Vehicle.h"

enum LandingShipEvent
{
    NPC_KVALDIR_RAIDER      = 25760, // Attacker
    NPC_KASKALA_DEFENDER    = 25764, // Defender
    NPC_LANDING_SHIP        = 25762, // Vehicle

    NPC_LANDINGZONE_1       = 31112800, // guid + 0 (waypoint_data)
    NPC_LANDINGZONE_2       = 31112810,
    NPC_LANDINGZONE_3       = 31112820,
    NPC_LANDINGZONE_4       = 31112830,
    NPC_LANDINGZONE_5       = 31112840,
    NPC_LANDINGZONE_6       = 31112850,
    NPC_LANDINGZONE_7       = 31112860,

    MAX_COUNT_DEFENDERS     = 5,

    EVENT_SPAWN_NEW_SHIP    = 1,
    EVENT_DO_LANDING        = 2,
    EVENT_ATTACK_START      = 3,

    DATA_SPAWN_ATTACKERS    = 0,
    DATA_LANDING_FORMATION  = 1,
    DATA_SELECT_WAYPOINT    = 2,
};

const Position DefenderSpawnPosition[35] =
{
    { 3037.59f, 4629.62f, 2.221f, 3.944f }, // Landingzone 1 Start
    { 3035.47f, 4618.12f, 2.248f, 3.825f },
    { 3049.23f, 4619.75f, 2.172f, 4.174f },
    { 3046.68f, 4600.72f, 3.932f, 3.659f },
    { 3055.37f, 4588.54f, 3.782f, 3.608f }, // Landingzone 1 End
    { 3065.90f, 4709.92f, 4.179f, 3.623f }, // Landingzone 2 Start
    { 3061.29f, 4715.08f, 2.960f, 3.905f },
    { 3074.07f, 4709.78f, 5.290f, 3.519f },
    { 3070.79f, 4717.49f, 3.940f, 3.528f },
    { 3062.24f, 4723.62f, 1.039f, 3.940f }, // Landingzone 2 End
    { 2988.11f, 4881.86f, 0.396f, 3.812f }, // Landingzone 3 Start
    { 2981.58f, 4891.89f, 0.478f, 3.334f },
    { 2988.70f, 4903.20f, 0.478f, 3.493f },
    { 2993.90f, 4895.58f, 0.396f, 3.477f },
    { 2999.89f, 4886.54f, 0.479f, 3.926f }, // Landingzone 3 End
    { 2836.84f, 4924.77f, 3.661f, 4.130f }, // Landingzone 4 Start
    { 2843.36f, 4907.04f, 2.622f, 3.089f },
    { 2856.52f, 4905.89f, 2.622f, 3.001f },
    { 2853.02f, 4920.29f, 3.561f, 3.824f },
    { 2842.44f, 4940.04f, 8.010f, 4.201f }, // Landingzone 4 End
    { 2777.33f, 4733.49f, 2.358f, 2.816f }, // Landingzone 5 Start
    { 2761.12f, 4716.25f, 3.227f, 2.329f },
    { 2757.98f, 4709.10f, 2.984f, 2.329f },
    { 2779.15f, 4720.76f, 3.338f, 3.046f },
    { 2784.26f, 4743.75f, 1.923f, 3.411f }, // Landingzone 5 End
    { 2778.59f, 4674.36f, 2.603f, 3.451f }, // Landingzone 6 Start
    { 2768.20f, 4668.87f, 2.661f, 3.094f },
    { 2779.13f, 4659.61f, 2.560f, 3.019f },
    { 2767.53f, 4651.40f, 2.459f, 2.807f },
    { 2773.93f, 4643.28f, 2.553f, 2.646f }, // Landingzone 6 End
    { 2829.62f, 4639.19f, 2.169f, 5.078f }, // Landingzone 7 Start
    { 2823.31f, 4640.84f, 2.530f, 5.351f },
    { 2822.97f, 4632.77f, 2.561f, 5.296f },
    { 2816.26f, 4635.98f, 2.560f, 5.084f },
    { 2814.12f, 4631.31f, 2.561f, 5.319f }, // Landingzone 7 End
};

const Position AttackerFormationLanding[35] =
{
    { 3008.939f, 4599.650f, 2.130f },       // Landingzone 1 Start
    { 3016.379f, 4597.209f, 2.248f },
    { 3014.979f, 4585.750f, 2.153f },
    { 3018.310f, 4574.899f, 1.776f },
    { 3022.280f, 4586.689f, 2.248f },       // Landingzone 1 End
    { 3043.040f, 4675.319f, 2.184f },       // Landingzone 2 Start
    { 3050.379f, 4686.490f, 1.318f },
    { 3042.030f, 4695.160f, 2.224f },
    { 3047.919f, 4703.310f, 2.247f },
    { 3042.320f, 4709.430f, 1.719f },       // Landingzone 2 End
    { 2928.940f, 4882.273f, 1.988f },       // Landingzone 3 Start
    { 2939.302f, 4883.741f, 2.505f },
    { 2938.787f, 4875.417f, 1.964f },
    { 2947.394f, 4873.381f, 0.396f },
    { 2942.688f, 4865.227f, 0.396f },       // Landingzone 3 End
    { 2804.899f, 4906.140f, 2.308f },       // Landingzone 4 Start
    { 2810.570f, 4897.589f, 2.659f },
    { 2817.889f, 4889.399f, 2.192f },
    { 2819.719f, 4897.250f, 2.558f },
    { 2814.580f, 4907.919f, 2.246f },       // Landingzone 4 End
    { 2739.729f, 4755.509f, 1.913f },       // Landingzone 5 Start
    { 2733.250f, 4750.839f, 2.241f },
    { 2738.399f, 4738.859f, 2.420f },
    { 2726.320f, 4732.253f, 1.571f },
    { 2723.719f, 4718.169f, 6.195f },       // Landingzone 5 End
    { 2694.739f, 4664.850f, 2.267f },       // Landingzone 6 Start
    { 2708.969f, 4675.870f, 3.031f },
    { 2714.159f, 4657.100f, 2.494f },
    { 2693.679f, 4650.240f, 0.728f },
    { 2709.879f, 4637.529f, 2.741f },       // Landingzone 6 End
    { 2821.139f, 4596.569f, 1.540f },       // Landingzone 7 Start
    { 2826.939f, 4603.790f, 2.288f },
    { 2835.260f, 4597.120f, 0.875f },
    { 2844.300f, 4603.970f, 2.139f },
    { 2835.244f, 4604.569f, 2.222f },       // Landingzone 7 End
};

const Position LandingShipPosition[7] =
{
    { 2551.566f, 4453.662f, 0.0f, 0.0f },   // Landingzone 1
    { 2551.566f, 4453.662f, 0.0f, 0.0f },   // Landingzone 2
    { 2562.110f, 4816.061f, 0.0f, 0.0f },   // Landingzone 3
    { 2562.110f, 4816.061f, 0.0f, 0.0f },   // Landingzone 4
    { 2522.770f, 4742.109f, 0.0f, 0.0f },   // Landingzone 5
    { 2459.669f, 4631.883f, 0.0f, 0.0f },   // Landingzone 6
    { 2519.007f, 4575.209f, 0.0f, 0.0f },   // Landingzone 7
};

struct npc_kaskala_landingshipAI : public ScriptedAI
{
    npc_kaskala_landingshipAI(Creature* creature) : ScriptedAI(creature) { }

    void SetData(uint32 id, uint32 value)
    {
        switch (id)
        {
            case DATA_SPAWN_ATTACKERS:
                SummonAttackers(value);
                _sumAttackers = value;
                break;
            case DATA_LANDING_FORMATION:
                _landingPoint = value;
                break;
            case DATA_SELECT_WAYPOINT:
                me->GetMotionMaster()->MovePath(value, false);
                if (WaypointPath const* path = sWaypointMgr->GetPath(value))
                    _lastWP = path->size() - 1;
                break;
            default:
                break;
        }
    }

    void SummonAttackers(uint32 amount)
    {
        std::vector<int8> seatList = { 0, 1, 2, 3, 4, 5, 6 };
        std::random_device random;
        std::mt19937 noIdea(random());
        std::shuffle(seatList.begin(), seatList.end(), noIdea);

        for (uint8 i = 0; i < amount; ++i)
            if (Creature* attacker = DoSummon(NPC_KVALDIR_RAIDER, me, 0.0f, 120 * IN_MILLISECONDS, TEMPSUMMON_TIMED_DESPAWN))
                attacker->EnterVehicle(me, seatList[i]);
    }

    void MovementInform(uint32 type, uint32 id) override 
    {
        if (type != WAYPOINT_MOTION_TYPE)
            return;

        if (id == _lastWP)
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_DO_LANDING, 5 * IN_MILLISECONDS);
        }
    }

    void SetGuidData(ObjectGuid guid, uint32 /*dataId*/) override
    {
        _landingZoneGUID = guid;
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_DO_LANDING:
                    //if (Creature* zone = me->FindNearestCreature(_entryZone, 20.0f))
                    if (Creature* zone = ObjectAccessor::GetCreature(*me, _landingZoneGUID))
                    {
                        zone->AI()->SetData(_sumAttackers, 0);

                        if (Vehicle* ship = me->GetVehicleKit())
                            for (uint8 i = 0; i < 7; ++i)
                                if (Unit* passenger = ship->GetPassenger(i))
                                {
                                    passenger->ExitVehicle();
                                    passenger->GetMotionMaster()->MovePoint(1, AttackerFormationLanding[_landingPoint]);
                                    passenger->ToCreature()->SetHomePosition(AttackerFormationLanding[_landingPoint]);
                                    passenger->SetWalk(false);

                                    ++_landingPoint;

                                    zone->AI()->SetGuidData(passenger->GetGUID());
                                }
                    }
                    me->DespawnOrUnsummon(30 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }
    }

private:
    EventMap    _events;
    uint8       _lastWP;
    uint32      _sumAttackers;
    uint32      _landingPoint;
    ObjectGuid  _landingZoneGUID;
};

template <uint32 INDEX_MIN, uint32 INDEX_SPAWN, uint32 PATH_ID, uint32 SPAWN_TIMER>
struct npc_kaskala_trigger_landingzoneAI : public PassiveAI
{
    npc_kaskala_trigger_landingzoneAI(Creature* creature) : PassiveAI(creature) { }

    void Reset() override
    {
        me->setActive(true);
        SpawnDefender();
        _events.Reset();
        _events.ScheduleEvent(EVENT_SPAWN_NEW_SHIP, urand(1, 10) * IN_MILLISECONDS);
    }

    void SpawnDefender()
    {
        for (uint8 i = 0; i < 5; ++i)
            if (Creature* defender = DoSummon(NPC_KASKALA_DEFENDER, DefenderSpawnPosition[INDEX_MIN + i], 5 * IN_MILLISECONDS))
            {
                _defenderGUID[i] = defender->GetGUID();
                defender->setActive(true);
            }
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        if (summon && summon->GetEntry() == NPC_KASKALA_DEFENDER)
            for (uint8 i = 0; i < 5; ++i)
                if (Creature* defender = ObjectAccessor::GetCreature(*me, _defenderGUID[i]))
                    if (defender->GetGUID() == summon->GetGUID())
                        if (Creature* respawn = DoSummon(NPC_KASKALA_DEFENDER, DefenderSpawnPosition[INDEX_MIN + i]))
                        {
                            respawn->setActive(true);
                            _defenderGUID[i] = respawn->GetGUID();
                        }
    }

    void SetGuidData(ObjectGuid guid, uint32 /*dataId*/) override
    {
        _attackerGUID[_myStatus--] = guid;
    }

    void SetData(uint32 type, uint32 /*data*/) override
    {
        _myStatus = type - 1;
        _attackerGUID->Clear();
        _events.ScheduleEvent(EVENT_ATTACK_START, 10 * IN_MILLISECONDS);
        _events.ScheduleEvent(EVENT_SPAWN_NEW_SHIP, SPAWN_TIMER * IN_MILLISECONDS);
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SPAWN_NEW_SHIP:
                    if (Creature* ship = DoSummon(NPC_LANDING_SHIP, LandingShipPosition[INDEX_SPAWN], 0, TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        ship->AI()->SetGuidData(me->GetGUID());
                        ship->setActive(true);
                        ship->SetSpeedRate(MOVE_WALK, 4.0f);
                        ship->AI()->SetData(DATA_SPAWN_ATTACKERS, 5);
                        ship->AI()->SetData(DATA_SELECT_WAYPOINT, PATH_ID);
                        ship->AI()->SetData(DATA_LANDING_FORMATION, INDEX_MIN);
                    }
                    break;
                case EVENT_ATTACK_START:
                    for (uint8 i = 0; i < 5 ; ++i)
                        if (Creature* attacker = ObjectAccessor::GetCreature(*me, _attackerGUID[i]))
                            if (Creature* defender = ObjectAccessor::GetCreature(*me, _defenderGUID[i]))
                                attacker->GetMotionMaster()->MovePoint(2, defender->GetPositionX(), defender->GetPositionY(), defender->GetPositionZ());
                    break;
                default:
                    break;
            }
        }
    }

private:
    ObjectGuid  _defenderGUID[MAX_COUNT_DEFENDERS];
    ObjectGuid  _attackerGUID[MAX_COUNT_DEFENDERS];
    EventMap    _events;
    uint32      _myStatus;
};

void AddSC_area_kaskala_rg()
{
    new CreatureScriptLoaderEx<npc_kaskala_landingshipAI>("npc_kaskala_landingship");
    new CreatureScriptLoaderEx<npc_kaskala_trigger_landingzoneAI<0 , 0, NPC_LANDINGZONE_1, 40>>("npc_kaskala_trigger_landingzone_1");
    new CreatureScriptLoaderEx<npc_kaskala_trigger_landingzoneAI<5 , 1, NPC_LANDINGZONE_2, 65>>("npc_kaskala_trigger_landingzone_2");
    new CreatureScriptLoaderEx<npc_kaskala_trigger_landingzoneAI<10, 2, NPC_LANDINGZONE_3, 50>>("npc_kaskala_trigger_landingzone_3");
    new CreatureScriptLoaderEx<npc_kaskala_trigger_landingzoneAI<15, 3, NPC_LANDINGZONE_4, 65>>("npc_kaskala_trigger_landingzone_4");
    new CreatureScriptLoaderEx<npc_kaskala_trigger_landingzoneAI<20, 4, NPC_LANDINGZONE_5, 70>>("npc_kaskala_trigger_landingzone_5");
    new CreatureScriptLoaderEx<npc_kaskala_trigger_landingzoneAI<25, 5, NPC_LANDINGZONE_6, 70>>("npc_kaskala_trigger_landingzone_6");
    new CreatureScriptLoaderEx<npc_kaskala_trigger_landingzoneAI<30, 6, NPC_LANDINGZONE_7, 50>>("npc_kaskala_trigger_landingzone_7");
}
