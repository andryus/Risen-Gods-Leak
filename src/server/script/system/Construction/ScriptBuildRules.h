#pragma once
#include "ScriptAction.h"
#include "ScriptFilter.h"
#include "ScriptModifier.h"
#include "ScriptEvent.h"
#include "ScriptExecutor.h"
#include "ScriptMacro.h"
#include "ScriptQuery.h"
#include "ScriptBuilder.h"
#include "ScriptEntryPoint.h"

#include "Debug/ScriptHook.h"

namespace script
{
   
    struct IgnoreT {};
    constexpr IgnoreT Ignore = {};

    struct MakeBlock{};
    static constexpr MakeBlock Block; 


    template<class Composite>
    auto operator~(Generator<Composite> composite)
    {
        return makeSwappedComposite(composite);
    }

    template<class Composite>
    auto operator&(Generator<Composite> composite)
    {
        return makePromotedComposite(composite);
    }

    template<class Composite>
    auto operator*(Generator<Composite> composite)
    {
        return makeDemotedComposite(composite);
    }


    template<class EntryPointT, class EntryT, class... Composites>
    auto makeEntryPoint(EntryT entry, Generator<CompositeList<Composites...>> list)
    {
        return wrapFirstListElement<EntryPointType<EntryPointT>::template type>(entry, list);
    }

    template<template<class> class Wrapper, template<class> class GeneratorT, class EventT, class CompositeT>
    auto applyWrapper(CompositeWrapperApplier<Wrapper, GeneratorT, EventT> wrapper, Generator<CompositeT> composite)
    {
        return wrapper.composite.ToGenerator() |= wrapper.Apply(composite.Extract());
    }

    /*************************************
    * List
    **************************************/

    template<class Action1, class Action2>
    auto operator+=(Action<Action1> action, Action<Action2> action2)
    {
        return makeCompositeList(action, action2);
    }

    template<class GetterT, class ActionT>
    auto operator+=(Getter<GetterT> getter, Action<ActionT> action)
    {
        return makeCompositeList(getter, action);
    }

    // block generation
    template<class... Actions>
    auto operator+=(MakeBlock, Action<CompositeList<Actions...>> action)
    {
        return action.MakeGenerator(CompositeWrapper<CompositeList<Actions...>>(action.Extract()));
    }

    template<class ActionT>
    auto operator+=(IgnoreT, Action<ActionT> action)
    {
        return action;
    }

    /*************************************
    * Repeat
    **************************************/

    template<class Event1, class Event2>
    auto operator,(Event<Event1> event1, Event<Event2> event2)
    {
        return makeRepeat(event1.Extract(), event2.Extract());
    }

    template<class... Events, class EventT>
    auto operator,(RepeatComposite<Events...> repeat, Event<EventT> event)
    {
        return repeat.Append(event.Extract());
    }

    template<class Query1, class Query2>
    auto operator,(Query<Query1> query1, Query<Query2> query2)
    {
        return makeRepeat(query1.Extract(), query2.Extract());
    }

    template<class... Queries, class QueryT>
    auto operator,(RepeatComposite<Queries...> repeat, Query<QueryT> query)
    {
        return repeat.Append(query.Extract());
    }

    template<class... Composites, class Other>
    auto operator|=(RepeatComposite<Composites...> repeat, Generator<Other> other)
    {
        const auto doRepeat = [other](auto element)
        {
            return element.ToGenerator() |= other.ToGenerator();
        };

        const auto tuple = repeat.ForEach(doRepeat);

        return other.MakeGenerator(CompositeWrapper(CompositeList(tuple)));
    }

    /*************************************
    * Combination
    **************************************/

    template<class Filter1, class Filter2>
    auto operator&&(Filter<Filter1> filter1, Filter<Filter2> filter2)
    {
        return makeCompositeChain<true>(filter1, filter2);
    }

    template<class Filter1, class Filter2>
    auto operator||(Filter<Filter1> filter1, Filter<Filter2> filter2)
    {
        return makeCompositeChain<false>(filter1, filter2);
    }

    /*************************************
    * Actions
    **************************************/

    template<class FilterT, class ActionT>
    auto operator|=(Action<FilterT> filter, Action<ActionT> action)
    {
        return makeCheckedComposite(filter.Extract(), action);
    }

    template<class Action1, class Action2>
    auto operator|=(ActionReturn<Action1> action1, Action<Action2> action2)
    {
        return makeModifiedComposite<false>(action1.Extract(), action2);
    }

    template<template<class> class Wrapper, class Action1, class Action2>
    auto operator|=(CompositeWrapperApplier<Wrapper, ActionReturn, Action1> action1, Action<Action2> action2)
    {
        return applyWrapper(action1, action2);
    }

    /*************************************
    * Entrypoints
    **************************************/

    // event
    template<class EventT, class ActionT>
    auto operator|=(Event<EventT> event, Action<ActionT> action)
    {
        return makeEntryPoint<EventEntryPoint>(event.Extract(), action);
    }

    template<class QueryT, class ModifierT, bool target>
    auto operator|=(Query<QueryT> query, Modifier<ModifierT, target> modifier)
    {
        return makeEntryPoint<QueryEntryPoint>(query.Extract(), modifier);
    }

    template<template<class> class Wrapper, class EventT, class ActionT>
    auto operator|=(CompositeWrapperApplier<Wrapper, Event, EventT> event, Action<ActionT> action)
    {
        return applyWrapper(event, action);
    }

    template<template<class> class Wrapper, class QueryT, class ModifierT, bool target>
    auto operator|=(CompositeWrapperApplier<Wrapper, Query, QueryT> query, Modifier<ModifierT, target> modifier)
    {
        return applyWrapper(query, modifier);
    }
    
    /*************************************
    * Filters
    **************************************/

    template<class FilterT, class ActionT>
    auto operator|=(Filter<FilterT> filter, Action<ActionT> action)
    {
        return makeCheckedComposite(filter.Extract(), action);
    }

    template<class FilterT, class EventT>
    auto operator|=(Filter<FilterT> filter, Event<EventT> event)
    {
        return makeCheckedComposite(filter.Extract(), event);
    }

    template<class FilterT, class ModifierT, bool target>
    auto operator|=(Filter<FilterT> filter, Modifier<ModifierT, target> modifier)
    {
        return makeCheckedComposite(filter.Extract(), modifier);
    }

    template<class Composite>
    auto operator|=(IgnoreT ignore, Composite composite)
    {
        return composite;
    }

    /*************************************
    * Modifiers
    **************************************/

    template<class ModifierT, class ActionT, bool Target>
    auto operator|=(Modifier<ModifierT, Target> modifier, Action<ActionT> action)
    {
        return makeModifiedComposite<Target>(modifier.Extract(), action);
    }

    template<class ModifierT, class FilterT, bool Target>
    auto operator|=(Modifier<ModifierT, Target> modifier, Filter<FilterT> filter)
    {
        return makeModifiedComposite<Target>(modifier.Extract(), filter);
    }

    template<class ModifierT, class EventT, bool Target>
    auto operator|=(Modifier<ModifierT, Target> modifier, Event<EventT> event)
    {
        return makeModifiedComposite<Target>(modifier.Extract(), event);
    }

    template<class Modifier1, bool Target1, class Modifier2, bool Target2>
    auto operator|=(Modifier<Modifier1, Target1> modifier1, Modifier<Modifier2, Target2> modifier2)
    {
        return makeModifiedComposite<Target1>(modifier1.Extract(), modifier2);
    }

    /*************************************
    * Executors
    **************************************/

    template<class ExecutorT, class ActionT>
    auto operator|=(Executor<ExecutorT> executor, Action<ActionT> action)
    {
        return makeExecutableComposite(executor.Extract(), action);
    }

    /*************************************
    * Macros
    *************************************/

    template<class MacroT, class Composite>
    auto applyMacro(Macro<MacroT> macro, Generator<Composite> composite)
    {
        return macro.Extract()(composite.ToGenerator());
    }

    template<class MacroT, class... Composites>
    auto applyMacro(Macro<MacroT> macro, Generator<CompositeList<Composites...>> list)
    {
        return replaceFirstListElement<true>(macro.Extract(), list);
    }

    template<class MacroT, class CompositeT>
    auto operator|=(Macro<MacroT> macro, Generator<CompositeT> composite)
    {
        return applyMacro(macro, composite);
    }

    template<class MacroT, class CompositeT>
    auto operator %(Macro<MacroT> macro, Generator<CompositeT> composite)
    {
        return applyMacro(macro, composite);
    }

    template<class MacroT>
    auto operator|=(Macro<MacroT> macro, IgnoreT ignore)
    {
        return macro.Extract()(ignore);
    }

    template<class MacroT, class Me>
    auto operator|=(Macro<MacroT> macro, ConstructionCtx<Me> ctx)
    {
        return macro.Extract()(ctx);
    }

    /*************************************
    * Invocation
    *************************************/

    // directly execute action on a target
    template<class Me, class CompositeT>
    decltype(auto) operator<<=(Me& me, Generator<CompositeT> generator)
    {
        static_assert(CanCastTo<typename CompositeT::Me, std::decay_t<Me>>(), "Can't execute composite. Cannot be called on Me-Type (no conversion to expected Me exists).");

        const auto params = makeParams(&me, NoParam{});
        const auto block = startLoggerBlock(params.logger);

        return generator.Extract().Call(params);
    }

    template<class Me, class CompositeT>
    decltype(auto) operator<<=(Me* me, Generator<CompositeT> generator)
    {
        if (me)
            return *me <<= generator;
        else
            return decltype((*me <<= generator))();
    }

    // execute entry-point registration
    template<class Me, class ActionT>
    decltype(auto) operator<<=(ConstructionCtx<Me> me, Action<ActionT>&& action)
    {
        return action.Extract().Register(me);
    }


#define BeginBlock (script::Block
#define EndBlock )


}
