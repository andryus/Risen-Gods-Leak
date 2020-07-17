#include "API/Addon/Endpoint.hpp"

#include <folly/futures/Promise.h>

#include "Configuration/Config.h"

#include "Packets/WorldPacket.h"

#include "Chat/Chat.h"
#include "Entities/Player/Player.h"
#include "Scripting/ScriptMgr.h"
#include "Server/WorldSession.h"

#include "API/Dispatcher.hpp"
#include "API/Request.hpp"

const size_t MAX_MESSAGE_LENGTH = 255;
const size_t MAX_CONTENT_LENGTH = MAX_MESSAGE_LENGTH - api::addon::PREFIX_LENGTH - 2;


namespace api::addon
{

    Endpoint::config_t Endpoint::config{};
    NotificationRegistry Endpoint::notifications;

    void Endpoint::Post(const std::string& opcode, json params)
    {
        std::lock_guard<std::mutex> lock(mutex);

        const uint8 id = GenerateId();
        if (id == 0)
            return;

        Send(RequestSource::Server, id, opcode, std::move(params));
    }

    void Endpoint::Post(const json& params)
    {
        std::lock_guard<std::mutex> lock(mutex);

        const uint8 id = GenerateId();
        if (id == 0)
            return;

        Send(RequestSource::Server, id, params);
    }

    void Endpoint::HandleIncomingMessage(std::string_view message)
    {
        if (message.size() < PREFIX_LENGTH + 3) // at least prefix + separator (\t) + request id (1) + 1 char of data
            return; // invalid message

        const std::string_view prefix = message.substr(0, PREFIX_LENGTH);
        if (prefix != PREFIX_CLIENT_REQUEST)
            return; // invalid prefix

        if (message[PREFIX_LENGTH] != '\t')
            return; // invalid message

        uint8 requestId = message[PREFIX_LENGTH + 1];
        if (requestId == 0 || requestId == 255)
            return; // invalid request id

        bool done = false;
        if (requestId > 127)
        {
            done = true;
            requestId -= 127;
        }

        const std::string_view content = message.substr(PREFIX_LENGTH + 2);

        TC_LOG_TRACE(
            "api.addon",
            "[ACC:%u] Incoming message (source: CLIENT, id: %hhu, done: %s): %s",
            session.GetAccountId(), requestId, done ? "true" : "false", std::string(content).c_str()
        );

        HandleClientRequest(requestId, content, done);
    }

    void Endpoint::CancelExpiredRequests()
    {
        std::lock_guard<std::mutex> lock(mutex);

        const auto now = std::chrono::system_clock::now();

        for (auto it = clientRequests.begin(); it != clientRequests.end(); ++it)
        {
            if (now - it->second.start > config.requestTimeout)
            {
                it = clientRequests.erase(it);
            }
        }
    }

    uint8 Endpoint::GenerateId()
    {
        static const auto increment = [](uint8 id)
        {
            id++;
            return (id > 127) ? (id - 127) : id;
        };

        lastServerRequestId = increment(lastServerRequestId);
        return lastServerRequestId;
    }

    inline void SendImpl(
        WorldSession& session, WorldPacket& packet, ChatHandler& handler, const std::string& serverName,
        const std::string& prefix, uint8 id, const std::string_view& message, bool finalMessage
    ) {
        const uint8 encodedId = finalMessage ? (id + 127) : id;

        std::stringstream m;
        m << prefix << '\t' << encodedId << message;

        handler.BuildChatPacket(
            packet,
            CHAT_MSG_WHISPER_FOREIGN,
            LANG_ADDON,
            ObjectGuid::Empty,
            session.GetPlayer()->GetGUID(),
            m.str(),
            CHAT_TAG_NONE,
            serverName,
            session.GetPlayer()->GetName()
        );
        session.SendPacket(std::move(packet));
    }

    void Endpoint::Send(RequestSource source, uint8 id, const json& data) const
    {
        WorldPacket packet;
        ChatHandler handler(&session);
        const std::string message(data.dump());

        TC_LOG_DEBUG(
            "api.addon",
            "[ACC:%u] Outgoing request (source: %s, id: %hhu): %s",
            session.GetAccountId(),
            source == RequestSource::Client ? "CLIENT" : "SERVER",
            id,
            message.c_str()
        );

        const std::string& prefix = GetPrefixString(source);

        std::string_view rest = std::string_view(message);
        while (rest.size() > MAX_CONTENT_LENGTH)
        {
            const std::string_view current = rest.substr(0, MAX_CONTENT_LENGTH);
            rest = rest.substr(MAX_CONTENT_LENGTH);

            TC_LOG_TRACE(
                "api.addon",
                "[ACC:%u] Sending message (source: %s, id: %hhu, done: false): %s",
                session.GetAccountId(),
                source == RequestSource::Client ? "CLIENT" : "SERVER",
                id,
                std::string(current).c_str()
            );

            SendImpl(session, packet, handler, config.serverName, prefix, id, current, false);
        }

        TC_LOG_TRACE(
            "api.addon",
            "[ACC:%u] Sending message (source: %s, id: %hhu, done: true): %s",
            session.GetAccountId(),
            source == RequestSource::Client ? "CLIENT" : "SERVER",
            id,
            std::string(rest).c_str()
        );

        SendImpl(session, packet, handler, config.serverName, prefix, id, rest, true);
    }

    void Endpoint::Send(RequestSource source, uint8 id, const std::string& opcode, json data) const
    {
        ASSERT(data.is_object(), "json data must be an object");
        data["opcode"] = opcode;
        Send(source, id, data);
    }

    void Endpoint::SendError(uint8 id, const std::string& message, json data /* = json::object() */) const
    {
        ASSERT(data.is_object(), "json data must be an object");
        data["error"] = message;
        Send(RequestSource::Client, id, data);
    }

    void Endpoint::HandleClientRequest(uint8 id, std::string_view content, bool done)
    {
        json data;

        {
            std::lock_guard<std::mutex> lock(mutex);

            ClientRequest& request = clientRequests[id];
            request.messageCount++;

            if (!done && request.messageCount >= config.maxMessagesPerRequest)
            {
                TC_LOG_DEBUG(
                    "api.addon",
                    "[ACC:%u] Dropping client request because of too many messages",
                    session.GetAccountId()
                );
                SendError(id, "too many messages in one request");
                clientRequests.erase(id);
                return;
            }

            request.buffer << content;

            if (!done)
                return;

            TC_LOG_DEBUG(
                "api.addon",
                "[ACC:%u] Incoming client request (id: %hhu): %s",
                session.GetAccountId(), id, request.buffer.str().c_str()
            );

            try
            {
                request.buffer >> data;
            }
            catch (json::parse_error err)
            {
                TC_LOG_DEBUG(
                    "api.addon",
                    "[ACC:%u] Rejecting due to malformed json (id: %hhu): %s",
                    session.GetAccountId(), id, err.what()
                );
                SendError(id, "malformed json");
                clientRequests.erase(id);
                return;
            }

            clientRequests.erase(id);

            if (!data.has("opcode"))
            {
                TC_LOG_DEBUG(
                    "api.addon",
                    "[ACC:%u] Rejecting due to missing opcode (id: %hhu)",
                    session.GetAccountId(), id
                );
                SendError(id, "opcode missing");
                return;
            }

            if (!data["opcode"].is_string())
            {
                TC_LOG_DEBUG(
                    "api.addon",
                    "[ACC:%u] Rejecting due to invalid opcode (id: %hhu): expected string, got %s",
                    session.GetAccountId(), id, data["opcode"].type_name()
                );
                SendError(id, "invalid opcode (not a string)");
                return;
            }
        }

        std::string opcode = data["opcode"].get<std::string>();

        Dispatcher::Handle(session, std::make_shared<Request>(
            session.GetAccountId(),
            std::move(opcode),
            std::move(data)
        ))
        .via(session.GetExecutor()) //TODO does not need to be sync to the session, but there is currently no other way to make sure the session still exists
        .then([this, id](json result)
        {
            Send(RequestSource::Client, id, std::move(result));
        })
        .onError([this, id](const InvalidArgumentException& err)
        {
            std::string message = "invalid argument: ";
            message += err.what();
            TC_LOG_DEBUG(
                "api.addon",
                "[ACC:%u] Rejecting due to error in handler (id: %hhu): %s",
                session.GetAccountId(), id, message.c_str()
            );
            SendError(id, message);
        })
        .onError([this, id](const APIException& err)
        {
            TC_LOG_DEBUG(
                "api.addon",
                "[ACC:%u] Rejecting due to error in handler (id: %hhu): %s",
                session.GetAccountId(), id, err.what()
            );
            SendError(id, err.what());
        })
        .onError([this, id](const json::exception& err)
        {
            TC_LOG_DEBUG(
                "api.addon",
                "[ACC:%u] Rejecting due to error in handler (id: %hhu): %s",
                session.GetAccountId(), id, err.what()
            );
            SendError(id, err.what());
        })
        .onError([](const std::exception& err)
        {
            // only exceptions inheriting from APIExcetion or json::exception are expected to be thrown
            TC_LOG_ERROR("api", "unknown exception occured in handler: %s", err.what());
        });
    }

}

class api_addonendpoint_world_script : public WorldScript
{
public:
    api_addonendpoint_world_script() : WorldScript("api_addonendpoint_world_script") {}

    void OnConfigLoad(bool /*reload*/) override
    {
        api::addon::Endpoint::config.maxMessagesPerRequest = sConfigMgr->GetIntDefault("API.MaxMessagesPerRequest", 20);
        api::addon::Endpoint::config.requestTimeout = Seconds(sConfigMgr->GetIntDefault("API.RequestTimeout", 10));
        api::addon::Endpoint::config.serverName = sConfigMgr->GetStringDefault("API.ServerName", "Server");
    }
};

class api_addonendpoint_player_script : public PlayerScript
{
public:
    api_addonendpoint_player_script() : PlayerScript("api_addonendpoint_player_script") {}

    void OnLogout(Player* p) override
    {
        api::addon::Endpoint::notifications.Unregister(p->GetSession()->GetAccountId());
    }
};

void AddSC_api_addonendpoint()
{
    new api_addonendpoint_world_script();
    new api_addonendpoint_player_script();
}
