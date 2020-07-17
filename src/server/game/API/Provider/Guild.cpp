#include "API/Provider/Providers.hpp"

#include "Accounts/RBAC.h"
#include "Guilds/GuildMgr.h"

#include "API/ResponseBuilder.hpp"

namespace api::providers
{

    Guild::FutureResultType Guild::Execute(uint32 id, std::shared_ptr<const json> requestedValues, std::shared_ptr<const Request> request) const
    {
        std::string name = sGuildMgr->GetGuildNameById(id);
        if (name == "")
            return boost::none;

        ResponseBuilder response(requestedValues, request);

        response.set("id", id, rbac::PERM_API_GUILD_BASIC);
        response.set("name", std::move(name), rbac::PERM_API_GUILD_BASIC);

        return ResultType(response.build());
    }

}
