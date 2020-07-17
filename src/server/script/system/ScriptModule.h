#pragma once
#include "ScriptState.h"
#include "util/meta/TypeChecks.hpp"

namespace script
{

    /*****************************************
    * ScriptModule
    *****************************************/

    class InvokationCtx;
    
    template<class Me> 
    struct ConstructionCtx;

    template<class Me>
    struct StoredTarget;

    struct ScriptState;

#define _CREATE_MODULE_FUNC(NAME) std::unique_ptr<script::ScriptState> script::createModule<Scripts::NAME>(InvokationCtx& ctx)

#define SCRIPT_MODULE(NAME, ME)                                       \
    namespace Scripts::Bind                                           \
    {                                                                 \
        struct NAME                                                   \
        {                                                             \
            using MeT = ME;                                           \
            using Ctx = script::ConstructionCtx<ME>;                  \
                                                                      \
            static void Create(Ctx me);                               \
        };                                                            \
    }                                                                 \
                                                                      \
    namespace Scripts                                                 \
    {                                                                 \
        struct NAME;                                                  \
    }                                                                 \
                                                                      \
    template<>                                                        \
    struct script::ScriptStateId<Scripts::NAME>                       \
    {                                                                 \
        uint64 operator()() const;                                    \
    };                                                                \
                                                                      \
    template<>                                                        \
    struct script::ScriptType<Scripts::NAME> : std::true_type {};     \
                                                                      \
    template<>                                                        \
    _CREATE_MODULE_FUNC(NAME);                                        \
                                                                      \
    SCRIPT_PRINTER(Scripts::NAME)                                     \
                                                                      \
    SCRIPT_STORED_TARGET(Scripts::NAME, script::StoredTarget<ME>)     \
                                                                      \
    template<>                                                        \
    struct script::ModuleOwner<Scripts::NAME> { using type = ME; };   \
    template<>                                                        \
    struct script::ModuleAccess<Scripts::NAME>                        \
    {                                                                 \
        Scripts::NAME* operator()(ME& owner) const;                   \
        ME* operator()(Scripts::NAME& module) const;                  \
    };                                                                \

#define SCRIPT_MODULE_IMPL(NAME)                                                        \
                                                                                        \
    uint64 script::ScriptStateId<Scripts::NAME>::operator()() const                     \
    {                                                                                   \
        static const uint64 id = typeid(Scripts::NAME).hash_code();                     \
                                                                                        \
        return id;                                                                      \
    }                                                                                   \
                                                                                        \
    template<>                                                                          \
    _CREATE_MODULE_FUNC(NAME)                                                           \
    {                                                                                   \
        auto state = std::make_unique<Scripts::NAME>();                                 \
                                                                                        \
        Scripts::Bind::NAME::Create(Scripts::Bind::NAME::Ctx(ctx, #NAME));              \
                                                                                        \
        return state;                                                                   \
    }                                                                                   \
                                                                                        \
    SCRIPT_PRINTER_IMPL(Scripts::NAME)                                                  \
    {                                                                                   \
        return std::string(script::print(value.GetBase()));                             \
    }                                                                                   \
                                                                                        \
    Scripts::NAME* script::ModuleAccess<Scripts::NAME>::operator()                      \
        (Scripts::Bind::NAME::MeT& owner) const                                         \
    {                                                                                   \
        Scriptable* scriptable = script::castSourceTo<script::Scriptable>(owner);       \
        ASSERT_DEV(scriptable, "");                                                     \
                                                                                        \
        return scriptable->template GetScriptState<Scripts::NAME>();                    \
    }                                                                                   \
                                                                                        \
    Scripts::Bind::NAME::MeT* script::ModuleAccess<Scripts::NAME>::operator()           \
        (Scripts::NAME& module) const                                                   \
    {                                                                                   \
        return script::castSourceTo<Scripts::Bind::NAME::MeT>(module.GetBase());        \
    }                                                                                   \
                                                                                        \
    void Scripts::Bind::NAME::Create(Ctx me)                                            \


#define SCRIPT_MODULE_STATE_IMPL(NAME) \
    struct Scripts::NAME final : public script::ScriptState

#define SCRIPT_MODULE_PRINT(NAME)          \
    std::string Print() const override     \
    {                                      \
        return #NAME ":\n    " + PrintCustom(); \
    }                                      \
                                           \
    std::string PrintCustom() const        \

#define SCRIPT_MODULE_PRINT_NONE \
    std::string Print() const override { return ""; }

}
