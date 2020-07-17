#pragma once

#include <atomic>

#include "RG/Logging/LogModule.hpp"

#include "Mail.h"

class TradeData;

namespace RG
{
    namespace Logging
    {
        class TradeLogModule : public LogModule
        {
            enum class Direction
            {
                GIVE = 1,
                GET = 2,
            };

        public:
            TradeLogModule();

            void Log(const Player* player, const TradeData* playerTrade, const Player* partner, const TradeData* partnerTrade);
            void Log(const MailDraft& draft, const MailReceiver& receiver, const MailSender& sender, const MailDraft::MailItemMap& items);

            void Initialize() override;
            void LoadConfig() override;

        private:
            std::atomic<uint32> tradeLogNumber_;
            std::atomic<uint32> mailLogNumber_;
            struct
            {
                struct
                {
                    bool enabled;
                } trade;
                struct
                {
                    bool enabled;
                    bool player_only;
                } mail;
            } config;
        };
    }
}

using RG::Logging::TradeLogModule;