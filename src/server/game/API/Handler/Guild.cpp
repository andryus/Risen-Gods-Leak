#include "Guilds/GuildMgr.h"

#include "API/Handler/Includes.hpp" // NOTE: this must always be the last include

folly::Future<json> HandleGuildName(std::shared_ptr<const Request> request)
{
    const json& params = request->parameters;

    const uint32 id = Get<uint32>(params, "id");

    return providers::Guild(ExtractRequestedValues(request), request)(id)
        .then([](providers::Guild::ResultType result) -> json
        {
            if (!result)
                throw APIException("guild not found");

            return std::move(*result);
        });
}

void RegisterGuildHandlers()
{
    Dispatcher::RegisterHandler("guild/show", HandleGuildName, rbac::PERM_API_GUILD_SHOW);
}
