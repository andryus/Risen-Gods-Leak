#include "Visibility.h"
#include "Player.h"
#include "WorldSession.h"
#include "Transport.h"
#include "Battleground.h"
#include "WorldPacket.h"
#include "ArenaReplaySystem.h"
#include "UpdateData.h"

#include "WorldDatabase.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"

std::unordered_map<uint8, std::unordered_map<uint32, VisibilityInfo>> VisibilityGroupBase::visiblityInfoMap;

void VisibilityGroupBase::LoadVisibilityInfo()
{
    visiblityInfoMap.clear();
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT sourceType, entry, visibilityType, data FROM visiblilty_data");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 visibility data entries. DB table `visiblilty_data` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        VisibilityInfo info;

        uint8 sourceType = fields[0].GetUInt8();
        uint32 entry = fields[1].GetUInt32();
        info.type = VisibilityTypes(fields[2].GetUInt8());
        info.data = fields[3].GetUInt32();

        visiblityInfoMap[sourceType][entry] = info;

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u faction change spell pairs in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

static const VisibilityInfo transportVisibility = { VISIBILITY_TYPE_MAP, 0 };

const VisibilityInfo* VisibilityGroupBase::GetVisibilityInfoFor(WorldObject * object)
{
    if (auto creature = object->ToCreature())
    {
        if (auto result = GetVisibilityInfo(VISIBILITY_SOURCE_TYPE_CREATURE_GUID, creature->GetSpawnId()))
            return result;
        if (auto result = GetVisibilityInfo(VISIBILITY_SOURCE_TYPE_CREATURE_ENTRY, creature->GetEntry()))
            return result;
    }
    if (auto gameObject = object->ToGameObject())
    {
        if (auto result = GetVisibilityInfo(VISIBILITY_SOURCE_TYPE_GAMEOBJECT_GUID, gameObject->GetSpawnId()))
            return result;
        if (auto result = GetVisibilityInfo(VISIBILITY_SOURCE_TYPE_GAMEOBJECT_ENTRY, gameObject->GetEntry()))
            return result;
        if (gameObject->IsTransport())
            return &transportVisibility;
    }
    return nullptr;
}

const VisibilityInfo * VisibilityGroupBase::GetVisibilityInfo(VisibilitySourceType type, uint32 entry)
{
    auto typeItr = visiblityInfoMap.find(uint8(type));
    if (typeItr != visiblityInfoMap.end())
    {
        auto infoItr = typeItr->second.find(entry);
        if (infoItr != typeItr->second.end())
            return &(infoItr->second);
    }
    return nullptr;
}

VisibilityGroup VisibilityGroupBase::GetOrCreateVisibilityGroupFor(WorldObject& obj, const VisibilityInfo* vInfo)
{
    if (!vInfo)
    {
        if (auto transport = obj.GetMotionTransport())
            if (auto transportGroup = transport->GetPassengerVisibilityGroup(&obj))
                return transportGroup;
        return WorldObjectVisibilityGroup::Create(&obj, obj.GetMap());
    }

    VisibilityGroup result;

    switch (vInfo->type)
    {
        case VISIBILITY_TYPE_MAP:
            result = obj.GetMap()->GetMapVisibilityGroup();
            break;
        case VISIBILITY_TYPE_ZONE:
            result = obj.GetMap()->GetZoneVisibilityGroup(vInfo->data, true);
            break;
        default: 
            result = WorldObjectVisibilityGroup::Create(&obj, obj.GetMap());
            break;
    }

    return result;
}

void VisibilityGroupBase::AddPlayer(Player* player)
{
    player->visibleGroups.insert(this);

    if (!objectMap.empty())
    {
        UpdateData data;
        for (auto itr : objectMap)
        {
            player->m_clientGUIDs.insert(itr->GetGUID());
            itr->BuildUpdateBlockForPlayer(data, player);
        }

        WorldPacket packet = data.BuildPacket(false);
        player->GetSession()->SendPacket(std::move(packet));

        for (auto itr : objectMap)
            if (auto unit = itr->ToUnit())
                for (auto pck : unit->BuildInitialVisiblePackets())
                    player->GetSession()->SendPacket(std::move(pck));
    }

    sessionMap.insert(player);
}

void VisibilityGroupBase::RemovePlayer(Player* player)
{
    player->visibleGroups.erase(this);

    if (!objectMap.empty())
    {
        std::vector<ObjectGuid> outOfRangeGuids;

        for (auto obj : objectMap)
        {
            player->m_clientGUIDs.erase(obj->GetGUID());
            outOfRangeGuids.push_back(obj->GetGUID());
        }
        player->FlushUpdateData(); //@TODO: is this needed?

        WorldPacket packet = buildOutOfRangePacket(std::move(outOfRangeGuids));

        player->SendDirectMessage(std::move(packet));
    }

    sessionMap.erase(player);
}

void VisibilityGroupBase::AddObject(WorldObject* object)
{
    for (auto itr : sessionMap)
    {
        itr->m_clientGUIDs.insert(object->GetGUID());

        UpdateData data;
        object->BuildUpdateBlockForPlayer(data, itr);

        WorldPacket packet = data.BuildPacket(false);
        itr->SendDirectMessage(std::move(packet));

        if (auto unit = object->ToUnit())
            for (auto pck : unit->BuildInitialVisiblePackets())
                itr->GetSession()->SendPacket(std::move(pck));
    }

    if (replay)
    {
        const WorldPacket packet = object->CreateUpdateBlockForPlayer(nullptr);
        replay->AddPacket(packet);

        if (Unit* unit = object->ToUnit())
            for (auto pck : unit->BuildInitialVisiblePackets())
                replay->AddPacket(packet);
    }

    objectMap.insert(object);
}

WorldPacket buildOutOfRangePacket(const WorldObject& object)
{
    return buildOutOfRangePacket(object.GetGUID());
}

void VisibilityGroupBase::RemoveObject(WorldObject* object)
{
    for (auto itr : sessionMap)
    {
        itr->FlushUpdateData(); //@TODO: is this needed?
        itr->m_clientGUIDs.erase(object->GetGUID());
        WorldPacket packet = buildOutOfRangePacket(object->GetGUID());
        itr->SendDirectMessage(std::move(packet));
    }

    if (replay)
        replay->AddPacket(buildOutOfRangePacket(*object));

    objectMap.erase(object);
}

void VisibilityGroupBase::BroadcastPacket(const WorldPacket& pck, WorldObject* sourceObj)
{
    DoForAllPlayer([data = pck](Player* plr) { plr->SendDirectMessage(data); });

    if (replay)
        replay->AddPacket(pck);
}

void VisibilityGroupBase::BroadcastPacket(const WorldPacket& pck, Player const * skipped_rcvr, WorldObject* sourceObj)
{
    DoForAllPlayer([data = pck, skipped_rcvr](Player* plr) { if (plr != skipped_rcvr) plr->SendDirectMessage(data); });

    if (replay)
        replay->AddPacket(pck);
}

void VisibilityGroupBase::Initialize(Map* base)
{
    this->base = base;

    if (auto replayData = base->GetReplayData())
        replay = replayData;
}

VisibilityGroupBase::VisibilityGroupBase() : base(nullptr) {}

VisibilityGroupBase::~VisibilityGroupBase() 
{
    for (auto itr : sessionMap)
        itr->visibleGroups.erase(this);
}

bool VisibilityGroupBase::HasPlayer(const Player * player)
{
    return sessionMap.find((Player*)player) != sessionMap.end();
}

WorldObjectVisibilityGroup::WorldObjectVisibilityGroup(WorldObject* owner) : owner(owner), isLargeObject(false), updateInterval(1000, urand(0, 1000)) { }

WorldObjectVisibilityGroup::~WorldObjectVisibilityGroup()
{
    owner->SetUpdateVisibilityGroup(nullptr);
}

void WorldObjectVisibilityGroup::SetLargeObject(bool value)
{
    if (value)
        owner->SetUpdateVisibilityGroup(this);
    isLargeObject = value;
}

VisibilityGroup WorldObjectVisibilityGroup::Create(WorldObject* owner, Map* base, float range)
{
    auto group = new WorldObjectVisibilityGroup(owner);
    auto result = util::shared_ptr<WorldObjectVisibilityGroup>(group);
    result->Initialize(base);

    if (auto creature = owner->ToCreature())
        if (creature->GetCombatReach() / DEFAULT_COMBAT_REACH > sWorld->getFloatConfig(CONFIG_DYNAMIC_VISIBILITY_LARGE_OBJECT_THRESHOLD))
            result->SetLargeObject(true);

    ASSERT(range >= 0.f);
    if (range > base->GetVisibilityRange())
    {
        owner->VisibilityRange = range;
        result->SetLargeObject(true);
    }

    return util::static_pointer_cast<VisibilityGroupBase>(result);
}

struct VisibilityUpdater
{
    WorldObject* owner;
    std::set<Player*> players;
    float dist;
    VisibilityUpdater(WorldObject* owner, float dist) : owner(owner), dist(dist) { }

    std::set<Player*>& GetPlayers() { return players; }

    template<class T> void Visit(T&) { }

    void Visit(Player&player) 
    {
        if (&player != owner && player.CanSeeOrDetect(owner, false, true)
            && player.IsWithinDist2d(owner, dist))
            players.insert(&player);
    }
};

void WorldObjectVisibilityGroup::UpdateVisibilityFor(WorldObject* observer)
{
    Player* player = observer->ToPlayer();
    if (player)
    {
        bool newVisibleState = player != owner && player->CanSeeOrDetect(owner, false, true);
        bool oldVisibleState;
        {
            oldVisibleState = GetPlayerSet().find(player) != GetPlayerSet().end();
        }

        if (newVisibleState != oldVisibleState)
        {
            if (newVisibleState)
                AddPlayer(player);
            else
                RemovePlayer(player);
        }
    }
    else if (auto creature = observer->ToCreature())
        for (auto itr : creature->GetSharedVisionList())
            UpdateVisibilityFor(itr);
    else if (auto dynamicObject = observer->ToDynObject())
        if (dynamicObject->GetCasterGUID().IsPlayer())
            if (Player* caster = ObjectAccessor::GetPlayer(*dynamicObject, dynamicObject->GetCasterGUID()))
                UpdateVisibilityFor(caster);
}

void WorldObjectVisibilityGroup::Update(uint32 diff)
{
    if (!isLargeObject)
        return;

    updateInterval.TUpdate(diff);
    if (updateInterval.TPassed())
    {
        updateInterval.TReset(diff, 1000);

        float dist = owner->GetVisibilityRange();

        VisibilityUpdater visibilityUpdate(owner, dist);
        owner->VisitNearbyObject(dist, visibilityUpdate);

        auto& playerList = visibilityUpdate.GetPlayers();
        auto currentPlayers = GetPlayerSet();

        do
        {
            if (currentPlayers.empty())
            {
                for (auto itr : playerList)
                    AddPlayer(itr);
                playerList.clear();
            }
            else if (playerList.empty())
            {
                for (auto itr : currentPlayers)
                    RemovePlayer(itr);
                currentPlayers.clear();
            }
            else if ((*currentPlayers.begin()) == (*playerList.begin()))
            {
                currentPlayers.erase(currentPlayers.begin());
                playerList.erase(playerList.begin());
            }
            else if ((*currentPlayers.begin()) < (*playerList.begin()))
            {
                RemovePlayer(*currentPlayers.begin());
                currentPlayers.erase(currentPlayers.begin());
            }
            else
            {
                AddPlayer(*playerList.begin());
                playerList.erase(playerList.begin());
            }
        } while (!currentPlayers.empty() || !playerList.empty());
    }
}

VisibilityGroup MapVisibilityGroup::Create(Map * base)
{
    auto group = new MapVisibilityGroup();
    auto result = util::shared_ptr<MapVisibilityGroup>(group);
    result->Initialize(base);
    return util::static_pointer_cast<MapVisibilityGroup>(result);
}

MapVisibilityGroup::MapVisibilityGroup() { }

VisibilityGroup ZoneVisibilityGroup::Create(Map * base)
{
    auto group = new ZoneVisibilityGroup();
    auto result = util::shared_ptr<ZoneVisibilityGroup>(group);
    result->Initialize(base);
    return util::static_pointer_cast<ZoneVisibilityGroup>(result);
}

ZoneVisibilityGroup::ZoneVisibilityGroup() { }
