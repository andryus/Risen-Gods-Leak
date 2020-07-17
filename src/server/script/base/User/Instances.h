#pragma once
#include "Base.h"
#include "InstanceHooks.h"

#define INSTANCE_SCRIPT(NAME) USER_SCRIPT(Instance, NAME)

template<>
struct script::OwnerBundle<Instance> : public script::ScriptBundle<Instance> {};
