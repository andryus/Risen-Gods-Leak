#include "InstanceMap.h"
#include "World.h"
#include "InstanceScript.h"
#include "Group.h"
#include "InstanceSaveMgr.h"
#include "WorldPacket.h"
#include "ScriptMgr.h"
#include "Opcodes.h"
#include "LFGMgr.h"
#include "DBCStores.h"

#include "API/LoadScript.h"
#include "EncounterHooks.h"
#include "CreatureOwner.h"

/* ******* Dungeon Instance Maps ******* */

InstanceMap* Map::ToInstanceMap() { return To<InstanceMap>(); }
InstanceMap const* Map::ToInstanceMap() const { return To<InstanceMap>(); }

InstanceMap::InstanceMap(uint32 id, time_t expiry, uint32 InstanceId, uint8 SpawnMode, uint8 team, Map* _parent)
    : Map(id, expiry, InstanceId, SpawnMode, _parent),
    m_resetAfterUnload(false), m_unloadWhenEmpty(false),
    instanceInfo(nullptr), activeTeam(team), dungeonId(0)
{
    //lets initialize visibility distance for dungeons
    InstanceMap::InitVisibilityDistance();

    // the timer is started by default, and stopped when the first player joins
    // this make sure it gets unloaded if for some reason no player joins
    m_unloadTimer = std::max(sWorld->getIntConfig(CONFIG_INSTANCE_UNLOAD_DELAY), (uint32)MIN_UNLOAD_DELAY);
}

InstanceMap::~InstanceMap()
{
    unloadScript(*this);
}

void InstanceMap::LoadFromDB()
{
    const uint32 id = GetInstanceId();

    // instance
    {
        PreparedStatement& stmt = *CharacterDatabase.GetPreparedStatement(CHAR_SEL_INSTANCE_EVENT_STATE);

        stmt.setUInt32(0, id);

        if (PreparedQueryResult result = CharacterDatabase.Query(&stmt))
        {
            std::vector<script::SerializedEvent> events;
            events.reserve(result->GetRowCount());

            do
            {
                const Field* fields = result->Fetch();

                const std::string_view event = fields[0].GetCString();
                const uint32 data = fields[1].GetUInt32();

                events.emplace_back(event, data);
            } while(result->NextRow());

            ASSERT(HasScript());
            ScriptCtx()->Deserialize(events);
        }
    }

    // encounters
    for (auto [encounterId, encounter] : encounters)
    {
        PreparedStatement& stmt = *CharacterDatabase.GetPreparedStatement(CHAR_SEL_INSTANCE_ENCOUNTER_STATE);

        stmt.setUInt32(0, id);
        stmt.setUInt32(1, encounterId);

        if (PreparedQueryResult result = CharacterDatabase.Query(&stmt))
        {
            Encounter::DataMap data;
            data.reserve(result->GetRowCount());

            do
            {
                Field* fields = result->Fetch();

                const std::string_view tag = fields[0].GetCString();
                const uint32 value = fields[1].GetUInt32();

                data.emplace(tag, value);
            } while (result->NextRow());

            encounter->ApplyData(data);
        }
    }
}

void InstanceMap::SaveToDB() const
{
    const uint32 id = GetInstanceId();

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    if (HasScript())
    {
        PreparedStatement& delStmt = *CharacterDatabase.GetPreparedStatement(CHAR_DEL_INSTANCE_EVENTS);
        delStmt.setUInt32(0, id);

        trans->Append(&delStmt);

        const auto events = ScriptCtx()->Serialize();

        for (const auto& event : events)
        {
            PreparedStatement& stmt = *CharacterDatabase.GetPreparedStatement(CHAR_INS_INSTANCE_EVENT_SAVE);

            stmt.setUInt32(0, id);
            stmt.setString(1, event.first);
            stmt.setUInt32(2, event.second);

            trans->Append(&stmt);
        }
    }

    // encounters
    PreparedStatement& delStmt = *CharacterDatabase.GetPreparedStatement(CHAR_DEL_INSTANCE_ENCOUNTERS);
    delStmt.setUInt32(0, id);

    trans->Append(&delStmt);

    for (auto [encounterId, encounter] : encounters)
    {
        const auto values = encounter->AquireData();

        for (auto [tag, data] : values)
        {
            PreparedStatement& stmt = *CharacterDatabase.GetPreparedStatement(CHAR_INS_INSTANCE_ENCOUNTER_SAVE);

            stmt.setUInt32(0, id);
            stmt.setUInt32(1, encounterId);
            stmt.setString(2, tag);
            stmt.setUInt32(3, data);

            trans->Append(&stmt);
        }
    }

    CharacterDatabase.CommitTransaction(trans);
}

void InstanceMap::FinishDungeon()
{
    ASSERT(dungeonId);

    for (Player& player : GetPlayers()) // @todo: better design
        if (Group* grp = player.GetGroup())
            if (grp->isLFGGroup())
            {
                sLFGMgr->FinishDungeon(grp->GetGUID(), dungeonId);
                return;
            }
}

void InstanceMap::InitVisibilityDistance()
{
    //init visibility distance for instances
    m_VisibleDistance = World::GetMaxVisibleDistanceInInstances();
    m_VisibilityNotifyPeriod = World::GetVisibilityNotifyPeriodInInstances();
}

void InstanceMap::OnWorldObjectAdded(WorldObject& object)
{
    *this <<= Fire::AddToInstance.Bind(&object);

    if (Creature* creature = object.ToCreature())
    {
        const uint32 creatureEntry = creature->GetEntry();
        const uint32 entry = sObjectMgr->GetEncounterForCreature(creatureEntry, GetDifficulty());

        if (entry != NO_ENCOUNTER)
        {
            auto itr = encounters.find(entry);
            if (itr == encounters.end())
            {
                TC_LOG_ERROR("maps", "InstanceMap::OnWorldObjectAdded Cannot find encounter %i for WorldObject %s", creatureEntry, creature->GetGUID().ToString().c_str());
                return;
            }

            creature->SetEncounter(itr->second);
            creature <<= Fire::RegisterEncounter.Bind(itr->second.get());
        }
    }
}

bool InstanceMap::OnInitScript(std::string_view scriptName)
{
    return loadScriptType(*this, scriptName);
}

std::string InstanceMap::OnQueryOwnerName() const
{
    return GetMapName();
}

/*
Do map specific checks to see if the player can enter
*/
bool InstanceMap::CanEnter(Player* player)
{
    if (_players.find(player->GetGUID()) != _players.end())
        TC_LOG_ERROR("maps", "InstanceMap::CanEnter - player %s(%u) already in map %d, %d, %d!", player->GetName().c_str(), player->GetGUID().GetCounter(), GetId(), GetInstanceId(), GetSpawnMode());

    // allow GM's to enter
    if (player->IsGameMaster())
        return Map::CanEnter(player);

    // cannot enter if the instance is full (player cap), GMs don't count
    uint32 maxPlayers = GetMaxPlayers();
    if (GetPlayersCountExceptGMs() >= maxPlayers && player->FindMap() != this)
    {
        TC_LOG_WARN("maps", "MAP: Instance '%u' of map '%s' cannot have more than '%u' players. Player '%s' rejected", GetInstanceId(), GetMapName(), maxPlayers, player->GetName().c_str());
        player->SendTransferAborted(GetId(), TRANSFER_ABORT_MAX_PLAYERS);
        return false;
    }

    // cannot enter while an encounter is in progress
    if (IsRaid() && !CanPlayerEnter(*player))
    {
        player->SendTransferAborted(GetId(), TRANSFER_ABORT_ZONE_IN_COMBAT);
        return false;
    }

    // cannot enter if instance is in use by another party/soloer that have a
    // permanent save in the same instance id

    for (Player& iPlayer : GetPlayers())
    {
        if (iPlayer.IsGameMaster()) // bypass GMs
            continue;

        if (!player->GetGroup()) // player has not group and there is someone inside, deny entry
        {
            player->SendTransferAborted(GetId(), TRANSFER_ABORT_MAX_PLAYERS);
            return false;
        }
        // player inside instance has no group or his groups is different to entering player's one, deny entry
        if (!iPlayer.GetGroup() || !iPlayer.IsInSameRaidWith(player))
        {
            player->SendTransferAborted(GetId(), TRANSFER_ABORT_MAX_PLAYERS);
            return false;
        }
        break;
    }

    return Map::CanEnter(player);
}

/*
Do map specific checks and add the player to the map if successful.
*/
bool InstanceMap::AddPlayerToMap(std::shared_ptr<Player> player)
{
    /// @todo Not sure about checking player level: already done in HandleAreaTriggerOpcode
    // GMs still can teleport player in instance.
    // Is it needed?

    {
        std::lock_guard<std::mutex> lock(_mapLock);
        // Check moved to void WorldSession::HandleMoveWorldportAckOpcode()
        //if (!CanEnter(player))
        //return false;

        // Dungeon only code
        if (IsDungeon())
        {
            Group* group = player->GetGroup();

            // increase current instances (hourly limit)
            if (!group || !group->isLFGGroup())
                player->AddInstanceEnterTime(GetInstanceId(), time(NULL));

            // get or create an instance save for the map
            InstanceSave* mapSave = sInstanceSaveMgr->GetInstanceSave(GetInstanceId());
            if (!mapSave)
            {
                TC_LOG_DEBUG("maps", "InstanceMap::Add: creating instance save for map %d spawnmode %d with instance id %d", GetId(), GetSpawnMode(), GetInstanceId());
                mapSave = sInstanceSaveMgr->AddInstanceSave(GetId(), GetInstanceId(), Difficulty(GetSpawnMode()), 0, true);
            }

            ASSERT(mapSave);

            // check for existing instance binds
            InstancePlayerBind* playerBind = player->GetBoundInstance(GetId(), Difficulty(GetSpawnMode()));
            if (playerBind && playerBind->perm)
            {
                // cannot enter other instances if bound permanently
                if (playerBind->save != mapSave)
                {
                    TC_LOG_ERROR("maps", "InstanceMap::Add: player %s(%d) is permanently bound to instance %s %d, %d, %d, %d, %d, %d but he is being put into instance %s %d, %d, %d, %d, %d, %d", player->GetName().c_str(), player->GetGUID().GetCounter(), GetMapName(), playerBind->save->GetMapId(), playerBind->save->GetInstanceId(), playerBind->save->GetDifficulty(), playerBind->save->GetPlayerCount(), playerBind->save->GetGroupCount(), playerBind->save->CanReset(), GetMapName(), mapSave->GetMapId(), mapSave->GetInstanceId(), mapSave->GetDifficulty(), mapSave->GetPlayerCount(), mapSave->GetGroupCount(), mapSave->CanReset());
                    return false;
                }
            }
            else
            {
                if (group)
                {
                    // solo saves should be reset when entering a group
                    InstanceGroupBind* groupBind = group->GetBoundInstance(this);
                    if (playerBind && playerBind->save != mapSave)
                    {
                        TC_LOG_ERROR("maps", "InstanceMap::Add: player %s(%d) is being put into instance %s %d, %d, %d, %d, %d, %d but he is in group %d and is bound to instance %d, %d, %d, %d, %d, %d!", player->GetName().c_str(), player->GetGUID().GetCounter(), GetMapName(), mapSave->GetMapId(), mapSave->GetInstanceId(), mapSave->GetDifficulty(), mapSave->GetPlayerCount(), mapSave->GetGroupCount(), mapSave->CanReset(), group->GetLeaderGUID().GetCounter(), playerBind->save->GetMapId(), playerBind->save->GetInstanceId(), playerBind->save->GetDifficulty(), playerBind->save->GetPlayerCount(), playerBind->save->GetGroupCount(), playerBind->save->CanReset());
                        if (groupBind)
                            TC_LOG_ERROR("maps", "InstanceMap::Add: the group is bound to the instance %s %d, %d, %d, %d, %d, %d", GetMapName(), groupBind->save->GetMapId(), groupBind->save->GetInstanceId(), groupBind->save->GetDifficulty(), groupBind->save->GetPlayerCount(), groupBind->save->GetGroupCount(), groupBind->save->CanReset());
                        //ABORT();
                        return false;
                    }
                    // bind to the group or keep using the group save
                    if (!groupBind)
                        group->BindToInstance(mapSave, false);
                    else
                    {
                        // cannot jump to a different instance without resetting it
                        if (groupBind->save != mapSave)
                        {
                            TC_LOG_ERROR("maps", "InstanceMap::Add: player %s(%d) is being put into instance %d, %d, %d but he is in group %d which is bound to instance %d, %d, %d!", player->GetName().c_str(), player->GetGUID().GetCounter(), mapSave->GetMapId(), mapSave->GetInstanceId(), mapSave->GetDifficulty(), group->GetLeaderGUID().GetCounter(), groupBind->save->GetMapId(), groupBind->save->GetInstanceId(), groupBind->save->GetDifficulty());
                            TC_LOG_ERROR("maps", "MapSave players: %d, group count: %d", mapSave->GetPlayerCount(), mapSave->GetGroupCount());
                            if (groupBind->save)
                                TC_LOG_ERROR("maps", "GroupBind save players: %d, group count: %d", groupBind->save->GetPlayerCount(), groupBind->save->GetGroupCount());
                            else
                                TC_LOG_ERROR("maps", "GroupBind save NULL");
                            return false;
                        }
                        // if the group/leader is permanently bound to the instance
                        // players also become permanently bound when they enter
                        if (groupBind->perm)
                        {
                            WorldPacket data(SMSG_INSTANCE_LOCK_WARNING_QUERY, 9);
                            data << uint32(60000);
                            data << uint32(GetCompletedEncounterMask());
                            data << uint8(0);
                            player->SendDirectMessage(std::move(data));
                            player->SetPendingBind(mapSave->GetInstanceId(), 60000);
                        }
                    }
                }
                else
                {
                    // set up a solo bind or continue using it
                    if (!playerBind)
                        player->BindToInstance(mapSave, false);
                    else
                        // cannot jump to a different instance without resetting it
                        ASSERT(playerBind->save == mapSave);
                }
            }
        }

        // for normal instances cancel the reset schedule when the
        // first player enters (no players yet)
        SetResetSchedule(false);

        TC_LOG_DEBUG("maps", "MAP: Player '%s' entered instance '%u' of map '%s'", player->GetName().c_str(), GetInstanceId(), GetMapName());
        // initialize unload state
        m_unloadTimer = 0;
        m_resetAfterUnload = false;
        m_unloadWhenEmpty = false;
    }

    // this will acquire the same mutex so it cannot be in the previous block
    Map::AddPlayerToMap(player);

    if (i_data)
        i_data->OnPlayerEnter(player.get());

    return true;
}

void InstanceMap::Update(const uint32 t_diff)
{
    Map::Update(t_diff);

    for (auto& [id, encounter] : encounters)
        encounter->Update(std::chrono::milliseconds(t_diff));

    if (i_data)
        i_data->Update(t_diff);
}

void InstanceMap::RemovePlayerFromMap(std::shared_ptr<Player> player)
{
    TC_LOG_DEBUG("maps", "MAP: Removing player '%s' from instance '%u' of map '%s' before relocating to another map", player->GetName().c_str(), GetInstanceId(), GetMapName());

    if (i_data)
        i_data->OnPlayerRemove(player.get());

    //if last player set unload timer
    if (!m_unloadTimer && _players.size() == 1)
        m_unloadTimer = m_unloadWhenEmpty ? MIN_UNLOAD_DELAY : std::max(sWorld->getIntConfig(CONFIG_INSTANCE_UNLOAD_DELAY), (uint32)MIN_UNLOAD_DELAY);
    Map::RemovePlayerFromMap(player);

    // for normal instances schedule the reset after all players have left
    SetResetSchedule(true);
    sInstanceSaveMgr->UnloadInstanceSave(GetInstanceId());
}

void InstanceMap::CreateInstanceData(bool load)
{
    if (i_data != nullptr)
        return;

    InstanceTemplate const* mInstance = sObjectMgr->GetInstanceTemplate(GetId());
    if (mInstance)
    {
        instanceInfo = mInstance;
        i_data = std::unique_ptr<InstanceScript>(sScriptMgr->CreateInstanceData(this));

        InitScript(instanceInfo->ScriptName);
    }

    const DungeonEncounterSpan storedEncounters = sObjectMgr->GetDungeonEncounterList(GetId(), GetDifficulty());

    for (const auto& storedEncounter : storedEncounters)
    {
        if (storedEncounter.lastEncounterDungeon)
        {
            ASSERT(!dungeonId);

            dungeonId = storedEncounter.lastEncounterDungeon;
        }

        auto encounter = std::make_shared<Encounter>(*this, storedEncounter.ScriptName, storedEncounter.lastEncounterDungeon != 0);
        encounter->Init();

        encounters.emplace(storedEncounter.dbcEntry->id, std::move(encounter));
    }

    if (load)
    {
        if (i_data)
        {
            /// @todo make a global storage for this
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_INSTANCE);
            stmt->setUInt16(0, uint16(GetId()));
            stmt->setUInt32(1, i_InstanceId);
            PreparedQueryResult result = CharacterDatabase.Query(stmt);

            if (result)
            {
                Field* fields = result->Fetch();
                std::string data = fields[0].GetString();
                i_data->SetCompletedEncountersMask(fields[1].GetUInt32());
                if (!data.empty())
                {
                    TC_LOG_DEBUG("maps", "Loading instance data for `%s` with id %u", instanceInfo->ScriptName.c_str(), i_InstanceId);
                    i_data->Load(data.c_str());
                }
            }
        }

        LoadFromDB();
    }
}

/*
Returns true if there are no players in the instance
*/
bool InstanceMap::Reset(uint8 method)
{
    // note: since the map may not be loaded when the instance needs to be reset
    // the instance must be deleted from the DB by InstanceSaveManager

    if (HavePlayers())
    {
        if (method == INSTANCE_RESET_ALL || method == INSTANCE_RESET_CHANGE_DIFFICULTY)
        {
            // notify the players to leave the instance so it can be reset
            for (Player& player : GetPlayers())
                player.SendResetFailedNotify(GetId());
        }
        else
        {
            if (method == INSTANCE_RESET_GLOBAL)
                // set the homebind timer for players inside (1 minute)
                for (Player& player : GetPlayers())
                    player.m_InstanceValid = false;

            // the unload timer is not started
            // instead the map will unload immediately after the players have left
            m_unloadWhenEmpty = true;
            m_resetAfterUnload = true;
        }
    }
    else
    {
        // unloaded at next update
        m_unloadTimer = MIN_UNLOAD_DELAY;
        m_resetAfterUnload = true;
    }

    return _players.empty();
}

void InstanceMap::PermBindAllPlayers(Player* source)
{
    if (!IsDungeon())
        return;

    InstanceSave* save = sInstanceSaveMgr->GetInstanceSave(GetInstanceId());
    if (!save)
    {
        TC_LOG_ERROR("maps", "Cannot bind player (GUID: %u, Name: %s), because no instance save is available for instance map (Name: %s, Entry: %u, InstanceId: %u)!", source->GetGUID().GetCounter(), source->GetName().c_str(), source->GetMap()->GetMapName(), source->GetMapId(), GetInstanceId());
        return;
    }

    Group* group = source->GetGroup();
    // group members outside the instance group don't get bound
    for (Player& player : GetPlayers())
    {
        // players inside an instance cannot be bound to other instances
        // some players may already be permanently bound, in this case nothing happens
        InstancePlayerBind* bind = player.GetBoundInstance(save->GetMapId(), save->GetDifficulty());
        if (!bind || !bind->perm)
        {
            player.BindToInstance(save, true);
            WorldPacket data(SMSG_INSTANCE_SAVE_CREATED, 4);
            data << uint32(0);
            player.SendDirectMessage(std::move(data));

            player.GetSession()->SendCalendarRaidLockout(save, true);
        }
    }
    if (group)
        group->BindToInstance(save, true);
}

void InstanceMap::UnloadAll()
{
    ASSERT(!HavePlayers());

    if (m_resetAfterUnload == true)
        DeleteRespawnTimes();

    Map::UnloadAll();
}

void InstanceMap::SendResetWarnings(uint32 timeLeft) const
{
    for (Player& player : GetPlayers())
        player.SendInstanceResetWarning(GetId(), player.GetDifficulty(IsRaid()), timeLeft, false);
}

void InstanceMap::SetResetSchedule(bool on)
{
    // only for normal instances
    // the reset time is only scheduled when there are no payers inside
    // it is assumed that the reset time will rarely (if ever) change while the reset is scheduled
    if (IsDungeon() && !HavePlayers() && !IsRaidOrHeroicDungeon())
    {
        if (InstanceSave* save = sInstanceSaveMgr->GetInstanceSave(GetInstanceId()))
            sInstanceSaveMgr->ScheduleReset(on, save->GetResetTime(), InstanceSaveManager::InstResetEvent(0, GetId(), Difficulty(GetSpawnMode()), GetInstanceId()));
        else
            TC_LOG_ERROR("maps", "InstanceMap::SetResetSchedule: cannot turn schedule %s, there is no save information for instance (map [id: %u, name: %s], instance id: %u, difficulty: %u)",
                on ? "on" : "off", GetId(), GetMapName(), GetInstanceId(), Difficulty(GetSpawnMode()));
    }
}

MapDifficulty const* Map::GetMapDifficulty() const
{
    return GetMapDifficultyData(GetId(), GetDifficulty());
}

uint32 InstanceMap::GetMaxPlayers() const
{
    MapDifficulty const* mapDiff = GetMapDifficulty();
    if (mapDiff && mapDiff->maxPlayers)
        return mapDiff->maxPlayers;

    return GetEntry()->maxPlayers;
}

uint32 InstanceMap::GetMaxResetDelay() const
{
    MapDifficulty const* mapDiff = GetMapDifficulty();
    return mapDiff ? mapDiff->resetTime : 0;
}

bool InstanceMap::IsEncounterInProgress() const
{
    if (GetActiveEncounter())
        return true;

    if (i_data)
        return i_data->IsEncounterInProgress();

    return false;
}


Encounter* InstanceMap::GetActiveEncounter(void) const
{
    for (auto&[id, encounter] : encounters) // @todo: raids can have multiple active encounters
        if (encounter->IsStarted())
            return encounter.get();

    return nullptr;
}

uint32 InstanceMap::GetCompletedEncounterMask() const
{
    uint32 mask = 0;
    for (auto&[id, encounter] : encounters)
        if (encounter->IsFinished())
            mask |= 1 << sDungeonEncounterStore.LookupEntry(id)->encounterIndex;

    if (i_data)
        mask |= i_data->GetCompletedEncounterMask();

    return mask;
}


bool InstanceMap::CanPlayerEnter(const Player& player) const
{
    if (IsEncounterInProgress())
    {
        if (player.IsLoading())
        {
            if (Encounter* encounter = GetActiveEncounter())
                return encounter->IsPlayerActive(player);
            if (i_data)
                return i_data->CanPlayerEnter(player);
        }

        return false;
    }

    return true;
}
