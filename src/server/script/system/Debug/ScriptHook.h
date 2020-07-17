#pragma once
#include "ScriptParams.h"
#ifdef _WIN64
#include <intrin.h>
#endif

namespace script
{
    
    template<class Functor>
    auto makePreparedBinding(Functor functor, typename Functor::Param param);

    template<class Composite, class HookT>
    struct HookedComposite
    {
        using Me = typename Composite::Me;
        using Param = typename Composite::Param;

        HookedComposite(Composite composite, HookT hook) :
            composite(composite), hook(hook) { }

        template<class Context>
        decltype(auto) Call(Params<Context> params)
        {
            const auto hookedParams = makeParams(params.target, params.param, params.logger, hook);

            return composite.Call(hookedParams);
        }

        auto Bind(Param param) const
        {
            return makePreparedBinding(*this, param);
        }

        Composite Extract() { return composite; }
        HookT Hook() { return hook; };

        auto ToGenerator() const
        {
            return MakeGenerator(*this);
        }

        template<class Other>
        static auto MakeGenerator(Other other)
        {
            return Composite::MakeGenerator(other);
        }

    private:

        Composite composite;
        HookT hook;
    };

    template<class CompositeT, class HookT>
    static auto makeHookedComposite(CompositeT composite, HookT hook)
    {
        return HookedComposite<CompositeT, HookT>(composite, hook).ToGenerator();
    }

}

#ifdef _WIN64
#define _SCRIPT_CODE_BREAK __debugbreak()
#else
#define _SCRIPT_CODE_BREAK __builtin_trap()
#endif

#define _SCRIPT_BREAKPOINT_VAR(NAME) \
    constexpr auto NAME = [](const auto& ctx)  \
    {                                          \
        _SCRIPT_CODE_BREAK;                    \
    };                                         \


#define SCRIPT_BREAKPOINT(NAME)                   \
    namespace Breakpoint                          \
    {                                             \
        constexpr _SCRIPT_BREAKPOINT_VAR(NAME);   \
    }                                             \

#define LocalBreakpoint \
    Hook([](const auto& ctx){ _SCRIPT_CODE_BREAK; })
