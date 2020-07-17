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
#ifndef __TRINITY_CHANNELMGR_H
#define __TRINITY_CHANNELMGR_H

#include "Common.h"
#include "Channel.h"

#include <map>
#include <string>

#include "World.h"

#include "SynchronizedPtr.hpp"

#define MAX_CHANNEL_PASS_STR 31

class GAME_API ChannelMgr
{
    using ChannelMap = std::map<std::pair<uint32, std::wstring>, std::shared_ptr<Channel>>;
    using PlayerChannelMap = std::multimap<ObjectGuid, std::weak_ptr<Channel>>;
    using SyncChannelPtr = util::synchronized_ptr<std::shared_ptr<Channel>, std::unique_lock<Channel::ChannelLock>>;

    private:
        ChannelMgr() { }
        ~ChannelMgr();

    public:
        static void JoinChannel(uint32 team, std::string const& name, uint32 channel_id, Player* p, std::string const& password);
        static SyncChannelPtr GetChannel(uint32 team, std::string const& name);
        static SyncChannelPtr GetChannelByIdWithPlayer(ObjectGuid guid, uint32 channelId);
        static bool LeaveChannel(uint32 team, std::string const& name, Player* player);

        static void KickOrBan(uint32 team, std::string const& channel, Player const* player, std::string const& badname, bool ban);
        static void Kick(uint32 team, std::string const& channel, Player const* player, std::string const& badname) { KickOrBan(team, channel, player, badname, false); }
        static void Ban(uint32 team, std::string const& channel, Player const* player, std::string const& badname) { KickOrBan(team, channel, player, badname, true); }

        static void RemovePlayer(Player* player);
    private:
        static ChannelMap channels;

        static PlayerChannelMap channelPlayerMap;

        static std::mutex mgrLock;
};

#endif
