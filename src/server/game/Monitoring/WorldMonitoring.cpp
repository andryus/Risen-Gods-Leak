#include "Monitoring/WorldMonitoring.h"

using namespace RG::Monitoring;

static const prometheus::Summary::Quantiles QUANTILES = {
        {0.5, 0.05},
        {0.9, 0.01},
        {0.95, 0.01},
        {1.0, 0.01},
};

WorldMonitoring& WorldMonitoring::Instance()
{
    static WorldMonitoring wm;
    return wm;
}

WorldMonitoring::WorldMonitoring() :
    Module("world"),
    family_players_(AddGauge("players", "Current number of player")),
    players(family_players_.Add({})),
    family_sessions_(AddGauge("sessions", "Current number of connected clients")),
    sessions(family_sessions_.Add({})),
    family_update_latency_(AddSummary("update_latency", "Time between two world updates")),
    update_latency(family_update_latency_.Add({}, QUANTILES)),
    family_update_duration_(AddSummary("update_duration", "Duration of a world update")),
    update_duration(family_update_duration_.Add({}, QUANTILES))
{}

void WorldMonitoring::ReportPlayer(bool add)
{
    if (add)
        Instance().players.Increment();
    else
        Instance().players.Decrement();
}
void WorldMonitoring::ReportSession(bool add)
{
    if (add)
        Instance().sessions.Increment();
    else
        Instance().sessions.Decrement();
}

void WorldMonitoring::ReportUpdate(std::chrono::milliseconds latency, std::chrono::milliseconds duration)
{
    Instance().update_latency.Observe(latency.count() / 1000.0f);
    Instance().update_duration.Observe(duration.count() / 1000.0f);
}
