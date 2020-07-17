
#include "UpdateTimer.hpp"
#include "Timer.h"

std::chrono::milliseconds UpdateTimer::StartTimer()
{
    const uint32 now = getMSTime();
    const uint32 diff = (startTime == 0 ? 0 : now - startTime);

    latency = latency * decayFactor + float(diff) * (1.f - decayFactor);
    latencySamples[pos] = diff;
    startTime = now;

    return std::chrono::milliseconds(diff);
}

std::chrono::milliseconds UpdateTimer::StopTimer()
{
    const uint32 now = getMSTime();
    const uint32 delta = now - startTime;

    duration = duration * decayFactor + float(delta) * (1.f - decayFactor);
    durationSamples[pos] = delta;

    pos = (pos + 1) % COUNT;

    return std::chrono::milliseconds(delta);
}

UpdateTimer::DiffInfo UpdateTimer::GetInfo() const
{
    DiffInfo info = {0, 0, 0, 0};

    for (size_t i = 0; i < COUNT; ++i) {
        if (info.maxLatency < latencySamples[i])
            info.maxLatency = latencySamples[i];
        if (info.maxDuration < durationSamples[i])
            info.maxDuration = durationSamples[i];
    }
    info.avgLatency = GetDiffAvg();
    info.avgDuration = GetDurationAvg();

    return info;
}
