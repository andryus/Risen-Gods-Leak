#pragma once

#include "Object.h"

#include "SharedDefines.h"
#include "Visibility.h"
#include "MovementInfo.h"
#include "WorldLocation.h"
#include "Grids/Notifiers/GridSearchers.h"

#include "Scriptable.h"

constexpr float MAX_VISIBILITY_DISTANCE      = SIZE_OF_GRIDS; // max distance for visible objects
constexpr float DEFAULT_VISIBILITY_DISTANCE  = 90.0f; // default visible distance, 90 yards on continents
constexpr float DEFAULT_WORLD_OBJECT_SIZE    = 0.388999998569489f; // player size, also currently used (correctly?) for any non Unit world objects

enum TempSummonType
{
    TEMPSUMMON_TIMED_OR_DEAD_DESPAWN = 1,             // despawns after a specified time OR when the creature disappears
    TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN = 2,             // despawns after a specified time OR when the creature dies
    TEMPSUMMON_TIMED_DESPAWN = 3,             // despawns after a specified time
    TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT = 4,             // despawns after a specified time after the creature is out of combat
    TEMPSUMMON_CORPSE_DESPAWN = 5,             // despawns instantly after death
    TEMPSUMMON_CORPSE_TIMED_DESPAWN = 6,             // despawns after a specified time after death
    TEMPSUMMON_DEAD_DESPAWN = 7,             // despawns when the creature disappears
    TEMPSUMMON_MANUAL_DESPAWN = 8              // despawns when UnSummon() is called
};

enum PhaseMasks
{
    PHASEMASK_NORMAL = 0x00000001,
    PHASEMASK_ANYWHERE = 0xFFFFFFFF
};

template <class T_VALUES, class T_FLAGS, class FLAG_TYPE, uint8 ARRAY_SIZE>
class FlaggedValuesArray32
{
public:
    FlaggedValuesArray32()
    {
        memset(&m_values, 0x00, sizeof(T_VALUES) * ARRAY_SIZE);
        m_flags = 0;
    }

    T_FLAGS  GetFlags() const { return m_flags; }
    bool     HasFlag(FLAG_TYPE flag) const { return m_flags & (1 << flag); }
    void     AddFlag(FLAG_TYPE flag) { m_flags |= (1 << flag); }
    void     DelFlag(FLAG_TYPE flag) { m_flags &= ~(1 << flag); }

    T_VALUES GetValue(FLAG_TYPE flag) const { return m_values[flag]; }
    void     SetValue(FLAG_TYPE flag, T_VALUES value) { m_values[flag] = value; }
    void     AddValue(FLAG_TYPE flag, T_VALUES value) { m_values[flag] += value; }

private:
    T_VALUES m_values[ARRAY_SIZE];
    T_FLAGS m_flags;
};


class Map;
class InstanceMap;
class Encounter;
class CreatureAI;
class TempSummon;
class Transport;
class MotionTransport;
class WorldPacket;
class ZoneScript;
class InstanceScript;


namespace impl
{
    template<class Notifier> // implemented in Map.h (to remove include)
    void visitMap(float x, float y, Map& map, float radius, Notifier& notifier);
}

class GAME_API WorldObject : public Object, public script::Scriptable, public WorldLocation
{
protected:
    explicit WorldObject();
public:
    virtual ~WorldObject();

    virtual void Update(uint32 /*time_diff*/) { }

    void _Create(uint32 guidlow, HighGuid guidhigh, uint32 phaseMask);
    virtual void RemoveFromWorld() override;

    void SetEncounter(std::weak_ptr<Encounter>&& encounter);
    Encounter* GetEncounter() const;

    void GetNearPoint2D(float &x, float &y, float distance, float absAngle) const;
    void GetNearPoint(WorldObject const* searcher, float &x, float &y, float &z, float searcher_size, float distance2d, float absAngle) const;
    void GetClosePoint(float &x, float &y, float &z, float size, float distance2d = 0, float angle = 0) const;
    void MovePosition(Position &pos, float dist, float angle) const;
    void GetNearPosition(Position &pos, float dist, float angle) const;
    /**
    * @brief This function moves a given position step - by - step until the given maximum distance is reached, a
    * collision occured or a single step exceeds the maximum height difference.
    * @param startPosition origin position (global coords when on transport)
    * @param distance maxium distance the position will be moved(in xy-coordinates)
    * @param orientation direction the position will be moved
    * @param stepSize distance for height checks(default parameter for this value will select the parameter automatically)
    * @param maxHeightDiff maximum height difference between steps(default parameter for this value will choose the
    * parameter automatically)
    */
    Position MovePositionToFirstCollision2D(const Position& startPosition, const float distance, const float orientation) const;
    /**
    * @brief This function moves a given position step-by-step until the given maximum distance is reached, a
    * collision occured or a single step exceeds the maximum height difference.
    * @param startPosition origin position (global coords when on transport)
    * @param distance maxium distance the position will be moved (in xyz-coordinates)
    * @param orientation direction the position will be moved
    * @param stepSize distance for height checks (default parameter for this value will select the parameter automatically)
    * @param maxHeightDiff maximum height difference between steps (default parameter for this value will choose the
    * parameter automatically)
    */
    Position MovePositionToFirstCollision3D(const Position& startPosition, const float distance, const float orientation) const;
    /**
     * @brief Returns the position at the first collision with an object or terrain when moving in a given direction while 
     * starting at the position of this object. A steep slope is also considered a collision for this function.
     * @param angle direction relative to the orientation of this object 
     * @param dist maxium distance the position will be moved (in xy-coordinates)
     */
    Position GetFirstCollisionPosition(const float dist, const float angle) const { return MovePositionToFirstCollision2D(*this, dist, angle + GetOrientation()); }
    void GetRandomNearPosition(Position &pos, float radius);
    void GetContactPoint(WorldObject const* obj, float &x, float &y, float &z, float distance2d = CONTACT_DISTANCE) const;
    void GetCombatPosition(Unit const* searcher, float &x, float &y, float &z, float absAngle, bool predictMovement, float basedist = -1.f);

    float GetObjectSize() const;
    void UpdateGroundPositionZ(float x, float y, float &z) const;
    /**
    * Calculates a valid z-coordinate for this object by looking for a nearby ground layer. Flying units will be
    * placed above the ground, ground inhabiting objects will be placed upon the surface.
    * @param x x-coordinate of check position
    * @param y y-coordinate of check position
    * @param z original height for searching nearby ground layer. This parameter will be overwritten by the
    * calculated result height.
    */
    void UpdateAllowedPositionZ(float x, float y, float &z) const;

    void GetRandomPoint(Position const &srcPos, float distance, float &rand_x, float &rand_y, float &rand_z) const;
    void GetRandomPoint(Position const &srcPos, float distance, Position &pos) const;

    uint32 GetInstanceId() const { return m_InstanceId; }

    virtual void SetPhaseMask(uint32 newPhaseMask, bool update);
    uint32 GetPhaseMask() const { return m_phaseMask; }
    bool InSamePhase(WorldObject const* obj) const;
    bool InSamePhase(uint32 phasemask) const { return (GetPhaseMask() & phasemask) != 0; }

    uint32 GetZoneId() const;
    uint32 GetAreaId() const;
    void GetZoneAndAreaId(uint32& zoneid, uint32& areaid) const;

    InstanceMap* GetInstance() const;
    InstanceScript* GetInstanceScript() const;

    const std::string& GetName() const { return m_name; }
    void SetName(std::string_view newname) { m_name = std::move(newname); }

    virtual std::string_view GetNameForLocaleIdx(LocaleConstant /*locale_idx*/) const { return m_name; }

    float GetDistance(WorldObject const* obj) const;
    float GetDistance(Position const &pos) const;
    float GetDistance(float x, float y, float z) const;
    float GetDistance2d(WorldObject const* obj) const;
    float GetDistance2d(float x, float y) const;
    float GetDistanceZ(WorldObject const* obj) const;

    bool IsSelfOrInSameMap(WorldObject const* obj) const;
    bool IsInMap(WorldObject const* obj) const;
    bool IsWithinDist3d(float x, float y, float z, float dist) const;
    bool IsWithinDist3d(Position const* pos, float dist) const;
    bool IsWithinDist2d(float x, float y, float dist) const;
    bool IsWithinDist2d(Position const* pos, float dist) const;
    // use only if you will sure about placing both object at same map
    bool IsWithinDist(WorldObject const* obj, float dist2compare, bool is3D = true) const;
    bool IsWithinDistInMap(WorldObject const* obj, float dist2compare, bool is3D = true) const;
    bool IsWithinLOS(float x, float y, float z) const;
    bool IsWithinLOSInMap(WorldObject const* obj) const;
    bool GetDistanceOrder(WorldObject const* obj1, WorldObject const* obj2, bool is3D = true) const;
    bool IsInRange(WorldObject const* obj, float minRange, float maxRange, bool is3D = true) const;
    bool IsInRange2d(float x, float y, float minRange, float maxRange) const;
    bool IsInRange3d(float x, float y, float z, float minRange, float maxRange) const;
    bool isInFront(WorldObject const* target, float arc = float(M_PI)) const;
    bool isInBack(WorldObject const* target, float arc = float(M_PI)) const;
    bool HasInLine(const WorldObject* target, float width) const;

    bool IsInBetween(WorldObject const* obj1, WorldObject const* obj2, float size = 0) const;

    virtual void CleanupsBeforeDelete(bool finalCleanup = true);  // used in destructor or explicitly before mass creature delete to remove cross-references to already deleted units

    virtual void SendMessageToSet(const WorldPacket& data, bool self);
    virtual void SendMessageToSetInRange(const WorldPacket& data, float dist, bool self);
    virtual void SendMessageToSet(const WorldPacket& data, const Player* skipped_rcvr);

    virtual uint8 getLevelForTarget(WorldObject const* /*target*/) const { return 1; }

    void MonsterSay(const char* text, uint32 language, WorldObject const* target);
    void MonsterYell(const char* text, uint32 language, WorldObject const* target);
    void MonsterTextEmote(const char* text, WorldObject const* target, bool IsBossEmote = false);
    void MonsterWhisper(const char* text, Player const* target, bool IsBossWhisper = false);
    void MonsterSay(int32 textId, uint32 language, WorldObject const* target);
    void MonsterYell(int32 textId, uint32 language, WorldObject const* target);
    void MonsterTextEmote(int32 textId, WorldObject const* target, bool IsBossEmote = false);
    void MonsterWhisper(int32 textId, Player const* target, bool IsBossWhisper = false);

    void PlayDistanceSound(uint32 sound_id, Player* target = NULL);
    void PlayDirectSound(uint32 sound_id, Player* target = NULL);

    void SendObjectDeSpawnAnim(ObjectGuid guid);

    virtual void SaveRespawnTime() { }
    void AddObjectToRemoveList();

    void ScheduleObjectUpdate() override;
    void AbortObjectUpdate() override;

    float GetGridActivationRange() const;
    float GetVisibilityRange() const;
    float GetSightRange(WorldObject const* target = NULL) const;
    bool CanSeeOrDetect(WorldObject const* obj, bool ignoreStealth = false, bool distanceCheck = false, bool checkAlert = false) const;
    bool CanDetectInvisibilityOf(WorldObject const* obj) const;

    VisibilityGroup GetVisibilityGroup() const { return visibilityGroup; }
    void UpdateVisibilityGroup();

    FlaggedValuesArray32<int32, uint32, StealthType, TOTAL_STEALTH_TYPES> m_stealth;
    FlaggedValuesArray32<int32, uint32, StealthType, TOTAL_STEALTH_TYPES> m_stealthDetect;

    FlaggedValuesArray32<int32, uint32, InvisibilityType, TOTAL_INVISIBILITY_TYPES> m_invisibility;
    FlaggedValuesArray32<int32, uint32, InvisibilityType, TOTAL_INVISIBILITY_TYPES> m_invisibilityDetect;

    FlaggedValuesArray32<int32, uint32, ServerSideVisibilityType, TOTAL_SERVERSIDE_VISIBILITY_TYPES> m_serverSideVisibility;
    FlaggedValuesArray32<int32, uint32, ServerSideVisibilityType, TOTAL_SERVERSIDE_VISIBILITY_TYPES> m_serverSideVisibilityDetect;

    // Low Level Packets
    void SendPlaySound(uint32 Sound, bool OnlySelf);

    virtual void SetMap(Map* map);
    virtual void ResetMap();
    Map* GetMap() const;
    Map* FindMap() const { return m_currMap; }
    //used to check all object's GetMap() calls when object is not in world!

    //this function should be removed in nearest time...
    Map const* GetBaseMap() const;

    void SetZoneScript();
    ZoneScript* GetZoneScript() const { return m_zoneScript; }

    TempSummon* SummonCreature(uint32 id, Position const &pos, TempSummonType spwtype = TEMPSUMMON_MANUAL_DESPAWN, uint32 despwtime = 0, uint32 vehId = 0) const;
    TempSummon* SummonCreature(uint32 id, float x, float y, float z, float ang = 0, TempSummonType spwtype = TEMPSUMMON_MANUAL_DESPAWN, uint32 despwtime = 0) const;
    GameObject* SummonGameObject(uint32 entry, float x, float y, float z, float ang, float rotation0, float rotation1, float rotation2, float rotation3, uint32 respawnTime /* s */);
    Creature*   SummonTrigger(float x, float y, float z, float ang, uint32 dur, CreatureAI* (*GetAI)(Creature*) = NULL);
    void SummonCreatureGroup(uint8 group, std::list<TempSummon*>* list = NULL);

    Creature*   FindNearestCreature(uint32 entry, float range, bool alive = true) const;
    GameObject* FindNearestGameObject(uint32 entry, float range) const;
    GameObject* FindNearestGameObjectOfType(GameobjectTypes type, float range) const;
    Player*     FindNearestPlayer(float distance) const;

    void GetGameObjectListWithEntryInGrid(std::list<GameObject*>& lList, uint32 uiEntry = 0, float fMaxSearchRange = 250.0f) const;
    void GetCreatureListWithEntryInGrid(std::list<Creature*>& lList, uint32 uiEntry = 0, float fMaxSearchRange = 250.0f) const;
    void GetPlayerListInGrid(std::list<Player*>& lList, float fMaxSearchRange = 250.0f, bool alive = true) const;

    virtual void UpdateObjectVisibility(bool forced = true);
    virtual void UpdateObjectVisibilityOnCreate()
    {
        UpdateObjectVisibility(true);
    }

    void BroadcastValuesUpdate() override;

    bool isActiveObject() const { return m_isActive; }
    void setActive(bool isActiveObject);

    template<class Notifier>
    void VisitMap(float radius, Notifier& notifier) const;
    template<class Notifier>
    void VisitNearbyObject(float radius, Notifier& notifier) const;

    template<class ObjectType, bool TakeFirst = false, class Check>
    ObjectType* VisitSingleNearbyObject(float radius, Check& check) const;

    template<class ObjectType, template<class> class Action, class TargetArg, typename... Checks>
    void VisitAnyNearbyObject(float radius, TargetArg& target, Checks&... checks) const;

    uint32  LastUsedScriptID;

    // Transports
    Transport* GetTransport() const { return m_transport; }
    MotionTransport* GetMotionTransport() const;
    float GetTransOffsetX() const { return m_movementInfo.transport.pos.GetPositionX(); }
    float GetTransOffsetY() const { return m_movementInfo.transport.pos.GetPositionY(); }
    float GetTransOffsetZ() const { return m_movementInfo.transport.pos.GetPositionZ(); }
    float GetTransOffsetO() const { return m_movementInfo.transport.pos.GetOrientation(); }
    uint32 GetTransTime()   const { return m_movementInfo.transport.time; }
    int8 GetTransSeat()     const { return m_movementInfo.transport.seat; }
    virtual ObjectGuid GetTransGUID() const;
    void SetTransport(Transport* t) { m_transport = t; }

    MovementInfo m_movementInfo;

    virtual float GetStationaryX() const { return GetPositionX(); }
    virtual float GetStationaryY() const { return GetPositionY(); }
    virtual float GetStationaryZ() const { return GetPositionZ(); }
    virtual float GetStationaryO() const { return GetOrientation(); }

    float VisibilityRange;
    void SetUpdateVisibilityGroup(WorldObjectVisibilityGroup* grp) { updateVisibilityGroup = grp; }

    static std::string PrintGUID(std::string_view name, ObjectGuid guid);

protected:
    std::string m_name;
    bool m_isActive;
    ZoneScript* m_zoneScript;

    // transports
    Transport* m_transport;

    //these functions are used mostly for Relocate() and Corpse/Player specific stuff...
    //use them ONLY in LoadFromDB()/Create() funcs and nowhere else!
    //mapId/instanceId should be set in SetMap() function!
    void SetLocationMapId(uint32 _mapId) { m_mapId = _mapId; }
    void SetLocationInstanceId(uint32 _instanceId) { m_InstanceId = _instanceId; }

    void UpdateVisibilityGroup(const uint32 diff) const;

    virtual bool IsNeverVisible() const { return !IsInWorld(); }
    virtual bool IsAlwaysVisibleFor(WorldObject const* /*seer*/) const { return false; }
    virtual bool IsInvisibleDueToDespawn() const { return false; }
    //difference from IsAlwaysVisibleFor: 1. after distance check; 2. use owner or charmer as seer
    virtual bool IsAlwaysDetectableFor(WorldObject const* /*seer*/) const { return false; }

    void ApplyEncounterTo(WorldObject& object);
private:
    Position _MovePositionToFirstCollision(const Position& startPosition, float distance, const float orientation, const bool is3D) const;

    std::string OnQueryOwnerName() const override final;
    std::string OnQueryFullName() const override final;

    bool OnInitScript(std::string_view scriptName) override;

    Map * m_currMap;                                    //current object's Map location

                                                        //uint32 m_mapId;                                     // object at map with map_id
    uint32 m_InstanceId;                                // in map copy with instance id
    uint32 m_phaseMask;                                 // in area phase state

    virtual bool _IsWithinDist(WorldObject const* obj, float dist2compare, bool is3D) const;

    bool CanNeverSee(WorldObject const* obj) const;
    virtual bool CanAlwaysSee(WorldObject const* /*obj*/) const { return false; }
    bool CanDetect(WorldObject const* obj, bool ignoreStealth, bool checkAlert = false) const;
    bool CanDetectStealthOf(WorldObject const* obj, bool checkAlert = false) const;

    WorldObjectVisibilityGroup* updateVisibilityGroup;
    VisibilityGroup visibilityGroup;

    
    std::weak_ptr<Encounter> encounter;

};




template<class Notifier>
void WorldObject::VisitMap(float radius, Notifier& notifier) const
{
    if (IsInWorld())
        impl::visitMap(GetPositionX(), GetPositionY(), *GetMap(), radius, notifier);
}

template<class Notifier>
void WorldObject::VisitNearbyObject(float radius, Notifier& notifier) const
{
    static_assert(Trinity::VisitorAcceptsObjectType<Notifier, WorldObject>, "Notifier doesn't accept any object-type (due to Visit-signature or explicit AcceptsObjectType-method.");

    if (IsInWorld())
        VisitMap(radius, notifier);
}

// todo: rework, probably make different function calls
template<class ObjectType, bool TakeFirst, class Check>
ObjectType* WorldObject::VisitSingleNearbyObject(float radius, Check& check) const
{
    ObjectType* outObject = nullptr;

    if constexpr(TakeFirst)
    {
        auto notifier = Trinity::createBaseObjectSearcher<ObjectType, Trinity::FirstAction>(this, outObject, check);
        VisitMap(radius, notifier);

        outObject = notifier.action.object;
    }
    else
    {
        auto notifier = Trinity::createBaseObjectSearcher<ObjectType, Trinity::LastAction>(this, outObject, check);
        VisitMap(radius, notifier);

        outObject = notifier.action.object;
    }

    return outObject;
}

template<class ObjectType, template<class> class Action, class TargetArg, typename... Checks>
void WorldObject::VisitAnyNearbyObject(float radius, TargetArg& target, Checks&... checks) const
{
    auto notifier = Trinity::createBaseObjectSearcher<ObjectType, Action>(this, target, checks...); // todo: use correct action
    VisitMap(radius, notifier);
}




namespace Trinity
{
    // Binary predicate to sort WorldObjects based on the distance to a reference WorldObject
    class ObjectDistanceOrderPred
    {
    public:
        ObjectDistanceOrderPred(WorldObject const* pRefObj, bool ascending = true) : m_refObj(pRefObj), m_ascending(ascending) { }
        bool operator()(WorldObject const* pLeft, WorldObject const* pRight) const
        {
            return m_ascending ? m_refObj->GetDistanceOrder(pLeft, pRight) : !m_refObj->GetDistanceOrder(pLeft, pRight);
        }
    private:
        WorldObject const* m_refObj;
        const bool m_ascending;
    };
}
