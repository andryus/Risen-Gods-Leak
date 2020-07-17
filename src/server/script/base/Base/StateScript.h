#pragma once
#include "API/ModuleScript.h"
#include "BaseOwner.h"

///////////////////////////////////////
/// States
///////////////////////////////////////

SCRIPT_MODULE(States, script::Scriptable);


/************************************
* ACTIONS
************************************/

SCRIPT_ACTION(PushState, Scripts::States)
SCRIPT_ACTION(PopState, Scripts::States)

SCRIPT_ACTION(StoreState, Scripts::States)
SCRIPT_ACTION1(RemoveState, Scripts::States, uint64, stateID)

/************************************
* MODIFIERS
************************************/

SCRIPT_GETTER(StateID, Scripts::States, uint64)

SCRIPT_TEMPLATE_ME_DATA(Action, Do, bool, PushStateEvent, Event, Event, REMOVE_EVENT)
{
    me <<= Do::PushState;
    const uint64 stateId = me <<= Get::StateID;
    
    me <<= REMOVE_EVENT 
        |= Do::RemoveState.Bind(stateId);

    return true;
}

SCRIPT_MACRO_TEMPLATE(Exec, StateEvent, CALL, REMOVE_EVENT)
{
    return BeginBlock
        += Do::PushStateEvent(REMOVE_EVENT)
        += CALL
        += Do::StoreState
    EndBlock;
             
}
