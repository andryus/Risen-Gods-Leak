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

#include "Common.h"
#include "Language.h"
#include "DatabaseEnv.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "Player.h"
#include "GameTime.h"
#include "GossipDef.h"
#include "World.h"
#include "ObjectMgr.h"
#include "GuildMgr.h"
#include "WorldSession.h"
#include "LootMgr.h"
#include "Chat.h"
#include "zlib.h"
#include "ObjectAccessor.h"
#include "Object.h"
#include "Battleground.h"
#include "OutdoorPvP.h"
#include "SocialMgr.h"
#include "AccountMgr.h"
#include "CreatureAI.h"
#include "DBCEnums.h"
#include "ScriptMgr.h"
#include "MapManager.h"
#include "GameObjectAI.h"
#include "Group.h"
#include "Spell.h"
#include "BattlegroundMgr.h"
#include "Battlefield.h"
#include "BattlefieldMgr.h"
#include "WhoListStorage.h"
#include "InstanceScript.h"
#include "ComplaintMgr.hpp"
#include "DBCStores.h"

#include "UnitHooks.h"

void WorldSession::HandleRepopRequestOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Recvd CMSG_REPOP_REQUEST Message");

    recvData.read_skip<uint8>();

    if (GetPlayer()->IsAlive() || GetPlayer()->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
        return;

    if (GetPlayer()->HasAuraType(SPELL_AURA_PREVENT_RESURRECTION))
        return; // silently return, client should display the error by itself

    // the world update order is sessions, players, creatures
    // the netcode runs in parallel with all of these
    // creatures can kill players
    // so if the server is lagging enough the player can
    // release spirit after he's killed but before he is updated
    if (GetPlayer()->getDeathState() == JUST_DIED)
    {
        TC_LOG_DEBUG("network", "HandleRepopRequestOpcode: got request after player %s(%d) was killed and before he was updated",
            GetPlayer()->GetName().c_str(), GetPlayer()->GetGUID().GetCounter());
        GetPlayer()->KillPlayer();
    }

    //this is spirit release confirm?
    GetPlayer()->RemoveGhoul();
    GetPlayer()->RemovePet(NULL, PET_SAVE_NOT_IN_SLOT, true);
    GetPlayer()->BuildPlayerRepop();
    GetPlayer()->RepopAtGraveyard();
}

void WorldSession::HandleGossipSelectOptionOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_GOSSIP_SELECT_OPTION");

    uint32 gossipListId;
    uint32 menuId;
    ObjectGuid guid;
    std::string code = "";

    recvData >> guid >> menuId >> gossipListId;

    if (!GetPlayer()->PlayerTalkClass->GetGossipMenu().GetItem(gossipListId))
    {
        recvData.rfinish();
        return;
    }

    if (GetPlayer()->PlayerTalkClass->IsGossipOptionCoded(gossipListId))
        recvData >> code;

    // Prevent cheating on C++ scripted menus
    if (GetPlayer()->PlayerTalkClass->GetGossipMenu().GetSenderGUID() != guid)
        return;

    Creature* unit = NULL;
    GameObject* go = NULL;
    if (guid.IsCreatureOrVehicle())
    {
        unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
        if (!unit)
        {
            TC_LOG_DEBUG("network", "WORLD: HandleGossipSelectOptionOpcode - %s not found or you can't interact with him.", guid.ToString().c_str());
            return;
        }
    }
    else if (guid.IsGameObject())
    {
        go = GetPlayer()->GetMap()->GetGameObject(guid);
        if (!go)
        {
            TC_LOG_DEBUG("network", "WORLD: HandleGossipSelectOptionOpcode - %s not found.", guid.ToString().c_str());
            return;
        }
    }
    else
    {
        TC_LOG_DEBUG("network", "WORLD: HandleGossipSelectOptionOpcode - unsupported %s.", guid.ToString().c_str());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    if ((unit && unit->GetCreatureTemplate()->ScriptID != unit->LastUsedScriptID) || (go && go->GetGOInfo()->ScriptId != go->LastUsedScriptID))
    {
        TC_LOG_DEBUG("network", "WORLD: HandleGossipSelectOptionOpcode - Script reloaded while in use, ignoring and set new scipt id");
        if (unit)
            unit->LastUsedScriptID = unit->GetCreatureTemplate()->ScriptID;
        if (go)
            go->LastUsedScriptID = go->GetGOInfo()->ScriptId;
        GetPlayer()->PlayerTalkClass->SendCloseGossip();
        return;
    }
    if (!code.empty())
    {
        if (unit)
        {
            if (UnitAI* ai = unit->AI())
                ai->sGossipSelectCode(GetPlayer(), menuId, gossipListId, code.c_str());
            if (!sScriptMgr->OnGossipSelectCode(GetPlayer(), unit, GetPlayer()->PlayerTalkClass->GetGossipOptionSender(gossipListId), GetPlayer()->PlayerTalkClass->GetGossipOptionAction(gossipListId), code.c_str()))
                GetPlayer()->OnGossipSelect(unit, gossipListId, menuId);
        }
        else
        {
            go->AI()->GossipSelectCode(GetPlayer(), menuId, gossipListId, code.c_str());
            if (!sScriptMgr->OnGossipSelectCode(GetPlayer(), go, GetPlayer()->PlayerTalkClass->GetGossipOptionSender(gossipListId), GetPlayer()->PlayerTalkClass->GetGossipOptionAction(gossipListId), code.c_str()))
                GetPlayer()->OnGossipSelect(go, gossipListId, menuId);
        }
    }
    else
    {
        if (unit)
        {
            if (*unit <<= Query::CanSeeGossipOption(gossipListId).Bind(GetPlayer()))
                *unit <<= Fire::GossipOption(gossipListId).Bind(GetPlayer());

            if (UnitAI* ai = unit->AI())
                ai->sGossipSelect(GetPlayer(), menuId, gossipListId);
            if (!sScriptMgr->OnGossipSelect(GetPlayer(), unit, GetPlayer()->PlayerTalkClass->GetGossipOptionSender(gossipListId), GetPlayer()->PlayerTalkClass->GetGossipOptionAction(gossipListId)))
                GetPlayer()->OnGossipSelect(unit, gossipListId, menuId);
        }
        else
        {
            go->AI()->GossipSelect(GetPlayer(), menuId, gossipListId);
            if (!sScriptMgr->OnGossipSelect(GetPlayer(), go, GetPlayer()->PlayerTalkClass->GetGossipOptionSender(gossipListId), GetPlayer()->PlayerTalkClass->GetGossipOptionAction(gossipListId)))
                GetPlayer()->OnGossipSelect(go, gossipListId, menuId);
        }
    }
}

void WorldSession::HandleWhoOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Recvd CMSG_WHO Message");

    uint32 matchCount = 0;

    uint32 levelMin, levelMax, racemask, classmask, zonesCount, strCount;
    uint32 zoneids[10];                                     // 10 is client limit
    std::string packetPlayerName, packetGuildName;

    recvData >> levelMin;                                   // maximal player level, default 0
    recvData >> levelMax;                                   // minimal player level, default 100 (MAX_LEVEL)
    recvData >> packetPlayerName;                           // player name, case sensitive...

    recvData >> packetGuildName;                            // guild name, case sensitive...

    recvData >> racemask;                                   // race mask
    recvData >> classmask;                                  // class mask
    recvData >> zonesCount;                                 // zones count, client limit = 10 (2.0.10)

    if (zonesCount > 10)
        return;                                             // can't be received from real client or broken packet

    for (uint32 i = 0; i < zonesCount; ++i)
    {
        uint32 temp;
        recvData >> temp;                                   // zone id, 0 if zone is unknown...
        zoneids[i] = temp;
        TC_LOG_DEBUG("network", "Zone %u: %u", i, zoneids[i]);
    }

    recvData >> strCount;                                   // user entered strings count, client limit=4 (checked on 2.0.10)

    if (strCount > 4)
        return;                                             // can't be received from real client or broken packet

    TC_LOG_DEBUG("network", "Minlvl %u, maxlvl %u, name %s, guild %s, racemask %u, classmask %u, zones %u, strings %u", levelMin, levelMax, packetPlayerName.c_str(), packetGuildName.c_str(), racemask, classmask, zonesCount, strCount);

    std::wstring str[4];                                    // 4 is client limit
    for (uint32 i = 0; i < strCount; ++i)
    {
        std::string temp;
        recvData >> temp;                                   // user entered string, it used as universal search pattern(guild+player name)?

        if (!Utf8toWStr(temp, str[i]))
            continue;

        wstrToLower(str[i]);

        TC_LOG_DEBUG("network", "String %u: %s", i, temp.c_str());
    }

    std::wstring wpacketPlayerName;
    std::wstring wpacketGuildName;
    if (!(Utf8toWStr(packetPlayerName, wpacketPlayerName) && Utf8toWStr(packetGuildName, wpacketGuildName)))
        return;

    wstrToLower(wpacketPlayerName);
    wstrToLower(wpacketGuildName);

    // client send in case not set max level value 100 but Trinity supports 255 max level,
    // update it to show GMs with characters after 100 level
    if (levelMax >= MAX_LEVEL)
        levelMax = STRONG_MAX_LEVEL;

    uint32 team = GetPlayer()->GetTeam();

    uint32 gmLevelInWhoList  = sWorld->getIntConfig(CONFIG_GM_LEVEL_IN_WHO_LIST);
    uint32 displayCount = 0;

    WorldPacket data(SMSG_WHO, 500);                      // guess size
    data << uint32(matchCount);                           // placeholder, count of players matching criteria
    data << uint32(displayCount);                         // placeholder, count of players displayed

    WhoListInfoVector const& whoList = sWhoListStorageMgr->GetWhoList();
    for (WhoListPlayerInfo const& target : whoList)
    {
        // player can see member of other team only if CONFIG_ALLOW_TWO_SIDE_WHO_LIST
        if (target.GetTeam() != team && !HasPermission(rbac::RBAC_PERM_TWO_SIDE_WHO_LIST))
            continue;

        // player can see MODERATOR, GAME MASTER, ADMINISTRATOR only if CONFIG_GM_IN_WHO_LIST
        if (!HasPermission(rbac::RBAC_PERM_WHO_SEE_ALL_SEC_LEVELS) && target.GetSecurity() > gmLevelInWhoList)
            continue;

        // check if target is globally visible for player
        if (GetPlayer()->GetGUID() != ObjectGuid(target.GetGuid()) && !target.IsVisible())
            if (AccountMgr::IsPlayerAccount(GetPlayer()->GetSession()->GetSecurity()) || target.GetSecurity() > GetPlayer()->GetSession()->GetSecurity())
                continue;

        // check if target's level is in level range
        uint8 lvl = target.GetLevel();
        if (lvl < levelMin || lvl > levelMax)
            continue;

        // check if class matches classmask
        uint8 class_ = target.GetClass();
        if (!(classmask & (1 << class_)))
            continue;

        // check if race matches racemask
        uint32 race = target.GetRace();
        if (!(racemask & (1 << race)))
            continue;

        if (!GetPlayer()->IsGameMaster() && target.GetSecretQueuing())
            continue;

        uint32 playerZoneId = target.GetZoneId();
        uint8 gender = target.GetGender();

        if (!GetPlayer()->IsGameMaster() && target.GetOrgZoneId() != 0)
            playerZoneId = target.GetOrgZoneId();

        bool showZones = true;
        for (uint32 i = 0; i < zonesCount; ++i)
        {
            if (zoneids[i] == playerZoneId)
            {
                showZones = true;
                break;
            }

            showZones = false;
        }
        if (!showZones)
            continue;

        std::wstring const& wideplayername = target.GetWidePlayerName();
        if (!(wpacketPlayerName.empty() || wideplayername.find(wpacketPlayerName) != std::wstring::npos))
            continue;

        std::wstring const& wideguildname = target.GetWideGuildName();
        if (!(wpacketGuildName.empty() || wideguildname.find(wpacketGuildName) != std::wstring::npos))
            continue;

        std::string aname;
        if (AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(playerZoneId))
            aname = areaEntry->area_name[GetSessionDbcLocale()];

        bool hideArenaMatches = !GetPlayer()->IsGameMaster() && target.GetOrgZoneId() != 0;

        bool s_show = true;
        for (uint32 i = 0; i < strCount; ++i)
        {
            if (!str[i].empty())
            {
                if (wideguildname.find(str[i]) != std::wstring::npos ||
                    wideplayername.find(str[i]) != std::wstring::npos ||
                    (Utf8FitTo(aname, str[i]) && !hideArenaMatches))
                {
                    s_show = true;
                    break;
                }
                s_show = false;
            }
        }
        if (!s_show)
            continue;

        // 49 is maximum player count sent to client - can be overridden
        // through config, but is unstable
        if ((matchCount++) >= sWorld->getIntConfig(CONFIG_MAX_WHO))
            continue;

        data << target.GetPlayerName();                   // player name
        data << target.GetGuildName();                    // guild name
        data << uint32(lvl);                              // player level
        data << uint32(class_);                           // player class
        data << uint32(race);                             // player race
        data << uint8(gender);                            // player gender
        data << uint32(playerZoneId);                     // player zone id

        ++displayCount;
    }

    data.put(0, displayCount);                            // insert right count, count displayed
    data.put(4, matchCount);                              // insert right count, count of matches

    SendPacket(std::move(data));
    TC_LOG_DEBUG("network", "WORLD: Send SMSG_WHO Message");
}

void WorldSession::HandleLogoutRequestOpcode(WorldPacket& /*recvData*/)
{
    TC_LOG_DEBUG("network", "WORLD: Recvd CMSG_LOGOUT_REQUEST Message, security - %u", GetSecurity());

    if (ObjectGuid lguid = GetPlayer()->GetLootGUID())
        DoLootRelease(lguid);

    bool instantLogout = (GetPlayer()->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING) && !GetPlayer()->IsInCombat()) ||
                         GetPlayer()->IsInFlight() || HasPermission(rbac::RBAC_PERM_INSTANT_LOGOUT);

    /// TODO: Possibly add RBAC permission to log out in combat
    bool canLogoutInCombat = GetPlayer()->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING);

    uint32 reason = 0;
    if (GetPlayer()->IsInCombat() && !canLogoutInCombat)
        reason = 1;
    else if (GetPlayer()->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR))
        reason = 3;                                         // is jumping or falling
    else if (GetPlayer()->duel || GetPlayer()->HasAura(9454)) // is dueling or frozen by GM via freeze command
        reason = 2;                                         // FIXME - Need the correct value

    WorldPacket data(SMSG_LOGOUT_RESPONSE, 1+4);
    data << uint32(reason);
    data << uint8(instantLogout);
    SendPacket(std::move(data));

    if (reason)
    {
        LogoutRequest(0);
        return;
    }

    userRequestedLogout = true;

    //instant logout in taverns/cities or on taxi or for admins, gm's, mod's if its enabled in worldserver.conf
    if (instantLogout)
    {
        LogoutPlayer(true);
        return;
    }

    // not set flags if player can't free move to prevent lost state at logout cancel
    if (GetPlayer()->CanFreeMove())
    {
        if (GetPlayer()->GetStandState() == UNIT_STAND_STATE_STAND)
            GetPlayer()->SetStandState(UNIT_STAND_STATE_SIT);

        WorldPacket data(SMSG_FORCE_MOVE_ROOT, (8+4));    // guess size
        data << GetPlayer()->GetPackGUID();
        data << (uint32)2;
        SendPacket(std::move(data));

        GetPlayer()->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
    }

    LogoutRequest(time(NULL));
}

void WorldSession::HandlePlayerLogoutOpcode(WorldPacket& /*recvData*/)
{
    TC_LOG_DEBUG("network", "WORLD: Recvd CMSG_PLAYER_LOGOUT Message");
}

void WorldSession::HandleLogoutCancelOpcode(WorldPacket& /*recvData*/)
{
    TC_LOG_DEBUG("network", "WORLD: Recvd CMSG_LOGOUT_CANCEL Message");

    // Player have already logged out serverside, too late to cancel
    if (!GetPlayer())
        return;

    LogoutRequest(0);

    WorldPacket data(SMSG_LOGOUT_CANCEL_ACK, 0);
    SendPacket(std::move(data));

    // not remove flags if can't free move - its not set in Logout request code.
    if (GetPlayer()->CanFreeMove())
    {
        //!we can move again
        WorldPacket data(SMSG_FORCE_MOVE_UNROOT, 8);       // guess size
        data << GetPlayer()->GetPackGUID();
        data << uint32(0);
        SendPacket(std::move(data));

        //! Stand Up
        GetPlayer()->SetStandState(UNIT_STAND_STATE_STAND);

        //! DISABLE_ROTATE
        GetPlayer()->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
    }

    TC_LOG_DEBUG("network", "WORLD: Sent SMSG_LOGOUT_CANCEL_ACK Message");
}

void WorldSession::HandleTogglePvP(WorldPacket& recvData)
{
    // this opcode can be used in two ways: Either set explicit new status or toggle old status
    if (recvData.size() == 1)
    {
        bool newPvPStatus;
        recvData >> newPvPStatus;
        GetPlayer()->ApplyModFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP, newPvPStatus);
        GetPlayer()->ApplyModFlag(PLAYER_FLAGS, PLAYER_FLAGS_PVP_TIMER, !newPvPStatus);
    }
    else
    {
        GetPlayer()->ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP);
        GetPlayer()->ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_PVP_TIMER);
    }

    if (GetPlayer()->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP))
    {
        if (!GetPlayer()->IsPvP() || GetPlayer()->pvpInfo.EndTimer)
            GetPlayer()->UpdatePvP(true, true);
    }
    else
    {
        if (!GetPlayer()->pvpInfo.IsHostile && GetPlayer()->IsPvP())
            GetPlayer()->pvpInfo.EndTimer = time(NULL);     // start toggle-off
    }

    //if (OutdoorPvP* pvp = GetPlayer()->GetOutdoorPvP())
    //    pvp->HandlePlayerActivityChanged(GetPlayer());
}

void WorldSession::HandleZoneUpdateOpcode(WorldPacket& recvData)
{
    uint32 newZone;
    recvData >> newZone;

    TC_LOG_DEBUG("network", "WORLD: Recvd ZONE_UPDATE: %u", newZone);

    // use server side data, but only after update the player position. See Player::UpdatePosition().
    GetPlayer()->SetNeedsZoneUpdate(true);

    //GetPlayer()->SendInitWorldStates(true, newZone);
}

void WorldSession::HandleSetSelectionOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData >> guid;

    GetPlayer()->SetSelection(guid);
}

void WorldSession::HandleStandStateChangeOpcode(WorldPacket& recvData)
{
    // TC_LOG_DEBUG("network", "WORLD: Received CMSG_STANDSTATECHANGE"); -- too many spam in log at lags/debug stop
    uint32 animstate;
    recvData >> animstate;

    GetPlayer()->SetStandState(animstate);
}

void WorldSession::HandleContactListOpcode(WorldPacket& recvData)
{
    uint32 unk;
    recvData >> unk;
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_CONTACT_LIST - Unk: %d", unk);
    GetPlayer()->GetSocial()->SendSocialList(GetPlayer());
}

void WorldSession::HandleAddFriendOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_ADD_FRIEND");

    std::string friendName = GetTrinityString(LANG_FRIEND_IGNORE_UNKNOWN);
    std::string friendNote;

    recvData >> friendName;

    recvData >> friendNote;

    if (!normalizePlayerName(friendName))
        return;

    TC_LOG_DEBUG("network", "WORLD: %s asked to add friend : '%s'",
        GetPlayer()->GetName().c_str(), friendName.c_str());

    if (!GetPlayer())
        return;

    ObjectGuid friendGuid;
    uint32 friendAccountId;
    uint32 team;

    if (auto entry = player::PlayerCache::Get(friendName))
    {
        friendGuid = entry.guid;
        friendAccountId = entry.account;
        team = Player::TeamForRace(entry.race);
    }
    else
    {
        sSocialMgr->SendFriendStatus(GetPlayer(), FRIEND_NOT_FOUND, ObjectGuid::Empty, false);
        return;
    }

    FriendsResult friendResult;


    if (!AccountMgr::IsPlayerAccount(AccountMgr::GetSecurity(friendAccountId, realm.Id.Realm)) && !HasPermission(rbac::RBAC_PERM_ALLOW_GM_FRIEND))
        friendResult = FRIEND_NOT_FOUND;
    else if (friendGuid == GetPlayer()->GetGUID())
        friendResult = FRIEND_SELF;
    else if (GetPlayer()->GetOTeam() != team && !HasPermission(rbac::RBAC_PERM_TWO_SIDE_ADD_FRIEND))
        friendResult = FRIEND_ENEMY;
    else if (GetPlayer()->GetSocial()->HasFriend(friendGuid))
        friendResult = FRIEND_ALREADY;
    else
    {
        Player* pFriend = ObjectAccessor::FindPlayer(friendGuid);
        if (pFriend && pFriend->IsVisibleGloballyFor(GetPlayer()) && !pFriend->IsSecretQueuingActive())
            friendResult = FRIEND_ADDED_ONLINE;
        else
            friendResult = FRIEND_ADDED_OFFLINE;

        if (!GetPlayer()->GetSocial()->AddToSocialList(friendGuid, false))
        {
            friendResult = FRIEND_LIST_FULL;
            TC_LOG_DEBUG("network", "WORLD: %s's friend list is full.", GetPlayer()->GetName().c_str());
        }
        else
        {
            GetPlayer()->GetSocial()->SetFriendNote(friendGuid, friendNote);
        }
    }

    sSocialMgr->SendFriendStatus(GetPlayer(), friendResult, friendGuid, false);

    TC_LOG_DEBUG("network", "WORLD: Sent (SMSG_FRIEND_STATUS)");
}

void WorldSession::HandleDelFriendOpcode(WorldPacket& recvData)
{
    ObjectGuid FriendGUID;

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_DEL_FRIEND");

    recvData >> FriendGUID;

    GetPlayer()->GetSocial()->RemoveFromSocialList(FriendGUID, false);

    sSocialMgr->SendFriendStatus(GetPlayer(), FRIEND_REMOVED, FriendGUID, false);

    TC_LOG_DEBUG("network", "WORLD: Sent motd (SMSG_FRIEND_STATUS)");
}

void WorldSession::HandleAddIgnoreOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_ADD_IGNORE");

    std::string ignoreName = GetTrinityString(LANG_FRIEND_IGNORE_UNKNOWN);

    recvData >> ignoreName;

    if (!normalizePlayerName(ignoreName))
        return;

    TC_LOG_DEBUG("network", "WORLD: %s asked to Ignore: '%s'",
        GetPlayer()->GetName().c_str(), ignoreName.c_str());

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUID_BY_NAME);

    stmt->setString(0, ignoreName);

    CharacterDatabase.AsyncQuery(stmt)
            .via(GetExecutor()).then([this](PreparedQueryResult result) { HandleAddIgnoreOpcodeCallBack(std::move(result)); });
}

void WorldSession::HandleAddIgnoreOpcodeCallBack(PreparedQueryResult result)
{
    if (!GetPlayer())
        return;

    ObjectGuid IgnoreGuid;
    FriendsResult ignoreResult;

    ignoreResult = FRIEND_IGNORE_NOT_FOUND;

    if (result)
    {
        IgnoreGuid = ObjectGuid(HighGuid::Player, (*result)[0].GetUInt32());

        if (IgnoreGuid)
        {
            if (IgnoreGuid == GetPlayer()->GetGUID())              //not add yourself
                ignoreResult = FRIEND_IGNORE_SELF;
            else if (GetPlayer()->GetSocial()->HasIgnore(IgnoreGuid))
                ignoreResult = FRIEND_IGNORE_ALREADY;
            else
            {
                ignoreResult = FRIEND_IGNORE_ADDED;

                // ignore list full
                if (!GetPlayer()->GetSocial()->AddToSocialList(IgnoreGuid, true))
                    ignoreResult = FRIEND_IGNORE_FULL;
            }

            // add complaint if someone gets added to ignore list
            if (Player* spammer = ObjectAccessor::FindPlayer(IgnoreGuid))
                spammer->GetComplaintMgr().RegisterComplaint(GetAccountId());
        }
    }

    sSocialMgr->SendFriendStatus(GetPlayer(), ignoreResult, IgnoreGuid, false);

    TC_LOG_DEBUG("network", "WORLD: Sent (SMSG_FRIEND_STATUS)");
}

void WorldSession::HandleDelIgnoreOpcode(WorldPacket& recvData)
{
    ObjectGuid IgnoreGUID;

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_DEL_IGNORE");

    recvData >> IgnoreGUID;

    GetPlayer()->GetSocial()->RemoveFromSocialList(IgnoreGUID, true);

    sSocialMgr->SendFriendStatus(GetPlayer(), FRIEND_IGNORE_REMOVED, IgnoreGUID, false);

    TC_LOG_DEBUG("network", "WORLD: Sent motd (SMSG_FRIEND_STATUS)");
}

void WorldSession::HandleSetContactNotesOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "CMSG_SET_CONTACT_NOTES");
    ObjectGuid guid;
    std::string note;
    recvData >> guid >> note;
    GetPlayer()->GetSocial()->SetFriendNote(guid, note);
}

void WorldSession::HandleBugOpcode(WorldPacket& recvData)
{
    uint32 suggestion, contentlen, typelen;
    std::string content, type;

    recvData >> suggestion >> contentlen >> content;

    recvData >> typelen >> type;

    if (suggestion == 0)
        TC_LOG_DEBUG("network", "WORLD: Received CMSG_BUG [Bug Report]");
    else
        TC_LOG_DEBUG("network", "WORLD: Received CMSG_BUG [Suggestion]");

    TC_LOG_DEBUG("network", "%s", type.c_str());
    TC_LOG_DEBUG("network", "%s", content.c_str());

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_BUG_REPORT);

    stmt->setString(0, type);
    stmt->setString(1, content);

    CharacterDatabase.Execute(stmt);
}

void WorldSession::HandleReclaimCorpseOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_RECLAIM_CORPSE");

    ObjectGuid guid;
    recvData >> guid;

    if (GetPlayer()->IsAlive())
    {
        TC_LOG_WARN("corpse", "Reclaim: player is alive");
        return;
    }

    // do not allow corpse reclaim in arena
    if (GetPlayer()->InArena())
    {
        TC_LOG_WARN("corpse", "Reclaim: player is in an arena");
        return;
    }

    // body not released yet
    if (!GetPlayer()->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
    {
        TC_LOG_WARN("corpse", "Reclaim: player is not a ghost");
        return;
    }

    Corpse* corpse = GetPlayer()->GetCorpse();

    if (!corpse)
    {
        TC_LOG_WARN("corpse", "Reclaim: can't find corpse");
        return;
    }

    // prevent resurrect before 30-sec delay after body release not finished
    if (time_t(corpse->GetGhostTime() + GetPlayer()->GetCorpseReclaimDelay(corpse->GetType() == CORPSE_RESURRECTABLE_PVP)) > time_t(time(NULL)))
    {
        TC_LOG_WARN("corpse", "Reclaim: corpse reclaim delay isn't over yet");
        return;
    }

    if (!corpse->IsWithinDistInMap(GetPlayer(), CORPSE_RECLAIM_RADIUS, true))
    {
        TC_LOG_WARN("corpse", "Reclaim: player is too far away");
        return;
    }

    // resurrect
    GetPlayer()->ResurrectPlayer(GetPlayer()->InBattleground() ? 1.0f : 0.5f);

    // spawn bones
    GetPlayer()->SpawnCorpseBones();
}

void WorldSession::HandleResurrectResponseOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_RESURRECT_RESPONSE");

    ObjectGuid guid;
    uint8 status;
    recvData >> guid;
    recvData >> status;

    if (GetPlayer()->IsAlive())
        return;

    if (status == 0)
    {
        GetPlayer()->clearResurrectRequestData();           // reject
        return;
    }

    if (GetPlayer()->IsValidGhoulResurrectRequest(guid))
    {
        GetPlayer()->GhoulResurrect();
        return;
    }

    if (!GetPlayer()->isResurrectRequestedBy(guid))
        return;

    GetPlayer()->ResurrectUsingRequestData();
}

void WorldSession::SendAreaTriggerMessage(const char* Text, ...)
{
    va_list ap;
    char szStr [1024];
    szStr[0] = '\0';

    va_start(ap, Text);
    vsnprintf(szStr, 1024, Text, ap);
    va_end(ap);

    uint32 length = strlen(szStr)+1;
    WorldPacket data(SMSG_AREA_TRIGGER_MESSAGE, 4+length);
    data << length;
    data << std::string_view(szStr, length);
    SendPacket(std::move(data));
}

void WorldSession::HandleAreaTriggerOpcode(WorldPacket& recvData)
{
    uint32 triggerId;
    recvData >> triggerId;

    TC_LOG_DEBUG("network", "CMSG_AREATRIGGER. Trigger ID: %u", triggerId);

    Player* player = GetPlayer();
    if (player->IsInFlight())
    {
        TC_LOG_DEBUG("network", "HandleAreaTriggerOpcode: Player '%s' (GUID: %u) in flight, ignore Area Trigger ID:%u",
            player->GetName().c_str(), player->GetGUID().GetCounter(), triggerId);
        return;
    }

    AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(triggerId);
    if (!atEntry)
    {
        TC_LOG_DEBUG("network", "HandleAreaTriggerOpcode: Player '%s' (GUID: %u) send unknown (by DBC) Area Trigger ID:%u",
            player->GetName().c_str(), player->GetGUID().GetCounter(), triggerId);
        return;
    }

    if (!player->IsInAreaTriggerRadius(atEntry))
    {
        TC_LOG_DEBUG("network", "HandleAreaTriggerOpcode: Player '%s' (GUID: %u) too far, ignore Area Trigger ID: %u",
            player->GetName().c_str(), player->GetGUID().GetCounter(), triggerId);
        return;
    }

    if (player->isDebugAreaTriggers)
        ChatHandler(player->GetSession()).PSendSysMessage(LANG_DEBUG_AREATRIGGER_REACHED, triggerId);

    if (sScriptMgr->OnAreaTrigger(player, atEntry))
        return;

    if (player->IsAlive())
        if (uint32 questId = sObjectMgr->GetQuestForAreaTrigger(triggerId))
            if (player->GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE)
                player->AreaExploredOrEventHappens(questId);

    // Check if player and inn faction match. 
    uint32 teamFactionMask = player->GetTeam() == ALLIANCE ? FACTION_MASK_ALLIANCE : FACTION_MASK_HORDE;
    if (teamFactionMask & sObjectMgr->GetFactionForTavernTrigger(triggerId)) // Also checks if triggerId is an inn trigger.
    {
        // set resting flag we are in the inn
        player->SetRestFlag(REST_FLAG_IN_TAVERN, atEntry->id);

        if (sWorld->IsFFAPvPRealm())
            player->RemoveByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP);

        return;
    }

    if (Battleground* bg = player->GetBattleground())
        if (bg->GetStatus() == STATUS_IN_PROGRESS)
            bg->HandleAreaTrigger(player, triggerId);

    if (OutdoorPvP* pvp = player->GetOutdoorPvP())
        if (pvp->HandleAreaTrigger(GetPlayer(), triggerId))
            return;

    AreaTrigger const* at = sObjectMgr->GetAreaTrigger(triggerId);
    if (!at)
        return;

    bool teleported = false;
    if (player->GetMapId() != at->target_mapId)
    {
        if (!sMapMgr->CanPlayerEnter(at->target_mapId, player, false))
            return;

        if (Group* group = player->GetGroup())
            if (group->isLFGGroup() && player->GetMap()->IsDungeon())
                teleported = player->TeleportToBGEntryPoint();
    }

    if (!teleported)
        player->TeleportTo(at->target_mapId, at->target_X, at->target_Y, at->target_Z, at->target_Orientation, TELE_TO_NOT_LEAVE_TRANSPORT);
}

void WorldSession::HandleUpdateAccountData(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_UPDATE_ACCOUNT_DATA");

    uint32 type, timestamp, decompressedSize;
    recvData >> type >> timestamp >> decompressedSize;

    TC_LOG_DEBUG("network", "UAD: type %u, time %u, decompressedSize %u", type, timestamp, decompressedSize);

    if (type > NUM_ACCOUNT_DATA_TYPES)
        return;

    if (decompressedSize == 0)                               // erase
    {
        SetAccountData(AccountDataType(type), 0, "");

        WorldPacket data(SMSG_UPDATE_ACCOUNT_DATA_COMPLETE, 4+4);
        data << uint32(type);
        data << uint32(0);
        SendPacket(std::move(data));

        return;
    }

    if (decompressedSize > 0xFFFF)
    {
        recvData.rfinish();                   // unnneded warning spam in this case
        TC_LOG_ERROR("network", "UAD: Account data packet too big, size %u", decompressedSize);
        return;
    }

    ByteBuffer dest;
    dest.resize(decompressedSize);

    uLongf realSize = decompressedSize;
    if (uncompress(dest.contents(), &realSize, recvData.contents() + recvData.rpos(), recvData.size() - recvData.rpos()) != Z_OK)
    {
        recvData.rfinish();                   // unnneded warning spam in this case
        TC_LOG_ERROR("network", "UAD: Failed to decompress account data");
        return;
    }

    recvData.rfinish();                       // uncompress read (recvData.size() - recvData.rpos())

    std::string adata;
    dest >> adata;

    SetAccountData(AccountDataType(type), timestamp, adata);

    WorldPacket data(SMSG_UPDATE_ACCOUNT_DATA_COMPLETE, 4+4);
    data << uint32(type);
    data << uint32(0);
    SendPacket(std::move(data));
}

void WorldSession::HandleRequestAccountData(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_REQUEST_ACCOUNT_DATA");

    uint32 type;
    recvData >> type;

    TC_LOG_DEBUG("network", "RAD: type %u", type);

    if (type >= NUM_ACCOUNT_DATA_TYPES)
        return;

    AccountData* adata = GetAccountData(AccountDataType(type));

    uint32 size = adata->Data.size();

    uLongf destSize = compressBound(size);

    ByteBuffer dest;
    dest.resize(destSize);

    if (size && compress(dest.contents(), &destSize, (uint8 const*)adata->Data.c_str(), size) != Z_OK)
    {
        TC_LOG_DEBUG("network", "RAD: Failed to compress account data");
        return;
    }

    dest.resize(destSize);

    WorldPacket data(SMSG_UPDATE_ACCOUNT_DATA, 8+4+4+4+destSize);
    data << uint64(GetPlayer() ? GetPlayer()->GetGUID() : ObjectGuid::Empty);
    data << uint32(type);                                   // type (0-7)
    data << uint32(adata->Time);                            // unix time
    data << uint32(size);                                   // decompressed length
    data.append(dest);                                      // compressed data
    SendPacket(std::move(data));
}

void WorldSession::HandleSetActionButtonOpcode(WorldPacket& recvData)
{
    uint8 button;
    uint32 packetData;
    recvData >> button >> packetData;
    TC_LOG_DEBUG("network", "CMSG_SET_ACTION_BUTTON Button: %u Data: %u", button, packetData);

    if (!packetData)
        GetPlayer()->removeActionButton(button);
    else
        GetPlayer()->addActionButton(button, ACTION_BUTTON_ACTION(packetData), ACTION_BUTTON_TYPE(packetData));
}

void WorldSession::HandleCompleteCinematic(WorldPacket& /*recvData*/)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_COMPLETE_CINEMATIC");
}

void WorldSession::HandleNextCinematicCamera(WorldPacket& /*recvData*/)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_NEXT_CINEMATIC_CAMERA");
}

void WorldSession::HandleMoveTimeSkippedOpcode(WorldPacket& recvData)
{
    /*  WorldSession::Update(getMSTime());*/
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_MOVE_TIME_SKIPPED");

    ObjectGuid guid;
    uint32 timeSkipped;
    recvData >> guid.ReadAsPacked();
    recvData >> timeSkipped;

    /* process anticheat check */
    if (GetPlayer()->GetGUID() == guid)
        GetPlayer()->GetAntiCheat()->SetTimeSkipped(timeSkipped);

    /*
        ObjectGuid guid;
        uint32 time_skipped;
        recvData >> guid;
        recvData >> time_skipped;
        TC_LOG_DEBUG("network", "WORLD: CMSG_MOVE_TIME_SKIPPED");

        //// @todo
        must be need use in Trinity
        We substract server Lags to move time (AntiLags)
        for exmaple
        GetPlayer()->ModifyLastMoveTime(-int32(time_skipped));
    */
}

void WorldSession::HandleFeatherFallAck(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_MOVE_FEATHER_FALL_ACK");

    // no used
    recvData.rfinish();                       // prevent warnings spam
}

void WorldSession::HandleMoveUnRootAck(WorldPacket& recvData)
{
    // no used
    recvData.rfinish();                       // prevent warnings spam
/*
    ObjectGuid guid;
    recvData >> guid;

    // now can skip not our packet
    if (GetPlayer()->GetGUID() != guid)
    {
        recvData.rfinish();                   // prevent warnings spam
        return;
    }

    TC_LOG_DEBUG("network", "WORLD: CMSG_FORCE_MOVE_UNROOT_ACK");

    recvData.read_skip<uint32>();                          // unk

    MovementInfo movementInfo;
    movementInfo.guid = guid;
    ReadMovementInfo(recvData, &movementInfo);
    recvData.read_skip<float>();                           // unk2
*/
}

void WorldSession::HandleMoveRootAck(WorldPacket& recvData)
{
    // no used
    recvData.rfinish();                       // prevent warnings spam
/*
    ObjectGuid guid;
    recvData >> guid;

    // now can skip not our packet
    if (GetPlayer()->GetGUID() != guid)
    {
        recvData.rfinish();                   // prevent warnings spam
        return;
    }

    TC_LOG_DEBUG("network", "WORLD: CMSG_FORCE_MOVE_ROOT_ACK");

    recvData.read_skip<uint32>();                          // unk

    MovementInfo movementInfo;
    ReadMovementInfo(recvData, &movementInfo);
*/
}

void WorldSession::HandleSetActionBarToggles(WorldPacket& recvData)
{
    uint8 actionBar;
    recvData >> actionBar;

    if (!GetPlayer())                                        // ignore until not logged (check needed because STATUS_AUTHED)
    {
        if (actionBar != 0)
            TC_LOG_ERROR("network", "WorldSession::HandleSetActionBarToggles in not logged state with value: %u, ignored", uint32(actionBar));
        return;
    }

    GetPlayer()->SetByteValue(PLAYER_FIELD_BYTES, 2, actionBar);
}

void WorldSession::HandlePlayedTime(WorldPacket& recvData)
{
    uint8 unk1;
    recvData >> unk1;                                      // 0 or 1 expected

    WorldPacket data(SMSG_PLAYED_TIME, 4 + 4 + 1);
    data << uint32(GetPlayer()->GetTotalPlayedTime());
    data << uint32(GetPlayer()->GetLevelPlayedTime());
    data << uint8(unk1);                                    // 0 - will not show in chat frame
    SendPacket(std::move(data));
}

void WorldSession::HandleInspectOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData >> guid;

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_INSPECT");

    if(!GetPlayer())
        return;

    Player *inspected = ObjectAccessor::GetPlayer(*GetPlayer(), guid);
    if (!inspected)
    {
        TC_LOG_DEBUG("network", "CMSG_INSPECT: No player found from %s", guid.ToString().c_str());
        return;
    }

    if (!GetPlayer()->IsWithinDistInMap(inspected, INSPECT_DISTANCE, false))
        return;

    if (GetPlayer()->IsValidAttackTarget(inspected))
        return;

    uint32 talent_points = 0x47;
    uint32 guid_size = inspected->GetPackGUID().size();
    WorldPacket data(SMSG_INSPECT_TALENT, guid_size+4+talent_points);
    data << inspected->GetPackGUID();

    if (GetPlayer()->CanBeGameMaster() || sWorld->getIntConfig(CONFIG_TALENTS_INSPECTING) + (GetPlayer()->GetTeamId() == inspected->GetTeamId()) > 1)
        inspected->BuildPlayerTalentsInfoData(&data);
    else
    {
        data << uint32(0);                                  // unspentTalentPoints
        data << uint8(0);                                   // talentGroupCount
        data << uint8(0);                                   // talentGroupIndex
    }

    inspected->BuildEnchantmentsInfoData(&data);
    SendPacket(std::move(data));
}

void WorldSession::HandleInspectHonorStatsOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData >> guid;

    if(!GetPlayer())
        return;

    Player *inspected = ObjectAccessor::GetPlayer(*GetPlayer(), guid);

    if (!inspected)
    {
        TC_LOG_DEBUG("network", "MSG_INSPECT_HONOR_STATS: No player found from %s", guid.ToString().c_str());
        return;
    }

    if (!GetPlayer()->IsWithinDistInMap(inspected, INSPECT_DISTANCE, false))
        return;

    if (GetPlayer()->IsValidAttackTarget(inspected))
        return;

    WorldPacket data(MSG_INSPECT_HONOR_STATS, 8+1+4*4);
    data << uint64(inspected->GetGUID());
    data << uint8(inspected->GetHonorPoints());
    data << uint32(inspected->GetUInt32Value(PLAYER_FIELD_KILLS));
    data << uint32(inspected->GetUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION));
    data << uint32(inspected->GetUInt32Value(PLAYER_FIELD_YESTERDAY_CONTRIBUTION));
    data << uint32(inspected->GetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS));
    SendPacket(std::move(data));
}

void WorldSession::HandleWorldTeleportOpcode(WorldPacket& recvData)
{
    uint32 time;
    uint32 mapid;
    float PositionX;
    float PositionY;
    float PositionZ;
    float Orientation;

    recvData >> time;                                      // time in m.sec.
    recvData >> mapid;
    recvData >> PositionX;
    recvData >> PositionY;
    recvData >> PositionZ;
    recvData >> Orientation;                               // o (3.141593 = 180 degrees)

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_WORLD_TELEPORT");

    if (GetPlayer()->IsInFlight())
    {
        TC_LOG_DEBUG("network", "Player '%s' (GUID: %u) in flight, ignore worldport command.",
            GetPlayer()->GetName().c_str(), GetPlayer()->GetGUID().GetCounter());
        return;
    }

    TC_LOG_DEBUG("network", "CMSG_WORLD_TELEPORT: Player = %s, Time = %u, map = %u, x = %f, y = %f, z = %f, o = %f",
        GetPlayer()->GetName().c_str(), time, mapid, PositionX, PositionY, PositionZ, Orientation);

    if (HasPermission(rbac::RBAC_PERM_OPCODE_WORLD_TELEPORT))
        GetPlayer()->TeleportTo(mapid, PositionX, PositionY, PositionZ, Orientation);
    else
        SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::HandleWhoisOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "Received opcode CMSG_WHOIS");
    std::string charname;
    recvData >> charname;

    if (!HasPermission(rbac::RBAC_PERM_OPCODE_WHOIS))
    {
        SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
        return;
    }

    if (charname.empty() || !normalizePlayerName (charname))
    {
        SendNotification(LANG_NEED_CHARACTER_NAME);
        return;
    }

    Player* player = sObjectAccessor->FindConnectedPlayerByName(charname);

    if (!player)
    {
        SendNotification(LANG_PLAYER_NOT_EXIST_OR_OFFLINE, charname.c_str());
        return;
    }

    uint32 accid = player->GetSession()->GetAccountId();

    PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_WHOIS);

    stmt->setUInt32(0, accid);

    PreparedQueryResult result = LoginDatabase.Query(stmt);

    if (!result)
    {
        SendNotification(LANG_ACCOUNT_FOR_PLAYER_NOT_FOUND, charname.c_str());
        return;
    }

    Field* fields = result->Fetch();
    std::string acc = fields[0].GetString();
    if (acc.empty())
        acc = "Unknown";
    std::string email = fields[1].GetString();
    if (email.empty())
        email = "Unknown";
    std::string lastip = fields[2].GetString();
    if (lastip.empty())
        lastip = "Unknown";

    std::string msg = charname + "'s " + "account is " + acc + ", e-mail: " + email + ", last ip: " + lastip;

    WorldPacket data(SMSG_WHOIS, msg.size()+1);
    data << msg;
    SendPacket(std::move(data));

    TC_LOG_DEBUG("network", "Received whois command from player %s for character %s",
        GetPlayer()->GetName().c_str(), charname.c_str());
}

void WorldSession::HandleComplainOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_COMPLAIN");

    uint8 spam_type;                                        // 0 - mail, 1 - chat
    ObjectGuid spammer_guid;
    uint32 unk1 = 0;
    uint32 unk2 = 0;
    uint32 unk3 = 0;
    uint32 unk4 = 0;
    std::string description = "";
    recvData >> spam_type;                                 // unk 0x01 const, may be spam type (mail/chat)
    recvData >> spammer_guid;                              // player guid
    switch (spam_type)
    {
        case 0:
            recvData >> unk1;                              // const 0
            recvData >> unk2;                              // probably mail id
            recvData >> unk3;                              // const 0
            break;
        case 1:
            recvData >> unk1;                              // probably language
            recvData >> unk2;                              // message type?
            recvData >> unk3;                              // probably channel id
            recvData >> unk4;                              // time
            recvData >> description;                       // spam description string (messagetype, channel name, player name, message)
            break;
    }

    // NOTE: all chat messages from this spammer automatically ignored by spam reporter until logout in case chat spam.
    // if it's mail spam - ALL mails from this spammer automatically removed by client

    // Complaint Received message
    WorldPacket data(SMSG_COMPLAIN_RESULT, 1);
    data << uint8(0);
    SendPacket(std::move(data));

    TC_LOG_DEBUG("network", "REPORT SPAM: type %u, %s, unk1 %u, unk2 %u, unk3 %u, unk4 %u, message %s", spam_type, spammer_guid.ToString().c_str(), unk1, unk2, unk3, unk4, description.c_str());

    if (Player* spammer = ObjectAccessor::FindPlayer(spammer_guid))
        spammer->GetComplaintMgr().RegisterComplaint(GetAccountId());
}

void WorldSession::HandleRealmSplitOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "CMSG_REALM_SPLIT");

    uint32 unk;
    std::string split_date = "01/01/01";
    recvData >> unk;

    WorldPacket data(SMSG_REALM_SPLIT, 4+4+split_date.size()+1);
    data << unk;
    data << uint32(0x00000000);                             // realm split state
    // split states:
    // 0x0 realm normal
    // 0x1 realm split
    // 0x2 realm split pending
    data << split_date;
    SendPacket(std::move(data));
    //TC_LOG_DEBUG("response sent %u", unk);
}

void WorldSession::HandleFarSightOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_FAR_SIGHT");

    bool apply;
    recvData >> apply;

    if (apply)
    {
        TC_LOG_DEBUG("network", "Added FarSight %s to player %u", GetPlayer()->GetGuidValue(PLAYER_FARSIGHT).ToString().c_str(), GetPlayer()->GetGUID().GetCounter());
        if (WorldObject* target = GetPlayer()->GetViewpoint())
            GetPlayer()->SetSeer(target);
        else
            TC_LOG_ERROR("network", "Player %s (%s) requests non-existing seer %s", GetPlayer()->GetName().c_str(), GetPlayer()->GetGUID().ToString().c_str(), GetPlayer()->GetGuidValue(PLAYER_FARSIGHT).ToString().c_str());
    }
    else
    {
        TC_LOG_DEBUG("network", "Player %u set vision to self", GetPlayer()->GetGUID().GetCounter());
        GetPlayer()->SetSeer(GetPlayer());
    }

    GetPlayer()->UpdateVisibilityForPlayer();
}

void WorldSession::HandleSetTitleOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "CMSG_SET_TITLE");

    int32 title;
    recvData >> title;

    // -1 at none
    if (title > 0 && title < MAX_TITLE_INDEX)
    {
       if (!GetPlayer()->HasTitle(title))
            return;
    }
    else
        title = 0;

    GetPlayer()->SetUInt32Value(PLAYER_CHOSEN_TITLE, title);
}

void WorldSession::HandleTimeSyncResp(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "CMSG_TIME_SYNC_RESP");

    uint32 counter, clientTicks;
    recvData >> counter >> clientTicks;

    if (counter != GetPlayer()->m_timeSyncCounter - 1)
        TC_LOG_DEBUG("network", "Wrong time sync counter from player %s (cheater?)", GetPlayer()->GetName().c_str());

    TC_LOG_DEBUG("network", "Time sync received: counter %u, client ticks %u, time since last sync %u", counter, clientTicks, clientTicks - GetPlayer()->m_timeSyncClient);

    uint32 ourTicks = clientTicks + (getMSTime() - GetPlayer()->m_timeSyncServer);

    // diff should be small
    TC_LOG_DEBUG("network", "Our ticks: %u, diff %u, latency %u", ourTicks, ourTicks - clientTicks, GetLatency());

    GetPlayer()->m_timeSyncClient = clientTicks;
}

void WorldSession::HandleResetInstancesOpcode(WorldPacket& /*recvData*/)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_RESET_INSTANCES");

    if (auto group = GetPlayer()->GetGroup())
    {
        if (group->IsLeader(GetPlayer()->GetGUID()))
            group->ResetInstances(INSTANCE_RESET_ALL, false, GetPlayer());
    }
    else
        GetPlayer()->ResetInstances(INSTANCE_RESET_ALL, false);
}

void WorldSession::HandleSetDungeonDifficultyOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "MSG_SET_DUNGEON_DIFFICULTY");

    uint32 mode;
    recvData >> mode;

    if (mode >= MAX_DUNGEON_DIFFICULTY)
    {
        TC_LOG_DEBUG("network", "WorldSession::HandleSetDungeonDifficultyOpcode: player %d sent an invalid instance mode %d!", GetPlayer()->GetGUID().GetCounter(), mode);
        return;
    }

    if (Difficulty(mode) == GetPlayer()->GetDungeonDifficulty())
        return;

    // cannot reset while in an instance
    Map* map = GetPlayer()->FindMap();
    if (map && map->IsDungeon())
    {
        TC_LOG_DEBUG("network", "WorldSession::HandleSetDungeonDifficultyOpcode: player (Name: %s, GUID: %u) tried to reset the instance while player is inside!",
            GetPlayer()->GetName().c_str(), GetPlayer()->GetGUID().GetCounter());
        return;
    }

    auto group = GetPlayer()->GetGroup();
    if (group)
    {
        if (group->IsLeader(GetPlayer()->GetGUID()))
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player* groupGuy = itr->GetSource();
                if (!groupGuy)
                    continue;

                if (!groupGuy->IsInMap(groupGuy))
                    return;

                if (groupGuy->GetMap()->IsNonRaidDungeon())
                {
                    TC_LOG_DEBUG("network", "WorldSession::HandleSetDungeonDifficultyOpcode: player %d tried to reset the instance while group member (Name: %s, GUID: %u) is inside!",
                        GetPlayer()->GetGUID().GetCounter(), groupGuy->GetName().c_str(), groupGuy->GetGUID().GetCounter());
                    return;
                }
            }
            // the difficulty is set even if the instances can't be reset
            //GetPlayer()->SendDungeonDifficulty(true);
            group->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, false, GetPlayer());
            group->SetDungeonDifficulty(Difficulty(mode));
        }
    }
    else
    {
        GetPlayer()->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, false);
        GetPlayer()->SetDungeonDifficulty(Difficulty(mode));
    }
}

void WorldSession::HandleSetRaidDifficultyOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "MSG_SET_RAID_DIFFICULTY");

    uint32 mode;
    recvData >> mode;

    if (mode >= MAX_RAID_DIFFICULTY)
    {
        TC_LOG_ERROR("network", "WorldSession::HandleSetRaidDifficultyOpcode: player %d sent an invalid instance mode %d!", GetPlayer()->GetGUID().GetCounter(), mode);
        return;
    }

    Map* map = GetPlayer()->FindMap();
    auto group = GetPlayer()->GetGroup();

    if (Difficulty(mode) == GetPlayer()->GetRaidDifficulty())
        return;

    // cannot reset while in an instance
    if (map && map->IsDungeon())
    {
        TC_LOG_DEBUG("network", "WorldSession::HandleSetRaidDifficultyOpcode: player %d tried to reset the instance while inside!", GetPlayer()->GetGUID().GetCounter());
        return;
    }

    if (group)
    {
        if (group->IsLeader(GetPlayer()->GetGUID()))
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player* groupGuy = itr->GetSource();
                if (!groupGuy)
                    continue;

                if (!groupGuy->IsInMap(groupGuy))
                    return;

                if (groupGuy->GetMap()->IsRaid())
                {
                    TC_LOG_DEBUG("network", "WorldSession::HandleSetRaidDifficultyOpcode: player %d tried to reset the instance while inside!", GetPlayer()->GetGUID().GetCounter());
                    return;
                }
            }
            // the difficulty is set even if the instances can't be reset
            //GetPlayer()->SendDungeonDifficulty(true);
            group->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, true, GetPlayer());
            group->SetRaidDifficulty(Difficulty(mode));
        }
    }
    else
    {
        GetPlayer()->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, true);
        GetPlayer()->SetRaidDifficulty(Difficulty(mode));
    }
}

void WorldSession::HandleChangePlayerDifficulty(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "Received CMSG_CHANGEPLAYER_DIFFICULTY");

    uint32 difficulty;

    recvData >> difficulty;

    if (difficulty >= MAX_RAID_DIFFICULTY)
    {
        TC_LOG_ERROR("network", "WorldSession::HandleChangePlayerDifficulty: player %d sent an invalid instance mode %d!", GetPlayer()->GetGUID().GetCounter(), difficulty);
        return;
    }

    Map* map = GetPlayer()->FindMap();
    auto group = GetPlayer()->GetGroup();

    if (!map || !map->GetEntry()->IsDynamicDifficultyMap())
    {
        TC_LOG_DEBUG("network", "WorldSession::HandleChangePlayerDifficulty: player %d tried to change difficulty in wrong map!", GetPlayer()->GetGUID().GetCounter());
        return;
    }

    auto instance = map->ToInstanceMap();

    if (!instance)
        return;

    switch (instance->GetMaxPlayers())
    {
        default:
        case 10:
            difficulty = (difficulty ? RAID_DIFFICULTY_10MAN_HEROIC : RAID_DIFFICULTY_10MAN_NORMAL);
            break;
        case 25:
            difficulty = (difficulty ? RAID_DIFFICULTY_25MAN_HEROIC : RAID_DIFFICULTY_25MAN_NORMAL);
            break;
    }

    if (Difficulty(difficulty) == GetPlayer()->GetRaidDifficulty() || !group)
        return;

    if (group->IsLeader(GetPlayer()->GetGUID()))
    {
        if (!GetPlayer()->Satisfy(sObjectMgr->GetAccessRequirement(map->GetId(), Difficulty(difficulty)), map->GetId(), true))
            return;

        for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* groupGuy = itr->GetSource();
            if (!groupGuy)
                continue;

            if (!groupGuy->IsInMap(groupGuy))
            {
                _player->SendDynamicDifficultyResult(ERR_DIFFICULTY_CHANGE_PLAYER_BUSY);
                return;
            }

            if (groupGuy->GetMap()->IsRaid())
            {
                if (!groupGuy->GetMap()->GetEntry()->IsDynamicDifficultyMap())
                {
                    GetPlayer()->SendResetInstanceFailed(0, groupGuy->GetMap()->GetId());
                    TC_LOG_DEBUG("network", "WorldSession::HandleSetRaidDifficultyOpcode: player %d tried to reset the instance while inside!", GetPlayer()->GetGUID().GetCounter());
                    return;
                }

                if (auto instance = groupGuy->GetMap()->ToInstanceMap())
                    if (instance->IsEncounterInProgress())
                    {
                        GetPlayer()->SendDynamicDifficultyResult(ERR_DIFFICULTY_CHANGE_ENCOUNTER);
                        return;
                    }

                if (groupGuy->IsInCombat())
                {
                    GetPlayer()->SendDynamicDifficultyResult(ERR_DIFFICULTY_CHANGE_COMBAT);
                    return;
                }
            }
        }

        const uint32 DIFFICULTY_CHANGE_COOLDOWN = 1 * MINUTE;

        auto diff = time(nullptr) - group->GetLastRaidDifficultyChange();
        if (diff < DIFFICULTY_CHANGE_COOLDOWN)
        {
            GetPlayer()->SendDynamicDifficultyResult(ERR_DIFFICULTY_CHANGE_COOLDOWN_S, DIFFICULTY_CHANGE_COOLDOWN - diff);
            return;
        }

        group->ResetLastRaidDifficultyChange();
        group->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, true, GetPlayer());
        group->SetRaidDifficulty(Difficulty(difficulty));

        for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* groupGuy = itr->GetSource();
            if (!groupGuy || !groupGuy->IsInMap(groupGuy) || !groupGuy->GetMap()->IsRaid())
                continue;

            groupGuy->SendDynamicDifficultyResult(ERR_DIFFICULTY_CHANGE_SUCCESS, DIFFICULTY_CHANGE_COOLDOWN);
            if (!groupGuy->TeleportTo(*groupGuy, TELE_FORCE_TELEPORT_FAR))
                groupGuy->RepopAtGraveyard();
        }

        for (Player& player : map->GetPlayers())
            if (!player.IsBeingTeleportedFar())
                player.RepopAtGraveyard();

        instance->ForceUnload();
    }
}

void WorldSession::HandleCancelMountAuraOpcode(WorldPacket& /*recvData*/)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_CANCEL_MOUNT_AURA");

    //If player is not mounted, so go out :)
    if (!GetPlayer()->IsMounted())                              // not blizz like; no any messages on blizz
    {
        ChatHandler(this).SendSysMessage(LANG_CHAR_NON_MOUNTED);
        return;
    }

    if (GetPlayer()->IsInFlight())                               // not blizz like; no any messages on blizz
    {
        ChatHandler(this).SendSysMessage(LANG_YOU_IN_FLIGHT);
        return;
    }

    GetPlayer()->RemoveAurasByType(SPELL_AURA_MOUNTED); // Calls Dismount()
}

void WorldSession::HandleMoveSetCanFlyAckOpcode(WorldPacket& recvData)
{
    // fly mode on/off
    TC_LOG_DEBUG("network", "WORLD: CMSG_MOVE_SET_CAN_FLY_ACK");

    ObjectGuid guid;                                            // guid - unused
    recvData >> guid.ReadAsPacked();

    recvData.read_skip<uint32>();                          // unk

    MovementInfo movementInfo;
    movementInfo.guid = guid;
    ReadMovementInfo(recvData, &movementInfo);

    recvData.read_skip<float>();                           // unk2

    GetPlayer()->m_mover->m_movementInfo.flags = movementInfo.GetMovementFlags();
}

void WorldSession::HandleRequestPetInfoOpcode(WorldPacket& /*recvData */)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_REQUEST_PET_INFO");

    GetPlayer()->PetSpellInitialize();
    GetPlayer()->PossessSpellInitialize();
}

void WorldSession::HandleSetTaxiBenchmarkOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_SET_TAXI_BENCHMARK_MODE");

    uint8 mode;
    recvData >> mode;

    mode ? GetPlayer()->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_TAXI_BENCHMARK) : GetPlayer()->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_TAXI_BENCHMARK);

    TC_LOG_DEBUG("network", "Client used \"/timetest %d\" command", mode);
}

void WorldSession::HandleQueryInspectAchievements(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData >> guid.ReadAsPacked();

    if(!GetPlayer())
        return;

    TC_LOG_DEBUG("network", "CMSG_QUERY_INSPECT_ACHIEVEMENTS [%s] Inspected Player [%s]", GetPlayer()->GetGUID().ToString().c_str(), guid.ToString().c_str());
    Player* player = ObjectAccessor::GetPlayer(*GetPlayer(), guid);
    if (!player)
        return;

    if (!GetPlayer()->IsWithinDistInMap(player, INSPECT_DISTANCE, false))
        return;

    if (GetPlayer()->IsValidAttackTarget(player))
        return;

    player->SendRespondInspectAchievements(GetPlayer());
}

void WorldSession::HandleWorldStateUITimerUpdate(WorldPacket& /*recvData*/)
{
    // empty opcode
    TC_LOG_DEBUG("network", "WORLD: CMSG_WORLD_STATE_UI_TIMER_UPDATE");

    WorldPacket data(SMSG_WORLD_STATE_UI_TIMER_UPDATE, 4);
    data << uint32(time(NULL));
    SendPacket(std::move(data));
}

void WorldSession::HandleReadyForAccountDataTimes(WorldPacket& /*recvData*/)
{
    // empty opcode
    TC_LOG_DEBUG("network", "WORLD: CMSG_READY_FOR_ACCOUNT_DATA_TIMES");

    SendAccountDataTimes(GLOBAL_CACHE_MASK);
}

void WorldSession::SendSetPhaseShift(uint32 PhaseShift)
{
    WorldPacket data(SMSG_SET_PHASE_SHIFT, 4);
    data << uint32(PhaseShift);
    SendPacket(std::move(data));
}
// Battlefield and Battleground
void WorldSession::HandleAreaSpiritHealerQueryOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_AREA_SPIRIT_HEALER_QUERY");

    ObjectGuid guid;
    recvData >> guid;

    if (Battleground* bg = GetPlayer()->GetBattleground())
        bg->SendAreaSpiritHealerQueryOpcode(GetPlayer(), guid);
    else if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(GetPlayer()->GetZoneId()))
        bf->SendAreaSpiritHealerQueryOpcode(GetPlayer(), guid);
}

void WorldSession::HandleAreaSpiritHealerQueueOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_AREA_SPIRIT_HEALER_QUEUE");

    if (Battleground* bg = GetPlayer()->GetBattleground())
        bg->AddPlayerToResurrectQueue(GetPlayer()->GetGUID());
    else if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(GetPlayer()->GetZoneId()))
        bf->AddPlayerToResurrectQueue(GetPlayer()->GetGUID());
}

void WorldSession::HandleHearthAndResurrect(WorldPacket& /*recvData*/)
{
    if (GetPlayer()->IsInFlight())
        return;

    if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(GetPlayer()->GetZoneId()))
    {
        if (bf->IsWarTime())
        {
            bf->AskToLeave(bf, GetPlayer());
            return;
        }
    }

    AreaTableEntry const* atEntry = sAreaTableStore.LookupEntry(GetPlayer()->GetAreaId());
    if (!atEntry || !(atEntry->flags & AREA_FLAG_WINTERGRASP_2))
        return;
    else if (atEntry && !(atEntry->flags & AREA_FLAG_WINTERGRASP))
    {
        // Don't create a corpse while porting out of wintergrasp, fixes dead ghost bug
        GetPlayer()->BuildPlayerRepop();
        GetPlayer()->ResurrectPlayer(1.0f);
    }

    GetPlayer()->TeleportTo(GetPlayer()->m_homebindMapId, GetPlayer()->m_homebindX, GetPlayer()->m_homebindY, GetPlayer()->m_homebindZ, GetPlayer()->GetOrientation());
}

void WorldSession::HandleInstanceLockResponse(WorldPacket& recvPacket)
{
    uint8 accept;
    recvPacket >> accept;

    if (!GetPlayer()->HasPendingBind())
    {
        TC_LOG_INFO("network", "InstanceLockResponse: Player %s (guid %u) tried to bind himself/teleport to graveyard without a pending bind!",
            GetPlayer()->GetName().c_str(), GetPlayer()->GetGUID().GetCounter());
        return;
    }

    if (accept)
        GetPlayer()->BindToInstance();
    else
        GetPlayer()->RepopAtGraveyard();

    GetPlayer()->SetPendingBind(0, 0);
}

void WorldSession::HandleUpdateMissileTrajectory(WorldPacket& recvPacket)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_UPDATE_MISSILE_TRAJECTORY");

    ObjectGuid guid;
    uint32 spellId;
    float elevation, speed;
    float curX, curY, curZ;
    float targetX, targetY, targetZ;
    uint8 moveStop;

    recvPacket >> guid >> spellId >> elevation >> speed;
    recvPacket >> curX >> curY >> curZ;
    recvPacket >> targetX >> targetY >> targetZ;
    recvPacket >> moveStop;

    Unit* caster = ObjectAccessor::GetUnit(*GetPlayer(), guid);
    Spell* spell = caster ? caster->GetCurrentSpell(CURRENT_GENERIC_SPELL) : NULL;
    if (!spell || spell->m_spellInfo->Id != spellId || !spell->m_targets.HasDst() || !spell->m_targets.HasSrc())
    {
        recvPacket.rfinish();
        return;
    }

    Position pos = *spell->m_targets.GetSrcPos();
    pos.Relocate(curX, curY, curZ);
    spell->m_targets.ModSrc(pos);

    pos = *spell->m_targets.GetDstPos();
    pos.Relocate(targetX, targetY, targetZ);
    spell->m_targets.ModDst(pos);

    spell->m_targets.SetElevation(elevation);
    spell->m_targets.SetSpeed(speed);

    if (moveStop)
    {
        uint32 opcode;
        recvPacket >> opcode;
        recvPacket.SetOpcode(opcode);
        HandleMovementOpcodes(recvPacket);
    }
}
