#pragma once
#include <memory>
#include <vector>
#include <unordered_map>

#include "Errors.h"

namespace script
{

    /*****************************************
    * Scriptable
    *****************************************/

    struct ScriptState;

    template<class Data>
    struct ScriptStateId;
    template<class Me>
    class Factory;

    class InvokationCtx;
    class ScriptLogFile;

    class SCRIPT_API Scriptable
    {
        using StatePtr = std::unique_ptr<ScriptState>;
        using StateContainer = std::unordered_map<uint64, StatePtr>;
    public:
        static constexpr uint64 INVALID_ID = -1;

        Scriptable();
        virtual ~Scriptable() = 0;

        template<class Data>
        Data* GetScriptState();
        uint64 GetScriptId() const;
        const StateContainer& ScriptModules() const;

        InvokationCtx* ScriptCtx() { return ctx.get(); }
        const InvokationCtx* ScriptCtx() const { return ctx.get(); }
        bool HasScript() const;
        ScriptLogFile* GetScriptLogFile() const;

        template<class Me>
        bool LoadScript(Me& me, std::string_view entry);

        bool InitScript(std::string_view entry);
        void ReloadScript();
        void ClearScript(bool forced = false);

        std::string_view FullName() const;

    private:

        virtual std::string OnQueryFullName() const;
        virtual std::string OnQueryOwnerName() const = 0;

        virtual bool OnInitScript(std::string_view entry) = 0;

        bool OnLoad(std::unique_ptr<InvokationCtx>&& ctx, StateContainer&& state);

        uint64 id = INVALID_ID;
        std::unique_ptr<InvokationCtx> ctx;
        StateContainer dataStore;

        std::string scriptName;
        mutable std::string fullName;
    };

    template<class Data>
    Data* Scriptable::GetScriptState()
    {
        const uint64 type = ScriptStateId<Data>()();

        auto itr = dataStore.find(type);
        if (itr != dataStore.end())
            return reinterpret_cast<Data*>(itr->second.get());
        else
            return nullptr;
    }

    template<class Me>
    bool Scriptable::LoadScript(Me& me, std::string_view entry)
    {
        auto&& [ctx, dataStore] = Factory<Me>::CreateScript(me, entry);

        return OnLoad(std::move(ctx), std::move(dataStore));
    }

}
