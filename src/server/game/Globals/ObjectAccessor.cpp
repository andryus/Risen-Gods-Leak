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

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

#include "ObjectAccessor.h"
#include "Corpse.h"
#include "Creature.h"
#include "DynamicObject.h"
#include "GameObject.h"
#include "GridNotifiers.h"
#include "Item.h"
#include "Map.h"
#include "Maps/MapManager.h"
#include "ObjectDefines.h"
#include "ObjectMgr.h"
#include "Pet.h"
#include "Player.h"
#include "PlayerCache.hpp"
#include "Transport.h"
#include "World.h"
#include "WorldPacket.h"
#include "Transport.h"

ObjectAccessor::ObjectAccessor() {}
ObjectAccessor::~ObjectAccessor() {}

Player* ObjectAccessor::FindConnectedPlayer(ObjectGuid const& guid)
{
    return HashMapHolder<Player>::Find(guid);
}

Player* ObjectAccessor::FindConnectedPlayerByName(std::string const& name)
{
    if (ObjectGuid const guid = player::PlayerCache::GetGUID(name))
        return FindConnectedPlayer(guid);

    return nullptr;
}

Player* ObjectAccessor::FindPlayer(ObjectGuid const& guid)
{
    Player* player = FindConnectedPlayer(guid);

    if (player && player->IsInWorld())
        return player;

    return nullptr;
}

Player* ObjectAccessor::FindPlayerByName(std::string const& name)
{
    if (ObjectGuid const guid = player::PlayerCache::GetGUID(name))
        return FindPlayer(guid);

    return nullptr;
}

WorldObject* ObjectAccessor::GetWorldObject(Map& map, ObjectGuid const& guid)
{
    switch (guid.GetHigh())
    {
        case HighGuid::Player:
            return GetPlayer(map, guid);
        case HighGuid::Corpse:
            return GetCorpse(map, guid);
        case HighGuid::DynamicObject:
            return GetDynamicObject(map, guid);
        case HighGuid::Gameobject:
        case HighGuid::Transport:
            return GetGameObject(map, guid);
        case HighGuid::Mo_Transport:
            return GetMotionTransport(map, guid);
        case HighGuid::Unit:
        case HighGuid::Vehicle:
            return GetCreature(map, guid);
        case HighGuid::Pet:
            return GetPet(map, guid);
        default:
            return nullptr;
    }
}

WorldObject* ObjectAccessor::GetWorldObject(WorldObject const& obj, ObjectGuid const& guid)
{
    return GetWorldObject(*obj.GetMap(), guid);
}

bool CheckIfHighGuidFitsTypeMask(HighGuid const high, uint32 const typemask)
{
    switch (high) {
        case HighGuid::Item: // also contains HighGuid::Container
            return typemask & TYPEMASK_CONTAINER; // TYPEMASK_CONTAINER includes TYPEMASK_ITEM
        case HighGuid::Player:
            return typemask & TYPEMASK_PLAYER;
        case HighGuid::Gameobject:
        case HighGuid::Transport:
        case HighGuid::Mo_Transport:
            return typemask & TYPEMASK_GAMEOBJECT;
        case HighGuid::Unit:
        case HighGuid::Pet:
        case HighGuid::Vehicle:
            return typemask & TYPEMASK_UNIT;
        case HighGuid::DynamicObject:
            return typemask & TYPEMASK_DYNAMICOBJECT;
        case HighGuid::Corpse:
            return typemask & TYPEMASK_CORPSE;
        case HighGuid::Instance:
        case HighGuid::Group:
            return false;
        default: // invalid or unhandled high guid
            ASSERT(false, "CheckIfHighGuidFitsTypeMask: Invalid HighGuid.");
    }
}

Object* ObjectAccessor::GetObjectByTypeMask(WorldObject const& obj, ObjectGuid const& guid, uint32 const typemask)
{
    if (!CheckIfHighGuidFitsTypeMask(guid.GetHigh(), typemask))
        return nullptr;

    if (guid.GetHigh() == HighGuid::Item)
    {
        if (Player const* player = obj.ToPlayer())
            return player->GetItemByGuid(guid);

        return nullptr;
    }

    return GetWorldObject(obj, guid);
}

Player* ObjectAccessor::GetPlayer(Map& map, ObjectGuid const& guid)
{
    return map.GetPlayer(guid);
}

Player* ObjectAccessor::GetPlayer(WorldObject const& obj, ObjectGuid const& guid)
{
    return GetPlayer(*obj.GetMap(), guid);
}

Corpse* ObjectAccessor::GetCorpse(Map& map, ObjectGuid const& guid)
{
    return map.GetCorpse(guid);
}

Corpse* ObjectAccessor::GetCorpse(WorldObject const& obj, ObjectGuid const& guid)
{
    return GetCorpse(*obj.GetMap(), guid);
}

DynamicObject* ObjectAccessor::GetDynamicObject(Map& map, ObjectGuid const& guid)
{
    return map.GetDynamicObject(guid);
}

DynamicObject* ObjectAccessor::GetDynamicObject(WorldObject const& obj, ObjectGuid const& guid)
{
    return GetDynamicObject(*obj.GetMap(), guid);
}

GameObject* ObjectAccessor::GetGameObject(Map& map, ObjectGuid const& guid)
{
    return map.GetGameObject(guid);
}

GameObject* ObjectAccessor::GetGameObject(uint32 const mapId, ObjectGuid const& guid)
{
    if (auto const map = sMapMgr->FindMap(mapId))
        return GetGameObject(*map, guid);

    return nullptr;
}

GameObject* ObjectAccessor::GetGameObject(WorldObject const& obj, ObjectGuid const& guid)
{
    return GetGameObject(*obj.GetMap(), guid);
}

MotionTransport* ObjectAccessor::GetMotionTransport(ObjectGuid const& guid)
{
    return HashMapHolder<MotionTransport>::Find(guid);
}

MotionTransport* ObjectAccessor::GetMotionTransport(Map& map, ObjectGuid const& guid)
{
    if (MotionTransport* trans = GetMotionTransport(guid))
        if (trans->FindMap() == &map)
            return trans;

    return nullptr;
}

MotionTransport* ObjectAccessor::GetMotionTransport(WorldObject const& obj, ObjectGuid const& guid)
{
    return GetMotionTransport(*obj.GetMap(), guid);
}

Unit* ObjectAccessor::GetUnit(Map& map, ObjectGuid const& guid)
{
    if (guid.IsPlayer())
        return GetPlayer(map, guid);
    else
        return GetCreature(map, guid);
}

Unit* ObjectAccessor::GetUnit(uint32 const mapId, ObjectGuid const& guid)
{
    if (auto const map = sMapMgr->FindMap(mapId))
        return GetUnit(*map, guid);

    return nullptr;
}

Unit* ObjectAccessor::GetUnit(WorldObject const& obj, ObjectGuid const& guid)
{
    return GetUnit(*obj.GetMap(), guid);
}

Creature* ObjectAccessor::GetCreature(Map& map, ObjectGuid const& guid)
{
    return map.GetCreature(guid);
}

Creature* ObjectAccessor::GetCreature(uint32 const mapId, ObjectGuid const& guid)
{
    if (auto const map = sMapMgr->FindMap(mapId))
        return GetCreature(*map, guid);

    return nullptr;
}

Creature* ObjectAccessor::GetCreature(WorldObject const& obj, ObjectGuid const& guid)
{
    return GetCreature(*obj.GetMap(), guid);
}

Pet* ObjectAccessor::GetPet(Map& map, ObjectGuid const& guid)
{
    if (Creature* c = GetCreature(map, guid))
        return c->ToPet();

    return nullptr;
}

Pet* ObjectAccessor::GetPet(WorldObject const& obj, ObjectGuid const& guid)
{
    return GetPet(*obj.GetMap(), guid);
}

void ObjectAccessor::AddObject(Player* object)
{
    HashMapHolder<Player>::Insert(object);
}

void ObjectAccessor::AddObject(MotionTransport* object)
{
    HashMapHolder<MotionTransport>::Insert(object);
}

void ObjectAccessor::RemoveObject(Player* object)
{
    HashMapHolder<Player>::Remove(object);
}

void ObjectAccessor::RemoveObject(MotionTransport* object)
{
    HashMapHolder<MotionTransport>::Remove(object);
}

void ObjectAccessor::SaveAllPlayers()
{
    boost::shared_lock<boost::shared_mutex> lock(*HashMapHolder<Player>::GetLock());

    for (auto& playerEntry : GetPlayers())
        playerEntry.second->SaveToDB();
}

Corpse* ObjectAccessor::GetCorpseForPlayerGUID(ObjectGuid guid)
{
    boost::shared_lock<boost::shared_mutex> lock(_corpseLock);

    Player2CorpsesMapType::iterator iter = i_player2corpse.find(guid);
    if (iter == i_player2corpse.end())
        return NULL;

    ASSERT(iter->second->GetType() != CORPSE_BONES);

    return iter->second;
}

void ObjectAccessor::AddCorpse(Corpse* corpse)
{
    ASSERT(corpse && corpse->GetType() != CORPSE_BONES);

    // Critical section
    {
        boost::unique_lock<boost::shared_mutex> lock(_corpseLock);

        ASSERT(i_player2corpse.find(corpse->GetOwnerGUID()) == i_player2corpse.end());
        i_player2corpse[corpse->GetOwnerGUID()] = corpse;

        // build mapid*cellid -> guid_set map
        Grids::CellCoord cellCoord = Grids::CellCoord::FromPosition(corpse->GetPositionX(), corpse->GetPositionY());
        sObjectMgr->AddCorpseCellData(corpse->GetMapId(), cellCoord.GetId(), corpse->GetOwnerGUID().GetCounter(), corpse->GetInstanceId());
    }
}

void ObjectAccessor::ConvertCorpseForPlayer(ObjectGuid player_guid, bool insignia /*=false*/, std::function<void(Corpse*)> callback)
{
    boost::upgrade_lock<boost::shared_mutex> lock(_corpseLock);

    Player2CorpsesMapType::iterator iter = i_player2corpse.find(player_guid);

    if (iter == i_player2corpse.end())
    {
        //in fact this function is called from several places
        //even when player doesn't have a corpse, not an error
        return;
    }

    ASSERT(iter->second->GetType() != CORPSE_BONES);

    Corpse* corpse = iter->second;

    TC_LOG_DEBUG("misc", "Deleting Corpse and spawned bones.");

    const Grids::CellCoord cellCoord = Grids::CellCoord::FromPosition(corpse->GetPositionX(), corpse->GetPositionY());

    // Critical section
    {
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

        // build mapid*cellid -> guid_set map
        sObjectMgr->DeleteCorpseCellData(corpse->GetMapId(), cellCoord.GetId(), corpse->GetOwnerGUID().GetCounter());

        i_player2corpse.erase(iter);
    }

    // remove corpse from DB
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    corpse->DeleteFromDB(trans);
    CharacterDatabase.CommitTransaction(trans);

    // Map can be NULL
    Map* map = corpse->FindMap();

    if (map)
    {
        map->Schedule([corpse, insignia](Map& map) -> Corpse*
        {
            if (corpse->IsInGrid())
                map.RemoveFromMap(corpse, false);
            else
            {
                corpse->RemoveFromWorld();
                corpse->ResetMap();
            }

            Corpse* bones = nullptr;
            // create the bones only if the map and the grid is loaded at the corpse's location
            // ignore bones creating option in case insignia

            if ((insignia ||
                (map.IsBattlegroundOrArena() ? sWorld->getBoolConfig(CONFIG_DEATH_BONES_BG_OR_ARENA) : sWorld->getBoolConfig(CONFIG_DEATH_BONES_WORLD))) &&
                map.IsGridLoaded(corpse->GetPositionX(), corpse->GetPositionY()))
            {
                // Create bones, don't change Corpse
                bones = Corpse::CreateBones(corpse->GetGUID().GetCounter(), map);

                for (uint8 i = OBJECT_FIELD_TYPE + 1; i < CORPSE_END; ++i)                    // don't overwrite guid and object type
                    bones->SetUInt32Value(i, corpse->GetUInt32Value(i));

                // bones->m_time = m_time;                              // don't overwrite time
                // bones->m_type = m_type;                              // don't overwrite type
                bones->Relocate(corpse->GetPositionX(), corpse->GetPositionY(), corpse->GetPositionZ(), corpse->GetOrientation());
                bones->SetPhaseMask(corpse->GetPhaseMask(), false);

                bones->SetUInt32Value(CORPSE_FIELD_FLAGS, CORPSE_FLAG_UNK2 | CORPSE_FLAG_BONES);
                bones->SetGuidValue(CORPSE_FIELD_OWNER, ObjectGuid::Empty);

                for (uint8 i = 0; i < EQUIPMENT_SLOT_END; ++i)
                {
                    if (corpse->GetUInt32Value(CORPSE_FIELD_ITEM + i))
                        bones->SetUInt32Value(CORPSE_FIELD_ITEM + i, 0);
                }

                // add bones in grid store if grid loaded where corpse placed
                map.AddToMap(bones);
            }

            // all references to the corpse should be removed at this point
            delete corpse;

            return bones;
        }).then(callback);
    }
    else
    {
        corpse->RemoveFromWorld();

        // all references to the corpse should be removed at this point
        delete corpse;
    }
}

void ObjectAccessor::RemoveOldCorpses()
{
    time_t now = time(NULL);
    Player2CorpsesMapType::iterator next;
    for (Player2CorpsesMapType::iterator itr = i_player2corpse.begin(); itr != i_player2corpse.end(); itr = next)
    {
        next = itr;
        ++next;

        if (!itr->second->IsExpired(now))
            continue;

        ConvertCorpseForPlayer(itr->first);
    }
}

void ObjectAccessor::UnloadAll()
{
    for (Player2CorpsesMapType::const_iterator itr = i_player2corpse.begin(); itr != i_player2corpse.end(); ++itr)
    {
        itr->second->RemoveFromWorld();
        delete itr->second;
    }
}

/// Define the static members of HashMapHolder

template <class T> GAME_API typename HashMapHolder<T>::MapType HashMapHolder<T>::_objectMap;
template <class T> GAME_API boost::shared_mutex HashMapHolder<T>::_lock;

/// Global definitions for the hashmap storage

template class HashMapHolder<Player>;
template class HashMapHolder<MotionTransport>;
