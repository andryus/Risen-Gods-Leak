#pragma once

#include "Define.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class GAME_API ItemIdGenerator
{
public:
    static constexpr uint32 MAX_ID = 0xFFFFFFFE;

    explicit ItemIdGenerator();
    ~ItemIdGenerator();

    void Initialize();
    void Shutdown();

    uint32 Generate();
    uint32 GenerateLower(uint32 ref, bool cache_only = false);
    void Free(uint32 id);
    void Free(const std::vector<uint32>& ids);

    bool IsCacheEnabled() const;
    std::size_t CacheCapacity() const;
    std::size_t CurrentCacheSize() const;
    std::size_t CacheLowThreshold() const;
    std::size_t CacheHighThreshold() const;

private:
    void ResizeCapacity(uint32 target);
    void FillCache();

private:
    uint32 cache_capacity_;                 //!< target capacity of the cache, actual size can be bigger
    uint32 cache_low_threshold_;            //!< try to keep cache size above this bound
    uint32 cache_high_threshold_;           //!< try to keep cache size below this bound
    uint32 cache_load_interval_size_;       //!< load this many IDs at once
    float cache_load_expected_fill_factor_; //!< expected IDs sparsity to scale interval

    uint32 config_capacity_;                //!< configured capacity, actual one may be scaled down
    float config_low_threshold_pct_;        //!< configured low threshold percentage
    float config_high_threshold_pct_;       //!< configured high threshold percentage

    uint32 initial_max_id_;                 //!< max ID at server startup, upper bound for scanning
    uint32 next_consecutive_id_;            //!< next free ID at the end of the sequence
    std::vector<uint32> cache_;             //!< free ID cache of IDs below next_consecutive_id_
    uint32 cache_max_id_;                   //!< Maximum id that could be contained in the cache

    std::unique_ptr<std::thread> cache_filler_;     //!< cache filler working thread
    std::atomic<bool> cache_filler_running_;        //!< running flag
    std::chrono::milliseconds cache_fill_delay_;    //!< delay between two consecutive load operations

    mutable std::mutex cache_lock_;                 //!< lock for ID access
    std::condition_variable cache_low_threshold_reached_;   //!< notifier when reaching low cache size
};
