#pragma once
#include <string>

#include "ScriptTarget.h"
#include "InvokationCtx.h"

namespace script
{

    /******************************
    * AI
    *
    * Wrapper for AI combined with an owner-type
    ******************************/

    SCRIPT_API void _createFile(InvokationCtx& ctx, bool verbose);

    template<class Me>
    struct ConstructionCtx
    {
        ConstructionCtx(InvokationCtx& ctx, std::string_view name) :
            ConstructionCtx(&ctx, name) { };

        ConstructionCtx(Me& me, std::string_view name) : 
            ConstructionCtx(castSourceTo<InvokationCtx>(me), name) { }

        void EnableLogFile(bool verbose = false) 
        {
            if (ctx)
                _createFile(*ctx, verbose); 
        }

        // invokable
        template<class DataT, class Invokable, class Hook>
        uint64 AddInvokable(DataT data, Invokable&& invokable, Hook hook)
        {
            return ctx->Add<Me>(data, std::move(invokable), name, hook);
        }

        template<class DataT, class Invokable, class Hook>
        uint64 AddQuery(DataT data, Invokable&& invokable, Hook hook)
        {
            return ctx->AddQuery<Me>(data, std::move(invokable), name, hook);
        }

        template<class DataT, class Invokable, class Hook>
        uint64 AddInvokableAndCall(DataT data, Invokable&& invokable, Hook hook, Me& me)
        {
            return ctx->AddAndCall<Me>(data, std::move(invokable), name, hook, me);
        }

        template<class Base>//, std::enable_if_t<script::IsBaseOf<Base, Me>, int> = 0>
        operator ConstructionCtx<Base>() const
        { 
            static_assert(script::IsBaseOf<Base, Me>, "Cannot generate ConstructionCtx for incompatible types.");

            return {ctx, name};
        }

        explicit operator bool() const
        {
            return ctx;
        }

    private:

        template<class Base>
        friend struct ConstructionCtx;

        ConstructionCtx(InvokationCtx* ctx, std::string_view name) :
            ctx(ctx), name(name) { };

        InvokationCtx* ctx;
        std::string_view name;
    };

}
