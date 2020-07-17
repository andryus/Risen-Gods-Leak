#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <json/json.hpp>

#include "Common.h"
#include "API/Request.hpp"

namespace api::addon
{
    
    class GAME_API NotificationRegistry
    {
    private:
        struct Entry
        {
            uint32 permission;
            std::unordered_set<AccountId> accountIds{};

            explicit Entry(uint32 permission) : permission(permission) {}
        };

        using Container = std::unordered_map<std::string, Entry>;

        Container notifications{};

        std::mutex mutex{};

    public:
        void Create(const std::string& notification, uint32 permission);

        enum class RegisterResult { Successful, UnknownNotification, AccessDenied };
        RegisterResult Register(const std::string& notification, const Request& request);

        enum class UnregisterResult { Successful, UnknownNotification, NotRegistered };
        UnregisterResult Unregister(const std::string& notification, const Request& request);

        void Unregister(AccountId account);

        void Send(const std::string& notification, json payload);
    };

}
