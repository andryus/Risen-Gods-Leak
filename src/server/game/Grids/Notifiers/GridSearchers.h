#pragma once

#include <stdint.h>
#include "GridDefines.h"
#include "VisitorTraits.h"

namespace Trinity
{

    template<class Action>
    struct DeferredAction
    {
        using ActionType = std::remove_reference_t<Action>;

        Action action;

        template<class... Args>
        DeferredAction(Args&&... args) :
            action(std::forward<Args>(args)...) { checkVisitorValidity<ActionType>(); }

        template<class Object>
        bool CanVisit(void) const
        {
            return CanVisitObjectType<Object>(action);
        }

        template<class Object>
        static constexpr bool AcceptsObjectType()
        {
            return VisitorAcceptsObjectType<ActionType, Object>;
        }

        template<class Object>
        void Visit(Object& m)
        {
            action.Visit(m);
        }

        bool IsDone() const
        {
            return IsVisitorDone(action);
        }
    };

    // ObjectTypeFilter ensures that only the functions we actually need are getting executed (and even generated at compile-time at all).
    template<class Object, class Action>
    struct ObjectTypeFilter :
        public DeferredAction<Action&>
    {
        using BaseAction = DeferredAction<Action&>;

        static_assert(BaseAction::template AcceptsObjectType<Object>(), "Action::Visit does not accept filtered Object-type.");

        ObjectTypeFilter(Action& action) :
            BaseAction(action) { }

        template<class _Object>
        static constexpr bool AcceptsObjectType()
        {
            return std::is_convertible_v<_Object&, Object&> && BaseAction::template AcceptsObjectType<_Object>();
        }

        template<class _Object>
        void Visit(_Object& m)
        {
            BaseAction::template Visit<_Object>(m);
        }
    };

    // CHECKS => see if object should be visited
    // phase mask
    struct GAME_API PhaseMaskCheck
    {
        uint32 i_phaseMask;

        PhaseMaskCheck(WorldObject const* searcher);

        template<class Object>
        bool operator()(Object* m)
        {
            return m->InSamePhase(i_phaseMask);
        }
    };

    struct MapTypeMaskCheck
    {
        uint32 mapTypeMask;

        MapTypeMaskCheck(uint32 mapTypeMask) :
            mapTypeMask(mapTypeMask) {}

        template<class Object>
        bool CanVisit(void) const
        {
            return visitorSupportByMask<Object>(mapTypeMask);
        }

        template<class Object>
        bool operator()(Object*)
        {
            return true;
        }
    };

    // ACTIONS => act on visited object

    template<class OutObject>
    struct FirstAction
    {
        using Object = std::remove_pointer_t<OutObject>;

        OutObject& object;

        FirstAction(OutObject& object) :
            object(object) {}

        bool IsDone(void) const
        {
            return object;
        }

        void Visit(Object& m)
        {
            assert(!object); // object should only be set once from outside, so assert to ensure minimal workload

            object = &m;
        };
    };

    template<class OutObject>
    struct LastAction
    {
        using Object = std::remove_pointer_t<OutObject>;

        OutObject& object;

        LastAction(OutObject& object) :
            object(object) { }

        void Visit(Object& m)
        {
            object = &m;
        };
    };

    template<class OutContainer>
    struct ContainerAction
    {
        using Object = typename ExtractContainerType<OutContainer>::type;

        OutContainer& objects;
        
        ContainerAction(OutContainer& objects) :
            objects(objects) { }

        void Visit(Object& object)
        {
            objects.push_back(&object);
        };
    };

    template<class Functor>
    struct FunctionAction
    {

        Functor i_functor;

        FunctionAction(Functor functor) :
            i_functor(functor) { checkFunctorValidity<Functor>(); }

        template<class Object>
        static constexpr bool AcceptsObjectType()
        {
            return FunctorAcceptsObjectType<std::remove_reference_t<Functor>, Object>;
        }

        template<class Object>
        void Visit(Object& object)
        {
            i_functor(&object);
        };
    };

    template<class Functor>
    constexpr auto createFunctionAction(Functor& functor)
    {
        return FunctionAction<Functor&>(functor);
    }

    template<class Functor>
    auto createFunctionAction(Functor&& functor)
    {
        return FunctionAction<Functor>(std::move(functor));
    }

    // SEARCHER => helper classes for combining elements

    // checked searcher => action with multiple checks
    template<class Action, typename... Checks>
    struct CheckedSearcher :
        public DeferredAction<Action&>
    {
        using CheckTuple = std::tuple<Checks&...>;

    private:

        template<class Object, size_t... I>
        bool EvalChecks(Object& o, std::index_sequence<I...>) const
        {
            return (std::get<I>(checks)(&o) && ...);
        }

        template<class Object, size_t... I>
        bool EvalCanVisit(std::index_sequence<I...>) const
        {
            return (CanVisitObjectType<Object>(std::get<I>(checks)) && ...);
        }

        template<size_t... I>
        bool EvalIsDone(std::index_sequence<I...>) const
        {
            return (IsVisitorDone(std::get<I>(checks)) || ...);
        }

        template<class Check, class Object>
        static constexpr bool CheckAcceptsObjectType()
        {
            return FunctorAcceptsObjectType<Check, Object>;
        }

        using Sequence = std::index_sequence_for<Checks...>;

    public:

        using BaseAction = DeferredAction<Action&>;

        template<class Object>
        static constexpr bool AcceptsObjectType()
        {
            return BaseAction::template AcceptsObjectType<Object>() && (CheckAcceptsObjectType<Checks, Object>() && ...);
        }

        CheckedSearcher(Action& action, Checks&... checks) :
            BaseAction(action), checks(std::tie(checks...)) { };

        template<class Object>
        bool CanVisit(void) const
        {
            return EvalCanVisit<Object>(Sequence()) && this->BaseAction::template CanVisit<Object>();
        }

        template<class Object>
        void Visit(Object& o)
        {
            if (EvalChecks(o, Sequence()))
                this->BaseAction::Visit(o);
        }

        bool IsDone(void) const
        {
            return EvalIsDone(Sequence()) || this->BaseAction::IsDone();
        }

        CheckTuple checks;
    };

    template<class Action, typename... Checks>
    auto createCheckedSearcher(Action& action, Checks&... checks)
    {
        return CheckedSearcher<Action, Checks...>(action, checks...);
    }

    template<class ObjectType, class Action, class... Checks>
    struct BaseObjectSearcher :
        public DeferredAction<ObjectTypeFilter<ObjectType, CheckedSearcher<Action, PhaseMaskCheck, Checks...>>>
    {
        using BaseAction = DeferredAction<ObjectTypeFilter<ObjectType, CheckedSearcher<Action, PhaseMaskCheck, Checks...>>>;
        using Searcher = CheckedSearcher<Action, PhaseMaskCheck, Checks...>;

        static_assert(BaseAction::template AcceptsObjectType<ObjectType>(), "Visitor/Checks-Chain do not support target ObjectType.");

        PhaseMaskCheck phaseMask;
        Action action;
        Searcher checkedSearcher;

        template<typename ActionArg>
        BaseObjectSearcher(WorldObject const* searcher, ActionArg&& arg, Checks&... checks) :
            BaseAction(checkedSearcher), phaseMask(searcher), action(std::forward<ActionArg>(arg)),
            checkedSearcher(action, phaseMask, checks...) { }
    };

    template<class ObjectType, template<class> class Action, class ActionCtorArg, class... Checks>
    auto createBaseObjectSearcher(WorldObject const* searcher, ActionCtorArg& arg, Checks&... check)
    {
        return BaseObjectSearcher<ObjectType, Action<ActionCtorArg>, Checks...>(searcher, arg, check...);
    }


}
