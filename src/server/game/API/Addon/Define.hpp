#pragma once

#include <string>

#include "Debugging/Errors.h"

#include "API/Define.hpp"

namespace api::addon
{

    const std::string PREFIX_CLIENT_REQUEST = "APIC";
    const std::string PREFIX_SERVER_REQUEST = "APIS";

    enum class RequestSource
    {
        Client,
        Server,
    };

    inline const std::string& GetPrefixString(RequestSource source)
    {
        switch (source)
        {
            case RequestSource::Client:
                return PREFIX_CLIENT_REQUEST;
            case RequestSource::Server:
                return PREFIX_SERVER_REQUEST;
            default:
                ASSERT(false, "invalid RequestSource");
        }
    }

    constexpr size_t PREFIX_LENGTH = 4;

}
