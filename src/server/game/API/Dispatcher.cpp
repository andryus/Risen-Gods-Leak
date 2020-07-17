#include "API/Dispatcher.hpp"

#include "Debugging/Errors.h"
#include "Logging/Log.h"

#include "Server/WorldSession.h"
#include "World/World.h"

namespace api
{
    
    std::unordered_map<std::string, Dispatcher::Entry> Dispatcher::handlers{};

    void Dispatcher::RegisterHandler(std::string opcode, Handler handler, AccountId permissionId)
    {
        Entry entry;
        entry.handler = std::move(handler);
        entry.permission = permissionId;

        const auto [it, successful] = handlers.emplace(std::move(opcode), std::move(entry));
        ASSERT(successful, "Duplicated handler \"%s\".", opcode.c_str());
    }

    folly::Future<json> Dispatcher::Handle(WorldSession& session, std::shared_ptr<Request> request)
    {
        const auto it = handlers.find(request->opcode);
        if (it == handlers.end())
            return folly::makeFuture<json>(UnknownOpcodeException());

        Entry& entry = it->second;

        if (!session.HasPermission(entry.permission))
            return folly::makeFuture<json>(AccessControlException());

        return folly::makeFutureWith([&, request = std::move(request)]()
        {
            return entry.handler(std::move(request));
        });
    }

}
