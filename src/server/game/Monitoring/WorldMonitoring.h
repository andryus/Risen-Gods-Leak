#pragma once

#ifdef CONFIG_WITH_PROMETHEUS_MONITORING
#include <Monitoring/Module.hpp>

namespace RG { namespace Monitoring {

class WorldMonitoring : public Module
{
private:
    WorldMonitoring();

    prometheus::Family<prometheus::Gauge>& family_players_;
    prometheus::Family<prometheus::Gauge>& family_sessions_;

    prometheus::Family<prometheus::Summary>& family_update_latency_;
    prometheus::Family<prometheus::Summary>& family_update_duration_;

    prometheus::Gauge& players;
    prometheus::Gauge& sessions;

    prometheus::Summary& update_latency;
    prometheus::Summary& update_duration;

public:
    static WorldMonitoring& Instance();

    static void ReportPlayer(bool add);
    static void ReportSession(bool add);
    static void ReportUpdate(std::chrono::milliseconds latency, std::chrono::milliseconds duration);
};
}}

#define MONITOR_WORLD(WHAT) ::RG::Monitoring::WorldMonitoring::WHAT
#else
#define MONITOR_WORLD(WHAT)
#endif

