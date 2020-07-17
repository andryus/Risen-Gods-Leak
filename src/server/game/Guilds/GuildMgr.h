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

#ifndef _GUILDMGR_H
#define _GUILDMGR_H

#include "Guild.h"

class GAME_API GuildMgr
{
friend class Guild;
private:
    GuildMgr();
    ~GuildMgr();

public:
    static GuildMgr* instance();

    Guild::GuildPtr GetGuildByLeader(ObjectGuid guid) const;
    Guild::GuildPtr GetGuildById(uint32 guildId) const;
    Guild::GuildPtr GetGuildByName(std::string const& guildName) const;
    std::string GetGuildNameById(uint32 guildId) const;

    Guild::GuildPtr CreateGuild(SQLTransaction& trans, Player* pLeader, std::string const& name);
    void LoadGuilds();

    void ResetTimes();

private:
    using Mutex = std::mutex;
    using Lock = std::lock_guard<Mutex>;

    mutable Mutex _mutex;

    typedef std::unordered_map<uint32, Guild*> GuildContainer;
    uint32 NextGuildId;
    GuildContainer GuildStore;


    void RemoveGuild(uint32 guildId);
    uint32 GenerateGuildId();
    void SetNextGuildId(uint32 Id) { NextGuildId = Id; }

};

#define sGuildMgr GuildMgr::instance()

#endif
