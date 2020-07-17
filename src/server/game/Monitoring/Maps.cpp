#include "Maps.hpp"
#include "Map.h"
#include "DBCStores.h"

using namespace RG::Monitoring;

Maps& Maps::Instance()
{
    static Maps map;
    return map;
}

Maps::Maps() :
        Module("map"),
        family_players_(AddGauge("players", "Current number of player")),
        family_active_objects_(AddGauge("active_objects", "Current number of active objects")),
        family_instances_(AddGauge("instances", "Current number of active instances")),
        family_visibility_distance_(AddGauge("visibility_distance", "Current visibility distance")),
        family_update_duration_(AddSummary("update_duration", "Time distribution for the map update")),
        family_update_latency_(AddSummary("update_latency", "Time between map updates"))
{
    static const auto MapTypeToMap = [](uint32 map_type) -> std::string
    {
        switch (map_type)
        {
            case MAP_COMMON:
                return "world";
            case MAP_INSTANCE:
                return "instance";
            case MAP_RAID:
                return "raid";
            case MAP_BATTLEGROUND:
                return "battleground";
            case MAP_ARENA:
                return "arena";
            default:
                return "<unknown>";
        }
    };

    static const prometheus::Summary::Quantiles quantiles = {
            {0.5, 0.05},
            {0.9, 0.01},
            {0.95, 0.01},
            {1.0, 0.01},
    };

    for (uint32 i = 0; i < sMapStore.GetNumRows(); ++i)
    {
        auto map = sMapStore.LookupEntry(i);
        if (!map)
            continue;

        std::map<std::string, std::string> labels{
                {"id", std::to_string(map->MapID)},
                {"name", map->name[LocaleConstant::LOCALE_enUS]},
                {"type", MapTypeToMap(map->map_type)}
        };

        per_map_counter_.emplace(map->MapID, PerMapCounter{
                family_players_.Add(labels),
                family_active_objects_.Add(labels),
                family_instances_.Add(labels),
                family_visibility_distance_.Add(labels),
                family_update_duration_.Add(labels, quantiles),
                family_update_latency_.Add(labels, quantiles),
        });
    }
}

void Maps::ReportPlayer(Map* map, bool add)
{
    auto& lookup = Instance().per_map_counter_;
    auto itr = lookup.find(static_cast<uint16>(map->GetId()));
    if (itr == lookup.end())
        return;

    auto& counters = itr->second;
    if (add)
        counters.players.Increment();
    else
        counters.players.Decrement();
}

void Maps::ReportActiveObject(Map* map, bool add)
{
    auto& lookup = Instance().per_map_counter_;
    auto itr = lookup.find(static_cast<uint16>(map->GetId()));
    if (itr == lookup.end())
        return;

    auto& counters = itr->second;
    if (add)
        counters.active_objects.Increment();
    else
        counters.active_objects.Decrement();
}

void Maps::ReportInstance(Map* map, bool add)
{
    auto& lookup = Instance().per_map_counter_;
    auto itr = lookup.find(static_cast<uint16>(map->GetId()));
    if (itr == lookup.end())
        return;

    auto& counters = itr->second;
    if (add)
        counters.instances.Increment();
    else
        counters.instances.Decrement();
}

void Maps::ReportVisibilityDistance(Map* map)
{
    auto& lookup = Instance().per_map_counter_;
    auto itr = lookup.find(static_cast<uint16>(map->GetId()));
    if (itr == lookup.end())
        return;

    auto& counters = itr->second;
    counters.visibility_distance.Set(map->GetDynamicVisibilityRange());
}

void Maps::ReportUpdate(Map* map, std::chrono::milliseconds latency, std::chrono::milliseconds duration)
{
    auto& lookup = Instance().per_map_counter_;
    auto itr = lookup.find(static_cast<uint16>(map->GetId()));
    if (itr == lookup.end())
        return;

    PerMapCounter& counters = itr->second;

    counters.update_duration.Observe(duration.count() / 1000.0f);
    counters.update_latency.Observe(latency.count() / 1000.0f);
}
