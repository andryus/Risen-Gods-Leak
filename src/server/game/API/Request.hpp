#pragma once

#include <string>

#include "Common.h"

#include "json/json.hpp"

#include "Accounts/RBAC.h"

class WorldSession;

namespace api
{

    class Request
    {
    public:
        const AccountId accountId;
        const std::string opcode;
        const json parameters;

        Request(AccountId accountId, std::string opcode, json parameters);

        bool HasPermission(uint32 perm) const;

    private:
        // TODO: remove this as soon as there is a real permission cache
        mutable rbac::RBACData permissionsCache;
    };

}
