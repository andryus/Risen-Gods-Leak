#pragma once
#include "API.h"
#include <string>

namespace script
{
    /*****************************************
    * ScriptState
    *****************************************/

    class Scriptable;

    struct SCRIPT_API ScriptState
    {
        // needs to be virtual for polymorphism (?)
        virtual ~ScriptState() = 0;

        void SetBase(Scriptable& base);

        Scriptable& GetBase() const;

        virtual std::string Print() const = 0;

    private:

        Scriptable * base = nullptr;
    };
    inline ScriptState::~ScriptState() = default;

}
