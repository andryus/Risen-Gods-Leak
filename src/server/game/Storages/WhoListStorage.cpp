/*
* Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
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

#include <Maps/MapManager.h>
#include "WhoListStorage.h"
#include "World.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "GuildMgr.h"
#include "WorldSession.h"

WhoListStorageMgr* WhoListStorageMgr::instance()
{
    static WhoListStorageMgr instance;
    return &instance;
}

void WhoListStorageMgr::Update()
{
    // clear current list
    _whoListStorage.clear();
    _whoListStorage.reserve(sWorld->GetPlayerCount()+1);

    HashMapHolder<Player>::MapType const& m = ObjectAccessor::GetPlayers();
    for (HashMapHolder<Player>::MapType::const_iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->second->FindMap() || !itr->second->IsInWorld() || itr->second->GetSession()->PlayerLoading())
            continue;

        std::string playerName = itr->second->GetName();
        std::wstring widePlayerName;
        if (!Utf8toWStr(playerName, widePlayerName))
            continue;

        wstrToLower(widePlayerName);

        std::string guildName = sGuildMgr->GetGuildNameById(itr->second->GetGuildId());
        std::wstring wideGuildName;
        if (!Utf8toWStr(guildName, wideGuildName))
            continue;

        wstrToLower(wideGuildName);

        uint32 org_zone_id = itr->second->GetZoneId();
        if (itr->second->InArena())
        {
            WorldLocation loc = itr->second->GetBGEntryPoint();
            org_zone_id = sMapMgr->GetZoneId(loc.m_mapId, loc.GetPositionX(), loc.GetPositionY(), loc.GetPositionZ());
        }

        _whoListStorage.emplace_back(itr->second->GetGUID(), itr->second->GetOTeam(), itr->second->GetSession()->GetSecurity(), itr->second->getLevel(),
            itr->second->getClass(), itr->second->getORace(), itr->second->GetZoneId(), itr->second->getGender(), itr->second->IsVisible(),
            widePlayerName, wideGuildName, playerName, guildName, itr->second->IsSecretQueuingActive(), org_zone_id);
    }
}
