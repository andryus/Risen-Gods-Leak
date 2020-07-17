#include "BaseOwner.h"
#include "ScriptAccessor.h"
#include "Random.h"

ScriptHandle::ScriptHandle(script::Scriptable* script)
{
    if (script)
        scriptId = script->GetScriptId();
    else
        scriptId = script::Scriptable::INVALID_ID;
}

script::Scriptable* ScriptHandle::Load() const
{
    return script::ScriptAccessor::GetScriptable(scriptId);
}

std::string ScriptHandle::Print() const
{
    return "Script (" + std::to_string(scriptId) + ')';
}

SCRIPT_PRINTER_IMPL(ResourceParam)
{
    return std::to_string(value.value) + '/' + std::to_string(value.max) + '(' + std::to_string(value.pct) + ')';
}

SCRIPT_PRINTER_IMPL(GameTime)
{
    auto string = std::to_string(value.Milliseconds() / 1000.0f);
    const size_t dot = string.find('.');

    return string.substr(0, dot + 4) + "s";
}

GameTime GameDuration::Get() const
{
    const uint32 time = urand(min.Milliseconds(), max.Milliseconds());

    return GameTime(time);
}

SCRIPT_PRINTER_IMPL(GameDuration)
{
    if (value.min == value.max)
        return script::print(value.min);
    else
        return script::print(value.min) + "-" + script::print(value.max);
}


SCRIPT_COMPONENT_IMPL(script::Scriptable, script::InvokationCtx)
{
    return owner.ScriptCtx();
}
