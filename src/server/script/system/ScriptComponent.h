#pragma once
#include <type_traits>
#include "ScriptTarget.h"

#define SCRIPT_COMPONENT(OWNER, TYPE)                    \
    template<>                                           \
    struct script::ComponentAccess<OWNER, TYPE>          \
    {                                                    \
        TYPE* operator()(OWNER& owner) const;            \
    };                                                   \


#define SCRIPT_COMPONENT_IMPL(OWNER, TYPE) \
    TYPE* script::ComponentAccess<OWNER, TYPE>::operator()(OWNER& owner) const

