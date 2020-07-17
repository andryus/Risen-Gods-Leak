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

#include "ChannelMgr.h"
#include "Player.h"
#include "PlayerCache.hpp"
#include "World.h"
#include "Packets/WorldPacket.h"

ChannelMgr::ChannelMap ChannelMgr::channels;
ChannelMgr::PlayerChannelMap ChannelMgr::channelPlayerMap;
std::mutex ChannelMgr::mgrLock;

ChannelMgr::~ChannelMgr()
{
    std::lock_guard<std::mutex> lock(mgrLock);

    channels.clear();
}

void ChannelMgr::JoinChannel(uint32 team, std::string const& name, uint32 channelId, Player* p, std::string const& password)
{
    std::lock_guard<std::mutex> lock(mgrLock);

    std::wstring wname;
    if (!Utf8toWStr(name, wname))
        return;

    wstrToLower(wname);

    ChannelMap::const_iterator i = channels.find(std::make_pair(team, wname));

    if (i == channels.end())
    {
        auto nchan = std::make_shared<Channel>(name, channelId, team);
        // this access don't need a channel lock, no one got the channel at this point
        nchan->JoinChannel(p, password);
        channels[std::make_pair(team, wname)] = nchan;
        channelPlayerMap.emplace(p->GetGUID(), nchan);
    }
    else
    {
        channelPlayerMap.emplace(p->GetGUID(), i->second);

        std::lock_guard<std::mutex> channelLock(i->second->lock);
        i->second->JoinChannel(p, password);
    }
}

ChannelMgr::SyncChannelPtr ChannelMgr::GetChannel(uint32 team, std::string const& name)
{
    std::lock_guard<std::mutex> lock(mgrLock);

    std::wstring wname;
    if (!Utf8toWStr(name, wname))
        return {};

    wstrToLower(wname);

    ChannelMap::const_iterator i = channels.find(std::make_pair(team, wname));

    if (i == channels.end())
        return {};

    std::unique_lock<Channel::ChannelLock> cLock(i->second->lock);
    return {i->second, std::move(cLock)};
}

bool ChannelMgr::LeaveChannel(uint32 team, std::string const& name, Player* player)
{
    std::lock_guard<std::mutex> lock(mgrLock);

    std::wstring wname;
    if (!Utf8toWStr(name, wname))
        return false;

    wstrToLower(wname);

    ChannelMap::const_iterator i = channels.find(std::make_pair(team, wname));

    if (i == channels.end())
        return false;

    auto channel = i->second;
    std::lock_guard<Channel::ChannelLock> channelLock(i->second->lock);

    bool wasInChannel = channel->LeaveChannel(player);

    if (wasInChannel)
    {
        ObjectGuid removed = player->GetGUID();

        auto range = channelPlayerMap.equal_range(removed);

        for(auto itr = range.first; itr != range.second; ++itr)
        {
            auto channelPtr = itr->second.lock();
            if (channelPtr == channel)
            {
                channelPlayerMap.erase(itr);
                break;
            }
        }
    }

    if (!channel->GetNumPlayers() && !channel->IsConstant())
        channels.erase(std::make_pair(team, wname));

    return wasInChannel;
}

void ChannelMgr::KickOrBan(uint32 team, std::string const& channel, Player const* player, std::string const& badname, bool ban)
{
    std::lock_guard<std::mutex> lock(mgrLock);


    std::wstring wname;
    if (!Utf8toWStr(channel, wname))
        return;

    wstrToLower(wname);

    ChannelMap::const_iterator i = channels.find(std::make_pair(team, wname));

    if (i == channels.end())
    {
        return;
    }

    std::unique_lock<Channel::ChannelLock> cLock(i->second->lock);
    if(i->second->KickOrBan(player, badname, ban))
    {
        ObjectGuid banned = player::PlayerCache::GetGUID(badname);

        auto range = channelPlayerMap.equal_range(banned);

        for(auto itr = range.first; itr != range.second; ++itr)
        {
            auto channelPtr = itr->second.lock();
            if (!channelPtr || channelPtr == i->second)
            {
                channelPlayerMap.erase(itr);
                break;
            }
        }
    }
}
void ChannelMgr::RemovePlayer(Player* player)
{
    std::lock_guard<std::mutex> lock(mgrLock);

    auto range = channelPlayerMap.equal_range(player->GetGUID());

    for(auto itr = range.first; itr != range.second; ++itr)
    {
        auto channelPtr = itr->second.lock();
        if (!channelPtr)
            continue;

        TC_LOG_INFO("chat.channel", "Player %u leaving channel %s due to logout.", player->GetGUID().GetCounter(), channelPtr->GetName().c_str());

        std::lock_guard<std::mutex> cLock(channelPtr->lock);

        channelPtr->LeaveChannel(player);
    }

    channelPlayerMap.erase(player->GetGUID());
}


ChannelMgr::SyncChannelPtr ChannelMgr::GetChannelByIdWithPlayer(ObjectGuid guid, uint32 channelId)
{
    std::lock_guard<std::mutex> lock(mgrLock);

    auto range = channelPlayerMap.equal_range(guid);

    for(auto itr = range.first; itr != range.second; ++itr)
    {
        auto channelPtr = itr->second.lock();
        if (channelPtr && channelPtr->GetChannelId() == channelId)
        {
            std::unique_lock<Channel::ChannelLock> cLock(channelPtr->lock);
            return {channelPtr, std::move(cLock)};
        }
    }

    return {};
}
