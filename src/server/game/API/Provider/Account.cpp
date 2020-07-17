#include "API/Provider/Providers.hpp"

#include "DatabaseEnv.h"
#include "Connections/LoginDatabase.h"

#include "Accounts/RBAC.h"
#include "World/World.h"

#include "API/ResponseBuilder.hpp"

namespace api::providers
{

    Account::FutureResultType Account::Execute(AccountId id, std::shared_ptr<const json> requestedValues, std::shared_ptr<const Request> request) const
    {
        ResponseBuilder response(requestedValues, request);

        if (id == 0u)
        {
            response.set("id", 0u, rbac::PERM_API_ACCOUNT_BASIC);
            response.set("name", "SERVER", rbac::PERM_API_ACCOUNT_BASIC);
            response.set("gmlevel", 200u, rbac::PERM_API_ACCOUNT_SECURITY);
            return FutureResultType(ResultType(response.build()));
        }

        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_API_SEL_ACCOUNT_INFO);
        stmt->setInt32(0, static_cast<int32>(realm.Id.Realm));
        stmt->setUInt32(1, id);
        return LoginDatabase.AsyncQuery(stmt)
            .then([id, response = std::move(response)](PreparedQueryResult result) mutable -> ResultType
            {
                if (!result)
                    return boost::none;

                Field* fields = result->Fetch();

                response.set("id", id, rbac::PERM_API_ACCOUNT_BASIC);
                response.set("name", fields[0].GetString(), rbac::PERM_API_ACCOUNT_BASIC);
                
                response.set("gmlevel", fields[1].GetUInt8(), rbac::PERM_API_ACCOUNT_SECURITY);

                response.set("mute", fields[2].GetUInt64(), rbac::PERM_API_ACCOUNT_PUNISHMENT);
                response.setWith("ban", [&]() -> json
                {
                    if (fields[3].IsNull())
                        return nullptr;

                    return {
                        { "time", fields[4].GetBool() ? -1 : static_cast<int>(fields[3].GetUInt32()) },
                        { "authorName", fields[5].GetString() },
                        { "reason", fields[6].GetString() },
                    };
                }, rbac::PERM_API_ACCOUNT_PUNISHMENT);

                return response.build();
            });
    }

}
