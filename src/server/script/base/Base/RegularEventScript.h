#pragma once
#include "API/ModuleScript.h"
#include "BaseOwner.h"

///////////////////////////////////////
/// RegularEvents
///////////////////////////////////////

SCRIPT_MODULE(RegularEvent, script::Scriptable)

/************************************
* EXECUTORS
************************************/

SCRIPT_EXEC(UntilSuccess, Scripts::RegularEvent)
SCRIPT_EXEC(UntilFailure, Scripts::RegularEvent)
