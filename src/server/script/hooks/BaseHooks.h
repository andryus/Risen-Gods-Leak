#pragma once
#include "BaseOwner.h"
#include "API/OwnerScript.h"
#include "ScriptModule.h"

// initialisation
SCRIPT_EVENT(Init, script::Scriptable)
SCRIPT_EVENT(UnInit, script::Scriptable)

// event processing
SCRIPT_EVENT_PARAM(ProcessEvents, script::Scriptable, DiffParam, diff);


template<class Me>
bool loadScriptType(Me& me, std::string_view entry)
{
    if (me.LoadScript(me, entry))
    {
        me <<= Fire::Init;

        return true;
    }
    else
        return false;
}

inline void unloadScript(script::Scriptable& base)
{
    if (base.HasScript())
        base <<= Fire::UnInit;

    base.ClearScript();
}

// DEBUG

SCRIPT_EVENT(DebugFinishTimers, script::Scriptable)
