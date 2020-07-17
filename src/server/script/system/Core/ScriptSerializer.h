#pragma once
#include <unordered_map>
#include <string>
#include <string_view>
#include <functional>
#include "Define.h"
#include "API.h"

namespace script
{
    template<class Invokable>
    struct InvokableId;

    using SerializedEvent = std::pair<std::string, uint32>;

    class SCRIPT_API ScriptSerializer
    {
        using SerializeMap = std::unordered_map<uint64, std::string_view>;
        using DeserializeMap = std::unordered_map<std::string_view, uint64>;
    public:

        static ScriptSerializer& GetInstance();

        template<class Event>
        bool RegisterEvent(std::string_view compositeName);

        SerializedEvent SerializeEvent(uint64 eventId, uint32 data) const;
        std::pair<uint64, uint32> DeserializeEvent(SerializedEvent event) const;

    private:

        ScriptSerializer() = default;
        ~ScriptSerializer() = default;
        
        SerializeMap serialize;
        DeserializeMap deserialize;
    };


    template<class Event>
    bool ScriptSerializer::RegisterEvent(std::string_view compositeName)
    {
        const uint64 type = InvokableId<Event>()();

        serialize.emplace(type, compositeName);
        deserialize.emplace(compositeName, type);

        return true;
    }

#define sScriptSerializer ScriptSerializer::GetInstance()

}