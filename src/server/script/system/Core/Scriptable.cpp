#include "Scriptable.h"
#include "InvokationCtx.h"
#include "ScriptModule.h"
#include "ScriptAccessor.h"

namespace script
{

    Scriptable::Scriptable()
    {
    }

    Scriptable::~Scriptable()
    {
    }

    bool Scriptable::InitScript(std::string_view entry)
    {
        scriptName = entry;
        const bool success = OnInitScript(scriptName);

        if (id == INVALID_ID)
            id = ScriptAccessor::AddScriptable(*this);

        ASSERT(ctx);
        fullName = OnQueryFullName();
        ctx->SetOwnerName(OnQueryOwnerName(), fullName);

        return success;
    }

    void Scriptable::ReloadScript()
    {
        InitScript(scriptName);
    }

    uint64 Scriptable::GetScriptId() const
    {
        return id;
    }

    const Scriptable::StateContainer& Scriptable::ScriptModules() const
    {
        return dataStore;
    }

    bool Scriptable::HasScript() const { return !dataStore.empty(); } // @todo:more secure way of determining if we have a script

    ScriptLogFile* Scriptable::GetScriptLogFile() const
    {
        if (ctx)
            return ctx->GetLogFile();
        else
            return nullptr;
    }

    bool Scriptable::OnLoad(std::unique_ptr<InvokationCtx>&& ctx, StateContainer&& state)
    {
        this->ctx = std::move(ctx);
        dataStore = std::move(state);

        return HasScript();
    }

    void Scriptable::ClearScript(bool forced)
    {
        if (id != INVALID_ID)
        {
            if (!forced)
            {
                ScriptAccessor::RemoveScriptable(id);

                id = INVALID_ID;
            }

            dataStore.clear();
        }
        else
            ASSERT(dataStore.empty());

        ctx = nullptr;
    }

    std::string_view Scriptable::FullName() const
    {
        if (fullName.empty())
            fullName = OnQueryFullName();
        return fullName;
    }

    std::string Scriptable::OnQueryFullName() const
    {
        return OnQueryOwnerName();
    }

}
