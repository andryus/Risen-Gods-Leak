#pragma once

#include "RG/Logging/LogModule.hpp"

class Player;


namespace RG
{
namespace Logging
{
    class LevelLogModule : public LogModule
    {
    public:
        LevelLogModule();

        void Log(Player* player, uint8 newLevel);
    };
}
}

using RG::Logging::LevelLogModule;
