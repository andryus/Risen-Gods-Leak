#pragma once
#include "ScriptModuleFactory.h"
#include "ScriptModule.h"
#include "ScriptFactory.h"

namespace script
{
    template<class Me>
    struct ConstructionCtx;

    template<class Me, class... Scripts>
    struct ScriptBundle
    {
        static void Create(StateContainer& container, InvokationCtx& ctx)
        {
            (ModuleFactory<Me>::template AddScriptState<Scripts>(container, ctx), ...);
        }
    };

    struct ScriptToken
    {
        std::string_view ScriptName;
    };

#define MAKE_SCRIPT_CLASS_NAME(ME, NAME) ME##NAME
#define MAKE_SCRIPT_NAME(PREFIX, NAME) #PREFIX "_" #NAME

#define _SCRIPT_NAMESPACE(NAME) NAME##s

#define _SCRIPT_CREATE_FUNC(ME, NAME) \
    NAME(script::ConstructionCtx<ME> me)

#define _SCRIPT_REGISTRATION(...) \
    script::Registration<__VA_ARGS__>

#define USER_SCRIPT_BASE(ME, PREFIX, NAME, ...)                             \
    namespace _SCRIPT_NAMESPACE(ME)::Bind                                   \
    {                                                                       \
       void _SCRIPT_CREATE_FUNC(ME, NAME);                                  \
    }                                                                       \
    namespace _SCRIPT_NAMESPACE(ME)::Register                               \
    {                                                                       \
        const auto NAME =                                                   \
            script::Registration<ME>::Create<__VA_ARGS__>(                  \
                Bind::NAME, MAKE_SCRIPT_NAME(PREFIX, NAME));                \
    }                                                                       \
                                                                            \
    namespace _SCRIPT_NAMESPACE(ME)                                         \
    {                                                                       \
        const auto NAME =                                                   \
            script::ScriptToken{MAKE_SCRIPT_NAME(PREFIX, NAME)};            \
    }                                                                       \
                                                                            \
    void _SCRIPT_NAMESPACE(ME)::Bind::_SCRIPT_CREATE_FUNC(ME, NAME)         \

#define USER_SCRIPT(ME, NAME, ...)       \
    USER_SCRIPT_BASE(ME, ME, NAME, __VA_ARGS__)  \
   
}
