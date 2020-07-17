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

#include "Map.h"
#include "Battleground.h"
#include "MMapFactory.h"
#include "DisableMgr.h"
#include "DynamicTree.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "MapInstanced.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Pet.h"
#include "ScriptMgr.h"
#include "Transport.h"
#include "Vehicle.h"
#include "VMapFactory.h"
#include "DisableMgr.h"
#include "MapManager.h"
#include "Totem.h"
#include "UpdateData.h"
#include "MapArea.h"
#include "DBCStores.h"
#include "ArenaReplay/ArenaReplaySystem.h"
#include "Monitoring/Maps.hpp"

#include "API/LoadScript.h"
#include "UnitHooks.h"
#include "MapHooks.h"

#define DEFAULT_GRID_EXPIRY     300
#define MAX_GRID_LOAD_TIME      50
#define MAX_CREATURE_ATTACK_RADIUS  (45.0f * sWorld->getRate(RATE_CREATURE_AGGRO))

template class GAME_API ExecutionContext<Map>;


namespace impl
{
    bool MapConvert<InstanceMap>::operator()(const Map& map) const
    {
        return map.IsDungeon();
    }

    bool MapConvert<MapInstanced>::operator()(const Map& map) const
    {
        return map.Instanceable();
    }

    bool MapConvert<BattlegroundMap>::operator()(const Map& map) const
    {
        return map.IsBattlegroundOrArena();
    }

    bool MapConvert<ArenaReplayMap>::operator()(const Map& map) const
    {
        return map.IsReplayMap();
    }
}

Map::~Map()
{
    sScriptMgr->OnDestroyMap(this);

    UnloadAll();
    
    if (!m_scriptSchedule.empty())
        sScriptMgr->DecreaseScheduledScriptCount(m_scriptSchedule.size());
}

inline TerrainMap* Map::GetGrid(float x, float y)
{
    Grids::CellCoord coord = Grids::CellCoord::FromPosition(x, y);
    // ensure GridMap is loaded
    EnsureTerrainLoaded(coord);

    coord.Inverse();
    return terrainMap[coord.GetGridX()][coord.GetGridY()];
}

bool Map::ExistVMap(uint32 mapid, int gx, int gy)
{
    if (VMAP::IVMapManager* vmgr = VMAP::VMapFactory::createOrGetVMapManager())
    {
        if (vmgr->isMapLoadingEnabled())
        {
            VMAP::LoadResult result = vmgr->existsMap((sWorld->GetDataPath() + "vmaps").c_str(), mapid, gx, gy);
            std::string name = vmgr->getDirFileName(mapid, gx, gy);
            switch (result)
            {
                case VMAP::LoadResult::Success:
                    break;
                case VMAP::LoadResult::FileNotFound:
                    TC_LOG_ERROR("maps", "VMap file '%s' does not exist", (sWorld->GetDataPath() + "vmaps/" + name).c_str());
                    TC_LOG_ERROR("maps", "Please place VMAP files (*.vmtree and *.vmtile) in the vmap directory (%s), or correct the DataDir setting in your worldserver.conf file.", (sWorld->GetDataPath() + "vmaps/").c_str());
                    return false;
                case VMAP::LoadResult::VersionMismatch:
                    TC_LOG_ERROR("maps", "VMap file '%s' couldn't be loaded", (sWorld->GetDataPath() + "vmaps/" + name).c_str());
                    TC_LOG_ERROR("maps", "This is because the version of the VMap file and the version of this module are different, please re-extract the maps with the tools compiled with this module.");
                    return false;
            }
        }
    }

    return true;
}

void Map::LoadMMap(int gx, int gy)
{
    if (!DisableMgr::IsPathfindingEnabled(GetId()))
        return;

    bool mmapLoadResult = MMAP::MMapFactory::createOrGetMMapManager()->loadMap((sWorld->GetDataPath() + "mmaps").c_str(), GetId(), gx, gy);

    if (mmapLoadResult)
        TC_LOG_DEBUG("mmaps", "MMAP loaded name:%s, id:%d, x:%d, y:%d (mmap rep.: x:%d, y:%d)", GetMapName(), GetId(), gx, gy, gx, gy);
    else
        TC_LOG_ERROR("mmaps", "Could not load MMAP name:%s, id:%d, x:%d, y:%d (mmap rep.: x:%d, y:%d)", GetMapName(), GetId(), gx, gy, gx, gy);
}

void Map::LoadVMap(int gx, int gy)
{
    if (!VMAP::VMapFactory::createOrGetVMapManager()->isMapLoadingEnabled())
        return;
                                                            // x and y are swapped !!
    int vmapLoadResult = VMAP::VMapFactory::createOrGetVMapManager()->loadMap((sWorld->GetDataPath()+ "vmaps").c_str(),  GetId(), gx, gy);
    switch (vmapLoadResult)
    {
        case VMAP::VMAP_LOAD_RESULT_OK:
            TC_LOG_DEBUG("maps", "VMAP loaded name:%s, id:%d, x:%d, y:%d (vmap rep.: x:%d, y:%d)", GetMapName(), GetId(), gx, gy, gx, gy);
            break;
        case VMAP::VMAP_LOAD_RESULT_ERROR:
            TC_LOG_ERROR("maps", "Could not load VMAP name:%s, id:%d, x:%d, y:%d (vmap rep.: x:%d, y:%d)", GetMapName(), GetId(), gx, gy, gx, gy);
            break;
        case VMAP::VMAP_LOAD_RESULT_IGNORED:
            TC_LOG_DEBUG("maps", "Ignored VMAP name:%s, id:%d, x:%d, y:%d (vmap rep.: x:%d, y:%d)", GetMapName(), GetId(), gx, gy, gx, gy);
            break;
    }
}

void Map::LoadMap(int gx, int gy, bool reload)
{
    if (i_InstanceId != 0)
    {
        if (terrainMap[gx][gy])
            return;

        // load grid map for base map
        if (!parentMap.terrainMap[gx][gy])
            parentMap.EnsureTerrainLoaded(Grids::CellCoord::FromGridCoord(gx, gy));

        terrainMap[gx][gy] = parentMap.terrainMap[gx][gy];
        return;
    }

    if (terrainMap[gx][gy] && !reload)
        return;

    //map already load, delete it before reloading (Is it necessary? Do we really need the ability the reload maps during runtime?)
    if (terrainMap[gx][gy])
    {
        TC_LOG_DEBUG("maps", "Unloading previously loaded map %u before reloading.", GetId());
        sScriptMgr->OnUnloadGridMap(this, terrainMap[gx][gy], gx, gy);

        delete (terrainMap[gx][gy]);
        terrainMap[gx][gy]=NULL;
    }

    // map file name
    char* tmp = NULL;
    int len = sWorld->GetDataPath().length() + strlen("maps/%03u%02u%02u.map") + 1;
    tmp = new char[len];
    snprintf(tmp, len, (char *)(sWorld->GetDataPath() + "maps/%03u%02u%02u.map").c_str(), GetId(), gx, gy);
    TC_LOG_DEBUG("maps", "Loading map %s", tmp);
    // loading data
    terrainMap[gx][gy] = new TerrainMap();
    if (!terrainMap[gx][gy]->loadData(tmp))
        TC_LOG_ERROR("maps", "Error loading map file: \n %s\n", tmp);
    delete[] tmp;

    sScriptMgr->OnLoadGridMap(this, terrainMap[gx][gy], gx, gy);
}

void Map::WorldObjectAdded(WorldObject& object)
{
    const uint32 areaId = GetAreaId(object.GetPositionX(), object.GetPositionY(), object.GetPositionZ());

    MapArea* area;
    auto itr = areas.find(areaId);
    if (itr != areas.end())
      area = itr->second.get();
    else
    {
        const std::string_view areaScript = sObjectMgr->GetAreaScript(areaId);
        auto newArea = std::make_shared<MapArea>(*this, areaId, areaScript);

        area = areas.emplace(areaId, std::move(newArea)).first->second.get();

        area->Init();
    }

    area->OnWorldObjectAdded(object);

    OnWorldObjectAdded(object);
}

void Map::OnWorldObjectAdded(WorldObject& object)
{
}

void Map::OnWorldObjectRemoved(WorldObject& object)
{
    for (auto itr = initObjects.begin(); itr != initObjects.end(); )
    {
        if (*itr == &object)
        {
            initObjects.erase(itr);
            return;
        }
        else
            ++itr;
    }
}

void Map::LoadMapAndVMap(int gx, int gy)
{
    LoadMap(gx, gy);
   // Only load the data for the base map
    if (i_InstanceId == 0)
    {
        LoadVMap(gx, gy);
        LoadMMap(gx, gy);
    }
}

#define DEFAULT_VISIBILITY_NOTIFY_PERIOD 1000

Map::Map(uint32 id, time_t expiry, uint32 InstanceId, uint8 SpawnMode, Map* _parent):
_creatureToMoveLock(false), _gameObjectsToMoveLock(false), _dynamicObjectsToMoveLock(false),
i_mapEntry(sMapStore.LookupEntry(id)), i_spawnMode(SpawnMode), i_InstanceId(InstanceId),
m_unloadTimer(0), m_VisibleDistance(DEFAULT_VISIBILITY_DISTANCE), m_VisibilityNotifyPeriod(DEFAULT_VISIBILITY_NOTIFY_PERIOD),
m_activeNonPlayersIter(m_activeNonPlayers.end()), _transportsUpdateIter(_transports.end()), dynamicVisibilityDistance(1.f),
i_scriptLock(false), _defaultLight(GetDefaultMapLight(id)), m_isReplayMap(false), relocationContainerIndex(0), relocationTimer(0), 
_force_unload(false), _vmap_disable_flags(0), _mmap_disabled(false), gridMap(uint16(id), SpawnMode, *this), parentMap(_parent ? *_parent : *this),
terrainMap(), diffTimer(UpdateTimer::MAP_DECAY)
{
    assert(terrainMap[24][36] == nullptr);

    relocatedUnitItr = relocatedUnits.begin();

    //lets initialize visibility distance for map
    Map::InitVisibilityDistance();

    sScriptMgr->OnCreateMap(this);
    ReloadMapConfig();
}

void Map::ReloadMapConfig()
{
    auto info = DisableMgr::GetDisableFlags(DisableType::DISABLE_TYPE_VMAP, GetId());
    if (info.first)
        _vmap_disable_flags = info.second;
    else
        _vmap_disable_flags = 0;

    _mmap_disabled = DisableMgr::IsDisabledFor(DisableType::DISABLE_TYPE_MMAP, GetId(), NULL, MMAP_DISABLE_PATHFINDING);
}

void Map::InitVisibilityDistance()
{
    //init visibility for continents
    m_VisibleDistance = World::GetMaxVisibleDistanceOnContinents();
    m_VisibilityNotifyPeriod = World::GetVisibilityNotifyPeriodOnContinents();
}

// Template specialization of utility methods
template<class T>
bool Map::AddToGrid(T* obj, bool loadGrid)
{
    return AddToGrid(obj, Grids::CellCoord(obj->GetPositionX(), obj->GetPositionY()), loadGrid);
}

template<class T>
bool Map::AddToGrid(T* obj, Grids::CellCoord const& pos, bool loadGrid)
{
    return gridMap.Insert(obj, pos, loadGrid);
}

template<class T>
bool Map::SwitchGridContainers(T* obj, bool loadGrid)
{
    return SwitchGridContainers(obj, Grids::CellCoord(obj->GetPositionX(), obj->GetPositionY()), loadGrid);
}

template <class T>
bool Map::SwitchGridContainers(T* obj, Grids::CellCoord const& pos, bool loadGrid)
{
    if (!loadGrid && !IsGridLoaded(pos))
        return false;
    obj->RemoveFromGrid();
    return AddToGrid(obj, pos, loadGrid);
}

template<class T>
void Map::DeleteFromWorld(T* obj)
{
    // Note: In case resurrectable corpse and pet its removed from global lists in own destructor
    delete obj;
}

template<>
void Map::DeleteFromWorld(Player* player)
{
    ASSERT(false);
}

template<>
void Map::DeleteFromWorld(MotionTransport* transport)
{
    ObjectAccessor::RemoveObject(transport);
    delete transport;
}

void Map::PreloadGrid(Grids::CellCoord const& coord)
{
    gridMap.PreloadGrid(coord);
}

//Create NGrid so the object can be added to it
//But object data is not loaded here
void Map::EnsureTerrainLoaded(const Grids::CellCoord &p)
{
    int gx = p.GetInversedGridX();
    int gy = p.GetInversedGridY();

    if (!terrainMap[gx][gy])
    {
        std::lock_guard<std::mutex> lock(_gridLock);

        if (!terrainMap[gx][gy])
            LoadMapAndVMap(gx, gy);
    }
}

//Create NGrid and load the object data in it
void Map::EnsureGridLoaded(const Grids::CellCoord& cell)
{
    EnsureTerrainLoaded(cell);
    gridMap.EnsureGridLoaded(cell);
}

bool Map::AddPlayerToMap(std::shared_ptr<Player> playerPtr)
{
    _players[playerPtr->GetGUID()] = playerPtr;
    Player* player = playerPtr.get();

    Grids::CellCoord cellCoord = Grids::CellCoord::FromPosition(player->GetPositionX(), player->GetPositionY());
    if (!cellCoord.IsValid())
    {
        TC_LOG_ERROR("maps", "Map::Add: Player (GUID: %u) has invalid coordinates X:%f Y:%f grid [%u:%u]", player->GetGUID().GetCounter(), player->GetPositionX(), player->GetPositionY(), cellCoord.GetGridX(), cellCoord.GetGridY());
        return false;
    }

    EnsureGridLoaded(cellCoord);
    AddToGrid(player, cellCoord);

    // Check if we are adding to correct map
    ASSERT (player->GetMap() == this);
    player->SetMap(this);
    player->AddToWorld();

    SendInitSelf(player);
    SendZoneDynamicInfo(player);

    player->m_clientGUIDs.clear();
    player->UpdateObjectVisibility(false);

    GetMapVisibilityGroup()->AddPlayer(player);

    sScriptMgr->OnPlayerEnterMap(this, player);
    MONITOR_MAPS(ReportPlayer(this, true));
    return true;
}

template<class T>// todo: remove  those methods once we are sure they aren't needed anymore 
void Map::InitializeObject(T* /*obj*/) { }

template<>
void Map::InitializeObject(Creature* obj)
{
}

template<>
void Map::InitializeObject(GameObject* obj)
{
}

template<class T>
bool Map::AddToMap(T* obj)
{
    /// @todo Needs clean up. An object should not be added to map twice.
    if (obj->IsInWorld())
    {
        ASSERT(obj->IsInGrid());
        obj->UpdateObjectVisibility(true);
        return true;
    }

    Grids::CellCoord cellCoord = Grids::CellCoord::FromPosition(obj->GetPositionX(), obj->GetPositionY());
    //It will create many problems (including crashes) if an object is not added to grid after creation
    //The correct way to fix it is to make AddToMap return false and delete the object if it is not added to grid
    //But now AddToMap is used in too many places, I will just see how many ASSERT failures it will cause
    ASSERT(cellCoord.IsValid());
    if (!cellCoord.IsValid())
    {
        TC_LOG_ERROR("maps", "Map::Add: %s has invalid coordinates X:%f Y:%f grid [%u:%u]", obj->GetGUID().ToString().c_str(), obj->GetPositionX(), obj->GetPositionY(), cellCoord.GetGridX(), cellCoord.GetGridY());
        return false; //Should delete object
    }

    EnsureGridLoaded(cellCoord);
    AddToGrid(obj, cellCoord);
    TC_LOG_DEBUG("maps", "Object %s enters grid[%u, %u]", obj->GetGUID().ToString().c_str(), cellCoord.GetGridX(), cellCoord.GetGridY());

    //Must already be set before AddToMap. Usually during obj->Create.
    //obj->SetMap(this);
    if (obj->isActiveObject())
        AddToActive(obj);

    obj->AddToWorld();

    InitializeObject(obj);

    initObjects.push_back(obj);

    //something, such as vehicle, needs to be update immediately
    //also, trigger needs to cast spell, if not update, cannot see visual
    obj->UpdateObjectVisibilityOnCreate();
    return true;
}

template<>
bool Map::AddToMap(Transport* obj)
{
    //TODO: Needs clean up. An object should not be added to map twice.
    if (obj->IsInWorld())
        return true;

    Grids::CellCoord cellCoord = Grids::CellCoord::FromPosition(obj->GetPositionX(), obj->GetPositionY());
    if (!cellCoord.IsValid())
    {
        TC_LOG_ERROR("maps", "Map::Add: %s has invalid coordinates X:%f Y:%f grid [%u:%u]", obj->GetGUID().ToString().c_str(), obj->GetPositionX(), obj->GetPositionY(), cellCoord.GetGridX(), cellCoord.GetGridY());
        return false; //Should delete object
    }

    obj->AddToWorld();
    _transports.insert(obj);

    // Broadcast creation to players
    for (Player& player : GetPlayers())
    {
        if (player.GetTransport() != obj)
        {
            WorldPacket packet = obj->CreateUpdateBlockForPlayer(&player);

            player.SendDirectMessage(std::move(packet));
        }
    }

    return true;
}

bool Map::IsGridLoaded(const Grids::CellCoord &p) const
{
    return gridMap.IsGridLoaded(p);
}

void Map::Update(const uint32 t_diff)
{
    if (sMapMgr->IsReloadMapConfigs())
        ReloadMapConfig();

    if (replay)
        replay->Update(t_diff);

    bool lagReportActive = sWorld->getBoolConfig(CONFIG_LAG_REPORT_ENABLED) && sLog->ShouldLog("rg.lagreport", LOG_LEVEL_WARN);

    uint32 diff_size[MAP_DIFF_VALUE_COUNT] = {0};
    uint32 diffTime = 0;
    uint32 startTime = getMSTime();
    if (lagReportActive)
        diffTime = startTime;

    if (!initObjects.empty())
    {
        auto _initObjects = initObjects;
        initObjects.clear();
        for (WorldObject* object : _initObjects)
        {
            WorldObjectAdded(*object);
        }

        for (WorldObject* object : _initObjects)
        {
            *object <<= Fire::InitWorld;
        }
    }

    _dynamicTree.update(t_diff);
    /// update worldsessions for existing players
    auto players_for_update = _players;
    for (auto& pair : players_for_update)
    {
        if (!pair.second)
        {
            TC_LOG_FATAL("server.worldserver", "Found invalid player reference during map update. map: %d, player: %s", GetId(), pair.first.ToString().c_str());
            continue;
        }

        Player& player = *(pair.second);
        if (player.IsInWorld())
        {
            //player->Update(t_diff);
            WorldSession* session = player.GetSession();
            MapSessionFilter updater(session);
            session->HandlePackets(updater);
        }
    }

    if (lagReportActive)
    {
        diff_size[MAP_DIFF_SESSION_UPDATE] = GetMSTimeDiffToNow(diffTime);
        diffTime = getMSTime();
    }

    /// update active cells around players and active objects
    gridMap.ResetVisitedCells();

    auto updater = Trinity::createFunctionAction([t_diff](auto* m)
    {
        using T = std::remove_pointer_t<decltype(m)>;

        if constexpr(!Trinity::isPlayer<T> && !Trinity::isCorpse<T>)
        {
            if (m->IsInWorld())
                m->Update(t_diff);
        }
    });

    if (sWorld->getBoolConfig(CONFIG_UPDATE_ALL_CELLS))
    {
        for (auto& pair : players_for_update)
        {
            if (!pair.second)
            {
                TC_LOG_FATAL("server.worldserver", "Found invalid player reference during map update. map: %d, player: %s", GetId(), pair.first.ToString().c_str());
                continue;
            }

            Player& player = *(pair.second);
            if (!player.IsInWorld())
                continue;

            // update players at tick
            player.Update(t_diff);
        }

        gridMap.VisitAllCells(updater);
    }
    else
    {
        const auto visitNearbyCellsOf = [this, &updater](WorldObject& obj)
        {
            // Check for valid position
            if (!obj.IsPositionValid())
                return;

            VisitOnce(obj.GetPositionX(), obj.GetPositionY(), obj.GetGridActivationRange(), updater, true);
        };

        // the player iterator is stored in the map object
        // to make sure calls to Map::Remove don't invalidate it
        for (auto& pair : players_for_update)
        {
            if (!pair.second)
            {
                TC_LOG_FATAL("server.worldserver", "Found invalid player reference during map update. map: %d, player: %s", GetId(), pair.first.ToString().c_str());
                continue;
            }

            Player& player = *(pair.second);
            if (!player.IsInWorld())
                continue;

            // update players at tick
            player.Update(t_diff);

            uint32 cellDiffTime = 0;
            if (lagReportActive)
                cellDiffTime = getMSTime();

            visitNearbyCellsOf(player);

            // Handle updates for creatures in combat with player and are more than 60 yards away
            if (player.IsInCombat())
            {
                std::vector<Creature*> updateList;
                HostileReference* ref = player.getHostileRefManager().getFirst();
                while (ref)
                {
                    if (Unit* unit = ref->GetSource()->GetOwner())
                        if (unit->ToCreature() && unit->GetMapId() == player.GetMapId() && !unit->IsWithinDistInMap(&player, GetVisibilityRange(), false))
                            updateList.push_back(unit->ToCreature());
                    ref = ref->next();
                }
                // Process deferred update list for player
                for (Creature* c : updateList)
                    visitNearbyCellsOf(*c);
            }

            if (lagReportActive &&
                GetMSTimeDiffToNow(cellDiffTime) > sWorld->getIntConfig(CONFIG_LAG_REPORT_MAX_DIFF_PLAYER_CELL))
                TC_LOG_WARN("rg.lagreport", "[Player Cell Lag Report] Map: %u Diff: %u x: %f y: %f z: %f GUID: %u",
                    GetId(),
                    GetMSTimeDiffToNow(cellDiffTime),
                    player.GetPositionX(),
                    player.GetPositionY(),
                    player.GetPositionZ(),
                    player.GetGUID().GetCounter());
        }

        if (lagReportActive)
        {
            diff_size[MAP_DIFF_UPDATE_PLAYER_CELLS] = uint32(getMSTime() - diffTime);
            diffTime = getMSTime();
        }

        // non-player active objects, increasing iterator in the loop in case of object removal
        for (m_activeNonPlayersIter = m_activeNonPlayers.begin(); m_activeNonPlayersIter != m_activeNonPlayers.end();)
        {
            WorldObject* obj = *m_activeNonPlayersIter;
            ++m_activeNonPlayersIter;

            if (!obj || !obj->IsInWorld())
                continue;

            uint32 cellDiffTime = 0;
            if (sWorld->getBoolConfig(CONFIG_LAG_REPORT_ENABLED))
                cellDiffTime = getMSTime();

            visitNearbyCellsOf(*obj);

            if (sWorld->getBoolConfig(CONFIG_LAG_REPORT_ENABLED) &&
                GetMSTimeDiffToNow(cellDiffTime) > sWorld->getIntConfig(CONFIG_LAG_REPORT_MAX_DIFF_OBJECT_CELL))
                TC_LOG_WARN("rg.lagreport", "[Object Cell Lag Report] Map: %u Diff: %u x: %f y: %f z: %f GUID: %u",
                    GetId(),
                    GetMSTimeDiffToNow(cellDiffTime),
                    obj->GetPositionX(),
                    obj->GetPositionY(),
                    obj->GetPositionZ(),
                    obj->GetGUID().GetCounter());
        }
    }

    if (lagReportActive)
        diff_size[MAP_DIFF_UPDATE_OBJECT_CELLS] = GetMSTimeDiffToNow(diffTime);

    for (_transportsUpdateIter = _transports.begin(); _transportsUpdateIter != _transports.end();)
    {
        WorldObject* obj = *_transportsUpdateIter;
        ++_transportsUpdateIter;

        if (!obj->IsInWorld())
            continue;

        obj->Update(t_diff);
    }

    ///- Process necessary scripts
    if (!m_scriptSchedule.empty())
    {
        i_scriptLock = true;
        ScriptsProcess();
        i_scriptLock = false;
    }

    if (lagReportActive)
        diffTime = getMSTime();

    MoveAllCreaturesInMoveList();
    MoveAllGameObjectsInMoveList();

    RemoveAllObjectsInRemoveList();

    gridMap.Update(t_diff);

    *this <<= Fire::ProcessEvents.Bind(GameTime(t_diff));

    for (auto& [id, area] : areas)
        area->Update(t_diff);

    if (!GetPlayers().empty() || !m_activeNonPlayers.empty())
        ProcessRelocationNotifies(t_diff);

    if (lagReportActive)
        diff_size[MAP_DIFF_RELOCATION_NOTIFIER] = GetMSTimeDiffToNow(diffTime);
    
    ExecutionContext::Run();

    auto diffSum = GetMSTimeDiffToNow(startTime);

    if (!Instanceable())
    {
        if (diffSum > sWorld->getIntConfig(CONFIG_DYNAMIC_VISIBILITY_MAX_THRESHOLD))
            dynamicVisibilityDistance = std::max(dynamicVisibilityDistance / sWorld->getFloatConfig(CONFIG_DYNAMIC_VISIBILITY_RATE),
                sWorld->getFloatConfig(CONFIG_DYNAMIC_VISIBILITY_MIN));
        else if (diffSum < sWorld->getIntConfig(CONFIG_DYNAMIC_VISIBILITY_MIN_THRESHOLD))
            dynamicVisibilityDistance = std::min(dynamicVisibilityDistance * sWorld->getFloatConfig(CONFIG_DYNAMIC_VISIBILITY_RATE),
                sWorld->getFloatConfig(CONFIG_DYNAMIC_VISIBILITY_MAX));
    }
    MONITOR_MAPS(ReportVisibilityDistance(this));

    while (!updateObjects.empty())
    {
        Object* obj = *updateObjects.begin();
        ASSERT(obj && obj->IsInWorld());
        updateObjects.erase(updateObjects.begin());
        obj->BroadcastValuesUpdate();
    }

    sScriptMgr->OnMapUpdate(this, t_diff);

    if (lagReportActive &&
        GetMSTimeDiffToNow(startTime) > sWorld->getIntConfig(CONFIG_LAG_REPORT_MAX_DIFF_MAPS))
        sLog->outMessage("rg.lagreport", LOG_LEVEL_WARN,
            "[Map Lag Report] Map: %u SUM: %u Player-Count: %u Object-Count: %u SESSION_UPDATE: %u PLAYER_CELLS: %u OBJECT_CELLS: %u RELOCATION_NOTIFIER: %u",
            GetId(),
            diffSum,
            GetPlayers().size(),
            m_activeNonPlayers.size(),
            diff_size[MAP_DIFF_SESSION_UPDATE],
            diff_size[MAP_DIFF_UPDATE_PLAYER_CELLS],
            diff_size[MAP_DIFF_UPDATE_OBJECT_CELLS],
            diff_size[MAP_DIFF_RELOCATION_NOTIFIER]);
}

template<class Owner, class Object1, class Object2>
struct MultiRadiusCheck
{
public:

    MultiRadiusCheck(Owner& owner, float radius1, float radius2) :
        owner(owner), radius1(radius1), radius2(radius2)
    {
    }

    bool operator()(Object1* object) const
    {
        return object->IsWithinDist(&owner, radius1);
    }

    bool operator()(Object2* object) const
    {
        return object->IsWithinDist(&owner, radius2);
    }

private:

    Owner& owner;
    float radius1;
    float radius2;
};

template<class Object1, class Object2, class Owner>
auto createRelocationCheck(Owner& owner, float radius1, float radius2)
{
    return MultiRadiusCheck<Owner, Object1, Object2>(owner, radius1, radius2);
}

void Map::ProcessRelocationNotifies(const uint32 diff)
{
    bool lagReportActive = sWorld->getBoolConfig(CONFIG_LAG_REPORT_ENABLED) && sLog->ShouldLog("rg.lagreport", LOG_LEVEL_WARN); 

    uint32 gridDiffTime = 0;
    uint32 cell_count = 0;
    uint32 notifier_count = 0;

    const float radius = GetDynamicVisibilityRange();
    const float gobjRadius = sWorld->getRate(RATE_VISIBILITY_GAME_OBJECTS) * GetVisibilityRange();

    const uint32 count = std::max(uint32(relocatedUnits.size()) / 10u, 1u);
    while (true)
    {
        if (relocatedUnitItr == relocatedUnits.end())
        {
            if (relocatedUnits.empty())
                break;
            relocatedUnitItr = relocatedUnits.begin();
        }

        relocatedUnit = *relocatedUnitItr;
        if (relocatedUnit->GetTypeId() == TYPEID_PLAYER)
        {
            Player* player = static_cast<Player*>(relocatedUnit);

            WorldObject const* viewPoint = player->m_seer;
            
            if (player != viewPoint && !viewPoint->IsPositionValid())
            {
                ++relocatedUnitItr;
                continue;
            }

            auto relocate = Trinity::createRelocationNotifier<WorldObject>(*player);
            auto check = createRelocationCheck<WorldObject, GameObject>(*player, radius, gobjRadius);
            auto checked = Trinity::createCheckedSearcher(relocate, check);

            player->VisitNearbyObject(std::max(radius, gobjRadius), checked);
        }
        else
        {
            Creature* creature = static_cast<Creature*>(relocatedUnit);

            auto relocate = Trinity::createRelocationNotifier<Unit>(*creature);
            auto check = createRelocationCheck<Creature, Player>(*creature, MAX_AGGRO_RADIUS, radius);
            auto checked = Trinity::createCheckedSearcher(relocate, check);

            creature->VisitNearbyObject(std::max(radius, MAX_AGGRO_RADIUS), checked);
        }

        if (relocatedUnit)
        {
            if (!relocatedUnit->isMoving())
            {
                relocatedUnitItr = relocatedUnits.erase(relocatedUnitItr);
                relocatedUnit->SetInRelocationList(false);
            }
            else
                ++relocatedUnitItr;
        }

        ++notifier_count;

        if (count > 200 && notifier_count % (count / 4) == 0)
        {
            if (GetMSTimeDiffToNow(GetDiffTimer().GetStartTime()) > GetDiffTimer().GetDiffAvg())
                break;
        }

        if (notifier_count >= count)
            break;
    }

    relocatedUnit = nullptr;
}

void Map::RemovePlayerFromMap(std::shared_ptr<Player> playerPtr)
{
    Player* player = playerPtr.get();

    sScriptMgr->OnPlayerLeaveMap(this, player);
    MONITOR_MAPS(ReportPlayer(this, false));

    player->RemoveFromWorld();

    player->UpdateObjectVisibility(true);
    if (player->IsInGrid())
        player->RemoveFromGrid();

    _players.erase(playerPtr->GetGUID());
}

template<class T>
void Map::RemoveFromMap(T *obj, bool remove)
{
    static_assert(!std::is_same_v<T, WorldObject>, "obj cannot be WorldObject (might fail to remove from move-list.");

    constexpr bool isTransport = std::is_same_v<T, Transport>;

    // todo: find out if o
    if constexpr(!isTransport)
    {
        ASSERT(i_objectsToRemove.find(obj) == i_objectsToRemove.end());
        if (obj->isActiveObject())
            RemoveFromActive(obj);

        OnWorldObjectRemoved(*obj);
    }

    obj->RemoveFromWorld();

    if constexpr(isTransport)
    {
        if (remove)
            GetMapVisibilityGroup()->RemoveObject(obj);

        if (_transportsUpdateIter != _transports.end())
        {
            TransportsContainer::iterator itr = _transports.find(obj);
            if (itr == _transports.end())
                return;
            if (itr == _transportsUpdateIter)
                ++_transportsUpdateIter;
            _transports.erase(itr);
        }
        else
            _transports.erase(obj);

        if (obj->IsInGrid())
            obj->RemoveFromGrid();
    }
    else
    {
        obj->UpdateObjectVisibility(true);
        obj->RemoveFromGrid();
    }

    obj->ResetMap();

    if constexpr(!isTransport)
    {
        if constexpr(Trinity::isCreature<T>)
            RemoveCreatureFromMoveList(obj);
        else if constexpr(Trinity::isUnit<T>)
        {
            if (Creature* creature = obj->ToCreature())
                RemoveCreatureFromMoveList(obj);
        }
        else if constexpr(Trinity::isGameObject<T>)
            RemoveGameObjectFromMoveList(obj);
        else if constexpr(Trinity::isDynObject<T>)
            RemoveDynamicObjectFromMoveList(obj);
    }

    if (remove)
    {
        // if option set then object already saved at this moment
        if (!sWorld->getBoolConfig(CONFIG_SAVE_RESPAWN_TIME_IMMEDIATELY))
            obj->SaveRespawnTime();
        DeleteFromWorld(obj);
    }
}

void Map::PlayerRelocation(Player* player, float x, float y, float z, float orientation)
{
    ASSERT(player);

    Grids::CellCoord old_cell = Grids::CellCoord::FromPosition(player->GetPositionX(), player->GetPositionY());
    Grids::CellCoord new_cell = Grids::CellCoord::FromPosition(x, y);

    //! If hovering, always increase our server-side Z position
    //! Client automatically projects correct position based on Z coord sent in monster move
    //! and UNIT_FIELD_HOVERHEIGHT sent in object updates
    if (player->HasUnitMovementFlag(MOVEMENTFLAG_HOVER))
        z += player->GetFloatValue(UNIT_FIELD_HOVERHEIGHT);

    player->Relocate(x, y, z, orientation);
    if (player->IsVehicle())
        player->GetVehicleKit()->RelocatePassengers();

    if (old_cell != new_cell)
    {
        TC_LOG_DEBUG("maps", "Player %s relocation grid[%u, %u]cell[%u, %u]->grid[%u, %u]cell[%u, %u]", player->GetName().c_str(), old_cell.GetGridX(), old_cell.GetGridY(), old_cell.GetCellX(), old_cell.GetCellY(), new_cell.GetGridX(), new_cell.GetGridY(), new_cell.GetCellX(), new_cell.GetCellY());

        player->RemoveFromGrid();

        if (old_cell.DiffGrid(new_cell))
            EnsureGridLoaded(new_cell);

        AddToGrid(player, new_cell);
    }

    player->UpdateObjectVisibility(false);
}

void Map::CreatureRelocation(Creature* creature, float x, float y, float z, float ang, bool respawnRelocationOnFail)
{
    ASSERT(CheckGridIntegrity(creature, false));

    Grids::CellCoord old_cell = creature->GetCurrentCell();
    Grids::CellCoord new_cell = Grids::CellCoord::FromPosition(x, y);

    if (!respawnRelocationOnFail && !IsGridLoaded(new_cell))
        return;

    //! If hovering, always increase our server-side Z position
    //! Client automatically projects correct position based on Z coord sent in monster move
    //! and UNIT_FIELD_HOVERHEIGHT sent in object updates
    if (creature->HasUnitMovementFlag(MOVEMENTFLAG_HOVER))
        z += creature->GetFloatValue(UNIT_FIELD_HOVERHEIGHT);

    // delay creature move for grid/cell to grid/cell moves
    if (old_cell.DiffCell(new_cell) || old_cell.DiffGrid(new_cell))
    {
        #ifdef TRINITY_DEBUG
            TC_LOG_DEBUG("maps", "Creature (GUID: %u Entry: %u) added to moving list from grid[%u, %u]cell[%u, %u] to grid[%u, %u]cell[%u, %u].", creature->GetGUID().GetCounter(), creature->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());
        #endif
        AddCreatureToMoveList(creature, x, y, z, ang);
        // in diffcell/diffgrid case notifiers called at finishing move creature in Map::MoveAllCreaturesInMoveList
    }
    else
    {
        creature->Relocate(x, y, z, ang);
        creature->UpdateMovementFlags();
        if (creature->IsVehicle())
            creature->GetVehicleKit()->RelocatePassengers();
        creature->UpdateObjectVisibility(false);
        RemoveCreatureFromMoveList(creature);
    }

    ASSERT(CheckGridIntegrity(creature, true));
}

void Map::GameObjectRelocation(GameObject* go, float x, float y, float z, float orientation, bool respawnRelocationOnFail)
{
    Grids::CellCoord integrity_check = Grids::CellCoord::FromPosition(go->GetPositionX(), go->GetPositionY());
    Grids::CellCoord old_cell = go->GetCurrentCell();

    ASSERT(integrity_check == old_cell);
    Grids::CellCoord new_cell = Grids::CellCoord::FromPosition(x, y);

    if (!respawnRelocationOnFail && !IsGridLoaded(new_cell))
        return;

    // delay creature move for grid/cell to grid/cell moves
    if (old_cell.DiffCell(new_cell) || old_cell.DiffGrid(new_cell))
    {
#ifdef TRINITY_DEBUG
        TC_LOG_DEBUG("maps", "GameObject (GUID: %u Entry: %u) added to moving list from grid[%u, %u]cell[%u, %u] to grid[%u, %u]cell[%u, %u].", go->GetGUID().GetCounter(), go->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());
#endif
        AddGameObjectToMoveList(go, x, y, z, orientation);
        // in diffcell/diffgrid case notifiers called at finishing move go in Map::MoveAllGameObjectsInMoveList
    }
    else
    {
        go->Relocate(x, y, z, orientation);
        go->UpdateModelPosition();
        go->UpdateObjectVisibility(false);
        RemoveGameObjectFromMoveList(go);
    }

    old_cell = go->GetCurrentCell();
    integrity_check = Grids::CellCoord::FromPosition(go->GetPositionX(), go->GetPositionY());
    ASSERT(integrity_check == old_cell);
}

void Map::DynamicObjectRelocation(DynamicObject* dynObj, float x, float y, float z, float orientation)
{
    Grids::CellCoord integrity_check = Grids::CellCoord::FromPosition(dynObj->GetPositionX(), dynObj->GetPositionY());
    Grids::CellCoord old_cell = dynObj->GetCurrentCell();

    ASSERT(integrity_check == old_cell);
    Grids::CellCoord new_cell = Grids::CellCoord::FromPosition(x, y);

    if (!IsGridLoaded(new_cell))
        return;

    // delay creature move for grid/cell to grid/cell moves
    if (old_cell.DiffCell(new_cell) || old_cell.DiffGrid(new_cell))
    {
#ifdef TRINITY_DEBUG
        TC_LOG_DEBUG("maps", "GameObject (GUID: %u) added to moving list from grid[%u, %u]cell[%u, %u] to grid[%u, %u]cell[%u, %u].", dynObj->GetGUID().GetCounter(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());
#endif
        AddDynamicObjectToMoveList(dynObj, x, y, z, orientation);
        // in diffcell/diffgrid case notifiers called at finishing move dynObj in Map::MoveAllGameObjectsInMoveList
    }
    else
    {
        dynObj->Relocate(x, y, z, orientation);
        dynObj->UpdateObjectVisibility(false);
        RemoveDynamicObjectFromMoveList(dynObj);
    }

    old_cell = dynObj->GetCurrentCell();
    integrity_check = Grids::CellCoord::FromPosition(dynObj->GetPositionX(), dynObj->GetPositionY());
    ASSERT(integrity_check == old_cell);
}

void Map::AddCreatureToMoveList(Creature* c, float x, float y, float z, float ang)
{
    if (_creatureToMoveLock) //can this happen?
        return;

    if (!c->IsGridActive())
        _creaturesToMove.insert(c);
    c->SetNewCellPosition(x, y, z, ang);
}

void Map::RemoveCreatureFromMoveList(Creature* c)
{
    if (_creatureToMoveLock) //can this happen?
        return;

    if (c->IsGridActive())
    {
        _creaturesToMove.erase(c);
        c->ResetGridState();
    }
}

void Map::AddGameObjectToMoveList(GameObject* go, float x, float y, float z, float ang)
{
    if (_gameObjectsToMoveLock) //can this happen?
        return;

    if (!go->IsGridActive())
        _gameObjectsToMove.insert(go);
    go->SetNewCellPosition(x, y, z, ang);
}

void Map::RemoveGameObjectFromMoveList(GameObject* go)
{
    if (_gameObjectsToMoveLock) //can this happen?
        return;

    if (go->IsGridActive())
    {
        _gameObjectsToMove.erase(go);
        go->ResetGridState();
    }
}

void Map::AddDynamicObjectToMoveList(DynamicObject* dynObj, float x, float y, float z, float ang)
{
    if (_dynamicObjectsToMoveLock) //can this happen?
        return;

    if (!dynObj->IsGridActive())
        _dynamicObjectsToMove.insert(dynObj);
    dynObj->SetNewCellPosition(x, y, z, ang);
}

void Map::RemoveDynamicObjectFromMoveList(DynamicObject* dynObj)
{
    if (_dynamicObjectsToMoveLock) //can this happen?
        return;

    if (dynObj->IsGridActive())
    {
        _dynamicObjectsToMove.erase(dynObj);
        dynObj->ResetGridState();
    }
}

void Map::MoveAllCreaturesInMoveList()
{
    _creatureToMoveLock = true;
    for (auto itr = _creaturesToMove.begin(); itr != _creaturesToMove.end(); ++itr)
    {
        Creature* c = *itr;
        if (c->FindMap() != this) //pet is teleported to another map
            continue;

        if(!c->IsGridActive())
            continue;

        c->ResetGridState();

        if (!c->IsInWorld())
            continue;

        // do move or do move to respawn or remove creature if previous all fail
        if (SwitchGridContainers(c, c->GetNewCellCoord(), c->isActiveObject()))
        {
            // update pos
            c->ApplyNewPosition(*c);

            if (c->IsVehicle())
                c->GetVehicleKit()->RelocatePassengers();
            //CreatureRelocationNotify(c, new_cell, new_cell.cellCoord());
            c->UpdateObjectVisibility(false);
            c->UpdateMovementFlags();
        }
        else
        {
            // if creature can't be move in new cell/grid (not loaded) move it to repawn cell/grid
            // creature coordinates will be updated and notifiers send
            if (!CreatureRespawnRelocation(c, false))
            {
                // ... or unload (if respawn grid also not loaded)
                #ifdef TRINITY_DEBUG
                    TC_LOG_DEBUG("maps", "Creature (GUID: %u Entry: %u) cannot be move to unloaded respawn grid.", c->GetGUID().GetCounter(), c->GetEntry());
                #endif
                //AddObjectToRemoveList(Pet*) should only be called in Pet::Remove
                //This may happen when a player just logs in and a pet moves to a nearby unloaded cell
                //To avoid this, we can load nearby cells when player log in
                //But this check is always needed to ensure safety
                /// @todo pets will disappear if this is outside CreatureRespawnRelocation
                //need to check why pet is frequently relocated to an unloaded cell
                if (c->IsPet())
                    ((Pet*)c)->Remove(PET_SAVE_NOT_IN_SLOT, true);
                else
                    AddObjectToRemoveList(c);
            }
        }
    }
    _creaturesToMove.clear();
    _creatureToMoveLock = false;
}

void Map::MoveAllGameObjectsInMoveList()
{
    _gameObjectsToMoveLock = true;
    for (auto itr = _gameObjectsToMove.begin(); itr != _gameObjectsToMove.end(); ++itr)
    {
        GameObject* go = *itr;
        if (go->FindMap() != this) //transport is teleported to another map
            continue;

        if (!go->IsGridActive())
            continue;

        go->ResetGridState();
        if (!go->IsInWorld())
            continue;

        // do move or do move to respawn or remove creature if previous all fail
        if (SwitchGridContainers(go, go->GetNewCellCoord(), go->isActiveObject()))
        {
            // update pos
            go->ApplyNewPosition(*go);
            go->UpdateModelPosition();
            go->UpdateObjectVisibility(false);
        }
        else
        {
            // if GameObject can't be move in new cell/grid (not loaded) move it to repawn cell/grid
            // GameObject coordinates will be updated and notifiers send
            if (!GameObjectRespawnRelocation(go, false))
            {
                // ... or unload (if respawn grid also not loaded)
#ifdef TRINITY_DEBUG
                TC_LOG_DEBUG("maps", "GameObject (GUID: %u Entry: %u) cannot be move to unloaded respawn grid.", go->GetGUID().GetCounter(), go->GetEntry());
#endif
                AddObjectToRemoveList(go);
            }
        }
    }
    _gameObjectsToMove.clear();
    _gameObjectsToMoveLock = false;
}

void Map::MoveAllDynamicObjectsInMoveList()
{
    _dynamicObjectsToMoveLock = true;
    for (auto itr = _dynamicObjectsToMove.begin(); itr != _dynamicObjectsToMove.end(); ++itr)
    {
        DynamicObject* dynObj = *itr;
        if (dynObj->FindMap() != this) //transport is teleported to another map
            continue;

        if (!dynObj->IsGridActive())
            continue;

		dynObj->ResetGridState();
        if (!dynObj->IsInWorld())
            continue;

        // do move or do move to respawn or remove creature if previous all fail
        if (SwitchGridContainers(dynObj, dynObj->GetNewCellCoord(), dynObj->isActiveObject()))
        {
            // update pos
            dynObj->ApplyNewPosition(*dynObj);
            dynObj->UpdateObjectVisibility(false);
        }
        else
        {
#ifdef TRINITY_DEBUG
            TC_LOG_DEBUG("maps", "DynamicObject (GUID: %u) cannot be moved to unloaded grid.", dynObj->GetGUID().GetCounter());
#endif
        }
    }

    _dynamicObjectsToMove.clear();
    _dynamicObjectsToMoveLock = false;
}

bool Map::CreatureRespawnRelocation(Creature* c, bool diffGridOnly)
{
    float resp_x, resp_y, resp_z, resp_o;
    c->GetRespawnPosition(resp_x, resp_y, resp_z, &resp_o);
    Grids::CellCoord resp_cell = Grids::CellCoord::FromPosition(resp_x, resp_y);

    //creature will be unloaded with grid
    if (diffGridOnly && !c->GetCurrentCell().DiffGrid(resp_cell))
        return false;

    c->CombatStop();
    c->GetMotionMaster()->Clear();

    #ifdef TRINITY_DEBUG
        TC_LOG_DEBUG("maps", "Creature (GUID: %u Entry: %u) moved from grid[%u, %u]cell[%u, %u] to respawn grid[%u, %u]cell[%u, %u].", c->GetGUID().GetCounter(), c->GetEntry(), c->GetCurrentCell().GridX(), c->GetCurrentCell().GridY(), c->GetCurrentCell().CellX(), c->GetCurrentCell().CellY(), resp_cell.GridX(), resp_cell.GridY(), resp_cell.CellX(), resp_cell.CellY());
    #endif

    // teleport it to respawn point (like normal respawn if player see)
    if (SwitchGridContainers(c, resp_cell, !diffGridOnly && c->isActiveObject()))
    {
        c->Relocate(resp_x, resp_y, resp_z, resp_o);
        c->GetMotionMaster()->Initialize();                 // prevent possible problems with default move generators
        //CreatureRelocationNotify(c, resp_cell, resp_cell.GetCellCoord());
        c->UpdateObjectVisibility(false);
        return true;
    }

    return false;
}

bool Map::GameObjectRespawnRelocation(GameObject* go, bool diffGridOnly)
{
    float resp_x, resp_y, resp_z, resp_o;
    go->GetRespawnPosition(resp_x, resp_y, resp_z, &resp_o);
    Grids::CellCoord resp_cell = Grids::CellCoord::FromPosition(resp_x, resp_y);

    //GameObject will be unloaded with grid
    if (diffGridOnly && !go->GetCurrentCell().DiffGrid(resp_cell))
        return false;

    #ifdef TRINITY_DEBUG
        TC_LOG_DEBUG("maps", "GameObject (GUID: %u Entry: %u) moved from grid[%u, %u]cell[%u, %u] to respawn grid[%u, %u]cell[%u, %u].", go->GetGUID().GetCounter(), go->GetEntry(), go->GetCurrentCell().GridX(), go->GetCurrentCell().GridY(), go->GetCurrentCell().CellX(), go->GetCurrentCell().CellY(), resp_cell.GridX(), resp_cell.GridY(), resp_cell.CellX(), resp_cell.CellY());
    #endif

    // teleport it to respawn point (like normal respawn if player see)
    if (SwitchGridContainers(go, resp_cell, !diffGridOnly && go->isActiveObject()))
    {
        go->Relocate(resp_x, resp_y, resp_z, resp_o);
        go->UpdateObjectVisibility(false);
        return true;
    }

    return false;
}

bool Map::UnloadGrid(Grids::CellCoord const& coord)
{
    int gx = coord.GetGridX();
    int gy = coord.GetGridY();

    // delete grid map, but don't delete if it is from parent map (and thus only reference)
    //+++if (GridMaps[gx][gy]) don't check for GridMaps[gx][gy], we might have to unload vmaps
    {
        if (i_InstanceId == 0)
        {
            if (terrainMap[gx][gy])
            {
                terrainMap[gx][gy]->unloadData();
                delete terrainMap[gx][gy];
            }
            VMAP::VMapFactory::createOrGetVMapManager()->unloadMap(GetId(), gx, gy);
            MMAP::MMapFactory::createOrGetMMapManager()->unloadMap(GetId(), gx, gy);
        }
        else
        {
            //@todo
            // ((MapInstanced*)m_parentMap)->RemoveGridMapReference(coord);
        }

        terrainMap[gx][gy] = NULL;
    }
    TC_LOG_DEBUG("maps", "Unloading grid[%u, %u] for map %u finished", gx, gy, GetId());
    return true;
}

void Map::RemoveAllPlayers()
{
    if (HavePlayers())
    {
        for (Player& player : GetPlayers())
        {
            if (!player.IsBeingTeleportedFar())
            {
                // this is happening for bg
                TC_LOG_ERROR("maps", "Map::UnloadAll: player %s is still in map %u during unload, this should not happen!", player.GetName().c_str(), GetId());
                player.TeleportTo(player.m_homebindMapId, player.m_homebindX, player.m_homebindY, player.m_homebindZ, player.GetOrientation());
            }
        }
    }
}

void Map::OnCellCleanup(Creature* creature)
{
    if (!CreatureRespawnRelocation(creature, true))
    {
        creature->CleanupsBeforeDelete();
        RemoveFromMap(creature, true);
    }
}

void Map::OnCellCleanup(GameObject* gameObject)
{
    if (!GameObjectRespawnRelocation(gameObject, true))
    {
        gameObject->CleanupsBeforeDelete();
        RemoveFromMap(gameObject, true);
    }
}

void Map::OnCellCleanup(DynamicObject* dynObject)
{
    dynObject->CleanupsBeforeDelete();
    RemoveFromMap(dynObject, true);
}

void Map::OnCellCleanup(Corpse* corpse)
{
    corpse->CleanupsBeforeDelete();
    RemoveFromMap(corpse, corpse->GetType() == CORPSE_BONES);
}

void Map::UnloadAll()
{
    // clear all delayed moves, useless anyway do this moves before map unload.
    _creaturesToMove.clear();
    _gameObjectsToMove.clear();

    gridMap.UnloadAll();

    for (TransportsContainer::iterator itr = _transports.begin(); itr != _transports.end();)
    {
        Transport* transport = *itr;
        ++itr;

        RemoveFromMap(transport, true);
    }
}

float Map::GetWaterOrGroundLevel(uint32 phasemask, float x, float y, float z, float* ground /*= NULL*/, bool /*swim = false*/) const
{
    if (const_cast<Map*>(this)->GetGrid(x, y))
    {
        // we need ground level (including grid height version) for proper return water level in point

        float ground_z = GetHeight(phasemask, x, y, z, true, 50.0f);
        
        if (ground_z <= INVALID_HEIGHT)
            ground_z = z - 1.0f;
        
        if (ground)
            *ground = ground_z;

        LiquidData liquid_status;

        ZLiquidStatus res = getLiquidStatus(x, y, ground_z, MAP_ALL_LIQUIDS, &liquid_status);
        return res ? liquid_status.level : ground_z;
    }

    return VMAP_INVALID_HEIGHT_VALUE;
}

float Map::GetHeight(float x, float y, float z, bool checkVMap /*= true*/, float maxSearchDist /*= DEFAULT_HEIGHT_SEARCH*/) const
{
    // find raw .map surface under Z coordinates
    float mapHeight = VMAP_INVALID_HEIGHT_VALUE;
    if (TerrainMap* gmap = const_cast<Map*>(this)->GetGrid(x, y))
    {
        float gridHeight = gmap->getHeight(x, y);
        // look from a bit higher pos to find the floor, ignore under surface case
        if (z + 2.0f > gridHeight)
            mapHeight = gridHeight;
    }

    float vmapHeight = VMAP_INVALID_HEIGHT_VALUE;
    if (checkVMap && !(_vmap_disable_flags & VMAP::VMAP_DISABLE_HEIGHT))
    {
        VMAP::IVMapManager* vmgr = VMAP::VMapFactory::createOrGetVMapManager();
        if (vmgr->isHeightCalcEnabled())
            vmapHeight = vmgr->getHeight(GetId(), x, y, z + 2.0f, maxSearchDist);   // look from a bit higher pos to find the floor
    }

    // mapHeight set for any above raw ground Z or <= INVALID_HEIGHT
    // vmapheight set for any under Z value or <= INVALID_HEIGHT
    if (vmapHeight > INVALID_HEIGHT)
    {
        if (mapHeight > INVALID_HEIGHT)
        {
            // we have mapheight and vmapheight and must select more appropriate

            // vmap height above map height
            // or if the distance of the vmap height is less the land height distance
            if (vmapHeight > mapHeight || std::fabs(mapHeight - z) > std::fabs(vmapHeight - z))
                return vmapHeight;
            else
                return mapHeight;                           // better use .map surface height
        }
        else
            return vmapHeight;                              // we have only vmapHeight (if have)
    }

    return mapHeight;                               // explicitly use map data
}

float Map::GetMinHeight(float x, float y) const
{
    if (TerrainMap const* grid = const_cast<Map*>(this)->GetGrid(x, y))
        return grid->getMinHeight(x, y);

    return -500.0f;
}

inline bool IsOutdoorWMO(uint32 mogpFlags, int32 /*adtId*/, int32 /*rootId*/, int32 /*groupId*/, WMOAreaTableEntry const* wmoEntry, AreaTableEntry const* atEntry)
{
    bool outdoor = true;

    if (wmoEntry && atEntry)
    {
        if (atEntry->flags & AREA_FLAG_OUTSIDE)
            return true;
        if (atEntry->flags & AREA_FLAG_INSIDE)
            return false;
    }

    outdoor = (mogpFlags & 0x8) != 0;

    if (wmoEntry)
    {
        if (wmoEntry->Flags & 4)
            return true;
        if (wmoEntry->Flags & 2)
            outdoor = false;
    }
    return outdoor;
}

bool Map::IsOutdoors(float x, float y, float z) const
{
    uint32 mogpFlags;
    int32 adtId, rootId, groupId;

    // no wmo found? -> outside by default
    if (!GetAreaInfo(x, y, z, mogpFlags, adtId, rootId, groupId))
        return true;

    AreaTableEntry const* atEntry = nullptr;
    WMOAreaTableEntry const* wmoEntry= GetWMOAreaTableEntryByTripple(rootId, adtId, groupId);
    if (wmoEntry)
    {
        TC_LOG_DEBUG("maps", "Got WMOAreaTableEntry! flag %u, areaid %u", wmoEntry->Flags, wmoEntry->areaId);
        atEntry = sAreaTableStore.LookupEntry(wmoEntry->areaId);
    }
    return IsOutdoorWMO(mogpFlags, adtId, rootId, groupId, wmoEntry, atEntry);
}

bool Map::GetAreaInfo(float x, float y, float z, uint32 &flags, int32 &adtId, int32 &rootId, int32 &groupId) const
{
    if (_vmap_disable_flags & VMAP::VMAP_DISABLE_AREAFLAG)
        return false;

    float vmap_z = z;
    VMAP::IVMapManager* vmgr = VMAP::VMapFactory::createOrGetVMapManager();
    if (vmgr->getAreaInfo(GetId(), x, y, vmap_z, flags, adtId, rootId, groupId))
    {
        // check if there's terrain between player height and object height
        if (TerrainMap* gmap = const_cast<Map*>(this)->GetGrid(x, y))
        {
            float _mapheight = gmap->getHeight(x, y);
            // z + 2.0f condition taken from GetHeight(), not sure if it's such a great choice...
            if (z + 2.0f > _mapheight &&  _mapheight > vmap_z)
                return false;
        }
        return true;
    }
    return false;
}

MapArea* Map::GetAreaForObject(const WorldObject& object) const
{
    const auto areaId = GetAreaId(object.GetPositionX(), object.GetPositionY(), object.GetPositionZ());

    auto itr = areas.find(areaId);
    if (itr != areas.end())
        return itr->second.get();

    return nullptr;
}

uint32 Map::GetAreaId(float x, float y, float z, bool *isOutdoors) const
{
    uint32 mogpFlags;
    int32 adtId, rootId, groupId;
    WMOAreaTableEntry const* wmoEntry = nullptr;
    AreaTableEntry const* atEntry = nullptr;
    bool haveAreaInfo = false;

    if (GetAreaInfo(x, y, z, mogpFlags, adtId, rootId, groupId))
    {
        haveAreaInfo = true;
        wmoEntry = GetWMOAreaTableEntryByTripple(rootId, adtId, groupId);
        if (wmoEntry)
            atEntry = sAreaTableStore.LookupEntry(wmoEntry->areaId);
    }

    uint32 areaId = 0;

    if (atEntry)
        areaId = atEntry->ID;
    else
    {
        if (TerrainMap* gmap = const_cast<Map*>(this)->GetGrid(x, y))
            areaId = gmap->getArea(x, y);

        // this used while not all *.map files generated (instances)
        if (!areaId)
            areaId = i_mapEntry->linked_zone;
    }

    if (isOutdoors)
    {
        if (haveAreaInfo)
            *isOutdoors = IsOutdoorWMO(mogpFlags, adtId, rootId, groupId, wmoEntry, atEntry);
        else
            *isOutdoors = true;
    }
    return areaId;
}

uint32 Map::GetAreaId(float x, float y, float z) const
{
    return GetAreaId(x, y, z, nullptr);
}

uint32 Map::GetZoneId(float x, float y, float z) const
{
    uint32 areaId = GetAreaId(x, y, z);
    if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(areaId))
        if (area->zone)
            return area->zone;

    return areaId;
}

void Map::GetZoneAndAreaId(uint32& zoneid, uint32& areaid, float x, float y, float z) const
{
    areaid = zoneid = GetAreaId(x, y, z);
    if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(areaid))
        if (area->zone)
            zoneid = area->zone;
}

uint8 Map::GetTerrainType(float x, float y) const
{
    if (TerrainMap* gmap = const_cast<Map*>(this)->GetGrid(x, y))
        return gmap->getTerrainType(x, y);
    else
        return 0;
}

ZLiquidStatus Map::getLiquidStatus(float x, float y, float z, uint8 ReqLiquidType, LiquidData* data) const
{
    ZLiquidStatus result = LIQUID_MAP_NO_WATER;
    float liquid_level = INVALID_HEIGHT;
    float ground_level = INVALID_HEIGHT;
    uint32 liquid_type = 0;

    if (!(_vmap_disable_flags & VMAP::VMAP_DISABLE_LIQUIDSTATUS))
    {
        VMAP::IVMapManager* vmgr = VMAP::VMapFactory::createOrGetVMapManager();
        if (vmgr->GetLiquidLevel(GetId(), x, y, z, ReqLiquidType, liquid_level, ground_level, liquid_type))
        {
            TC_LOG_DEBUG("maps", "getLiquidStatus(): vmap liquid level: %f ground: %f type: %u", liquid_level, ground_level, liquid_type);
            // Check water level and ground level
            if (liquid_level > ground_level && z > ground_level - 2)
            {
                // All ok in water -> store data
                if (data)
                {
                    // hardcoded in client like this
                    if (GetId() == 530 && liquid_type == 2)
                        liquid_type = 15;

                    uint32 liquidFlagType = 0;
                    if (LiquidTypeEntry const* liq = sLiquidTypeStore.LookupEntry(liquid_type))
                        liquidFlagType = liq->Type;

                    if (liquid_type && liquid_type < 21)
                    {
                        if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(GetAreaId(x, y, z)))
                        {
                            uint32 overrideLiquid = area->LiquidTypeOverride[liquidFlagType];
                            if (!overrideLiquid && area->zone)
                            {
                                area = sAreaTableStore.LookupEntry(area->zone);
                                if (area)
                                    overrideLiquid = area->LiquidTypeOverride[liquidFlagType];
                            }

                            if (LiquidTypeEntry const* liq = sLiquidTypeStore.LookupEntry(overrideLiquid))
                            {
                                liquid_type = overrideLiquid;
                                liquidFlagType = liq->Type;
                            }
                        }
                    }

                    data->level = liquid_level;
                    data->depth_level = ground_level;

                    data->entry = liquid_type;
                    data->type_flags = 1 << liquidFlagType;
                }

                float delta = liquid_level - z;

                // Get position delta
                if (delta > 2.0f)                   // Under water
                    return LIQUID_MAP_UNDER_WATER;
                if (delta > 0.0f)                   // In water
                    return LIQUID_MAP_IN_WATER;
                if (delta > -0.1f)                   // Walk on water
                    return LIQUID_MAP_WATER_WALK;
                result = LIQUID_MAP_ABOVE_WATER;
            }
        }
    }

    if (TerrainMap* gmap = const_cast<Map*>(this)->GetGrid(x, y))
    {
        LiquidData map_data;
        ZLiquidStatus map_result = gmap->getLiquidStatus(x, y, z, ReqLiquidType, &map_data);
        // Not override LIQUID_MAP_ABOVE_WATER with LIQUID_MAP_NO_WATER:
        if (map_result != LIQUID_MAP_NO_WATER && (map_data.level > ground_level))
        {
            if (data)
            {
                // hardcoded in client like this
                if (GetId() == 530 && map_data.entry == 2)
                    map_data.entry = 15;

                *data = map_data;
            }
            return map_result;
        }
    }
    return result;
}

float Map::GetWaterLevel(float x, float y) const
{
    if (TerrainMap* gmap = const_cast<Map*>(this)->GetGrid(x, y))
        return gmap->getLiquidLevel(x, y);
    else
        return 0;
}

bool Map::isInLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2, uint32 phasemask) const
{
    if (!(_vmap_disable_flags & VMAP::VMAP_DISABLE_LOS))
    {
        if (!VMAP::VMapFactory::createOrGetVMapManager()->isInLineOfSight(GetId(), x1, y1, z1, x2, y2, z2))
            return false;
    }
    return _dynamicTree.isInLineOfSight(x1, y1, z1, x2, y2, z2, phasemask);
}

bool Map::getStaticObjectHitPos(float x1, float y1, float z1, float x2, float y2, float z2, float& rx, float& ry, float& rz, float modifyDist)
{
    if (_vmap_disable_flags & VMAP::VMAP_DISABLE_LOS)
    {
        rx = x2;
        ry = y2;
        rz = z2;
        return false;
    }

    return VMAP::VMapFactory::createOrGetVMapManager()->getObjectHitPos(GetId(), x1, y1, z1, x2, y2, z2, rx, ry, rz, modifyDist);
}

bool Map::getDynamicObjectHitPos(uint32 phasemask, float x1, float y1, float z1, float x2, float y2, float z2, float& rx, float& ry, float& rz, float modifyDist)
{
    G3D::Vector3 startPos(x1, y1, z1);
    G3D::Vector3 dstPos(x2, y2, z2);

    G3D::Vector3 resultPos;
    bool result = _dynamicTree.getObjectHitPos(phasemask, startPos, dstPos, resultPos, modifyDist);

    rx = resultPos.x;
    ry = resultPos.y;
    rz = resultPos.z;
    return result;
}

float Map::GetHeight(uint32 phasemask, float x, float y, float z, bool vmap/*=true*/, float maxSearchDist/*=DEFAULT_HEIGHT_SEARCH*/) const
{
    return std::max<float>(GetHeight(x, y, z, vmap, maxSearchDist), _dynamicTree.getHeight(x, y, z, maxSearchDist, phasemask));
}

bool Map::IsInWater(float x, float y, float pZ, LiquidData* data) const
{
    LiquidData liquid_status;
    LiquidData* liquid_ptr = data ? data : &liquid_status;
    return (getLiquidStatus(x, y, pZ, MAP_ALL_LIQUIDS, liquid_ptr) & (LIQUID_MAP_IN_WATER | LIQUID_MAP_UNDER_WATER)) != 0;
}

bool Map::IsUnderWater(float x, float y, float z) const
{
    return (getLiquidStatus(x, y, z, MAP_LIQUID_TYPE_WATER | MAP_LIQUID_TYPE_OCEAN) & LIQUID_MAP_UNDER_WATER) != 0;
}

bool Map::CheckGridIntegrity(Creature* c, bool moved) const
{
    Grids::CellCoord const& cur_cell = c->GetCurrentCell();
    Grids::CellCoord xy_cell = Grids::CellCoord::FromPosition(c->GetPositionX(), c->GetPositionY());
    if (xy_cell != cur_cell)
    {
        TC_LOG_DEBUG("maps", "Creature (GUID: %u) X: %f Y: %f (%s) is in grid[%u, %u]cell[%u, %u] instead of grid[%u, %u]cell[%u, %u]",
            c->GetGUID().GetCounter(),
            c->GetPositionX(), c->GetPositionY(), (moved ? "final" : "original"),
            cur_cell.GetGridX(), cur_cell.GetGridY(), cur_cell.GetCellX(), cur_cell.GetCellY(),
            xy_cell.GetGridX(),  xy_cell.GetGridY(),  xy_cell.GetCellX(),  xy_cell.GetCellY());
        return true;                                        // not crash at error, just output error in debug mode
    }

    return true;
}

char const* Map::GetMapName() const
{
    return i_mapEntry ? i_mapEntry->name[sWorld->GetDefaultDbcLocale()] : "UNNAMEDMAP\x0";
}

void Map::UpdateObjectVisibility(WorldObject* obj)
{
    Trinity::VisibleChangesNotifier notifier(*obj);
    obj->VisitNearbyObject(obj->GetVisibilityRange(), notifier);
}

void Map::UpdateObjectsVisibilityFor(Player* player)
{
    auto notifier = Trinity::createVisibleNotifier(*player);
    player->VisitNearbyObject(player->GetSightRange(), notifier);
}

void Map::SendInitSelf(Player* player)
{
    TC_LOG_DEBUG("maps", "Creating player data for himself %u", player->GetGUID().GetCounter());

    UpdateData data;

    // attach to player data current transport data
    if (Transport* transport = player->GetTransport())
    {
        transport->BuildUpdateBlockForPlayer(data, player);
    }

    // build data for self presence in world at own client (one time for map)
    player->BuildUpdateBlockForPlayer(data, player);

    // build other passengers at transport also (they always visible and marked as visible and will not send at visibility update at add to map
    if (Transport* transport = player->GetTransport())
        for (Transport::PassengerSet::const_iterator itr = transport->GetPassengers().begin(); itr != transport->GetPassengers().end(); ++itr)
            if (player != (*itr) && player->HaveAtClient(*itr))
                (*itr)->BuildUpdateBlockForPlayer(data, player);

    player->SendDirectMessage(data.BuildPacket());
}

void Map::DelayedUpdate(const uint32 t_diff)
{
    // Don't unload grids if it's battleground, since we may have manually added GOs, creatures, those doesn't load from DB at grid re-load !
    // This isn't really bother us, since as soon as we have instanced BG-s, the whole map unloads as the BG gets ended
    if (!IsBattlegroundOrArena())
    {
        // @todo
    }
}

void Map::AddObjectToRemoveList(WorldObject* obj)
{
    ASSERT(obj->GetMapId() == GetId() && obj->GetInstanceId() == GetInstanceId());

    *obj <<= Fire::UnInitWorld;
    obj->CleanupsBeforeDelete(false);                            // remove or simplify at least cross referenced links

    i_objectsToRemove.insert(obj);
    //TC_LOG_DEBUG("maps", "Object (GUID: %u TypeId: %u) added to removing list.", obj->GetGUID().GetCounter(), obj->GetTypeId());
}

void Map::RemoveAllObjectsInRemoveList()
{
    //TC_LOG_DEBUG("maps", "Object remover 1 check.");
    while (!i_objectsToRemove.empty())
    {
        std::set<WorldObject*>::iterator itr = i_objectsToRemove.begin();
        WorldObject* obj = *itr;

        i_objectsToRemove.erase(itr);

        switch (obj->GetTypeId())
        {
            case TYPEID_CORPSE:
            {
                Corpse* corpse = ObjectAccessor::GetCorpse(*obj, obj->GetGUID());
                if (!corpse)
                    TC_LOG_ERROR("maps", "Tried to delete corpse/bones %u that is not in map.", obj->GetGUID().GetCounter());
                else
                {
                    ASSERT(corpse->GetType() == CORPSE_BONES);
                    RemoveFromMap(corpse, true);
                }
                break;
            }
            case TYPEID_DYNAMICOBJECT:
                RemoveFromMap(obj->ToDynObject(), true);
                break;
            case TYPEID_GAMEOBJECT:
            {
                GameObject* go = obj->ToGameObject();
                if (Transport* transport = go->ToTransport())
                    RemoveFromMap(transport, true);
                else
                    RemoveFromMap(go, true);
                break;
            }
            case TYPEID_UNIT:
                // in case triggered sequence some spell can continue casting after prev CleanupsBeforeDelete call
                // make sure that like sources auras/etc removed before destructor start
                obj->ToCreature()->CleanupsBeforeDelete();
                RemoveFromMap(obj->ToCreature(), true);
                break;
            default:
                TC_LOG_ERROR("maps", "Non-grid object (TypeId: %hhu) is in grid object remove list, ignored.", obj->GetTypeId());
                break;
        }

    }

    //TC_LOG_DEBUG("maps", "Object remover 2 check.");
}

uint32 Map::GetPlayersCountExceptGMs() const
{
    uint32 count = 0;
    for (Player& player : GetPlayers())
        if (!player.IsGameMaster())
            count++;
    return count;
}

void Map::SendToPlayers(const WorldPacket& data) const
{
    for (Player& player : GetPlayers())
        player.SendDirectMessage(data);
}

template<class T>
void Map::AddToActive(T* obj)
{
    AddToActiveHelper(obj);
}

template <>
void Map::AddToActive(Creature* c)
{
    AddToActiveHelper(c);

    // also not allow unloading spawn grid to prevent creating creature clone at load
    if (!c->IsPet() && c->GetSpawnId())
    {
        float x, y, z;
        c->GetRespawnPosition(x, y, z);
        Grids::CellCoord p = Grids::CellCoord::FromPosition(x, y);

        EnsureTerrainLoaded(p);
    }
}

template<>
void Map::AddToActive(DynamicObject* d)
{
    AddToActiveHelper(d);
}

template<>
void Map::AddToActive(GameObject* obj)
{
    AddToActiveHelper(obj);
}

template<class T>
void Map::RemoveFromActive(T* /*obj*/) { }

template <>
void Map::RemoveFromActive(Creature* c)
{
    RemoveFromActiveHelper(c);

    // also allow unloading spawn grid
    if (!c->IsPet() && c->GetSpawnId())
    {
        float x, y, z;
        c->GetRespawnPosition(x, y, z);
    }
}

template<>
void Map::RemoveFromActive(DynamicObject* obj)
{
    RemoveFromActiveHelper(obj);
}

template<>
void Map::RemoveFromActive(GameObject* obj)
{
    RemoveFromActiveHelper(obj);
}

template GAME_API bool Map::AddToMap(Corpse*);
template GAME_API bool Map::AddToMap(Creature*);
template GAME_API bool Map::AddToMap(GameObject*);
template GAME_API bool Map::AddToMap(DynamicObject*);

template GAME_API void Map::RemoveFromMap(Corpse*, bool);
template GAME_API void Map::RemoveFromMap(Creature*, bool);
template GAME_API void Map::RemoveFromMap(GameObject*, bool);
template GAME_API void Map::RemoveFromMap(DynamicObject*, bool);
template GAME_API void Map::RemoveFromMap(Transport*, bool);

namespace
{
    //! we don't mind leaking instance ids here, if an id is reused the timer is likly to have run out
    std::unordered_map<uint32, uint32> _last_force_unload_time;
}

bool Map::ForceUnload()
{
    if (HavePlayers())
        return false;

    _force_unload = true;

    _last_force_unload_time[GetInstanceId()] = getMSTime();

    return true;
}

bool Map::CanForceUnload()
{
    if (_force_unload)
        return false;

    auto itr = _last_force_unload_time.find(GetInstanceId());
    if (itr == _last_force_unload_time.end())
        return true;

    uint32 diff = GetMSTimeDiffToNow(itr->second);
    return diff > sWorld->getIntConfig(RG_CONFIG_INSTANCE_UNLOAD_COOLDOWN);
}

bool Map::CanEnter(Player* /*player*/)
{
    if (_force_unload)
        return false;

    return true;
}


/* ******* Battleground Instance Maps ******* */

BattlegroundMap::BattlegroundMap(uint32 id, time_t expiry, uint32 InstanceId, Map* _parent, uint8 spawnMode)
  : Map(id, expiry, InstanceId, spawnMode, _parent), m_bg(NULL)
{
    //lets initialize visibility distance for BG/Arenas
    BattlegroundMap::InitVisibilityDistance();
}

BattlegroundMap::~BattlegroundMap()
{
    if (m_bg)
    {
        //unlink to prevent crash, always unlink all pointer reference before destruction
        m_bg->SetBgMap(NULL);
        m_bg = NULL;
    }
}

void BattlegroundMap::InitVisibilityDistance()
{
    //init visibility distance for BG/Arenas
    m_VisibleDistance = World::GetMaxVisibleDistanceInBGArenas();
    m_VisibilityNotifyPeriod = World::GetVisibilityNotifyPeriodInBGArenas();
}

bool BattlegroundMap::CanEnter(Player* player)
{
    if (_players.find(player->GetGUID()) != _players.end())
    {
        TC_LOG_ERROR("maps", "BGMap::CanEnter - player %u is already in map!", player->GetGUID().GetCounter());
        ABORT();
        return false;
    }

    if (player->GetBattlegroundId() != GetInstanceId())
        return false;

    // player number limit is checked in bgmgr, no need to do it here

    return Map::CanEnter(player);
}

bool BattlegroundMap::AddPlayerToMap(std::shared_ptr<Player> player)
{
    {
        std::lock_guard<std::mutex> lock(_mapLock);
        //Check moved to void WorldSession::HandleMoveWorldportAckOpcode()
        //if (!CanEnter(player))
            //return false;
        // reset instance validity, battleground maps do not homebind
        player->m_InstanceValid = true;
    }
    return Map::AddPlayerToMap(player);
}

void BattlegroundMap::RemovePlayerFromMap(std::shared_ptr<Player> player)
{
    TC_LOG_DEBUG("maps", "MAP: Removing player '%s' from bg '%u' of map '%s' before relocating to another map", player->GetName().c_str(), GetInstanceId(), GetMapName());
    Map::RemovePlayerFromMap(player);
}

void BattlegroundMap::SetUnload()
{
    m_unloadTimer = MIN_UNLOAD_DELAY;
}

void BattlegroundMap::RemoveAllPlayers()
{
    if (HavePlayers())
        for (Player& player : GetPlayers())
            if (!player.IsBeingTeleportedFar())
                player.TeleportTo(player.GetBattlegroundEntryPoint());
}

ArenaReplayMap::ArenaReplayMap(uint32 id, time_t expiry, uint32 InstanceId, Map* _parent, uint8 spawnMode)
    : Map(id, expiry, InstanceId, spawnMode, _parent), closeTimer(0), m_replaying(false), m_replaySpeed(1.f)
{
    //lets initialize visibility distance for BG/Arenas
    ArenaReplayMap::InitVisibilityDistance();
    m_isReplayMap = true;
    m_replaySpeed = 1;
}

ArenaReplayMap::~ArenaReplayMap() { }

void ArenaReplayMap::InitVisibilityDistance()
{
    //init visibility distance for BG/Arenas
    m_VisibleDistance = World::GetMaxVisibleDistanceInBGArenas();
    m_VisibilityNotifyPeriod = World::GetVisibilityNotifyPeriodInBGArenas();
}

bool ArenaReplayMap::AddPlayerToMap(std::shared_ptr<Player> player)
{
    {
        std::lock_guard<std::mutex> lock(_mapLock);
        player->m_InstanceValid = true;
    }
    return Map::AddPlayerToMap(player);
}

void ArenaReplayMap::RemovePlayerFromMap(std::shared_ptr<Player> player)
{
    player->SetGuidValue(PLAYER_FARSIGHT, ObjectGuid::Empty);
    Map::RemovePlayerFromMap(player);
}

void ArenaReplayMap::SetUnload()
{
    m_unloadTimer = MIN_UNLOAD_DELAY;
}

Player* Map::GetPlayer(ObjectGuid const& guid)
{
    ThreadsafetyDebugger::AssertAllowed(ThreadsafetyDebugger::Context::NEEDS_MAP, GetId());

    auto itr = _players.find(guid);
    if (itr != _players.end())
        return itr->second.get();
    return nullptr;
}

Creature* Map::GetCreature(ObjectGuid const& guid)
{
    ThreadsafetyDebugger::AssertAllowed(ThreadsafetyDebugger::Context::NEEDS_MAP, GetId());

    if (auto const creature = _objectStore.Find<Creature>(guid))
        return &(*creature);

    return nullptr;
}

GameObject* Map::GetGameObject(ObjectGuid const& guid)
{
    ThreadsafetyDebugger::AssertAllowed(ThreadsafetyDebugger::Context::NEEDS_MAP, GetId());

    if (auto const object = _objectStore.Find<GameObject>(guid))
        return &(*object);

    return nullptr;
}

Transport* Map::GetTransport(ObjectGuid const& guid)
{
    ThreadsafetyDebugger::AssertAllowed(ThreadsafetyDebugger::Context::NEEDS_MAP, GetId());

    if (guid.GetHigh() == HighGuid::Mo_Transport)
        return ObjectAccessor::GetMotionTransport(*this, guid);

    if (GameObject* go = GetGameObject(guid))
        return go->ToTransport();

    return nullptr;
}

DynamicObject* Map::GetDynamicObject(ObjectGuid const& guid)
{
    ThreadsafetyDebugger::AssertAllowed(ThreadsafetyDebugger::Context::NEEDS_MAP, GetId());

    if (auto const object = _objectStore.Find<DynamicObject>(guid))
        return &(*object);

    return nullptr;
}

Corpse* Map::GetCorpse(ObjectGuid const& guid)
{
    ThreadsafetyDebugger::AssertAllowed(ThreadsafetyDebugger::Context::NEEDS_MAP, GetId());

    if (auto const corpse = _objectStore.Find<Corpse>(guid))
        return &(*corpse);

    return nullptr;
}

std::string Map::OnQueryOwnerName() const
{
    return GetMapName();
}

bool Map::OnInitScript(std::string_view entry)
{
    return loadScriptType(*this, entry);
}

void Map::SaveCreatureRespawnTime(uint32 dbGuid, time_t respawnTime)
{
    if (!respawnTime)
    {
        // Delete only
        RemoveCreatureRespawnTime(dbGuid);
        return;
    }

    _creatureRespawnTimes[dbGuid] = respawnTime;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_CREATURE_RESPAWN);
    stmt->setUInt32(0, dbGuid);
    stmt->setUInt32(1, uint32(respawnTime));
    stmt->setUInt16(2, GetId());
    stmt->setUInt32(3, GetInstanceId());
    CharacterDatabase.Execute(stmt);
}

void Map::RemoveCreatureRespawnTime(uint32 dbGuid)
{
    _creatureRespawnTimes.erase(dbGuid);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CREATURE_RESPAWN);
    stmt->setUInt32(0, dbGuid);
    stmt->setUInt16(1, GetId());
    stmt->setUInt32(2, GetInstanceId());
    CharacterDatabase.Execute(stmt);
}

void Map::SaveGORespawnTime(uint32 dbGuid, time_t respawnTime)
{
    if (!respawnTime)
    {
        // Delete only
        RemoveGORespawnTime(dbGuid);
        return;
    }

    _goRespawnTimes[dbGuid] = respawnTime;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_GO_RESPAWN);
    stmt->setUInt32(0, dbGuid);
    stmt->setUInt32(1, uint32(respawnTime));
    stmt->setUInt16(2, GetId());
    stmt->setUInt32(3, GetInstanceId());
    CharacterDatabase.Execute(stmt);
}

void Map::RemoveGORespawnTime(uint32 dbGuid)
{
    _goRespawnTimes.erase(dbGuid);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GO_RESPAWN);
    stmt->setUInt32(0, dbGuid);
    stmt->setUInt16(1, GetId());
    stmt->setUInt32(2, GetInstanceId());
    CharacterDatabase.Execute(stmt);
}

void Map::LoadRespawnTimes()
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CREATURE_RESPAWNS);
    stmt->setUInt16(0, GetId());
    stmt->setUInt32(1, GetInstanceId());
    if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 loguid      = fields[0].GetUInt32();
            uint32 respawnTime = fields[1].GetUInt32();

            _creatureRespawnTimes[loguid] = time_t(respawnTime);
        } while (result->NextRow());
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GO_RESPAWNS);
    stmt->setUInt16(0, GetId());
    stmt->setUInt32(1, GetInstanceId());
    if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 loguid      = fields[0].GetUInt32();
            uint32 respawnTime = fields[1].GetUInt32();

            _goRespawnTimes[loguid] = time_t(respawnTime);
        } while (result->NextRow());
    }
}

void Map::DeleteRespawnTimes()
{
    _creatureRespawnTimes.clear();
    _goRespawnTimes.clear();

    DeleteRespawnTimesInDB(GetId(), GetInstanceId());
}

void Map::DeleteRespawnTimesInDB(uint16 mapId, uint32 instanceId)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CREATURE_RESPAWN_BY_INSTANCE);
    stmt->setUInt16(0, mapId);
    stmt->setUInt32(1, instanceId);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GO_RESPAWN_BY_INSTANCE);
    stmt->setUInt16(0, mapId);
    stmt->setUInt32(1, instanceId);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

time_t Map::GetLinkedRespawnTime(ObjectGuid guid) const
{
    ObjectGuid linkedGuid = sObjectMgr->GetLinkedRespawnGuid(guid);
    switch (linkedGuid.GetHigh())
    {
        case HighGuid::Unit:
            return GetCreatureRespawnTime(linkedGuid.GetCounter());
        case HighGuid::Gameobject:
            return GetGORespawnTime(linkedGuid.GetCounter());
        default:
            break;
    }

    return time_t(0);
}

void Map::SendZoneDynamicInfo(Player* player)
{
    uint32 zoneId = GetZoneId(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ());
    ZoneDynamicInfoMap::const_iterator itr = _zoneDynamicInfo.find(zoneId);
    if (itr == _zoneDynamicInfo.end())
        return;

    if (uint32 music = itr->second.MusicId)
    {
        WorldPacket data(SMSG_PLAY_MUSIC, 4);
        data << uint32(music);
        player->SendDirectMessage(std::move(data));
    }

    if (uint32 weather = itr->second.WeatherId)
    {
        WorldPacket data(SMSG_WEATHER, 4 + 4 + 1);
        data << uint32(weather);
        data << float(itr->second.WeatherGrade);
        data << uint8(0);
        player->SendDirectMessage(std::move(data));
    }

    if (uint32 overrideLight = itr->second.OverrideLightId)
    {
        WorldPacket data(SMSG_OVERRIDE_LIGHT, 4 + 4 + 1);
        data << uint32(_defaultLight);
        data << uint32(overrideLight);
        data << uint32(itr->second.LightFadeInTime);
        player->SendDirectMessage(std::move(data));
    }
}

void Map::SetZoneMusic(uint32 zoneId, uint32 musicId)
{
    if (_zoneDynamicInfo.find(zoneId) == _zoneDynamicInfo.end())
        _zoneDynamicInfo.insert(ZoneDynamicInfoMap::value_type(zoneId, ZoneDynamicInfo()));

    _zoneDynamicInfo[zoneId].MusicId = musicId;

    if (!GetPlayers().empty())
    {
        WorldPacket data(SMSG_PLAY_MUSIC, 4);
        data << uint32(musicId);

        for (Player& player : GetPlayers())
            if (player.GetZoneId() == zoneId)
                player.SendDirectMessage(data);
    }
}

void Map::SetZoneWeather(uint32 zoneId, uint32 weatherId, float weatherGrade)
{
    if (_zoneDynamicInfo.find(zoneId) == _zoneDynamicInfo.end())
        _zoneDynamicInfo.insert(ZoneDynamicInfoMap::value_type(zoneId, ZoneDynamicInfo()));

    ZoneDynamicInfo& info = _zoneDynamicInfo[zoneId];
    info.WeatherId = weatherId;
    info.WeatherGrade = weatherGrade;

    if (!GetPlayers().empty())
    {
        WorldPacket data(SMSG_WEATHER, 4 + 4 + 1);
        data << uint32(weatherId);
        data << float(weatherGrade);
        data << uint8(0);

        for (Player& player : GetPlayers())
            if (player.GetZoneId() == zoneId)
                player.SendDirectMessage(data);
    }
}

void Map::SetZoneOverrideLight(uint32 zoneId, uint32 lightId, uint32 fadeInTime)
{
    if (_zoneDynamicInfo.find(zoneId) == _zoneDynamicInfo.end())
        _zoneDynamicInfo.insert(ZoneDynamicInfoMap::value_type(zoneId, ZoneDynamicInfo()));

    ZoneDynamicInfo& info = _zoneDynamicInfo[zoneId];
    info.OverrideLightId = lightId;
    info.LightFadeInTime = fadeInTime;
    Map::PlayerList const& players = GetPlayers();

    if (!GetPlayers().empty())
    {
        WorldPacket data(SMSG_OVERRIDE_LIGHT, 4 + 4 + 1);
        data << uint32(_defaultLight);
        data << uint32(lightId);
        data << uint32(fadeInTime);

        for (Player& player : GetPlayers())
            if (player.GetZoneId() == zoneId)
                player.SendDirectMessage(data);
    }
}

void Map::UpdateAreaDependentAuras()
{
    for (Player& player : GetPlayers())
        if (player.IsInWorld())
        {
            player.UpdateAreaDependentAuras(player.GetAreaId());
            player.UpdateZoneDependentAuras(player.GetZoneId());
        }
}

VisibilityGroup Map::GetMapVisibilityGroup()
{
    if (!mapVisibilityGroup)
    {
        mapVisibilityGroup = MapVisibilityGroup::Create(this);
        for (Player& player : GetPlayers())
            if (player.IsInWorld())
                mapVisibilityGroup->AddPlayer(&player);

    }
    return mapVisibilityGroup;
}

VisibilityGroup Map::GetZoneVisibilityGroup(uint32 zone, bool create)
{
    auto itr = zoneVisibilityGroup.find(zone);
    if (create && itr == zoneVisibilityGroup.end())
    {
        auto zonegroup = ZoneVisibilityGroup::Create(this);
        for (Player& player : GetPlayers())
        {
            if (player.IsInWorld() && player.GetZoneId() == zone)
                zonegroup->AddPlayer(&player);
        }
        zoneVisibilityGroup.insert(std::make_pair(zone, zonegroup));
        return GetZoneVisibilityGroup(zone, false);
    }
    return (itr == zoneVisibilityGroup.end() ? VisibilityGroup() : itr->second);
}

void Map::AddToActiveHelper(WorldObject* obj)
{
    m_activeNonPlayers.insert(obj);
    MONITOR_MAPS(ReportActiveObject(this, true));
}

void Map::RemoveFromActiveHelper(WorldObject* obj)
{
    // Map::Update for active object in proccess
    if (m_activeNonPlayersIter != m_activeNonPlayers.end())
    {
        ActiveNonPlayers::iterator itr = m_activeNonPlayers.find(obj);
        if (itr == m_activeNonPlayers.end())
            return;
        if (itr == m_activeNonPlayersIter)
            ++m_activeNonPlayersIter;
        m_activeNonPlayers.erase(itr);
        MONITOR_MAPS(ReportActiveObject(this, false));
    }
    else
    {
        m_activeNonPlayers.erase(obj);
        MONITOR_MAPS(ReportActiveObject(this, false));
    }
}

void Map::AddToUpdateObjectList(Object* obj)
{
    updateObjects.insert(obj);
}

void Map::RemoveFromUpdateObjectList(Object* obj)
{
    updateObjects.erase(obj);
}

void Map::AddUnitToRelocateList(Unit& unit)
{
    if (unit.IsInRelocationList())
        return;
    unit.SetInRelocationList(true);
    const auto itr = relocatedUnits.find(&unit);
    if (itr == relocatedUnits.end())
        relocatedUnits.insert(&unit);
}

void Map::RemoveUnitFromRelocateList(Unit& unit)
{
    unit.SetInRelocationList(false);
    const auto itr = relocatedUnits.find(&unit);
    if (itr != relocatedUnits.end())
    {
        if (itr == relocatedUnitItr)
        {
            relocatedUnitItr = relocatedUnits.erase(itr);
            relocatedUnit = nullptr;
        }
        else if (itr != relocatedUnits.end())
            relocatedUnits.erase(itr);
    }
}
 
void Map::SummonCreature(uint32 entry, float x, float y, float z, SummonPropertiesEntry const* properties /*= NULL*/, uint32 duration /*= 0*/, Unit* summoner /*= NULL*/, uint32 spellId /*= 0*/, uint32 vehId /*= 0*/)
{
    Position pos;
    pos.m_positionX = x;
    pos.m_positionY = y;
    pos.m_positionZ = z;

    SummonCreature(entry, pos, properties, duration, summoner, spellId, vehId);
}

GameObject* Map::SummonGameObject(uint32 entry, Position pos, uint32 phaseMask, float rot0, float rot1, float rot2, float rot3)
{
    GameObject* obj = GameObject::Create(entry, *this, phaseMask, pos, G3D::Quat(0, 0, rot2, rot3), 100);
    if (!obj)
        return nullptr;

    obj->SetRespawnTime(0);
    obj->SetSpawnedByDefault(false);

    // Wild object not have owner and check clickable by players
    AddToMap(obj);

    return obj;
}

TempSummon* Map::SummonCreature(uint32 entry, Position const& pos, SummonPropertiesEntry const* properties /*= NULL*/, uint32 duration /*= 0*/, Unit* summoner /*= NULL*/, uint32 spellId /*= 0*/, uint32 vehId /*= 0*/)
{
    uint32 mask = UNIT_MASK_SUMMON;
    if (properties)
    {
        switch (properties->Category)
        {
        case SUMMON_CATEGORY_PET:
            mask = UNIT_MASK_GUARDIAN;
            break;
        case SUMMON_CATEGORY_PUPPET:
            mask = UNIT_MASK_PUPPET;
            break;
        case SUMMON_CATEGORY_VEHICLE:
            mask = UNIT_MASK_MINION;
            break;
        case SUMMON_CATEGORY_WILD:
        case SUMMON_CATEGORY_ALLY:
        case SUMMON_CATEGORY_UNK:
        {
            switch (properties->Type)
            {
            case SUMMON_TYPE_MINION:
            case SUMMON_TYPE_GUARDIAN:
            case SUMMON_TYPE_GUARDIAN2:
                mask = UNIT_MASK_GUARDIAN;
                break;
            case SUMMON_TYPE_TOTEM:
            case SUMMON_TYPE_LIGHTWELL:
                mask = UNIT_MASK_TOTEM;
                break;
            case SUMMON_TYPE_VEHICLE:
            case SUMMON_TYPE_VEHICLE2:
                mask = UNIT_MASK_SUMMON;
                break;
            case SUMMON_TYPE_MINIPET:
                mask = UNIT_MASK_MINION;
                break;
            default:
                if (properties->Flags & 512) // Mirror Image, Summon Gargoyle
                    mask = UNIT_MASK_GUARDIAN;
                break;
            }
            break;
        }
        default:
            return NULL;
        }
    }

    uint32 phase = PHASEMASK_NORMAL;
    if (summoner)
        phase = summoner->GetPhaseMask();

    TempSummon* summon = nullptr;
    switch (mask)
    {
    case UNIT_MASK_SUMMON:
        summon = TempSummon::Create(properties, summoner, entry, this, pos, phase, nullptr, vehId);
        break;
    case UNIT_MASK_GUARDIAN:
        summon = Guardian::Create(properties, summoner, entry, this, pos, phase, nullptr, vehId);
        break;
    case UNIT_MASK_PUPPET:
        summon = Puppet::Create(properties, summoner, entry, this, pos, phase, nullptr, vehId);
        break;
    case UNIT_MASK_TOTEM:
        summon = Totem::Create(properties, summoner, entry, this, pos, phase, nullptr, vehId);
        break;
    case UNIT_MASK_MINION:
        summon = Minion::Create(properties, summoner, entry, this, pos, phase, nullptr, vehId);
        break;
    }

    if (!summon)
        return nullptr;

    summon->SetUInt32Value(UNIT_CREATED_BY_SPELL, spellId);

    summon->SetHomePosition(pos);

    summon->InitStats(duration);
    AddToMap(summon->ToCreature());
    summon->InitSummon();

    // call MoveInLineOfSight for nearby creatures
    auto notifier = Trinity::createRelocationNotifier<Unit>(*summon);
    summon->VisitNearbyObject(GetVisibilityRange(), notifier);

    if (summoner)
        summoner->OnSummon(*summon);

    return summon;
}

/**
* Summons group of creatures.
*
* @param group Id of group to summon.
* @param list  List to store pointers to summoned creatures.
*/

void Map::SummonCreatureGroup(uint8 group, std::list<TempSummon*>* list /*= NULL*/)
{
    std::vector<TempSummonData> const* data = sObjectMgr->GetSummonGroup(GetId(), SUMMONER_TYPE_MAP, group);
    if (!data)
        return;

    for (std::vector<TempSummonData>::const_iterator itr = data->begin(); itr != data->end(); ++itr)
        if (TempSummon* summon = SummonCreature(itr->entry, itr->pos, NULL, itr->time))
            if (list)
                list->push_back(summon);
}

MapInstanced* Map::ToMapInstanced() { return To<MapInstanced>(); }
MapInstanced const* Map::ToMapInstanced() const { return To<MapInstanced>(); }

BattlegroundMap* Map::ToBattlegroundMap() { return To<BattlegroundMap>(); }
BattlegroundMap const* Map::ToBattlegroundMap() const { return To<BattlegroundMap>(); }

ArenaReplayMap* Map::ToArenaReplayMap() { return To<ArenaReplayMap>(); }
ArenaReplayMap const* Map::ToArenaReplayMap() const { return To<ArenaReplayMap>(); }
