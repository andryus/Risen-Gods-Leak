#pragma once

#include "json/json.hpp"

#include "Entities/Object/ObjectGuid.h"
#include "Entities/WorldObject/WorldLocation.h"

namespace nlohmann
{

    template<>
    struct adl_serializer<ObjectGuid>
    {
        static void to_json(json& j, const ObjectGuid& guid)
        {
            j = guid.GetRawValue();
        }

        static void from_json(const json& j, ObjectGuid& guid)
        {
            guid.Set(j.get<uint64>());
        }
    };

    template<>
    struct adl_serializer<WorldLocation>
    {
        static void to_json(json& j, const WorldLocation& loc)
        {
            j = json::object({
                { "map", loc.GetMapId() },
                { "x", loc.GetPositionX() },
                { "y", loc.GetPositionY() },
                { "z", loc.GetPositionZ() },
            });
        }

        static void from_json(const json& j, WorldLocation& loc)
        {
            loc.m_mapId     = j.at("map").get<uint32>();
            loc.m_positionX = j.at("x").get<float>();
            loc.m_positionY = j.at("y").get<float>();
            loc.m_positionZ = j.at("z").get<float>();
        }
    };

    template<typename T>
    struct adl_serializer<boost::optional<T>>
    {
        static void to_json(json& j, const boost::optional<T>& opt)
        {
            if (opt)
                j = *opt;
            else
                j = nullptr;
        }

        static void from_json(json& j, const boost::optional<T>& opt)
        {
            if (j.is_null())
                opt = boost::none;
            else
                opt = static_cast<T>(j);
        }
    };

}
