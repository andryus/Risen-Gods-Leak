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

#ifndef TRINITY_MAP_H
#define TRINITY_MAP_H

#include "Define.h"

#include "Scriptable.h"

#include "DBCStructure.h"
#include "GridDefines.h"
#include "Timer.h"
#include "SharedDefines.h"
#include "GridContainer.hpp"
#include "DynamicTree.h"
#include "GameObjectModel.h"
#include "Visibility.h"
#include "TerrainMap.hpp"

#include "Threading/ExecutionContext.hpp"
#include "Maps/MapObjectStore.hpp"

#include "UpdateTimer.hpp"

#include <bitset>
#include <list>

class Unit;
class WorldPacket;
class InstanceScript;
class Group;
class InstanceSave;
class Object;
class WorldObject;
class TempSummon;
class Player;
class CreatureGroup;
struct ScriptInfo;
struct ScriptAction;
struct Position;
class Battleground;
class MapInstanced;
class BattlegroundMap;
class InstanceMap;
class Transport;
class VisibilityGroupBase;
class ArenaReplayMap;
class ReplayEntry;
class ReplayData;
class MapArea;
struct CharacterNameData;
namespace Trinity { struct ObjectUpdater; }

struct ScriptAction
{
    ObjectGuid sourceGUID;
    ObjectGuid targetGUID;
    ObjectGuid ownerGUID;                                   ///> owner of source if source is item
    ScriptInfo const* script;                               ///> pointer to static script data
};

#pragma pack(push, 1)

struct InstanceTemplate
{
    uint32 Parent;
    std::string ScriptName;
    uint32 ScriptId;
    bool AllowMount;
};

enum LevelRequirementVsMode
{
    LEVELREQUIREMENT_HEROIC = 70
};

struct ZoneDynamicInfo
{
    ZoneDynamicInfo() : MusicId(0), WeatherId(0), WeatherGrade(0.0f),
        OverrideLightId(0), LightFadeInTime(0) { }

    uint32 MusicId;
    uint32 WeatherId;
    float WeatherGrade;
    uint32 OverrideLightId;
    uint32 LightFadeInTime;
};

#pragma pack(pop)

namespace impl
{
    template<class MapT>
    struct MapConvert
    {
        constexpr bool operator()(const Map& map) const
        {
            return false;
        }
    };

    template<>
    struct MapConvert<InstanceMap>
    {
        bool operator()(const Map& map) const;
    };

    template<>
    struct MapConvert<MapInstanced>
    {
        bool operator()(const Map& map) const;
    };

    template<>
    struct MapConvert<BattlegroundMap>
    {
        bool operator()(const Map& map) const;
    };

    template<>
    struct MapConvert<ArenaReplayMap>
    {
        bool operator()(const Map& map) const;
    };
}

template<class Target>
bool canCastMap(const Map& map)
{
    return impl::MapConvert<Target>()(map);
}


#define MAX_FALL_DISTANCE     250000.0f                     // "unlimited fall" to find VMap ground if it is available, just larger than MAX_HEIGHT - INVALID_HEIGHT
#define DEFAULT_HEIGHT_SEARCH     50.0f                     // default search distance to find height at nearby locations
#define MIN_UNLOAD_DELAY      1                             // immediate unload
#define UNIT_RELOCATION_CONTAINER_COUNT 20

typedef std::map<uint32/*leaderDBGUID*/, CreatureGroup*>        CreatureGroupHolderType;

typedef std::unordered_map<uint32 /*zoneId*/, ZoneDynamicInfo> ZoneDynamicInfoMap;

extern template class ExecutionContext<Map>;

class GAME_API Map : public ExecutionContext<Map>,
    public script::Scriptable
{
    friend class MapReference;
    public:
        Map(uint32 id, time_t, uint32 InstanceId, uint8 SpawnMode, Map* _parent = NULL);
        virtual ~Map();

        using OwnedPlayerContainer = std::unordered_map<ObjectGuid, std::shared_ptr<Player>>;

        /**
         * @brief Wrapper class for iterating through all player on a map. Players must not be removed from the map
         * while iterating though this list.
         */
        class PlayerList
        {
            OwnedPlayerContainer const& _list;

        public:
            PlayerList(OwnedPlayerContainer const& list) : _list(list) {}

            class const_iterator
            {
                OwnedPlayerContainer::const_iterator _itr;

            public:
                const_iterator(const OwnedPlayerContainer::const_iterator itr) : _itr(itr) {}

                Player& operator*() const { return *_itr->second; }
                Player* operator->() const { return _itr->second.get(); }

                bool operator==(const const_iterator& r) const { return (_itr == r._itr); }
                bool operator!=(const const_iterator& r) const { return (_itr != r._itr); }
                const_iterator& operator++() { ++_itr; return *this; }
            };

            /**
             * @brief Creates a copy of the underlying container.
             */
            OwnedPlayerContainer copy() const { return _list; }

            const_iterator begin() const { return const_iterator(_list.begin()); }
            const_iterator end() const { return const_iterator(_list.end()); }

            std::size_t size() const { return _list.size(); }
            bool empty() const { return _list.empty(); }
        };

        MapEntry const* GetEntry() const { return i_mapEntry; }

        // currently unused for normal maps
        bool CanUnload(uint32 diff)
        {
            if (_force_unload)
                return true;

            if (!m_unloadTimer)
                return false;

            if (m_unloadTimer <= diff)
                return true;

            m_unloadTimer -= diff;
            return false;
        }

        /**
         * Causes unload on next map update tick
         * \note Requires map to be already empty
         */
        bool ForceUnload();
        bool CanForceUnload();

        virtual bool AddPlayerToMap(std::shared_ptr<Player>);
        virtual void RemovePlayerFromMap(std::shared_ptr<Player>);
        template<class T> bool AddToMap(T *);
        template<class T> void RemoveFromMap(T *, bool);

        virtual void Update(const uint32);

        float GetDynamicVisibilityRange() const { return dynamicVisibilityDistance * m_VisibleDistance; }
        float GetVisibilityRange() const { return m_VisibleDistance; }
        //function for setting up visibility distance for maps on per-type/per-Id basis
        virtual void InitVisibilityDistance();

        void PlayerRelocation(Player*, float x, float y, float z, float orientation);
        void CreatureRelocation(Creature* creature, float x, float y, float z, float ang, bool respawnRelocationOnFail = true);
        void GameObjectRelocation(GameObject* go, float x, float y, float z, float orientation, bool respawnRelocationOnFail = true);
        void DynamicObjectRelocation(DynamicObject* go, float x, float y, float z, float orientation);

        bool IsGridLoaded(Grids::CellCoord const& coord) const;
        bool IsGridLoaded(float x, float y) const { return IsGridLoaded(Grids::CellCoord::FromPosition(x, y)); }
        void PreloadGrid(Grids::CellCoord const& coord);
        void EnsureGridLoaded(Grids::CellCoord const& coord);

        void LoadGrid(float x, float y) { EnsureGridLoaded(Grids::CellCoord::FromPosition(x, y)); }
        bool UnloadGrid(Grids::CellCoord const& coord);
        virtual void UnloadAll();

        uint32 GetId(void) const { return i_mapEntry->MapID; }

        static bool ExistMap(uint32 mapid, int gx, int gy);
        static bool ExistVMap(uint32 mapid, int gx, int gy);

        Map const* GetParent() const { return &parentMap; }

        // some calls like isInWater should not use vmaps due to processor power
        // can return INVALID_HEIGHT if under z+2 z coord not found height
        float GetHeight(float x, float y, float z, bool checkVMap = true, float maxSearchDist = DEFAULT_HEIGHT_SEARCH) const;
        float GetMinHeight(float x, float y) const;

        ZLiquidStatus getLiquidStatus(float x, float y, float z, uint8 ReqLiquidType, LiquidData* data = nullptr) const;

        MapArea* GetAreaForObject(const WorldObject& object) const;
        uint32 GetAreaId(float x, float y, float z, bool *isOutdoors) const;
        bool GetAreaInfo(float x, float y, float z, uint32& mogpflags, int32& adtId, int32& rootId, int32& groupId) const;
        uint32 GetAreaId(float x, float y, float z) const;
        uint32 GetZoneId(float x, float y, float z) const;
        void GetZoneAndAreaId(uint32& zoneid, uint32& areaid, float x, float y, float z) const;

        bool IsOutdoors(float x, float y, float z) const;

        uint8 GetTerrainType(float x, float y) const;
        float GetWaterLevel(float x, float y) const;
        bool IsInWater(float x, float y, float z, LiquidData* data = nullptr) const;
        bool IsUnderWater(float x, float y, float z) const;

        void MoveAllCreaturesInMoveList();
        void MoveAllGameObjectsInMoveList();
        void MoveAllDynamicObjectsInMoveList();
        void RemoveAllObjectsInRemoveList();
        virtual void RemoveAllPlayers();

        // used only in MoveAllCreaturesInMoveList and ObjectGridUnloader
        void OnCellCleanup(Creature* creature);
        void OnCellCleanup(GameObject* gameObject);
        void OnCellCleanup(DynamicObject* dynObject);
        void OnCellCleanup(Corpse* corpse);

        // assert print helper
        bool CheckGridIntegrity(Creature* c, bool moved) const;

        uint32 GetInstanceId() const { return i_InstanceId; }
        uint8 GetSpawnMode() const { return (i_spawnMode); }
        virtual bool CanEnter(Player* /*player*/);
        const char* GetMapName() const;

        // have meaning only for instanced map (that have set real difficulty)
        Difficulty GetDifficulty() const { return Difficulty(GetSpawnMode()); }
        bool IsRegularDifficulty() const { return GetDifficulty() == REGULAR_DIFFICULTY; }
        MapDifficulty const* GetMapDifficulty() const;

        bool Instanceable() const { return i_mapEntry && i_mapEntry->Instanceable(); }
        bool IsDungeon() const { return i_mapEntry && i_mapEntry->IsDungeon(); }
        bool IsNonRaidDungeon() const { return i_mapEntry && i_mapEntry->IsNonRaidDungeon(); }
        bool IsRaid() const { return i_mapEntry && i_mapEntry->IsRaid(); }
        bool IsRaidOrHeroicDungeon() const { return IsRaid() || i_spawnMode > DUNGEON_DIFFICULTY_NORMAL; }
        bool IsHeroic() const { return IsRaid() ? i_spawnMode >= RAID_DIFFICULTY_10MAN_HEROIC : i_spawnMode >= DUNGEON_DIFFICULTY_HEROIC; }
        bool Is25ManRaid() const { return IsRaid() && i_spawnMode & RAID_DIFFICULTY_MASK_25MAN; }   // since 25man difficulties are 1 and 3, we can check them like that
        bool IsBattleground() const { return !m_isReplayMap && i_mapEntry->IsBattleground(); }
        bool IsBattleArena() const { return !m_isReplayMap && i_mapEntry && i_mapEntry->IsBattleArena(); }
        bool IsBattlegroundOrArena() const { return !m_isReplayMap && i_mapEntry && i_mapEntry->IsBattlegroundOrArena(); }
        bool GetEntrancePos(int32 &mapid, float &x, float &y)
        {
            if (!i_mapEntry)
                return false;
            return i_mapEntry->GetEntrancePos(mapid, x, y);
        }

        void AddObjectToRemoveList(WorldObject* obj);
        virtual void DelayedUpdate(const uint32 diff);

        void UpdateObjectVisibility(WorldObject* obj);
        void UpdateObjectsVisibilityFor(Player* player);

        bool HavePlayers() const { return _players.size() > 0; }
        uint32 GetPlayersCountExceptGMs() const;

        void SendToPlayers(const WorldPacket& data) const;

        PlayerList GetPlayers() const { return PlayerList(_players); }

        //per-map script storage
        void ScriptsStart(std::map<uint32, std::multimap<uint32, ScriptInfo> > const& scripts, uint32 id, Object* source, Object* target);
        void ScriptCommandStart(ScriptInfo const& script, uint32 delay, Object* source, Object* target);

        // must called with AddToWorld
        template<class T>
        void AddToActive(T* obj);

        // must called with RemoveFromWorld
        template<class T>
        void RemoveFromActive(T* obj);

        template<class VisitorType> void Visit(const float &x, const float &y, const float& radius, VisitorType& visitor, bool loadGrids = false);
        template<class VisitorType> void VisitFirstFound(const float &x, const float &y, const float& radius, VisitorType& visitor, bool loadGrids = false);
        CreatureGroupHolderType CreatureGroupHolder;

        void SummonCreature(uint32 entry, float x, float y, float z, SummonPropertiesEntry const* properties = NULL, uint32 duration = 0, Unit* summoner = NULL, uint32 spellId = 0, uint32 vehId = 0);
        GameObject* SummonGameObject(uint32 entry, Position pos, uint32 phaseMask, float rot0, float rot1, float rot2, float rot3);
        TempSummon* SummonCreature(uint32 entry, Position const& pos, SummonPropertiesEntry const* properties = NULL, uint32 duration = 0, Unit* summoner = NULL, uint32 spellId = 0, uint32 vehId = 0);
        void SummonCreatureGroup(uint8 group, std::list<TempSummon*>* list = NULL);

        template<class Target>
        Target* To();
        template<class Target>
        const Target* To() const;

        MapInstanced* ToMapInstanced();
        MapInstanced const* ToMapInstanced() const;

        InstanceMap* ToInstanceMap();
        InstanceMap const* ToInstanceMap() const;

        BattlegroundMap* ToBattlegroundMap();
        BattlegroundMap const* ToBattlegroundMap() const;

        ArenaReplayMap* ToArenaReplayMap();
        ArenaReplayMap const* ToArenaReplayMap() const;

        float GetWaterOrGroundLevel(uint32 phasemask, float x, float y, float z, float* ground = NULL, bool swim = false) const;


        float GetHeight(uint32 phasemask, float x, float y, float z, bool vmap = true, float maxSearchDist = DEFAULT_HEIGHT_SEARCH) const;
        bool isInLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2, uint32 phasemask) const;
        void Balance() { _dynamicTree.balance(); }
        void RemoveGameObjectModel(const GameObjectModel& model) { _dynamicTree.remove(model); }
        void InsertGameObjectModel(const GameObjectModel& model) { _dynamicTree.insert(model); }
        bool ContainsGameObjectModel(const GameObjectModel& model) const { return _dynamicTree.contains(model);}
        bool getStaticObjectHitPos(float x1, float y1, float z1, float x2, float y2, float z2, float& rx, float &ry, float& rz, float modifyDist);
        bool getDynamicObjectHitPos(uint32 phasemask, float x1, float y1, float z1, float x2, float y2, float z2, float& rx, float &ry, float& rz, float modifyDist);

        /*
            RESPAWN TIMES
        */
        time_t GetLinkedRespawnTime(ObjectGuid guid) const;
        time_t GetCreatureRespawnTime(uint32 dbGuid) const
        {
            std::unordered_map<uint32 /*dbGUID*/, time_t>::const_iterator itr = _creatureRespawnTimes.find(dbGuid);
            if (itr != _creatureRespawnTimes.end())
                return itr->second;

            return time_t(0);
        }

        time_t GetGORespawnTime(uint32 dbGuid) const
        {
            std::unordered_map<uint32 /*dbGUID*/, time_t>::const_iterator itr = _goRespawnTimes.find(dbGuid);
            if (itr != _goRespawnTimes.end())
                return itr->second;

            return time_t(0);
        }

        void SaveCreatureRespawnTime(uint32 dbGuid, time_t respawnTime);
        void RemoveCreatureRespawnTime(uint32 dbGuid);
        void SaveGORespawnTime(uint32 dbGuid, time_t respawnTime);
        void RemoveGORespawnTime(uint32 dbGuid);
        void LoadRespawnTimes();
        void DeleteRespawnTimes();

        static void DeleteRespawnTimesInDB(uint16 mapId, uint32 instanceId);

        void SendZoneDynamicInfo(Player* player);

        void SetZoneMusic(uint32 zoneId, uint32 musicId);
        void SetZoneWeather(uint32 zoneId, uint32 weatherId, float weatherGrade);
        void SetZoneOverrideLight(uint32 zoneId, uint32 lightId, uint32 fadeInTime);

        void UpdateAreaDependentAuras();

        void ReloadMapConfig();

        bool IsPathfindingEnabled() const { return !_mmap_disabled; }

        VisibilityGroup GetMapVisibilityGroup();
        VisibilityGroup GetZoneVisibilityGroup(uint32 zone, bool create);

        /**
        * Adds an WorldObject to a list of objects, that changed its state.
        */
        void AddToUpdateObjectList(Object* obj);
        /**
        * Removes an WorldObject to a list of objects, that changed its state.
        */
        void RemoveFromUpdateObjectList(Object* obj);
        /**
         * Add a unit to the list of units that will start a relocation visitor within the next few update call. 
         */
        void AddUnitToRelocateList(Unit& unit);
        void RemoveUnitFromRelocateList(Unit& unit);

        bool IsReplayMap() const { return m_isReplayMap; }
        std::shared_ptr<ReplayData> GetReplayData() const { return replay; }
        void SetReplayData(std::shared_ptr<ReplayData> data) { replay = data; }

        virtual void OnWorldObjectAdded(WorldObject& object);
        virtual void OnWorldObjectRemoved(WorldObject& object);

    private:

        void WorldObjectAdded(WorldObject& object);

        template<class VisitorType> void VisitOnce(const float &x, const float &y, const float& radius, VisitorType& visitor, bool loadGrids = false);

        void LoadMapAndVMap(int gx, int gy);
        void LoadVMap(int gx, int gy);
        void LoadMap(int gx, int gy, bool reload = false);
        void LoadMMap(int gx, int gy);
        TerrainMap* GetGrid(float x, float y);
        void EnsureTerrainLoaded(Grids::CellCoord const& coord);

        void SendInitSelf(Player* player);

        template<class T> void InitializeObject(T* obj);
        void AddCreatureToMoveList(Creature* c, float x, float y, float z, float ang);
        void RemoveCreatureFromMoveList(Creature* c);
        void AddGameObjectToMoveList(GameObject* go, float x, float y, float z, float ang);
        void RemoveGameObjectFromMoveList(GameObject* go);
        void AddDynamicObjectToMoveList(DynamicObject* go, float x, float y, float z, float ang);
        void RemoveDynamicObjectFromMoveList(DynamicObject* go);

        bool CreatureRespawnRelocation(Creature* c, bool diffGridOnly);
        bool GameObjectRespawnRelocation(GameObject* go, bool diffGridOnly);

        bool _creatureToMoveLock;
        std::unordered_set<Creature*> _creaturesToMove;

        bool _gameObjectsToMoveLock;
        std::unordered_set<GameObject*> _gameObjectsToMove;

        bool _dynamicObjectsToMoveLock;
        std::unordered_set<DynamicObject*> _dynamicObjectsToMove;
        
        void ScriptsProcess();
    protected:
        std::mutex _mapLock;
        std::mutex _gridLock;

        MapEntry const* i_mapEntry;
        uint8 i_spawnMode;
        uint32 i_InstanceId;
        uint32 m_unloadTimer;
        float m_VisibleDistance;
        DynamicMapTree _dynamicTree;

        int32 m_VisibilityNotifyPeriod;

        typedef std::set<WorldObject*> ActiveNonPlayers;
        ActiveNonPlayers m_activeNonPlayers;
        ActiveNonPlayers::iterator m_activeNonPlayersIter;

        // Objects that must update even in inactive grids without activating them
        typedef std::set<Transport*> TransportsContainer;
        TransportsContainer _transports;
        TransportsContainer::iterator _transportsUpdateIter;

        bool m_isReplayMap;

    private:
        Player* _GetScriptPlayerSourceOrTarget(Object* source, Object* target, const ScriptInfo* scriptInfo) const;
        Creature* _GetScriptCreatureSourceOrTarget(Object* source, Object* target, const ScriptInfo* scriptInfo, bool bReverse = false) const;
        Unit* _GetScriptUnit(Object* obj, bool isSource, const ScriptInfo* scriptInfo) const;
        Player* _GetScriptPlayer(Object* obj, bool isSource, const ScriptInfo* scriptInfo) const;
        Creature* _GetScriptCreature(Object* obj, bool isSource, const ScriptInfo* scriptInfo) const;
        WorldObject* _GetScriptWorldObject(Object* obj, bool isSource, const ScriptInfo* scriptInfo) const;
        void _ScriptProcessDoor(Object* source, Object* target, const ScriptInfo* scriptInfo) const;
        GameObject* _FindGameObject(WorldObject* pWorldObject, uint32 guid) const;

        //used for fast base_map (e.g. MapInstanced class object) search for
        //InstanceMaps and BattlegroundMaps...
        Map& parentMap;

        Grids::GridMap gridMap;
        TerrainMap* terrainMap[MAX_NUMBER_OF_GRIDS][MAX_NUMBER_OF_GRIDS];

        //these functions used to process player/mob aggro reactions and
        //visibility calculations. Highly optimized for massive calculations
        void ProcessRelocationNotifies(const uint32 diff);

        bool i_scriptLock;
        std::set<WorldObject*> i_objectsToRemove;

        std::set<Object*> updateObjects;

        std::set<Unit*> relocatedUnits;
        std::set<Unit*>::iterator relocatedUnitItr;
        Unit* relocatedUnit;
        uint32 relocationContainerIndex;
        uint32 relocationTimer;

        typedef std::multimap<time_t, ScriptAction> ScriptScheduleMap;
        ScriptScheduleMap m_scriptSchedule;

        // Type specific code for add/remove to/from grid
        template<class T>
        bool AddToGrid(T* object, bool loadGrid = false);
        template<class T>
        bool AddToGrid(T* object, Grids::CellCoord const& pos, bool loadGrid = false);

        template<class T> 
        bool SwitchGridContainers(T* obj, bool loadGrid = false);
        template<class T> 
        bool SwitchGridContainers(T* obj, Grids::CellCoord const& pos, bool loadGrid = false);

        template<class T>
        void DeleteFromWorld(T*);

        void AddToActiveHelper(WorldObject* obj);
        void RemoveFromActiveHelper(WorldObject* obj);

        VisibilityGroup mapVisibilityGroup;
        std::unordered_map<uint32, VisibilityGroup> zoneVisibilityGroup;
        float dynamicVisibilityDistance;

        std::unordered_map<uint32 /*dbGUID*/, time_t> _creatureRespawnTimes;
        std::unordered_map<uint32 /*dbGUID*/, time_t> _goRespawnTimes;

        ZoneDynamicInfoMap _zoneDynamicInfo;
        uint32 _defaultLight;
        bool _force_unload;
        uint32 _vmap_disable_flags;
        bool _mmap_disabled;

        std::shared_ptr<ReplayData> replay;

    public:
        Player* GetPlayer(ObjectGuid const& guid);
        Creature* GetCreature(ObjectGuid const& guid);
        GameObject* GetGameObject(ObjectGuid const& guid); //< Note: Returns all types of GameObjects except MotionTransports. Use ObjectAccessor::GetMotionTransport() instead
        Transport* GetTransport(ObjectGuid const& guid);
        DynamicObject* GetDynamicObject(ObjectGuid const& guid);
        Corpse* GetCorpse(ObjectGuid const& guid);

        boost::optional<Creature&> GetCreatureBySpawnId(uint32 const spawnId) { return _objectStore.GetCreatureBySpawnId(spawnId); }
        boost::optional<GameObject&> GetGameObjectBySpawnId(uint32 const spawnId) { return _objectStore.GetGameObjectBySpawnId(spawnId); }

        auto GetCreatures() { return _objectStore.GetAll<Creature>(); }
        auto GetGameObjects() { return _objectStore.GetAll<GameObject>(); }

        auto GetCreaturesByEntry(uint32 const entry) { return _objectStore.GetCreaturesByEntry(entry); }
        auto GetGameObjectsByEntry(uint32 const entry) { return _objectStore.GetGameObjectsByEntry(entry); }

        template<typename T>
        void AddObject(T& obj) { _objectStore.Insert(obj); }

        template<typename T>
        void RemoveObject(T& obj) { _objectStore.Remove(obj); }

        UpdateTimer& GetDiffTimer() { return diffTimer; }

    protected:
        OwnedPlayerContainer _players;
    private:
        using AreaMap = std::unordered_map<uint32, std::shared_ptr<MapArea>>;

        MapObjectStore _objectStore{};

        UpdateTimer diffTimer;
        AreaMap areas;
        std::vector<WorldObject*> initObjects;

        std::string OnQueryOwnerName() const override;
        bool OnInitScript(std::string_view entry) override;
};

template<class Target>
Target* Map::To()
{
    if (canCastMap<Target>(*this))
        return static_cast<Target*>(this);

    return nullptr;
}

template<class Target>
const Target* Map::To() const
{
    if (canCastMap<Target>(*this))
        return static_cast<const Target*>(this);

    return nullptr;
}


class BattlegroundMap : public Map
{
    public:
        BattlegroundMap(uint32 id, time_t, uint32 InstanceId, Map* _parent, uint8 spawnMode);
        ~BattlegroundMap();

        bool AddPlayerToMap(std::shared_ptr<Player>) override;
        void RemovePlayerFromMap(std::shared_ptr<Player>) override;
        bool CanEnter(Player* player) override;
        void SetUnload();
        //void UnloadAll(bool pForce);
        void RemoveAllPlayers() override;

        virtual void InitVisibilityDistance() override;
        Battleground* GetBG() { return m_bg; }
        void SetBG(Battleground* bg) { m_bg = bg; }
    private:
        Battleground* m_bg;
};

class ArenaReplayMap : public Map
{
    public:
        ArenaReplayMap(uint32 id, time_t, uint32 InstanceId, Map* _parent, uint8 spawnMode);
        ~ArenaReplayMap();

        bool AddPlayerToMap(std::shared_ptr<Player>) override;
        void RemovePlayerFromMap(std::shared_ptr<Player>) override;
        void SetUnload();
        void Update(const uint32 diff) override;
        void StartReplay();
        void SetReplay(ReplayEntry* entry);
        std::shared_ptr<ReplayData> GetReplayData() { return replayData; }
        void SetReplaySpeed(float speed) { m_replaySpeed = speed; }

        void InitVisibilityDistance() override;

    private:
        ReplayEntry* replayEntry;
        std::shared_ptr<ReplayData> replayData;
        std::vector<std::pair<uint32, WorldPacket>>::const_iterator replayItr;
        uint32 m_curReplayTime;
        uint32 closeTimer;
        bool m_replaying;
        float m_replaySpeed;
};

template <class VisitorType>
void Map::VisitFirstFound(const float& x, const float& y, const float& radius, VisitorType& visitor, bool loadGrids)
{
    if (radius > SIZE_OF_GRIDS / 2.f)
    {
        Grids::OctagonCellArea area(x, y, radius);
        if (!loadGrids)
            area.SetNoGridLoad();
        gridMap.Visit(area, visitor);
    }
    else
    {
        Grids::SquareCellArea area(x, y, radius);
        if (!loadGrids)
            area.SetNoGridLoad();
        gridMap.Visit(area, visitor);
    }
}

template <class VisitorType>
void Map::Visit(const float& x, const float& y, const float& radius, VisitorType& visitor, bool loadGrids)
{
    if (radius > SIZE_OF_GRIDS / 2.f)
    {
        Grids::OctagonCellArea area(x, y, radius);
        if (!loadGrids)
            area.SetNoGridLoad();
        gridMap.Visit(area, visitor);
    }
    else
    {
        Grids::SquareCellArea area(x, y, radius);
        if (!loadGrids)
            area.SetNoGridLoad();
        gridMap.Visit(area, visitor);
    }
}

template <class VisitorType>
void Map::VisitOnce(const float& x, const float& y, const float& radius, VisitorType& visitor, bool loadGrids)
{
    if (radius > SIZE_OF_GRIDS / 2.f)
    {
        Grids::OctagonCellArea area(x, y, radius);
        if (!loadGrids)
            area.SetNoGridLoad();
        gridMap.VisitOnce(area, visitor);
    }
    else
    {
        Grids::SquareCellArea area(x, y, radius);
        if (!loadGrids)
            area.SetNoGridLoad();
        gridMap.VisitOnce(area, visitor);
    }
}

namespace impl
{
    template<class Notifier> // declaration from WorldObject.h
    void visitMap(float x, float y, Map& map, float radius, Notifier& notifier)
    {
        map.Visit(x, y, radius, notifier);
    }
}


#endif
