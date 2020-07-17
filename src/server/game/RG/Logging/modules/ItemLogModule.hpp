#pragma once

#include "RG/Logging/LogModule.hpp"
#include <unordered_set>

class Item;

namespace RG
{
    namespace Logging
    {
        enum class ItemLogType
        {
            UNKNOWN,    // not used, only for completeness (compare to Database. 0 is empty string there)
            ADDED,
            DESTROYED,
            DESTROYED_BY_PLAYER,
            DESTROYED_BY_EXPIRE_COMMAND,
            DESTROYED_BY_TIME_EXPIRE,
            DESTROYED_BY_USE,
            DESTROYED_BY_TURN_IN,
            DESTROYED_BY_ZONE_LEAVING,
            DESTROYED_BY_ENTERING_ARENA,
            DESTROYED_BY_QUEST_ACCEPT,
            TRADED,
            SEND_MAIL,
            TAKE_MAIL,
            SOLD,
            REBOUGHT,
            AUCTIONED,
            AUCTION_SUCCESSFUL,
            TO_GUILDBANK,
            FROM_GUILDBANK,
            ADDED_BY_NEED,
            ADDED_BY_GREED,
            ADDED_BY_PM,
            ADDED_BY_GM,
            ADDED_BY_QUEST,
            ADDED_BY_QUEST_REWARD,
            ADDED_BY_REFUND,
            DESTROYED_BY_WRAPPING,
            BOUND,
        };

        class GAME_API ItemLogModule : public LogModule
        {
        public:
            ItemLogModule();

            void LoadConfig() override;

            void Log(Item* item, ItemLogType type, uint32 count = 0, uint32 ownerGUID = 0, uint32 targetGUID = 0);

        private:
            inline bool IsWhitelisted(uint32 entry) const;
            inline bool IsBlacklisted(uint32 entry) const;

        private:
            struct
            {
                uint32 quality;
                std::unordered_set<uint32> whitelist;
                std::unordered_set<uint32> blacklist;
            } config;
        };
    }
}

using RG::Logging::ItemLogModule;
