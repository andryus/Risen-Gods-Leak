#pragma once
#include "ScriptParams.h"

namespace script
{
    template<class Me>
    struct ConstructionCtx;

    /*************************************************
    * StoredEntrypoint
    *************************************************/

    template<class Composite, class Me>
    struct StoredEntrypoint
    {
        using Param = typename Composite::Param;

        StoredEntrypoint(Composite composite, Me& me) :
            me(me), composite(composite) {}

        template<class Context>
        decltype(auto) Call(Params<Context> params)
        {
            auto callParams = makeParams(me.Load(params.logger), params.param, params);

            return composite.Call(callParams);
        }

    private:

        StoredParam<Me&> me;
        Composite composite;
    };

    /*************************************************
    * EventCtx
    *************************************************/

    template<class EntryT>
    struct EntryCtx
    {
        using Ctx = typename EntryT::CtxT::CallType::CtxT;

        static uint32 Data(EntryT entry) { return static_cast<uint32>(entry.data); }
    };

    template<class Container, class Composite>
    struct IndirectEntry
    {
        using Ctx = typename EntryCtx<Composite>::Ctx;

        static uint32 Data(Container entry) { return EntryCtx<Composite>::Data(entry.composite); }
    };

    template<class Modifier, class Composite, bool IsTarget>
    struct ModifiedComposite;
    template<class Modifier, class Composite, bool IsTarget>
    struct EntryCtx<ModifiedComposite<Modifier, Composite, IsTarget>> :
        public IndirectEntry<ModifiedComposite<Modifier, Composite, IsTarget>, Composite> {};

    template<class Composite>
    struct SwappedComposite;
    template<class Composite>
    struct EntryCtx<SwappedComposite<Composite>> :
        public IndirectEntry<SwappedComposite<Composite>, Composite> {};

    template<class Composite>
    struct DemotedComposite;
    template<class Composite>
    struct EntryCtx<DemotedComposite<Composite>> :
        public IndirectEntry<DemotedComposite<Composite>, Composite> {};

    template<class Composite>
    struct EntryCtx<PromotedComposite<Composite>> :
        public IndirectEntry<PromotedComposite<Composite>, Composite> {};

    /*************************************************
    * EntryPoint - Hooked
    *************************************************/

    template<class Derived, class EntryT, class Composite>
    struct EntryPoint
    {
        using Me = typename EntryCtx<EntryT>::Ctx::Me;
        using Param = typename EntryT::Param;

        EntryPoint(EntryT entry, Composite composite) :
            entry(entry), composite(composite) {}

        template<class Context>
        bool Call(Params<Context> params)
        {
            using StoredMe = typename Context::Target;
            StoredMe* storedMe = params.target;
            if (!storedMe)
                return false;

            const auto[data, localParams] = entry.Call(params);

            auto* me = QueryTarget(localParams.target);
            if (!me)
                return false;

            using MeT = std::remove_pointer_t<decltype(me)>;
            ConstructionCtx<MeT> ctx(*me, "");
            if (!ctx)
                return false;

            if constexpr (std::is_same_v<MeT, StoredMe>)
                if (me == storedMe)
                {
                    Derived::AddInvokable(ctx, *me, *me, data, composite, params.hook);
                    return true;
                }
                
            StoredEntrypoint<Composite, StoredMe> stored(composite, *storedMe);

            Derived::AddInvokable(ctx, *storedMe, *me, data, std::move(stored), params.hook);

            return true;
        }

        template<class MeT, class HookT = DefaultHook>
        bool Register(ConstructionCtx<MeT> ctx, HookT hook = HookT{})
        {
            using CallCtx = typename EntryCtx<EntryT>::Ctx;
            const uint32 data = EntryCtx<EntryT>::Data(entry);

            Derived::template AddInvokable<CallCtx>(ctx, composite, data, hook);

            return true;
        }

        USE_GENERATOR(Derived);

        EntryT entry;
        Composite composite;

    private:

        template<class MeT>
        static auto QueryTarget(MeT* me)
        {
            if constexpr(!IsBaseOf<Me, MeT>)
                return castSourceTo<Me>(me);
            else
                return me;
        }
    };

    template<class Derived>
    struct EntryPointType
    {
        template<class EntryT, class CompositeT>
        using type = EntryPoint<Derived, EntryT, CompositeT>;
    };

    template<class EntryPointT, class EntryT, class Composite>
    auto makeEntryPoint(EntryT entry, Generator<Composite> composite)
    {
        return EntryPointT::MakeGenerator(EntryPoint<EntryPointT, EntryT, Composite>(entry, composite.Extract()));
    }

    /*************************************************
    * EntryPoint - Hooked
    *************************************************/

    template<class Composite, class Hook>
    struct HookedComposite;

    template<class Derived, class EntryT, class Hook, class Composite>
    struct EntryPoint<Derived, HookedComposite<EntryT, Hook>, Composite> :
        public EntryPoint<Derived, EntryT, Composite>
    {
        using Me = typename EntryPoint<Derived, EntryT, Composite>::Me;
        using EntryPoint<Derived, EntryT, Composite>::Register;

        EntryPoint(HookedComposite<EntryT, Hook>&& hooked, Composite&& composite) :
            EntryPoint<Derived, EntryT, Composite>(hooked.Extract(), std::move(composite)), hook(hooked.Hook()) {}

        template<class Context>
        bool Call(Params<Context> params)
        {
            auto hookedParams = makeParams(params.target, params.param, params.logger, hook);

            return EntryPoint<Derived, EntryT, Composite>::Call(hookedParams);
        }

        template<class MeT>
        bool Register(ConstructionCtx<MeT> ctx)
        {
            return EntryPoint<Derived, EntryT, Composite>::Register(ctx, hook);
        }

        Hook hook;
    };

    /*************************************************
    * EntryPointUndoable
    *************************************************/

    template<class Functor>
    struct EntryPointUndoable
    {
        constexpr EntryPointUndoable(Functor functor) :
            functor(functor) {};

        template<class Context>
        bool Call(Params<Context> params)
        {
            if (auto* ctx = castSourceTo<InvokationCtx>(params.target))
            {
                functor(*ctx);

                return true;
            }

            return false;
        }

        bool canUndo = false;

    private:

        Functor functor;
    };

    template<class Me, class Target, class Functor>
    auto addEntryPointUndoable(Me& me, Target& target, Functor functor)
    {
        const EntryPointUndoable<Functor> undoable(functor);

        BindingUndo::AddFor(undoable, me, target, NoParam{});
    }

}
