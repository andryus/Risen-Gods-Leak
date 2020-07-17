#pragma once

#include <memory>
#include <vector>
#include <tuple>

#include "Define.h"
#include "Errors.h"
#include "ScriptUtil.h"
#include "util/meta/TypeChecks.hpp"

namespace script
{
    class InvokationCtx;

    template<class Data>
    struct ScriptStateId {};

    template<class Me>
    struct OwnerBundle 
    {
        static void Create(StateContainer& container, InvokationCtx& ctx) { }
    };

    /****************************
    * Registry
    ****************************/

    template<class Module>
    std::unique_ptr<script::ScriptState> createModule(InvokationCtx& ctx)
    {
        static_assert(util::meta::False<Module>, "Failed to create script module.");

        return nullptr;
    }

    class InvokationCtx;

    struct NoBase;
    template<class Target>
    struct TargetBase;
    
    template<class Target, class Source>
    constexpr bool CanCastTo();
    template<class Module>
    struct ModuleOwner;

    template<class Me>
    struct ModuleFactory
    {
        static StateContainer CreateScriptState(InvokationCtx& ctx)
        {
            StateContainer container;
            
            LoadScriptState(container, ctx);

            return container;
        }

        template<class State>
        static void AddScriptState(StateContainer& container, InvokationCtx& ctx)
        {
            static_assert(CanCastTo<typename ModuleOwner<State>::type, Me>(), "Cannot add module to Me (no conversion to expected owner type exists.)");

            const uint64 stateId = ScriptStateId<State>()();

            ASSERT(!container.count(stateId));

            container.emplace(stateId, createModule<State>(ctx));
        }

    private:

        template<class Other>
        friend struct ModuleFactory;

        static void LoadScriptState(StateContainer& container, InvokationCtx& ctx)
        {
            OwnerBundle<Me>::Create(container, ctx);

            using Base = typename TargetBase<Me>::type;

            if constexpr(!std::is_same_v<Base, NoBase>)
                ModuleFactory<Base>::LoadScriptState(container, ctx);
        }

    };

}
