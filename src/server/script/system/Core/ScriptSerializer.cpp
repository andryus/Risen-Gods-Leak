#include "ScriptSerializer.h"

namespace script
{

    ScriptSerializer& ScriptSerializer::GetInstance()
    {
        static ScriptSerializer serializer;

        return serializer;
    }

    SerializedEvent ScriptSerializer::SerializeEvent(uint64 eventId, uint32 data) const
    {
        auto itr = serialize.find(eventId);

        if (itr != serialize.end())
            return { std::string(itr->second), data };
        else
            return {};
    }

    std::pair<uint64, uint32> ScriptSerializer::DeserializeEvent(SerializedEvent event) const
    {
        auto itr = deserialize.find(event.first);

        if (itr != deserialize.end())
            return { itr->second, event.second };
        else
            return { -1, -1 };
    }

}