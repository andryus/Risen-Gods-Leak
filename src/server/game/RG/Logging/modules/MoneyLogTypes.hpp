#pragma once

#include "Define.h"

namespace RG
{
    namespace Logging
    {
        enum class MoneyLogType : uint8
        {
            NULL_TYPE = 0,
            UNKNOWN,
            AH_SELL,
            AH_BID_UPDATE,
            AH_BID,
            AH_BUYOUT_BID,
            AH_BUYOUT,
            AH_CANCEL,
            COMMAND_MOD,
            COMMAND_COMPLETE,
            GUILD,
            GUILD_DEPOSIT,
            GUILD_WITHDRAW,
            ITEM,
            ITEM_SELL,
            ITEM_REBUY,
            ITEM_REFUND,
            LOOTED,
            MAIL,
            MAIL_COD,
            MAIL_TAKE,
            DUAL_SPEC,
            QUEST_REWARD,
            TRADE_GIVE,
            TRADE_GET,
            SPELL,
        };
    }
}
