#include "ScriptState.h"
#include "Errors.h"

namespace script
{

    void ScriptState::SetBase(Scriptable & base)
    {
        ASSERT(!this->base);

        this->base = &base;
    }

    Scriptable& ScriptState::GetBase() const
    {
        ASSERT(base);

        return *base;
    }

}
