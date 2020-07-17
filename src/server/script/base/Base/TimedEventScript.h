#pragma once

#include <chrono>

#include "API/ModuleScript.h"
#include "Base/StateScript.h"

///////////////////////////////////////
/// TimedEvents
///////////////////////////////////////

SCRIPT_MODULE(TimedEvent, script::Scriptable);
 
/************************************
* EXECUTORS
************************************/

SCRIPT_EXEC_EX(After, Scripts::TimedEvent, GameDuration, duration);
SCRIPT_EXEC_EX(Every, Scripts::TimedEvent, GameDuration, duration);

SCRIPT_MACRO_DATA(Exec, TempState, CALL, GameDuration, duration)
{
    return BeginBlock
        += Do::PushState
        += CALL 
        += Get::StateID |= BeginBlock
            += Do::StoreState 
            += Exec::After(duration) 
                |= Do::RemoveState
        EndBlock
    EndBlock;
        // ^ Do::PopState; @todo: reimplement // if the call failed, state is undone immediately  
}