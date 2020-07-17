#include "EncounterLogModule.hpp"

#include <boost/optional.hpp>

#include "Config.h"
#include "Player.h"
#include "Group.h"
#include "World.h"
#include "Creature.h"
#include "Map.h"

RG::Logging::EncounterLogModule::EncounterLogModule() :
    LogModule("Encounter")
{ }

void RG::Logging::EncounterLogModule::Initialize()
{
    QueryResult result = RGDatabase.Query("SELECT MAX(refId) FROM log_encounter");
    if (result)
        logNumber_.store((*result)[0].GetUInt32() + 1);

    extendedLoggingMaps_.clear();

    std::istringstream is{ sConfigMgr->GetStringDefault(GetOption("ExtendedMaps").c_str(), "") };
    for (std::string line; std::getline(is, line, ',');)
    {
        std::string mapString;
        std::string maskString;

        size_t pos = line.find(':');
        if (pos != std::string::npos)
        {
            mapString = line.substr(0, pos);
            maskString = line.erase(0, pos + 1);
        }
        else
        {
            mapString = line;
            maskString = std::to_string(SPAWNMASK_RAID_ALL | SPAWNMASK_DUNGEON_ALL | SPAWNMASK_CONTINENT);
        }

        extendedLoggingMaps_.emplace(std::stoul(mapString), std::stoul(maskString));
    }
}

void RG::Logging::EncounterLogModule::Log(Unit* creature, Type type, bool force)
{
    if (!IsEnabled())
        return;

    Map* instanceMap = creature->GetMap();
    if (!instanceMap)
        return;

    if (!force && !(type == Type::KILL || type == Type::SPECIAL))
    {
        auto itr = extendedLoggingMaps_.find(instanceMap->GetId());
        if (itr == extendedLoggingMaps_.end())
            return;

        if (!((1 << instanceMap->GetSpawnMode()) & itr->second))
            return;
    }

    const auto players = instanceMap->GetPlayers();

    boost::optional<const Group&> group = boost::none;
    for (const Player& player : players)
        if (!player.IsGameMaster() && player.GetGroup() != nullptr)
        {
            group = *player.GetGroup();
            break;
        }

    uint32 logId = logNumber_++;
    if (logId >= 0xFFFFFFFE)
    {
        TC_LOG_ERROR("misc", "Boss Log ids overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }

    SQLTransaction trans = RGDatabase.BeginTransaction();
    PreparedStatement* stmt;
    
    // hacky log for the players
    SQLTransaction trans2 = CharacterDatabase.BeginTransaction();
    PreparedStatement* stmt2;

    stmt = RGDatabase.GetPreparedStatement(RG_INS_ENCOUNTER);
    stmt->setUInt32(0, logId);
    stmt->setUInt16(1, instanceMap->GetId());
    stmt->setUInt8 (2, static_cast<uint8>(GetMode(instanceMap)));
    stmt->setUInt32(3, creature->GetInstanceId());
    stmt->setUInt32(4, creature->GetEntry());
    stmt->setUInt8 (5, static_cast<uint8>(type));
    stmt->setUInt32(6, players.size());
    stmt->setUInt32(7, group ? group->GetLeaderGUID().GetCounter() : 0);
    stmt->setUInt32(8, group && group->GetLootMethod() == MASTER_LOOT ? group->GetMasterLooterGuid().GetCounter() : 0);

    trans->Append(stmt);

    for (Player& groupMember : players)
    {
        stmt = RGDatabase.GetPreparedStatement(RG_INS_ENCOUNTER_MEMBER);
        stmt->setUInt32(0, logId);
        stmt->setUInt32(1, groupMember.GetGUID().GetCounter());
        stmt->setUInt32(2, groupMember.IsAlive());
        trans->Append(stmt);

        // hacky log for the players
        if (type == Type::KILL && instanceMap->IsRaid())
        {
            stmt2 = CharacterDatabase.GetPreparedStatement(CHAR_INS_TEMP_INSTANCE);
            stmt2->setUInt32(0, groupMember.GetGUID().GetCounter());
            stmt2->setUInt16(1, instanceMap->GetId());
            stmt2->setUInt8 (2, static_cast<uint8>(GetMode(instanceMap)));
            trans2->Append(stmt2);
        }
    }

    // loot log for the boss - does not work with chests
    if (Creature* c = creature->ToCreature())
    {
        const Loot& loot = c->loot;
        for (const LootItem& item : loot.items)
        {
            stmt = RGDatabase.GetPreparedStatement(RG_INS_ENCOUNTER_LOOT);
            stmt->setUInt32(0, logId);
            stmt->setUInt32(1, item.itemid);
            trans->Append(stmt);
        }
        for (const LootItem& item : loot.quest_items)
        {
            stmt = RGDatabase.GetPreparedStatement(RG_INS_ENCOUNTER_LOOT);
            stmt->setUInt32(0, logId);
            stmt->setUInt32(1, item.itemid);
            trans->Append(stmt);
        }
    }

    RGDatabase.CommitTransaction(trans);
    // hacky log for the players
    CharacterDatabase.CommitTransaction(trans2);
}

RG::Logging::EncounterLogModule::Mode RG::Logging::EncounterLogModule::GetMode(Map* map)
{
    if (!map->IsDungeon())
        return Mode::DEFAULT;

    if (map->IsRaid())
    {
        switch (map->GetDifficulty())
        {
        case RAID_DIFFICULTY_10MAN_NORMAL:
            return Mode::RAID_10MAN;
        case RAID_DIFFICULTY_10MAN_HEROIC:
            return Mode::RAID_10MAN_HEROIC;
        case RAID_DIFFICULTY_25MAN_NORMAL:
            return Mode::RAID_25MAN;
        case RAID_DIFFICULTY_25MAN_HEROIC:
            return Mode::RAID_25MAN_HEROIC;
        }
    }

    switch (map->GetDifficulty())
    {
    case DUNGEON_DIFFICULTY_NORMAL:
        return Mode::INSTANCE_NORMAL;
    case DUNGEON_DIFFICULTY_HEROIC:
        return Mode::INSTANCE_HEROIC;
    case DUNGEON_DIFFICULTY_EPIC:
        return Mode::INSTANCE_EPIC;
    default:
        break;
    }

    return Mode::DEFAULT;
}
