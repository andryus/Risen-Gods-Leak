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

#include "AccountMgr.h"
#include "ArenaTeam.h"
#include "ArenaTeamMgr.h"
#include "Battleground.h"
#include "CalendarMgr.h"
#include "Chat.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Language.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Pet.h"
#include "PlayerDump.h"
#include "Player.h"
#include "PlayerData.h"
#include "ReputationMgr.h"
#include "GitRevision.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "SocialMgr.h"
#include "Util.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "DBCStores.h"
#include "Entities/Player/PlayerCache.hpp"
#include "RG/Logging/LogManager.hpp"
#include "Monitoring/WorldMonitoring.h"


class LoginQueryHolder : public SQLQueryHolder
{
    private:
        uint32 m_accountId;
        ObjectGuid m_guid;
    public:
        LoginQueryHolder(uint32 accountId, ObjectGuid guid)
            : m_accountId(accountId), m_guid(guid) { }
        ObjectGuid GetGuid() const { return m_guid; }
        uint32 GetAccountId() const { return m_accountId; }
        bool Initialize();
};

bool LoginQueryHolder::Initialize()
{
    SetSize(MAX_PLAYER_LOGIN_QUERY);

    bool res = true;
    uint32 lowGuid = m_guid.GetCounter();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_FROM, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GROUP_MEMBER);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GROUP, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INSTANCE);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BOUND_INSTANCES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURAS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AURAS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELL);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_DAILY);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_DAILY_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_WEEKLY);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_WEEKLY_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_MONTHLY);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MONTHLY_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_SEASONAL);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SEASONAL_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_REPUTATION);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_REPUTATION, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INVENTORY);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_INVENTORY, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ACTIONS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACTIONS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SOCIALLIST);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SOCIAL_LIST, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_HOMEBIND);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_HOME_BIND, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELLCOOLDOWNS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELL_COOLDOWNS, stmt);

    if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_DECLINEDNAMES);
        stmt->setUInt32(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_DECLINED_NAMES, stmt);
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUILD_MEMBER);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GUILD, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ARENAINFO);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ARENA_INFO, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ACHIEVEMENTS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACHIEVEMENTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_CRITERIAPROGRESS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CRITERIA_PROGRESS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_EQUIPMENTSETS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_EQUIPMENT_SETS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_BGDATA);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BG_DATA, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GLYPHS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GLYPHS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_TALENTS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_TALENTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_ACCOUNT_DATA);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACCOUNT_DATA, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SKILLS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SKILLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_RANDOMBG);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_RANDOM_BG, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_BANNED);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BANNED, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUSREW);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_REW, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_INSTANCELOCKTIMES);
    stmt->setUInt32(0, m_accountId);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_INSTANCE_LOCK_TIMES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_COMPLAINTS);
    stmt->setUInt32(0, m_accountId);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACCOUNT_COMPLAINTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PVP_RANK_BY_GUID);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_PVP_RANK, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CUSTOM_FLAGS);
    stmt->setInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CUSTOM_FLAGS, stmt);

    return res;
}

void WorldSession::HandleCharEnum(SharedPreparedQueryResult result)
{
    WorldPacket data(SMSG_CHAR_ENUM, 100);                  // we guess size

    uint8 num = 0;

    data << num;

    _legitCharacters.clear();
    if (result)
    {
        do
        {
            ObjectGuid guid(HighGuid::Player, (*result)[0].GetUInt32());

            if (_legitCharacters.find(guid) != _legitCharacters.end() || num >= 10)
            {
                TC_LOG_ERROR("entities.player.loading", "%s has duplicated enum entry. Skipped.", guid.ToString().c_str());
                continue;
            }

            if (Player::BuildEnumData(result, &data))
            {
                // Do not allow banned characters to log in
                if (!(*result)[20].GetUInt32())
                    _legitCharacters.insert(guid);

                ++num;
            }
        }
        while (result->NextRow());
    }

    data.put<uint8>(0, num);

    SendPacket(std::move(data));
}

void WorldSession::HandleCharEnumOpcode(WorldPacket & /*recvData*/)
{
    // remove expired bans
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_EXPIRED_BANS);
    CharacterDatabase.Execute(stmt);

    /// get all the data necessary for loading all characters (along with their pets) on the account

    if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ENUM_DECLINED_NAME);
    else
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ENUM);

    stmt->setUInt8(0, PET_SAVE_AS_CURRENT);
    stmt->setUInt32(1, GetAccountId());

    CharacterDatabase.AsyncQuery(stmt)
            .via(GetExecutor()).then([this](PreparedQueryResult result) { HandleCharEnum(std::move(result)); });
}

void WorldSession::HandleCharCreateOpcode(WorldPacket& recvData)
{
    std::string name;
    uint8 race_, class_;

    recvData >> name;

    recvData >> race_;
    recvData >> class_;

    // extract other data required for player creating
    uint8 gender, skin, face, hairStyle, hairColor, facialHair, outfitId;
    recvData >> gender >> skin >> face;
    recvData >> hairStyle >> hairColor >> facialHair >> outfitId;

    WorldPacket data(SMSG_CHAR_CREATE, 1);                  // returned with diff.values in all cases

    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_TEAMMASK))
    {
        if (uint32 mask = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED))
        {
            bool disabled = false;

            uint32 team = Player::TeamForRace(race_);
            switch (team)
            {
                case ALLIANCE:
                    disabled = (mask & (1 << 0)) != 0;
                    break;
                case HORDE:
                    disabled = (mask & (1 << 1)) != 0;
                    break;
            }

            if (disabled)
            {
                data << uint8(CHAR_CREATE_DISABLED);
                SendPacket(std::move(data));
                return;
            }
        }
    }

    ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(class_);
    if (!classEntry)
    {
        data << uint8(CHAR_CREATE_FAILED);
        SendPacket(std::move(data));
        TC_LOG_ERROR("network", "Class (%u) not found in DBC while creating new char for account (ID: %u): wrong DBC files or cheater?", class_, GetAccountId());
        return;
    }

    ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(race_);
    if (!raceEntry)
    {
        data << uint8(CHAR_CREATE_FAILED);
        SendPacket(std::move(data));
        TC_LOG_ERROR("network", "Race (%u) not found in DBC while creating new char for account (ID: %u): wrong DBC files or cheater?", race_, GetAccountId());
        return;
    }

    // prevent character creating Expansion race without Expansion account
    if (raceEntry->expansion > Expansion())
    {
        data << uint8(CHAR_CREATE_EXPANSION);
        TC_LOG_ERROR("network", "Expansion %u account:[%d] tried to Create character with expansion %u race (%u)", Expansion(), GetAccountId(), raceEntry->expansion, race_);
        SendPacket(std::move(data));
        return;
    }

    // prevent character creating Expansion class without Expansion account
    if (classEntry->expansion > Expansion())
    {
        data << uint8(CHAR_CREATE_EXPANSION_CLASS);
        TC_LOG_ERROR("network", "Expansion %u account:[%d] tried to Create character with expansion %u class (%u)", Expansion(), GetAccountId(), classEntry->expansion, class_);
        SendPacket(std::move(data));
        return;
    }

    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RACEMASK))
    {
        uint32 raceMaskDisabled = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_RACEMASK);
        if ((1 << (race_ - 1)) & raceMaskDisabled)
        {
            data << uint8(CHAR_CREATE_DISABLED);
            SendPacket(std::move(data));
            return;
        }
    }

    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_CLASSMASK))
    {
        uint32 classMaskDisabled = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_CLASSMASK);
        if ((1 << (class_ - 1)) & classMaskDisabled)
        {
            data << uint8(CHAR_CREATE_DISABLED);
            SendPacket(std::move(data));
            return;
        }
    }

    // prevent character creating with invalid name
    if (!normalizePlayerName(name))
    {
        data << uint8(CHAR_NAME_NO_NAME);
        SendPacket(std::move(data));
        TC_LOG_ERROR("network", "Account:[%d] but tried to Create character with empty [name] ", GetAccountId());
        return;
    }

    // check name limitations
    uint8 res = ObjectMgr::CheckPlayerName(name, true);
    if (res != CHAR_NAME_SUCCESS)
    {
        data << uint8(res);
        SendPacket(std::move(data));
        return;
    }

    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RESERVEDNAME) && sObjectMgr->IsReservedName(name))
    {
        data << uint8(CHAR_NAME_RESERVED);
        SendPacket(std::move(data));
        return;
    }

    if (class_ == CLASS_DEATH_KNIGHT && !HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_HEROIC_CHARACTER))
    {
        // speedup check for heroic class disabled case
        uint32 heroic_free_slots = sWorld->getIntConfig(CONFIG_HEROIC_CHARACTERS_PER_REALM);
        if (heroic_free_slots == 0)
        {
            data << uint8(CHAR_CREATE_UNIQUE_CLASS_LIMIT);
            SendPacket(std::move(data));
            return;
        }

        // speedup check for heroic class disabled case
        uint32 req_level_for_heroic = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_MIN_LEVEL_FOR_HEROIC_CHARACTER);
        if (req_level_for_heroic > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        {
            data << uint8(CHAR_CREATE_LEVEL_REQUIREMENT);
            SendPacket(std::move(data));
            return;
        }
    }

    auto createInfo = std::make_shared<CharacterCreateInfo>(name, race_, class_, gender, skin, face, hairStyle, hairColor, facialHair, outfitId, recvData);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHECK_NAME);
    stmt->setString(0, name);
    CharacterDatabase.AsyncQuery(stmt)
            .via(GetExecutor())
            .then([this, createInfo](PreparedQueryResult result)
            {
                if (result)
                {
                    WorldPacket data(SMSG_CHAR_CREATE, 1);
                    data << uint8(CHAR_CREATE_NAME_IN_USE);
                    SendPacket(std::move(data));
                    throw folly::FutureCancellation();
                }

                PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_SUM_REALM_CHARACTERS);
                stmt->setUInt32(0, GetAccountId());
                return LoginDatabase.AsyncQuery(stmt);
            })
            .then([this, createInfo](PreparedQueryResult result)
            {
                uint64 acctCharCount = 0;
                if (result)
                {
                    Field* fields = result->Fetch();
                    acctCharCount = uint64(fields[0].GetDouble());
                }

                if (acctCharCount >= sWorld->getIntConfig(CONFIG_CHARACTERS_PER_ACCOUNT))
                {
                    WorldPacket data(SMSG_CHAR_CREATE, 1);
                    data << uint8(CHAR_CREATE_ACCOUNT_LIMIT);
                    SendPacket(std::move(data));
                    throw folly::FutureCancellation();
                }

                PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_MAXLEVEL);
                stmt->setUInt32(0, GetAccountId());
                return LoginDatabase.AsyncQuery(stmt);
            })
            .then([this, createInfo](PreparedQueryResult result)
            {
                if (result)
                {
                    Field *fields = result->Fetch();
                    createInfo->MaxLevel = fields[0].GetUInt8();
                }

                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAXLEVEL);
                stmt->setUInt32(0, GetAccountId());
                return CharacterDatabase.AsyncQuery(stmt);
            })
            .then([this, createInfo](PreparedQueryResult result)
            {
                if (result)
                {
                    Field *fields = result->Fetch();
                    createInfo->MaxRealmLevel = fields[0].GetUInt8();
                }

                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_SUM_CHARS);
                stmt->setUInt32(0, GetAccountId());
                return CharacterDatabase.AsyncQuery(stmt);
            })
            .then([this, createInfo](PreparedQueryResult result)
            {
                if (result)
                {
                    Field* fields = result->Fetch();
                    createInfo->CharCount = uint8(fields[0].GetUInt64()); // SQL's COUNT() returns uint64 but it will always be less than uint8.Max

                    if (createInfo->CharCount >= sWorld->getIntConfig(CONFIG_CHARACTERS_PER_REALM))
                    {
                        WorldPacket data(SMSG_CHAR_CREATE, 1);
                        data << uint8(CHAR_CREATE_SERVER_LIMIT);
                        SendPacket(std::move(data));
                        throw folly::FutureCancellation();
                    }
                }

                bool allowTwoSideAccounts = !sWorld->IsPvPRealm() || HasPermission(rbac::RBAC_PERM_TWO_SIDE_CHARACTER_CREATION);
                uint32 skipCinematics = sWorld->getIntConfig(CONFIG_SKIP_CINEMATICS);

                if (!allowTwoSideAccounts || skipCinematics == 1 || createInfo->Class == CLASS_DEATH_KNIGHT)
                {
                    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_CREATE_INFO);
                    stmt->setUInt32(0, GetAccountId());
                    stmt->setUInt32(1, (skipCinematics == 1 || createInfo->Class == CLASS_DEATH_KNIGHT) ? 10 : 1);
                    return CharacterDatabase.AsyncQuery(stmt);
                }

                return folly::makeFuture(PreparedQueryResult(nullptr));
            })
            .then([this, createInfo](PreparedQueryResult result)
            {
                bool haveSameRace = false;
                uint8 heroicReqLevel = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_MIN_LEVEL_FOR_HEROIC_CHARACTER);
                uint8 startLevel = sWorld->getIntConfig(CONFIG_START_PLAYER_LEVEL);
                bool allowTwoSideAccounts = !sWorld->IsPvPRealm() || HasPermission(rbac::RBAC_PERM_TWO_SIDE_CHARACTER_CREATION);
                bool hasHeroicReqLevel = (createInfo->MaxLevel >= heroicReqLevel);
                uint32 skipCinematics = sWorld->getIntConfig(CONFIG_SKIP_CINEMATICS);
                bool checkHeroicReqs = createInfo->Class == CLASS_DEATH_KNIGHT && !HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_HEROIC_CHARACTER);

                if (result)
                {
                    uint32 team = Player::TeamForRace(createInfo->Race);
                    uint32 freeHeroicSlots = sWorld->getIntConfig(CONFIG_HEROIC_CHARACTERS_PER_REALM);

                    Field* field = result->Fetch();
                    uint8 accRace  = field[0].GetUInt8();

                    if (checkHeroicReqs)
                    {
                        uint8 accClass = field[1].GetUInt8();
                        if (accClass == CLASS_DEATH_KNIGHT)
                        {
                            if (freeHeroicSlots > 0)
                                --freeHeroicSlots;

                            if (freeHeroicSlots == 0)
                            {
                                WorldPacket data(SMSG_CHAR_CREATE, 1);
                                data << uint8(CHAR_CREATE_UNIQUE_CLASS_LIMIT);
                                SendPacket(std::move(data));
                                return;
                            }
                        }
                    }

                    // need to check team only for first character
                    /// @todo what to if account already has characters of both races?
                    if (!allowTwoSideAccounts)
                    {
                        uint32 accTeam = 0;
                        if (accRace > 0)
                            accTeam = Player::TeamForRace(accRace);

                        if (accTeam != team)
                        {
                            WorldPacket data(SMSG_CHAR_CREATE, 1);
                            data << uint8(CHAR_CREATE_PVP_TEAMS_VIOLATION);
                            SendPacket(std::move(data));
                            return;
                        }
                    }

                    // search same race for cinematic or same class if need
                    /// @todo check if cinematic already shown? (already logged in?; cinematic field)
                    while ((skipCinematics == 1 && !haveSameRace) || createInfo->Class == CLASS_DEATH_KNIGHT)
                    {
                        if (!result->NextRow())
                            break;

                        field = result->Fetch();
                        accRace = field[0].GetUInt8();

                        if (!haveSameRace)
                            haveSameRace = createInfo->Race == accRace;

                        if (checkHeroicReqs)
                        {
                            uint8 acc_class = field[1].GetUInt8();
                            if (acc_class == CLASS_DEATH_KNIGHT)
                            {
                                if (freeHeroicSlots > 0)
                                    --freeHeroicSlots;

                                if (freeHeroicSlots == 0)
                                {
                                    WorldPacket data(SMSG_CHAR_CREATE, 1);
                                    data << uint8(CHAR_CREATE_UNIQUE_CLASS_LIMIT);
                                    SendPacket(std::move(data));
                                    return;
                                }
                            }

                            if (!hasHeroicReqLevel)
                            {
                                if (createInfo->MaxLevel >= heroicReqLevel)
                                    hasHeroicReqLevel = true;
                            }
                        }
                    }
                }

                if (checkHeroicReqs && !hasHeroicReqLevel)
                {
                    WorldPacket data(SMSG_CHAR_CREATE, 1);
                    data << uint8(CHAR_CREATE_LEVEL_REQUIREMENT);
                    SendPacket(std::move(data));
                    return;
                }

                if (createInfo->Data.rpos() < createInfo->Data.wpos())
                {
                    uint8 unk;
                    createInfo->Data >> unk;
                    TC_LOG_DEBUG("network", "Character creation %s (account %u) has unhandled tail data: [%u]", createInfo->Name.c_str(), GetAccountId(), unk);
                }

                std::shared_ptr<Player> newChar = Player::Create(*this, *createInfo);
                if (!newChar) // player creation failed
                {
                    WorldPacket data(SMSG_CHAR_CREATE, 1);
                    data << uint8(CHAR_CREATE_ERROR);
                    SendPacket(std::move(data));
                    return;
                }

                if ((haveSameRace && skipCinematics == 1) || skipCinematics == 2)
                    newChar->setCinematic(1);                          // not show intro

                newChar->SetAtLoginFlag(AT_LOGIN_FIRST);               // First login

                // Player created, save it now
                newChar->SaveToDB(true);
                createInfo->CharCount += 1;

                // Find correct maxlevel for realmcharacters table
                uint8 crtLevel = createInfo->MaxRealmLevel;
                if (createInfo->Class == CLASS_DEATH_KNIGHT)
                {
                    // This is only needed for accounts with security level
                    if (heroicReqLevel > crtLevel)
                        crtLevel = heroicReqLevel;
                }
                else
                {
                    // In case of first character
                    if (startLevel > crtLevel)
                        crtLevel = startLevel;
                }


                SQLTransaction trans = LoginDatabase.BeginTransaction();

                PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_REALM_CHARACTERS_BY_REALM);
                stmt->setUInt32(0, GetAccountId());
                stmt->setUInt32(1, realm.Id.Realm);
                trans->Append(stmt);

                stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_REALM_CHARACTERS);
                stmt->setUInt32(0, createInfo->CharCount);
                stmt->setUInt8(1, crtLevel);
                stmt->setUInt32(2, GetAccountId());
                stmt->setUInt32(3, realm.Id.Realm);
                trans->Append(stmt);

                LoginDatabase.CommitTransaction(trans);

                WorldPacket data(SMSG_CHAR_CREATE, 1);
                data << uint8(CHAR_CREATE_SUCCESS);
                SendPacket(std::move(data));

                std::string IP_str = GetRemoteAddress();
                TC_LOG_INFO("entities.player.character", "Account: %d (IP: %s) Create Character:[%s] (GUID: %u)", GetAccountId(), IP_str.c_str(), createInfo->Name.c_str(), newChar->GetGUID().GetCounter());
                sScriptMgr->OnPlayerCreate(newChar.get());
                player::PlayerCache::Update(newChar.get());

                newChar->CleanupsBeforeDelete();
            });
}

void WorldSession::HandleCharDeleteOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData >> guid;
    // Initiating
    uint32 initAccountId = GetAccountId();

    // can't delete loaded character
    if (ObjectAccessor::FindPlayer(guid))
    {
        sScriptMgr->OnPlayerFailedDelete(guid, initAccountId);
        return;
    }

    uint32 accountId = 0;
    uint8 level = 0;
    std::string name;

    // is guild leader
    if (sGuildMgr->GetGuildByLeader(guid))
    {
        sScriptMgr->OnPlayerFailedDelete(guid, initAccountId);
        WorldPacket data(SMSG_CHAR_DELETE, 1);
        data << uint8(CHAR_DELETE_FAILED_GUILD_LEADER);
        SendPacket(std::move(data));
        return;
    }

    // is arena team captain
    if (sArenaTeamMgr->GetArenaTeamByCaptain(guid))
    {
        sScriptMgr->OnPlayerFailedDelete(guid, initAccountId);
        WorldPacket data(SMSG_CHAR_DELETE, 1);
        data << uint8(CHAR_DELETE_FAILED_ARENA_CAPTAIN);
        SendPacket(std::move(data));
        return;
    }

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_DATA_BY_GUID);
    stmt->setUInt32(0, guid.GetCounter());

    if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
    {
        Field* fields = result->Fetch();
        accountId = fields[0].GetUInt32();
        name = fields[1].GetString();
        level = fields[2].GetUInt8();
    }

    // prevent deleting other players' characters using cheating tools
    if (accountId != initAccountId)
    {
        sScriptMgr->OnPlayerFailedDelete(guid, initAccountId);
        return;
    }

    std::string IP_str = GetRemoteAddress();
    TC_LOG_INFO("entities.player.character", "Account: %d, IP: %s deleted character: %s, %s, Level: %u", accountId, IP_str.c_str(), name.c_str(), guid.ToString().c_str(), level);

    // To prevent hook failure, place hook before removing reference from DB
    sScriptMgr->OnPlayerDelete(guid, initAccountId); // To prevent race conditioning, but as it also makes sense, we hand the accountId over for successful delete.
    // Shouldn't interfere with character deletion though

    if (sLog->ShouldLog("entities.player.dump", LOG_LEVEL_INFO)) // optimize GetPlayerDump call
    {
        std::string dump;
        if (PlayerDumpWriter().GetDump(guid.GetCounter(), dump))
            sLog->outCharDump(dump.c_str(), accountId, guid.GetCounter(), name.c_str());
    }

    sCalendarMgr->RemoveAllPlayerEventsAndInvites(guid);
    Player::DeleteFromDB(guid, accountId);

    WorldPacket data(SMSG_CHAR_DELETE, 1);
    data << uint8(CHAR_DELETE_SUCCESS);
    SendPacket(std::move(data));
}

void WorldSession::HandlePlayerLoginOpcode(WorldPacket& recvData)
{
    if (PlayerLoading() || GetPlayer() != NULL)
    {
        std::string IP_str = GetRemoteAddress();
        TC_LOG_ERROR("network", "Player tryes to login again, AccountId = %d, IP = %s", GetAccountId(), IP_str.c_str());
        KickPlayer();
        return;
    }

    m_playerLoading = true;
    ObjectGuid playerGuid;

    TC_LOG_DEBUG("network", "WORLD: Recvd Player Logon Message");

    recvData >> playerGuid;

    if (!IsLegitCharacterForAccount(playerGuid))
    {
        TC_LOG_ERROR("network", "Account (%u) can't login with that character (%s).", GetAccountId(), playerGuid.ToString().c_str());
        KickPlayer();
        return;
    }

    // Begin 
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_PET_BY_OWNER);
    stmt->setUInt32(0, playerGuid.GetCounter());
    CharacterDatabase.AsyncQuery(stmt)
            .via(GetExecutor()).then([this, playerGuid](PreparedQueryResult result) { HandlePetListOpcodeCallBack(playerGuid, std::move(result)); });
}

void WorldSession::BeginLoadPlayer(ObjectGuid playerGuid)
{
    LoginQueryHolder *holder = new LoginQueryHolder(GetAccountId(), playerGuid);
    if (!holder->Initialize())
    {
        delete holder;                                      // delete all unprocessed queries
        m_playerLoading = false;
        return;
    }

    CharacterDatabase.DelayQueryHolder((SQLQueryHolder*)holder)
        .via(GetExecutor()).then([this](QueryResultHolder result) { HandlePlayerLogin((LoginQueryHolder*)result.release()); });
}

void WorldSession::HandlePetListOpcodeCallBack(ObjectGuid playerGuid, PreparedQueryResult result)
{
    _petLoadCallback.clear();

    std::vector<QueryResultHolderFuture> futures;

    if (result)
    {
        uint32 row = 0;

        do
        {
            row++;

            Field* fields = result->Fetch();

            uint32 petnumber = fields[0].GetUInt32();

            auto query = PetCache::BuildPetLoadQuery(playerGuid.GetCounter(), petnumber);
            if (query)
                futures.push_back(CharacterDatabase.DelayQueryHolder(query));
        } while (result->NextRow());
    }

    if (!futures.empty())
    {
        auto all = folly::collectAll(futures.begin(), futures.end());
        all.via(GetExecutor()).then([this, playerGuid](std::vector<folly::Try<QueryResultHolder>> result)
        {
            while (!result.empty())
            {
                auto next = std::move(result.back());
                result.pop_back();
                if (next.hasValue())
                    _petLoadCallback.push_back(std::move(next.value()));
            }
            BeginLoadPlayer(playerGuid);
        });
    }
    else // no pets
        BeginLoadPlayer(playerGuid);
}

void WorldSession::HandlePlayerLogin(LoginQueryHolder* holder)
{
    ObjectGuid const playerGuid = holder->GetGuid();

    // "GetAccountId() == db stored account id" checked in LoadFromDB (prevent login not own character using cheating tools)
    std::shared_ptr<Player> pCurrChar = Player::LoadFromDB(playerGuid, *this, *holder);
    if (!pCurrChar) // if loading failed
    {
        SetPlayer(nullptr);
        KickPlayer(); // disconnect client, player no set to session and it will not deleted or saved at kick
        delete holder; // delete all unprocessed queries
        m_playerLoading = false;
        return;
    }
    MONITOR_WORLD(ReportPlayer(true));

    // for send server info and strings (config)
    ChatHandler chH = ChatHandler(this);

    pCurrChar->GetMotionMaster()->Initialize();
    pCurrChar->SendDungeonDifficulty(false);

    {
        WorldPacket data(SMSG_LOGIN_VERIFY_WORLD, 20);
        data << pCurrChar->GetMapId();
        data << pCurrChar->GetPositionX();
        data << pCurrChar->GetPositionY();
        data << pCurrChar->GetPositionZ();
        data << pCurrChar->GetOrientation();
        SendPacket(std::move(data));
    }

    // load player specific part before send times
    LoadAccountData(holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOAD_ACCOUNT_DATA), PER_CHARACTER_CACHE_MASK);
    SendAccountDataTimes(PER_CHARACTER_CACHE_MASK);

    {    
        WorldPacket data(SMSG_FEATURE_SYSTEM_STATUS, 2);         // added in 2.2.0
        data << uint8(2);                                       // unknown value
        data << uint8(0);                                       // enable(1)/disable(0) voice chat interface in client
        SendPacket(std::move(data));
    }

    // Send MOTD
    {
        WorldPacket data(SMSG_MOTD, 50);                     // new in 2.0.1
        data << (uint32)0;

        uint32 linecount=0;
        std::string str_motd = sWorld->GetMotd();
        std::string::size_type pos, nextpos;

        pos = 0;
        while ((nextpos= str_motd.find('@', pos)) != std::string::npos)
        {
            if (nextpos != pos)
            {
                data << str_motd.substr(pos, nextpos-pos);
                ++linecount;
            }
            pos = nextpos+1;
        }

        if (pos<str_motd.length())
        {
            data << str_motd.substr(pos);
            ++linecount;
        }

        data.put(0, linecount);

        SendPacket(std::move(data));
        TC_LOG_DEBUG("network", "WORLD: Sent motd (SMSG_MOTD)");

        // send server info
        if (sWorld->getIntConfig(CONFIG_ENABLE_SINFO_LOGIN) == 1)
            chH.PSendSysMessage(GitRevision::GetFullVersion());

        TC_LOG_DEBUG("network", "WORLD: Sent server info");
    }

    //QueryResult* result = CharacterDatabase.PQuery("SELECT guildid, rank FROM guild_member WHERE guid = '%u'", pCurrChar->GetGUID().GetCounter());
    if (PreparedQueryResult resultGuild = holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOAD_GUILD))
    {
        Field* fields = resultGuild->Fetch();
        pCurrChar->SetInGuild(fields[0].GetUInt32());
        pCurrChar->SetRank(fields[1].GetUInt8());
    }
    else if (pCurrChar->GetGuildId())                        // clear guild related fields in case wrong data about non existed membership
    {
        pCurrChar->SetInGuild(0);
        pCurrChar->SetRank(0);
    }

    if (pCurrChar->GetGuildId() != 0)
    {
        if (auto guild = sGuildMgr->GetGuildById(pCurrChar->GetGuildId()))
            guild->SendLoginInfo(this);
        else
        {
            // remove wrong guild data
            TC_LOG_ERROR("network", "Player %s (GUID: %u) marked as member of not existing guild (id: %u), removing guild membership for player.", pCurrChar->GetName().c_str(), pCurrChar->GetGUID().GetCounter(), pCurrChar->GetGuildId());
            pCurrChar->SetInGuild(0);
        }
    }

    {
        WorldPacket data(SMSG_LEARNED_DANCE_MOVES, 4 + 4);
        data << uint32(0);
        data << uint32(0);
        SendPacket(std::move(data));
    }

    pCurrChar->SendInitialPacketsBeforeAddToMap();

    //Show cinematic at the first time that player login
    if (!pCurrChar->getCinematic())
    {
        pCurrChar->setCinematic(1);

        if (ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(pCurrChar->getClass()))
        {
            if (cEntry->CinematicSequence)
                pCurrChar->SendCinematicStart(cEntry->CinematicSequence);
            else if (ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(pCurrChar->getRace()))
                pCurrChar->SendCinematicStart(rEntry->CinematicSequence);

            // send new char string if not empty
            if (!sWorld->GetNewCharString().empty())
                chH.PSendSysMessage("%s", sWorld->GetNewCharString().c_str());
        }
    }

    if (!pCurrChar->GetMap()->AddPlayerToMap(pCurrChar) || !pCurrChar->CheckInstanceLoginValid())
    {
        AreaTrigger const* at = sObjectMgr->GetGoBackTrigger(pCurrChar->GetMapId());
        if (at)
            pCurrChar->TeleportTo(at->target_mapId, at->target_X, at->target_Y, at->target_Z, pCurrChar->GetOrientation());
        else
            pCurrChar->TeleportTo(pCurrChar->m_homebindMapId, pCurrChar->m_homebindX, pCurrChar->m_homebindY, pCurrChar->m_homebindZ, pCurrChar->GetOrientation());
    }

    ObjectAccessor::AddObject(pCurrChar.get());
    //TC_LOG_DEBUG("Player %s added to Map.", pCurrChar->GetName().c_str());

    pCurrChar->SendInitialPacketsAfterAddToMap();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_ONLINE);

    stmt->setUInt32(0, pCurrChar->GetGUID().GetCounter());

    CharacterDatabase.Execute(stmt);

    stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_ONLINE);

    stmt->setUInt32(0, GetAccountId());

    LoginDatabase.Execute(stmt);

    pCurrChar->SetInGameTime(game::time::GetGameTimeMS());

    // announce group about member online (must be after add to player list to receive announce to self)
    if (Group* group = pCurrChar->GetGroup())
    {
        //pCurrChar->groupInfo.group->SendInit(this); // useless
        group->SendUpdate();
        group->ResetMaxEnchantingLevel();
    }

    // friend status
    sSocialMgr->SendFriendStatus(pCurrChar.get(), FRIEND_ONLINE, pCurrChar->GetGUID(), true);

    // Place character in world (and load zone) before some object loading
    pCurrChar->LoadCorpse();

    // setting Ghost+speed if dead
    if (pCurrChar->m_deathState != ALIVE)
    {
        // not blizz like, we must correctly save and load player instead...
        if (pCurrChar->getRace() == RACE_NIGHTELF)
            pCurrChar->CastSpell(pCurrChar.get(), 20584, true, nullptr);// auras SPELL_AURA_INCREASE_SPEED(+speed in wisp form), SPELL_AURA_INCREASE_SWIM_SPEED(+swim speed in wisp form), SPELL_AURA_TRANSFORM (to wisp form)
        pCurrChar->CastSpell(pCurrChar.get(), 8326, true, nullptr);     // auras SPELL_AURA_GHOST, SPELL_AURA_INCREASE_SPEED(why?), SPELL_AURA_INCREASE_SWIM_SPEED(why?)

        pCurrChar->SetMovement(MOVE_WATER_WALK);
    }

    pCurrChar->ContinueTaxiFlight();

    auto& petCache = pCurrChar->GetPetCache();
    for (auto&& result : _petLoadCallback)
        petCache.LoadFromDB(result.get());
    _petLoadCallback.clear();

    // reset for all pets before pet loading
    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_RESET_PET_TALENTS))
        Pet::resetTalentsForAllPetsOf(pCurrChar.get());

    // Load pet if any (if player not alive and in taxi flight or another then pet will remember as temporary unsummoned)
    pCurrChar->LoadPet();

    // Set FFA PvP for non GM in non-rest mode
    if (sWorld->IsFFAPvPRealm() && !pCurrChar->IsGameMaster() && !pCurrChar->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING))
        pCurrChar->SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP);

    if (pCurrChar->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP))
        pCurrChar->SetContestedPvP();

    // Apply at_login requests
    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_RESET_SPELLS))
    {
        pCurrChar->ResetSpells();
        SendNotification(LANG_RESET_SPELLS);
    }

    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_RESET_TALENTS))
    {
        pCurrChar->ResetTalents(true);
        pCurrChar->SendTalentsInfoData(false);              // original talents send already in to SendInitialPacketsBeforeAddToMap, resend reset state
        SendNotification(LANG_RESET_TALENTS);
    }

    bool firstLogin = pCurrChar->HasAtLoginFlag(AT_LOGIN_FIRST);
    if (firstLogin)
    {
        pCurrChar->RemoveAtLoginFlag(AT_LOGIN_FIRST);
        // Remove Faction/Race Change on login if loaded char from template
        pCurrChar->RemoveAtLoginFlag(AT_LOGIN_CHANGE_FACTION);
        pCurrChar->RemoveAtLoginFlag(AT_LOGIN_CHANGE_RACE);
    
        PlayerInfo const* info = sObjectMgr->GetPlayerInfo(pCurrChar->getRace(), pCurrChar->getClass());
        for (uint32 spellId : info->castSpells)
            pCurrChar->CastSpell(pCurrChar.get(), spellId, true);
    }

    // show time before shutdown if shutdown planned.
    if (sWorld->IsShuttingDown())
        sWorld->ShutdownMsg(true, pCurrChar.get());

    if (sWorld->getBoolConfig(CONFIG_ALL_TAXI_PATHS))
        pCurrChar->SetTaxiCheater(true);

    if (pCurrChar->IsGameMaster())
        SendNotification(LANG_GM_ON);

    std::string IP_str = GetRemoteAddress();
    TC_LOG_INFO("entities.player.character", "Account: %d (IP: %s) Login Character:[%s] (GUID: %u) Level: %d",
        GetAccountId(), IP_str.c_str(), pCurrChar->GetName().c_str(), pCurrChar->GetGUID().GetCounter(), pCurrChar->getLevel());

    // UUACP - Log logins
    PreparedStatement* stmt2 = LoginDatabase.GetPreparedStatement(LOGIN_INS_UUACP_LOG);
    stmt2->setUInt32(0, realm.Id.Realm);
    stmt2->setUInt32(1, GetAccountId());
    stmt2->setUInt32(2, pCurrChar->GetGUID().GetCounter());
    stmt2->setString(3, IP_str.c_str());
    stmt2->setString(4, "login");
    LoginDatabase.Execute(stmt2);

    if (!pCurrChar->IsStandState() && !pCurrChar->HasUnitState(UNIT_STATE_STUNNED))
        pCurrChar->SetStandState(UNIT_STAND_STATE_STAND);

    m_playerLoading = false;

    // Handle Login-Achievements (should be handled after loading)
    GetPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ON_LOGIN, 1);

    sScriptMgr->OnPlayerLogin(pCurrChar.get(), firstLogin);

    delete holder;

    if (pCurrChar->GetTeam() != pCurrChar->GetOTeam())
        pCurrChar->FitPlayerInTeam(pCurrChar->GetBattleground() && !pCurrChar->GetBattleground()->isArena() ? true : false, pCurrChar->GetBattleground());
}

void WorldSession::HandleSetFactionAtWar(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_SET_FACTION_ATWAR");

    uint32 repListID;
    uint8  flag;

    recvData >> repListID;
    recvData >> flag;

    GetPlayer()->GetReputationMgr().SetAtWar(repListID, flag != 0);
}

//I think this function is never used :/ I dunno, but i guess this opcode not exists
void WorldSession::HandleSetFactionCheat(WorldPacket & /*recvData*/)
{
    TC_LOG_ERROR("network", "WORLD SESSION: HandleSetFactionCheat, not expected call, please report.");
    GetPlayer()->GetReputationMgr().SendStates();
}

void WorldSession::HandleTutorialFlag(WorldPacket& recvData)
{
    uint32 data;
    recvData >> data;

    uint8 index = uint8(data / 32);
    if (index >= MAX_ACCOUNT_TUTORIAL_VALUES)
        return;

    uint32 value = (data % 32);

    uint32 flag = GetTutorialInt(index);
    flag |= (1 << value);
    SetTutorialInt(index, flag);
}

void WorldSession::HandleTutorialClear(WorldPacket & /*recvData*/)
{
    for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
        SetTutorialInt(i, 0xFFFFFFFF);
}

void WorldSession::HandleTutorialReset(WorldPacket & /*recvData*/)
{
    for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
        SetTutorialInt(i, 0x00000000);
}

void WorldSession::HandleSetWatchedFactionOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_SET_WATCHED_FACTION");
    uint32 fact;
    recvData >> fact;
    GetPlayer()->SetUInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX, fact);
}

void WorldSession::HandleSetFactionInactiveOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_SET_FACTION_INACTIVE");
    uint32 replistid;
    uint8 inactive;
    recvData >> replistid >> inactive;

    GetPlayer()->GetReputationMgr().SetInactive(replistid, inactive != 0);
}

void WorldSession::HandleShowingHelmOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "CMSG_SHOWING_HELM for %s", GetPlayer()->GetName().c_str());
    recvData.read_skip<uint8>(); // unknown, bool?
    GetPlayer()->ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_HELM);
}

void WorldSession::HandleShowingCloakOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "CMSG_SHOWING_CLOAK for %s", GetPlayer()->GetName().c_str());
    recvData.read_skip<uint8>(); // unknown, bool?
    GetPlayer()->ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_CLOAK);
}

void WorldSession::HandleCharRenameOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    std::string newName;

    recvData >> guid;
    recvData >> newName;

    // prevent character rename to invalid name
    if (!normalizePlayerName(newName))
    {
        WorldPacket data(SMSG_CHAR_RENAME, 1);
        data << uint8(CHAR_NAME_NO_NAME);
        SendPacket(std::move(data));
        return;
    }

    uint8 res = ObjectMgr::CheckPlayerName(newName, true);
    if (res != CHAR_NAME_SUCCESS)
    {
        WorldPacket data(SMSG_CHAR_RENAME, 1+8+(newName.size()+1));
        data << uint8(res);
        data << uint64(guid);
        data << newName;
        SendPacket(std::move(data));
        return;
    }

    // check name limitations
    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RESERVEDNAME) && sObjectMgr->IsReservedName(newName))
    {
        WorldPacket data(SMSG_CHAR_RENAME, 1);
        data << uint8(CHAR_NAME_RESERVED);
        SendPacket(std::move(data));
        return;
    }

    // Ensure that the character belongs to the current account, that rename at login is enabled
    // and that there is no character with the desired new name

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_FREE_NAME);

    stmt->setUInt32(0, guid.GetCounter());
    stmt->setUInt32(1, GetAccountId());
    stmt->setUInt16(2, AT_LOGIN_RENAME);
    stmt->setUInt16(3, AT_LOGIN_RENAME);
    stmt->setString(4, newName);

    CharacterDatabase.AsyncQuery(stmt)
            .via(GetExecutor()).then([this, newName](PreparedQueryResult result) { HandleChangePlayerNameOpcodeCallBack(std::move(result), newName); });
}

void WorldSession::HandleChangePlayerNameOpcodeCallBack(PreparedQueryResult result, std::string const& newName)
{
    if (!result)
    {
        WorldPacket data(SMSG_CHAR_RENAME, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket(std::move(data));
        return;
    }

    Field* fields = result->Fetch();

    uint32 guidLow      = fields[0].GetUInt32();
    std::string oldName = fields[1].GetString();

    ObjectGuid guid = ObjectGuid(HighGuid::Player, guidLow);

    // Update name and at_login flag in the db
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_NAME);

    stmt->setString(0, newName);
    stmt->setUInt16(1, AT_LOGIN_RENAME);
    stmt->setUInt32(2, guidLow);
    trans->Append(stmt);

    // Removed declined name from db
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_DECLINED_NAME);
    stmt->setUInt32(0, guidLow);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);

    TC_LOG_INFO("entities.player.character", "Account: %d (IP: %s) Character:[%s] (%s) Changed name to: %s", GetAccountId(), GetRemoteAddress().c_str(), oldName.c_str(), guid.ToString().c_str(), newName.c_str());

    WorldPacket data(SMSG_CHAR_RENAME, 1+8+(newName.size()+1));
    data << uint8(RESPONSE_SUCCESS);
    data << uint64(guid);
    data << newName;
    SendPacket(std::move(data));

    RG_LOG<PremiumLogModule>(guid, RG::Logging::PremiumLogModule::Action::Apply, CodeType::CharChangeName);

    player::PlayerCache::UpdateNameData(guid, newName);
}

void WorldSession::HandleSetPlayerDeclinedNames(WorldPacket& recvData)
{
    ObjectGuid guid;

    recvData >> guid;

    // not accept declined names for unsupported languages
    std::string name;
    if (player::PlayerCache::Exists(guid))
        name = player::PlayerCache::GetName(guid);
    else
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
        data << uint32(1);
        data << uint64(guid);
        SendPacket(std::move(data));
        return;
    }

    std::wstring wname;
    if (!Utf8toWStr(name, wname))
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
        data << uint32(1);
        data << uint64(guid);
        SendPacket(std::move(data));
        return;
    }

    if (!isCyrillicCharacter(wname[0]))                      // name already stored as only single alphabet using
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
        data << uint32(1);
        data << uint64(guid);
        SendPacket(std::move(data));
        return;
    }

    std::string name2;
    DeclinedName declinedname;

    recvData >> name2;

    if (name2 != name)                                       // character have different name
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
        data << uint32(1);
        data << uint64(guid);
        SendPacket(std::move(data));
        return;
    }

    for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
    {
        recvData >> declinedname.name[i];
        if (!normalizePlayerName(declinedname.name[i]))
        {
            WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
            data << uint32(1);
            data << uint64(guid);
            SendPacket(std::move(data));
            return;
        }
    }

    if (!ObjectMgr::CheckDeclinedNames(wname, declinedname))
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
        data << uint32(1);
        data << uint64(guid);
        SendPacket(std::move(data));
        return;
    }

    for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        CharacterDatabase.EscapeString(declinedname.name[i]);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_DECLINED_NAME);
    stmt->setUInt32(0, guid.GetCounter());
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_DECLINED_NAME);
    stmt->setUInt32(0, guid.GetCounter());

    for (uint8 i = 0; i < 5; i++)
        stmt->setString(i+1, declinedname.name[i]);

    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);

    WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
    data << uint32(0);                                      // OK
    data << uint64(guid);
    SendPacket(std::move(data));
}

void WorldSession::HandleAlterAppearance(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "CMSG_ALTER_APPEARANCE");

    uint32 Hair, Color, FacialHair, SkinColor;
    recvData >> Hair >> Color >> FacialHair >> SkinColor;

    BarberShopStyleEntry const* bs_hair = sBarberShopStyleStore.LookupEntry(Hair);

    if (!bs_hair || bs_hair->type != 0 || bs_hair->race != GetPlayer()->getRace() || bs_hair->gender != GetPlayer()->getGender())
        return;

    BarberShopStyleEntry const* bs_facialHair = sBarberShopStyleStore.LookupEntry(FacialHair);

    if (!bs_facialHair || bs_facialHair->type != 2 || bs_facialHair->race != GetPlayer()->getRace() || bs_facialHair->gender != GetPlayer()->getGender())
        return;

    BarberShopStyleEntry const* bs_skinColor = sBarberShopStyleStore.LookupEntry(SkinColor);

    if (bs_skinColor && (bs_skinColor->type != 3 || bs_skinColor->race != GetPlayer()->getRace() || bs_skinColor->gender != GetPlayer()->getGender()))
        return;

    GameObject* go = GetPlayer()->FindNearestGameObjectOfType(GAMEOBJECT_TYPE_BARBER_CHAIR, 5.0f);
    if (!go)
    {
        WorldPacket data(SMSG_BARBER_SHOP_RESULT, 4);
        data << uint32(2);
        SendPacket(std::move(data));
        return;
    }

    if (GetPlayer()->GetStandState() != UNIT_STAND_STATE_SIT_LOW_CHAIR + go->GetGOInfo()->barberChair.chairheight)
    {
        WorldPacket data(SMSG_BARBER_SHOP_RESULT, 4);
        data << uint32(2);
        SendPacket(std::move(data));
        return;
    }

    uint32 cost = GetPlayer()->GetBarberShopCost(bs_hair->hair_id, Color, bs_facialHair->hair_id, bs_skinColor);

    // 0 - ok
    // 1, 3 - not enough money
    // 2 - you have to seat on barber chair
    if (!GetPlayer()->HasEnoughMoney(cost))
    {
        WorldPacket data(SMSG_BARBER_SHOP_RESULT, 4);
        data << uint32(1);                                  // no money
        SendPacket(std::move(data));
        return;
    }
    else
    {
        WorldPacket data(SMSG_BARBER_SHOP_RESULT, 4);
        data << uint32(0);                                  // ok
        SendPacket(std::move(data));
    }

    GetPlayer()->ModifyMoney(-int32(cost));                     // it isn't free
    GetPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER, cost);

    GetPlayer()->SetByteValue(PLAYER_BYTES, 2, uint8(bs_hair->hair_id));
    GetPlayer()->SetByteValue(PLAYER_BYTES, 3, uint8(Color));
    GetPlayer()->SetByteValue(PLAYER_BYTES_2, 0, uint8(bs_facialHair->hair_id));
    if (bs_skinColor)
        GetPlayer()->SetByteValue(PLAYER_BYTES, 0, uint8(bs_skinColor->hair_id));

    GetPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP, 1);

    GetPlayer()->SetStandState(0);                              // stand up
}

void WorldSession::HandleRemoveGlyph(WorldPacket& recvData)
{
    uint32 slot;
    recvData >> slot;

    if (slot >= MAX_GLYPH_SLOT_INDEX)
    {
        TC_LOG_DEBUG("network", "Client sent wrong glyph slot number in opcode CMSG_REMOVE_GLYPH %u", slot);
        return;
    }

    if (uint32 glyph = GetPlayer()->GetGlyph(slot))
    {
        if (GlyphPropertiesEntry const* gp = sGlyphPropertiesStore.LookupEntry(glyph))
        {
            GetPlayer()->RemoveAurasDueToSpell(gp->SpellId);
            GetPlayer()->SetGlyph(slot, 0);
            GetPlayer()->SendTalentsInfoData(false);
        }
    }
}

void WorldSession::HandleCharCustomize(WorldPacket& recvData)
{
    ObjectGuid guid;
    std::string newName;

    recvData >> guid;
    if (!IsLegitCharacterForAccount(guid))
    {
        TC_LOG_ERROR("network", "Account %u, IP: %s tried to customise %s, but it does not belong to their account!",
            GetAccountId(), GetRemoteAddress().c_str(), guid.ToString().c_str());
        recvData.rfinish();
        KickPlayer();
        return;
    }

    recvData >> newName;

    uint8 gender, skin, face, hairStyle, hairColor, facialHair;
    recvData >> gender >> skin >> hairColor >> hairStyle >> facialHair >> face;

    auto cache_entry = player::PlayerCache::Get(guid);
    if (!cache_entry)
    {
        WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket(std::move(data));
        return;
    }

    uint32 at_loginFlags = cache_entry.at_login;
    if (!(at_loginFlags & AT_LOGIN_CUSTOMIZE))
    {
        WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket(std::move(data));
        return;
    }

    // prevent character rename to invalid name
    if (!normalizePlayerName(newName))
    {
        WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
        data << uint8(CHAR_NAME_NO_NAME);
        SendPacket(std::move(data));
        return;
    }

    uint8 res = ObjectMgr::CheckPlayerName(newName, true);
    if (res != CHAR_NAME_SUCCESS)
    {
        WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
        data << uint8(res);
        SendPacket(std::move(data));
        return;
    }

    // check name limitations
    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RESERVEDNAME) && sObjectMgr->IsReservedName(newName))
    {
        WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
        data << uint8(CHAR_NAME_RESERVED);
        SendPacket(std::move(data));
        return;
    }

    // character with this name already exist
    if (ObjectGuid newguid = player::PlayerCache::GetGUID(newName))
    {
        if (newguid != guid)
        {
            WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
            data << uint8(CHAR_CREATE_NAME_IN_USE);
            SendPacket(std::move(data));
            return;
        }
    }

    auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_NAME);
    stmt->setUInt32(0, guid.GetCounter());
    auto result = CharacterDatabase.Query(stmt);

    if (result)
    {
        std::string oldname = result->Fetch()[0].GetString();
        TC_LOG_INFO("entities.player.character", "Account: %d (IP: %s), Character[%s] (%s) Customized to: %s", GetAccountId(), GetRemoteAddress().c_str(), oldname.c_str(), guid.ToString().c_str(), newName.c_str());
    }

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    Player::Customize(trans, guid, gender, skin, face, hairStyle, hairColor, facialHair);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_NAME_AT_LOGIN);
    stmt->setString(0, newName);
    stmt->setUInt16(1, uint16(AT_LOGIN_CUSTOMIZE));
    stmt->setUInt32(2, guid.GetCounter());
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_DECLINED_NAME);
    stmt->setUInt32(0, guid.GetCounter());
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans)
            .via(GetExecutor()).then(
            [=](bool success)
            {
                if (!success)
                    return;

                RG_LOG<PremiumLogModule>(guid, RG::Logging::PremiumLogModule::Action::Apply, CodeType::CharCustomize);
                player::PlayerCache::UpdateNameData(guid, newName, gender);

                WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1+8+(newName.size()+1)+6);
                data << uint8(RESPONSE_SUCCESS);
                data << uint64(guid);
                data << newName;
                data << uint8(gender);
                data << uint8(skin);
                data << uint8(face);
                data << uint8(hairStyle);
                data << uint8(hairColor);
                data << uint8(facialHair);
                SendPacket(std::move(data));
            });
}

void WorldSession::HandleEquipmentSetSave(WorldPacket &recvData)
{
    TC_LOG_DEBUG("network", "CMSG_EQUIPMENT_SET_SAVE");

    const uint64 setGuid = readPackGUID(recvData);

    uint32 index;
    recvData >> index;
    if (index >= MAX_EQUIPMENT_SET_INDEX)                    // client set slots amount
        return;

    std::string name;
    recvData >> name;

    std::string iconName;
    recvData >> iconName;

    EquipmentSet eqSet;

    eqSet.Guid      = setGuid;
    eqSet.Name      = name;
    eqSet.IconName  = iconName;
    eqSet.state     = EQUIPMENT_SET_NEW;

    for (uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        ObjectGuid itemGuid;
        recvData >> itemGuid.ReadAsPacked();

        // if client sends 0, it means empty slot
        if (itemGuid.IsEmpty())
        {
            eqSet.Items[i] = 0;
            continue;
        }

        // equipment manager sends "1" (as raw GUID) for slots set to "ignore" (don't touch slot at equip set)
        if (itemGuid.GetRawValue() == 1)
        {
            // ignored slots saved as bit mask because we have no free special values for Items[i]
            eqSet.IgnoreMask |= 1 << i;
            continue;
        }

        // some cheating checks
        Item* item = GetPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (!item || item->GetGUID() != itemGuid)
        {
            eqSet.Items[i] = 0;
            continue;
        }

        eqSet.Items[i] = itemGuid.GetCounter();
    }

    GetPlayer()->SetEquipmentSet(index, eqSet);
}

void WorldSession::HandleEquipmentSetDelete(WorldPacket &recvData)
{
    TC_LOG_DEBUG("network", "CMSG_EQUIPMENT_SET_DELETE");

    const uint64 setGuid = readPackGUID(recvData);

    GetPlayer()->DeleteEquipmentSet(setGuid);
}

void WorldSession::HandleEquipmentSetUse(WorldPacket &recvData)
{
    TC_LOG_DEBUG("network", "CMSG_EQUIPMENT_SET_USE");

    for (uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        ObjectGuid itemGuid;
        recvData >> itemGuid.ReadAsPacked();

        uint8 srcbag, srcslot;
        recvData >> srcbag >> srcslot;

        TC_LOG_DEBUG("entities.player.items", "%s: srcbag %u, srcslot %u", itemGuid.ToString().c_str(), srcbag, srcslot);

        // check if item slot is set to "ignored" (raw value == 1), must not be unequipped then
        if (itemGuid.GetRawValue() == 1)
            continue;

        // Only equip weapons in combat
        if (GetPlayer()->IsInCombat() && i != EQUIPMENT_SLOT_MAINHAND && i != EQUIPMENT_SLOT_OFFHAND && i != EQUIPMENT_SLOT_RANGED)
            continue;

        Item* item = GetPlayer()->GetItemByGuid(itemGuid);

        uint16 dstpos = i | (INVENTORY_SLOT_BAG_0 << 8);

        if (!item)
        {
            Item* uItem = GetPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
            if (!uItem)
                continue;

            ItemPosCountVec sDest;
            InventoryResult msg = GetPlayer()->CanStoreItem(NULL_BAG, NULL_SLOT, sDest, uItem, false);
            if (msg == EQUIP_ERR_OK)
            {
                if (_player->CanUnequipItem(dstpos, true) != EQUIP_ERR_OK)
                    continue;

                GetPlayer()->RemoveItem(INVENTORY_SLOT_BAG_0, i, true);
                GetPlayer()->StoreItem(sDest, uItem, true);
            }
            else
                GetPlayer()->SendEquipError(msg, uItem, NULL);

            continue;
        }

        if (item->GetPos() == dstpos)
            continue;

        if (_player->CanEquipItem(i, dstpos, item, true) != EQUIP_ERR_OK)
            continue;

        GetPlayer()->SwapItem(item->GetPos(), dstpos);
    }

    WorldPacket data(SMSG_EQUIPMENT_SET_USE_RESULT, 1);
    data << uint8(0);                                       // 4 - equipment swap failed - inventory is full
    SendPacket(std::move(data));
}

void WorldSession::HandleCharFactionOrRaceChange(WorldPacket& recvData)
{
    ObjectGuid guid;
    std::string newname;
    uint8 gender, skin, face, hairStyle, hairColor, facialHair, race;
    recvData >> guid;

    if (!IsLegitCharacterForAccount(guid))
    {
        TC_LOG_ERROR("network", "Account %u, IP: %s tried to factionchange character %s, but it does not belong to their account!",
            GetAccountId(), GetRemoteAddress().c_str(), guid.ToString().c_str());
        recvData.rfinish();
        KickPlayer();
        return;
    }

    recvData >> newname;
    recvData >> gender >> skin >> hairColor >> hairStyle >> facialHair >> face >> race;

    uint32 lowGuid = guid.GetCounter();

    // get the players old (at this moment current) race
    auto info = player::PlayerCache::Get(guid);
    if (!info)
    {
        WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket(std::move(data));
        return;
    }

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_CLASS_LVL_AT_LOGIN);
    stmt->setUInt32(0, lowGuid);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);
    if (!result)
    {
        WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket(std::move(data));
        return;
    }

    Field* fields = result->Fetch();
    uint8 oldRace = info.race;
    uint32 playerClass = uint32(fields[0].GetUInt8());
    uint32 level = uint32(fields[1].GetUInt8());
    uint32 at_loginFlags = fields[2].GetUInt16();
    uint32 used_loginFlag = ((recvData.GetOpcode() == CMSG_CHAR_RACE_CHANGE) ? AT_LOGIN_CHANGE_RACE : AT_LOGIN_CHANGE_FACTION);

    if (!sObjectMgr->GetPlayerInfo(race, playerClass))
    {
        WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket(std::move(data));
        return;
    }

    if (!(at_loginFlags & used_loginFlag))
    {
        WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket(std::move(data));
        return;
    }

    // player have to change their race or faction
    if (oldRace == race)
    {
        WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket(std::move(data));
        return;
    }

    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RACEMASK))
    {
        uint32 raceMaskDisabled = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_RACEMASK);
        if ((1 << (race - 1)) & raceMaskDisabled)
        {
            WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
            data << uint8(CHAR_CREATE_ERROR);
            SendPacket(std::move(data));
            return;
        }
    }

    // prevent character rename to invalid name
    if (!normalizePlayerName(newname))
    {
        WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
        data << uint8(CHAR_NAME_NO_NAME);
        SendPacket(std::move(data));
        return;
    }

    uint8 res = ObjectMgr::CheckPlayerName(newname, true);
    if (res != CHAR_NAME_SUCCESS)
    {
        WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
        data << uint8(res);
        SendPacket(std::move(data));
        return;
    }

    // check name limitations
    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RESERVEDNAME) && sObjectMgr->IsReservedName(newname))
    {
        WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
        data << uint8(CHAR_NAME_RESERVED);
        SendPacket(std::move(data));
        return;
    }

    // character with this name already exist
    if (ObjectGuid newguid = player::PlayerCache::GetGUID(newname))
    {
        if (newguid != guid)
        {
            WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
            data << uint8(CHAR_CREATE_NAME_IN_USE);
            SendPacket(std::move(data));
            return;
        }
    }

    // remove force rename on race/faction change because faction/racechange already contains a rename
    used_loginFlag |= AT_LOGIN_RENAME;
    // resurrect the character in case he's dead
    sObjectAccessor->ConvertCorpseForPlayer(guid);

    CharacterDatabase.EscapeString(newname);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    Player::Customize(trans, guid, gender, skin, face, hairStyle, hairColor, facialHair);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_FACTION_OR_RACE);
    stmt->setString(0, newname);
    stmt->setUInt8(1, race);
    stmt->setUInt16(2, used_loginFlag);
    stmt->setUInt32(3, lowGuid);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_DECLINED_NAME);
    stmt->setUInt32(0, lowGuid);
    trans->Append(stmt);

    player::PlayerCache::UpdateNameData(guid, newname, gender, race);

    TeamId team = TEAM_ALLIANCE;

    // Search each faction is targeted
    switch (race)
    {
        case RACE_ORC:
        case RACE_TAUREN:
        case RACE_UNDEAD_PLAYER:
        case RACE_TROLL:
        case RACE_BLOODELF:
            team = TEAM_HORDE;
            break;
        default:
            break;
    }

    // Switch Languages
    // delete all languages first
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_SKILL_LANGUAGES);
    stmt->setUInt32(0, lowGuid);
    trans->Append(stmt);

    // Now add them back
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_SKILL_LANGUAGE);
    stmt->setUInt32(0, lowGuid);

    // Faction specific languages
    if (team == TEAM_HORDE)
        stmt->setUInt16(1, 109);
    else
        stmt->setUInt16(1, 98);

    trans->Append(stmt);

    // Race specific languages
    if (race != RACE_ORC && race != RACE_HUMAN)
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_SKILL_LANGUAGE);
        stmt->setUInt32(0, lowGuid);

        switch (race)
        {
            case RACE_DWARF:
                stmt->setUInt16(1, 111);
                break;
            case RACE_DRAENEI:
                stmt->setUInt16(1, 759);
                break;
            case RACE_GNOME:
                stmt->setUInt16(1, 313);
                break;
            case RACE_NIGHTELF:
                stmt->setUInt16(1, 113);
                break;
            case RACE_UNDEAD_PLAYER:
                stmt->setUInt16(1, 673);
                break;
            case RACE_TAUREN:
                stmt->setUInt16(1, 115);
                break;
            case RACE_TROLL:
                stmt->setUInt16(1, 315);
                break;
            case RACE_BLOODELF:
                stmt->setUInt16(1, 137);
                break;
        }

        trans->Append(stmt);
    }

    if (recvData.GetOpcode() == CMSG_CHAR_FACTION_CHANGE)
    {
        // Delete all Flypaths
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_TAXI_PATH);
        stmt->setUInt32(0, lowGuid);
        trans->Append(stmt);

        if (level > 7)
        {
            // Update Taxi path
            // this doesn't seem to be 100% blizzlike... but it can't really be helped.
            std::ostringstream taximaskstream;
            uint32 numFullTaximasks = level / 7;
            if (numFullTaximasks > 11)
                numFullTaximasks = 11;
            if (team == TEAM_ALLIANCE)
            {
                if (playerClass != CLASS_DEATH_KNIGHT)
                {
                    for (uint8 i = 0; i < numFullTaximasks; ++i)
                        taximaskstream << uint32(sAllianceTaxiNodesMask[i]) << ' ';
                }
                else
                {
                    for (uint8 i = 0; i < numFullTaximasks; ++i)
                        taximaskstream << uint32(sAllianceTaxiNodesMask[i] | sDeathKnightTaxiNodesMask[i]) << ' ';
                }
            }
            else
            {
                if (playerClass != CLASS_DEATH_KNIGHT)
                {
                    for (uint8 i = 0; i < numFullTaximasks; ++i)
                        taximaskstream << uint32(sHordeTaxiNodesMask[i]) << ' ';
                }
                else
                {
                    for (uint8 i = 0; i < numFullTaximasks; ++i)
                        taximaskstream << uint32(sHordeTaxiNodesMask[i] | sDeathKnightTaxiNodesMask[i]) << ' ';
                }
            }

            uint32 numEmptyTaximasks = 11 - numFullTaximasks;
            for (uint8 i = 0; i < numEmptyTaximasks; ++i)
                taximaskstream << "0 ";
            taximaskstream << '0';
            std::string taximask = taximaskstream.str();

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_TAXIMASK);
            stmt->setString(0, taximask);
            stmt->setUInt32(1, lowGuid);
            trans->Append(stmt);
        }

        if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD))
        {
            // Reset guild and delete surcoat
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUILD_MEMBER);

            stmt->setUInt32(0, lowGuid);

            PreparedQueryResult result = CharacterDatabase.Query(stmt);
            if (result)
                if (auto guild = sGuildMgr->GetGuildById((result->Fetch()[0]).GetUInt32()))
                    guild->DeleteMember(trans, guid);

            // Delete surcoat
            PreparedStatement* stmtg = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_SURCOAT);
            stmtg->setUInt32(0, guid.GetCounter());
            trans->Append(stmtg);

            Player::LeaveAllArenaTeams(guid);
        }

        if (!HasPermission(rbac::RBAC_PERM_TWO_SIDE_ADD_FRIEND))
        {
            // Delete Friend List
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_SOCIAL_BY_GUID);
            stmt->setUInt32(0, lowGuid);
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_SOCIAL_BY_FRIEND);
            stmt->setUInt32(0, lowGuid);
            trans->Append(stmt);
        }

        // Reset homebind and position
        PreparedStatement* stmthb = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PLAYER_HOMEBIND);
        stmthb->setUInt32(0, lowGuid);
        trans->Append(stmthb);

        PreparedStatement* stmthb2 = WorldDatabase.GetPreparedStatement(WORLD_LOAD_FAC_CHANGE_HOMEBIND);
        stmthb2->setUInt8(0, race);
        result = WorldDatabase.Query(stmthb2);

        Field *newhomebind = result->Fetch();

        Player::SavePositionInDB(trans, newhomebind[0].GetUInt16(),newhomebind[2].GetFloat(),newhomebind[3].GetFloat(),newhomebind[4].GetFloat(),0.0f,newhomebind[1].GetUInt16(),guid);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PLAYER_HOMEBIND);
        stmt->setUInt32(0, lowGuid);
        stmt->setUInt16(1, newhomebind[0].GetUInt16());
        stmt->setUInt16(2, newhomebind[1].GetUInt16());
        stmt->setFloat (3, newhomebind[2].GetFloat());
        stmt->setFloat (4, newhomebind[3].GetFloat());
        stmt->setFloat (5, newhomebind[4].GetFloat());
        trans->Append(stmt);

        // Achievement conversion
        for (std::map<uint32, uint32>::const_iterator it = sObjectMgr->FactionChangeAchievements.begin(); it != sObjectMgr->FactionChangeAchievements.end(); ++it)
        {
            uint32 achiev_alliance = it->first;
            uint32 achiev_horde = it->second;

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT_BY_ACHIEVEMENT);
            stmt->setUInt16(0, uint16(team == TEAM_ALLIANCE ? achiev_alliance : achiev_horde));
            stmt->setUInt32(1, lowGuid);
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_ACHIEVEMENT);
            stmt->setUInt16(0, uint16(team == TEAM_ALLIANCE ? achiev_alliance : achiev_horde));
            stmt->setUInt16(1, uint16(team == TEAM_ALLIANCE ? achiev_horde : achiev_alliance));
            stmt->setUInt32(2, lowGuid);
            trans->Append(stmt);
        }

        // Item conversion
        for (std::map<uint32, uint32>::const_iterator it = sObjectMgr->FactionChangeItems.begin(); it != sObjectMgr->FactionChangeItems.end(); ++it)
        {
            uint32 item_alliance = it->first;
            uint32 item_horde = it->second;

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_INVENTORY_FACTION_CHANGE);
            stmt->setUInt32(0, (team == TEAM_ALLIANCE ? item_alliance : item_horde));
            stmt->setUInt32(1, (team == TEAM_ALLIANCE ? item_horde : item_alliance));
            stmt->setUInt32(2, lowGuid);
            trans->Append(stmt);
        }

        // Delete all current quests
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_QUESTSTATUS);
        stmt->setUInt32(0, guid.GetCounter());
        trans->Append(stmt);

        // Delete all related quest items
        PreparedStatement* stmtq = CharacterDatabase.GetPreparedStatement(CHAR_SEL_QUEST_STATUS);
        stmtq->setUInt32(0, lowGuid);
        PreparedQueryResult activequests = CharacterDatabase.Query(stmtq);

        if (activequests)
        {
            do
            {
                Field* questidf = activequests->Fetch();
                uint32 questid = questidf[0].GetUInt32();

                // Select questitems from quest_template table
                PreparedStatement* stmtqi = WorldDatabase.GetPreparedStatement(WORLD_LOAD_ITEMS_FOR_QUEST);
                stmtqi->setUInt32(0, questid);
                PreparedQueryResult activequestsitems = WorldDatabase.Query(stmtqi);
                if (activequestsitems)
                {
                    Field* questtempf = activequestsitems->Fetch();

                    for (uint8 i = 1; i < 8; ++i) // check for every item if character has item in item_instance
                    {
                        uint32 itemEntry = questtempf[i].GetUInt32();
                        PreparedStatement* stmtii = CharacterDatabase.GetPreparedStatement(CHAR_SEL_QUEST_ITEM_ITEM_INSTANCE);
                        stmtii->setUInt32(0, itemEntry);
                        stmtii->setUInt32(1, lowGuid);
                        PreparedQueryResult iteminstance = CharacterDatabase.Query(stmtii);

                        if (iteminstance) // if player has item in item instance, remove from inventory and item_instance
                        {
                            Field* itemguidf = iteminstance->Fetch();
                            uint32 itemguid = itemguidf[0].GetUInt32();

                            ItemTemplate const* item = sObjectMgr->GetItemTemplate(itemEntry);
                            if (item && item->Bonding != BIND_QUEST_ITEM)
                                continue;

                            PreparedStatement* stmtdel1 = CharacterDatabase.GetPreparedStatement(CHAR_DEL_QUEST_ITEM_FROM_INVENTORY);
                            stmtdel1->setUInt32(0, itemguid);
                            stmtdel1->setUInt32(1, lowGuid);
                            trans->Append(stmtdel1);

                            PreparedStatement* stmtdel2 = CharacterDatabase.GetPreparedStatement(CHAR_DEL_QUEST_ITEM_FROM_ITEM_INSTANCE);
                            stmtdel2->setUInt32(0, itemguid);
                            stmtdel2->setUInt32(1, lowGuid);
                            trans->Append(stmtdel2);
                        }
                    }
                }
            } while (activequests->NextRow());
        }

        // Quest conversion
        for (auto it : sObjectMgr->FactionChangeQuests)
        {
            uint32 quest_alliance = it.first;
            uint32 quest_horde = it.second;

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_QUESTSTATUS_REWARDED_BY_QUEST);
            stmt->setUInt32(0, lowGuid);
            stmt->setUInt32(1, (team == TEAM_ALLIANCE ? quest_alliance : quest_horde));
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_QUESTSTATUS_REWARDED_FACTION_CHANGE);
            stmt->setUInt32(0, (team == TEAM_ALLIANCE ? quest_alliance : quest_horde));
            stmt->setUInt32(1, (team == TEAM_ALLIANCE ? quest_horde : quest_alliance));
            stmt->setUInt32(2, lowGuid);
            trans->Append(stmt);
        }

        // Mark all rewarded quests as "active" (will count for completed quests achievements)
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_QUESTSTATUS_REWARDED_ACTIVE);
        stmt->setUInt32(0, lowGuid);
        trans->Append(stmt);

        // Disable all old-faction specific quests
        {
            auto const& questTemplates = sObjectMgr->GetQuestTemplates();
            for (auto iter : questTemplates)
            {
                Quest const quest = iter.second;
                uint32 newRaceMask = (team == TEAM_ALLIANCE) ? RACEMASK_ALLIANCE : RACEMASK_HORDE;
                if (!(quest.GetRequiredRaces() & newRaceMask))
                {
                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_QUESTSTATUS_REWARDED_ACTIVE_BY_QUEST);
                    stmt->setUInt32(0, lowGuid);
                    stmt->setUInt32(1, quest.GetQuestId());
                    trans->Append(stmt);
                }
            }
        }

        // Spell conversion
        for (std::map<uint32, uint32>::const_iterator it = sObjectMgr->FactionChangeSpells.begin(); it != sObjectMgr->FactionChangeSpells.end(); ++it)
        {
            uint32 spell_alliance = it->first;
            uint32 spell_horde = it->second;

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_SPELL_BY_SPELL);
            stmt->setUInt32(0, (team == TEAM_ALLIANCE ? spell_alliance : spell_horde));
            stmt->setUInt32(1, lowGuid);
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_SPELL_FACTION_CHANGE);
            stmt->setUInt32(0, (team == TEAM_ALLIANCE ? spell_alliance : spell_horde));
            stmt->setUInt32(1, (team == TEAM_ALLIANCE ? spell_horde : spell_alliance));
            stmt->setUInt32(2, lowGuid);
            trans->Append(stmt);
        }

        // Reputation conversion
        for (std::map<uint32, uint32>::const_iterator it = sObjectMgr->FactionChangeReputation.begin(); it != sObjectMgr->FactionChangeReputation.end(); ++it)
        {
            uint32 reputation_alliance = it->first;
            uint32 reputation_horde = it->second;
            uint32 newReputation = (team == TEAM_ALLIANCE) ? reputation_alliance : reputation_horde;
            uint32 oldReputation = (team == TEAM_ALLIANCE) ? reputation_horde : reputation_alliance;

            // select old standing set in db
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_REP_BY_FACTION);
            stmt->setUInt32(0, oldReputation);
            stmt->setUInt32(1, lowGuid);
            PreparedQueryResult result = CharacterDatabase.Query(stmt);

            if (!result)
            {
                WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
                data << uint8(CHAR_CREATE_ERROR);
                SendPacket(std::move(data));
                return;
            }

            Field* fields = result->Fetch();
            int32 oldDBRep = fields[0].GetInt32();
            FactionEntry const* factionEntry = sFactionStore.LookupEntry(oldReputation);

            // old base reputation
            int32 oldBaseRep = sObjectMgr->GetBaseReputationOf(factionEntry, oldRace, playerClass);

            // new base reputation
            int32 newBaseRep = sObjectMgr->GetBaseReputationOf(sFactionStore.LookupEntry(newReputation), race, playerClass);

            // final reputation shouldnt change
            int32 FinalRep = oldDBRep + oldBaseRep;
            int32 newDBRep = FinalRep - newBaseRep;

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_REP_BY_FACTION);
            stmt->setUInt32(0, newReputation);
            stmt->setUInt32(1, lowGuid);
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_REP_FACTION_CHANGE);
            stmt->setUInt16(0, uint16(newReputation));
            stmt->setInt32(1, newDBRep);
            stmt->setUInt16(2, uint16(oldReputation));
            stmt->setUInt32(3, lowGuid);
            trans->Append(stmt);
        }


        // Titles
        // remove current title
        PreparedStatement* stmtct = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CURR_TITLE);
        stmtct->setUInt64(0, 0);
        stmtct->setUInt32(1, lowGuid);
        trans->Append(stmtct);

        // Convert titles
        PreparedStatement* stmtt = WorldDatabase.GetPreparedStatement(WORLD_LOAD_FAC_CHANGE_TITLES);
        PreparedQueryResult resulttitles = WorldDatabase.Query(stmtt);

        if (resulttitles)
        {
            uint64 alliiid = 0;
            uint64 hordeiid = 0;
            int action = 0;
            uint64 maskno[6]; // in this array the single numberpackages from the mask are saved

                              // get full titlemask from db
            PreparedStatement* stmtar = CharacterDatabase.GetPreparedStatement(CHAR_SEL_KNOWN_TITLES);
            stmtar->setUInt32(0, lowGuid);
            PreparedQueryResult knowntitles = CharacterDatabase.Query(stmtar);

            Field* ktfield = knowntitles->Fetch();
            std::string titlesmask = ktfield[0].GetString();

            for (int i = 0; i<6; i++)
            {
                // divide whole mask to single packages
                int strpos = titlesmask.find(" ");
                std::string f = titlesmask.substr(0, strpos);
                titlesmask = titlesmask.substr(strpos + 1);

                // convert string to int and save in array maskno
                std::stringstream Str;
                Str << f;
                Str >> maskno[i];
            }

            // whole mask: 0 0 343 324 0 234
            // one numberpackage would be 343
            do
            {
                Field* titlefields = resulttitles->Fetch();
                alliiid = titlefields[0].GetUInt16();
                hordeiid = titlefields[1].GetUInt16();
                action = titlefields[2].GetUInt8();        // action=0 means swap, action=1 means delete

                int oldmaskpos = 0, newmaskpos = 0, oldmaskvalue = 0, newmaskvalue = 0;

                // if change to alliance, look for horde title which has to be changed
                //maskpos is the numberpackage counted from left in which the title is
                //maskvalue is the value inside the numberpackage which this title has
                oldmaskpos = (team == 0) ? hordeiid / 32 : alliiid / 32;
                oldmaskvalue = (team == 0) ? hordeiid % 32 : alliiid % 32;
                newmaskpos = (team == 0) ? alliiid / 32 : hordeiid / 32;
                newmaskvalue = (team == 0) ? alliiid % 32 : hordeiid % 32;


                if ((maskno[oldmaskpos] & (uint64)pow(2.0, (int)oldmaskvalue))) // if fulfilled the char has the title(means this bit is 1) so change or delete
                {
                    // disadd old title
                    maskno[oldmaskpos] = (maskno[oldmaskpos] ^ (uint64)pow(2.0, (int)oldmaskvalue));

                    if (action == 0) // only set new title if action is 0 so swap titles
                                     // readd new converted title
                        maskno[newmaskpos] = (uint64)pow(2.0, (int)newmaskvalue) | maskno[newmaskpos];
                }
            } while (resulttitles->NextRow());

            // save new mask from int to string
            std::ostringstream Str;
            for (int i = 0; i<6; i++)
            {
                Str << maskno[i];
                if (i != 5)
                    Str << " ";
            }
            std::string updatedmask(Str.str());

            //write new mask in db
            PreparedStatement* stmttt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_TITLE_MASK);
            stmttt->setString(0, updatedmask.c_str());
            stmttt->setUInt32(1, lowGuid);
            trans->Append(stmttt);
        }
    }

    CharacterDatabase.CommitTransaction(trans)
            .via(GetExecutor()).then(
            [=](bool success)
            {
                if (!success)
                    return;

                std::string IP_str = GetRemoteAddress();
                TC_LOG_DEBUG("entities.player", "%s (IP: %s) changed race from %u to %u", GetPlayerInfo().c_str(), IP_str.c_str(), oldRace, race);

                if (recvData.GetOpcode() == CMSG_CHAR_FACTION_CHANGE)
                    RG_LOG<PremiumLogModule>(guid, RG::Logging::PremiumLogModule::Action::Apply, CodeType::CharChangeFaction);
                else if (recvData.GetOpcode() == CMSG_CHAR_RACE_CHANGE)
                    RG_LOG<PremiumLogModule>(guid, RG::Logging::PremiumLogModule::Action::Apply, CodeType::CharChangeRace);

                WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1 + 8 + (newname.size() + 1) + 1 + 1 + 1 + 1 + 1 + 1 + 1);
                data << uint8(RESPONSE_SUCCESS);
                data << uint64(guid);
                data << newname;
                data << uint8(gender);
                data << uint8(skin);
                data << uint8(face);
                data << uint8(hairStyle);
                data << uint8(hairColor);
                data << uint8(facialHair);
                data << uint8(race);
                SendPacket(std::move(data));
            });
}
