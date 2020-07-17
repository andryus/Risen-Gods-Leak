#include "ServerStatisticsModule.hpp"

#include "RG/Logging/LogManager.hpp"
#include "Config.h"
#include "ScriptMgr.h"
#include "GameTime.h"

namespace RG
{
    namespace Logging
    {
        class PlayerStatisticsCollector : public WorldScript
        {
        public:
            PlayerStatisticsCollector() : WorldScript("rg_logging_server_statistics_collector") {}

            void OnUpdate(uint32 diff) override
            {
                Manager::GetModule<ServerStatisticsModule>()->Update(diff);
            }
        };
    }
}

RG::Logging::ServerStatisticsModule::ServerStatisticsModule() :
    LogModule("ServerStatistics"),
    timer_(),
    updatediff({ 0, 0, 0 })
{
    // Module handled as singleton
    new PlayerStatisticsCollector();
}

void RG::Logging::ServerStatisticsModule::LoadConfig()
{
    LogModule::LoadConfig();

    config.interval = static_cast<uint32>(sConfigMgr->GetIntDefault(GetOption("Interval").c_str(), 5 * MINUTE)) * IN_MILLISECONDS;

    timer_.SetInterval(config.interval);
}

void RG::Logging::ServerStatisticsModule::Update(uint32 diff)
{
    if (!IsEnabled())
        return;

    updatediff.counter++;
    updatediff.sum += diff;
    updatediff.peak = std::max(updatediff.peak, diff);

    timer_.Update(diff);
    if (!timer_.Passed())
        return;

    timer_.Reset();

    CollectData();
}

void RG::Logging::ServerStatisticsModule::CollectData()
{
    uint32 players  = sWorld->GetPlayerCount();
    uint32 sessions = sWorld->GetActiveSessionCount();
    uint32 queue    = sWorld->GetQueuedSessionCount();
    uint32 uptime   = game::time::GetUptime();

    PreparedStatement* stmt = RGDatabase.GetPreparedStatement(RG_INS_PLAYER_STATISTICS);
    stmt->setUInt32(0, uptime);
    stmt->setUInt32(1, players);
    stmt->setUInt32(2, sessions);
    stmt->setUInt32(3, queue);
    RGDatabase.Execute(stmt);

    if (updatediff.counter > 0) // prevent division by zero
    { 
        uint32 avgUptimeDiff = updatediff.sum / updatediff.counter;
        stmt = RGDatabase.GetPreparedStatement(RG_INS_SERVER_STATISTICS);
        stmt->setUInt32(0, avgUptimeDiff);
        stmt->setUInt32(1, updatediff.peak);
        RGDatabase.Execute(stmt);
    }

    updatediff.counter = 0;
    updatediff.sum = 0;
    updatediff.peak = 0;
}