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

#ifndef TRINITY_MAPMANAGER_H
#define TRINITY_MAPMANAGER_H

#include "Map.h"
#include "MapUpdater.h"
#include "MapInstanced.h"

#include <functional>

class Transport;
struct TransportCreatureProto;
class WorldLocation;

class GAME_API MapManager
{
    public:
        Map* CreateBaseMap(uint32 mapId);
        Map* FindBaseNonInstanceMap(uint32 mapId) const;
        Map* CreateMap(uint32 mapId, Player* player);
        Map* FindMap(const uint32 mapId, const uint32 instanceId = 0) const;

        uint32 GetAreaId(uint32 mapid, float x, float y, float z) const
        {
            Map const* m = const_cast<MapManager*>(this)->CreateBaseMap(mapid);
            return m->GetAreaId(x, y, z);
        }
        uint32 GetZoneId(uint32 mapid, float x, float y, float z) const
        {
            Map const* m = const_cast<MapManager*>(this)->CreateBaseMap(mapid);
            return m->GetZoneId(x, y, z);
        }
        void GetZoneAndAreaId(uint32& zoneid, uint32& areaid, uint32 mapid, float x, float y, float z)
        {
            Map const* m = const_cast<MapManager*>(this)->CreateBaseMap(mapid);
            m->GetZoneAndAreaId(zoneid, areaid, x, y, z);
        }

        void Initialize(void);
        void Update(uint32);

        void SetGridCleanUpDelay(uint32 t)
        {
            if (t < MIN_GRID_DELAY)
                i_gridCleanUpDelay = MIN_GRID_DELAY;
            else
                i_gridCleanUpDelay = t;
        }

        //void LoadGrid(int mapid, int instId, float x, float y, const WorldObject* obj, bool no_unload = false);
        void UnloadAll();

        static bool ExistMapAndVMap(uint32 mapid, float x, float y);
        static bool IsValidMAP(uint32 mapid, bool startUp);

        static bool IsValidMapCoord(uint32 mapid, float x, float y)
        {
            return IsValidMAP(mapid, false) && Trinity::IsValidMapCoord(x, y);
        }

        static bool IsValidMapCoord(uint32 mapid, float x, float y, float z)
        {
            return IsValidMAP(mapid, false) && Trinity::IsValidMapCoord(x, y, z);
        }

        static bool IsValidMapCoord(uint32 mapid, float x, float y, float z, float o)
        {
            return IsValidMAP(mapid, false) && Trinity::IsValidMapCoord(x, y, z, o);
        }

        static bool IsValidMapCoord(const WorldLocation& loc);

        void DoDelayedMovesAndRemoves();

        bool CanPlayerEnter(uint32 mapid, Player* player, bool loginCheck = false);
        void InitializeVisibilityDistanceInfo();

        /* statistics */
        uint32 GetNumInstances();
        uint32 GetNumPlayersInInstances();

        // Instance ID management
        void InitInstanceIds();
        uint32 GenerateInstanceId();
        void RegisterInstanceId(uint32 instanceId);
        void FreeInstanceId(uint32 instanceId);

        uint32 GetNextInstanceId() const { return _nextInstanceId; };
        void SetNextInstanceId(uint32 nextInstanceId) { _nextInstanceId = nextInstanceId; };

        MapUpdater * GetMapUpdater() { return &m_updater; }

        bool IsReloadMapConfigs() const { return _reload_map_configs; }
        void ReloadMapConfigs() { _reload_map_configs = true; }

        /**
         * Execute the given function for each map.
         */
        template<typename F>
        void ExecuteForEach(F&& func);
        
        /**
         * Execute the given function for each map with the given id.
         * 
         * @note    The function will only be called multiple times if the map
         *  specified by {@code mapId} is instanceable.
         */
        template<typename F>
        void ExecuteForEach(uint32 mapId, F&& func);

        void LoadAllCells(Map* map);
        void LoadAllCells();
        void LoadSpecificCells();

        bool IsUnloadingEnabled(Map* map, Grids::CellCoord const& p);
        bool IsUnloadingEnabled(Map* map);

    private:
        Map* FindBaseMap(uint32 mapId) const
        {
            const auto it = i_maps.find(mapId);
            return (it == i_maps.end() ? nullptr : it->second);
        }

        std::mutex _mapsLock;
        uint32 i_gridCleanUpDelay;
        std::unordered_map<uint32, Map*>  i_maps;

        std::vector<bool> _instanceIds;
        uint32 _nextInstanceId;
        MapUpdater m_updater;
        bool _reload_map_configs;

    //singleton implementation
    public:
        static MapManager* instance();

        MapManager(const MapManager&) = delete;
        MapManager& operator=(const MapManager&) = delete;

    private:
        MapManager();
        ~MapManager();
};

template <typename F>
void MapManager::ExecuteForEach(F&& func)
{
    std::lock_guard<std::mutex> lock{ _mapsLock };

    for (auto& mapEntry : i_maps)
    {
        Map& map = *mapEntry.second;

        if (MapInstanced* mapInstanced = map.ToMapInstanced())
        {
            for (auto& instanceEntry : mapInstanced->GetInstancedMaps())
                instanceEntry.second->Schedule(func);
        }
        else
            map.Schedule(func);
    }
}

template <typename F>
void MapManager::ExecuteForEach(uint32 const mapId, F&& func)
{
    std::lock_guard<std::mutex> lock{ _mapsLock };

    auto const itr = i_maps.find(mapId);
    if (itr != i_maps.end())
    {
        Map& map = *itr->second;

        if (MapInstanced* mapInstanced = map.ToMapInstanced())
        {
            for (auto& instanceEntry : mapInstanced->GetInstancedMaps())
                instanceEntry.second->Schedule(func);
        }
        else
            map.Schedule(func);
    }
}

#define sMapMgr MapManager::instance()

#endif
