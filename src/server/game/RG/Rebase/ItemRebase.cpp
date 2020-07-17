#include "ItemRebase.hpp"

#include "Configuration/Config.h"
#include "DatabaseEnv.h"
#include "Entities/Player/Player.h"
#include "Entities/Player/PlayerCache.hpp"
#include "Globals/ObjectAccessor.h"
#include "Globals/ObjectMgr.h"
#include "Scripting/ScriptMgr.h"
#include "Threading/ThreadName.hpp"
#include "GameTime.h"

using namespace RG::Rebase;

void ItemRebase::Initialize()
{
    LoadConfig();
    startup_done_ = true;

    if (enabled_)
    {
        auto conn_info = *CharacterDatabase.GetConnectionInfo();
        connection_    = std::make_unique<CharacterDatabaseConnection>(conn_info);
        if (connection_->Open() != 0)
        {
            TC_LOG_ERROR("server.rebase.item", "Unable to open database connection for item rebase, disabling feature.");
            enabled_ = false;
            return;
        }

        connection_->m_stmts.resize(MAX_STATEMENTS);
        connection_->PrepareStatement(CALL_REBASE_ITEM, "CALL RebaseItem(?, ?, ?)", CONNECTION_BOTH);
        if (connection_->m_prepareError)
        {
            TC_LOG_ERROR("server.rebase.item", "Unable to prepare sql statements, disabling feature.");
            enabled_ = false;
            return;
        }
    }
}

void ItemRebase::LoadConfig()
{
    bool old_enabled = enabled_;
    enabled_        = sConfigMgr->GetBoolDefault("RG.Rebase.Item.Enabled", false);

    std::string strategy = sConfigMgr->GetStringDefault("RG.Rebase.Item.Online.Strategy", "random");
    if (strategy == "highest")
        online_strategy_ = OnlineStrategy::HIGHTEST;
    else if (strategy == "random")
        online_strategy_ = OnlineStrategy::RANDOM;
    else
        online_strategy_ = OnlineStrategy::RANDOM;

    online_batch_size_     = static_cast<uint32>(sConfigMgr->GetIntDefault("RG.Rebase.Item.Online.BatchSize", 10));
    online_batch_delay_    = std::chrono::milliseconds(sConfigMgr->GetIntDefault("RG.Rebase.Item.Online.Delay", 1000));
    online_cache_keep_pct_ = sConfigMgr->GetFloatDefault("RG.Rebase.Item.CacheKeepPct", 0.3f);
    online_after_startup_delay_ = std::chrono::seconds(sConfigMgr->GetIntDefault("RG.Rebase.Item.Online.AfterStartupDelay", 60));
    online_player_threshold_ = static_cast<uint32>(sConfigMgr->GetIntDefault("RG.Rebase.Item.Online.PlayerThreshold", 1000));
    online_no_progress_slowdown_ = sConfigMgr->GetFloatDefault("RG.Rebase.Item.Online.NoProgressSlowdown", 2);

    offline_max_wait_ = std::chrono::seconds(sConfigMgr->GetIntDefault("RG.Rebase.Item.Offline.MaxWait", 10));
    offline_ah_limit_ = static_cast<uint32>(sConfigMgr->GetIntDefault("RG.Rebase.Item.Offline.Auctionhouse.Limit", 100000));
    offline_gb_limit_ = static_cast<uint32>(sConfigMgr->GetIntDefault("RG.Rebase.Item.Offline.GuildBank.Limit", 1000000));

    if (startup_done_)
    {
        if (enabled_ && !old_enabled)
            Start();
        else if (!enabled_ && old_enabled)
            Stop();
    }
}

void ItemRebase::Start()
{
    if (!enabled_ || running_)
        return;

    TC_LOG_INFO("server.rebase.item", "Starting item rebase worker");
    running_ = true;
    worker_  = std::make_unique<std::thread>(&ItemRebase::Run, this);
}

void ItemRebase::Stop()
{
    if (!enabled_ || !running_)
        return;

    TC_LOG_INFO("server.rebase.item", "Stopping item rebase worker");
    running_ = false;
    if (worker_)
        worker_->join();

    connection_.reset();
}

std::vector<ObjectGuid> ItemRebase::GetCharacterBatch(uint32 count)
{
    std::vector<ObjectGuid> batch;

    static const std::string highest_query(
            "SELECT\n"
            "    DISTINCT (SELECT c.guid FROM characters AS c WHERE c.guid = ii.owner_guid AND c.online = 0) AS guid\n"
            "FROM item_instance AS ii\n"
            "WHERE ii.owner_guid != 0\n"
            "ORDER BY ii.guid DESC\n"
            "LIMIT %u");
    static const std::string random_query(
            "SELECT\n"
            "    guid\n"
            "FROM characters\n"
            "WHERE online = 0\n"
            "ORDER BY RAND()\n"
            "LIMIT %u");

    std::string query;
    switch (online_strategy_)
    {
        default:
        case OnlineStrategy::RANDOM:
            query = Trinity::StringFormat(random_query, count);
            break;
        case OnlineStrategy::HIGHTEST:
            query = Trinity::StringFormat(highest_query, count);
            break;
    }

    if (auto result = QueryResult(connection_->Query(query.c_str())))
        while (result->NextRow())
        {
            auto& field = (*result)[0];
            if (!field.IsNull())
                batch.push_back(ObjectGuid(HighGuid::Player, field.GetUInt32()));
        }

    return batch;
}

bool ItemRebase::PrepareCharacter(ObjectGuid guid)
{
    Player::LockCharacter(guid);
    return true;
}

void ItemRebase::FinishCharacter(ObjectGuid guid)
{
    Player::UnlockCharacter(guid);
}

std::pair<uint64, uint64> ItemRebase::ProcessCharacter(ObjectGuid guid)
{
    // check if player is online despite being locked
    if (ObjectAccessor::FindConnectedPlayer(guid))
        return std::make_pair(0, 0);

    static const std::string query(
            "SELECT\n"
            "    ii.guid\n"
            "FROM item_instance AS ii\n"
            "LEFT JOIN auctionhouse AS a ON (ii.guid = a.itemguid)\n"
            "WHERE a.id IS NULL AND ii.owner_guid = %u\n"
            "ORDER BY ii.guid DESC");

    std::vector<uint32> items;
    if (auto result = QueryResult(connection_->Query(Trinity::StringFormat(query, guid.GetCounter()).c_str())))
        while (result->NextRow())
            items.push_back((*result)[0].GetUInt32());

    if (items.empty())
        return std::make_pair(0, 0);

    SQLTransaction trans = std::make_shared<Transaction>();

    std::vector<uint32> freed;
    for (uint32 item : items)
    {
        if (ProcessItem(trans, item, guid.GetCounter()))
            freed.push_back(item);
    }

    connection_->ExecuteTransaction(trans);

    // we can't free this during the batch to avoid possible reuse before the actual commit
    sObjectMgr->GetItemIdGenerator().Free(freed);

    return std::make_pair(freed.size(), items.size());
}

bool ItemRebase::ProcessItem(SQLTransaction& trans, uint32 item, uint32 owner)
{
    const uint32 new_guid = sObjectMgr->GetItemIdGenerator().GenerateLower(item, true);
    if (new_guid == 0)
        return false;

    // delete is handled by the driver
    auto stmt = new PreparedStatement(CALL_REBASE_ITEM);
    stmt->setUInt32(0, item);
    stmt->setUInt32(1, new_guid);
    stmt->setUInt32(2, owner);
    trans->Append(stmt);

    return true;
}

bool ItemRebase::CanContinue() const
{
    if (std::chrono::seconds(game::time::GetUptime()) < online_after_startup_delay_)
        return false;

    if (sWorld->GetPlayerCount() >= online_player_threshold_)
        return false;

    const auto& generator = sObjectMgr->GetItemIdGenerator();
    return generator.CurrentCacheSize() > generator.CacheCapacity() * online_cache_keep_pct_;
}

void ItemRebase::Run()
{
    util::thread::nameing::SetName("Item-Rebaser");

    uint32 consecutive_no_progress = 0;

    while (running_ && enabled_)
    {
        if (!CanContinue())
        {
            std::this_thread::sleep_for(online_batch_delay_ * 10);
            continue;
        }

        auto start = getMSTime();
        TC_LOG_INFO("server.rebase.item", "Start rebasing item batch (size: %u)", online_batch_size_);

        auto batch = GetCharacterBatch(online_batch_size_);

        uint64 batch_count = 0;
        uint64 total_count = 0;

        std::for_each(batch.begin(), batch.end(), [this](ObjectGuid guid) { PrepareCharacter(guid); });
        for (auto guid : batch)
        {
            auto pair = ProcessCharacter(guid);
            batch_count += pair.first;
            total_count += pair.second;
        }
        std::for_each(batch.begin(), batch.end(), [this](ObjectGuid guid) { FinishCharacter(guid); });

        auto delta = GetMSTimeDiffToNow(start);
        TC_LOG_INFO("server.rebase.item", "Finished rebasing " UI64FMTD " of " UI64FMTD " items in %u ms",
                batch_count, total_count, delta);

        if (batch_count == 0)
            consecutive_no_progress = std::min(25u, consecutive_no_progress + 1);
        else
            consecutive_no_progress = 0;

        std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(online_batch_delay_ * (1 + online_no_progress_slowdown_ * consecutive_no_progress)));
    }
}

ItemRebase* ItemRebase::Instance()
{
    static ItemRebase instance;
    return &instance;
}

void ItemRebase::RebaseAuctionhouse()
{
    if (!enabled_)
        return;

    TC_LOG_INFO("server.rebase.item", "Starting auctionhouse rebase...");
    auto start_time = getMSTime();

    static const std::string query(
            "SELECT\n"
            "    itemguid, itemowner\n"
            "FROM auctionhouse\n"
            "ORDER BY itemguid DESC\n"
            "LIMIT %u");

    std::vector<std::pair<uint32, uint32>> items;
    if (auto result = QueryResult(connection_->Query(Trinity::StringFormat(query, offline_ah_limit_).c_str())))
        while (result->NextRow())
            items.push_back(std::make_pair((*result)[0].GetUInt32(), (*result)[1].GetUInt32()));

    WaitForCache(items.size());
    auto updated = ProcessItems(items);

    TC_LOG_INFO("server.rebase.item", "Rebased " SZFMTD " of " SZFMTD " auctionhouse items in %ums",
            updated, items.size(), GetMSTimeDiffToNow(start_time));
}

void ItemRebase::RebaseGuildBankAndMails()
{
    if (!enabled_)
        return;

    TC_LOG_INFO("server.rebase.item", "Starting guild bank rebase...");
    auto start_time = getMSTime();

    static const std::string query(
            "SELECT\n"
            "    guid\n"
            "FROM item_instance\n"
            "WHERE owner_guid = 0\n"
            "ORDER BY guid DESC\n"
            "LIMIT %u");

    std::vector<std::pair<uint32, uint32>> items;
    if (auto result = QueryResult(connection_->Query(Trinity::StringFormat(query,offline_gb_limit_).c_str())))
        while (result->NextRow())
            items.push_back(std::make_pair((*result)[0].GetUInt32(), 0));

    WaitForCache(items.size());
    auto updated = ProcessItems(items);

    TC_LOG_INFO("server.rebase.item", "Rebased " SZFMTD " of " SZFMTD " guild bank items in %ums",
            updated, items.size(), GetMSTimeDiffToNow(start_time));
}

void ItemRebase::WaitForCache(std::size_t expected_size) const
{
    auto start_time = std::chrono::steady_clock::now();

    const auto& generator = sObjectMgr->GetItemIdGenerator();

    std::size_t size = generator.CurrentCacheSize();
    while (size < expected_size
           && size < generator.CacheHighThreshold()
           && (std::chrono::steady_clock::now() - start_time) < offline_max_wait_)
    {
        std::this_thread::yield();
        size = generator.CurrentCacheSize();
    }
}

std::size_t ItemRebase::ProcessItems(const std::vector<std::pair<uint32, uint32>>& items)
{
    SQLTransaction trans = std::make_shared<Transaction>();

    std::vector<uint32> freed;
    for (auto& pair : items)
    {
        if (ProcessItem(trans, pair.first, pair.second))
            freed.push_back(pair.first);
    }

    connection_->ExecuteTransaction(trans);
    sObjectMgr->GetItemIdGenerator().Free(freed);

    return freed.size();
}

namespace RG { namespace Rebase {

class ItemRebaseWorldHook : public WorldScript
{
public:
    ItemRebaseWorldHook() : WorldScript("rg_rebase_item_rebase_world_hook") {}

    void OnConfigLoad(bool reload)
    {
        if (reload)
            ItemRebase::Instance()->LoadConfig();
    }

    void OnLoad() override
    {
        ItemRebase::Instance()->Start();
    }

    void OnShutdown() override 
    {
        ItemRebase::Instance()->Stop();
    }
};

void AddSC_rg_rebase_item()
{
    new ItemRebaseWorldHook();
}

}}
