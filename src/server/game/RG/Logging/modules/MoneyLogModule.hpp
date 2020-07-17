#pragma once

#include "RG/Logging/LogModule.hpp"
#include "MoneyLogTypes.hpp"
#include <list>
#include <unordered_map>

class Player;


namespace RG
{
    namespace Logging
    {
        namespace detail
        {
            struct MoneyConfig
            {
                bool everything;

                uint32 interval;
                uint32 incoming;
                uint32 outgoing;
            };
        }

        class MoneyTransactions
        {
        public:
            MoneyTransactions(uint32 playerGuid, const detail::MoneyConfig& config);

            void Insert(MoneyLogType type, int32 amount, uint32 data);
            void FlushIfNeeded();

        private:
            bool NeedsFlush();
            void ExpireEntries();

            struct Entry
            {
                MoneyLogType type;
                int32 amount;
                uint32 data;

                time_t time;
                bool logged;
            };

            const detail::MoneyConfig& config_;
            uint32 playerGuid_;
            std::list<Entry> entries_;

            uint32 startTime;
            uint32 input;
            uint32 output;
        };

        class MoneyLogModule : public LogModule
        {
            friend class MoneyTransactions;

        public:
            MoneyLogModule();

            void LoadConfig() override;

            void Log(Player* player, int32 amount, MoneyLogType type, uint32 data);

            void RemovePlayer(uint32 guid);
        private:
            detail::MoneyConfig config;

            std::unordered_map<uint32, MoneyTransactions> playerTransactions_;
        };
    }
}

using RG::Logging::MoneyLogModule;
