#include "MoneyLogModule.hpp"
#include "RG/Logging/LogManager.hpp"

#include "Config.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

namespace RG
{
    namespace Logging
    {
        class PlayerLogoutListener : public PlayerScript
        {
        public:
            PlayerLogoutListener() : PlayerScript("rg_logging_money_logout_listener") {}

            void OnLogout(Player* player) override 
            {
                Manager::GetModule<MoneyLogModule>()->RemovePlayer(player->GetGUID().GetCounter());
            }
        };
    }
}

RG::Logging::MoneyLogModule::MoneyLogModule() : 
    LogModule("Money"),
    playerTransactions_()
{
    // the class is keept as a singleton, so this is valid
    new PlayerLogoutListener();
}

void RG::Logging::MoneyLogModule::Log(Player* player, int32 amount, MoneyLogType type, uint32 data)
{
    if (!IsEnabled())
        return;

    if (config.everything || AccountMgr::IsStaffAccount(player->GetSession()->GetSecurity()))
    {
        PreparedStatement* stmt = RGDatabase.GetPreparedStatement(RG_INS_MOD_MONEY_IMMEDIATE);
        stmt->setUInt32(0, player->GetGUID().GetCounter());
        stmt->setInt32(1, amount);
        stmt->setUInt8(2, static_cast<uint8>(type));
        stmt->setUInt32(3, data);
        RGDatabase.Execute(stmt);
    }
    else
    {
        auto itr = playerTransactions_.find(player->GetGUID().GetCounter());
        if (itr == playerTransactions_.end())
            itr = playerTransactions_.insert({ player->GetGUID().GetCounter(), MoneyTransactions(player->GetGUID().GetCounter(), config) }).first;

        MoneyTransactions& trans = itr->second;
        trans.Insert(type, amount, data);
        trans.FlushIfNeeded();
    }
}

void RG::Logging::MoneyLogModule::LoadConfig()
{
    LogModule::LoadConfig();

    config.everything = sConfigMgr->GetBoolDefault(GetOption("Everything").c_str(), false);
    config.interval = sConfigMgr->GetIntDefault(GetOption("Interval").c_str(), 60);
    config.incoming = sConfigMgr->GetIntDefault(GetOption("Incoming").c_str(), 10000) * GOLD;
    config.outgoing = sConfigMgr->GetIntDefault(GetOption("Outgoing").c_str(), 10000) * GOLD;
}

void RG::Logging::MoneyLogModule::RemovePlayer(uint32 guid)
{
    playerTransactions_.erase(guid);
}


RG::Logging::MoneyTransactions::MoneyTransactions(uint32 playerGuid, const detail::MoneyConfig& config) : 
    playerGuid_(playerGuid),
    config_(config),
    startTime(getMSTime()),
    input(0),
    output(0)
{ }

void RG::Logging::MoneyTransactions::Insert(MoneyLogType type, int32 amount, uint32 data)
{
    entries_.push_back({ type, amount, data, time(nullptr), false });

    if (amount > 0)
        input += amount;
    else
        output -= amount;
}

bool RG::Logging::MoneyTransactions::NeedsFlush()
{
    if (input > config_.incoming || output > config_.outgoing)
        return true;

    return false;
}

void RG::Logging::MoneyTransactions::ExpireEntries()
{
    time_t current = time(nullptr);

    while (!entries_.empty())
    {
        const Entry& entry = entries_.front();

        if (current - entry.time > config_.interval)
        {
            if (entry.amount > 0)
                input -= entry.amount;
            else
                output += entry.amount;

            entries_.pop_front();
        }
        else
            break;
    }
}

void RG::Logging::MoneyTransactions::FlushIfNeeded()
{
    ExpireEntries();

    if (!NeedsFlush())
        return;

    SQLTransaction trans = RGDatabase.BeginTransaction();
    PreparedStatement* stmt;

    for (auto& entry : entries_)
    {
        if (entry.logged)
            continue;

        stmt = RGDatabase.GetPreparedStatement(RG_INS_MOD_MONEY);
        stmt->setUInt32(0, entry.time);
        stmt->setUInt32(1, playerGuid_);
        stmt->setInt32(2, entry.amount);
        stmt->setUInt8(3, static_cast<uint8>(entry.type));
        stmt->setUInt32(4, entry.data);

        entry.logged = true;

        trans->Append(stmt);
    }

    RGDatabase.CommitTransaction(trans);
}
