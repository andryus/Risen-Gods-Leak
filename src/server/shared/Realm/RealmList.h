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

#ifndef _REALMLIST_H
#define _REALMLIST_H

#include "Common.h"
#include "Define.h"
#include "Realm/Realm.h"
#include "QueryResult.h"
#include <map>
#include <vector>
#include <unordered_set>

#include <boost/asio/deadline_timer.hpp>

namespace boost
{
    namespace system
    {
        class error_code;
    }
}

namespace Trinity
{
    namespace Asio
    {
        class IoContext;
    }
}

/// Storage object for the list of realms on the server
class RealmList
{
public:
    typedef std::map<RealmHandle, Realm> RealmMap;

    static RealmList* Instance();

    ~RealmList();

    void Initialize(Trinity::Asio::IoContext& ioContext, uint32 updateInterval, std::function<PreparedQueryResult()> realmQuery);
    void Close();

    RealmMap const& GetRealms() const { return _realms; }
    Realm const* GetRealm(RealmHandle const& id) const;

private:
    RealmList();

    void UpdateRealms(boost::system::error_code const& error);
    void UpdateRealm(RealmHandle const& id, uint32 build, const std::string& name, ip::address const& address, ip::address const& localAddr,
        ip::address const& localSubmask, uint16 port, uint8 icon, RealmFlags flag, uint8 timezone, AccountTypes allowedSecurityLevel, float population);

    RealmMap _realms;
    uint32 _updateInterval;
    boost::asio::deadline_timer* _updateTimer;
    boost::asio::ip::tcp::resolver* _resolver;
    std::function<PreparedQueryResult()> _realmQuery;
};

#define sRealmList RealmList::Instance()
#endif
