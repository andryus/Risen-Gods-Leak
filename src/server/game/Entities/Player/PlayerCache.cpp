#include "PlayerCache.hpp"

#include "Log.h"
#include "Player.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "World.h"
#include "Packets/WorldPacket.h"

namespace player
{
    PlayerCache::MutexType PlayerCache::mutex;

namespace
{

template<typename T>
const T& do_lookup(const PlayerCache::Store& store, ObjectGuid guid, T PlayerCache::Entry::*member, const T& def)
{
    const auto& index = store.get<PlayerCache::guid_tag>();
    auto itr = index.find(guid);
    if (itr == index.end())
        return def;
    else
        return (*itr).*member;
}

template<typename T, typename Getter>
const T& do_lookup(const PlayerCache::Store& store, ObjectGuid guid, Getter&& getter, const T& def)
{
    const auto& index = store.get<PlayerCache::guid_tag>();
    auto itr = index.find(guid);
    if (itr == index.end())
        return def;
    else
        return getter(*itr);
}


template<typename T>
const T& do_lookup(const PlayerCache::Store& store, const std::string& name, T PlayerCache::Entry::*member, const T& def)
{
    const auto& index = store.get<PlayerCache::name_tag>();
    auto range = index.equal_range(name);
    for (auto itr = range.first; itr != range.second; ++itr)
    {
        const auto& entry = *itr;
        if ((entry.at_login & AT_LOGIN_RENAME) != 0)
            continue;

        return entry.*member;
    }

    return def;
}

template<typename T>
void do_update(PlayerCache::Store& store, ObjectGuid guid, T PlayerCache::Entry::*member, const T& value)
{
    auto& index = store.get<PlayerCache::guid_tag>();
    auto itr = index.find(guid);
    if (itr != index.end())
        index.modify(itr, [member, &value](PlayerCache::Entry& entry) { entry.*member = value; });
}

template<typename Updater>
void do_update(PlayerCache::Store& store, ObjectGuid guid, Updater&& updater)
{
    auto& index = store.get<PlayerCache::guid_tag>();
    auto itr = index.find(guid);
    if (itr != index.end())
        index.modify(itr, updater);
}

}

PlayerCache::Store PlayerCache::cache_{};

bool PlayerCache::Exists(ObjectGuid guid)
{
    ReadLock lock(mutex);
    const auto& index =  cache_.get<guid_tag>();
    auto itr = index.find(guid);
    return itr != index.end();
}

bool PlayerCache::Exists(const std::string& name)
{
    ReadLock lock(mutex);
    const auto& index = cache_.get<name_tag>();
    auto range = index.equal_range(name);
    for (auto itr = range.first; itr != range.second; ++itr)
        if ((itr->at_login & AT_LOGIN_RENAME) == 0)
            return true;

    return false;
}

PlayerCache::PlayerCacheEntry PlayerCache::Get(ObjectGuid guid)
{
    ReadLock lock(mutex);
    const auto& index =  cache_.get<guid_tag>();
    auto itr = index.find(guid);
    if (itr != index.end())
        return (*itr);
    else
        return PlayerCacheEntry();
}

PlayerCache::PlayerCacheEntry PlayerCache::Get(const std::string& name)
{
    ReadLock lock(mutex);
    const auto& index = cache_.get<name_tag>();
    auto range = index.equal_range(name);
    for (auto itr = range.first; itr != range.second; ++itr)
        if ((itr->at_login & AT_LOGIN_RENAME) == 0)
            return (*itr);
    return PlayerCacheEntry();
}

ObjectGuid PlayerCache::GetGUID(const std::string& name)
{
    static const ObjectGuid NOT_FOUND = ObjectGuid::Empty;

    ReadLock lock(mutex);
    return do_lookup(cache_, name, &Entry::guid, NOT_FOUND);
}

const std::string& PlayerCache::GetName(ObjectGuid guid)
{
    static const std::string NOT_FOUND = "<not found>";

    ReadLock lock(mutex);
    return do_lookup(cache_, guid, [](const Entry& entry) -> const std::string&
                     {
                         static const std::string RENAME = "<rename>";

                         if ((entry.at_login & AT_LOGIN_RENAME) != 0)
                             return RENAME;
                         return entry.name;
                     },
                     NOT_FOUND);
}

uint32 PlayerCache::GetTeam(ObjectGuid guid)
{
    static const uint8 NOT_FOUND = RACE_NONE;

    ReadLock lock(mutex);
    return Player::TeamForRace(do_lookup(cache_, guid, &Entry::race, NOT_FOUND));
}

uint32 PlayerCache::GetAccountId(ObjectGuid guid)
{
    static const uint32 NOT_FOUND = 0;

    ReadLock lock(mutex);
    return do_lookup(cache_, guid, &Entry::account, NOT_FOUND);
}

uint32 PlayerCache::GetAccountId(const std::string& name)
{
    static const uint32 NOT_FOUND = 0;

    ReadLock lock(mutex);
    return do_lookup(cache_, name, &Entry::account, NOT_FOUND);
}

uint32 PlayerCache::GetGuildId(ObjectGuid guid)
{
    static const uint32 NOT_FOUND = 0;

    ReadLock lock(mutex);
    return do_lookup(cache_, guid, &Entry::guild, NOT_FOUND);
}

uint8 PlayerCache::GetLevel(ObjectGuid guid)
{
    static const uint8 NOT_FOUND = 0;

    ReadLock lock(mutex);
    return do_lookup(cache_, guid, &Entry::level, NOT_FOUND);
}

uint32 PlayerCache::GetZoneId(ObjectGuid guid)
{
    static const uint32 NOT_FOUND = 0;

    ReadLock lock(mutex);
    return do_lookup(cache_, guid, &Entry::zoneId, NOT_FOUND);
}

void PlayerCache::Init()
{
    TC_LOG_INFO("player.cache", "Initializing player cache...");
    uint32 start = getMSTime();
    uint32 count = 0;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_CACHE);
    if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
    {
        do
        {
            Field* fields = result->Fetch();

            Entry entry;
            entry.guid     = ObjectGuid(HighGuid::Player, fields[0].GetUInt32());
            entry.name     = fields[1].GetString();
            entry.account  = fields[2].GetUInt32();
            entry.race     = fields[3].GetUInt8();
            entry.class_   = fields[4].GetUInt8();
            entry.gender   = fields[5].GetUInt8();
            entry.level    = fields[6].GetUInt8();
            entry.at_login = fields[7].GetUInt16();
            entry.guild    = fields[8].IsNull() ? 0 : fields[8].GetUInt32();
            entry.zoneId   = fields[9].GetUInt16();

            cache_.insert(std::move(entry));
            ++count;
        } while (result->NextRow());
    }

    TC_LOG_INFO("player.cache", ">> Loaded %u player cache entries in %u ms", count, GetMSTimeDiffToNow(start));
}

void PlayerCache::Add(Entry entry, bool load_from_db)
{
    bool missing = false;
//    missing = missing || entry.guid == 0;     // mus be provided
    missing = missing || entry.name.empty();
    missing = missing || entry.account == 0;
    missing = missing || entry.race == 0;
    missing = missing || entry.class_ == 0;
    missing = missing || entry.gender == 0;
    missing = missing || entry.level == 0;
    missing = missing || entry.zoneId == 0;
//    missing = missing || entry.at_login == 0; // is a valid state
//    missing = missing || entry.guild == 0;    // is a valid state

    if (load_from_db && missing)
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_CACHE_BY_GUID);
        stmt->setUInt32(0, entry.guid.GetCounter());
        if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
        {
            Field* fields = result->Fetch();

            if (entry.name.empty())
                entry.name     = fields[0].GetString();
            if (entry.account == 0)
                entry.account  = fields[1].GetUInt32();
            if (entry.race == 0)
                entry.race     = fields[2].GetUInt8();
            if (entry.class_ == 0)
                entry.class_   = fields[3].GetUInt8();
            if (entry.gender == 0)
                entry.gender   = fields[4].GetUInt8();
            if (entry.level == 0)
                entry.level    = fields[5].GetUInt8();
            entry.at_login = fields[6].GetUInt16();
            entry.guild = fields[7].IsNull() ? 0 : fields[7].GetUInt32();
            if (entry.zoneId == 0)
                entry.zoneId   = fields[8].GetUInt16();
        }
    }

    WriteLock lock(mutex);
    cache_.insert(std::move(entry));
}

void PlayerCache::Update(const Player* player)
{
    const auto assign = [&player](Entry& entry)
    {
        entry.guid = player->GetGUID();
        entry.name = player->GetName();
        entry.account = player->GetSession() ? player->GetSession()->GetAccountId() : 0;
        entry.race = player->getRace();
        entry.class_ = player->getClass();
        entry.gender = player->getGender();
        entry.level = player->getLevel();
        entry.at_login = player->GetAtLoginFlags();
        entry.guild = player->GetGuildId();
        entry.zoneId = player->GetZoneId();
    };

    WriteLock lock(mutex);
    auto& index =  cache_.get<guid_tag>();
    auto itr = index.find(player->GetGUID());
    if (itr != index.end())
    {
        index.modify(itr, assign);
    }
    else
    {
        Entry entry;
        assign(entry);
        index.insert(std::move(entry));
    }
}

void PlayerCache::Delete(ObjectGuid guid)
{
    WriteLock lock(mutex);
    auto& index = cache_.get<guid_tag>();
    auto itr = index.find(guid);
    if (itr != index.end())
        index.erase(itr);
}

void PlayerCache::UpdateNameData(ObjectGuid guid, const std::string& name, uint8 gender, uint8 race)
{
    WriteLock lock(mutex);
    auto& index = cache_.get<PlayerCache::guid_tag>();
    auto itr = index.find(guid);
    if (itr == index.end())
        return;

    index.modify(itr, [&name, gender, race](PlayerCache::Entry& entry)
    {
        entry.name = name;
        if (gender != GENDER_NONE)
            entry.gender = gender;
        if (race != RACE_NONE)
            entry.race = race;

        entry.at_login &= ~(AT_LOGIN_RENAME | AT_LOGIN_CHANGE_RACE | AT_LOGIN_CHANGE_FACTION);
    });

    WorldPacket data(SMSG_INVALIDATE_PLAYER, 8);
    data << guid;
    sWorld->SendGlobalMessage(data);
}

void PlayerCache::UpdateLevel(ObjectGuid guid, uint8 level)
{
    ReadLock lock(mutex);
    do_update(cache_, guid, &Entry::level, level);
}

void PlayerCache::UpdateAtLogin(ObjectGuid guid, uint16 at_login, PlayerCache::FlagMode mode)
{
    ReadLock lock(mutex);
    do_update(cache_, guid, [mode, at_login](Entry& entry)
    {
        switch (mode)
        {
            case FlagMode::ADD:
                entry.at_login |= at_login;
                break;
            case FlagMode::REMOVE:
                entry.at_login &= ~at_login;
                break;
            case FlagMode::ASSIGN:
                entry.at_login = at_login;
                break;
        }
    });
}

void PlayerCache::UpdateGuild(ObjectGuid guid, uint32 guild)
{
    ReadLock lock(mutex);
    do_update(cache_, guid, &Entry::guild, guild);
}

void PlayerCache::UpdateZoneId(ObjectGuid guid, uint32 zoneId)
{
    ReadLock lock(mutex);

    do_update(cache_, guid, &Entry::zoneId, zoneId);
}

}
