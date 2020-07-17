/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
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

#include "RGDatabase.h"

void RGDatabaseConnection::DoPrepareStatements()
{
    if (!m_reconnecting)
        m_stmts.resize(MAX_RGDATABASE_STATEMENTS);

    /* ################ LOAD PREPARED STATEMENTS HERE ################ */

    PrepareStatement(RG_ADD_LEVEL_LOG, "INSERT INTO log_level (player, playedLevel, playedTotal, newLevel, levelChange) VALUES (?, ?, ?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(RG_ADD_ITEM_LOG, "INSERT INTO log_item VALUES (NOW(), ?, ?, ?, ?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(RG_ADD_COMMAND_LOG, "INSERT INTO log_command (`account`, `guid`, `command`, `parameter`, `position_x`, `position_y`, `position_z`, `map`, `area`, `target_type`, `target_guid_or_entry`) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(RG_INS_MOD_MONEY, "INSERT INTO log_money VALUES(FROM_UNIXTIME(?), ?, ?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(RG_INS_MOD_MONEY_IMMEDIATE, "INSERT INTO log_money VALUES(NOW(), ?, ?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(RG_INS_PLAYER_STATISTICS, "INSERT INTO log_player_count (uptime, players, sessions, queued) VALUES (?, ?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(RG_INS_SERVER_STATISTICS, "INSERT INTO log_update_diff (difftime, peak) VALUES (?, ?)", CONNECTION_ASYNC);
    PrepareStatement(RG_INS_GM_SPELL_CAST, "INSERT INTO log_gm_spells (`account`, `character`, `spell`, `caster`, `caster_map`, `caster_x`, `caster_y`, `caster_z`, `target_mask`, `target_unit_guid`, `target_item_guid`, `target_dest_x`, `target_dest_y`, `target_dest_z`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", CONNECTION_ASYNC);

    PrepareStatement(RG_INS_TRADE, "INSERT INTO log_trade (`id`, `player`, `partner`, `player_money`, `partner_money`) VALUES (?, ?, ?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(RG_INS_TRADE_SPELL, "INSERT INTO log_trade_spell (`id`, `direction`, `spell`, `target_guid`, `target_entry`) VALUES (?, ?, ?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(RG_INS_TRADE_ITEM, "INSERT INTO log_trade_item (`id`, `direction`, `slot`, `guid`, `entry`, `count`) VALUES (?, ?, ?, ?, ?, ?)", CONNECTION_ASYNC);

    PrepareStatement(RG_INS_MAIL, "INSERT INTO log_mail (`id`, `sender`, `receiver`, `subject`, `money`, `cod`) VALUES (?, ?, ?, ?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(RG_INS_MAIL_ITEM, "INSERT INTO log_mail_item (`id`, `slot`, `guid`, `entry`, `count`) VALUES (?, ?, ?, ?, ?)", CONNECTION_ASYNC);

    PrepareStatement(RG_INS_ENCOUNTER, "INSERT INTO log_encounter (refId, map, mode, instanceId, encounter, type, memberCount, groupLead, masterLooter) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(RG_INS_ENCOUNTER_MEMBER, "INSERT INTO log_encounter_member (refId, player, alive) VALUES (?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(RG_INS_ENCOUNTER_LOOT, "INSERT INTO log_encounter_loot (refId, item) VALUES (?, ?)", CONNECTION_ASYNC);

    PrepareStatement(RG_INS_MUTE, "INSERT INTO log_mute (account, `character`, duration, author, reason) VALUES (?, ?, SEC_TO_TIME(?), ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(RG_SEL_MUTES, "SELECT DATE_FORMAT(timestamp, '%d.%m.%y %T'), TIME_TO_SEC(duration), author, reason FROM log_mute WHERE account = ? ORDER BY timestamp DESC LIMIT 10", CONNECTION_SYNCH);

    PrepareStatement(RG_INS_PREMIUM, "INSERT INTO log_premium (`player`, `action`, `state`, `type`, `amount`, `code`) VALUES (?, ?, ?, ?, ?, ?)", CONNECTION_ASYNC);

    /* ############## END OF LOADING PREPARED STATEMENTS ############## */

}
