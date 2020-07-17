
#pragma once

#include "Define.h"
#include <array>
#include <chrono>

class COMMON_API UpdateTimer
{
public:
    static constexpr float WORLD_DECAY = 0.5f;
    static constexpr float MAP_DECAY = 0.9f;

    struct DiffInfo { uint32 maxDuration, avgDuration, maxLatency, avgLatency; };

    UpdateTimer(float decay) : decayFactor(decay), startTime(0), duration(0.f), latency(0.f), pos(0) { durationSamples.fill(0); latencySamples.fill(0); };

    std::chrono::milliseconds StartTimer();
    std::chrono::milliseconds StopTimer();

    DiffInfo GetInfo() const;

    uint32 GetStartTime() const { return startTime; }
    float GetDiffAvg() const { return latency; }
    float GetDurationAvg() const { return duration; }
private:
    static const uint32 COUNT = 128;
    const float decayFactor;

    uint32 startTime;

    uint8 pos;
    std::array<uint32, COUNT> durationSamples;
    std::array<uint32, COUNT> latencySamples;
    float duration;
    float latency;


};
