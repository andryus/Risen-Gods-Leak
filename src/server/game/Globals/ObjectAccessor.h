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

#ifndef TRINITY_OBJECTACCESSOR_H
#define TRINITY_OBJECTACCESSOR_H

#include <mutex>
#include <unordered_map>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "Define.h"
#include "UpdateData.h"
#include "Object.h"


class Creature;
class Corpse;
class Unit;
class GameObject;
class DynamicObject;
class WorldObject;
class Vehicle;
class Map;
class Pet;
class WorldRunnable;
class Transport;
class MotionTransport;

template <class T>
class HashMapHolder
{
    static_assert(std::is_same<Player, T>::value || std::is_same<MotionTransport, T>::value,
        "Only Player and MotionTransport can be registered in global HashMapHolder");

    public:
        typedef std::unordered_map<ObjectGuid, T*> MapType;

        static void Insert(T* o)
        {
            boost::unique_lock<boost::shared_mutex> lock(_lock);

            _objectMap[o->GetGUID()] = o;
        }

        static void Remove(T* o)
        {
            boost::unique_lock<boost::shared_mutex> lock(_lock);

            _objectMap.erase(o->GetGUID());
        }

        static T* Find(ObjectGuid guid)
        {
            boost::shared_lock<boost::shared_mutex> lock(_lock);

            typename MapType::iterator itr = _objectMap.find(guid);
            return (itr != _objectMap.end()) ? itr->second : NULL;
        }

        static MapType& GetContainer() { return _objectMap; }

        static boost::shared_mutex* GetLock() { return &_lock; }

    private:
        //Non instanceable only static
        HashMapHolder() { }

        static GAME_API boost::shared_mutex _lock;
        static GAME_API MapType _objectMap;
};

class GAME_API ObjectAccessor
{
    public:
        // these methods return Player even if he is not in world, for example teleporting
        static Player* FindConnectedPlayer(ObjectGuid const& guid);
        static Player* FindConnectedPlayerByName(std::string const& name);

        // these methods return Player only if in world
        static Player* FindPlayer(ObjectGuid const& guid);
        static Player* FindPlayerByName(std::string const& name);

        // Note: when using this, you must use the hashmapholder's lock
        static HashMapHolder<Player>::MapType const& GetPlayers() { return HashMapHolder<Player>::GetContainer(); }


        static WorldObject* GetWorldObject(Map& map, ObjectGuid const& guid);
        static WorldObject* GetWorldObject(WorldObject const& obj, ObjectGuid const& guid);

        static Object* GetObjectByTypeMask(WorldObject const& obj, ObjectGuid const& guid, uint32 const typemask);

        static Player* GetPlayer(Map& map, ObjectGuid const& guid);
        static Player* GetPlayer(WorldObject const& obj, ObjectGuid const& guid);

        static Corpse* GetCorpse(Map& map, ObjectGuid const& guid);
        static Corpse* GetCorpse(WorldObject const& obj, ObjectGuid const& guid);

        static DynamicObject* GetDynamicObject(Map& map, ObjectGuid const& guid);
        static DynamicObject* GetDynamicObject(WorldObject const& obj, ObjectGuid const& guid);

        // Note: Returns all types of GameObjects except MotionTransports! Use GetMotionTransport() instead.
        static GameObject* GetGameObject(Map& map, ObjectGuid const& guid);
        static GameObject* GetGameObject(uint32 const mapId, ObjectGuid const& guid);
        static GameObject* GetGameObject(WorldObject const& obju, ObjectGuid const& guid);

        static MotionTransport* GetMotionTransport(ObjectGuid const& guid);
        static MotionTransport* GetMotionTransport(Map& map, ObjectGuid const& guid);
        static MotionTransport* GetMotionTransport(WorldObject const& obj, ObjectGuid const& guid);

        static Unit* GetUnit(Map& map, ObjectGuid const& guid);
        static Unit* GetUnit(uint32 const mapId, ObjectGuid const& guid);
        static Unit* GetUnit(WorldObject const& obj, ObjectGuid const& guid);

        static Creature* GetCreature(Map& map, ObjectGuid const& guid);
        static Creature* GetCreature(uint32 const mapId, ObjectGuid const& guid);
        static Creature* GetCreature(WorldObject const& obj, ObjectGuid const& guid);

        static Pet* GetPet(Map& map, ObjectGuid const& guid);
        static Pet* GetPet(WorldObject const& obj, ObjectGuid const& guid);


        static void AddObject(Player* object);
        static void AddObject(MotionTransport* object);
        static void RemoveObject(Player* object);
        static void RemoveObject(MotionTransport* object);

        static void SaveAllPlayers();

        //Thread safe
        Corpse* GetCorpseForPlayerGUID(ObjectGuid guid);
        void AddCorpse(Corpse* corpse);
        void ConvertCorpseForPlayer(ObjectGuid player_guid, bool insignia = false, std::function<void(Corpse*)> callback = [](Corpse*){});

        //Thread unsafe
        void RemoveOldCorpses();
        void UnloadAll();

    private:
        using Player2CorpsesMapType = std::unordered_map<ObjectGuid, Corpse*>;

        Player2CorpsesMapType i_player2corpse;

        boost::shared_mutex _corpseLock;
    // singleton implementation
    public:
        static ObjectAccessor* instance()
        {
            static ObjectAccessor instance;
            return &instance;
        }

        ObjectAccessor(const ObjectAccessor&) = delete;
        ObjectAccessor& operator=(const ObjectAccessor&) = delete;

    private:
        ObjectAccessor();
        ~ObjectAccessor();
};

#define sObjectAccessor ObjectAccessor::instance()
#endif
