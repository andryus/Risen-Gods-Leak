#include "ThreadsafetyDebugger.h"

#include "Log.h"

thread_local bool ThreadsafetyDebugger::isEnabled = true;
thread_local uint64 ThreadsafetyDebugger::currentAllowedContexts{};
thread_local std::array<uint64, static_cast<size_t>(ThreadsafetyDebugger::Context::_COUNT)> ThreadsafetyDebugger::currentAllowedValues{};
thread_local std::array<uint64, static_cast<size_t>(DeadlockDetection::Index::_COUNT)> DeadlockDetection::currentValues{};

#ifdef THREADSAFETY_DEBUG_ASSERT
#   define THREADSAFETY_ASSERT ASSERT
#else
#   define THREADSAFETY_ASSERT(cond, ...) do { if (!(cond)) TC_LOG_ERROR("misc", __VA_ARGS__); } while(0)
#endif

void ThreadsafetyDebugger::Enable()
{
    isEnabled = true;
}

void ThreadsafetyDebugger::Disable()
{
    isEnabled = false;
}

void ThreadsafetyDebugger::Allow(Context const context, uint64 const value)
{
#ifdef THREADSAFETY_DEBUG_ENABLE
    if (IsAllowed(context))
    {
        THREADSAFETY_ASSERT(
            AllowedValue(context) == ALLOW_ALL,
            "Tried to allow context " UI64FMTD " but context is already allowed with a specific value.", GetIndex(context)
        );

        if (AllowedValue(context) == ALLOW_ALL)
            AllowedValue(context) = value;
    }
    else
    {
        currentAllowedContexts |= GetMask(context);
        AllowedValue(context) = value;
    }
#endif
}

void ThreadsafetyDebugger::Disallow(Context const context, uint64 const value)
{
#ifdef THREADSAFETY_DEBUG_ENABLE
    uint64& currentValue = AllowedValue(context);

    THREADSAFETY_ASSERT(
        IsAllowed(context),
        "Tried to remove context flag " UI64FMTD " but context does not have flag.", GetIndex(context)
    );
    THREADSAFETY_ASSERT(
        currentValue == value || currentValue == ALLOW_ALL,
        "Tried to remove context flag " UI64FMTD " but context has invalid value " UI64FMTD, GetIndex(context), currentValue
    );

    if (currentValue == value || currentValue == ALLOW_ALL)
    {
        currentAllowedContexts &= ~GetMask(context);
        currentValue = 0;
    }
#endif
}

bool ThreadsafetyDebugger::IsAllowed(Context const context)
{
#ifdef THREADSAFETY_DEBUG_ENABLE
    return (GetMask(context) & currentAllowedContexts) != 0;
#else
    return true;
#endif
}

void ThreadsafetyDebugger::AssertAllowed(Context const context)
{
#ifdef THREADSAFETY_DEBUG_ENABLE
    if (!isEnabled)
        return;

    THREADSAFETY_ASSERT(
        IsAllowed(context),
        "Tried to execute a function needing " UI64FMTD " but context only allows " UI64FMTD, GetIndex(context), ThreadsafetyDebugger::currentAllowedContexts
    );
#endif
}

void ThreadsafetyDebugger::AssertAllowed(Context const context, uint64 const value)
{
#ifdef THREADSAFETY_DEBUG_ENABLE
    if (!isEnabled)
        return;

    AssertAllowed(context);

    uint64& currentValue = AllowedValue(context);
    THREADSAFETY_ASSERT(
        currentValue == ALLOW_ALL || currentValue == value,
        "Tried to execute a function with wrong context value: " UI64FMTD " exepected: " UI64FMTD " at index " UI64FMTD,
        value, currentValue, GetIndex(context)
    );
#endif
}

uint64& ThreadsafetyDebugger::AllowedValue(Context const c)
{
    return currentAllowedValues[GetIndex(c)];
}


void DeadlockDetection::Increase(Index idx)
{
#ifdef THREADSAFETY_DEBUG_ENABLE
    currentValues[static_cast<uint64>(idx)]++;
#endif
}

void DeadlockDetection::Decrease(Index idx)
{
#ifdef THREADSAFETY_DEBUG_ENABLE
    THREADSAFETY_ASSERT(currentValues[static_cast<uint64>(idx)] > 0, "Decreased lock counter below zero at index " UI64FMTD, static_cast<uint64>(idx));
    currentValues[static_cast<uint64>(idx)]--;
#endif
}
