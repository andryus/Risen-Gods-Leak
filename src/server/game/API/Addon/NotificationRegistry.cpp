#include "API/Addon/NotificationRegistry.hpp"

#include <boost/optional.hpp>
#include <boost/range/adaptors.hpp>

#include "Entities/Player/Player.h"
#include "Globals/ObjectAccessor.h"
#include "Server/WorldSession.h"
#include "API/Addon/PostToAll.hpp"

namespace api::addon
{
    void NotificationRegistry::Create(const std::string& notification, uint32 permission)
    {
        std::lock_guard<std::mutex> lock(mutex);

        notifications.emplace(
            std::piecewise_construct,
            std::make_tuple(notification),
            std::make_tuple(permission)
        );
    }

    NotificationRegistry::RegisterResult NotificationRegistry::Register(const std::string& notification, const Request& request)
    {
        std::lock_guard<std::mutex> lock(mutex);

        const auto it = notifications.find(notification);
        if (it == notifications.end())
            return RegisterResult::UnknownNotification;

        if (!request.HasPermission(it->second.permission))
            return RegisterResult::AccessDenied;

        it->second.accountIds.insert(request.accountId);
        return RegisterResult::Successful;
    }

    NotificationRegistry::UnregisterResult NotificationRegistry::Unregister(const std::string& notification, const Request& request)
    {
        std::lock_guard<std::mutex> lock(mutex);

        const auto it = notifications.find(notification);
        if (it == notifications.end())
            return UnregisterResult::UnknownNotification;

        const size_t num = it->second.accountIds.erase(request.accountId);
        return (num == 0) ? UnregisterResult::NotRegistered : UnregisterResult::Successful;
    }

    void NotificationRegistry::Unregister(AccountId account)
    {
        std::lock_guard<std::mutex> lock(mutex);

        for (auto& notification : notifications)
            notification.second.accountIds.erase(account);
    }

    void NotificationRegistry::Send(const std::string& notification, json payload)
    {
        const auto it = notifications.find(notification);
        if (it == notifications.end())
            return;

        json data = json::object();
        data["event"] = notification;
        data["payload"] = std::move(payload);

        sWorld->GetExecutor()->add([this, it, data = std::move(data)]() mutable
        {
            std::lock_guard<std::mutex> lock(mutex);

            const auto mapAccountIdsToSessions = boost::adaptors::transformed([](AccountId id) -> WorldSession&
            {
                WorldSession* session = sWorld->FindSession(id);
                ASSERT(session);
                return *session;
            });

            PostToAll(
                it->second.accountIds | mapAccountIdsToSessions,
                "event/fire",
                std::move(data)
            );
        });
    }

}
