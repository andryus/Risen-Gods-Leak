#pragma once

#include "Common.h"
#include "Timer.h"
#include <memory>
#include <unordered_set>
#include <boost/thread.hpp>
#include <boost/optional.hpp>
#include "ThreadPool.hpp"
#include "SharedPtr.hpp"

class Player;
class WorldObject;
class WorldPacket;
class Map;
class VisibilityGroupBase;

typedef util::shared_ptr<VisibilityGroupBase> VisibilityGroup;

typedef std::unordered_set<VisibilityGroupBase*> VisibilityGroupSet;

enum VisibilityTypes
{
    VISIBILITY_TYPE_OBJECT          = 0,
    VISIBILITY_TYPE_MAP             = 1,
    VISIBILITY_TYPE_ZONE            = 2
};

enum VisibilitySourceType
{
    VISIBILITY_SOURCE_TYPE_CREATURE_ENTRY        = 0,
    VISIBILITY_SOURCE_TYPE_CREATURE_GUID         = 1,
    VISIBILITY_SOURCE_TYPE_GAMEOBJECT_ENTRY      = 2,
    VISIBILITY_SOURCE_TYPE_GAMEOBJECT_GUID       = 3,
};

struct VisibilityInfo
{
    VisibilityTypes type;
    uint32 data;
    VisibilityInfo() : type(VISIBILITY_TYPE_OBJECT), data(0) { }
    VisibilityInfo(VisibilityTypes t, uint32 d) : type(t), data(d) { }
};

class ReplayData;

class VisibilityGroupBase
{
    public:
        void AddPlayer(Player* player);
        void RemovePlayer(Player* player);
        void AddObject(WorldObject* object);
        void RemoveObject(WorldObject* object);
        void BroadcastPacket(const WorldPacket& pck, WorldObject* sourceObj = nullptr);
        void BroadcastPacket(const WorldPacket& pck, Player const* skipped_rcvr, WorldObject* sourceObj = nullptr);
        template<typename Function>
        void DoForAllPlayer(Function&& operation)
        {
            for (auto itr : sessionMap)
                operation(itr);
        }
        bool HasPlayer(const Player* player);

        static void LoadVisibilityInfo();
        static const VisibilityInfo* GetVisibilityInfoFor(WorldObject* object);
        static VisibilityGroup GetOrCreateVisibilityGroupFor(WorldObject& obj, const VisibilityInfo* vInfo);
        virtual ~VisibilityGroupBase();
    protected:
        void Initialize(Map* base);
        const std::set<Player*>& GetPlayerSet() { return sessionMap; }
        VisibilityGroupBase();
        Map* base;
    private:
        static const VisibilityInfo* GetVisibilityInfo(VisibilitySourceType type, uint32 entry);
        template<typename Function>
        static void ProcessOperation(VisibilityGroup group, Function&& operation)
        {
            group->DoForAllPlayer(std::forward<Function>(operation));
        }
        std::set<Player*> sessionMap;
        std::unordered_set<WorldObject*> objectMap;
        static std::unordered_map<uint8, std::unordered_map<uint32, VisibilityInfo>> visiblityInfoMap;
        std::shared_ptr<ReplayData> replay;
};

class WorldObjectVisibilityGroup : public VisibilityGroupBase
{
    public:
        void Update(uint32 diff);
        void UpdateVisibilityFor(WorldObject* observer);
        static VisibilityGroup Create(WorldObject* owner, Map* base, float range = 0.f);
        WorldObjectVisibilityGroup(WorldObject* owner);
        ~WorldObjectVisibilityGroup();
    protected:
        WorldObject* owner;
    private:
        void SetLargeObject(bool value);
        bool isLargeObject;
        PeriodicTimer updateInterval;
};

class MapVisibilityGroup : public VisibilityGroupBase
{
    public:
        static VisibilityGroup Create(Map* base);
        MapVisibilityGroup();
};

class ZoneVisibilityGroup : public VisibilityGroupBase
{
    public:
        static VisibilityGroup Create(Map* base);
        ZoneVisibilityGroup();
};
