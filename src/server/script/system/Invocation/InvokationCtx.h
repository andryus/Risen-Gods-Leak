#pragma once
#include <unordered_map>
#include <unordered_set>
#include <boost/functional/hash.hpp>

#include "Define.h"
#include "InvokableContainer.h"
#include "ScriptLogFile.h"
#include "ScriptLogger.h"
#include "ScriptBinding.h"

namespace script
{
    template<class Target, class Param, class DebugHook>
    struct Context;

   /******************************
    * ScriptAI
    *
    * An AI is composed of multiple modules
    ******************************/

    class InvokationCtx;

    template<class Invokable>
    struct InvokableId {};

    // used to allow storing an AI as a polymorphic pointer
    class SCRIPT_API InvokationCtx
    {
        using QueryResponse = std::unique_ptr<BaseInvokable>;

        using InvokableKey = std::pair<uint64, uint32>;
        using InvokableMap = std::unordered_map<InvokableKey, InvokableContainer, boost::hash<InvokableKey>>;

        using SerializedEvent = std::pair<std::string, uint32>;
    public:
        static constexpr uint32 ANY_CALL = (uint32)-1;

        InvokationCtx(std::string_view name);
        ~InvokationCtx();

        InvokationCtx(const InvokationCtx&) = delete;
        void operator=(const InvokationCtx&) = delete;

        void SetOwnerName(std::string_view name, std::string_view fullName);
        void ChangeLogging(LogType log);
        ScriptLogFile* GetLogFile() const;

        template<class Me, class InvocationData, class Invokable, class Hook>
        uint64 Add(InvocationData data, Invokable&& invokable, std::string_view name, Hook hook);
        template<class InvocationData>
        void Remove(InvocationData data, uint64 id);

        template<class Me, class InvocationData, class Invokable, class Hook>
        uint64 AddAndCall(InvocationData data, Invokable&& invokable, std::string_view name, Hook hook, Me& me);
        template<class Binding, class InvocationData>
        bool Call(const Binding& binding, InvocationData data, typename InvocationData::Me& me, typename InvocationData::Param param, InvocationMode mode);

        template<class MeT, class QueryData, class Invokable, class Hook>
        uint64 AddQuery(QueryData data, Invokable&& invokable, std::string_view name, Hook hook);
        template<class QueryData>
        void RemoveQuery(QueryData data, uint64 id);

        template<class Binding, class QueryData>
        auto Query(const Binding& binding, QueryData data, typename QueryData::Me& me, typename QueryData::Param param);

        void Clear();

        std::vector<SerializedEvent> Serialize() const;
        void Deserialize(span<SerializedEvent> input);

    private:

        template<class InvocationData>
        InvokableContainer* GetContainer(InvocationData data);
        template<class QueryData>
        BaseInvokable* GetQuery(QueryData data);
        template<class QueryData>
        InvokableContainer* GetQueryContainer(QueryData data);

        template<class InvocationData>
        bool AquireContainerForCall(InvokableContainer*& target, InvocationData data);

        template<class InvocationData>
        static auto GetInvocationKey(InvocationData data);

        InvokableMap storedInvokations;
        InvokableMap storedQueries;

        std::unordered_set<InvokableKey, boost::hash<InvokableKey>> activeInvocations;
        std::unique_ptr<ScriptLogFile> file;
    };


    template<class MeT, class InvocationData, class Invokable, class Hook>
    uint64 InvokationCtx::Add(InvocationData data, Invokable&& invokable, std::string_view name, Hook hook)
    {
        const auto key = GetInvocationKey(data);
        auto& container = storedInvokations[key];

        using Me = typename InvocationData::Me;
        using Param = typename InvocationData::Param;

        return container.template Add<Context<Me, Param, Hook>, MeT, bool>(std::move(invokable), name, hook);
    }

    template<class InvocationData>
    void InvokationCtx::Remove(InvocationData data, uint64 id)
    {
        InvokableContainer* container = GetContainer(data);
        ASSERT(container);

        container->Remove(id);
    }


    template<class Me, class InvocationData, class Invokable, class Hook>
    uint64 InvokationCtx::AddAndCall(InvocationData data, Invokable&& invokable, std::string_view name, Hook hook, Me& me)
    {
        static_assert(std::is_same_v<typename InvocationData::Param, NoParam>, "Can only immediately call registered invokation when no parameter is expected. Check logic errors in binding-generators.");
        static_assert(InvocationData::CallOnAdd, "AddAndCall may only be used with Events that support it.");

        const uint64 invokationId = Add<Me>(data, std::move(invokable), name, hook);

        const auto key = GetInvocationKey(data);

        if (activeInvocations.count(key))
        {
            auto& container = storedInvokations[key];

            // todo: log/print
            auto params = makeParams(&me, script::NoParam{}, nullptr, hook);

            container.GetTop()->template Cast<Context<Me, NoParam, Hook>, bool>().Execute(params);
        }

        return invokationId;
    }

    template<class Binding, class InvocationData>
    bool InvokationCtx::Call(const Binding& binding, InvocationData data, typename InvocationData::Me& me, typename InvocationData::Param param, InvocationMode mode)
    {
        constexpr uint8 ARRAY_SIZE = InvocationData::HasData ? 2 : 1;
        std::array<InvokableContainer*, ARRAY_SIZE> containers = {};

        bool hasAny = false;
        hasAny |= AquireContainerForCall(containers[0], data);
        
        if constexpr(InvocationData::HasData)
            hasAny |= AquireContainerForCall(containers[1], InvocationData{ANY_CALL});

        if (hasAny)
        {
            auto block = ScriptLogFile::RequestFileBlock(file.get());
            printBinding(binding, &me, param, block.Logger());

            const auto params = makeParams(&me, param);
            
            bool success = false;

            for (InvokableContainer* container : containers)
            {
                if (container)
                    success |= container->Execute(params, mode);
            }

            block.SetSuccess(success);

            return success;
        }

        return false;
    }

    template<class Me, class QueryData, class Invokable, class Hook>
    uint64 InvokationCtx::AddQuery(QueryData data, Invokable&& invokable, std::string_view name, Hook hook)
    {
        const auto key = GetInvocationKey(data);
        auto& container = storedInvokations[key];

        using MeT = typename QueryData::Me;
        using Param = typename QueryData::Param;

        return storedQueries[key].template Add<Context<Me, Param, Hook>, MeT, typename QueryData::Return>(std::move(invokable), name, hook);
    }

    template<class QueryData>
    void InvokationCtx::RemoveQuery(QueryData data, uint64 id)
    {
        InvokableContainer* container = GetQueryContainer(data);
        ASSERT(container);

        container->Remove(id);
    }


    template<class Binding, class QueryData>
    auto InvokationCtx::Query(const Binding& binding, QueryData data, typename QueryData::Me& me, typename QueryData::Param param)
    {
        using Return = typename QueryData::Return;
        using ContextT = Context<typename QueryData::Me, typename QueryData::Param>;

        const auto params = makeParams(&me, param);

        if (BaseInvokable* query = GetQuery(data))
        {
            auto block = ScriptLogFile::RequestFileBlock(file.get());
            printBinding(binding, &me, param, block.Logger());

            return query->Cast<ContextT, Return>().Execute(params);
        }

        if (BaseInvokable* query = GetQuery(QueryData{ANY_CALL}))
        {
            auto block = ScriptLogFile::RequestFileBlock(file.get());
            printBinding(binding, &me, param, block.Logger());

            return query->Cast<ContextT, Return>().Execute(params);
        }

        return QueryData::ReturnDefault(param);
    }

    template<class InvocationData>
    InvokableContainer* InvokationCtx::GetContainer(InvocationData data)
    {
        const auto key = GetInvocationKey(data);
        
        auto itr = storedInvokations.find(key);
        if (itr != storedInvokations.end())
            return &itr->second;
        else
            return nullptr;
    }

    template<class QueryData>
    BaseInvokable* InvokationCtx::GetQuery(QueryData data)
    {
        if (InvokableContainer* container = GetQueryContainer(data))
            return container->GetTop();
        else
            return nullptr;
    }

    template<class QueryData>
    InvokableContainer* InvokationCtx::GetQueryContainer(QueryData data)
    {
        const auto key = GetInvocationKey(data);

        auto itr = storedQueries.find(key);
        if (itr != storedQueries.end())
            return &itr->second;
        else
            return nullptr;
    }

    template<class InvocationData>
    bool InvokationCtx::AquireContainerForCall(InvokableContainer*& target, InvocationData data)
    {
        target = GetContainer(data);
        if constexpr(InvocationData::CallOnAdd)
            activeInvocations.emplace(GetInvocationKey(data));

        return target != nullptr;
    }

    template<class InvocationData>
    auto InvokationCtx::GetInvocationKey(InvocationData data)
    {
        static const uint64 type = InvokableId<typename InvocationData::Invoke>()();

        return std::make_pair(type, data.data);
    }

}