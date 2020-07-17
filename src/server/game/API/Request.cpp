#include "API/Request.hpp"

#include "Accounts/AccountMgr.h"
#include "World/World.h"

namespace api
{
    Request::Request(AccountId accountId, std::string opcode, json parameters):
        accountId(accountId),
        opcode(std::move(opcode)),
        parameters(std::move(parameters)),
        permissionsCache(accountId, "", realm.Id.Realm, sAccountMgr->GetSecurity(accountId, realm.Id.Realm))
    {
        permissionsCache.LoadFromDB();
    }

    bool Request::HasPermission(uint32 perm) const
    {
        return true; // TODO
        //return permissionsCache.HasPermission(perm);
    }

}
