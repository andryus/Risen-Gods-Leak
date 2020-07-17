#pragma once

#include <chrono>
#include <prometheus/summary.h>

namespace RG { namespace Monitoring {

class AutoLatencyTimer
{
    using Clock = std::chrono::steady_clock;

public:
    explicit AutoLatencyTimer(prometheus::Summary* target) :
            target_(target),
            start_(target ? Clock::now() : Clock::time_point())
    {}

    ~AutoLatencyTimer()
    {
        if (target_)
        {
            auto delta = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start_);
            target_->Observe(delta.count() / 1000.0);
        }
    }

private:
    prometheus::Summary* const target_;
    Clock::time_point const start_;
};

}}
