#include "Server/WorldSession.h"
#include "World/World.h"

#include "API/Handler/Includes.hpp" // NOTE: this must always be the last include

folly::Future<json> HandleFeatures(std::shared_ptr<const Request> request)
{
    const json& params = request->parameters;

    AssertType(params, "features", json::value_t::array);

    json features = json::object();

    for (auto& feature : params["features"])
    {
        if (!feature.is_string())
            throw APIException("every value in <features> must be a string");

        std::string const name = feature.get<std::string>();

        auto const it = Dispatcher::handlers.find(name);
        if (it == Dispatcher::handlers.end())
        {
            features[name] = false;
            continue;
        }

        features[name] = request->HasPermission(it->second.permission);
    }

    return {{
        { "features", features },
    }};
}

void RegisterGeneralHandlers()
{
    Dispatcher::RegisterHandler("features", HandleFeatures, rbac::PERM_API_GENERAL_FEATURES);
}
