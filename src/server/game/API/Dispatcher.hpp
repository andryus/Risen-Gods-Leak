#pragma once

#include <memory>
#include <unordered_map>

#include <folly/futures/Future.h>

#include "json/json.hpp"

#include "API/Define.hpp"
#include "API/Request.hpp"

folly::Future<json> HandleFeatures(std::shared_ptr<const api::Request> request);

namespace api
{
    
    class UnknownOpcodeException : public APIException
    {
    public:
        UnknownOpcodeException() : APIException("unknown opcode") {}
    };

    class Dispatcher
    {
        friend folly::Future<json> (::HandleFeatures)(std::shared_ptr<const api::Request> request);

    public:
        using Handler = std::function<folly::Future<json>(std::shared_ptr<const Request>)>;

    private:
        struct Entry
        {
            Handler handler;
            AccountId permission;
        };
        static std::unordered_map<std::string, Entry> handlers;

    private:
        Dispatcher() {}

    public:
        static void RegisterHandler(std::string opcode, Handler handler, AccountId permissionId);

        //TODO remove session parameter once there is an api to check permissions without a session
        static folly::Future<json> Handle(WorldSession& session, std::shared_ptr<Request> request);
    };

}
