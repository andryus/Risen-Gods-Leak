#pragma once
#include "API/OwnerScript.h"
#include "BaseOwner.h"
#include "Random.h"
#include "ScriptComponent.h"
/**************************************
* ACTIONS
**************************************/

SCRIPT_TEMPLATE_ME_DATA(Action, Do, bool, UseScript, Token, Token, Script)
{
    if (script::Scriptable* scriptable = script::castSourceTo<script::Scriptable>(me))
        return scriptable->InitScript(Script.ScriptName);
    
    return false;
}

/**************************************
* FILTERS
**************************************/

SCRIPT_FILTER_EX1_INLINE(Below, script::Unbound, uint32, value, ResourceParam, resource)
{
    return resource.value <= value;
}

SCRIPT_FILTER_EX1_INLINE(BelowPct, script::Unbound, uint8, pct, ResourceParam, resource)
{
    return resource.pct <= pct;
}

SCRIPT_FILTER_EX1_INLINE(LessEqual, script::Unbound, uint8, value, ResourceParam, resource)
{
    return resource.value <= value;
}

SCRIPT_FILTER_EX1_INLINE(GreaterEqual, script::Unbound, uint8, value, ResourceParam, resource)
{
    return resource.value >= value;
}

SCRIPT_FILTER1_INLINE(BoolTrue, script::Unbound, bool, val)
{
    return val == true;
}

SCRIPT_FILTER_EX_INLINE(Chance, script::Unbound, float, percent)
{
    return frand(0.0f, 1.0f) <= percent;
}

SCRIPT_TEMPLATE_PARAM(Filter, If, bool, ParamValid, ParamValid)
{
    return param != nullptr;
}


/**************************************
* MODIFIER
**************************************/

template<class Element>
using OneOfPair = std::pair<Element, Element>;

SCRIPT_GETTER2_INLINE(FirstCalled, script::Scriptable, bool, bool, called)
{
    const bool wasCalled = called;
    called = true;
    return !wasCalled;
}

SCRIPT_GETTER2_INLINE(IncrementalInt, script::Scriptable, uint8, uint8, callCount)
{
    return callCount++;
}

SCRIPT_GETTER_EX1_INLINE(AddInt, script::Unbound, uint32, uint32, add, uint32, value)
{
    return value + add;
}

SCRIPT_GETTER_EX2_INLINE(WrappedInt, script::Scriptable, uint8, OneOfPair<uint8>, range, uint8, callCount)
{
    if (callCount < range.first)
        callCount = range.first;

   uint8 count = callCount++;
   count -= range.first;
   count %= (range.second - range.first);
   count += range.first;

    return count;
}

SCRIPT_TEMPLATE_DATA(Getter, Get, Element, RandomElement, Element, script::StaticList<Element>, list)
{
    if (list.empty())
        return Element();
    else
        return list[urand(0, list.size() - 1)];
}

SCRIPT_TEMPLATE_DATA_PARAM(Getter, Get, EigtherOr, Element, OneOfPair<Element>, pair, bool, first)
{
    return first ? pair.first : pair.second;
}

SCRIPT_TEMPLATE_DATA(Getter, Get, Element, Constant, Element, Element, value)
{
    return value;
}

SCRIPT_TEMPLATE_ME(Getter, Get, Me*, Me, FromMe)
{
    return &me;
}

SCRIPT_TEMPLATE_PARAM(Selector, Select, Param, Param, FromParam)
{
    return param;
}

SCRIPT_TEMPLATE_PARAM(Getter, Util, Param, ReturnParam, ReturnParam)
{
    return param;
}

SCRIPT_MACRO(Util, ForwardTo, EVENT)
{
    const auto local = EVENT.Extract();
    using CallType = typename decltype(local)::CtxT::CallType;

    if constexpr (decltype(local)::CtxT::IsAction)
        return ~(*EVENT |= script::Action<CallType>(local.data));
    else
        return ~(*EVENT |= script::Getter<CallType>(local.data));
}

SCRIPT_MACRO_TEMPLATE(Util, ReplaceParam, CALL, SELECTOR)
{
    return ~(SELECTOR |= ~CALL); 
}

/**************************************
* EXECUTORS
**************************************/

SCRIPT_EXEC_STATE_INLINE(Once, script::Scriptable, bool, executed)
{
    if (!executed)
    {
        Execute();

        executed = true;
    }
}

SCRIPT_EXEC_EX_STATE_INLINE(Max, script::Scriptable, uint8, numTimes, uint8, count)
{
    if (count < numTimes)
    {
        Execute();

        count++;
    }
}

SCRIPT_EXEC_EX_STATE_INLINE(WhenCalled, script::Scriptable, uint8, numTimes, uint8, count)
{
    if (count < numTimes)
    {
        count++;
        if (count == numTimes)
            Execute();
    }
}
