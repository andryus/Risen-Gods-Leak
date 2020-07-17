#pragma once

#include <string>
#include <mutex>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include "Define.h"
#include "SharedDefines.h"
#include "SpinLock.hpp"
#include "ObjectGuid.h"

class Player;

namespace player
{

class GAME_API PlayerCache
{
    using MutexType = util::SpinLock;
    using ReadLock = std::unique_lock<MutexType>;
    using WriteLock = std::unique_lock<MutexType>;

public:
    struct Entry
    {
        ObjectGuid guid;        // const
        std::string name;
        uint32 account;     // const
        uint8 race;
        uint8 class_;       // const
        uint8 gender;
        uint8 level;
        uint16 at_login;
        uint32 guild;
        uint32 zoneId;


        constexpr explicit operator bool() const { return !guid.IsEmpty(); }
    };

    struct guid_tag {};
    struct name_tag {};

    enum class FlagMode { ADD, REMOVE, ASSIGN };

    using Store = boost::multi_index::multi_index_container<
            Entry,
            boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<
                            boost::multi_index::tag<guid_tag>,
                            boost::multi_index::member<Entry, ObjectGuid, &Entry::guid>
                    >,
                    boost::multi_index::hashed_non_unique<
                            boost::multi_index::tag<name_tag>,
                            boost::multi_index::member<Entry, std::string, &Entry::name>
                    >
            >
    >;

public:
    static bool Exists(ObjectGuid guid);
    static bool Exists(const std::string& name);

    using PlayerCacheEntry = const Entry;

    static PlayerCacheEntry Get(ObjectGuid guid);
    static PlayerCacheEntry Get(const std::string& name);

    static ObjectGuid GetGUID(const std::string& name);
    static const std::string& GetName(ObjectGuid guid);
    static uint32 GetTeam(ObjectGuid guid);
    static uint32 GetAccountId(ObjectGuid guid);
    static uint32 GetAccountId(const std::string& name);
    static uint32 GetGuildId(ObjectGuid guid);
    static uint8  GetLevel(ObjectGuid guid);
    static uint32 GetZoneId(ObjectGuid guid);

    static void UpdateNameData(ObjectGuid guid, const std::string& name, uint8 gender = GENDER_NONE, uint8 race = RACE_NONE);
    static void UpdateLevel(ObjectGuid guid, uint8 level);
    static void UpdateAtLogin(ObjectGuid guid, uint16 at_login, FlagMode mode = FlagMode::ASSIGN);
    static void UpdateGuild(ObjectGuid guid, uint32 guild);
    static void UpdateZoneId(ObjectGuid guid, uint32 zoneId);

    static void Init();
    static void Add(Entry entry, bool load_from_db = false);
    static void Update(const Player* player);
    static void Delete(ObjectGuid guid);

private:
    static MutexType mutex;
    static Store cache_;
};

}
