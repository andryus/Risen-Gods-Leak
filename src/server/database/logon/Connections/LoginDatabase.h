/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

#ifndef _LOGINDATABASE_H
#define _LOGINDATABASE_H

#include "DatabaseWorkerPool.h"
#include "MySQLConnection.h"

enum LoginDatabaseStatements
{
    /*  Naming standard for defines:
        {DB}_{SEL/INS/UPD/DEL/REP}_{Summary of data changed}
        When updating more than one field, consider looking at the calling function
        name for a suiting suffix.
    */

    LOGIN_SEL_REALMLIST,
    LOGIN_DEL_EXPIRED_IP_BANS,
    LOGIN_UPD_EXPIRED_ACCOUNT_BANS,
    LOGIN_SEL_LOGONCHALLENGE,
    LOGIN_UPD_LOGONPROOF,
    LOGIN_SEL_RECONNECTCHALLENGE,
    LOGIN_UPD_FAILEDLOGINS,
    LOGIN_INS_ACCOUNT_AUTO_BANNED,
    LOGIN_INS_IP_AUTO_BANNED,
    LOGIN_SEL_REALM_CHARACTER_COUNTS,
    
    LOGIN_UPD_VS,
    LOGIN_SEL_IP_INFO,

    MAX_LOGINDATABASE_STATEMENTS
};

class LoginDatabaseConnection : public MySQLConnection
{
public:
    typedef LoginDatabaseStatements Statements;

    //- Constructors for sync and async connections
    LoginDatabaseConnection(MySQLConnectionInfo& connInfo) : MySQLConnection(connInfo, "Logon") { }
    LoginDatabaseConnection(ProducerConsumerQueue<SQLOperation*>* q, MySQLConnectionInfo& connInfo) :
            MySQLConnection(q, connInfo, "Logon")
    {}

    //- Loads database type specific prepared statements
    void DoPrepareStatements() override;
};

typedef DatabaseWorkerPool<LoginDatabaseConnection> LoginDatabaseWorkerPool;

#endif
