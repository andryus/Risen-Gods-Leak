#include "API/Addon/Endpoint.hpp"

#include "API/Handler/Includes.hpp"

folly::Future<json> HandleEventRegister(std::shared_ptr<const Request> request)
{
    const json& params = request->parameters;

    const std::string event = Get<std::string>(params, "event");

    const auto result = api::addon::Endpoint::notifications.Register(event, *request);

    switch (result)
    {
        case addon::NotificationRegistry::RegisterResult::Successful:
            return json::object();

        case addon::NotificationRegistry::RegisterResult::UnknownNotification:
            throw APIException("unknown event");

        case addon::NotificationRegistry::RegisterResult::AccessDenied:
            throw AccessControlException();
    }

    ASSERT(false);
}

folly::Future<json> HandleEventUnregister(std::shared_ptr<const Request> request)
{
    const json& params = request->parameters;

    const std::string event = Get<std::string>(params, "event");

    const auto result = api::addon::Endpoint::notifications.Unregister(event, *request);

    switch (result)
    {
        case addon::NotificationRegistry::UnregisterResult::Successful:
            return json::object();

        case addon::NotificationRegistry::UnregisterResult::UnknownNotification:
            throw APIException("unknown event");

        case addon::NotificationRegistry::UnregisterResult::NotRegistered:
            throw APIException("not registered for this event");
    }

    ASSERT(false);
}

void RegisterEventHandlers()
{
    Dispatcher::RegisterHandler("event/register", HandleEventRegister, rbac::PERM_API_EVENT_REGISTER);
    Dispatcher::RegisterHandler("event/unregister", HandleEventUnregister, rbac::PERM_API_EVENT_REGISTER);
}
