#pragma once

#include "Define.h"
#include "Timer.h"
#include <string>
#include <cstdio>


class WorldSession;
class WorldPacket;
class Player;
class Item;
struct AuctionEntry;

namespace support
{
    class Ticket;
}

namespace RG
{
namespace Suspicious
{
    bool IsActive();

    class PacketLogger
    {
    public:
        enum Direction
        {
            CLIENT_TO_SERVER,
            SERVER_TO_CLIENT
        };

    public:
        explicit PacketLogger(uint32 id, bool account);
        ~PacketLogger();

        void LogPacket(Direction direction, WorldPacket const& packet);

    private:
        void Initialize(uint32 id, bool account);

    private:
        FILE* _file;
    };

    enum Condition
    {
        CONDITION_UNREGULAR_ITEM_SELL,
        CONDITION_TICKET_CREATE,
        CONDITION_AUCTION_CANCEL,
        CONDITION_NETWORK_EXCEPTION,
    };

    enum class NetworkExceptionType
    {
        INVALID_HEADER,
        UNEXPECTED_OPCODE,
        UNPROCESSED_TAIL,
        EXCEPTION,
        OPCODE_NEVER,
        OPCODE_UNHANDLED,
        OPCODE_UNKNOWN,
    };

    class Monitor
    {
    public:
        explicit Monitor(WorldSession* session);
        ~Monitor();

        void Update(uint32 diff);

        bool CheckConditionMeets(Condition condition, Item* item, uint32 money) const;
        bool CheckConditionMeets(Condition condition, const support::Ticket& ticket) const;
        bool CheckConditionMeets(Condition condition, const AuctionEntry* auction) const;
        bool CheckConditionMeets(Condition condition, NetworkExceptionType type) const;

        bool IsLoggingActive() const { return _logger != nullptr; }
        PacketLogger* GetLogger() const { return _logger; }

        void SetPlayer(Player* player) { _player = player; }
        void Reload();

    private:
        void StartLogging() const;
        void StopLogging();

        std::string GetPlayerName() const;
        uint32 GetPlayerGUID() const;

    private:
        WorldSession* _session;
        bool _permanent;
        Player* _player;
        mutable TimeTracker _timer;
        mutable PacketLogger* _logger;

        mutable struct AuctionTracker
        {
            AuctionTracker() : counter(0), timer(0) {}

            uint32      counter;
            TimeTracker timer;
        } auctiontracker;
    };
}
}
