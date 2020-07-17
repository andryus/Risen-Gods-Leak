#pragma once

#include <chrono>
#include <mutex>
#include <sstream>
#include <unordered_map>

#include "Define.h"
#include "json/json.hpp"

#include "API/Addon/Define.hpp"
#include "API/Addon/NotificationRegistry.hpp"

class WorldSession;
class api_addonendpoint_world_script;

namespace api::addon
{

    class GAME_API Endpoint
    {
        friend class ::api_addonendpoint_world_script;

    public:
        static NotificationRegistry notifications;

    private:
        struct config_t
        {
            uint32 maxMessagesPerRequest;
            std::chrono::system_clock::duration requestTimeout;
            std::string serverName;
        };
        static config_t config;

        WorldSession& session;
        std::mutex mutex{};

        struct ClientRequest
        {
            std::stringstream buffer{};
            uint32 messageCount = 0;
            std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        };
        std::unordered_map<uint8, ClientRequest> clientRequests{};

        uint8 lastServerRequestId = 0;

    public:
        explicit Endpoint(WorldSession& session) : session(session) {}

        void Post(const std::string& opcode, json params);
        void Post(const json& data);

        void HandleIncomingMessage(std::string_view message);
        void CancelExpiredRequests();

        /// intended to be used ONLY by ChatHandler
        static const std::string& GetServerName()
        {
            return config.serverName;
        }

    private:
        uint8 GenerateId();

        void Send(RequestSource source, uint8 id, const json& data) const;
        void Send(RequestSource source, uint8 id, const std::string& opcode, json data) const;
        void SendError(uint8 id, const std::string& message, json data = json::object()) const;

        void HandleClientRequest(uint8 id, std::string_view content, bool done);
    };

}
