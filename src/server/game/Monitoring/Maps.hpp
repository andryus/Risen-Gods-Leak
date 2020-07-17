#pragma once

#ifdef CONFIG_WITH_PROMETHEUS_MONITORING
#include <Monitoring/Module.hpp>

class Map;

namespace RG { namespace Monitoring {

class Maps : public Module
{
private:
    Maps();

public:
    static Maps& Instance();
    static void ReportPlayer(Map* map, bool add);
    static void ReportActiveObject(Map* map, bool add);
    static void ReportVisibilityDistance(Map* map);
    static void ReportInstance(Map* map, bool add);
    static void ReportUpdate(Map* map, std::chrono::milliseconds latency, std::chrono::milliseconds duration);

private:
    prometheus::Family<prometheus::Gauge>& family_players_;
    prometheus::Family<prometheus::Gauge>& family_active_objects_;
    prometheus::Family<prometheus::Gauge>& family_instances_;
    prometheus::Family<prometheus::Gauge>& family_visibility_distance_;

    prometheus::Family<prometheus::Summary>& family_update_duration_;
    prometheus::Family<prometheus::Summary>& family_update_latency_;

    struct PerMapCounter
    {
        prometheus::Gauge& players;
        prometheus::Gauge& active_objects;
        prometheus::Gauge& instances;
        prometheus::Gauge& visibility_distance;

        prometheus::Summary& update_duration;
        prometheus::Summary& update_latency;
    };
    std::unordered_map<uint16, PerMapCounter> per_map_counter_;
};

}}
#define MONITOR_MAPS(WHAT) ::RG::Monitoring::Maps::WHAT
#else
#define MONITOR_MAPS(WHAT)
#endif
