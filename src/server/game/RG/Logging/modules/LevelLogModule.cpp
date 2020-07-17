#include "LevelLogModule.hpp"

#include "Config.h"
#include "Player.h"

RG::Logging::LevelLogModule::LevelLogModule() :
    LogModule("Level")
{}

void RG::Logging::LevelLogModule::Log(Player* player, uint8 newLevel)
{
    if (!player)
        return;

    if (!IsEnabled())
        return;

    PreparedStatement* stmt = RGDatabase.GetPreparedStatement(RG_ADD_LEVEL_LOG);
    stmt->setUInt32(0, player->GetGUID().GetCounter());
    stmt->setUInt32(1, player->GetLevelPlayedTime());
    stmt->setUInt32(2, player->GetTotalPlayedTime());
    stmt->setUInt32(3, newLevel);
    stmt->setInt16(4, static_cast<int16>(newLevel) - static_cast<int16>(player->getLevel()));
    RGDatabase.Execute(stmt);
}
