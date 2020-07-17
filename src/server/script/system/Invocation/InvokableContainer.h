#pragma once
#include <vector>
#include <memory>
#include "ScriptInvokable.h"
#include "Random.h"

namespace script
{
    struct BaseInvokable;

    template<class Context>
    struct Params;

    enum class InvocationMode : uint8
    {
        ALL,
        RANDOM
    };

    class SCRIPT_API InvokableContainer
    {
    public:

        enum class InvokableState : uint8
        {
            Default,
            InUse,
            Delete
        };

        struct InvokableEntry
        {
            using Ptr = std::unique_ptr<BaseInvokable>;

            InvokableEntry(Ptr invokable, uint64 id) : 
                invokable(std::move(invokable)), id(id) {}

            std::unique_ptr<BaseInvokable> invokable;
            uint64 id;
            InvokableState state = InvokableState::Default;
        };

        InvokableContainer();
        ~InvokableContainer();

        InvokableContainer(const InvokableContainer&) = delete;
        void operator=(const InvokableContainer&) = delete;

        BaseInvokable* GetTop() const;

        template<class Context, class Me, class Return, class Functor, class Hook>
        uint64 Add(Functor&& functor, std::string_view name, Hook hook);

        void Remove(uint64 id);

        template<class Context>
        bool Execute(Params<Context> params, InvocationMode mode);

    private:
        using InvocableVector = std::vector<InvokableEntry>;

        template<class Context>
        bool ExecuteInvocable(InvocableVector::iterator& itr, Params<Context> params);

        InvocableVector invokables;
        uint64 invocableId = 0;
    };

    template<class Context, class Me, class Return, class Functor, class Hook>
    uint64 InvokableContainer::Add(Functor&& functor, std::string_view name, Hook hook)
    {
        const uint64 id = invocableId++;

        invokables.emplace_back(makeInvokablePtr<Context, Me, Return>(std::move(functor), name, hook), id);

        return id;
    }

    template<class Context>
    bool InvokableContainer::Execute(Params<Context> params, InvocationMode mode)
    {
        const auto block = startLoggerBlock(params.logger);

        if (mode == InvocationMode::ALL)
        {
            bool executed = false;

            for (auto itr = invokables.begin(); itr != invokables.end();)
            {
                executed |= ExecuteInvocable(itr, params);
            }

            return executed;
        }
        else
        {
            ASSERT(!invokables.empty());

            const uint32 index = urand(0, invokables.size() - 1);

            auto itr = invokables.begin() + index;

            return ExecuteInvocable(itr, params);
        }
    }

    template<class Context>
    bool InvokableContainer::ExecuteInvocable(InvocableVector::iterator& itr, Params<Context> params)
    {
        if (itr->state == InvokableState::Default)
            itr->state = InvokableState::InUse;

        const bool invoked = itr->invokable->Cast<Context, bool>().Execute(params);

        if (itr->state == InvokableState::Delete)
            itr = invokables.erase(itr);
        else
        {
            itr->state = InvokableState::Default;
            ++itr;
        }

        return invoked;
    }

}
