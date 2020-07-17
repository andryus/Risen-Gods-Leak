#pragma once
#include <memory>
#include <vector>

#include "ScriptParams.h"
#include "ScriptLogger.h"

template<class Param, class Arg>
void InvalidParams()
{
#ifdef _WIN64
    static_assert(std::is_void_v<Param>, "Param cannot be converted to Arg: " __FUNCSIG__);
#else
    static_assert(std::is_void_v<Param>, "Param cannot be converted to Arg.");
#endif
}

namespace script
{

    /*************************************
    * Invokable
    **************************************/

    template<class Context, class Return>
    struct ContextInvokable;

    struct BaseInvokable
    {

        virtual ~BaseInvokable() = 0;

        template<class Context, class Return>
        decltype(auto) Cast()
        {
            return *static_cast<ContextInvokable<Context, Return>*>(this);
        }

    };

    inline BaseInvokable::~BaseInvokable() = default;

    using InvokablePtr = std::unique_ptr<BaseInvokable>;

    template<class Context, class Return>
    struct ContextInvokable :
        public BaseInvokable
    {
        using BaseInvokable::BaseInvokable;

        virtual Return Execute(Params<Context> params) = 0;
    };

    template<class Me, class Param, class Return = bool>
    using ContextInvokablePointer = std::unique_ptr<ContextInvokable<Context<Me, Param>, Return>>;

    template<class Context, class Me, class Return, class Functor, class Hook>
    struct Invokable :
        public ContextInvokable<Context, Return>
    {
        Invokable(Functor&& functor, std::string_view name, Hook hook) :
            functor(std::move(functor)), name(name), hook(hook) { }

        Return Execute(Params<Context> params) override
        {
            if (params.logger)
                params.logger->AddModule(name);

            auto hookedParams = makeParams<Me>(script::castSourceTo<Me>(params.target), params.param, params.logger, hook);

            const auto block = startLoggerBlock(params.logger);

            auto value = functor.Call(hookedParams);
            if constexpr(std::is_pointer_v<Return>)
                return script::castSourceTo<std::remove_pointer_t<Return>>(value);
            else if constexpr(std::is_same_v<Return, bool>)
            {
                const auto evalReturn = [](auto value)
                {
                    return !value;
                };

                return !applyForReturnValue<bool>(value, evalReturn);
            }
            else
                return value;
        }

    private:

        Functor functor;
        std::string_view name;
        Hook hook;
    };

    template<class Context, class Me, class Return, class Functor, class Hook>
    auto makeInvokablePtr(Functor&& functor, std::string_view name, Hook hook)
    {
        using InvokeFunctor = Invokable<Context, Me, Return, Functor, Hook>;

        return std::make_unique<InvokeFunctor>(std::move(functor), name, hook);
    }

}