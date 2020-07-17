#pragma once

#include "API/Addon/Endpoint.hpp"
#include "Server/WorldSession.h"

namespace api::addon
{

    template<typename RangeT>
    void PostToAll(RangeT sessions, const json& data)
    {
        for (WorldSession& session : sessions)
            session.GetAddonAPIEndpoint().Post(data);
    }

    template<typename RangeT>
    void PostToAll(RangeT sessions, const std::string& opcode, json params)
    {
        ASSERT(params.is_object(), "json data must be an object");
        params["opcode"] = opcode;

        PostToAll(sessions, params);
    }

}
