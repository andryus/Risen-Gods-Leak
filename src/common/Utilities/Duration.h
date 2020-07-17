#pragma once

#include <chrono>

using Milliseconds = std::chrono::milliseconds;
using Seconds      = std::chrono::seconds;
using Minutes      = std::chrono::minutes;
using Hours        = std::chrono::hours;

using std::chrono::duration_cast;

using namespace std::chrono_literals; /// Makes time literals (s, min, h, ...) globally available


using SystemClock = std::chrono::system_clock;
using TimePoint   = SystemClock::time_point;
using Duration    = SystemClock::duration;

inline time_t ToUnixTime(TimePoint t)
{
    return SystemClock::to_time_t(t);
}

inline TimePoint FromUnixTime(time_t t)
{
    return SystemClock::from_time_t(t);
}
