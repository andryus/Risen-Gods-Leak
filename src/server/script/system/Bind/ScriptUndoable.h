#pragma once
#include "StoredInvocation.h"

namespace script
{

    /****************************************
    * StoredUndo
    *****************************************/

    template<class Functor, class Context>
    struct StoredUndoInvokation final :
        public StoredInvokation<Functor, Context>
    {
        using StoredInvokation<Functor, Context>::StoredInvokation;

        bool Invoke(ScriptLogger* logger) override
        {
            StoredInvokation<Functor, Context>::functor.canUndo = false;

            return StoredInvokation<Functor, Context>::Invoke(logger);
        }

        std::unique_ptr<InvokationInterface> Clone() const override
        {
            return std::make_unique<StoredUndoInvokation<Functor, Context>>(*this);
        }
    };

    template<class Param, class Functor, class Target>
    InvocationInterfacePtr makeStoredUndo(Functor&& functor, Target* target, Param param)
    {
        using ContextT = Context<Target, Param>;

        return std::make_unique<StoredUndoInvokation<std::decay_t<Functor>, ContextT>>(std::move(functor), *target, param, -1);
    }

    // Call to request storage of an undo-operator for an action
    template<class Param, class Me, class Composite, class Target>
    void addUndoableTo(Me& me, Composite&& composite, Target* target, Param param)
    {
        Scriptable* script = castSourceTo<script::Scriptable>(me);
        ASSERT(script);

        auto undoable = makeStoredUndo<Param>(std::move(composite), target, param);

        UndoHandler::AddUndoable(*script, std::move(undoable));
    }

    template<class Param, class Me, class Composite, class Target>
    void addUndoableTo(uint64 stateId, Me& me, Composite&& composite, Target* target, Param param)
    {
        Scriptable* script = castSourceTo<script::Scriptable>(me);
        ASSERT(script);

        auto undoable = makeStoredUndo<Param>(std::move(composite), target, param);

        UndoHandler::AddUndoable(stateId, *script, std::move(undoable));
    }

    /****************************************
    * Binding
    *****************************************/

    struct BindingUndo
    {
        template<class Param, class Composite, class Me>
        static void Add(Composite composite, Me& me, Param param)
        {
            addUndoableTo<Param>(me, composite, &me, param);
        }

        template<class Param, class Composite, class Me, class Target>
        static void AddFor(Composite composite, Me& me, Target& target, Param param)
        {
            addUndoableTo<Param>(me, composite, &target, param);
        }
    };

}
