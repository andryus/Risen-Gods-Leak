#include "WorldObject.h"
#include "InstanceMap.h"
#include "Corpse.h"
#include "Transport.h"
#include "Player.h"
#include "Creature.h"
#include "Chat.h"
#include "ObjectAccessor.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "ThreadsafetyDebugger.h"
#include "BattlefieldMgr.h"
#include "OutdoorPvPMgr.h"
#include "ArenaReplaySystem.h"
#include "Packets/WorldPacket.h"
#include <G3D/LineSegment.h>

#include "API/LoadScript.h"
#include "UnitHooks.h"
#include "IVMapManager.h"

constexpr float SIGHT_RANGE_UNIT = 50.0f;

WorldObject::WorldObject() : WorldLocation(), LastUsedScriptID(0), m_name(""), m_isActive(false),
m_zoneScript(NULL), m_transport(NULL), m_currMap(NULL), m_InstanceId(0), updateVisibilityGroup(nullptr),
m_phaseMask(PHASEMASK_NORMAL), VisibilityRange(0.f)
{
    m_serverSideVisibility.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_ALIVE | GHOST_VISIBILITY_GHOST);
    m_serverSideVisibilityDetect.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_ALIVE);
}

WorldObject::~WorldObject()
{
    if (m_currMap)
        ResetMap();
}

void WorldObject::setActive(bool on)
{
    if (m_isActive == on)
        return;

    if (GetTypeId() == TYPEID_PLAYER)
        return;

    m_isActive = on;

    if (!IsInWorld())
        return;

    Map* map = FindMap();
    if (!map)
        return;

    if (on)
    {
        if (GetTypeId() == TYPEID_UNIT)
            map->AddToActive(this->ToCreature());
        else if (GetTypeId() == TYPEID_DYNAMICOBJECT)
            map->AddToActive((DynamicObject*)this);
        else if (GetTypeId() == TYPEID_GAMEOBJECT)
            map->AddToActive((GameObject*)this);
    }
    else
    {
        if (GetTypeId() == TYPEID_UNIT)
            map->RemoveFromActive(this->ToCreature());
        else if (GetTypeId() == TYPEID_DYNAMICOBJECT)
            map->RemoveFromActive((DynamicObject*)this);
        else if (GetTypeId() == TYPEID_GAMEOBJECT)
            map->RemoveFromActive((GameObject*)this);
    }
}

void WorldObject::CleanupsBeforeDelete(bool finalCleanup)
{
    if (finalCleanup)
        unloadScript(*this);

    if (IsInWorld())
        RemoveFromWorld();

    if (Transport* transport = GetTransport())
        transport->RemovePassenger(this);
}

void WorldObject::_Create(uint32 guidlow, HighGuid guidhigh, uint32 phaseMask)
{
    Object::_Create(guidlow, 0, guidhigh);
    m_phaseMask = phaseMask;
}

void WorldObject::RemoveFromWorld()
{
    if (!IsInWorld())
        return;

    if (visibilityGroup)
    {
        visibilityGroup->RemoveObject(this);
        visibilityGroup = VisibilityGroup();
    }

    Object::RemoveFromWorld();
}

void WorldObject::SetEncounter(std::weak_ptr<Encounter>&& encounter)
{
    this->encounter = std::move(encounter);
}

Encounter* WorldObject::GetEncounter() const
{
    return encounter.lock().get();
}

uint32 WorldObject::GetZoneId() const
{
    return GetBaseMap()->GetZoneId(m_positionX, m_positionY, m_positionZ);
}

uint32 WorldObject::GetAreaId() const
{
    return GetBaseMap()->GetAreaId(m_positionX, m_positionY, m_positionZ);
}

void WorldObject::GetZoneAndAreaId(uint32& zoneid, uint32& areaid) const
{
    GetBaseMap()->GetZoneAndAreaId(zoneid, areaid, m_positionX, m_positionY, m_positionZ);
}

InstanceMap* WorldObject::GetInstance() const
{
    Map* map = GetMap();
    return map->ToInstanceMap();
}

InstanceScript* WorldObject::GetInstanceScript() const
{
    InstanceMap* instance = GetInstance();

    return instance ? instance->GetInstanceScript() : nullptr;
}

float WorldObject::GetDistanceZ(const WorldObject* obj) const
{
    float dz = std::fabs(GetPositionZ() - obj->GetPositionZ());
    float sizefactor = GetObjectSize() + obj->GetObjectSize();
    float dist = dz - sizefactor;
    return (dist > 0 ? dist : 0);
}

void WorldObject::UpdateVisibilityGroup(const uint32 diff) const
{
    if (updateVisibilityGroup)
        updateVisibilityGroup->Update(diff);
}

bool WorldObject::_IsWithinDist(WorldObject const* obj, float dist2compare, bool is3D) const
{
    float sizefactor = GetObjectSize() + obj->GetObjectSize();
    float maxdist = dist2compare + sizefactor;

    if (GetTransport() && obj->GetTransport() && obj->GetTransport()->GetGUID().GetCounter() == GetTransport()->GetGUID().GetCounter())
    {
        float dtx = m_movementInfo.transport.pos.m_positionX - obj->m_movementInfo.transport.pos.m_positionX;
        float dty = m_movementInfo.transport.pos.m_positionY - obj->m_movementInfo.transport.pos.m_positionY;
        float disttsq = dtx * dtx + dty * dty;
        if (is3D)
        {
            float dtz = m_movementInfo.transport.pos.m_positionZ - obj->m_movementInfo.transport.pos.m_positionZ;
            disttsq += dtz * dtz;
        }
        return disttsq < (maxdist * maxdist);
    }

    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float distsq = dx * dx + dy * dy;
    if (is3D)
    {
        float dz = GetPositionZ() - obj->GetPositionZ();
        distsq += dz * dz;
    }

    return distsq < maxdist * maxdist;
}

bool WorldObject::IsWithinLOSInMap(const WorldObject* obj) const
{
    if (!IsInMap(obj))
        return false;

    if (obj->GetTypeId() == TYPEID_UNIT)
        if (obj->GetEntry() == 36980) // Ice Tomb 
            return true;

    if (GetTypeId() == TYPEID_UNIT) // Ice Tomb
        if (GetEntry() == 36980)
            return true;

    float ox, oy, oz;
    obj->GetPosition(ox, oy, oz);
    return IsWithinLOS(ox, oy, oz);
}

float WorldObject::GetDistance(const WorldObject* obj) const
{
    float d = GetExactDist(obj) - GetObjectSize() - obj->GetObjectSize();
    return d > 0.0f ? d : 0.0f;
}

float WorldObject::GetDistance(const Position &pos) const
{
    float d = GetExactDist(&pos) - GetObjectSize();
    return d > 0.0f ? d : 0.0f;
}

float WorldObject::GetDistance(float x, float y, float z) const
{
    float d = GetExactDist(x, y, z) - GetObjectSize();
    return d > 0.0f ? d : 0.0f;
}

float WorldObject::GetDistance2d(const WorldObject* obj) const
{
    float d = GetExactDist2d(obj) - GetObjectSize() - obj->GetObjectSize();
    return d > 0.0f ? d : 0.0f;
}

float WorldObject::GetDistance2d(float x, float y) const
{
    float d = GetExactDist2d(x, y) - GetObjectSize();
    return d > 0.0f ? d : 0.0f;
}

bool WorldObject::IsSelfOrInSameMap(const WorldObject* obj) const
{
    if (this == obj)
        return true;
    return IsInMap(obj);
}

bool WorldObject::IsInMap(const WorldObject* obj) const
{
    if (obj)
        return IsInWorld() && obj->IsInWorld() && (GetMap() == obj->GetMap());
    return false;
}

bool WorldObject::IsWithinDist3d(float x, float y, float z, float dist) const
{
    return IsInDist(x, y, z, dist + GetObjectSize());
}

bool WorldObject::IsWithinDist3d(const Position* pos, float dist) const
{
    return IsInDist(pos, dist + GetObjectSize());
}

bool WorldObject::IsWithinDist2d(float x, float y, float dist) const
{
    return IsInDist2d(x, y, dist + GetObjectSize());
}

bool WorldObject::IsWithinDist2d(const Position* pos, float dist) const
{
    return IsInDist2d(pos, dist + GetObjectSize());
}

bool WorldObject::IsWithinDist(WorldObject const* obj, float dist2compare, bool is3D /*= true*/) const
{
    return obj && _IsWithinDist(obj, dist2compare, is3D);
}

bool WorldObject::IsWithinDistInMap(WorldObject const* obj, float dist2compare, bool is3D /*= true*/) const
{
    return obj && IsInMap(obj) && InSamePhase(obj) && _IsWithinDist(obj, dist2compare, is3D);
}

bool WorldObject::IsWithinLOS(float ox, float oy, float oz) const
{
    if (IsInWorld())
        return GetMap()->isInLineOfSight(GetPositionX(), GetPositionY(), GetPositionZ() + 2.f, ox, oy, oz + 2.f, GetPhaseMask());

    return true;
}

bool WorldObject::GetDistanceOrder(WorldObject const* obj1, WorldObject const* obj2, bool is3D /* = true */) const
{
    const float dx1 = GetPositionX() - obj1->GetPositionX();
    const float dy1 = GetPositionY() - obj1->GetPositionY();
    float distsq1 = math::distance2dSq(dx1, dy1);
    if (is3D)
    {
        float dz1 = GetPositionZ() - obj1->GetPositionZ();
        distsq1 += math::square(dz1);
    }

    const float dx2 = GetPositionX() - obj2->GetPositionX();
    const float dy2 = GetPositionY() - obj2->GetPositionY();
    float distsq2 = math::distance2dSq(dx2, dy2);
    if (is3D)
    {
        const float dz2 = GetPositionZ() - obj2->GetPositionZ();
        distsq2 += math::square(dz2);
    }

    return distsq1 < distsq2;
}

bool WorldObject::IsInRange(WorldObject const* obj, float minRange, float maxRange, bool is3D /* = true */) const
{
    const float dx = GetPositionX() - obj->GetPositionX();
    const float dy = GetPositionY() - obj->GetPositionY();
    float distsq = math::distance2dSq(dx, dy);
    if (is3D)
    {
        float dz = GetPositionZ() - obj->GetPositionZ();
        distsq += math::square(dz);
    }

    const float sizefactor = GetObjectSize() + obj->GetObjectSize();

    // check only for real range
    if (minRange > 0.0f)
    {
        float mindist = minRange + sizefactor;
        if (distsq < mindist * mindist)
            return false;
    }

    float maxdist = maxRange + sizefactor;
    return distsq < maxdist * maxdist;
}

bool WorldObject::IsInRange2d(float x, float y, float minRange, float maxRange) const
{
    const float dx = GetPositionX() - x;
    const float dy = GetPositionY() - y;
    float distsq = math::distance2dSq(dx, dy);

    const float sizefactor = GetObjectSize();

    // check only for real range
    if (minRange > 0.0f)
    {
        const float mindist = minRange + sizefactor;
        if (distsq < mindist * mindist)
            return false;
    }

    const float maxdist = maxRange + sizefactor;
    return distsq < maxdist * maxdist;
}

bool WorldObject::IsInRange3d(float x, float y, float z, float minRange, float maxRange) const
{
    const float dx = GetPositionX() - x;
    const float dy = GetPositionY() - y;
    const float dz = GetPositionZ() - z;
    const float distsq = math::distance3dSq(dx, dy, dz);

    const float sizefactor = GetObjectSize();

    // check only for real range
    if (minRange > 0.0f)
    {
        const float mindist = minRange + sizefactor;
        if (distsq < mindist * mindist)
            return false;
    }

    const float maxdist = maxRange + sizefactor;
    return distsq < maxdist * maxdist;
}
bool WorldObject::IsInBetween(const WorldObject* obj1, const WorldObject* obj2, float size) const
{
    if (!obj1 || !obj2)
        return false;

    float dist = GetExactDist2d(obj1->GetPositionX(), obj1->GetPositionY());

    // not using sqrt() for performance
    if ((dist * dist) >= obj1->GetExactDist2dSq(obj2->GetPositionX(), obj2->GetPositionY()))
        return false;

    if (!size)
        size = GetObjectSize() / 2;

    float angle = obj1->GetAngle(obj2);

    // not using sqrt() for performance
    return (size * size) >= GetExactDist2dSq(obj1->GetPositionX() + std::cos(angle) * dist, obj1->GetPositionY() + std::sin(angle) * dist);
}

bool WorldObject::isInFront(WorldObject const* target, float arc) const
{
    return HasInArc(arc, target);
}

bool WorldObject::isInBack(WorldObject const* target, float arc) const
{
    return !HasInArc(2 * float(M_PI) - arc, target);
}

void WorldObject::GetRandomPoint(const Position &pos, float distance, float &rand_x, float &rand_y, float &rand_z) const
{
    if (!distance)
    {
        pos.GetPosition(rand_x, rand_y, rand_z);
        return;
    }

    // angle to face `obj` to `this`
    float angle = (float)rand_norm()*static_cast<float>(2 * M_PI);
    float new_dist = (float)rand_norm() + (float)rand_norm();
    new_dist = distance * (new_dist > 1 ? new_dist - 2 : new_dist);

    rand_x = pos.m_positionX + new_dist * std::cos(angle);
    rand_y = pos.m_positionY + new_dist * std::sin(angle);
    rand_z = pos.m_positionZ;

    Trinity::NormalizeMapCoord(rand_x);
    Trinity::NormalizeMapCoord(rand_y);
    UpdateGroundPositionZ(rand_x, rand_y, rand_z);            // update to LOS height if available
}

void WorldObject::GetRandomPoint(const Position &srcPos, float distance, Position &pos) const
{
    float x, y, z;
    GetRandomPoint(srcPos, distance, x, y, z);
    pos.Relocate(x, y, z, GetOrientation());
}

void WorldObject::UpdateGroundPositionZ(float x, float y, float &z) const
{
    float new_z = GetMap()->GetHeight(GetPhaseMask(), x, y, z + 2.0f, true);
    if (new_z > INVALID_HEIGHT)
        z = new_z + 0.05f;                                   // just to be sure that we are not a few pixel under the surface
}

void WorldObject::UpdateAllowedPositionZ(float x, float y, float &z) const
{
    // TODO: Allow transports to be part of dynamic vmap tree
    if (auto transport = GetTransport())
    {
        if ((ToCreature() && ToCreature()->CanFly()) || (ToPlayer() && ToPlayer()->CanFly()))
            return;

        z = transport->GetModelCollisionHeight(x, y, z).first;
        return;
    }

    switch (GetTypeId())
    {
    case TYPEID_UNIT:
    {
        // non fly unit don't must be in air
        // non swim unit must be at ground (mostly speedup, because it don't must be in water and water level check less fast
        if (!ToCreature()->CanFly())
        {
            bool canSwim = ToCreature()->CanSwim();
            float ground_z = z;
            float max_z = canSwim
                ? GetMap()->GetWaterOrGroundLevel(GetPhaseMask(), x, y, z, &ground_z, !ToUnit()->HasAuraType(SPELL_AURA_WATER_WALK))
                : ((ground_z = GetMap()->GetHeight(GetPhaseMask(), x, y, z, true)));
            if (max_z > INVALID_HEIGHT)
            {
                if (z > max_z)
                    z = max_z;
                else if (z < ground_z)
                    z = ground_z;
            }
        }
        else
        {
            float ground_z = GetMap()->GetHeight(GetPhaseMask(), x, y, z, true);
            if (z < ground_z)
                z = ground_z;
        }
        break;
    }
    case TYPEID_PLAYER:
    {
        // for server controlled moves playr work same as creature (but it can always swim)
        if (!ToPlayer()->CanFly())
        {
            float ground_z = z;
            float max_z = GetMap()->GetWaterOrGroundLevel(GetPhaseMask(), x, y, z, &ground_z, !ToUnit()->HasAuraType(SPELL_AURA_WATER_WALK));
            if (max_z > INVALID_HEIGHT)
            {
                if (z > max_z)
                    z = max_z;
                else if (z < ground_z)
                    z = ground_z;
            }
        }
        else
        {
            float ground_z = GetMap()->GetHeight(GetPhaseMask(), x, y, z, true);
            if (z < ground_z)
                z = ground_z;
        }
        break;
    }
    default:
    {
        float ground_z = GetMap()->GetHeight(GetPhaseMask(), x, y, z, true);
        if (ground_z > INVALID_HEIGHT)
            z = ground_z;
        break;
    }
    }
}

bool WorldObject::HasInLine(const WorldObject* target, float width) const
{
    if (!HasInArc(float(M_PI), target))
        return false;
    width += target->GetObjectSize();
    float angle = GetRelativeAngle(target);
    return std::fabs(std::sin(angle)) * GetExactDist2d(target->GetPositionX(), target->GetPositionY()) < width;
}
float WorldObject::GetGridActivationRange() const
{
    if (ToPlayer())
        return GetMap()->GetDynamicVisibilityRange();
    else if (ToCreature())
        return ToCreature()->m_SightDistance;
    else
        return 0.0f;
}

float WorldObject::GetVisibilityRange() const
{
    if (VisibilityRange)
        return VisibilityRange;
    if (isActiveObject() && !ToPlayer())
        return MAX_VISIBILITY_DISTANCE;
    if (GameObject const* game_object = ToGameObject())
        return GetMap()->GetVisibilityRange()* sWorld->getRate(RATE_VISIBILITY_GAME_OBJECTS);
    else
    {
        float mod;
        if (auto unitTarget = ToCreature())
        {
            mod = unitTarget->GetCombatReach() / DEFAULT_COMBAT_REACH + 1.f;
            mod -= sWorld->getFloatConfig(CONFIG_DYNAMIC_VISIBILITY_LARGE_OBJECT_THRESHOLD);
            mod = std::max(mod, 1.f);
            mod = std::min(mod, sWorld->getFloatConfig(CONFIG_DYNAMIC_VISIBILITY_MAX_OBJECT_VISIBILITY));
        }
        else
            mod = 1.f;
        return GetMap()->GetDynamicVisibilityRange()* mod;
    }
}

float WorldObject::GetSightRange(const WorldObject* target) const
{
    if (ToUnit())
    {
        if (ToPlayer())
        {
            if (target && target->isActiveObject() && !target->ToPlayer())
                return MAX_VISIBILITY_DISTANCE;
            else
                return GetMap()->GetDynamicVisibilityRange();
        }
        else if (ToCreature())
            return ToCreature()->m_SightDistance;
        else
            return SIGHT_RANGE_UNIT;
    }

    return 0.0f;
}

bool WorldObject::CanSeeOrDetect(WorldObject const* obj, bool ignoreStealth, bool distanceCheck, bool checkAlert) const
{
    if (this == obj)
        return true;

    if (obj->IsNeverVisible() || CanNeverSee(obj))
        return false;

    if (obj->IsAlwaysVisibleFor(this) || CanAlwaysSee(obj))
        return true;

    bool corpseVisibility = false;
    if (distanceCheck)
    {
        bool corpseCheck = false;
        if (Player const* thisPlayer = ToPlayer())
        {
            if (thisPlayer->isDead() && thisPlayer->GetHealth() > 0 && // Cheap way to check for ghost state
                !(obj->m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GHOST) & m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GHOST) & GHOST_VISIBILITY_GHOST))
            {
                if (Corpse* corpse = thisPlayer->GetCorpse())
                {
                    corpseCheck = true;
                    if (corpse->IsWithinDist(thisPlayer, obj->GetVisibilityRange(), false))
                        if (corpse->IsWithinDist(obj, obj->GetVisibilityRange(), false))
                            corpseVisibility = true;
                }
            }
        }

        WorldObject const* viewpoint = this;
        if (Player const* player = this->ToPlayer())
            viewpoint = player->GetViewpoint();

        if (!viewpoint)
            viewpoint = this;

        if (!corpseCheck && !viewpoint->IsWithinDist(obj, obj->GetVisibilityRange(), false))
            return false;
    }

    // GM visibility off or hidden NPC
    if (!obj->m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GM))
    {
        // Stop checking other things for GMs
        if (m_serverSideVisibilityDetect.GetValue(SERVERSIDE_VISIBILITY_GM))
            return true;
    }
    else
        return m_serverSideVisibilityDetect.GetValue(SERVERSIDE_VISIBILITY_GM) >= obj->m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GM);

    // Ghost players, Spirit Healers, and some other NPCs
    if (!corpseVisibility && !(obj->m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GHOST) & m_serverSideVisibilityDetect.GetValue(SERVERSIDE_VISIBILITY_GHOST)))
    {
        // Alive players can see dead players in some cases, but other objects can't do that
        if (Player const* thisPlayer = ToPlayer())
        {
            if (Player const* objPlayer = obj->ToPlayer())
            {
                if (thisPlayer->GetTeam() != objPlayer->GetTeam() || !thisPlayer->IsGroupVisibleFor(objPlayer))
                    return false;
            }
            else
                return false;
        }
        else
            return false;
    }

    // Traps can only be detected within melee distance
    if (const GameObject* go = obj->ToGameObject())
    {
        // Only stealthed traps(such like hunter traps) can't be seen from player
        if (go->GetGoType() == GAMEOBJECT_TYPE_TRAP && go->GetGOInfo()->trap.stealthed == 1)
        {
            if (const Player* player = ToPlayer())
            {
                if (Unit* owner = go->GetOwner())
                {
                    // Friendly players should also see the traps.
                    if (owner->IsFriendlyTo(player))
                        return true;
                    else if (player->getClass() != CLASS_ROGUE) // Unfriendly players except rogues are never allowed to see traps
                        return false;

                    // Rogues can see traps in melee range. Higher chance if they have detect traps active
                    if (obj->IsWithinDist(this, player->HasAura(2836) ? 12.0f : 4.0f, false))
                        return true;

                    return false;
                }
            }
        }
    }

    if (obj->IsInvisibleDueToDespawn())
        return false;

    if (!CanDetect(obj, ignoreStealth, checkAlert))
        return false;

    return true;
}

void WorldObject::UpdateVisibilityGroup()
{
    ASSERT(IsInWorld());

    VisibilityGroup group = VisibilityGroupBase::GetOrCreateVisibilityGroupFor(*this, VisibilityGroupBase::GetVisibilityInfoFor(this));

    if (visibilityGroup)
        visibilityGroup->RemoveObject(this);
    visibilityGroup = group;
    group->AddObject(this);
}

bool WorldObject::CanNeverSee(WorldObject const* obj) const
{
    return GetMap() != obj->GetMap() || !InSamePhase(obj);
}

bool WorldObject::CanDetect(WorldObject const* obj, bool ignoreStealth, bool checkAlert) const
{
    const WorldObject* seer = this;

    // Pets don't have detection, they use the detection of their masters
    if (Unit const* thisUnit = ToUnit())
        if (Unit* controller = thisUnit->GetCharmerOrOwner())
            seer = controller;

    if (obj->IsAlwaysDetectableFor(seer))
        return true;

    if (!ignoreStealth && !seer->CanDetectInvisibilityOf(obj))
        return false;

    if (!ignoreStealth && !seer->CanDetectStealthOf(obj, checkAlert))
        return false;

    return true;
}

bool WorldObject::CanDetectInvisibilityOf(WorldObject const* obj) const
{
    uint32 mask = obj->m_invisibility.GetFlags() & m_invisibilityDetect.GetFlags();

    // Check for not detected types
    if (mask != obj->m_invisibility.GetFlags())
        return false;

    // It isn't possible in invisibility to detect something that can't detect the invisible object
    // (it's at least true for spell: 66)
    // It seems like that only Units are affected by this check (couldn't see arena doors with preparation invisibility)
    if (obj->ToUnit())
        if ((m_invisibility.GetFlags() & obj->m_invisibilityDetect.GetFlags()) != m_invisibility.GetFlags())
            return false;

    for (uint32 i = 0; i < TOTAL_INVISIBILITY_TYPES; ++i)
    {
        if (!(mask & (1 << i)))
            continue;

        int32 objInvisibilityValue = obj->m_invisibility.GetValue(InvisibilityType(i));
        int32 ownInvisibilityDetectValue = m_invisibilityDetect.GetValue(InvisibilityType(i));

        // Too low value to detect
        if (ownInvisibilityDetectValue < objInvisibilityValue)
            return false;
    }

    return true;
}

bool WorldObject::CanDetectStealthOf(WorldObject const* obj, bool checkAlert) const
{
    // Combat reach is the minimal distance (both in front and behind),
    //   and it is also used in the range calculation.
    // One stealth point increases the visibility range by 0.3 yard.

    if (!obj->m_stealth.GetFlags())
        return true;

    float distance = GetExactDist(obj);
    float combatReach = 0.0f;

    Unit const* unit = ToUnit();
    if (unit)
        combatReach = unit->GetCombatReach();

    if (distance < combatReach)
        return true;

    if (!HasInArc(float(M_PI), obj))
        return false;

    GameObject const* go = ToGameObject();
    for (uint32 i = 0; i < TOTAL_STEALTH_TYPES; ++i)
    {
        if (!(obj->m_stealth.GetFlags() & (1 << i)))
            continue;

        if (unit && unit->HasAuraTypeWithMiscvalue(SPELL_AURA_DETECT_STEALTH, i))
            return true;

        // Starting points
        int32 detectionValue = 30;

        // Level difference: 5 point / level, starting from level 1.
        // There may be spells for this and the starting points too, but
        // not in the DBCs of the client.
        detectionValue += int32(getLevelForTarget(obj) - 1) * 5;

        // Apply modifiers
        detectionValue += m_stealthDetect.GetValue(StealthType(i));
        if (go)
            if (Unit* owner = go->GetOwner())
                detectionValue -= int32(owner->getLevelForTarget(this) - 1) * 5;

        detectionValue -= obj->m_stealth.GetValue(StealthType(i));

        // Calculate max distance
        float visibilityRange = float(detectionValue) * 0.3f + combatReach;

        // If this unit is an NPC then player detect range doesn't apply
        if (unit && unit->GetTypeId() == TYPEID_PLAYER && visibilityRange > MAX_PLAYER_STEALTH_DETECT_RANGE)
            visibilityRange = MAX_PLAYER_STEALTH_DETECT_RANGE;

        // When checking for alert state, look 8% further, and then 1.5 yards more than that.
        if (checkAlert)
            visibilityRange += (visibilityRange * 0.08f) + 1.5f;

        // If checking for alert, and creature's visibility range is greater than aggro distance, No alert
        Unit const* tunit = obj->ToUnit();
        if (checkAlert && unit && unit->ToCreature() && visibilityRange >= unit->ToCreature()->GetAttackDistance(tunit) + unit->ToCreature()->m_CombatDistance)
            return false;

        if (distance > visibilityRange)
            return false;
    }

    return true;
}

void WorldObject::SendPlaySound(uint32 Sound, bool OnlySelf)
{
    WorldPacket data(SMSG_PLAY_SOUND, 4);
    data << Sound;
    if (OnlySelf && GetTypeId() == TYPEID_PLAYER)
        this->ToPlayer()->SendDirectMessage(std::move(data));
    else
        SendMessageToSet(data, true); // ToSelf ignored in this case
}

namespace Trinity
{
    class MonsterChatBuilder
    {
    public:
        MonsterChatBuilder(WorldObject const* obj, ChatMsg msgtype, int32 textId, uint32 language, WorldObject const* target)
            : i_object(obj), i_msgtype(msgtype), i_textId(textId), i_language(Language(language)), i_target(target) { }
        void operator()(WorldPacket& data, LocaleConstant loc_idx)
        {
            char const* text = sObjectMgr->GetTrinityString(i_textId, loc_idx);
            ChatHandler::BuildChatPacket(data, i_msgtype, i_language, i_object, i_target, text, 0, "", loc_idx);
        }

    private:
        WorldObject const* i_object;
        ChatMsg i_msgtype;
        int32 i_textId;
        Language i_language;
        WorldObject const* i_target;
    };

    class MonsterCustomChatBuilder
    {
    public:
        MonsterCustomChatBuilder(WorldObject const* obj, ChatMsg msgtype, const char* text, uint32 language, WorldObject const* target)
            : i_object(obj), i_msgtype(msgtype), i_text(text), i_language(Language(language)), i_target(target)
        {}
        void operator()(WorldPacket& data, LocaleConstant loc_idx)
        {
            ChatHandler::BuildChatPacket(data, i_msgtype, i_language, i_object, i_target, i_text, 0, "", loc_idx);
        }

    private:
        WorldObject const* i_object;
        ChatMsg i_msgtype;
        const char* i_text;
        Language i_language;
        WorldObject const* i_target;
    };
}                                                           // namespace Trinity

void WorldObject::MonsterSay(const char* text, uint32 language, WorldObject const* target)
{
    Trinity::MonsterCustomChatBuilder say_build(this, CHAT_MSG_MONSTER_SAY, text, language, target);
    Trinity::LocalizedPacketDo<Trinity::MonsterCustomChatBuilder> say_do(say_build);
    Trinity::ObjectDistCheck check(this, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY));

    VisitAnyNearbyObject<Player, Trinity::FunctionAction>(sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY), say_do, check);
}

void WorldObject::MonsterSay(int32 textId, uint32 language, WorldObject const* target)
{
    Trinity::MonsterChatBuilder say_build(this, CHAT_MSG_MONSTER_SAY, textId, language, target);
    Trinity::LocalizedPacketDo<Trinity::MonsterChatBuilder> say_do(say_build);
    Trinity::ObjectDistCheck check(this, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY));

    VisitAnyNearbyObject<Player, Trinity::FunctionAction>(sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY), say_do, check);
}

void WorldObject::MonsterYell(const char* text, uint32 language, WorldObject const* target)
{
    Trinity::MonsterCustomChatBuilder say_build(this, CHAT_MSG_MONSTER_YELL, text, language, target);
    Trinity::LocalizedPacketDo<Trinity::MonsterCustomChatBuilder> say_do(say_build);
    Trinity::ObjectDistCheck check(this, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_YELL));

    VisitAnyNearbyObject<Player, Trinity::FunctionAction>(sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_YELL), say_do);
}

void WorldObject::MonsterYell(int32 textId, uint32 language, WorldObject const* target)
{
    Trinity::MonsterChatBuilder say_build(this, CHAT_MSG_MONSTER_YELL, textId, language, target);
    Trinity::LocalizedPacketDo<Trinity::MonsterChatBuilder> say_do(say_build);
    Trinity::ObjectDistCheck check(this, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_YELL));

    VisitAnyNearbyObject<Player, Trinity::FunctionAction>(sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_YELL), say_do);
}

void WorldObject::MonsterTextEmote(const char* text, WorldObject const* target, bool IsBossEmote)
{
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, IsBossEmote ? CHAT_MSG_RAID_BOSS_EMOTE : CHAT_MSG_MONSTER_EMOTE, LANG_UNIVERSAL,
        this, target, text);
    SendMessageToSetInRange(data, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_TEXTEMOTE), true);
}

void WorldObject::MonsterTextEmote(int32 textId, WorldObject const* target, bool IsBossEmote)
{
    Trinity::MonsterChatBuilder say_build(this, IsBossEmote ? CHAT_MSG_RAID_BOSS_EMOTE : CHAT_MSG_MONSTER_EMOTE, textId, LANG_UNIVERSAL, target);
    Trinity::LocalizedPacketDo<Trinity::MonsterChatBuilder> say_do(say_build);
    Trinity::ObjectDistCheck check(this, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_TEXTEMOTE));

    VisitAnyNearbyObject<Player, Trinity::FunctionAction>(sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_TEXTEMOTE), say_do);
}

void WorldObject::MonsterWhisper(const char* text, Player const* target, bool IsBossWhisper)
{
    if (!target)
        return;

    LocaleConstant loc_idx = target->GetSession()->GetSessionDbLocaleIndex();
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, IsBossWhisper ? CHAT_MSG_RAID_BOSS_WHISPER : CHAT_MSG_MONSTER_WHISPER, LANG_UNIVERSAL, this, target, text, 0, "", loc_idx);
    target->SendDirectMessage(std::move(data));
}

void WorldObject::MonsterWhisper(int32 textId, Player const* target, bool IsBossWhisper)
{
    if (!target)
        return;

    LocaleConstant loc_idx = target->GetSession()->GetSessionDbLocaleIndex();
    char const* text = sObjectMgr->GetTrinityString(textId, loc_idx);
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, IsBossWhisper ? CHAT_MSG_RAID_BOSS_WHISPER : CHAT_MSG_MONSTER_WHISPER, LANG_UNIVERSAL, this, target, text, 0, "", loc_idx);

    target->SendDirectMessage(std::move(data));
}

void WorldObject::SendMessageToSet(const WorldPacket& data, bool self)
{
    if (IsInWorld())
        visibilityGroup->BroadcastPacket(data);
}

void WorldObject::SendMessageToSetInRange(const WorldPacket& data, float dist, bool /*self*/)
{
    Trinity::MessageDistDeliverer notifier(this, data, dist);
    VisitNearbyObject(dist, notifier);
}

void WorldObject::SendMessageToSet(const WorldPacket& data, Player const* skipped_rcvr)
{
    if (IsInWorld())
        visibilityGroup->BroadcastPacket(data, skipped_rcvr);
}

void WorldObject::SendObjectDeSpawnAnim(ObjectGuid guid)
{
    WorldPacket data(SMSG_GAMEOBJECT_DESPAWN_ANIM, 8);
    data << uint64(guid);
    SendMessageToSet(data, true);
}

void WorldObject::SetMap(Map* map)
{
    ASSERT(map);
    ASSERT(!IsInWorld() || GetTypeId() == TYPEID_CORPSE);
    if (m_currMap == map) // command add npc: first create, than loadfromdb
        return;
    if (m_currMap)
    {
        TC_LOG_ERROR("misc", "WorldObject::SetMap: obj %s new map %u %u, old map %u %u", GetGUID().ToString().c_str(), map->GetId(), map->GetInstanceId(), m_currMap->GetId(), m_currMap->GetInstanceId());

        if (m_currMap->GetId() != map->GetId() || m_currMap->GetInstanceId() != map->GetInstanceId())
            ABORT();
    }
    m_currMap = map;
    m_mapId = map->GetId();
    m_InstanceId = map->GetInstanceId();
}

void WorldObject::ResetMap()
{
    ASSERT(m_currMap);
    ASSERT(!IsInWorld());
    m_currMap = NULL;
    //maybe not for corpse
    //m_mapId = 0;
    //m_InstanceId = 0;
}

Map* WorldObject::GetMap() const
{
    ASSERT(m_currMap);
    ThreadsafetyDebugger::AssertAllowed(ThreadsafetyDebugger::Context::NEEDS_MAP, m_currMap->GetId());
    return m_currMap;
}

Map const* WorldObject::GetBaseMap() const
{
    ASSERT(m_currMap);
    return m_currMap->GetParent();
}

void WorldObject::AddObjectToRemoveList()
{
    ASSERT(m_uint32Values);

    Map* map = FindMap();
    if (!map)
    {
        TC_LOG_ERROR("misc", "Object (TypeId: %hhu Entry: %u GUID: %u) at attempt add to move list not have valid map (Id: %u).", GetTypeId(), GetEntry(), GetGUID().GetCounter(), GetMapId());
        return;
    }

    map->AddObjectToRemoveList(this);
}

void WorldObject::ScheduleObjectUpdate()
{
    if (IsInWorld())
        GetMap()->AddToUpdateObjectList(this);
}

void WorldObject::AbortObjectUpdate()
{
    if (IsInWorld())
        GetMap()->RemoveFromUpdateObjectList(this);
}


void WorldObject::SetZoneScript()
{
    if (Map* map = FindMap())
    {
        if (map->IsDungeon())
            m_zoneScript = (ZoneScript*)((InstanceMap*)map)->GetInstanceScript();
        else if (!map->IsBattlegroundOrArena())
        {
            if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(GetZoneId()))
                m_zoneScript = bf;
            else
                m_zoneScript = sOutdoorPvPMgr->GetZoneScript(GetZoneId());
        }
    }
}

TempSummon* WorldObject::SummonCreature(uint32 entry, const Position &pos, TempSummonType spwtype, uint32 duration, uint32 /*vehId*/) const
{
    if (Map* map = FindMap())
    {
        if (TempSummon* summon = map->SummonCreature(entry, pos, NULL, duration, isType(TYPEMASK_UNIT) ? (Unit*)this : NULL))
        {
            summon->SetTempSummonType(spwtype);

            return summon;
        }
    }

    return NULL;
}

TempSummon* WorldObject::SummonCreature(uint32 id, float x, float y, float z, float ang /*= 0*/, TempSummonType spwtype /*= TEMPSUMMON_MANUAL_DESPAWN*/, uint32 despwtime /*= 0*/) const
{
    if (!x && !y && !z)
    {
        GetClosePoint(x, y, z, GetObjectSize());
        ang = GetOrientation();
    }
    Position pos;
    pos.Relocate(x, y, z, ang);
    return SummonCreature(id, pos, spwtype, despwtime, 0);
}

GameObject* WorldObject::SummonGameObject(uint32 entry, float x, float y, float z, float ang, float rotation0, float rotation1, float rotation2, float rotation3, uint32 respawnTime)
{
    if (!IsInWorld())
        return NULL;

    GameObjectTemplate const* goinfo = sObjectMgr->GetGameObjectTemplate(entry);
    if (!goinfo)
    {
        TC_LOG_ERROR("sql.sql", "Gameobject template %u not found in database!", entry);
        return NULL;
    }

    Map* map = GetMap();
    G3D::Quat rot{ rotation0, rotation1, rotation2, rotation3 };
    GameObject* go = GameObject::Create(entry, *map, GetPhaseMask(), Position(x, y, z, ang), rot, 100);
    if (!go)
        return nullptr;

    go->SetRespawnTime(respawnTime);
    if (GetTypeId() == TYPEID_PLAYER || GetTypeId() == TYPEID_UNIT) //not sure how to handle this
        ToUnit()->AddGameObject(go);
    else
        go->SetSpawnedByDefault(false);

    map->AddToMap(go);

    *this <<= Fire::SummonGO.Bind(go);

    return go;
}

Creature* WorldObject::SummonTrigger(float x, float y, float z, float ang, uint32 duration, CreatureAI* (*GetAI)(Creature*))
{
    TempSummonType summonType = (duration == 0) ? TEMPSUMMON_DEAD_DESPAWN : TEMPSUMMON_TIMED_DESPAWN;
    Creature* summon = SummonCreature(WORLD_TRIGGER, x, y, z, ang, summonType, duration);
    if (!summon)
        return nullptr;

    //summon->SetName(GetName());
    if (GetTypeId() == TYPEID_PLAYER || GetTypeId() == TYPEID_UNIT)
    {
        summon->setFaction(((Unit*)this)->getFaction());
        summon->SetLevel(((Unit*)this)->getLevel());
    }

    if (GetAI)
        summon->AIM_Initialize(GetAI(summon));
    return summon;
}

/**
* Summons group of creatures. Should be called only by instances of Creature and GameObject classes.
*
* @param group Id of group to summon.
* @param list  List to store pointers to summoned creatures.
*/
void WorldObject::SummonCreatureGroup(uint8 group, std::list<TempSummon*>* list /*= NULL*/)
{
    ASSERT((GetTypeId() == TYPEID_GAMEOBJECT || GetTypeId() == TYPEID_UNIT) && "Only GOs and creatures can summon npc groups!");

    std::vector<TempSummonData> const* data = sObjectMgr->GetSummonGroup(GetEntry(), GetTypeId() == TYPEID_GAMEOBJECT ? SUMMONER_TYPE_GAMEOBJECT : SUMMONER_TYPE_CREATURE, group);
    if (!data)
    {
        TC_LOG_WARN("scripts", "%s (%s) tried to summon non-existing summon group %u.", GetName().c_str(), GetGUID().ToString().c_str(), group);
        return;
    }

    for (std::vector<TempSummonData>::const_iterator itr = data->begin(); itr != data->end(); ++itr)
        if (TempSummon* summon = SummonCreature(itr->entry, itr->pos, itr->type, itr->time))
            if (list)
                list->push_back(summon);
}

Creature* WorldObject::FindNearestCreature(uint32 entry, float range, bool alive) const
{
    Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck checker(*this, entry, alive, range);

    return VisitSingleNearbyObject<Creature>(range, checker);
}

GameObject* WorldObject::FindNearestGameObject(uint32 entry, float range) const
{
    Trinity::NearestGameObjectEntryInObjectRangeCheck checker(*this, entry, range);

    return VisitSingleNearbyObject<GameObject>(range, checker);
}

GameObject* WorldObject::FindNearestGameObjectOfType(GameobjectTypes type, float range) const
{
    Trinity::NearestGameObjectTypeInObjectRangeCheck checker(*this, type, range);

    return VisitSingleNearbyObject<GameObject>(range, checker);
}

Player* WorldObject::FindNearestPlayer(float distance) const
{
    Trinity::NearestPlayerInObjectRangeCheck checker(this, distance);

    return VisitSingleNearbyObject<Player>(distance, checker);
}

void WorldObject::GetGameObjectListWithEntryInGrid(std::list<GameObject*>& gameObjectList, uint32 entry, float maxSearchRange /*= 250.0f*/) const
{
    Trinity::AllGameObjectsWithEntryInRange check(this, entry, maxSearchRange);

    VisitAnyNearbyObject<GameObject, Trinity::ContainerAction>(maxSearchRange, gameObjectList, check);
}

void WorldObject::GetCreatureListWithEntryInGrid(std::list<Creature*>& creatureList, uint32 entry, float maxSearchRange) const
{
    Trinity::AllCreaturesOfEntryInRange check(this, entry, maxSearchRange);

    VisitAnyNearbyObject<Creature, Trinity::ContainerAction>(maxSearchRange, creatureList, check);
}

void WorldObject::GetPlayerListInGrid(std::list<Player*>& playerList, float maxSearchRange, bool alive) const
{
    Trinity::AnyPlayerInObjectRangeCheck check(this, maxSearchRange, alive);

    VisitAnyNearbyObject<Player, Trinity::ContainerAction>(maxSearchRange, playerList, check);
}

//===================================================================================================

void WorldObject::GetNearPoint2D(float &x, float &y, float distance2d, float absAngle) const
{
    x = GetPositionX() + (GetObjectSize() + distance2d) * std::cos(absAngle);
    y = GetPositionY() + (GetObjectSize() + distance2d) * std::sin(absAngle);

    Trinity::NormalizeMapCoord(x);
    Trinity::NormalizeMapCoord(y);
}

void WorldObject::GetNearPoint(WorldObject const* /*searcher*/, float &x, float &y, float &z, float searcher_size, float distance2d, float absAngle) const
{
    GetNearPoint2D(x, y, distance2d + searcher_size, absAngle);
    z = GetPositionZ();
    // Should "searcher" be used instead of "this" when updating z coordinate ?
    UpdateAllowedPositionZ(x, y, z);

    // if detection disabled, return first point
    if (!sWorld->getBoolConfig(CONFIG_DETECT_POS_COLLISION))
        return;

    // return if the point is already in LoS
    if (IsWithinLOS(x, y, z))
        return;

    // remember first point
    float first_x = x;
    float first_y = y;
    float first_z = z;

    // loop in a circle to look for a point in LoS using small steps
    for (float angle = float(M_PI) / 8; angle < float(M_PI) * 2; angle += float(M_PI) / 8)
    {
        GetNearPoint2D(x, y, distance2d + searcher_size, absAngle + angle);
        z = GetPositionZ();
        UpdateAllowedPositionZ(x, y, z);
        if (IsWithinLOS(x, y, z))
            return;
    }

    // still not in LoS, give up and return first position found
    x = first_x;
    y = first_y;
    z = first_z;
}

void WorldObject::GetClosePoint(float &x, float &y, float &z, float size, float distance2d /*= 0*/, float angle /*= 0*/) const
{
    // angle calculated from current orientation
    GetNearPoint(nullptr, x, y, z, size, distance2d, GetOrientation() + angle);
}

void WorldObject::GetNearPosition(Position &pos, float dist, float angle) const
{
    GetPosition(&pos);
    MovePosition(pos, dist, angle);
}

void WorldObject::GetRandomNearPosition(Position &pos, float radius)
{
    GetPosition(&pos);
    MovePosition(pos, radius * (float)rand_norm(), (float)rand_norm() * static_cast<float>(2 * M_PI));
}

void WorldObject::GetContactPoint(const WorldObject* obj, float &x, float &y, float &z, float distance2d /*= CONTACT_DISTANCE*/) const
{
    // angle to face `obj` to `this` using distance includes size of `obj`
    GetNearPoint(obj, x, y, z, obj->GetObjectSize(), distance2d, GetAngle(obj));
}

float WorldObject::GetObjectSize() const
{
    return (m_valuesCount > UNIT_FIELD_COMBATREACH) ? m_floatValues[UNIT_FIELD_COMBATREACH] : DEFAULT_WORLD_OBJECT_SIZE;
}

void WorldObject::MovePosition(Position &pos, float dist, float angle) const
{
    angle += GetOrientation();
    float destx, desty, destz, ground, floor;
    destx = pos.m_positionX + dist * std::cos(angle);
    desty = pos.m_positionY + dist * std::sin(angle);

    // Prevent invalid coordinates here, position is unchanged
    if (!Trinity::IsValidMapCoord(destx, desty, pos.m_positionZ))
    {
        TC_LOG_FATAL("misc", "WorldObject::MovePosition: Object (TypeId: %hhu Entry: %u GUID: %u) has invalid coordinates X: %f and Y: %f were passed!",
            GetTypeId(), GetEntry(), GetGUID().GetCounter(), destx, desty);
        return;
    }

    ground = GetMap()->GetHeight(GetPhaseMask(), destx, desty, MAX_HEIGHT, true);
    floor = GetMap()->GetHeight(GetPhaseMask(), destx, desty, pos.m_positionZ, true);
    destz = std::fabs(ground - pos.m_positionZ) <= std::fabs(floor - pos.m_positionZ) ? ground : floor;

    float step = dist / 10.0f;

    for (uint8 j = 0; j < 10; ++j)
    {
        // do not allow too big z changes
        if (std::fabs(pos.m_positionZ - destz) > 6)
        {
            destx -= step * std::cos(angle);
            desty -= step * std::sin(angle);
            ground = GetMap()->GetHeight(GetPhaseMask(), destx, desty, MAX_HEIGHT, true);
            floor = GetMap()->GetHeight(GetPhaseMask(), destx, desty, pos.m_positionZ, true);
            destz = std::fabs(ground - pos.m_positionZ) <= std::fabs(floor - pos.m_positionZ) ? ground : floor;
        }
        // we have correct destz now
        else
        {
            pos.Relocate(destx, desty, destz);
            break;
        }
    }

    Trinity::NormalizeMapCoord(pos.m_positionX);
    Trinity::NormalizeMapCoord(pos.m_positionY);
    UpdateGroundPositionZ(pos.m_positionX, pos.m_positionY, pos.m_positionZ);
    pos.SetOrientation(GetOrientation());
}

// @todo: replace with WorldObject::UpdateAllowedPositionZ
float NormalizeZforCollision(WorldObject const* obj, float x, float y, float z)
{
    if (const auto transport = obj->GetTransport())
    {
        if ((obj->ToCreature() && obj->ToCreature()->CanFly()) || (obj->ToPlayer() && obj->ToPlayer()->CanFly()))
            return z;

        auto [lowerFloor, lowerHit] = transport->GetModelCollisionHeight(x, y, z);
        auto [upperFloor, upperHit] = transport->GetModelCollisionHeight(x, y, z + DEFAULT_HEIGHT_SEARCH / 2.f);
        if (lowerHit && upperHit)
            return std::fabs(lowerFloor - z) <= std::fabs(upperFloor - z) ? lowerFloor : upperFloor;
        else if (lowerHit)
            return lowerFloor;
        else if (upperHit)
            return upperFloor;
        else
            return VMAP_INVALID_HEIGHT_VALUE;
    }

    float ground = obj->GetMap()->GetHeight(obj->GetPhaseMask(), x, y, MAX_HEIGHT, true);
    float floor = obj->GetMap()->GetHeight(obj->GetPhaseMask(), x, y, z + 2.0f, true);
    float helper = std::fabs(ground - z) <= std::fabs(floor - z) ? ground : floor;
    if (z > helper) // must be above ground
    {
        if (Unit const* unit = obj->ToUnit())
        {
            if (unit->CanFly())
                return z;
        }
        LiquidData liquid_status;
        ZLiquidStatus res = obj->GetMap()->getLiquidStatus(x, y, z, MAP_ALL_LIQUIDS, &liquid_status);
        if (res && liquid_status.level > helper) // water must be above ground
        {
            if (liquid_status.level > z) // z is underwater
                return z;
            else
                return std::fabs(liquid_status.level - z) <= std::fabs(helper - z) ? liquid_status.level : helper;
        }
    }
    return helper;
}

Position WorldObject::MovePositionToFirstCollision2D(const Position& startPosition, const float distance, const float orientation) const
{
    return _MovePositionToFirstCollision(startPosition, distance, orientation, false);
}

Position WorldObject::MovePositionToFirstCollision3D(const Position& startPosition, const float distance, const float orientation) const
{
    return _MovePositionToFirstCollision(startPosition, distance, orientation, true);
}

Position WorldObject::_MovePositionToFirstCollision(const Position& startPosition, float dist, const float angle, const bool is3D) const
{
    Position target;
    const Position direction(std::cos(angle), std::sin(angle), 0.f);

    // check for slope
    
    const auto moveToMaxDistance = [&target, &direction, &startPosition, is3D, this, &dist]()
    {
        target = startPosition;
        target.m_positionZ = NormalizeZforCollision(this, target.GetPositionX(), target.GetPositionY(), target.GetPositionZ());
        if (target.m_positionZ == VMAP_INVALID_HEIGHT_VALUE)
        {
            target = startPosition;
            return;
        }
        const auto isWithinSlope = [](Position oldPos, Position newPos)
        {
            const float slope = math::slope3d(newPos.GetPositionX(), oldPos.GetPositionX(), newPos.GetPositionY(), oldPos.GetPositionY(), newPos.GetPositionZ(), oldPos.GetPositionZ());

            return std::abs(slope) <= 1.58f;
        };

        const uint8 steps = std::clamp(int(dist / .5f), 1, 20);

        const float stepLength = dist / steps;
        Position step = direction;
        step.Scale(stepLength);
        uint8 numSteps = steps;
        const float maxDistSq = dist * dist;
        do
        {
            Position nextTarget = target;
            nextTarget.Add(step);

            nextTarget.m_positionZ = NormalizeZforCollision(this, nextTarget.GetPositionX(), nextTarget.GetPositionY(), nextTarget.GetPositionZ());

            if (!isWithinSlope(target, nextTarget) || (is3D && maxDistSq < startPosition.GetExactDistSq(&nextTarget)))
                break;

            target = nextTarget;
        } while (--numSteps > 0);
    };

    moveToMaxDistance();

    // check for collisions

    const auto moveBack = [&moveToMaxDistance, &startPosition, &dist, is3D](float hitx, float hity, float hitz)
    {
        if (is3D)
            dist = std::min(dist, startPosition.GetExactDist(hitx, hity, hitz - 0.5f));
        else
            dist = std::min(dist, startPosition.GetExactDist2d(hitx, hity));
        moveToMaxDistance();
    };

    if (Transport const* transport = GetTransport())
    {
        const G3D::Vector3 dir = G3D::Vector3(target.m_positionX - startPosition.m_positionX, 
            target.m_positionY - startPosition.m_positionY, target.m_positionZ - startPosition.m_positionZ).direction();
        if (dir.isUnit())
        {
            const G3D::Ray ray(G3D::Point3(startPosition.m_positionX, startPosition.m_positionY, startPosition.m_positionZ + 2.f), dir);   // direction with length of 1
            if (transport->m_model->intersectRay(ray, dist, true, GetPhaseMask()))
            {
                dist -= CONTACT_DISTANCE;
                moveToMaxDistance();
            }
        }
    }
    else
    {
        float hitx, hizy, hitz;
        if (GetMap()->getStaticObjectHitPos(startPosition.m_positionX, startPosition.m_positionY, startPosition.m_positionZ + 0.5f,
            target.m_positionX, target.m_positionY, target.m_positionZ + 0.5f, hitx, hizy, hitz, -CONTACT_DISTANCE))
            moveBack(hitx, hizy, hitz);
        if (GetMap()->getDynamicObjectHitPos(GetPhaseMask(), target.m_positionX, target.m_positionY, target.m_positionZ + 0.5f,
            target.m_positionX, target.m_positionY, target.m_positionZ + 0.5f, hitx, hizy, hitz, -CONTACT_DISTANCE))
            moveBack(hitx, hizy, hitz);
    }

    Trinity::NormalizeMapCoord(target.m_positionX);
    Trinity::NormalizeMapCoord(target.m_positionY);
    
    target.SetOrientation(GetOrientation());

    return target;
}

void WorldObject::SetPhaseMask(uint32 newPhaseMask, bool update)
{
    m_phaseMask = newPhaseMask;

    if (update && IsInWorld())
        UpdateObjectVisibility();
}

bool WorldObject::InSamePhase(WorldObject const* obj) const
{
    return InSamePhase(obj->GetPhaseMask());
}

void WorldObject::PlayDistanceSound(uint32 sound_id, Player* target /*= NULL*/)
{
    WorldPacket data(SMSG_PLAY_OBJECT_SOUND, 4 + 8);
    data << uint32(sound_id);
    data << uint64(GetGUID());
    if (target)
        target->SendDirectMessage(std::move(data));
    else
        SendMessageToSet(data, true);
}

void WorldObject::PlayDirectSound(uint32 sound_id, Player* target /*= NULL*/)
{
    WorldPacket data(SMSG_PLAY_SOUND, 4);
    data << uint32(sound_id);
    if (target)
        target->SendDirectMessage(std::move(data));
    else
        SendMessageToSet(data, true);
}

void WorldObject::UpdateObjectVisibility(bool /*forced*/)
{
    //updates object's visibility for nearby players
    Trinity::VisibleChangesNotifier notifier(*this);
    VisitNearbyObject(GetVisibilityRange(), notifier);
}

void WorldObject::BroadcastValuesUpdate()
{
    const bool flush = isType(TYPEMASK_UNIT) && _changesMask.test(UNIT_FIELD_TARGET);

    if (auto self = ToPlayer())
        self->ApplyValuesUpdateBlock(*this, flush);

    visibilityGroup->DoForAllPlayer([this, flush](Player* player) { player->ApplyValuesUpdateBlock(*this, flush); });

    if (FindMap())
        if (auto replay = GetMap()->GetReplayData())
        {
            WorldPacket packet = CreateValuesUpdateBlockForPlayer(nullptr);
            replay->AddPacket(std::move(packet));
        }

    ClearUpdateMask(false);
}

ObjectGuid WorldObject::GetTransGUID() const
{
    if (GetTransport())
        return GetTransport()->GetGUID();
    return ObjectGuid::Empty;
}

std::string WorldObject::PrintGUID(std::string_view name, ObjectGuid guid)
{
    std::string _guid = std::to_string(guid.GetCounter());

    if (const uint32 entry = guid.GetEntry())
    {
        _guid += ", Entry: " + std::to_string(entry);

        if (name.empty())
        {
            if (guid.IsAnyTypeCreature())
            {
                if (auto templ = sObjectMgr->GetCreatureTemplate(entry))
                    name = templ->Name;
            }
            else if (guid.IsAnyTypeGameObject())
            {
                if (auto templ = sObjectMgr->GetGameObjectTemplate(entry))
                    name = templ->name;
            }
        }
    }

    if (name.empty())
        name = "UNKNOWN";

    std::string out;
    out += '\"';
    out += name;
    out += '\"';
    out += " (";
    out += guid.GetTypeName();
    out += ": " + _guid + ')';

    return out;
}

MotionTransport* WorldObject::GetMotionTransport() const
{
    if (m_transport)
        return m_transport->ToMotionTransport();
    return nullptr;
}

void WorldObject::GetCombatPosition(Unit const* searcher, float &x, float &y, float &z, float absAngle, bool predictMovement, float basedist)
{
    ASSERT(searcher);
    ASSERT(isType(TYPEMASK_UNIT));

    if (predictMovement)
    {
        Position originalPos(*this);
        ToUnit()->CalculateExpectedPosition(900).GetPosition(x, y, z);
        Relocate(x, y, z);
        GetCombatPosition(searcher, x, y, z, GetAngle(searcher), false);
        Relocate(originalPos);
        return;
    }

    // maximum allowed distance
    float maxdist = searcher->GetMeleeRange(static_cast<Unit*>(this)) * 0.95f;
    // preferred distance to target
    if (basedist < 0 || basedist > maxdist)
        basedist = searcher->GetMeleeRange(static_cast<Unit*>(this)) * 0.9f;
    float distance = basedist;
    float maxdistSq = maxdist * maxdist;

    // if searcher is a flying creature just choose nearstest point (if in LOS)
    if (auto npc = searcher->ToCreature())
    {
        if (npc->CanFly())
        {
            const float NEAR_GROUND_THRESHOLD = 3.f;
            float ground_z = GetMap()->GetHeight(searcher->GetPhaseMask(), searcher->m_positionX, searcher->m_positionY, searcher->m_positionZ, true);
            bool nearGround = std::abs(searcher->m_positionZ - ground_z) < NEAR_GROUND_THRESHOLD;
            if (nearGround)
            {
                ground_z = GetMap()->GetHeight(GetPhaseMask(), m_positionX, m_positionY, m_positionZ, true);
                nearGround = std::abs(m_positionZ - ground_z) < NEAR_GROUND_THRESHOLD;
            }

            x = searcher->m_positionX - m_positionX;
            y = searcher->m_positionY - m_positionY;
            z = nearGround ? 0.f : searcher->m_positionZ - m_positionZ;
            if (x || y || z)
            {
                float distFactor = std::min(basedist, maxdist) / sqrt(x*x + y * y + z * z);
                x = m_positionX + x * distFactor;
                y = m_positionY + y * distFactor;
                if (nearGround)
                    z = GetMap()->GetHeight(searcher->GetPhaseMask(), x, y, m_positionZ, true);
                else
                    z = m_positionZ + z * distFactor;
                if (IsWithinLOS(x, y, z))
                {
                    Trinity::NormalizeMapCoord(x);
                    Trinity::NormalizeMapCoord(y);
                    return;
                }
            }
        }
    }

    TC_LOG_TRACE("rg.debug", "GetCombatPosition this: " UI64FMTD ", inworld: %u, searcher: " UI64FMTD ", map: %u, x: %f, y: %f, z: %f, basedist: %f, maxdist: %f",
        GetGUID().GetRawValue(), IsInWorld(), searcher->GetGUID().GetRawValue(), GetMapId(), x, y, z, basedist, maxdist);

    for (float checkRange = 1.f; checkRange >= 0.f; checkRange -= 0.2f)
    {
        distance = basedist * checkRange;
        x = GetPositionX() + distance * std::cos(absAngle);
        y = GetPositionY() + distance * std::sin(absAngle);

        z = std::max(GetPositionZ(), searcher->GetPositionZ());
        searcher->UpdateAllowedPositionZ(x, y, z);

        if (GetExactDistSq(x, y, z) <= maxdistSq)
            break;
        else if (distance <= 0.2f)
        {
            GetPosition(x, y, z);
            break;
        }
    }

    // return if the point is already in LoS
    if (IsWithinLOS(x, y, z))
    {
        Trinity::NormalizeMapCoord(x);
        Trinity::NormalizeMapCoord(y);
        return;
    }

    // remember first point
    float first_x = x;
    float first_y = y;
    float first_z = z;

    bool side = false;

    // loop in a circle to look for a point in LoS using small steps
    for (float angle = float(M_PI) / 8; angle < float(M_PI) * 2; angle += float(M_PI) / 8)
    {
        for (float checkRange = 1.f; checkRange >= 0.f; checkRange -= 0.2f)
        {
            distance = basedist * checkRange;
            x = GetPositionX() + distance * std::cos(absAngle + (side ? angle : float(M_PI) * 2 - angle));
            y = GetPositionY() + distance * std::sin(absAngle + (side ? angle : float(M_PI) * 2 - angle));

            z = std::max(GetPositionZ(), searcher->GetPositionZ());
            searcher->UpdateAllowedPositionZ(x, y, z);

            if (GetExactDistSq(x, y, z) <= maxdistSq)
                break;
            else if (distance <= 0.2f)
            {
                GetPosition(x, y, z);
                break;
            }
        }
        if (IsWithinLOS(x, y, z))
        {
            Trinity::NormalizeMapCoord(x);
            Trinity::NormalizeMapCoord(y);
            return;
        }

        side = !side;
    }

    // still not in LoS, give up and return first position found
    x = first_x;
    y = first_y;
    z = first_z;
}

void WorldObject::ApplyEncounterTo(WorldObject& object)
{
    object.encounter = encounter;
}

std::string WorldObject::OnQueryOwnerName() const
{
    return std::to_string(GetGUID().GetRawValue());
}

std::string WorldObject::OnQueryFullName() const
{
    return PrintGUID(GetName(), GetGUID());
}

bool WorldObject::OnInitScript(std::string_view scriptName)
{
    return loadScriptType(*this, scriptName);
}
