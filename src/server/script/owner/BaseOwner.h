#pragma once
#include "API/OwnerScript.h"
#include "ScriptComponent.h"

///////////////////////////////////////
/// BaseScript
///////////////////////////////////////

namespace script { class Scriptable; }

struct ScriptHandle
{
    using Stored = script::Scriptable;

    ScriptHandle() = default;
    ScriptHandle(script::Scriptable& script) :
        ScriptHandle(&script) {}
    ScriptHandle(script::Scriptable* script);
    
    script::Scriptable* Load() const;

    std::string Print() const;

private:

    uint64 scriptId = 0;
};

SCRIPT_STORED_TARGET(script::Scriptable, ScriptHandle);

template<class Type>
struct script::TypePrinter<Type, std::enable_if_t<script::CanCastTo<script::Scriptable, Type>()>>
{
    std::string_view operator()(const Type& object) const
    {
        if (auto* base = script::castSourceTo<script::Scriptable>(const_cast<Type&>(object)))
            return base->FullName();
        else
            return "<INCOMPATBILE>";
    }
};

/**************************************
* PARAMS
**************************************/

struct ResourceParam
{
    ResourceParam() = default;
    ResourceParam(uint32 value) :
        ResourceParam(value, value) {}
    ResourceParam(uint32 value, uint32 max) :
        value(value), max(max), pct(max ? ((value * 100) / max) : 0.0f) { }

    uint32 value;
    uint32 max;
    uint8 pct;
};

SCRIPT_PRINTER(ResourceParam);

struct GameTime
{
    using Time = std::chrono::milliseconds;

    constexpr GameTime() = default;
    constexpr GameTime(Time time) :
        time(time) {}
    template<class Type, class Interval>
    constexpr GameTime(std::chrono::duration<Type, Interval> time) :
        time(std::chrono::duration_cast<Time>(time)) {}
    constexpr explicit GameTime(uint32 time) :
        time(std::chrono::milliseconds(time)) {}

    explicit constexpr operator bool() const { return time != std::chrono::milliseconds(); }

    constexpr bool operator==(GameTime time) const { return this->time == time.time; }
    constexpr bool operator!=(GameTime time) const { return this->time != time.time; }

    constexpr uint32 Milliseconds() const { return time.count(); }

    std::chrono::milliseconds time = {};
};

struct GameDuration
{
    constexpr GameDuration() = default;
    template<class Type, class Interval>
    constexpr GameDuration(std::chrono::duration<Type, Interval> time) :
        GameDuration(GameTime(time)) {}
    constexpr GameDuration(GameTime time) :
        GameDuration(time, time) {}
    constexpr GameDuration(GameTime min, GameTime max) :
        min(min), max(max) {}

    explicit constexpr operator bool() const { return min || max; }

    constexpr bool operator==(GameDuration duration) const { return min == duration.min && max == duration.max; }
    constexpr bool operator!=(GameDuration duration) const { return min != duration.min || max != duration.max; }

    GameTime Get() const;

    GameTime min;
    GameTime max;
};

using DiffParam = GameTime;

SCRIPT_PRINTER(GameTime);
SCRIPT_PRINTER(GameDuration);

SCRIPT_COMPONENT(script::Scriptable, script::InvokationCtx);
