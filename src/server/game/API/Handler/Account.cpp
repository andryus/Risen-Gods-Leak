#include "Entities/Player/PlayerCache.hpp"
#include "Server/WorldSession.h"

#include "API/Handler/Includes.hpp" // NOTE: this must always be the last include

folly::Future<json> HandleAccountShow(std::shared_ptr<const Request> request)
{
    const json& params = request->parameters;

    const int64 idValue = Get<int64>(params, "id");
    const AccountId id = (idValue < 0 ? request->accountId : static_cast<uint32>(idValue));

    return providers::Account(ExtractRequestedValues(request), request)(id)
        .then([](providers::Account::ResultType result) -> json
        {
            if (!result)
                throw APIException("account not found");

            return std::move(*result);
        });
}

void RegisterAccountHandlers()
{
    Dispatcher::RegisterHandler("account/show", HandleAccountShow, rbac::PERM_API_ACCOUNT_SHOW);
}
