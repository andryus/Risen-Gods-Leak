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

#include "Channel.h"
#include "Chat.h"
#include "ObjectMgr.h"
#include "SocialMgr.h"
#include "World.h"
#include "DatabaseEnv.h"
#include "AccountMgr.h"
#include "Player.h"
#include "DBCStores.h"
#include "Packets/WorldPacket.h"
#include "Entities/Player/PlayerCache.hpp"

Channel::Channel(std::string const& name, uint32 channelId, uint32 team):
    _announce(true),
    _ownership(true),
    _IsSaved(false),
    _flags(0),
    _channelId(channelId),
    _Team(team),
    _ownerGUID(),
    _name(name),
    _password("")
{
    // set special flags if built-in channel
    if (ChatChannelsEntry const* ch = sChatChannelsStore.LookupEntry(channelId)) // check whether it's a built-in channel
    {
        _announce = false;                                 // no join/leave announces
        _ownership = false;                                // no ownership handout

        _flags |= CHANNEL_FLAG_GENERAL;                    // for all built-in channels

        if (ch->flags & CHANNEL_DBC_FLAG_TRADE)             // for trade channel
            _flags |= CHANNEL_FLAG_TRADE;

        if (ch->flags & CHANNEL_DBC_FLAG_CITY_ONLY2)        // for city only channels
            _flags |= CHANNEL_FLAG_CITY;

        if (ch->flags & CHANNEL_DBC_FLAG_LFG)               // for LFG channel
            _flags |= CHANNEL_FLAG_LFG;
        else                                                // for all other channels
            _flags |= CHANNEL_FLAG_NOT_LFG;
    }
    else                                                    // it's custom channel
    {
        _flags |= CHANNEL_FLAG_CUSTOM;

        // If storing custom channels in the db is enabled either load or save the channel
        if (sWorld->getBoolConfig(CONFIG_PRESERVE_CUSTOM_CHANNELS))
        {
            PreparedStatement *stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHANNEL);
            stmt->setString(0, name);
            stmt->setUInt32(1, _Team);
            PreparedQueryResult result = CharacterDatabase.Query(stmt);

            if (result) //load
            {
                Field* fields = result->Fetch();
                _announce = fields[0].GetBool();
                _ownership = fields[1].GetBool();
                _password  = fields[2].GetString();
                const char* db_BannedList = fields[3].GetCString();

                if (db_BannedList)
                {
                    Tokenizer tokens(db_BannedList, ' ');
                    for (Tokenizer::const_iterator i = tokens.begin(); i != tokens.end(); ++i)
                    {
                        ObjectGuid banned_guid((uint64)strtoull(*i, NULL, 10));
                        if (banned_guid)
                        {
                            TC_LOG_DEBUG("chat.system", "Channel(%s) loaded bannedStore guid: %s", name.c_str(), banned_guid.ToString().c_str());
                            bannedStore.insert(banned_guid);
                        }
                    }
                }
            }
            else // save
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHANNEL);
                stmt->setString(0, name);
                stmt->setUInt32(1, _Team);
                CharacterDatabase.Execute(stmt);
                TC_LOG_DEBUG("chat.system", "Channel(%s) saved in database", name.c_str());
            }

            _IsSaved = true;
        }
    }
}

void Channel::UpdateChannelInDB() const
{
    if (_IsSaved)
    {
        std::ostringstream banlist;
        BannedContainer::const_iterator iter;
        for (iter = bannedStore.begin(); iter != bannedStore.end(); ++iter)
            banlist << iter->GetRawValue() << ' ';

        std::string banListStr = banlist.str();

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHANNEL);
        stmt->setBool(0, _announce);
        stmt->setBool(1, _ownership);
        stmt->setString(2, _password);
        stmt->setString(3, banListStr);
        stmt->setString(4, _name);
        stmt->setUInt32(5, _Team);
        CharacterDatabase.Execute(stmt);

        TC_LOG_DEBUG("chat.system", "Channel(%s) updated in database", _name.c_str());
    }
}

void Channel::UpdateChannelUseageInDB() const
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHANNEL_USAGE);
    stmt->setString(0, _name);
    stmt->setUInt32(1, _Team);
    CharacterDatabase.Execute(stmt);
}

void Channel::SetModerator(ObjectGuid guid, bool set)
{
    if (playersStore[guid].IsModerator() != set)
    {
        const uint8 oldFlag = GetPlayerFlags(guid);
        playersStore[guid].SetModerator(set);

        WorldPacket data = MakeModeChange(guid, oldFlag);
        SendToAll(data);
    }
}

void Channel::SetMute(ObjectGuid guid, bool set)
{
    if (playersStore[guid].IsMuted() != set)
    {
        const uint8 oldFlag = GetPlayerFlags(guid);
        playersStore[guid].SetMuted(set);

        const WorldPacket data = MakeModeChange(guid, oldFlag);
        SendToAll(data);
    }
}

void Channel::CleanOldChannelsInDB()
{
    if (sWorld->getIntConfig(CONFIG_PRESERVE_CUSTOM_CHANNEL_DURATION) > 0)
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_OLD_CHANNELS);
        stmt->setUInt32(0, sWorld->getIntConfig(CONFIG_PRESERVE_CUSTOM_CHANNEL_DURATION) * DAY);
        CharacterDatabase.Execute(stmt);

        TC_LOG_DEBUG("chat.system", "Cleaned out unused custom chat channels.");
    }
}

void Channel::JoinChannel(Player* player, std::string const& pass)
{
    ObjectGuid guid = player->GetGUID();
    if (IsOn(guid))
    {
        // Do not send error message for built-in channels
        if (!IsConstant())
        {
            WorldPacket data = MakePlayerAlreadyMember(guid);
            SendToOne(std::move(data), guid);
        }
        return;
    }

    if (IsBanned(guid))
    {
        WorldPacket data = MakeBanned();
        SendToOne(std::move(data), guid);
        return;
    }

    if (!_password.empty() && pass != _password)
    {
        WorldPacket data = MakeWrongPassword();
        SendToOne(std::move(data), guid);
        return;
    }

    if (HasFlag(CHANNEL_FLAG_LFG) &&
        sWorld->getBoolConfig(CONFIG_RESTRICTED_LFG_CHANNEL) &&
        AccountMgr::IsPlayerAccount(player->GetSession()->GetSecurity()) && //FIXME: Move to RBAC
        player->GetGroup())
    {
        WorldPacket data = MakeNotInLfg();
        SendToOne(std::move(data), guid);
        return;
    }

    if (_announce && !player->GetSession()->HasPermission(rbac::RBAC_PERM_SILENTLY_JOIN_CHANNEL))
    {
        const WorldPacket data = MakeJoined(guid, _name);
        SendToAll(data);
    }

    PlayerInfo pinfo;
    pinfo.player = guid;
    pinfo.flags = MEMBER_FLAG_NONE;
    playersStore[guid] = pinfo;

    WorldPacket data = MakeYouJoined();
    SendToOne(std::move(data), guid);

    JoinNotify(guid);

    // Custom channel handling
    if (!IsConstant())
    {
        // Update last_used timestamp in db
        if (!playersStore.empty())
            UpdateChannelUseageInDB();

        // If the channel has no owner yet and ownership is allowed, set the new owner.
        if (!_ownerGUID && _ownership)
        {
            SetOwner(guid, playersStore.size() > 1);
            playersStore[guid].SetModerator(true);
        }
    }
}

bool Channel::LeaveChannel(Player* player)
{
    ObjectGuid guid = player->GetGUID();
    if (!IsOn(guid))
    {
        return false;
    }

    bool changeowner = playersStore[guid].IsOwner();

    playersStore.erase(guid);

    if (_announce && !player->GetSession()->HasPermission(rbac::RBAC_PERM_SILENTLY_JOIN_CHANNEL))
    {
        WorldPacket data = MakeLeft(guid, _name);
        SendToAll(data);
    }

    LeaveNotify(guid);

    if (!IsConstant())
    {
        // Update last_used timestamp in db
        UpdateChannelUseageInDB();

        // If the channel owner left and there are still playersStore inside, pick a new owner
        if (changeowner && _ownership && !playersStore.empty())
        {
            ObjectGuid newowner = playersStore.begin()->second.player;
            playersStore[newowner].SetModerator(true);
            SetOwner(newowner);
        }
    }
    return true;
}

bool Channel::KickOrBan(Player const* player, std::string const& badname, bool ban)
{
    ObjectGuid good = player->GetGUID();

    if (!IsOn(good))
    {
        WorldPacket data = MakeNotMember(_name);
        SendToOne(std::move(data), good);
        return false;
    }

    if (!playersStore[good].IsModerator() && !player->GetSession()->HasPermission(rbac::RBAC_PERM_CHANGE_CHANNEL_NOT_MODERATOR))
    {
        WorldPacket data = MakeNotModerator();
        SendToOne(std::move(data), good);
        return false;
    }

    Player* bad = sObjectAccessor->FindConnectedPlayerByName(badname);
    ObjectGuid victim = bad ? bad->GetGUID() : ObjectGuid::Empty;
    if (!victim || !IsOn(victim))
    {
        WorldPacket data = MakePlayerNotFound(badname);
        SendToOne(std::move(data), good);
        return false;
    }

    bool changeowner = _ownerGUID == victim;

    if (!player->GetSession()->HasPermission(rbac::RBAC_PERM_CHANGE_CHANNEL_NOT_MODERATOR) && changeowner && good != _ownerGUID)
    {
        WorldPacket data = MakeNotOwner();
        SendToOne(std::move(data), good);
        return false;
    }

    if (ban && !IsBanned(victim))
    {
        bannedStore.insert(victim);
        UpdateChannelInDB();

        if (!player->GetSession()->HasPermission(rbac::RBAC_PERM_SILENTLY_JOIN_CHANNEL))
        {
           auto packet = MakePlayerBanned(victim, good);
            SendToAll(packet);
        }
        else
        {
            auto packet = MakePlayerBanned(victim, good);
            bad->SendDirectMessage(packet);
            player->SendDirectMessage(std::move(packet));
        }
    }
    else if (!player->GetSession()->HasPermission(rbac::RBAC_PERM_SILENTLY_JOIN_CHANNEL))
    {
        auto packet = MakePlayerKicked(victim, good);
        SendToAll(packet);
    }
    else
    {
        auto packet = MakePlayerKicked(victim, good);
        bad->SendDirectMessage(packet);
        player->SendDirectMessage(std::move(packet));
    }

    bad->SendDirectMessage(MakeYouLeft(GetName(), GetChannelId(), IsConstant()));
    playersStore.erase(victim);



    if (changeowner && _ownership && !playersStore.empty())
    {
        ObjectGuid newowner = good;
        playersStore[newowner].SetModerator(true);
        SetOwner(newowner);
    }

    return true;
}

void Channel::UnBan(Player const* player, std::string const& badname)
{
    const ObjectGuid good = player->GetGUID();

    if (!IsOn(good))
    {
        WorldPacket data = MakeNotMember(_name);
        SendToOne(std::move(data), good);
        return;
    }

    if (!playersStore[good].IsModerator() && !player->GetSession()->HasPermission(rbac::RBAC_PERM_CHANGE_CHANNEL_NOT_MODERATOR))
    {
        WorldPacket data = MakeNotModerator();
        SendToOne(std::move(data), good);
        return;
    }

    const Player* bad = sObjectAccessor->FindConnectedPlayerByName(badname);
    const ObjectGuid victim = bad ? bad->GetGUID() : ObjectGuid::Empty;

    if (!victim || !IsBanned(victim))
    {
        WorldPacket data = MakePlayerNotFound(badname);
        SendToOne(std::move(data), good);
        return;
    }

    bannedStore.erase(victim);

    const WorldPacket data = MakePlayerUnbanned(victim, good);
    SendToAll(data);

    UpdateChannelInDB();
}

void Channel::ChangePassword(Player const* player, std::string const& pass)
{
    const ObjectGuid guid = player->GetGUID();

    if (!IsOn(guid))
    {
        WorldPacket data = MakeNotMember(_name);
        SendToOne(std::move(data), guid);
        return;
    }

    if (!playersStore[guid].IsModerator() && !player->GetSession()->HasPermission(rbac::RBAC_PERM_CHANGE_CHANNEL_NOT_MODERATOR))
    {
        WorldPacket data = MakeNotModerator();
        SendToOne(std::move(data), guid);
        return;
    }

    _password = pass;

    WorldPacket data = MakePasswordChanged(guid);
    SendToAll(data);

    UpdateChannelInDB();
}

void Channel::SetMode(Player const* player, std::string const& p2n, bool mod, bool set)
{
    const ObjectGuid guid = player->GetGUID();

    if (!IsOn(guid))
    {
        WorldPacket data = MakeNotMember(_name);
        SendToOne(std::move(data), guid);
        return;
    }

    if (!playersStore[guid].IsModerator() && !player->GetSession()->HasPermission(rbac::RBAC_PERM_CHANGE_CHANNEL_NOT_MODERATOR))
    {
        WorldPacket data = MakeNotModerator();
        SendToOne(std::move(data), guid);
        return;
    }

    if (guid == _ownerGUID && std::string(p2n) == player->GetName() && mod)
        return;

    const Player* newp = sObjectAccessor->FindConnectedPlayerByName(p2n);
    const ObjectGuid victim = newp ? newp->GetGUID() : ObjectGuid::Empty;

    if (!victim || !IsOn(victim) ||
        (player->GetTeam() != newp->GetTeam() &&
        (!player->GetSession()->HasPermission(rbac::RBAC_PERM_TWO_SIDE_INTERACTION_CHANNEL) ||
        !newp->GetSession()->HasPermission(rbac::RBAC_PERM_TWO_SIDE_INTERACTION_CHANNEL))))
    {
        WorldPacket data = MakePlayerNotFound(p2n);
        SendToOne(std::move(data), guid);
        return;
    }

    if (_ownerGUID == victim && _ownerGUID != guid)
    {
        WorldPacket data = MakeNotOwner();
        SendToOne(std::move(data), guid);
        return;
    }

    if (mod)
        SetModerator(newp->GetGUID(), set);
    else
        SetMute(newp->GetGUID(), set);
}

void Channel::SetOwner(Player const* player, std::string const& newname)
{
    ObjectGuid guid = player->GetGUID();

    if (!IsOn(guid))
    {
        WorldPacket data = MakeNotMember(_name);
        SendToOne(std::move(data), guid);
        return;
    }

    if (!player->GetSession()->HasPermission(rbac::RBAC_PERM_CHANGE_CHANNEL_NOT_MODERATOR) && guid != _ownerGUID)
    {
        WorldPacket data = MakeNotOwner();
        SendToOne(std::move(data), guid);
        return;
    }

    Player* newp = sObjectAccessor->FindConnectedPlayerByName(newname);
    ObjectGuid victim = newp ? newp->GetGUID() : ObjectGuid::Empty;

    if (!victim || !IsOn(victim) ||
        (player->GetTeam() != newp->GetTeam() &&
        (!player->GetSession()->HasPermission(rbac::RBAC_PERM_TWO_SIDE_INTERACTION_CHANNEL) ||
        !newp->GetSession()->HasPermission(rbac::RBAC_PERM_TWO_SIDE_INTERACTION_CHANNEL))))
    {
        WorldPacket data = MakePlayerNotFound(newname);
        SendToOne(std::move(data), guid);
        return;
    }

    playersStore[victim].SetModerator(true);
    SetOwner(victim);
}

void Channel::SendWhoOwner(ObjectGuid guid)
{
    WorldPacket data(0);
    if (IsOn(guid))
       data = MakeChannelOwner();
    else
       data = MakeNotMember(_name);
    SendToOne(std::move(data), guid);
}

void Channel::List(Player const* player)
{
    ObjectGuid guid = player->GetGUID();

    if (!IsOn(guid))
    {
        WorldPacket data = MakeNotMember(_name);
        SendToOne(std::move(data), guid);
        return;
    }

    TC_LOG_DEBUG("chat.system", "SMSG_CHANNEL_LIST %s Channel: %s",
        player->GetSession()->GetPlayerInfo().c_str(), GetName().c_str());

    WorldPacket data(SMSG_CHANNEL_LIST, 1+(GetName().size()+1)+1+4+playersStore.size()*(8+1));
    data << uint8(1);                                   // channel type?
    data << GetName();                                  // channel name
    data << uint8(GetFlags());                          // channel flags?

    size_t pos = data.wpos();
    data << uint32(0);                                  // size of list, placeholder

    uint32 gmLevelInWhoList = sWorld->getIntConfig(CONFIG_GM_LEVEL_IN_WHO_LIST);

    uint32 count  = 0;
    for (PlayerContainer::const_iterator i = playersStore.begin(); i != playersStore.end(); ++i)
    {
        Player* member = ObjectAccessor::FindConnectedPlayer(i->first);

        // PLAYER can't see MODERATOR, GAME MASTER, ADMINISTRATOR characters
        // MODERATOR, GAME MASTER, ADMINISTRATOR can see all
        if (member &&
            (player->GetSession()->HasPermission(rbac::RBAC_PERM_WHO_SEE_ALL_SEC_LEVELS) ||
             member->GetSession()->GetSecurity() <= gmLevelInWhoList) &&
            member->IsVisibleGloballyFor(player) &&
            (!member->IsSecretQueuingActive() || player->IsGameMaster())) //RG Custom "Verstecktes Anmelden"
        {
            data << uint64(i->first);
            data << uint8(i->second.flags);             // flags seems to be changed...
            ++count;
        }
    }

    data.put<uint32>(pos, count);

    SendToOne(std::move(data), guid);
}

void Channel::Announce(Player const* player)
{
    ObjectGuid guid = player->GetGUID();

    if (!IsOn(guid))
    {
        WorldPacket data = MakeNotMember(_name);
        SendToOne(std::move(data), guid);
        return;
    }

    if (!playersStore[guid].IsModerator() && !player->GetSession()->HasPermission(rbac::RBAC_PERM_CHANGE_CHANNEL_NOT_MODERATOR))
    {
        WorldPacket data = MakeNotModerator();
        SendToOne(std::move(data), guid);
        return;
    }

    _announce = !_announce;

    WorldPacket data(0);
    if (_announce)
       data = MakeAnnouncementsOn(guid);
    else
       data = MakeAnnouncementsOff(guid);
    SendToAll(data);

    UpdateChannelInDB();
}

void Channel::Say(ObjectGuid guid, std::string const& what, uint32 lang)
{
    if (what.empty())
        return;

    // TODO: Add proper RBAC check
    if (sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_CHANNEL))
        lang = LANG_UNIVERSAL;

    if (!IsOn(guid))
    {
        WorldPacket data = MakeNotMember(_name);
        SendToOne(std::move(data), guid);
        return;
    }

    if (playersStore[guid].IsMuted())
    {
        WorldPacket data = MakeMuted();
        SendToOne(std::move(data), guid);
        return;
    }

    WorldPacket data;
    if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
        ChatHandler::BuildChatPacket(data, CHAT_MSG_CHANNEL, Language(lang), player, player, what, 0, _name);
    else
        ChatHandler::BuildChatPacket(data, CHAT_MSG_CHANNEL, Language(lang), guid, guid, what, 0, "", "", 0, false, _name);

    SendToAll(data, !playersStore[guid].IsModerator() ? guid : ObjectGuid::Empty);
}

void Channel::Invite(Player const* player, std::string const& newname)
{
    ObjectGuid guid = player->GetGUID();

    if (!IsOn(guid))
    {
        WorldPacket data = MakeNotMember(_name);
        SendToOne(std::move(data), guid);
        return;
    }

    Player* newp = sObjectAccessor->FindConnectedPlayerByName(newname);
    if (!newp || !newp->isGMVisible())
    {
        WorldPacket data = MakePlayerNotFound(newname);
        SendToOne(std::move(data), guid);
        return;
    }

    //RG Custom "Verstecktes Anmelden"
    if (!player->IsGameMaster() && newp->IsSecretQueuingActive())
    {
        WorldPacket data = MakePlayerNotFound(newname);
        SendToOne(std::move(data), guid);
        return;
    }

    if (IsBanned(newp->GetGUID()))
    {
        WorldPacket data = MakePlayerInviteBanned(newname);
        SendToOne(std::move(data), guid);
        return;
    }

    if (newp->GetTeam() != player->GetTeam() &&
        (!player->GetSession()->HasPermission(rbac::RBAC_PERM_TWO_SIDE_INTERACTION_CHANNEL) ||
        !newp->GetSession()->HasPermission(rbac::RBAC_PERM_TWO_SIDE_INTERACTION_CHANNEL)))
    {
        WorldPacket data = MakeInviteWrongFaction();
        SendToOne(std::move(data), guid);
        return;
    }

    if (IsOn(newp->GetGUID()))
    {
        WorldPacket data = MakePlayerAlreadyMember(newp->GetGUID());
        SendToOne(std::move(data), guid);
        return;
    }

    if (!newp->GetSocial()->HasIgnore(guid))
    {
        WorldPacket data = MakeInvite(guid);
        SendToOne(std::move(data), newp->GetGUID());
    }

    WorldPacket data = MakePlayerInvited(newp->GetName());
    SendToOne(std::move(data), guid);
}

void Channel::SetOwner(ObjectGuid guid, bool exclaim)
{
    if (_ownerGUID)
    {
        // [] will re-add player after it possible removed
        PlayerContainer::iterator p_itr = playersStore.find(_ownerGUID);
        if (p_itr != playersStore.end())
            p_itr->second.SetOwner(false);
    }

    _ownerGUID = guid;
    if (_ownerGUID)
    {
        const uint8 oldFlag = GetPlayerFlags(_ownerGUID);
        playersStore[_ownerGUID].SetModerator(true);
        playersStore[_ownerGUID].SetOwner(true);

        const WorldPacket data = MakeModeChange(_ownerGUID, oldFlag);
        SendToAll(data);

        if (exclaim)
        {
            const WorldPacket data = MakeOwnerChanged(_ownerGUID);
            SendToAll(data);
        }

        UpdateChannelInDB();
    }
}

void Channel::SendToAll(const WorldPacket& data, ObjectGuid guid)
{
    for (PlayerContainer::const_iterator i = playersStore.begin(); i != playersStore.end(); ++i)
        if (Player* player = ObjectAccessor::FindConnectedPlayer(i->first))
            if (!guid || !player->GetSocial()->HasIgnore(guid))
                player->SendDirectMessage(data);
}

void Channel::SendToAllButOne(const WorldPacket& data, ObjectGuid who)
{
    for (PlayerContainer::const_iterator i = playersStore.begin(); i != playersStore.end(); ++i)
        if (i->first != who)
            if (Player* player = ObjectAccessor::FindConnectedPlayer(i->first))
                player->SendDirectMessage(data);
}

void Channel::SendToOne(WorldPacket&& data, ObjectGuid who)
{
    if (Player* player = ObjectAccessor::FindConnectedPlayer(who))
        player->GetSession()->SendPacket(std::move(data));
}

void Channel::Voice(ObjectGuid /*guid1*/, ObjectGuid /*guid2*/)
{

}

void Channel::DeVoice(ObjectGuid /*guid1*/, ObjectGuid /*guid2*/)
{

}
WorldPacket Channel::MakeNotifyPacket(uint8 notify_type)
{
    return Channel::MakeNotifyPacket(notify_type, _name);
}

WorldPacket Channel::MakeNotifyPacket(uint8 notify_type, std::string const& name)
{
    WorldPacket data(SMSG_CHANNEL_NOTIFY, 1 + name.size());
    data << uint8(notify_type);
    data << name;
    return data;
}

WorldPacket Channel::MakeJoined(ObjectGuid guid, std::string const& name)
{
    WorldPacket data = MakeNotifyPacket(CHAT_JOINED_NOTICE, name);
    data << uint64(guid);
    return data;
}

WorldPacket Channel::MakeLeft(ObjectGuid guid, std::string const& name)
{
    WorldPacket data = MakeNotifyPacket(CHAT_LEFT_NOTICE, name);
    data << guid;
    return data;
}

WorldPacket Channel::MakeYouJoined()
{
    WorldPacket data = MakeNotifyPacket(CHAT_YOU_JOINED_NOTICE);
    data << uint8(GetFlags());
    data << uint32(GetChannelId());
    data << uint32(0);
    return data;
}

WorldPacket Channel::MakeYouLeft(std::string const& name, uint32 channelId, bool isConstant)
{
    WorldPacket data = MakeNotifyPacket(CHAT_YOU_LEFT_NOTICE, name);
    data << uint32(channelId);
    data << uint8(isConstant);
    return data;
}

WorldPacket Channel::MakeWrongPassword()
{
    return MakeNotifyPacket(CHAT_WRONG_PASSWORD_NOTICE);
}

WorldPacket Channel::MakeNotMember(std::string const& name)
{
    return MakeNotifyPacket(CHAT_NOT_MEMBER_NOTICE, name);
}

WorldPacket Channel::MakeNotModerator()
{
    return MakeNotifyPacket(CHAT_NOT_MODERATOR_NOTICE);
}

WorldPacket Channel::MakePasswordChanged(ObjectGuid guid)
{
    WorldPacket data = MakeNotifyPacket(CHAT_PASSWORD_CHANGED_NOTICE);
    data << guid;
    return data;
}

WorldPacket Channel::MakeOwnerChanged(ObjectGuid guid)
{
    WorldPacket data = MakeNotifyPacket(CHAT_OWNER_CHANGED_NOTICE);
    data << guid;
    return data;
}

WorldPacket Channel::MakePlayerNotFound(std::string const& name)
{
    WorldPacket data = MakeNotifyPacket(CHAT_PLAYER_NOT_FOUND_NOTICE);
    data << name;
    return data;
}

WorldPacket Channel::MakeNotOwner()
{
   return MakeNotifyPacket(CHAT_NOT_OWNER_NOTICE);
}

WorldPacket Channel::MakeChannelOwner()
{
    const std::string_view name = player::PlayerCache::GetName(_ownerGUID);

    WorldPacket data = MakeNotifyPacket(CHAT_CHANNEL_OWNER_NOTICE);
    data << ((IsConstant() || !_ownerGUID) ? "Nobody" : name);
    return data;
}

WorldPacket Channel::MakeModeChange(ObjectGuid guid, uint8 oldflags)
{
    WorldPacket data = MakeNotifyPacket(CHAT_MODE_CHANGE_NOTICE);
    data << guid;
    data << uint8(oldflags);
    data << uint8(GetPlayerFlags(guid));
    return data;
}

WorldPacket Channel::MakeAnnouncementsOn(ObjectGuid guid)
{
    WorldPacket data = MakeNotifyPacket(CHAT_ANNOUNCEMENTS_ON_NOTICE);
    data << uint64(guid);
    return data;
}

WorldPacket Channel::MakeAnnouncementsOff(ObjectGuid guid)
{
    WorldPacket data = MakeNotifyPacket(CHAT_ANNOUNCEMENTS_OFF_NOTICE);
    data << guid;
    return data;
}

WorldPacket Channel::MakeMuted()
{
    return MakeNotifyPacket(CHAT_MUTED_NOTICE);
}

WorldPacket Channel::MakePlayerKicked(ObjectGuid bad, ObjectGuid good)
{
    WorldPacket data = MakeNotifyPacket(CHAT_PLAYER_KICKED_NOTICE);
    data << uint64(bad);
    data << uint64(good);
    return data;
}

WorldPacket Channel::MakeBanned()
{
   return MakeNotifyPacket(CHAT_BANNED_NOTICE);
}

WorldPacket Channel::MakePlayerBanned(ObjectGuid bad, ObjectGuid good)
{
    WorldPacket data = MakeNotifyPacket(CHAT_PLAYER_BANNED_NOTICE);
    data << uint64(bad);
    data << uint64(good);
    return data;
}

WorldPacket Channel::MakePlayerUnbanned(ObjectGuid bad, ObjectGuid good)
{
    WorldPacket data = MakeNotifyPacket(CHAT_PLAYER_UNBANNED_NOTICE);
    data << uint64(bad);
    data << uint64(good);
    return data;
}

WorldPacket Channel::MakePlayerNotBanned(const std::string &name)
{
    WorldPacket data = MakeNotifyPacket(CHAT_PLAYER_NOT_BANNED_NOTICE);
    data << name;

    return data;
}

WorldPacket Channel::MakePlayerAlreadyMember(ObjectGuid guid)
{
    WorldPacket data = MakeNotifyPacket(CHAT_PLAYER_ALREADY_MEMBER_NOTICE);
    data << uint64(guid);

    return data;
}

WorldPacket Channel::MakeInvite(ObjectGuid guid)
{
    WorldPacket data = MakeNotifyPacket(CHAT_INVITE_NOTICE);
    data << uint64(guid);
    return data;
}

WorldPacket Channel::MakeInviteWrongFaction()
{
    return MakeNotifyPacket(CHAT_INVITE_WRONG_FACTION_NOTICE);
}

WorldPacket Channel::MakeWrongFaction()
{
    return MakeNotifyPacket(CHAT_WRONG_FACTION_NOTICE);
}

WorldPacket Channel::MakeInvalidName()
{
    return MakeNotifyPacket(CHAT_INVALID_NAME_NOTICE);
}

WorldPacket Channel::MakeNotModerated()
{
    return MakeNotifyPacket(CHAT_NOT_MODERATED_NOTICE);
}

WorldPacket Channel::MakePlayerInvited(const std::string& name)
{
    WorldPacket data = MakeNotifyPacket(CHAT_PLAYER_INVITED_NOTICE);
    data << name;
    return data;
}

WorldPacket Channel::MakePlayerInviteBanned(const std::string& name)
{
    WorldPacket data = MakeNotifyPacket(CHAT_PLAYER_INVITE_BANNED_NOTICE);
    data << name;
    return data;
}

WorldPacket Channel::MakeThrottled()
{
    return MakeNotifyPacket(CHAT_THROTTLED_NOTICE);
}

WorldPacket Channel::MakeNotInArea()
{
    return MakeNotifyPacket(CHAT_NOT_IN_AREA_NOTICE);
}

WorldPacket Channel::MakeNotInLfg()
{
    return MakeNotifyPacket(CHAT_NOT_IN_LFG_NOTICE);
}

WorldPacket Channel::MakeVoiceOn(ObjectGuid guid)
{
    WorldPacket data = MakeNotifyPacket(CHAT_VOICE_ON_NOTICE);
    data << uint64(guid);
    return data;
}

WorldPacket Channel::MakeVoiceOff(ObjectGuid guid)
{
    WorldPacket data = MakeNotifyPacket(CHAT_VOICE_OFF_NOTICE);
    data << uint64(guid);

    return data;
}

void Channel::JoinNotify(ObjectGuid guid)
{
    WorldPacket data(IsConstant() ? SMSG_USERLIST_ADD : SMSG_USERLIST_UPDATE, 8 + 1 + 1 + 4 + GetName().size());
    data << uint64(guid);
    data << uint8(GetPlayerFlags(guid));
    data << uint8(GetFlags());
    data << uint32(GetNumPlayers());
    data << GetName();

    if (IsConstant())
        SendToAllButOne(data, guid);
    else
        SendToAll(data);
}

void Channel::LeaveNotify(ObjectGuid guid)
{
    WorldPacket data(SMSG_USERLIST_REMOVE, 8 + 1 + 4 + GetName().size());
    data << uint64(guid);
    data << uint8(GetFlags());
    data << uint32(GetNumPlayers());
    data << GetName();

    if (IsConstant())
        SendToAllButOne(data, guid);
    else
        SendToAll(data);
}
