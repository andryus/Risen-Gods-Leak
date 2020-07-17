#pragma once

#include <chrono>

namespace RG { namespace Monitoring {

class LatencyTimer
{
    using Clock = std::chrono::steady_clock;

public:
    void Start()
    {
        start_ = Clock::now();
    }

    auto Stop()
    {
        return Clock::now() - start_;
    }

private:
    Clock::time_point start_{};
};

}}
