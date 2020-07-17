#pragma once

#include "Define.h"
#include "Connections/CharacterDatabase.h"
#include <vector>
#include <memory>
#include "ObjectGuid.h"

namespace RG { namespace Rebase {

class ItemRebase
{
public:
    static ItemRebase* Instance();

    void Initialize();
    void Start();
    void Stop();
    void LoadConfig();

    void RebaseGuildBankAndMails();
    void RebaseAuctionhouse();

private:
    void Run();

    bool CanContinue() const;
    std::vector<ObjectGuid> GetCharacterBatch(uint32 count);
    bool PrepareCharacter(ObjectGuid guid);
    std::pair<uint64, uint64> ProcessCharacter(ObjectGuid guid);
    void FinishCharacter(ObjectGuid guid);

    void WaitForCache(std::size_t expected_size) const;
    std::size_t ProcessItems(const std::vector<std::pair<uint32, uint32>>& items);
    bool ProcessItem(SQLTransaction& trans, uint32 item, uint32 owner = 0);

private:
    enum PreparedStatements
    {
        CALL_REBASE_ITEM,
        MAX_STATEMENTS
    };
    std::unique_ptr<CharacterDatabaseConnection> connection_;
    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> worker_;

    bool enabled_ = false;
    bool startup_done_ = false;

    enum class OnlineStrategy { RANDOM, HIGHTEST };

    OnlineStrategy online_strategy_;
    uint32 online_batch_size_ = 10;
    std::chrono::milliseconds online_batch_delay_{1000};
    float online_cache_keep_pct_ = 0.3f;
    std::chrono::seconds online_after_startup_delay_{60};
    uint32 online_player_threshold_ = 1000;
    float online_no_progress_slowdown_ = 2;

    std::chrono::seconds offline_max_wait_{10};
    uint32 offline_ah_limit_ = 100000;
    uint32 offline_gb_limit_ = 1000000;
};

}}
