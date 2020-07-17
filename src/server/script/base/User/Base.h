#pragma once
#include "Base/BaseScript.h"
#include "ScriptCore.h"
#include "BaseOwner.h"
#include "BaseHooks.h"

#include "StateScript.h"
#include "RegularEventScript.h"
#include "TimedEventScript.h"

template<>
struct script::OwnerBundle<script::Scriptable> : 
    public script::ScriptBundle<script::Scriptable, 
               Scripts::States, 
               Scripts::RegularEvent, 
               Scripts::TimedEvent> {};
