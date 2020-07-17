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

#include "Battleground.h"
#include "BattlegroundRV.h"
#include "ObjectAccessor.h"
#include "Language.h"
#include "Player.h"
#include "WorldPacket.h"
#include "GameObject.h"
#include "Map.h"

BattlegroundRV::BattlegroundRV()
{
    BgObjects.resize(BG_RV_OBJECT_MAX);

    StartDelayTimes[BG_STARTING_EVENT_FIRST]  = BG_START_DELAY_1M;
    StartDelayTimes[BG_STARTING_EVENT_SECOND] = BG_START_DELAY_30S;
    StartDelayTimes[BG_STARTING_EVENT_THIRD]  = BG_START_DELAY_15S;
    StartDelayTimes[BG_STARTING_EVENT_FOURTH] = BG_START_DELAY_NONE;
    StartMessageIds[BG_STARTING_EVENT_FIRST]  = LANG_ARENA_ONE_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_ARENA_THIRTY_SECONDS;
    StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_ARENA_FIFTEEN_SECONDS;
    StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_ARENA_HAS_BEGUN;
}

void BattlegroundRV::PostUpdateImpl(uint32 diff)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
    {
        if (GetStatus() == STATUS_WAIT_JOIN)
        {
            if (GetStartDelayTime() <= 5000 && loweredPillar == 0)
            {
                loweredPillar = (urand(1, 2) == 1 ? 1 : 3);
                for (uint8 p = 1; p <= 4; p++)
                {
                    TogglePillar(p, false);
                    RemoveTransportFlag(p);
                }
                DelObject(BG_RV_OBJECT_FIRE_1);
                DelObject(BG_RV_OBJECT_FIRE_2);
            }
        }
        return;
    }

    _events.Update(diff);

    while (uint32 eventId = _events.ExecuteEvent())
    {
        switch (eventId)
        {
            case EVENT_BG_RV_CLOSE_FIRE_DOOR:
                // close fire doors at game start
                for (uint8 i = BG_RV_OBJECT_FIRE_1; i <= BG_RV_OBJECT_FIREDOOR_2; ++i)
                    DoorClose(i);
                break;
            case EVENT_BG_RV_TOGGLE_FIRST_PILLAR:
                TogglePillar(loweredPillar, false);
                _events.ScheduleEvent(EVENT_BG_RV_ADD_TRANSPORT_FLAG, TIMER_BG_RV_ADD_TRANSPORT_FLAG);
                _events.ScheduleEvent(EVENT_BG_RV_TOGGLE_NEXT_PILLAR, TIMER_BG_RV_EVENT_TOGGLE_NEXT_PILLAR);
                break;
            case EVENT_BG_RV_TOGGLE_NEXT_PILLAR:
                TogglePillar(loweredPillar, true);
                loweredPillar = (loweredPillar % 4) + 1;
                TogglePillar(loweredPillar, false);
                _events.ScheduleEvent(EVENT_BG_RV_REMOVE_TRANSPORT_FLAG, TIMER_BG_RV_REMOVE_TRANSPORT_FLAG);
                _events.ScheduleEvent(EVENT_BG_RV_ADD_TRANSPORT_FLAG, TIMER_BG_RV_ADD_TRANSPORT_FLAG);
                _events.ScheduleEvent(EVENT_BG_RV_TOGGLE_NEXT_PILLAR, TIMER_BG_RV_EVENT_TOGGLE_NEXT_PILLAR);
                break;
            case EVENT_BG_RV_REMOVE_TRANSPORT_FLAG:
                RemoveTransportFlag(loweredPillar);
                RemoveTransportFlag((loweredPillar - 1) == 0 ? 4 : (loweredPillar - 1));
                break;
            case EVENT_BG_RV_ADD_TRANSPORT_FLAG:
                AddTransportFlag(loweredPillar);
                AddTransportFlag((loweredPillar % 4) + 1);
                break;
            default:
                break;
        }
    }
}

void BattlegroundRV::StartingEventOpenDoors()
{
    //! Buff respawn
    for (uint32 i = BG_RV_OBJECT_BUFF_1; i <= BG_RV_OBJECT_BUFF_2; ++i)
        SpawnBGObject(i, 60);

    // open fence at game start
    for (uint8 i = BG_RV_OBJECT_FENCE_1; i <= BG_RV_OBJECT_FENCE_2; ++i)
        DoorOpen(i);

    //disable pillars due to, e.g., totems :(
    //_events.ScheduleEvent(EVENT_BG_RV_TOGGLE_FIRST_PILLAR, TIMER_BG_RV_TOGGLE_FIRST_PILLAR);

    DelObject(BG_RV_READY_MARKER_H);
    DelObject(BG_RV_READY_MARKER_A);
}

void BattlegroundRV::AddPlayer(Player* player)
{
    Battleground::AddPlayer(player);
    PlayerScores[player->GetGUID()] = new BattlegroundScore;

    UpdateWorldState(BG_RV_WORLD_STATE_A, GetAlivePlayersCountByTeam(ALLIANCE));
    UpdateWorldState(BG_RV_WORLD_STATE_H, GetAlivePlayersCountByTeam(HORDE));
}

void BattlegroundRV::RemovePlayer(Player* player, ObjectGuid guid, uint32 team)
{
    if (GetStatus() == STATUS_WAIT_LEAVE)
        return;

    UpdateWorldState(BG_RV_WORLD_STATE_A, GetAlivePlayersCountByTeam(ALLIANCE));
    UpdateWorldState(BG_RV_WORLD_STATE_H, GetAlivePlayersCountByTeam(HORDE));

    CheckArenaWinConditions();

    Battleground::RemovePlayer(player, guid, team);
}

void BattlegroundRV::HandleKillPlayer(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if (!killer)
    {
        TC_LOG_ERROR("bg.battleground", "BattlegroundRV: Killer player not found");
        return;
    }

    Battleground::HandleKillPlayer(player, killer);

    UpdateWorldState(BG_RV_WORLD_STATE_A, GetAlivePlayersCountByTeam(ALLIANCE));
    UpdateWorldState(BG_RV_WORLD_STATE_H, GetAlivePlayersCountByTeam(HORDE));

    CheckArenaWinConditions();
}

bool BattlegroundRV::IsPlayerUnderMap(float z)
{
    return (z < ARENA_RV_GROUND && GetStatus() != STATUS_WAIT_JOIN);
}

bool BattlegroundRV::HandlePlayerUnderMap(Player* player)
{
    for (uint8 pillar = 1; pillar <= 4; pillar++)
    {
        uint8 coll = 0;
        uint8 acc = 0;
        uint8 p = 0;

        GetPillarTypes(pillar, p, coll, acc);

        if (GameObject* go = GetBgMap()->GetGameObject(BgObjects[p]))
        {
            if (player->GetDistance2d(go) <= ((pillar == 1 || pillar == 3) ? 2.0f : 4.2f))
            {
                G3D::Vector3 v;
                v.x = player->GetPositionX() - go->GetPositionX();
                v.y = player->GetPositionY() - go->GetPositionY();
                v.z = 0;
                v = v.unit();
                v *= ((pillar == 1 || pillar == 3) ? 2.2f : 4.4f);
                player->NearTeleportTo(go->GetPositionX() + v.x, go->GetPositionY() + v.y, 28.276f, player->GetOrientation());
                return true;
            }
        }

    }
    player->TeleportTo(GetMapId(), 763.5f, -284, 28.276f, 2.422f);
    return true;
}

void BattlegroundRV::HandleAreaTrigger(Player* player, uint32 trigger)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    switch (trigger)
    {
        case 5224:
        case 5226:
            break;
        case 5447:
            HandlePlayerUnderMap(player);
            break;
        // fire was removed in 3.2.0
        case 5473:
        case 5474:
            break;
        default:
            Battleground::HandleAreaTrigger(player, trigger);
            break;
    }
}

void BattlegroundRV::FillInitialWorldStates(WorldPacket &data)
{
    data << uint32(BG_RV_WORLD_STATE_A) << uint32(GetAlivePlayersCountByTeam(ALLIANCE));
    data << uint32(BG_RV_WORLD_STATE_H) << uint32(GetAlivePlayersCountByTeam(HORDE));
    data << uint32(BG_RV_WORLD_STATE)   << uint32(1);
}

bool BattlegroundRV::SetupBattleground()
{
    float odiff = M_PI_F / (2.0f * PILLAR_COLLISION_COUNT);
    // elevators
    if (!AddObject(BG_RV_OBJECT_ELEVATOR_1, BG_RV_OBJECT_TYPE_ELEVATOR_1, 763.536377f, -294.535767f, 26.015383f, 3.141593f, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_ELEVATOR_2, BG_RV_OBJECT_TYPE_ELEVATOR_2, 763.506348f, -273.873352f, 26.015383f, 0.000000f, 0, 0, 0, RESPAWN_IMMEDIATELY)
    // fence
        || !AddObject(BG_RV_OBJECT_FENCE_1, BG_RV_OBJECT_TYPE_FENCE_1, 730.474121f, -299.783386f, 28.276712f, 6.251659f, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_FENCE_2, BG_RV_OBJECT_TYPE_FENCE_2, 796.802490f, -268.913544f, 28.276751f, 3.121081f, 0, 0, 0, RESPAWN_IMMEDIATELY)
    // buffs
        || !AddObject(BG_RV_OBJECT_BUFF_1, BG_RV_OBJECT_TYPE_BUFF_1, 763.768555f, -248.416779f, 28.276682f, 0.034906f, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_BUFF_2, BG_RV_OBJECT_TYPE_BUFF_2, 763.866577f, -319.967896f, 28.276682f, 2.600535f, 0, 0, 0, RESPAWN_IMMEDIATELY)
    // fire
        || !AddObject(BG_RV_OBJECT_FIRE_1, BG_RV_OBJECT_TYPE_FIRE_1, 743.543457f, -283.799469f, 28.286655f, 3.141593f, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_FIRE_2, BG_RV_OBJECT_TYPE_FIRE_2, 782.971802f, -283.799469f, 28.286655f, 3.141593f, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_FIREDOOR_1, BG_RV_OBJECT_TYPE_FIREDOOR_1, 743.711060f, -284.099609f, 27.542587f, 3.141593f, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_FIREDOOR_2, BG_RV_OBJECT_TYPE_FIREDOOR_2, 783.221252f, -284.133362f, 27.535686f, 0.000000f, 0, 0, 0, RESPAWN_IMMEDIATELY)
    // Gear
        || !AddObject(BG_RV_OBJECT_GEAR_1, BG_RV_OBJECT_TYPE_GEAR_1, 763.664551f, -261.872986f, 26.686588f, 0.000000f, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_GEAR_2, BG_RV_OBJECT_TYPE_GEAR_2, 763.578979f, -306.146149f, 26.665222f, 3.141593f, 0, 0, 0, RESPAWN_IMMEDIATELY)
    // Pulley
        || !AddObject(BG_RV_OBJECT_PULLEY_1, BG_RV_OBJECT_TYPE_PULLEY_1, 700.722290f, -283.990662f, 39.517582f, 3.141593f, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PULLEY_2, BG_RV_OBJECT_TYPE_PULLEY_2, 826.303833f, -283.996429f, 39.517582f, 0.000000f, 0, 0, 0, RESPAWN_IMMEDIATELY)
    // Pilars
        || !AddObject(BG_RV_OBJECT_PILAR_1, BG_RV_OBJECT_TYPE_PILAR_1, 763.632385f, -306.162384f, 25.909504f, 3.141593f, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PILAR_2, BG_RV_OBJECT_TYPE_PILAR_2, 723.644287f, -284.493256f, 24.648525f, 3.141593f, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PILAR_3, BG_RV_OBJECT_TYPE_PILAR_3, 763.611145f, -261.856750f, 25.909504f, 0.000000f, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PILAR_4, BG_RV_OBJECT_TYPE_PILAR_4, 802.211609f, -284.493256f, 24.648525f, 0.000000f, 0, 0, 0, RESPAWN_IMMEDIATELY)
    // Pilars Collision
        || !AddObject(BG_RV_OBJECT_PILAR_COLLISION_1_1, BG_RV_OBJECT_TYPE_PILAR_COLLISION_1, 763.632385f, -306.162384f, 30.639660f, 3.141593f, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PILAR_COLLISION_1_2, BG_RV_OBJECT_TYPE_PILAR_COLLISION_1, 763.632385f, -306.162384f, 30.639660f, 3.141593f + odiff * 1, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PILAR_COLLISION_2_1, BG_RV_OBJECT_TYPE_PILAR_COLLISION_2, 723.644287f, -284.493256f, 32.382710f, 0.000000f,             0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PILAR_COLLISION_2_2, BG_RV_OBJECT_TYPE_PILAR_COLLISION_2, 723.644287f, -284.493256f, 32.382710f, 0.000000f + odiff * 1, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PILAR_COLLISION_3_1, BG_RV_OBJECT_TYPE_PILAR_COLLISION_3, 763.611145f, -261.856750f, 30.639660f, 4.712389f,             0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PILAR_COLLISION_3_2, BG_RV_OBJECT_TYPE_PILAR_COLLISION_3, 763.611145f, -261.856750f, 30.639660f, 4.712389f + odiff * 1, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PILAR_COLLISION_4_1, BG_RV_OBJECT_TYPE_PILAR_COLLISION_4, 802.211609f, -284.493256f, 32.382710f, 3.141593f,             0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PILAR_COLLISION_4_2, BG_RV_OBJECT_TYPE_PILAR_COLLISION_4, 802.211609f, -284.493256f, 32.382710f, 3.141593f + odiff * 1, 0, 0, 0, RESPAWN_IMMEDIATELY))
    {
        TC_LOG_ERROR("sql.sql", "BatteGroundRV: Failed to spawn some object!");
        return false;
    }

    float xH, yH, zH, oH, xA, yA, zA, oA;
    xH = 0; yH = 0; zH = 0; oH = 3.16124f; xA = 0; yA = 0; zA = 0; oA = 0;
    GetTeamStartLoc(HORDE, xH, yH, zH, oH);
    GetTeamStartLoc(ALLIANCE, xA, yA, zA, oA);
    if (!AddObject(BG_RV_READY_MARKER_H, GO_READY_MARKER, xH+1.8f, yH+2.2f, zH, oH, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_READY_MARKER_A, GO_READY_MARKER, xA+1.8f, yA+2.2f, zA, oA, 0, 0, 0, 0, RESPAWN_IMMEDIATELY))
    {
        TC_LOG_ERROR("sql.sql", "BatteGroundRV: Failed to spawn ready-marker!");
        return false;
    }

    for (uint8 p = 1; p <= 4; p++)
        TogglePillar(p, false);
    loweredPillar = 0;

    UseGameObject(BG_RV_OBJECT_FIRE_1);
    UseGameObject(BG_RV_OBJECT_FIRE_2);

    return true;
}

void BattlegroundRV::GetPillarTypes(uint8 pillar, uint8& pillarType, uint8& collType, uint8& accType)
{
    switch (pillar)
    {
    case 1:
        collType = BG_RV_OBJECT_PILAR_COLLISION_1_1;
        pillarType = BG_RV_OBJECT_PILAR_1;
        accType = BG_RV_OBJECT_GEAR_2;
        break;
    case 2:
        collType = BG_RV_OBJECT_PILAR_COLLISION_2_1;
        pillarType = BG_RV_OBJECT_PILAR_2;
        accType = BG_RV_OBJECT_PULLEY_1;
        break;
    case 3:
        collType = BG_RV_OBJECT_PILAR_COLLISION_3_1;
        pillarType = BG_RV_OBJECT_PILAR_3;
        accType = BG_RV_OBJECT_GEAR_1;
        break;
    case 4:
        collType = BG_RV_OBJECT_PILAR_COLLISION_4_1;
        pillarType = BG_RV_OBJECT_PILAR_4;
        accType = BG_RV_OBJECT_PULLEY_2;
        break;
    default:
        return;
    }
}

void BattlegroundRV::RemoveTransportFlag(uint8 pillar)
{
    uint8 coll = 0;
    uint8 acc = 0;
    uint8 p = 0;

    GetPillarTypes(pillar, p, coll, acc);
    if (GameObject* go = GetBgMap()->GetGameObject(BgObjects[p]))
        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_TRANSPORT);

    GameObjectSendUpdateToPlayers(p);
}

void BattlegroundRV::AddTransportFlag(uint8 pillar)
{
    uint8 coll = 0;
    uint8 acc = 0;
    uint8 p = 0;

    GetPillarTypes(pillar, p, coll, acc);
    if (GameObject* go = GetBgMap()->GetGameObject(BgObjects[p]))
        go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_TRANSPORT);

    GameObjectSendUpdateToPlayers(p);
}

void BattlegroundRV::TogglePillar(uint8 pillar, bool movePlayers)
{
    uint8 coll = 0;
    uint8 acc = 0;
    uint8 p = 0;

    GetPillarTypes(pillar, p, coll, acc);

    if (GameObject* go = GetBgMap()->GetGameObject(BgObjects[p]))
    {
        if (movePlayers)
        {
            for (BattlegroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
            {
                Player* player = ObjectAccessor::GetPlayer(*GetBgMap(), itr->first);

                if (player && player->GetDistance2d(go) <= ((pillar == 1 || pillar == 3) ? 2.0f : 4.2f))
                {
                    player->NearTeleportTo(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ() + 0.1f, player->GetOrientation());
                }
            }
        }
        UseGameObject(p);
    }
    for (uint8 i = 0; i < PILLAR_COLLISION_COUNT; i++)
    {
        if (GameObject* go = GetBgMap()->GetGameObject(BgObjects[coll + i]))
            UseGameObject(coll + i);
    }
    if (GameObject* go = GetBgMap()->GetGameObject(BgObjects[acc]))
        UseGameObject(acc);

    GameObjectSendUpdateToPlayers(p);
}

void BattlegroundRV::DoAction(uint32 action, ObjectGuid var)
{
    if (GetStatus() == STATUS_IN_PROGRESS && action == 1)
    {
        if (Creature* c = GetBgMap()->GetCreature(ObjectGuid(var)))
        {
            for (uint8 pillar = 1; pillar <= 4; pillar++)
            {
                uint8 coll = 0;
                uint8 acc = 0;
                uint8 p = 0;

                GetPillarTypes(pillar, p, coll, acc);

                if (GameObject* go = GetBgMap()->GetGameObject(BgObjects[p]))
                {
                    if (c->GetDistance2d(go) <= ((pillar == 1 || pillar == 3) ? 2.0f : 4.2f))
                    {
                        G3D::Vector3 v;
                        v.x = c->GetPositionX() - go->GetPositionX();
                        v.y = c->GetPositionY() - go->GetPositionY();
                        v.z = 0;
                        v = v.unit();
                        v *= ((pillar == 1 || pillar == 3) ? 2.2f : 4.4f);
                        c->NearTeleportTo(go->GetPositionX() + v.x, go->GetPositionY() + v.y, 28.276f, c->GetOrientation());
                        break;
                    }
                }

            }
        }
    }
}

void BattlegroundRV::GameObjectSendUpdateToPlayers(uint32 type)
{
    for (BattlegroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
    {
        Player* player = ObjectAccessor::GetPlayer(*GetBgMap(), itr->first);
        if (!player)
            continue;

        if (GameObject* go = GetBGObject(type))
            go->SendUpdateToPlayer(player);
    }
}

void BattlegroundRV::UseGameObject(uint32 type)
{
    if (GameObject* go = GetBgMap()->GetGameObject(BgObjects[type]))
    {
        go->SetLootState(GO_READY);
        go->UseDoorOrButton(10000, false, NULL);
    }
}
