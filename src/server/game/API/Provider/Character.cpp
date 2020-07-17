#include "API/Provider/Providers.hpp"

#include "Accounts/RBAC.h"
#include "Entities/Object/ObjectGuid.h"
#include "Entities/Player/Player.h"
#include "Entities/Player/PlayerCache.hpp"
#include "Globals/ObjectAccessor.h"

#include "API/ResponseBuilder.hpp"
#include "API/JsonSerializers.hpp"

namespace api::providers
{

    Character::FutureResultType Character::Execute(ObjectGuid guid, std::shared_ptr<const json> requestedValues, std::shared_ptr<const Request> request) const
    {
        const player::PlayerCache::PlayerCacheEntry entry = player::PlayerCache::Get(guid);
        if (!entry)
            return boost::none;

        ResponseBuilder response(requestedValues, request);

        response.set("id",      entry.guid,    rbac::PERM_API_CHARACTER_BASIC);
        response.set("name",    entry.name,    rbac::PERM_API_CHARACTER_BASIC);
        response.set("level",   entry.level,   rbac::PERM_API_CHARACTER_BASIC);
        response.set("class",   entry.class_,  rbac::PERM_API_CHARACTER_BASIC);
        response.set("race",    entry.race,    rbac::PERM_API_CHARACTER_BASIC);
        response.set("gender",  entry.gender,  rbac::PERM_API_CHARACTER_BASIC);

        response.setWith("is_online", [&]() { return ObjectAccessor::FindConnectedPlayer(guid) != nullptr; }, rbac::PERM_API_CHARACTER_STATUS);

        auto account = MakeProvider<Account>(requestedValues, "account", request)(entry.account);
        auto guild = MakeProvider<Guild>(requestedValues, "guild", request)(entry.guild);

        return folly::collect(account, guild)
            .then([response = std::move(response)](std::tuple<Account::ResultType, Guild::ResultType> t) mutable -> ResultType
            {
                auto& [account, guild] = t;

                response.setOptional("account", std::move(account), rbac::PERM_API_CHARACTER_BASIC);
                response.setOptional("guild", std::move(guild), rbac::PERM_API_CHARACTER_BASIC);

                return response.build();
            });
    }

}
