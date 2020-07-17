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

#ifndef _RGDATABASE_H
#define _RGDATABASE_H

#include "DatabaseWorkerPool.h"
#include "MySQLConnection.h"

enum RGDatabaseStatements
{
    /*  Naming standard for defines:
        {DB}_{SET/DEL/ADD/REP}_{Summary of data changed}
        When updating more than one field, consider looking at the calling function
        name for a suiting suffix.
    */

    RG_ADD_LEVEL_LOG,
    RG_ADD_ITEM_LOG,
    RG_ADD_COMMAND_LOG,

    RG_INS_MOD_MONEY,
    RG_INS_MOD_MONEY_IMMEDIATE,
    RG_INS_PLAYER_STATISTICS,
    RG_INS_SERVER_STATISTICS,
    RG_INS_GM_SPELL_CAST,

    RG_INS_TRADE,
    RG_INS_TRADE_SPELL,
    RG_INS_TRADE_ITEM,

    RG_INS_MAIL,
    RG_INS_MAIL_ITEM,

    RG_INS_ENCOUNTER,
    RG_INS_ENCOUNTER_MEMBER,
    RG_INS_ENCOUNTER_LOOT,

    RG_INS_MUTE,
    RG_SEL_MUTES,

    RG_INS_PREMIUM,

    MAX_RGDATABASE_STATEMENTS,
};

class RGDatabaseConnection : public MySQLConnection
{
public:
    typedef RGDatabaseStatements Statements;

    //- Constructors for sync and async connections
    RGDatabaseConnection(MySQLConnectionInfo& connInfo) : MySQLConnection(connInfo, "RG") {}
    RGDatabaseConnection(ProducerConsumerQueue<SQLOperation*>* q, MySQLConnectionInfo& connInfo) :
            MySQLConnection(q, connInfo, "RG")
    {}

    //- Loads database type specific prepared statements
    void DoPrepareStatements();
};

typedef DatabaseWorkerPool<RGDatabaseConnection> RGDatabaseWorkerPool;

#endif
