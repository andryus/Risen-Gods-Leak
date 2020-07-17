/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITY_FORMULAS_H
#define TRINITY_FORMULAS_H

#include "Define.h"

class Unit;
class Player;

enum ContentLevels : uint8;
enum XPColorChar : uint8;

namespace Trinity
{
    namespace Honor
    {
        float hk_honor_at_level_f(uint8 level, float multiplier = 1.0f);

        uint32 hk_honor_at_level(uint8 level, float multiplier = 1.0f);
    } // namespace Trinity::Honor

    namespace XP
    {
        uint8 GetGrayLevel(uint8 pl_level);
        XPColorChar GetColorCode(uint8 pl_level, uint8 mob_level);

        uint8 GetZeroDifference(uint8 pl_level);

        uint32 BaseGain(uint8 pl_level, uint8 mob_level, ContentLevels content);
        uint32 Gain(Player* player, Unit* u);

        float xp_in_group_rate(uint32 count, bool isRaid);
    } // namespace Trinity::XP
} // namespace Trinity

#endif
