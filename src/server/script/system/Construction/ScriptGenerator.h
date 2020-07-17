#pragma once
#include "ScriptParams.h"
#include "ScriptHook.h"

namespace script
{

    /****************************************
    * Generator
    *****************************************/

    template<class Functor>
    struct Generator
    {

        Generator() = default;
        constexpr Generator(Functor functor) : functor(functor) {}

        constexpr auto Extract() { return std::move(functor); }

        auto ToGenerator() const
        {
            return MakeGenerator(functor);
        }

        template<class Other>
        static auto MakeGenerator(Other other)
        {
            return Functor::MakeGenerator(other);
        }

        template<class HookT>
        auto Hook(HookT hook) const
        {
            return functor.Hook(hook);
        }

        auto Breakpoint() const
        {
            _SCRIPT_BREAKPOINT_VAR(LocalBreak);

            return Hook(LocalBreak);
        }

    protected:

        const Functor& GetFunctor() const { return functor; }
    private:

        Functor functor;
    };

    template<template<class> class GeneratorT, class Composite>
    constexpr auto makeGenerator(Composite composite)
    {
        return GeneratorT<Composite>(composite);
    }

#define USE_GENERATOR(COMPOSITE)                      \
    auto ToGenerator() const                          \
    {                                                 \
        return MakeGenerator(*this);                  \
    }                                                 \
                                                      \
    template<class Other>                             \
    static auto MakeGenerator(Other other)            \
    {                                                 \
        return COMPOSITE::MakeGenerator(other);       \
    }                                                 \

#define USE_POSTFIX_MANIPULATORS(Composite, GENERATOR)          \
    auto Demoted() const                                        \
    {                                                           \
        return script::makeWrapperApplier                       \
            <DemotedComposite, script::GENERATOR>(              \
                script::GENERATOR<Composite>::GetFunctor());    \
    }                                                           \
                                                                \
    auto Promoted() const                                       \
    {                                                           \
        return script::makeWrapperApplier                       \
            <PromotedComposite, script::GENERATOR>(             \
                script::GENERATOR<Composite>::GetFunctor());    \
    }                                                           \
                                                                \
    auto Swapped() const                                        \
    {                                                           \
        return script::makeWrapperApplier                       \
            <SwappedComposite, script::GENERATOR>(              \
                script::GENERATOR<Composite>::GetFunctor());    \
    }                                                           \


    /******************************
    * SwappedComposite
    *******************************/

    template<class Composite>
    struct SwappedComposite
    {
        using Me = typename Composite::Param;
        using Param = typename Composite::Me;

        SwappedComposite(Composite composite) :
            composite(composite) { }

        template<class Context>
        decltype(auto) Call(Params<Context> params)
        {
            using Target = std::remove_pointer_t<std::decay_t<typename Context::Param>>;
            using Param = std::add_pointer_t<typename Context::Target>;

            const auto newParams = makeParams<Target, Param>(params.param, params.target, params);

            return composite.Call(newParams);
        }

        USE_GENERATOR(Composite);

        Composite composite;
    };

    template<class Composite>
    auto makeSwappedComposite(Generator<Composite> composite)
    {
        return Composite::MakeGenerator(SwappedComposite<Composite>(composite.Extract()));
    }

    /******************************
    * PromotedComposite
    *******************************/

    template<class Composite>
    struct PromotedComposite
    {
        using Me = typename Composite::Param;
        using Param = typename Composite::Param;

        PromotedComposite(Composite composite) :
            composite(composite) { }

        template<class Context>
        decltype(auto) Call(Params<Context> params)
        {
            const auto newParams = makeParams(params.target, params.target, params);

            return composite.Call(newParams);
        }

        USE_GENERATOR(Composite);

        Composite composite;
    };

    template<class Composite>
    auto makePromotedComposite(Generator<Composite> composite)
    {
        return Composite::MakeGenerator(PromotedComposite<Composite>(composite.Extract()));
    }

    /******************************
    * DemotedComposite
    *******************************/

    template<class Composite>
    struct DemotedComposite
    {
        using Me = typename Composite::Me;
        using Param = typename Composite::Me;

        DemotedComposite(Composite composite) :
            composite(composite) { }

        template<class Context>
        decltype(auto) Call(Params<Context> params)
        {
            const auto newParams = makeParams(params.param, params.param, params);

            return composite.Call(newParams);
        }

        USE_GENERATOR(Composite);

        Composite composite;
    };

    template<class Composite>
    auto makeDemotedComposite(Generator<Composite> composite)
    {
        return Composite::MakeGenerator(DemotedComposite<Composite>(composite.Extract()));
    }

    /******************************
    * CompositeWrapperApplier
    ******************************/

    template<template<class> class Wrapper, template<class> class GeneratorT, class Composite>
    struct CompositeWrapperApplier
    {
        CompositeWrapperApplier(Composite composite) :
            composite(composite)
        {
        }

        template<class Other>
        static auto Apply(Other other)
        {
            return Wrapper<Other>(other).ToGenerator();
        }

        Composite composite;
    };

    template<template<class> class Wrapper, template<class> class GeneratorT, class Composite>
    auto makeWrapperApplier(Composite composite)
    {
        return CompositeWrapperApplier<Wrapper, GeneratorT, Composite>(composite);
    }


    /******************************
    * CompositeDecorator
    ******************************/
    
    template<class Me>
    struct ConstructionCtx;

    template<template<class> class GeneratorT, class Composite>
    struct CompositeDecorator
    {
        using Me = typename Composite::Me;
        using Param = typename Composite::Param;

        CompositeDecorator(Composite composite) :
            composite(composite) {}

        template<class Context>
        decltype(auto) Call(Params<Context> params)
        {
            return composite.Call(params);
        }

        bool Register(ConstructionCtx<Me> me)
        {
            return composite.Register(me);
        }

        auto ToGenerator(void) const
        {
            return MakeGenerator(composite);
        }

        template<class Other>
        static auto MakeGenerator(Other other)
        {
            return makeGenerator<GeneratorT>(other);
        }

    private:
        Composite composite;
    };

    template<template<class> class GeneratorT, class Composite>
    auto makeDecorator(Composite composite)
    {
        return makeGenerator<GeneratorT>(CompositeDecorator<GeneratorT, Composite>(composite));
    }

}
