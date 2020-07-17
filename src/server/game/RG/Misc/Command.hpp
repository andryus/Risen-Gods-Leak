#pragma once

#include "Define.h"
#include <vector>

class ChatCommand;


namespace RG
{
    namespace Command
    {
        void ReloadQuestCompleteWhitelist();
        GAME_API bool IsQuestCompleteQuestWhitelisted(uint32 quest_id);
    }
}
