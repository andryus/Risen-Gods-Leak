#pragma once
#include <optional>

#include "ScriptInvokable.h"
#include "Utilities/TypeTraits.h"

namespace script
{
    /******************************
    * CompositeWrapper
    ******************************/

    template<class Composite>
    struct CompositeWrapper
    {
        using Me = typename Composite::Me;
        using Param = typename Composite::Param;

        CompositeWrapper(Composite composite) :
            composite(composite) {}

        auto Extract() const { return composite; }

        template<class Context>
        decltype(auto) Call(Params<Context> params)
        {
            return composite.Call(params);
        }

        bool Register(ConstructionCtx<Me> me)
        {
            return composite.Register(me);
        }

        USE_GENERATOR(Composite);

    private:
        Composite composite;
    };

    /******************************
    * CompositeList
    ******************************/

    template<class... Composites>
    struct PackedTypes {};
    template<class CompositeT, class... Chain>
    struct PackedTypes<CompositeT, Chain...>
    {
        using Me = typename CompositeT::Me;
        using Param = typename CompositeT::Param;
        using Composite = CompositeT;
    };

    template<class... Composites>
    struct CompositeList
    {
        using Me = typename PackedTypes<Composites...>::Me;
        using Param = typename PackedTypes<Composites...>::Param;
        using Composite = typename PackedTypes<Composites...>::Composite;

        CompositeList(std::tuple<Composites...> list) :
            list(list) {}

        template<class Context>
        decltype(auto) Call(Params<Context> params)
        {
            return CallList<0>(params);
        }

        template<class MeT, class HookT = DefaultHook>
        bool Register(ConstructionCtx<MeT> ctx, HookT hook = HookT{})
        {
            return RegisterList(ctx, hook, std::index_sequence_for<Composites...>{});
        }

        template<bool UseGenerator, class Functor>
        auto ReplaceElement(Functor functor) const
        {
            auto element = GetFirstElement<UseGenerator>();
            auto replaced = functor(element);

            if constexpr(UseGenerator)
                return ReplaceFirst<Composites...>(replaced.Extract());
            else
                return ReplaceFirst<Composites...>(replaced);
        }

        auto Extract() { return std::move(list); }

        USE_GENERATOR(Composite);

    private:

        template<class MeT, class HookT, size_t... Index>
        bool RegisterList(ConstructionCtx<MeT> ctx, HookT hook, std::index_sequence<Index...>)
        {
            return (std::get<Index>(list).Register(ctx, hook), ...);
        }

        template<bool UseGenerator>
        auto GetFirstElement() const
        {
            if constexpr(UseGenerator)
                return std::get<0>(list).ToGenerator();
            else
                return std::get<0>(list);
        }

        template<class First, class... Rest, class New>
        auto ReplaceFirst(New composite) const
        {
            return CompositeList<New, Rest...>(ReplaceFirstElement(composite, std::index_sequence_for<Rest...>{})).ToGenerator();
        }

        template<class New, size_t ... Index>
        auto ReplaceFirstElement(New composite, std::index_sequence<Index...>) const
        {
            return std::make_tuple(composite, std::get<Index + 1>(list)...);
        }

        template<size_t Index, class Context>
        bool CallList(Params<Context> params)
        {
            if constexpr (Index < sizeof...(Composites))
            {
                auto result = std::get<Index>(list).Call(params);

                constexpr auto eval = [](auto result)
                {
                    return !result;
                };

                bool called = !applyForReturnValue<bool>(result, eval);

                called |= CallList<Index + 1>(params);

                return called;
            }
            else
                return false;
        }

        std::tuple<Composites...> list;
    };

    template<class First, class Second>
    auto makeCompositeList(Generator<First> first, Generator<Second> second)
    {
        return CompositeList<First, Second>(std::make_tuple(first.Extract(), second.Extract())).ToGenerator();
    }

    template<class Next, class... Composites>
    auto makeCompositeList(Generator<Next> next, Generator<CompositeList<Composites...>> list)
    {
        return CompositeList<Next, Composites...>(std::tuple_cat(std::make_tuple(next.Extract()), list.Extract().Extract())).ToGenerator();
    }

    template<bool UseGenerator = false, class Functor, class... Composites>
    auto replaceFirstListElement(Functor functor, Generator<CompositeList<Composites...>> list)
    {
        return list.Extract().template ReplaceElement<UseGenerator>(functor);
    }

    template<template<class, class> class Wrapper, class Composite, class... Composites>
    auto wrapFirstListElement(Composite composite, Generator<CompositeList<Composites...>> list)
    {
        const auto wrap = [composite](auto element)
        {
            return Wrapper<Composite, std::decay_t<decltype(element)>>(composite, element);
        };

        return replaceFirstListElement(wrap, list);
    }

    /******************************
    * CheckedComposite
    *******************************/

    template<class Check, class Composite>
    struct CheckedComposite
    {
        using Me = typename Check::Me;
        using Param = typename Check::Param;

        CheckedComposite(Check check, Composite composite) :
            check(check), composite(composite) { }

        template<class Context>
        auto Call(Params<Context> params)
        {
            if (check.Call(params))
            {
                const auto log = startLoggerBlock(params.logger);

                return composite.Call(params);
            }
            else
            {
                reportLoggerFailure(params.logger, false, "");
                return decltype(composite.Call(params))();
            }
        }

        USE_GENERATOR(Composite);

    private:

        Check check;
        Composite composite;
    };

    template<class Check, class Composite>
    auto makeCheckedComposite(Check check, Generator<Composite> composite)
    {
        return CheckedComposite<Check, Composite>(check, composite.Extract()).ToGenerator();
    }

    template<class Check, class... Composites>
    auto makeCheckedComposite(Check check, Generator<CompositeList<Composites...>> list)
    {
        return wrapFirstListElement<CheckedComposite>(check, list);
    }

    /******************************
    * ModifiedComposite
    *******************************/

    template<class Modifier, class Action, class Context1, class Context2>
    void reportInvalidParams(Params<Context1> oldParams, Params<Context2> newParams)
    {
        static_assert(std::is_invocable_v<decltype(Action::template Call<Context2>), Context2>, "Cannot call action with modified parameters.");
    }

    template<class Modifier, class Composite, bool IsTarget>
    struct ModifiedComposite
    {
        using Me = typename Composite::Me;
        using Param = typename Composite::Param;

        ModifiedComposite(Modifier modifier, Composite composite) :
            modifier(modifier), composite(composite) { }

        template<class Context>
        decltype(auto) Call(Params<Context> params)
        {
            const auto returnValue = modifier.Call(params);
            const auto log = startLoggerBlock(params.logger);

            const auto call = [params, &composite = composite](auto& element)
            {
                const auto newParams = MakeParams(element, params);

                return composite.Call(newParams);
            };

            return applyForReturnValue<typename Composite::Param>(returnValue, call);
        }

        USE_GENERATOR(Composite);

    private:

        template<class Param, class Context>
        static auto MakeParams(Param&& param, Params<Context> params)
        {
            if constexpr(IsTarget)
                return makeParams<std::remove_pointer_t<std::decay_t<Param>>>(param, params.param, params);
            else
                return makeParams(params.target, param, params);
        }

    public:

        Modifier modifier;
        Composite composite;
    };


    template<bool Target>
    struct ModifiedCompositeType
    {
        template<class ModifierT, class Composite>
        using type = ModifiedComposite<ModifierT, Composite, Target>;
    };


    template<bool IsTarget, class Modifier, class Composite>
    auto makeModifiedComposite(Modifier modifier, Generator<Composite> composite)
    {
        return Composite::MakeGenerator(ModifiedComposite<Modifier, Composite, IsTarget>(modifier, composite.Extract()));
    }

    template<bool IsTarget, class Modifier, class... Composites>
    auto makeModifiedComposite(Modifier modifier, Generator<CompositeList<Composites...>> list)
    {
        return wrapFirstListElement<ModifiedCompositeType<IsTarget>::template type>(modifier, list);
    }

    /******************************
    * ExecutableComposite
    *******************************/

    template<class ExecutorT, class Composite>
    struct ExecutableComposite
    {
        using Me = typename Composite::Me;
        using Param = typename Composite::Param;

        ExecutableComposite(ExecutorT executor, Composite composite) :
            executor(std::move(executor)), composite(std::move(composite)) { }

        template<class Context>
        bool Call(Params<Context> params)
        {
            uint64 stateId = -1;
            if (Scriptable* scriptable = script::castSourceTo<Scriptable>(params.target))
                stateId = UndoHandler::GetStateId(*scriptable);

            StoredInvokation<Composite, Context> invocation(composite, *params.target, params.param, stateId);

            auto hook = [&invocation, hook = params.hook](auto& ctx)
            {
                ctx._exec._setInvokation(invocation);

                hook(ctx);
            };

            const auto execParams = makeParams(params.target, params.param, params.logger, hook);

            return executor.Call(execParams);
        }

        USE_GENERATOR(Composite);

    private:

        ExecutorT executor;
        Composite composite;
    };

    template<class Executor, class Composite>
    auto makeExecutableComposite(Executor executor, Generator<Composite> composite)
    {
        return Composite::MakeGenerator(ExecutableComposite<Executor, Composite>(executor, composite.Extract()));
    }

    template<class Executor, class... Composites>
    auto makeExecutableComposite(Executor executor, Generator<CompositeList<Composites...>> list)
    {
        return wrapFirstListElement<ExecutableComposite>(executor, list);
    }

    /******************************
    * RepeatComposite
    ******************************/

    template<class... Composites>
    struct RepeatComposite
    {
        RepeatComposite(Composites... composites) :
            composites(composites...) {}
        RepeatComposite(std::tuple<Composites...> composites) :
            composites(composites) {}

        template<class Functor>
        auto ForEach(Functor functor) const
        {
            return ForEachImpl(functor, std::index_sequence_for<Composites...>{});
        }

        template<class Next>
        auto Append(Next next) const
        {
            return RepeatComposite<Composites..., Next>(std::tuple_cat(composites, std::make_tuple(next)));
        }

    private:

        template<class Functor, size_t... Index>
        auto ForEachImpl(Functor functor, std::index_sequence<Index...>) const
        {
            return std::make_tuple(functor(std::get<Index>(composites)).Extract()...);
        }

        std::tuple<Composites...> composites;
    };

    template<class... Composites>
    auto makeRepeat(Composites... composites)
    {
        return RepeatComposite<Composites...>(composites...);
    }

    /******************************
    * CompositeChain
    ******************************/

    template<bool Op, class... Composites>
    struct CompositeChain
    {
        using Me = typename PackedTypes<Composites...>::Me;
        using Param = typename PackedTypes<Composites...>::Param;
        using Composite = typename PackedTypes<Composites...>::Composite;

        CompositeChain(std::tuple<Composites...> chain) :
            chain(chain) {}

        auto Extract() { return std::move(chain); }

        template<class Context>
        decltype(auto) Call(Params<Context> params)
        {
            return CallChain(params, std::index_sequence_for<Composites...>{});
        }

        USE_GENERATOR(Composite);

    private:

        template<class Context, size_t... Index>
        decltype(auto) CallChain(Params<Context> params, std::index_sequence<Index...>)
        {
            return CallComposite(params, std::get<Index>(chain)...);
        }

        template<class Context, class Composite, class Composite2, class ... Rest>
        static bool CallComposite(Params<Context> params, Composite& composite, Composite2& composite2, Rest... composites)
        {
            const bool result = composite.Call(params);

            if (result == Op)
                return CallComposite(params, composite2, composites...);
            else
                return !Op;
        }

        template<class Context, class Composite>
        static bool CallComposite(Params<Context> params, Composite& composite)
        {
            return composite.Call(params);
        }

        std::tuple<Composites...> chain;
    };

    template<bool Op, class First, class Next>
    auto makeCompositeChain(Generator<First> first, Generator<Next> next)
    {
        return First::MakeGenerator(CompositeChain<Op, First, Next>(std::make_tuple(first.Extract(), next.Extract())));
    }

    template<bool Op, class First, class... Composites, class Next>
    auto makeCompositeChain(Generator<CompositeChain<Op, First, Composites...>> chain, Generator<Next> next)
    {
        return First::MakeGenerator(CompositeChain<Op, First, Composites..., Next>(std::tuple_cat(chain.Extract().Extract(), std::make_tuple(next.Extract()))));
    }

}
