#include "Entities/Player/PlayerCache.hpp"
#include "Globals/ObjectAccessor.h"
#include "Server/WorldSession.h"

#include "API/Handler/Includes.hpp" // NOTE: this must always be the last include

folly::Future<json> HandleCharacterShow(std::shared_ptr<const Request> request)
{
    const json& params = request->parameters;

    const ObjectGuid guid = Get<ObjectGuid>(params, "id");

    return providers::Character(ExtractRequestedValues(request), request)(guid)
        .then([](providers::Guild::ResultType result) -> json
        {
            if (!result)
                throw APIException("character not found");

            return std::move(*result);
        });
}

void RegisterCharacterHandlers()
{
    Dispatcher::RegisterHandler("character/show", HandleCharacterShow, rbac::PERM_API_CHARACTER_SHOW);
}
