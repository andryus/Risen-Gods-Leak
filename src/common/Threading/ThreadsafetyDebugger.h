#pragma once

#include <array>
#include <limits>

#include "Errors.h"
#include "API.h"
#include "Log.h"

class ThreadsafetyDebugger
{
public:
    enum class Context
    {
        NONE = 0,

        /// Allow in synchronized world thread
        NEEDS_WORLD,

        /// Allow in map update context
        NEEDS_MAP,

        /// Allow more than one locked group
        ALLOW_MULTIPLE_GROUP_LOCKS,

        _COUNT,
    };

    static constexpr uint64 ALLOW_ALL = std::numeric_limits<uint64>::max();

    static COMMON_API void Enable();
    static COMMON_API void Disable();

    static COMMON_API void Allow(Context const f, uint64 const value = ALLOW_ALL);
    static COMMON_API void Disallow(Context const f, uint64 const value = ALLOW_ALL);
    static COMMON_API bool IsAllowed(Context const c);
    static COMMON_API void AssertAllowed(Context const f);
    static COMMON_API void AssertAllowed(Context const f, uint64 const val);

private:
    static constexpr size_t GetIndex(Context c) { return static_cast<size_t>(c); }
    static constexpr uint64 GetMask(Context c) { return 1ull << static_cast<int>(c); }
    static COMMON_API uint64& AllowedValue(Context const c);

    static thread_local bool isEnabled;
    static thread_local uint64 currentAllowedContexts;
    static thread_local std::array<uint64, static_cast<size_t>(Context::_COUNT)> currentAllowedValues;
};

class DeadlockDetection
{
public:
    enum class Index
    {
        NONE = 0,

        /// Locked group count
        GROUP_LOCK,

        _COUNT,
    };

    static COMMON_API void Increase(Index idx);
    static COMMON_API void Decrease(Index idx);

    template<typename Function>
    static void Check(Index idx, Function&& check, char const* message)
    {
#ifdef THREADSAFETY_DEBUG_ENABLE
        if (!check(currentValues[static_cast<size_t>(idx)]))
#   ifdef THREADSAFETY_DEBUG_ASSERT
            Trinity::Assert(__FILE__, __LINE__, __FUNCTION__, "", "DeadlockDetection::Check failed", message);
#   else
            TC_LOG_ERROR("misc", message);
#   endif
#endif
    }

private:
    static thread_local std::array<uint64, static_cast<size_t>(Index::_COUNT)> currentValues;
};
