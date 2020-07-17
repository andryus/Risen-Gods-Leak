#include "ItemIdGenerator.hpp"

#include "Configuration/Config.h"
#include "DatabaseEnv.h"
#include "Threading/ThreadName.hpp"
#include "Utilities/Timer.h"

#include <boost/algorithm/clamp.hpp>

ItemIdGenerator::ItemIdGenerator() :
    cache_capacity_(0),
    cache_low_threshold_(0),
    cache_high_threshold_(0),
    cache_load_interval_size_(100000),
    cache_load_expected_fill_factor_(0.5f),
    next_consecutive_id_(1),
    cache_(0),
    cache_max_id_(0),
    cache_filler_running_(false),
    cache_fill_delay_(std::chrono::seconds(1))
{}

ItemIdGenerator::~ItemIdGenerator()
{
    if (cache_filler_running_)
        Shutdown();
}

void ItemIdGenerator::Initialize()
{
    // select the biggest used ID
    QueryResult result = CharacterDatabase.Query("SELECT MAX(guid) FROM item_instance");
    if (result && !(*result)[0].IsNull())
        initial_max_id_ = (*result)[0].GetUInt32();
    else
        initial_max_id_ = 0;

    next_consecutive_id_ = initial_max_id_ + 1;

    TC_LOG_INFO("server.id.generator", "Initialized item ID generator: %u (%f%%, %f bits)",
            next_consecutive_id_, float(next_consecutive_id_) / MAX_ID * 100.f, std::log2(next_consecutive_id_));

    // read config values
    config_capacity_ = static_cast<uint32>(sConfigMgr->GetIntDefault("RG.Guid.Item.CacheSize", 10000));
    config_low_threshold_pct_ = sConfigMgr->GetFloatDefault("RG.Guid.Item.LowThreshold", 0.5f);
    config_high_threshold_pct_ = sConfigMgr->GetFloatDefault("RG.Guid.Item.HighThreshold", 0.9f);
    cache_load_interval_size_ = static_cast<uint32>(sConfigMgr->GetIntDefault("RG.Guid.Item.LoadIntervalSize", 100000));
    cache_load_expected_fill_factor_ = boost::algorithm::clamp(sConfigMgr->GetFloatDefault("RG.Guid.Item.LoadExpectedFillFactor", 0.5f), 0.f, 0.99f);
    cache_fill_delay_ = std::chrono::milliseconds(sConfigMgr->GetIntDefault("RG.Guid.Item.LoadDelay", 1000));

    // initialize cache subsystem
    ResizeCapacity(initial_max_id_);
    if (IsCacheEnabled())
    {
        cache_.reserve(cache_capacity_);

        cache_max_id_ = 0;   // we are using the cache, so disallow feedback for now
        cache_filler_running_ = true;
        cache_filler_ = std::make_unique<std::thread>(&ItemIdGenerator::FillCache, this);
    }
    else
    {
        // caching is disabled, allow any feedback
        cache_max_id_ = std::numeric_limits<uint32>::max();
    }
}

void ItemIdGenerator::Shutdown()
{
    if (IsCacheEnabled())
    {
        cache_filler_running_ = false;
        cache_low_threshold_reached_.notify_all();

        cache_filler_->join();
    }
}

void ItemIdGenerator::Free(uint32 id)
{
    TC_LOG_TRACE("server.id.generator", "Freeing item ID: %u", id);

    std::lock_guard<std::mutex> lock(cache_lock_);
    if (id > cache_max_id_)
        return;

    cache_.push_back(id);
    std::push_heap(cache_.begin(), cache_.end(), std::greater<>());
}

void ItemIdGenerator::Free(const std::vector<uint32>& ids)
{
    if (ids.empty())
        return;

    TC_LOG_TRACE("server.id.generator", "Freeing multiple item IDs in range: %u to %u (count: " SZFMTD ")", ids.front(), ids.back(), ids.size());

    std::lock_guard<std::mutex> lock(cache_lock_);
    for (uint32 id : ids)
    {
        if (id > cache_max_id_)
            continue;

        cache_.push_back(id);
        std::push_heap(cache_.begin(), cache_.end(), std::greater<>());
    }
}

uint32 ItemIdGenerator::Generate()
{
    auto id = GenerateLower(MAX_ID);
    ASSERT(id != 0 && "Item ID overflow");
    return id;
}

uint32 ItemIdGenerator::GenerateLower(uint32 ref, bool cache_only)
{
    std::lock_guard<std::mutex> lock(cache_lock_);

    bool cache_empty = cache_.empty();

    if (cache_empty)
    {
        if (cache_only)
            return 0;

        const uint32 next = next_consecutive_id_;

        if (next < ref)
        {
            TC_LOG_TRACE("server.id.generator", "Generated new item ID: %u", next);
            ++next_consecutive_id_;
            return next;
        }
        else
            return 0;
    }
    else
    {
        const uint32 next = cache_.front();
        if (next < ref)
        {
            std::pop_heap(cache_.begin(), cache_.end(), std::greater<>());
            cache_.pop_back();

            if (cache_.size() < cache_low_threshold_)
                cache_low_threshold_reached_.notify_one();

            TC_LOG_TRACE("server.id.generator", "Generate new item ID (cached): %u", next);

            return next;
        }
        else
            return 0;
    }
}

void ItemIdGenerator::FillCache()
{
    TC_LOG_INFO("server.id.generator", "Started item ID cache filler");
    util::thread::nameing::SetName("Item-ID-Cacher");

    auto conn_info = *CharacterDatabase.GetConnectionInfo();
    CharacterDatabaseConnection connection(conn_info);
    if (connection.Open() != 0)
    {
        TC_LOG_ERROR("server.id.generator", "Cannot open database connection for item ID cacher");
        return;
    }

    static const std::string gap_search_query(
            "SELECT\n"
            "    guid\n"
            "FROM item_instance\n"
            "WHERE guid BETWEEN %u AND %u\n"
            "ORDER BY guid ASC");

    // current ID of the cache scan
    uint32 current_scan_id = 1;

    const auto load_and_insert = [this, &connection, &current_scan_id]()
    {
        TC_LOG_DEBUG("server.id.generator", "Loading cache data from database...");
        auto start_time = getMSTime();

        std::vector<uint32> free;

        // interval size is scaled via the fill factor
        const uint32 interval_size = static_cast<uint32>(cache_load_interval_size_ * std::min(1.f / (1.f - cache_load_expected_fill_factor_), 10.f));
        // we cannot advance beyond the initial database ID, since we do not want to cause duplicates
        const uint32 interval_end = std::min(initial_max_id_, current_scan_id + interval_size);

        // select used guids...
        if (auto result = QueryResult(connection.Query(Trinity::StringFormat(gap_search_query, current_scan_id, interval_end).c_str())))
        {
            while (result->NextRow())
            {
                // ... and iterate over the gaps in between two usages
                const uint32 next_in_use = (*result)[0].GetUInt32();
                for (; current_scan_id < next_in_use; ++current_scan_id)
                    free.push_back(current_scan_id);

                current_scan_id = next_in_use + 1;
            }
        }

        // fill last gap between last used guid and scan interval end
        // we can include the end because otherwise current scan id would be one past the end
        for (; current_scan_id <= interval_end; ++current_scan_id)
            free.push_back(current_scan_id);

        TC_LOG_DEBUG("server.id.generator", "Loaded " SZFMTD " cache entries from database in %ums", free.size(), GetMSTimeDiffToNow(start_time));

        // no IDs found
        // we can continue if we haven't reached the end yet
        if (free.empty())
            return interval_end < initial_max_id_;

        TC_LOG_DEBUG("server.id.generator", "Inserting new data into cache...");
        start_time = getMSTime();

        // insert the new elements into the cache
        std::size_t size;
        {
            std::lock_guard<std::mutex> lock(cache_lock_);

            auto new_element = cache_.insert(cache_.end(), free.begin(), free.end());
            for (; new_element != cache_.end(); ++new_element)
                std::push_heap(cache_.begin(), new_element, std::greater<>());

            size = cache_.size();
            cache_max_id_ = current_scan_id - 1;
        }

        TC_LOG_DEBUG("server.id.generator", "Insert took %ums, cache size: " SZFMTD ", advanced scan index to: %u",
                GetMSTimeDiffToNow(start_time), size, current_scan_id);

        return true;
    };

    while (cache_filler_running_)
    {
        {
            std::unique_lock<std::mutex> lock(cache_lock_);

            // if we are currently trying to fill the cache, skip the wait
            if (cache_.size() > cache_high_threshold_)
            {
                // wait until cache depletion
                while (cache_filler_running_
                       && cache_.size() >= cache_low_threshold_)
                    cache_low_threshold_reached_.wait(lock);
            }

            if (!cache_filler_running_)
                break;
        }

        bool has_more = load_and_insert();
        if (!has_more)
        {
            // we cannot load more ids from the db, so allow any id in the cache
            cache_max_id_ = std::numeric_limits<uint32>::max();
            TC_LOG_INFO("server.id.generator", "Loaded all possible cache entries, terminating loader thread.");
            break;
        }

        std::this_thread::sleep_for(cache_fill_delay_);
    }

    cache_filler_running_ = false;
}

bool ItemIdGenerator::IsCacheEnabled() const
{
    return cache_capacity_ > 0;
}

void ItemIdGenerator::ResizeCapacity(uint32 target)
{
    cache_capacity_ = std::min(target, config_capacity_);
    cache_low_threshold_ = static_cast<uint32>(config_low_threshold_pct_ * cache_capacity_);
    cache_high_threshold_ = static_cast<uint32>(config_high_threshold_pct_ * cache_capacity_);
}

std::size_t ItemIdGenerator::CacheCapacity() const
{
    return cache_capacity_;
}

std::size_t ItemIdGenerator::CurrentCacheSize() const
{
    std::unique_lock<std::mutex> lock(cache_lock_);
    return cache_.size();
}

std::size_t ItemIdGenerator::CacheLowThreshold() const
{
    return cache_low_threshold_;
}

std::size_t ItemIdGenerator::CacheHighThreshold() const
{
    return cache_high_threshold_;
}
