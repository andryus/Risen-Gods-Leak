#include "InvokationCtx.h"
#include "ScriptInvokable.h"
#include "ScriptSerializer.h"

namespace script
{

    InvokationCtx::InvokationCtx(std::string_view name)
    {
        file = std::make_unique<ScriptLogFile>(std::string(name));
    }
    InvokationCtx::~InvokationCtx() {}

    void InvokationCtx::SetOwnerName(std::string_view name, std::string_view fullName)
    {
        file->SetOwnerName(name, fullName);
    }

    void InvokationCtx::ChangeLogging(LogType log)
    {
        file->ChangeLogging(log);
    }

    ScriptLogFile* InvokationCtx::GetLogFile() const
    {
        return file.get();
    }

    void InvokationCtx::Clear()
    {
        storedInvokations.clear();
    }

    std::vector<InvokationCtx::SerializedEvent> InvokationCtx::Serialize() const
    {
        std::vector<SerializedEvent> serialized;
        serialized.reserve(activeInvocations.size());
        for (const InvokableKey invocation : activeInvocations)
        {
            if (invocation.second == -1)
                continue;

            auto value = sScriptSerializer.SerializeEvent(invocation.first, invocation.second);
            if (!value.first.empty())
                serialized.emplace_back(std::move(value));
        }

        return serialized;
    }

    void InvokationCtx::Deserialize(span<InvokationCtx::SerializedEvent> input)
    {
        storedInvokations.reserve(input.size());

        for (const SerializedEvent event : input)
        {
            const auto [id, data] = sScriptSerializer.DeserializeEvent(event);

            activeInvocations.emplace(id, data);
            activeInvocations.emplace(id, -1);
        }
    }

}
