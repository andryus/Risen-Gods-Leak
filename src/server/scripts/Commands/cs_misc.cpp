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

#include <chrono>
#include <memory>

#include "Chat.h"
#include "Chat/Command/Commands/LegacyCommand.hpp"
#include "Chat/Command/Commands/SimpleCommand.hpp"
#include "Chat/Command/GenericCommandScriptLoader.hpp"
#include "ScriptMgr.h"
#include "AccountMgr.h"
#include "ArenaTeamMgr.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "BattlegroundQueue.h"
#include "GridNotifiers.h"
#include "Group.h"
#include "InstanceSaveMgr.h"
#include "Language.h"
#include "MovementGenerator.h"
#include "Opcodes.h"
#include "SpellAuras.h"
#include "TargetedMovementGenerator.h"
#include "WeatherMgr.h"
#include "Player.h"
#include "Pet.h"
#include "LFG.h"
#include "GroupMgr.h"
#include "MMapFactory.h"
#include "DisableMgr.h"
#include "SpellHistory.h"
#include "WorldPacket.h"
#include "GuildMgr.h"
#include "DBCStores.h"
#include "RG/Logging/LogManager.hpp"
#include "Entities/Player/PlayerCache.hpp"

namespace chat { namespace command { namespace handler {

    bool HandlePvPstatsCommand(ChatHandler * handler, char const* /*args*/)
    {
        if (sWorld->getBoolConfig(CONFIG_BATTLEGROUND_STORE_STATISTICS_ENABLE))
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PVPSTATS_FACTIONS_OVERALL);
            PreparedQueryResult result = CharacterDatabase.Query(stmt);

            if (result)
            {
                Field* fields = result->Fetch();
                uint32 horde_victories = fields[1].GetUInt32();

                if (!(result->NextRow()))
                    return false;

                fields = result->Fetch();
                uint32 alliance_victories = fields[1].GetUInt32();

                handler->PSendSysMessage(LANG_PVPSTATS, alliance_victories, horde_victories);
            }
            else
                return false;
        }
        else
            handler->PSendSysMessage(LANG_PVPSTATS_DISABLED);

        return true;
    }

    bool HandleDevCommand(ChatHandler* handler, const char* args)
    {
        WorldSession& session = *handler->GetSession();
        Player& player = *handler->GetSession()->GetPlayer();

        if (!*args)
        {
            if (player.HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_DEVELOPER))
                session.SendNotification(LANG_DEV_ON);
            else
                session.SendNotification(LANG_DEV_OFF);
            return false;
        }

        const std::string_view argstr = args;

        if (argstr == "on")
        {
            if (!player.isDeveloper())
            {
                player.SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_DEVELOPER);
                session.SendNotification(LANG_DEV_ON);
                player.UpdateTriggerVisibility();
            }
            return true;
        }

        if (argstr == "off")
        {
            if (player.isDeveloper())
            {
                player.RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_DEVELOPER);
                session.SendNotification(LANG_DEV_OFF);
                player.UpdateTriggerVisibility();
            }
            return true;
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    bool HandleGPSCommand(ChatHandler* handler, char const* args)
    {
        WorldObject* object = NULL;
        if (*args)
        {
            ObjectGuid guid = handler->extractGuidFromLink((char*)args);
            if (guid)
                object = (WorldObject*)ObjectAccessor::GetObjectByTypeMask(*handler->GetSession()->GetPlayer(), guid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT);

            if (!object)
            {
                handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
                handler->SetSentErrorMessage(true);
                return false;
            }
        }
        else
        {
            object = handler->getSelectedUnit();

            if (!object)
            {
                handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        Grids::CellCoord cellCoord = Grids::CellCoord::FromPosition(object->GetPositionX(), object->GetPositionY());

        uint32 zoneId, areaId;
        object->GetZoneAndAreaId(zoneId, areaId);
        uint32 mapId = object->GetMapId();

        MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
        AreaTableEntry const* zoneEntry = sAreaTableStore.LookupEntry(zoneId);
        AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(areaId);

        float zoneX = object->GetPositionX();
        float zoneY = object->GetPositionY();

        Map2ZoneCoordinates(zoneX, zoneY, zoneId);

        Map const* map = object->GetMap();
        float groundZ = map->GetHeight(object->GetPhaseMask(), object->GetPositionX(), object->GetPositionY(), MAX_HEIGHT);
        float floorZ = map->GetHeight(object->GetPhaseMask(), object->GetPositionX(), object->GetPositionY(), object->GetPositionZ());

        int gridX = cellCoord.GetInversedGridX();
        int gridY = cellCoord.GetInversedGridY();

        uint32 haveMap = Map::ExistMap(mapId, gridX, gridY) ? 1 : 0;
        uint32 haveVMap = Map::ExistVMap(mapId, gridX, gridY) ? 1 : 0;
        uint32 haveMMap = (DisableMgr::IsPathfindingEnabled(mapId) && MMAP::MMapFactory::createOrGetMMapManager()->GetNavMesh(handler->GetSession()->GetPlayer()->GetMapId())) ? 1 : 0;

        if (haveVMap)
        {
            if (map->IsOutdoors(object->GetPositionX(), object->GetPositionY(), object->GetPositionZ()))
                handler->PSendSysMessage(LANG_GPS_POSITION_OUTDOORS);
            else
                handler->PSendSysMessage(LANG_GPS_POSITION_INDOORS);
        }
        else
            handler->PSendSysMessage(LANG_GPS_NO_VMAP);

        char const* unknown = handler->GetTrinityString(LANG_UNKNOWN);

        handler->PSendSysMessage(LANG_MAP_POSITION,
            mapId, (mapEntry ? mapEntry->name[handler->GetSessionDbcLocale()] : unknown),
            zoneId, (zoneEntry ? zoneEntry->area_name[handler->GetSessionDbcLocale()] : unknown),
            areaId, (areaEntry ? areaEntry->area_name[handler->GetSessionDbcLocale()] : unknown),
            object->GetPhaseMask(),
            object->GetPositionX(), object->GetPositionY(), object->GetPositionZ(), object->GetOrientation(),
            cellCoord.GetGridX(), cellCoord.GetGridY(), cellCoord.GetCellX(), cellCoord.GetCellY(), object->GetInstanceId(),
            zoneX, zoneY, groundZ, floorZ, haveMap, haveVMap, haveMMap);

        LiquidData liquidStatus;
        ZLiquidStatus status = map->getLiquidStatus(object->GetPositionX(), object->GetPositionY(), object->GetPositionZ(), MAP_ALL_LIQUIDS, &liquidStatus);

        if (status)
            handler->PSendSysMessage(LANG_LIQUID_STATUS, liquidStatus.level, liquidStatus.depth_level, liquidStatus.entry, liquidStatus.type_flags, status);

        return true;
    }

    bool HandleAuraCommand(ChatHandler* handler, char const* args)
    {
        Unit* target = handler->getSelectedUnit();
        if (!target)
        {
            handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r or Htalent form
        uint32 spellId = handler->extractSpellIdFromLink((char*)args);

        if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId))
            Aura::TryRefreshStackOrCreate(spellInfo, MAX_EFFECT_MASK, target, target);

        return true;
    }

    bool HandleUnAuraCommand(ChatHandler* handler, char const* args)
    {
        Unit* target = handler->getSelectedUnit();
        if (!target)
        {
            handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string argstr = args;
        if (argstr == "all")
        {
            if (!handler->HasPermission(rbac::RBAC_PERM_RG_COMMAND_UNAURA_ALL))
                return false;

            target->RemoveAllAuras();
            return true;
        }

        // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r or Htalent form
        uint32 spellId = handler->extractSpellIdFromLink((char*)args);
        if (!spellId)
            return false;

        target->RemoveAurasDueToSpell(spellId);

        return true;
    }
            
    // Teleport to Player
    bool HandleAppearCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        ObjectGuid targetGuid;
        std::string targetName;
        if (!handler->extractPlayerTarget((char*)args, &target, &targetGuid, &targetName))
            return false;

        Player* _player = handler->GetSession()->GetPlayer();
        if (target == _player || targetGuid == _player->GetGUID())
        {
            handler->SendSysMessage(LANG_CANT_TELEPORT_SELF);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (target)
        {
            // check online security
            if (handler->HasLowerSecurity(target, ObjectGuid::Empty))
                return false;

            std::string chrNameLink = handler->playerLink(targetName);

            Map* map = target->GetMap();
            if (map->IsBattlegroundOrArena())
            {
                // only allow if gm mode is on
                if (!_player->IsGameMaster())
                {
                    handler->PSendSysMessage(LANG_CANNOT_GO_TO_BG_GM, chrNameLink.c_str());
                    handler->SetSentErrorMessage(true);
                    return false;
                }
                // if both players are in different bgs
                else if (_player->GetBattlegroundId() && _player->GetBattlegroundId() != target->GetBattlegroundId())
                    _player->LeaveBattleground(false); // Note: should be changed so _player gets no Deserter debuff

                                                        // all's well, set bg id
                                                        // when porting out from the bg, it will be reset to 0
                _player->SetBattlegroundId(target->GetBattlegroundId(), target->GetBattlegroundTypeId());
                // remember current position as entry point for return at bg end teleportation
                if (!_player->GetMap()->IsBattlegroundOrArena())
                    _player->SetBattlegroundEntryPoint();
            }
            else if (map->IsDungeon())
            {
                // we have to go to instance, and can go to player only if:
                //   1) we are in his group (either as leader or as member)
                //   2) we are not bound to any group and have GM mode on
                if (_player->GetGroup())
                {
                    // we are in group, we can go only if we are in the player group
                    if (_player->GetGroup() != target->GetGroup())
                    {
                        handler->PSendSysMessage(LANG_CANNOT_GO_TO_INST_PARTY, chrNameLink.c_str());
                        handler->SetSentErrorMessage(true);
                        return false;
                    }
                }
                else
                {
                    // we are not in group, let's verify our GM mode
                    if (!_player->IsGameMaster())
                    {
                        handler->PSendSysMessage(LANG_CANNOT_GO_TO_INST_GM, chrNameLink.c_str());
                        handler->SetSentErrorMessage(true);
                        return false;
                    }
                }

                // if the player or the player's group is bound to another instance
                // the player will not be bound to another one
                InstancePlayerBind* bind = _player->GetBoundInstance(target->GetMapId(), target->GetDifficulty(map->IsRaid()));
                if (!bind)
                {
                    Group* group = _player->GetGroup();
                    // if no bind exists, create a solo bind
                    InstanceGroupBind* gBind = group ? group->GetBoundInstance(target) : NULL;                // if no bind exists, create a solo bind
                    if (!gBind)
                        if (InstanceSave* save = sInstanceSaveMgr->GetInstanceSave(target->GetInstanceId()))
                            _player->BindToInstance(save, !save->CanReset());
                }

                if (map->IsRaid())
                    _player->SetRaidDifficulty(target->GetRaidDifficulty());
                else
                    _player->SetDungeonDifficulty(target->GetDungeonDifficulty());
            }

            handler->PSendSysMessage(LANG_APPEARING_AT, chrNameLink.c_str());

            // stop flight if need
            if (_player->IsInFlight())
            {
                _player->GetMotionMaster()->MovementExpired();
                _player->CleanupAfterTaxiFlight();
            }
            // save only in non-flight case
            else
                _player->SaveRecallPosition();

            // to point to see at target with same orientation
            float x, y, z;
            target->GetContactPoint(_player, x, y, z);

            _player->TeleportTo(target->GetMapId(), x, y, z, _player->GetAngle(target), TELE_TO_GM_MODE);
            _player->SetPhaseMask(target->GetPhaseMask(), true);
        }
        else
        {
            // check offline security
            if (handler->HasLowerSecurity(NULL, targetGuid))
                return false;

            std::string nameLink = handler->playerLink(targetName);

            handler->PSendSysMessage(LANG_APPEARING_AT, nameLink.c_str());

            // to point where player stay (if loaded)
            float x, y, z, o;
            uint32 map;
            bool in_flight;
            if (!Player::LoadPositionFromDB(map, x, y, z, o, in_flight, targetGuid))
                return false;

            // stop flight if need
            if (_player->IsInFlight())
            {
                _player->GetMotionMaster()->MovementExpired();
                _player->CleanupAfterTaxiFlight();
            }
            // save only in non-flight case
            else
                _player->SaveRecallPosition();

            _player->TeleportTo(map, x, y, z, _player->GetOrientation());
        }

        return true;
    }
            
    // Summon Player
    bool HandleSummonCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        ObjectGuid targetGuid;
        std::string targetName;
        if (!handler->extractPlayerTarget((char*)args, &target, &targetGuid, &targetName))
            return false;

        Player* _player = handler->GetSession()->GetPlayer();
        if (target == _player || targetGuid == _player->GetGUID())
        {
            handler->PSendSysMessage(LANG_CANT_TELEPORT_SELF);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (target)
        {
            std::string nameLink = handler->playerLink(targetName);
            // check online security
            if (handler->HasLowerSecurity(target, ObjectGuid::Empty))
                return false;

            if (target->IsBeingTeleported())
            {
                handler->PSendSysMessage(LANG_IS_TELEPORTED, nameLink.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }

            Map* map = handler->GetSession()->GetPlayer()->GetMap();

            if (map->IsBattlegroundOrArena())
            {
                // only allow if gm mode is on
                if (!_player->IsGameMaster())
                {
                    handler->PSendSysMessage(LANG_CANNOT_GO_TO_BG_GM, nameLink.c_str());
                    handler->SetSentErrorMessage(true);
                    return false;
                }
                // if both players are in different bgs
                else if (target->GetBattlegroundId() && handler->GetSession()->GetPlayer()->GetBattlegroundId() != target->GetBattlegroundId())
                    target->LeaveBattleground(false); // Note: should be changed so target gets no Deserter debuff

                // all's well, set bg id
                // when porting out from the bg, it will be reset to 0
                target->SetBattlegroundId(handler->GetSession()->GetPlayer()->GetBattlegroundId(), handler->GetSession()->GetPlayer()->GetBattlegroundTypeId());
                // remember current position as entry point for return at bg end teleportation
                if (!target->GetMap()->IsBattlegroundOrArena())
                    target->SetBattlegroundEntryPoint();
            }
            else if (map->IsDungeon())
            {
                Map* destMap = target->GetMap();

                if (destMap->Instanceable() && destMap->GetInstanceId() != map->GetInstanceId())
                    target->UnbindInstance(map->GetInstanceId(), target->GetDungeonDifficulty(), true);

                // we are in an instance, and can only summon players in our group with us as leader
                if (!handler->GetSession()->GetPlayer()->GetGroup() || !target->GetGroup() ||
                    (target->GetGroup()->GetLeaderGUID() != handler->GetSession()->GetPlayer()->GetGUID()) ||
                    (handler->GetSession()->GetPlayer()->GetGroup()->GetLeaderGUID() != handler->GetSession()->GetPlayer()->GetGUID()))
                    // the last check is a bit excessive, but let it be, just in case
                {
                    handler->PSendSysMessage(LANG_CANNOT_SUMMON_TO_INST, nameLink.c_str());
                    handler->SetSentErrorMessage(true);
                    return false;
                }
            }

            handler->PSendSysMessage(LANG_SUMMONING, nameLink.c_str(), "");
            if (handler->needReportToTarget(target))
                ChatHandler(target->GetSession()).PSendSysMessage(LANG_SUMMONED_BY, handler->playerLink(_player->GetName()).c_str());

            // stop flight if need
            if (target->IsInFlight())
            {
                target->GetMotionMaster()->MovementExpired();
                target->CleanupAfterTaxiFlight();
            }
            // save only in non-flight case
            else
                target->SaveRecallPosition();

            // before GM
            float x, y, z;
            handler->GetSession()->GetPlayer()->GetClosePoint(x, y, z, target->GetObjectSize());
            target->TeleportTo(handler->GetSession()->GetPlayer()->GetMapId(), x, y, z, target->GetOrientation());
            target->SetPhaseMask(handler->GetSession()->GetPlayer()->GetPhaseMask(), true);
        }
        else
        {
            // check offline security
            if (handler->HasLowerSecurity(NULL, targetGuid))
                return false;

            std::string nameLink = handler->playerLink(targetName);

            handler->PSendSysMessage(LANG_SUMMONING, nameLink.c_str(), handler->GetTrinityString(LANG_OFFLINE));

            // in point where GM stay
            auto trans = CharacterDatabase.BeginTransaction();
            Player::SavePositionInDB(trans,
                handler->GetSession()->GetPlayer()->GetMapId(),
                handler->GetSession()->GetPlayer()->GetPositionX(),
                handler->GetSession()->GetPlayer()->GetPositionY(),
                handler->GetSession()->GetPlayer()->GetPositionZ(),
                handler->GetSession()->GetPlayer()->GetOrientation(),
                handler->GetSession()->GetPlayer()->GetZoneId(),
                targetGuid);
            CharacterDatabase.CommitTransaction(trans);
        }

        return true;
    }

    bool HandleDieCommand(ChatHandler* handler, char const* /*args*/)
    {
        Unit* target = handler->getSelectedUnit();

        if (!target || !handler->GetSession()->GetPlayer()->GetTarget())
        {
            handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (Player* player = target->ToPlayer())
            if (handler->HasLowerSecurity(player, ObjectGuid::Empty, false))
                return false;

        if (target->IsAlive())
        {
            if (sWorld->getBoolConfig(CONFIG_DIE_COMMAND_MODE))
                handler->GetSession()->GetPlayer()->Kill(target);
            else
                handler->GetSession()->GetPlayer()->DealDamage(target, target->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
        }

        return true;
    }

    bool HandleReviveCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        ObjectGuid targetGuid;
        if (!handler->extractPlayerTarget((char*)args, &target, &targetGuid))
            return false;

        if (target)
        {
            target->ResurrectPlayer(target->GetSession()->HasPermission(rbac::RBAC_PERM_RESURRECT_WITH_FULL_HPS) ? 1.0f : 0.5f);
            target->SpawnCorpseBones();
            target->SaveToDB();
        }
        else
            // will resurrected at login without corpse
            sObjectAccessor->ConvertCorpseForPlayer(targetGuid);

        return true;
    }

    bool HandleDismountCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player;
        if (handler->getSelectedUnit()->GetTypeId() == TYPEID_PLAYER)
            player = handler->getSelectedUnit()->ToPlayer();
        else
            player = handler->GetSession()->GetPlayer();

        // If player is not mounted, so go out :)
        if (!player->IsMounted())
        {
            handler->SendSysMessage(LANG_CHAR_NON_MOUNTED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (player->IsInFlight())
        {
            handler->SendSysMessage(LANG_YOU_IN_FLIGHT);
            handler->SetSentErrorMessage(true);
            return false;
        }

        player->Dismount();
        player->RemoveAurasByType(SPELL_AURA_MOUNTED);
        return true;
    }

    bool HandleGUIDCommand(ChatHandler* handler, char const* /*args*/)
    {
        ObjectGuid guid = handler->GetSession()->GetPlayer()->GetTarget();

        if (guid.IsEmpty())
        {
            handler->SendSysMessage(LANG_NO_SELECTION);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 dbguid = 0;
        if (Creature* creature = handler->getSelectedCreature())
            dbguid = creature->GetSpawnId();

        handler->PSendSysMessage(LANG_OBJECT_GUID, guid.GetCounter(), static_cast<uint32>(guid.GetHigh()), dbguid);
        return true;
    }

    // move item to other slot
    bool HandleItemMoveCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char const* param1 = strtok((char*)args, " ");
        if (!param1)
            return false;

        char const* param2 = strtok(NULL, " ");
        if (!param2)
            return false;

        uint8 srcSlot = uint8(atoi(param1));
        uint8 dstSlot = uint8(atoi(param2));

        if (srcSlot == dstSlot)
            return true;

        if (!handler->GetSession()->GetPlayer()->IsValidPos(INVENTORY_SLOT_BAG_0, srcSlot, true))
            return false;

        if (!handler->GetSession()->GetPlayer()->IsValidPos(INVENTORY_SLOT_BAG_0, dstSlot, false))
            return false;

        uint16 src = ((INVENTORY_SLOT_BAG_0 << 8) | srcSlot);
        uint16 dst = ((INVENTORY_SLOT_BAG_0 << 8) | dstSlot);

        handler->GetSession()->GetPlayer()->SwapItem(src, dst);

        return true;
    }

    bool HandleCooldownCommand(ChatHandler* handler, char const* args)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string nameLink = handler->GetNameLink(target);

        if (!*args)
        {
            target->GetSpellHistory()->ResetAllCooldowns();
            handler->PSendSysMessage(LANG_REMOVEALL_COOLDOWN, nameLink.c_str());
        }
        else
        {
            // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r or Htalent form
            uint32 spellIid = handler->extractSpellIdFromLink((char*)args);
            if (!spellIid)
                return false;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellIid);
            if (!spellInfo)
            {
                handler->PSendSysMessage(LANG_UNKNOWN_SPELL, target == handler->GetSession()->GetPlayer() ? handler->GetTrinityString(LANG_YOU) : nameLink.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }

            target->GetSpellHistory()->ResetCooldown(spellIid, true);
            handler->PSendSysMessage(LANG_REMOVE_COOLDOWN, spellIid, target == handler->GetSession()->GetPlayer() ? handler->GetTrinityString(LANG_YOU) : nameLink.c_str());
        }
        return true;
    }

    bool HandleGetDistanceCommand(ChatHandler* handler, char const* args)
    {
        WorldObject* obj = NULL;

        if (*args)
        {
            ObjectGuid guid = handler->extractGuidFromLink((char*)args);
            if (guid)
                obj = (WorldObject*)ObjectAccessor::GetObjectByTypeMask(*handler->GetSession()->GetPlayer(), guid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT);

            if (!obj)
            {
                handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
                handler->SetSentErrorMessage(true);
                return false;
            }
        }
        else
        {
            obj = handler->getSelectedUnit();

            if (!obj)
            {
                handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        handler->PSendSysMessage(LANG_DISTANCE, handler->GetSession()->GetPlayer()->GetDistance(obj), handler->GetSession()->GetPlayer()->GetDistance2d(obj), handler->GetSession()->GetPlayer()->GetExactDist(obj), handler->GetSession()->GetPlayer()->GetExactDist2d(obj));
        return true;
    }
    // Teleport player to last position
    bool HandleRecallCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        if (!handler->extractPlayerTarget((char*)args, &target))
            return false;

        // check online security
        if (handler->HasLowerSecurity(target, ObjectGuid::Empty))
            return false;

        if (target->IsBeingTeleported())
        {
            handler->PSendSysMessage(LANG_IS_TELEPORTED, handler->GetNameLink(target).c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        // stop flight if need
        if (target->IsInFlight())
        {
            target->GetMotionMaster()->MovementExpired();
            target->CleanupAfterTaxiFlight();
        }

        target->SetPhaseMask(target->m_recallPhase, true);
        target->TeleportTo(target->m_recallMap, target->m_recallX, target->m_recallY, target->m_recallZ, target->m_recallO);
        return true;
    }

    bool HandleSaveCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();

        // save GM account without delay and output message
        if (handler->GetSession()->HasPermission(rbac::RBAC_PERM_COMMANDS_SAVE_WITHOUT_DELAY))
        {
            if (Player* target = handler->getSelectedPlayer())
                target->SaveToDB();
            else
                player->SaveToDB();
            handler->SendSysMessage(LANG_PLAYER_SAVED);
            return true;
        }

        // save if the player has last been saved over 20 seconds ago
        uint32 saveInterval = sWorld->getIntConfig(CONFIG_INTERVAL_SAVE);
        if (saveInterval == 0 || (saveInterval > 20 * IN_MILLISECONDS && player->GetSaveTimer() <= saveInterval - 20 * IN_MILLISECONDS))
            player->SaveToDB();

        return true;
    }

    // Save all players in the world
    bool HandleSaveAllCommand(ChatHandler* handler, char const* /*args*/)
    {
        sObjectAccessor->SaveAllPlayers();
        handler->SendSysMessage(LANG_PLAYERS_SAVED);
        return true;
    }

    // kick player
    bool HandleKickPlayerCommand(ChatHandler* handler, char const* args)
    {
        Player* target = NULL;
        std::string playerName;
        if (!handler->extractPlayerTarget((char*)args, &target, NULL, &playerName))
            return false;

        if (handler->GetSession() && target == handler->GetSession()->GetPlayer())
        {
            handler->SendSysMessage(LANG_COMMAND_KICKSELF);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // check online security
        if (handler->HasLowerSecurity(target, ObjectGuid::Empty))
            return false;

        std::string kickReasonStr = handler->GetTrinityString(LANG_NO_REASON);
        if (*args != '\0')
        {
            char const* kickReason = strtok(NULL, "\r");
            if (kickReason != NULL)
                kickReasonStr = kickReason;
        }

        if (sWorld->getBoolConfig(CONFIG_SHOW_KICK_IN_WORLD))
            sWorld->SendWorldText(LANG_COMMAND_KICKMESSAGE_WORLD, (handler->GetSession() ? handler->GetSession()->GetPlayerName().c_str() : "Server"), playerName.c_str(), kickReasonStr.c_str());
        else
            handler->PSendSysMessage(LANG_COMMAND_KICKMESSAGE, playerName.c_str());

        target->GetSession()->KickPlayer();

        return true;
    }

    bool HandleUnstuckCommand(ChatHandler* handler, char const* args)
    {
        // No args required for players
        if (handler->GetSession() && !handler->GetSession()->HasPermission(rbac::RBAC_PERM_COMMANDS_USE_UNSTUCK_WITH_ARGS))
        {
            // 7355: "Stuck"
            if (Player* player = handler->GetSession()->GetPlayer())
                player->CastSpell(player, 7355, false);
            return true;
        }

        if (!*args)
            return false;

        char* player_str = strtok((char*)args, " ");
        if (!player_str)
            return false;

        std::string location_str = "inn";
        if (char const* loc = strtok(NULL, " "))
            location_str = loc;

        Player* player = NULL;
        if (!handler->extractPlayerTarget(player_str, &player))
            return false;

        if (player->IsInFlight() || player->IsInCombat())
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(7355);
            if (!spellInfo)
                return false;

            if (Player* caster = handler->GetSession()->GetPlayer())
                Spell::SendCastResult(caster, spellInfo, 0, SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW);

            return false;
        }

        if (location_str == "inn")
        {
            player->TeleportTo(player->m_homebindMapId, player->m_homebindX, player->m_homebindY, player->m_homebindZ, player->GetOrientation());
            return true;
        }

        if (location_str == "graveyard")
        {
            player->RepopAtGraveyard();
            return true;
        }

        if (location_str == "startzone")
        {
            player->TeleportTo(player->GetStartPosition());
            return true;
        }

        //Not a supported argument
        return false;

    }

    bool HandleLinkGraveCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* px = strtok((char*)args, " ");
        if (!px)
            return false;

        uint32 graveyardId = uint32(atoi(px));

        uint32 team;

        char* px2 = strtok(NULL, " ");

        if (!px2)
            team = 0;
        else if (strncmp(px2, "horde", 6) == 0)
            team = HORDE;
        else if (strncmp(px2, "alliance", 9) == 0)
            team = ALLIANCE;
        else
            return false;

        WorldSafeLocsEntry const* graveyard = sWorldSafeLocsStore.LookupEntry(graveyardId);

        if (!graveyard)
        {
            handler->PSendSysMessage(LANG_COMMAND_GRAVEYARDNOEXIST, graveyardId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();

        uint32 zoneId = player->GetZoneId();

        AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(zoneId);
        if (!areaEntry || areaEntry->zone != 0)
        {
            handler->PSendSysMessage(LANG_COMMAND_GRAVEYARDWRONGZONE, graveyardId, zoneId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (sObjectMgr->AddGraveYardLink(graveyardId, zoneId, team))
            handler->PSendSysMessage(LANG_COMMAND_GRAVEYARDLINKED, graveyardId, zoneId);
        else
            handler->PSendSysMessage(LANG_COMMAND_GRAVEYARDALRLINKED, graveyardId, zoneId);

        return true;
    }

    bool HandleNearGraveCommand(ChatHandler* handler, char const* args)
    {
        uint32 team;

        size_t argStr = strlen(args);

        if (!*args)
            team = 0;
        else if (strncmp((char*)args, "horde", argStr) == 0)
            team = HORDE;
        else if (strncmp((char*)args, "alliance", argStr) == 0)
            team = ALLIANCE;
        else
            return false;

        Player* player = handler->GetSession()->GetPlayer();
        uint32 zone_id = player->GetZoneId();

        WorldSafeLocsEntry const* graveyard = sObjectMgr->GetClosestGraveYard(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetMapId(), team);
        if (graveyard)
        {
            uint32 graveyardId = graveyard->ID;

            GraveYardData const* data = sObjectMgr->FindGraveYardData(graveyardId, zone_id);
            if (!data)
            {
                handler->PSendSysMessage(LANG_COMMAND_GRAVEYARDERROR, graveyardId);
                handler->SetSentErrorMessage(true);
                return false;
            }

            team = data->team;

            std::string team_name = handler->GetTrinityString(LANG_COMMAND_GRAVEYARD_NOTEAM);

            if (team == 0)
                team_name = handler->GetTrinityString(LANG_COMMAND_GRAVEYARD_ANY);
            else if (team == HORDE)
                team_name = handler->GetTrinityString(LANG_COMMAND_GRAVEYARD_HORDE);
            else if (team == ALLIANCE)
                team_name = handler->GetTrinityString(LANG_COMMAND_GRAVEYARD_ALLIANCE);

            handler->PSendSysMessage(LANG_COMMAND_GRAVEYARDNEAREST, graveyardId, team_name.c_str(), zone_id);
        }
        else
        {
            std::string team_name;

            if (team == HORDE)
                team_name = handler->GetTrinityString(LANG_COMMAND_GRAVEYARD_HORDE);
            else if (team == ALLIANCE)
                team_name = handler->GetTrinityString(LANG_COMMAND_GRAVEYARD_ALLIANCE);

            if (!team)
                handler->PSendSysMessage(LANG_COMMAND_ZONENOGRAVEYARDS, zone_id);
            else
                handler->PSendSysMessage(LANG_COMMAND_ZONENOGRAFACTION, zone_id, team_name.c_str());
        }

        return true;
    }

    bool HandleShowAreaCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Player* playerTarget = handler->getSelectedPlayer();
        if (!playerTarget)
        {
            handler->SendSysMessage(LANG_NO_CHAR_SELECTED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        AreaTableEntry const* area = sAreaTableStore.LookupEntry(atoi(args));
        if (!area)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        int32 offset = area->exploreFlag / 32;
        if (offset >= PLAYER_EXPLORED_ZONES_SIZE)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 val = uint32((1 << (area->exploreFlag % 32)));
        uint32 currFields = playerTarget->GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset);
        playerTarget->SetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset, uint32((currFields | val)));

        handler->SendSysMessage(LANG_EXPLORE_AREA);
        return true;
    }

    bool HandleHideAreaCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Player* playerTarget = handler->getSelectedPlayer();
        if (!playerTarget)
        {
            handler->SendSysMessage(LANG_NO_CHAR_SELECTED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        AreaTableEntry const* area = sAreaTableStore.LookupEntry(atoi(args));
        if (!area)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        int32 offset = area->exploreFlag / 32;
        if (offset >= PLAYER_EXPLORED_ZONES_SIZE)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 val = uint32((1 << (area->exploreFlag % 32)));
        uint32 currFields = playerTarget->GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset);
        playerTarget->SetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset, uint32((currFields ^ val)));

        handler->SendSysMessage(LANG_UNEXPLORE_AREA);
        return true;
    }

    bool HandleAddItemCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 itemId = 0;

        if (args[0] == '[')                                        // [name] manual form
        {
            char const* itemNameStr = strtok((char*)args, "]");

            if (itemNameStr && itemNameStr[0])
            {
                std::string itemName = itemNameStr + 1;
                WorldDatabase.EscapeString(itemName);

                PreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_ITEM_TEMPLATE_BY_NAME);
                stmt->setString(0, itemName);
                PreparedQueryResult result = WorldDatabase.Query(stmt);

                if (!result)
                {
                    handler->PSendSysMessage(LANG_COMMAND_COULDNOTFIND, itemNameStr + 1);
                    handler->SetSentErrorMessage(true);
                    return false;
                }
                itemId = result->Fetch()->GetUInt32();
            }
            else
                return false;
        }
        else                                                    // item_id or [name] Shift-click form |color|Hitem:item_id:0:0:0|h[name]|h|r
        {
            char const* id = handler->extractKeyFromLink((char*)args, "Hitem");
            if (!id)
                return false;
            itemId = atoul(id);
        }

        char const* ccount = strtok(NULL, " ");

        int32 count = 1;

        if (ccount)
            count = strtol(ccount, NULL, 10);

        if (count == 0)
            count = 1;

        Player* player = handler->GetSession()->GetPlayer();
        Player* playerTarget = handler->getSelectedPlayer();
        if (!playerTarget)
            playerTarget = player;

        TC_LOG_DEBUG("misc", handler->GetTrinityString(LANG_ADDITEM), itemId, count);

        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
        if (!itemTemplate)
        {
            handler->PSendSysMessage(LANG_COMMAND_ITEMIDINVALID, itemId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // Subtract
        if (count < 0)
        {
            uint32 to_remove_count = static_cast<uint32>(-count);
            if (playerTarget->GetItemCount(itemId, false) >= to_remove_count)
            {
                playerTarget->DestroyItemCount(itemId, to_remove_count, true, false);
                handler->PSendSysMessage(LANG_REMOVEITEM, itemId, to_remove_count, handler->GetNameLink(playerTarget).c_str());
                return true;
            }
            else
            {
                handler->PSendSysMessage(LANG_CANNOT_REMOVE_ITEM, itemId, to_remove_count);
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        // Adding items
        uint32 noSpaceForCount = 0;

        // check space and find places
        ItemPosCountVec dest;
        InventoryResult msg = playerTarget->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count, &noSpaceForCount);
        if (msg != EQUIP_ERR_OK)                               // convert to possible store amount
            count -= noSpaceForCount;

        if (count == 0 || dest.empty())                         // can't add any
        {
            handler->PSendSysMessage(LANG_ITEM_CANNOT_CREATE, itemId, noSpaceForCount);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Item* item = playerTarget->StoreNewItem(dest, itemId, true, Item::GenerateItemRandomPropertyId(itemId));
        // RG-Custom ItemLogging
        RG_LOG<ItemLogModule>(item, RG::Logging::ItemLogType::ADDED_BY_GM, count);

        // remove binding (let GM give it to another player later)
        if (player == playerTarget)
            for (ItemPosCountVec::const_iterator itr = dest.begin(); itr != dest.end(); ++itr)
                if (Item* item1 = player->GetItemByPos(itr->pos))
                    item1->SetBinding(false);

        if (count > 0 && item)
        {
            player->SendNewItem(item, count, false, true);
            if (player != playerTarget)
                playerTarget->SendNewItem(item, count, true, false);
        }

        if (noSpaceForCount > 0)
            handler->PSendSysMessage(LANG_ITEM_CANNOT_CREATE, itemId, noSpaceForCount);

        return true;
    }

    bool HandleAddItemSetCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char const* id = handler->extractKeyFromLink((char*)args, "Hitemset"); // number or [name] Shift-click form |color|Hitemset:itemset_id|h[name]|h|r
        if (!id)
            return false;

        uint32 itemSetId = atoul(id);

        // prevent generation all items with itemset field value '0'
        if (itemSetId == 0)
        {
            handler->PSendSysMessage(LANG_NO_ITEMS_FROM_ITEMSET_FOUND, itemSetId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();
        Player* playerTarget = handler->getSelectedPlayer();
        if (!playerTarget)
            playerTarget = player;

        TC_LOG_DEBUG("misc", handler->GetTrinityString(LANG_ADDITEMSET), itemSetId);

        bool found = false;
        ItemTemplateContainer const* its = sObjectMgr->GetItemTemplateStore();
        for (ItemTemplateContainer::const_iterator itr = its->begin(); itr != its->end(); ++itr)
        {
            if (itr->second.ItemSet == itemSetId)
            {
                found = true;
                ItemPosCountVec dest;
                InventoryResult msg = playerTarget->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itr->second.ItemId, 1);
                if (msg == EQUIP_ERR_OK)
                {
                    Item* item = playerTarget->StoreNewItem(dest, itr->second.ItemId, true);
                    // RG-Custom ItemLogging
                    RG_LOG<ItemLogModule>(item, RG::Logging::ItemLogType::ADDED_BY_GM, 1);

                    // remove binding (let GM give it to another player later)
                    if (player == playerTarget)
                        item->SetBinding(false);

                    player->SendNewItem(item, 1, false, true);
                    if (player != playerTarget)
                        playerTarget->SendNewItem(item, 1, true, false);
                }
                else
                {
                    player->SendEquipError(msg, NULL, NULL, itr->second.ItemId);
                    handler->PSendSysMessage(LANG_ITEM_CANNOT_CREATE, itr->second.ItemId, 1);
                }
            }
        }

        if (!found)
        {
            handler->PSendSysMessage(LANG_NO_ITEMS_FROM_ITEMSET_FOUND, itemSetId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }

    bool HandleBankCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->GetSession()->SendShowBank(handler->GetSession()->GetPlayer()->GetGUID());
        return true;
    }

    bool HandleChangeWeather(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        // Weather is OFF
        if (!sWorld->getBoolConfig(CONFIG_WEATHER))
        {
            handler->SendSysMessage(LANG_WEATHER_DISABLED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // *Change the weather of a cell
        char const* px = strtok((char*)args, " ");
        char const* py = strtok(NULL, " ");

        if (!px || !py)
            return false;

        uint32 type = uint32(atoi(px));                         //0 to 3, 0: fine, 1: rain, 2: snow, 3: sand
        float grade = float(atof(py));                          //0 to 1, sending -1 is instand good weather

        Player* player = handler->GetSession()->GetPlayer();
        uint32 zoneid = player->GetZoneId();

        Weather* weather = WeatherMgr::FindWeather(zoneid);

        if (!weather)
            weather = WeatherMgr::AddWeather(zoneid);
        if (!weather)
        {
            handler->SendSysMessage(LANG_NO_WEATHER);
            handler->SetSentErrorMessage(true);
            return false;
        }

        weather->SetWeather(WeatherType(type), grade);

        return true;
    }


    bool HandleMaxSkillCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->getSelectedPlayerOrSelf();
        if (!player)
        {
            handler->SendSysMessage(LANG_NO_CHAR_SELECTED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // each skills that have max skill value dependent from level seted to current level max skill value
        player->UpdateSkillsToMaxSkillsForLevel();
        return true;
    }

    bool HandleSetSkillCommand(ChatHandler* handler, char const* args)
    {
        // number or [name] Shift-click form |color|Hskill:skill_id|h[name]|h|r
        char const* skillStr = handler->extractKeyFromLink((char*)args, "Hskill");
        if (!skillStr)
            return false;

        char const* levelStr = strtok(NULL, " ");
        if (!levelStr)
            return false;

        char const* maxPureSkill = strtok(NULL, " ");

        int32 skill = atoi(skillStr);
        if (skill <= 0)
        {
            handler->PSendSysMessage(LANG_INVALID_SKILL_ID, skill);
            handler->SetSentErrorMessage(true);
            return false;
        }

        int32 level = atol(levelStr);

        Player* target = handler->getSelectedPlayerOrSelf();
        if (!target)
        {
            handler->SendSysMessage(LANG_NO_CHAR_SELECTED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        SkillLineEntry const* skillLine = sSkillLineStore.LookupEntry(skill);
        if (!skillLine)
        {
            handler->PSendSysMessage(LANG_INVALID_SKILL_ID, skill);
            handler->SetSentErrorMessage(true);
            return false;
        }

        bool targetHasSkill = target->GetSkillValue(skill) != 0;

        // If our target does not yet have the skill they are trying to add to them, the chosen level also becomes
        // the max level of the new profession.
        uint16 max = maxPureSkill ? atoul(maxPureSkill) : targetHasSkill ? target->GetPureMaxSkillValue(skill) : uint16(level);

        if (level <= 0 || level > max || max <= 0)
            return false;

        // If the player has the skill, we get the current skill step. If they don't have the skill, we
        // add the skill to the player's book with step 1 (which is the first rank, in most cases something
        // like 'Apprentice <skill>'.
        target->SetSkill(skill, targetHasSkill ? target->GetSkillStep(skill) : 1, level, max);
        handler->PSendSysMessage(LANG_SET_SKILL, skill, skillLine->name[handler->GetSessionDbcLocale()], handler->GetNameLink(target).c_str(), level, max);
        return true;
    }

    // ##################################
    // ## .pinfo
    // ##################################
    struct PlayerInfo
    {
        // Character
        uint32 lowguid              = 0;
        bool online                 = false;
        bool gamemaster             = false;
        uint32 totalPlayerTime      = 0;
        bool alive;
        uint32 money                = 0;

        // Account
        std::string accountName;
        std::string eMail;
        std::string regMail;
        uint32 security             = 0;
        std::string lastIp;
        uint8 locked                = 0;
        std::string lastLogin;
        uint32 failedLogins         = 0;
        uint32 latency              = 0;
        std::string OS;

        // Mute
        std::chrono::seconds muteTimeLeft;

        // Ban
        int64 banTime               = -1;
        std::string banType;
        std::string banReason;
        std::string banAuthor;

        // Position
        uint32 mapId                = 0;
        uint32 areaId               = 0;
        uint32 phase                = 0;
    };

    void PInfoOutputData(WorldSession& requester, std::shared_ptr<PlayerInfo> info)
    {
        ChatHandler handler(&requester);
        auto locale = handler.GetSessionDbcLocale();

        uint32 account;
        uint32 guild;
        uint8  race;
        uint8  class_;
        uint8  gender;
        uint8  level;
        std::string nameLink;

        {
            auto cacheEntry = player::PlayerCache::Get(ObjectGuid(HighGuid::Player, info->lowguid));

            account = cacheEntry.account;
            guild   = cacheEntry.guild;
            race    = cacheEntry.race;
            class_  = cacheEntry.class_;
            gender  = cacheEntry.gender;
            level   = cacheEntry.level;
            // Creates a chat link to the character. Returns nameLink
            nameLink = handler.playerLink(cacheEntry.name);
        }

        std::string guildName;
        if (guild)
            guildName = sGuildMgr->GetGuildNameById(guild);


        // Initiate output
        // Output I. LANG_PINFO_PLAYER
        handler.PSendSysMessage(LANG_PINFO_PLAYER, info->online ? "" : handler.GetTrinityString(LANG_OFFLINE), nameLink.c_str(), info->lowguid);

        // Output II. LANG_PINFO_GM_ACTIVE if character is gamemaster
        if (info->online && info->gamemaster)
            handler.PSendSysMessage(LANG_PINFO_GM_ACTIVE);

        // Output III. LANG_PINFO_BANNED if ban exists and is applied
        if (info->banTime >= 0)
            handler.PSendSysMessage(LANG_PINFO_BANNED, info->banType.c_str(), info->banReason.c_str(), info->banTime > 0 ? secsToTimeString(info->banTime - time(NULL), true).c_str() : handler.GetTrinityString(LANG_PERMANENTLY), info->banAuthor.c_str());

        // Output IV. LANG_PINFO_MUTED if mute is applied
        if (info->muteTimeLeft > std::chrono::seconds::zero())
            handler.PSendSysMessage(LANG_PINFO_MUTED, FormatDuration(info->muteTimeLeft));

        // Output V. LANG_PINFO_ACC_ACCOUNT
        handler.PSendSysMessage(LANG_PINFO_ACC_ACCOUNT, info->accountName.c_str(), account, info->security);

        // Output VI. LANG_PINFO_ACC_LASTLOGIN
        handler.PSendSysMessage(LANG_PINFO_ACC_LASTLOGIN, info->lastLogin.c_str(), info->failedLogins);

        // Output VII. LANG_PINFO_ACC_OS
        handler.PSendSysMessage(LANG_PINFO_ACC_OS, info->OS.c_str(), info->latency);

        // Output VIII. LANG_PINFO_ACC_REGMAILS
        handler.PSendSysMessage(LANG_PINFO_ACC_REGMAILS, info->regMail.c_str(), info->eMail.c_str());

        // Output IX. LANG_PINFO_ACC_IP
        handler.PSendSysMessage(LANG_PINFO_ACC_IP, info->lastIp.c_str(), info->locked ? handler.GetTrinityString(LANG_YES) : handler.GetTrinityString(LANG_NO));

        // Output X. LANG_PINFO_CHR_LEVEL
        handler.PSendSysMessage(LANG_PINFO_CHR_LEVEL_HIGH, level);

        // Output XI. LANG_PINFO_CHR_RACE
        handler.PSendSysMessage(
            LANG_PINFO_CHR_RACE,
            (gender == 0 ? handler.GetTrinityString(LANG_CHARACTER_GENDER_MALE) : handler.GetTrinityString(LANG_CHARACTER_GENDER_FEMALE)),
            GetRaceName(race, locale),
            GetClassName(class_, locale)
        );

        // Output XII. LANG_PINFO_CHR_ALIVE
        handler.PSendSysMessage(LANG_PINFO_CHR_ALIVE, handler.GetTrinityString(info->alive ? LANG_YES : LANG_NO));

        // Output XIII. LANG_PINFO_CHR_PHASE if player is not in GM mode (GM is in every phase)
        if (info->online && !info->gamemaster)
            handler.PSendSysMessage(LANG_PINFO_CHR_PHASE, info->phase);

        // Output XIV. LANG_PINFO_CHR_MONEY
        uint32 gold = info->money / GOLD;
        uint32 silver = (info->money % GOLD) / SILVER;
        uint32 copper = (info->money % GOLD) % SILVER;
        handler.PSendSysMessage(LANG_PINFO_CHR_MONEY, gold, silver, copper);

        // Position data
        MapEntry const* map = sMapStore.LookupEntry(info->mapId);
        AreaTableEntry const* area = sAreaTableStore.LookupEntry(info->areaId);
        std::string areaName;
        std::string zoneName;
        if (area)
        {
            areaName = area->area_name[locale];

            AreaTableEntry const* zone = sAreaTableStore.LookupEntry(area->zone);
            if (zone)
                zoneName = zone->area_name[locale];
        }

        handler.PSendSysMessage(
            LANG_PINFO_CHR_MAP, map->name[locale],
            (!zoneName.empty()) ? zoneName.c_str() : handler.GetTrinityString(LANG_UNKNOWN),
            (!areaName.empty()) ? areaName.c_str() : handler.GetTrinityString(LANG_UNKNOWN)
        );

        // Output XVII. - XVIX. if they are not empty
        if (!guildName.empty())
            handler.PSendSysMessage(LANG_PINFO_CHR_GUILD, guildName.c_str(), guild);

        // Output XX. LANG_PINFO_CHR_PLAYEDTIME
        handler.PSendSysMessage(LANG_PINFO_CHR_PLAYEDTIME, (secsToTimeString(info->totalPlayerTime, true, true)).c_str());
    }

    void PInfoFillAccountData(WorldSession& requester, std::shared_ptr<PlayerInfo> info)
    {
        // Query the prepared statement for login data
        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_PINFO);
        stmt->setInt32(0, int32(realm.Id.Realm));
        stmt->setUInt32(1, player::PlayerCache::GetAccountId(ObjectGuid(HighGuid::Player, info->lowguid)));

        requester.SetCommandExecuting(true);
        LoginDatabase.AsyncQuery(stmt)
            .via(requester.GetExecutor())
            .then([&requester, info](PreparedQueryResult result)
            {
                requester.SetCommandExecuting(false);
                if (!result)
                    return;

                if (!requester.GetPlayer()) // the requesting player has logged out in the meantime
                    return;

                ChatHandler handler(&requester);


                Field* fields = result->Fetch();

                info->accountName = fields[0].GetString();
                info->security    = fields[1].GetUInt8();

                // Only fetch these fields if commander has sufficient rights)
                if (handler.HasPermission(rbac::RBAC_PERM_COMMANDS_PINFO_CHECK_PERSONAL_DATA) && (requester.GetSecurity() >= info->security))
                {
                    info->eMail = fields[2].GetString();
                    info->regMail = fields[3].GetString();
                    info->lastIp = fields[4].GetString();
                    info->lastLogin = fields[5].GetString();
                }
                else
                {
                    info->eMail = handler.GetTrinityString(LANG_UNAUTHORIZED);
                    info->regMail = handler.GetTrinityString(LANG_UNAUTHORIZED);
                    info->lastIp = handler.GetTrinityString(LANG_UNAUTHORIZED);
                    info->lastLogin = handler.GetTrinityString(LANG_UNAUTHORIZED);
                }

                info->muteTimeLeft = std::chrono::seconds{ fields[6].GetUInt64() };
                info->failedLogins = fields[7].GetUInt32();
                info->locked       = fields[8].GetUInt8();
                info->OS           = fields[9].GetString();

                // field 10 = ban time
                // field 11 = boolean: permanent ban
                if (!fields[10].IsNull())
                {
                    info->banTime = static_cast<int64>(fields[11].GetBool() ? 0 : fields[10].GetUInt32());
                    info->banAuthor = fields[12].GetString();
                    info->banReason = fields[13].GetString();
                }

                PInfoOutputData(requester, info);
            });
    }

    void PInfoFillOnlineCharData(WorldSession& requester, Player& player, std::shared_ptr<PlayerInfo> info)
    {
        info->online = true;
        info->alive = player.IsAlive();
        info->gamemaster = player.IsGameMaster();
        info->latency = player.GetSession()->GetLatency();
        info->money = player.GetMoney();
        info->totalPlayerTime = player.GetTotalPlayedTime();
        info->muteTimeLeft = std::chrono::duration_cast<std::chrono::seconds>(player.GetSession()->GetMuteMgr().GetMuteTimeLeft());

        info->mapId = player.GetMapId();
        info->areaId = player.GetAreaId();
        info->phase = player.GetPhaseMask();

        PInfoFillAccountData(requester, info);
    }

    void PInfoFillOfflineCharData(WorldSession& requester, uint32 playerGuid, std::shared_ptr<PlayerInfo> info)
    {
        requester.SetCommandExecuting(true);
        // Query informations from the DB
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_PINFO);
        stmt->setUInt32(0, info->lowguid);
        CharacterDatabase.AsyncQuery(stmt)
            .via(requester.GetExecutor())
            .then([&requester, info](PreparedQueryResult result)
            {
                requester.SetCommandExecuting(false);
                if (!result)
                    return;

                if (!requester.GetPlayer()) // the requesting player has logged out in the meantime
                    return;

                ChatHandler handler(&requester);

                Field* fields = result->Fetch();

                info->totalPlayerTime = fields[0].GetUInt32();
                //level               = fields[1].GetUInt8();
                info->money           = fields[2].GetUInt32();
                //accId               = fields[3].GetUInt32();
                //raceid              = fields[4].GetUInt8();
                //classid             = fields[5].GetUInt8();
                info->mapId           = fields[6].GetUInt16();
                info->areaId          = fields[7].GetUInt16();
                //gender              = fields[8].GetUInt8();

                uint32 health         = fields[9].GetUInt32();
                uint32 playerFlags    = fields[10].GetUInt32();

                info->alive = health && !(playerFlags & PLAYER_FLAGS_GHOST);

                PInfoFillAccountData(requester, info);
            });
    }

    /**
    * @name Player command: .pinfo
    * @date 05/19/2013
    *
    * @brief Prints information about a character and it's linked account to the commander
    *
    * Non-applying information, e.g. a character that is not in gm mode right now or
    * that is not banned/muted, is not printed
    *
    * This can be done either by giving a name or by targeting someone, else, it'll use the commander
    *
    * @param args name   Prints information according to the given name to the commander
    *             target Prints information on the target to the commander
    *             none   No given args results in printing information on the commander
    *
    * @return Several pieces of information about the character and the account
    **/

    bool HandlePInfoCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        ObjectGuid targetGuid;

        // To make sure we get a target, we convert our guid to an omniversal...
        ObjectGuid parseGUID = ObjectGuid((uint64)atoull((char*)args));

        // ... and make sure we get a target, somehow.
        if (player::PlayerCache::Exists(parseGUID))
        {
            target = ObjectAccessor::FindConnectedPlayer(parseGUID);
            targetGuid = parseGUID;
        }
        // if not, then return false. Which shouldn't happen, now should it ?
        else if (!handler->extractPlayerTarget((char*)args, &target, &targetGuid, NULL))
            return false;

        if (handler->HasLowerSecurity(target, targetGuid))
            return false;

        /* The variables we extract for the command. They are
            * default as "does not exist" to prevent problems
            * The output is printed in the follow manner:
            *
            * Player %s %s (guid: %u)                   - I.    LANG_PINFO_PLAYER
            * ** GM Mode active, Phase: -1              - II.   LANG_PINFO_GM_ACTIVE (if GM)
            * ** Banned: (Type, Reason, Time, By)       - III.  LANG_PINFO_BANNED (if banned)
            * ** Muted for: Time                        - IV.   LANG_PINFO_MUTED (if muted)
            * * Account: %s (id: %u), GM Level: %u      - V.    LANG_PINFO_ACC_ACCOUNT
            * * Last Login: %u (Failed Logins: %u)      - VI.   LANG_PINFO_ACC_LASTLOGIN
            * * Uses OS: %s - Latency: %u ms            - VII.  LANG_PINFO_ACC_OS
            * * Registration Email: %s - Email: %s      - VIII. LANG_PINFO_ACC_REGMAILS
            * * Last IP: %u (Locked: %s)                - IX.   LANG_PINFO_ACC_IP
            * * Level: %u (%u/%u XP (%u XP left)        - X.    LANG_PINFO_CHR_LEVEL
            * * Race: %s %s, Class %s                   - XI.   LANG_PINFO_CHR_RACE
            * * Alive ?: %s                             - XII.  LANG_PINFO_CHR_ALIVE
            * * Phase: %s                               - XIII. LANG_PINFO_CHR_PHASE (if not GM)
            * * Money: %ug%us%uc                        - XIV.  LANG_PINFO_CHR_MONEY
            * * Map: %s, Area: %s                       - XV.   LANG_PINFO_CHR_MAP
            * * Guild: %s (Id: %u)                      - XVI.  LANG_PINFO_CHR_GUILD (if in guild)
            * * Played time: %s                         - XX.   LANG_PINFO_CHR_PLAYEDTIME
            *
            * Not all of them can be moved to the top. These should
            * place the most important ones to the head, though.
            *
            * For a cleaner overview, I segment each output in Roman numerals
            */

        auto info = std::make_shared<PlayerInfo>();

        info->lowguid = targetGuid.GetCounter();

        if (target)
            PInfoFillOnlineCharData(*handler->GetSession(), *target, info);
        else
            PInfoFillOfflineCharData(*handler->GetSession(), targetGuid.GetCounter(), info);

        return true;
    }

    // ##################################
    // ## END .pinfo
    // ##################################

    bool HandleRespawnCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();

        // accept only explicitly selected target (not implicitly self targeting case)
        Unit* target = handler->getSelectedUnit();
        if (player->GetTarget() && target)
        {
            if (target->GetTypeId() != TYPEID_UNIT || target->IsPet())
            {
                handler->SendSysMessage(LANG_SELECT_CREATURE);
                handler->SetSentErrorMessage(true);
                return false;
            }

            if (target->isDead())
            {
                target->ToCreature()->Respawn();
                return true;
            }

            return false;
        }

        if (handler->GetSession()->HasPermission(rbac::RBAC_PERM_RG_COMMAND_RESPAWN_AREA))
        {
            Trinity::RespawnDo u_do;
            player->VisitAnyNearbyObject<WorldObject, Trinity::FunctionAction>(player->GetGridActivationRange(), u_do);

            return true;
        }

        return false;
    }

    // mute player for the given time
    bool HandleMuteCommand(ChatHandler* handler, char const* args)
    {
        char* nameStr;
        char* delayStr;
        handler->extractOptFirstArg((char*)args, &nameStr, &delayStr);
        if (!delayStr)
            return false;

        char const* muteReason = strtok(NULL, "\r");
        std::string muteReasonStr = muteReason ? muteReason : handler->GetTrinityString(LANG_NO_REASON);

        Player* target;
        ObjectGuid targetGuid;
        std::string targetName;
        if (!handler->extractPlayerTarget(nameStr, &target, &targetGuid, &targetName))
            return false;

        uint32 accountId = target ? target->GetSession()->GetAccountId() : player::PlayerCache::GetAccountId(targetGuid);

        // if the player is online on another character, use that one
        if (!target)
            if (WorldSession* session = sWorld->FindSession(accountId))
                target = session->GetPlayer();

        // must have strong lesser security level
        if (handler->HasLowerSecurity(target, targetGuid, true))
            return false;

        uint32 muteDurationInMinutes = uint32(atoi(delayStr));
        uint32 authorAccountId = handler->GetSession() ? handler->GetSession()->GetAccountId() : 0;
        std::string authorName = handler->GetSession() ? handler->GetSession()->GetPlayerName() : handler->GetTrinityString(LANG_CONSOLE);

        if (target)
        {
            // Target is online, mute will be in effect right away.
            target->GetSession()->GetMuteMgr().Mute(
                std::chrono::minutes{ muteDurationInMinutes },
                authorAccountId,
                muteReasonStr
            );

            // Announce to mute target
            ChatHandler(target->GetSession()).PSendSysMessage(
                LANG_YOUR_CHAT_DISABLED,
                muteDurationInMinutes,
                authorName.c_str(),
                muteReasonStr.c_str()
            );
        }
        else
        {
            // Target is offline, mute will be in effect on next login.
            MuteMgr::MuteOffline(
                accountId,
                std::chrono::minutes{ muteDurationInMinutes },
                authorAccountId,
                muteReasonStr
            );
        }

        // Announce to world / mute author
        if (sWorld->getBoolConfig(CONFIG_SHOW_MUTE_IN_WORLD))
        {
            sWorld->SendWorldText(
                LANG_COMMAND_MUTEMESSAGE_WORLD,
                (handler->GetSession() ? handler->GetSession()->GetPlayerName().c_str() : "Server"),
                handler->playerLink(targetName).c_str(),
                muteDurationInMinutes,
                muteReasonStr.c_str()
            );
        }
        else
        {
            handler->PSendSysMessage(
                target ? LANG_YOU_DISABLE_CHAT : LANG_COMMAND_DISABLE_CHAT_DELAYED,
                handler->playerLink(targetName).c_str(),
                muteDurationInMinutes,
                muteReasonStr.c_str()
            );
        }

        return true;
    }

    // unmute player
    bool HandleUnmuteCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        ObjectGuid targetGuid;
        std::string targetName;
        if (!handler->extractPlayerTarget((char*)args, &target, &targetGuid, &targetName))
            return false;

        uint32 accountId = target ? target->GetSession()->GetAccountId() : player::PlayerCache::GetAccountId(targetGuid);

        // find only player from same account if any
        if (!target)
            if (WorldSession* session = sWorld->FindSession(accountId))
                target = session->GetPlayer();

        // must have strong lesser security level
        if (handler->HasLowerSecurity(target, targetGuid, true))
            return false;

        if (target)
        {
            if (target->CanSpeak())
            {
                handler->SendSysMessage(LANG_CHAT_ALREADY_ENABLED);
                handler->SetSentErrorMessage(true);
                return false;
            }

            target->GetSession()->GetMuteMgr().Unmute();

            ChatHandler(target->GetSession()).PSendSysMessage(LANG_YOUR_CHAT_ENABLED);
        }
        else
        {
            MuteMgr::UnmuteOffline(accountId);
        }

        handler->PSendSysMessage(LANG_YOU_ENABLE_CHAT, handler->playerLink(targetName).c_str());

        return true;
    }

    // mutehistory command
    bool HandleMuteInfoCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char *nameStr = strtok((char*)args, "");
        if (!nameStr)
            return false;

        std::string accountName = nameStr;
        if (!Utf8ToUpperOnlyLatin(accountName))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 accountId = AccountMgr::GetId(accountName);
        if (!accountId)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            return false;
        }

        PreparedStatement* _stmt = RGDatabase.GetPreparedStatement(RG_SEL_MUTES);
        _stmt->setUInt32(0, accountId);
        PreparedQueryResult result = RGDatabase.Query(_stmt);

        if (!result)
        {
            handler->PSendSysMessage(LANG_COMMAND_MUTEHISTORY_EMPTY, accountName);
            return true;
        }

        handler->PSendSysMessage(LANG_COMMAND_MUTEHISTORY, accountName);
        do
        {
            Field* fields = result->Fetch();
            handler->PSendSysMessage(
                LANG_COMMAND_MUTEHISTORY_OUTPUT,
                fields[0].GetCString(),
                FormatDuration(fields[1].GetUInt32(), true),
                fields[3].GetCString(),
                std::to_string(fields[2].GetUInt32()).c_str()
            );
        } while (result->NextRow());
        return true;
    }

    bool HandleMovegensCommand(ChatHandler* handler, char const* /*args*/)
    {
        Unit* unit = handler->getSelectedUnit();
        if (!unit)
        {
            handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage(LANG_MOVEGENS_LIST, (unit->GetTypeId() == TYPEID_PLAYER ? "Player" : "Creature"), unit->GetGUID().GetCounter());

        MotionMaster* motionMaster = unit->GetMotionMaster();
        float x, y, z;
        motionMaster->GetDestination(x, y, z);

        for (uint8 i = 0; i < MAX_MOTION_SLOT; ++i)
        {
            MovementGenerator* movementGenerator = motionMaster->GetMotionSlot(i);
            if (!movementGenerator)
            {
                handler->SendSysMessage("Empty");
                continue;
            }

            switch (movementGenerator->GetMovementGeneratorType())
            {
            case IDLE_MOTION_TYPE:
                handler->SendSysMessage(LANG_MOVEGENS_IDLE);
                break;
            case RANDOM_MOTION_TYPE:
                handler->SendSysMessage(LANG_MOVEGENS_RANDOM);
                break;
            case WAYPOINT_MOTION_TYPE:
                handler->SendSysMessage(LANG_MOVEGENS_WAYPOINT);
                break;
            case ANIMAL_RANDOM_MOTION_TYPE:
                handler->SendSysMessage(LANG_MOVEGENS_ANIMAL_RANDOM);
                break;
            case CONFUSED_MOTION_TYPE:
                handler->SendSysMessage(LANG_MOVEGENS_CONFUSED);
                break;
            case CHASE_MOTION_TYPE:
            {
                Unit* target = NULL;
                if (unit->GetTypeId() == TYPEID_PLAYER)
                    target = static_cast<ChaseMovementGenerator<Player> const*>(movementGenerator)->GetTarget();
                else
                    target = static_cast<ChaseMovementGenerator<Creature> const*>(movementGenerator)->GetTarget();

                if (!target)
                    handler->SendSysMessage(LANG_MOVEGENS_CHASE_NULL);
                else if (target->GetTypeId() == TYPEID_PLAYER)
                    handler->PSendSysMessage(LANG_MOVEGENS_CHASE_PLAYER, target->GetName().c_str(), target->GetGUID().GetCounter());
                else
                    handler->PSendSysMessage(LANG_MOVEGENS_CHASE_CREATURE, target->GetName().c_str(), target->GetGUID().GetCounter());
                break;
            }
            case FOLLOW_MOTION_TYPE:
            {
                Unit* target = NULL;
                if (unit->GetTypeId() == TYPEID_PLAYER)
                    target = static_cast<FollowMovementGenerator<Player> const*>(movementGenerator)->GetTarget();
                else
                    target = static_cast<FollowMovementGenerator<Creature> const*>(movementGenerator)->GetTarget();

                if (!target)
                    handler->SendSysMessage(LANG_MOVEGENS_FOLLOW_NULL);
                else if (target->GetTypeId() == TYPEID_PLAYER)
                    handler->PSendSysMessage(LANG_MOVEGENS_FOLLOW_PLAYER, target->GetName().c_str(), target->GetGUID().GetCounter());
                else
                    handler->PSendSysMessage(LANG_MOVEGENS_FOLLOW_CREATURE, target->GetName().c_str(), target->GetGUID().GetCounter());
                break;
            }
            case HOME_MOTION_TYPE:
            {
                if (unit->GetTypeId() == TYPEID_UNIT)
                    handler->PSendSysMessage(LANG_MOVEGENS_HOME_CREATURE, x, y, z);
                else
                    handler->SendSysMessage(LANG_MOVEGENS_HOME_PLAYER);
                break;
            }
            case FLIGHT_MOTION_TYPE:
                handler->SendSysMessage(LANG_MOVEGENS_FLIGHT);
                break;
            case POINT_MOTION_TYPE:
            {
                handler->PSendSysMessage(LANG_MOVEGENS_POINT, x, y, z);
                break;
            }
            case FLEEING_MOTION_TYPE:
                handler->SendSysMessage(LANG_MOVEGENS_FEAR);
                break;
            case DISTRACT_MOTION_TYPE:
                handler->SendSysMessage(LANG_MOVEGENS_DISTRACT);
                break;
            case EFFECT_MOTION_TYPE:
                handler->SendSysMessage(LANG_MOVEGENS_EFFECT);
                break;
            default:
                handler->PSendSysMessage(LANG_MOVEGENS_UNKNOWN, movementGenerator->GetMovementGeneratorType());
                break;
            }
        }
        return true;
    }

    bool HandleComeToMeCommand(ChatHandler* handler, char const* /*args*/)
    {
        Creature* caster = handler->getSelectedCreature();
        if (!caster)
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();

        caster->GetMotionMaster()->MovePoint(0, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ());

        return true;
    }

    bool HandleDamageCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* str = strtok((char*)args, " ");

        if (strcmp(str, "go") == 0)
        {
            char* guidStr = strtok(NULL, " ");
            if (!guidStr)
            {
                handler->SendSysMessage(LANG_BAD_VALUE);
                handler->SetSentErrorMessage(true);
                return false;
            }

            int32 guid = atoi(guidStr);
            if (!guid)
            {
                handler->SendSysMessage(LANG_BAD_VALUE);
                handler->SetSentErrorMessage(true);
                return false;
            }

            char* damageStr = strtok(NULL, " ");
            if (!damageStr)
            {
                handler->SendSysMessage(LANG_BAD_VALUE);
                handler->SetSentErrorMessage(true);
                return false;
            }

            int32 damage = atoi(damageStr);
            if (!damage)
            {
                handler->SendSysMessage(LANG_BAD_VALUE);
                handler->SetSentErrorMessage(true);
                return false;
            }

            if (Player* player = handler->GetSession()->GetPlayer())
            {
                GameObject* go = NULL;

                if (GameObjectData const* goData = sObjectMgr->GetGOData(guid))
                    go = handler->GetObjectGlobalyWithGuidOrNearWithDbGuid(guid, goData->id);

                if (!go)
                {
                    handler->PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, guid);
                    handler->SetSentErrorMessage(true);
                    return false;
                }

                if (!go->IsDestructibleBuilding())
                {
                    handler->SendSysMessage(LANG_INVALID_GAMEOBJECT_TYPE);
                    handler->SetSentErrorMessage(true);
                    return false;
                }

                go->ModifyHealth(-damage, player);
                handler->PSendSysMessage(LANG_GAMEOBJECT_DAMAGED, go->GetName().c_str(), guid, -damage, go->GetGOValue()->Building.Health);
            }

            return true;
        }

        Unit* target = handler->getSelectedUnit();
        if (!target || !handler->GetSession()->GetPlayer()->GetTarget())
        {
            handler->SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (Player* player = target->ToPlayer())
            if (handler->HasLowerSecurity(player, ObjectGuid::Empty, false))
                return false;

        if (!target->IsAlive())
            return true;

        char* damageStr = strtok((char*)args, " ");
        if (!damageStr)
            return false;

        int32 damage_int = atoi((char*)damageStr);
        if (damage_int <= 0)
            return true;

        uint32 damage = damage_int;

        char* schoolStr = strtok((char*)NULL, " ");

        // flat melee damage without resistence/etc reduction
        if (!schoolStr)
        {
            handler->GetSession()->GetPlayer()->DealDamage(target, damage, NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
            if (target != handler->GetSession()->GetPlayer())
                handler->GetSession()->GetPlayer()->SendAttackStateUpdate(HITINFO_AFFECTS_VICTIM, target, 1, SPELL_SCHOOL_MASK_NORMAL, damage, 0, 0, VICTIMSTATE_HIT, 0);
            return true;
        }

        uint32 school = atoi((char*)schoolStr);
        if (school >= MAX_SPELL_SCHOOL)
            return false;

        SpellSchoolMask schoolmask = SpellSchoolMask(1 << school);

        if (Unit::IsDamageReducedByArmor(schoolmask))
            damage = handler->GetSession()->GetPlayer()->CalcArmorReducedDamage(target, damage, NULL, BASE_ATTACK);

        char* spellStr = strtok((char*)NULL, " ");

        // melee damage by specific school
        if (!spellStr)
        {
            uint32 absorb = 0;
            uint32 resist = 0;

            handler->GetSession()->GetPlayer()->CalcAbsorbResist(target, schoolmask, SPELL_DIRECT_DAMAGE, damage, damage, &absorb, &resist);

            if (damage <= absorb + resist)
                return true;

            damage -= absorb + resist;

            handler->GetSession()->GetPlayer()->DealDamageMods(target, damage, &absorb);
            handler->GetSession()->GetPlayer()->DealDamage(target, damage, NULL, DIRECT_DAMAGE, schoolmask, NULL, false);
            handler->GetSession()->GetPlayer()->SendAttackStateUpdate(HITINFO_AFFECTS_VICTIM, target, 1, schoolmask, damage, absorb, resist, VICTIMSTATE_HIT, 0);
            return true;
        }

        // non-melee damage

        // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r or Htalent form
        uint32 spellid = handler->extractSpellIdFromLink((char*)args);
        if (!spellid || !sSpellMgr->GetSpellInfo(spellid))
            return false;

        handler->GetSession()->GetPlayer()->SpellNonMeleeDamageLog(target, spellid, damage);
        return true;
    }

    bool HandleCombatStopCommand(ChatHandler* handler, char const* args)
    {
        Player* target = NULL;

        if (args && args[0] != '\0')
        {
            target = sObjectAccessor->FindPlayerByName(args);
            if (!target)
            {
                handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        if (!target)
        {
            if (!handler->extractPlayerTarget((char*)args, &target))
                return false;
        }

        // check online security
        if (handler->HasLowerSecurity(target, ObjectGuid::Empty))
            return false;

        target->CombatStop();
        target->getHostileRefManager().deleteReferences();
        return true;
    }

    bool HandleFlushArenaPointsCommand(ChatHandler* handler, char const* /*args*/)
    {
        sArenaTeamMgr->DistributeArenaPoints();
        handler->SendSysMessage("Distributed arena points");
        return true;
    }

    bool HandleRepairitemsCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        if (!handler->extractPlayerTarget((char*)args, &target))
            return false;

        // check online security
        if (handler->HasLowerSecurity(target, ObjectGuid::Empty))
            return false;

        // Repair items
        target->DurabilityRepairAll(false, 0, false);

        handler->PSendSysMessage(LANG_YOU_REPAIR_ITEMS, handler->GetNameLink(target).c_str());
        if (handler->needReportToTarget(target))
            ChatHandler(target->GetSession()).PSendSysMessage(LANG_YOUR_ITEMS_REPAIRED, handler->GetNameLink().c_str());

        return true;
    }

    bool HandleFreezeCommand(ChatHandler* handler, char const* args)
    {
        Player* player = handler->getSelectedPlayer(); // Selected player, if any. Might be null.
        uint32 freezeDuration = 0; // Freeze Duration (in seconds)
        bool canApplyFreeze = false; // Determines if every possible argument is set so Freeze can be applied
        bool getDurationFromConfig = false; // If there's no given duration, we'll retrieve the world cfg value later

        /*
            Possible Freeze Command Scenarios:
            case 1 - .freeze (without args and a selected player)
            case 2 - .freeze duration (with a selected player)
            case 3 - .freeze player duration
            case 4 - .freeze player (without specifying duration)
        */

        // case 1: .freeze
        if (!*args)
        {
            // Might have a selected player. We'll check it later
            // Get the duration from world cfg
            getDurationFromConfig = true;
        }
        else
        {
            // Get the args that we might have (up to 2)
            char const* arg1 = strtok((char*)args, " ");
            char const* arg2 = strtok(NULL, " ");

            // Analyze them to see if we got either a playerName or duration or both
            if (arg1)
            {
                if (isNumeric(arg1))
                {
                    // case 2: .freeze duration
                    // We have a selected player. We'll check him later
                    freezeDuration = uint32(atoi(arg1));
                    canApplyFreeze = true;
                }
                else
                {
                    // case 3 or 4: .freeze player duration | .freeze player
                    // find the player
                    std::string name = arg1;
                    normalizePlayerName(name);
                    player = sObjectAccessor->FindPlayerByName(name);
                    // Check if we have duration set
                    if (arg2 && isNumeric(arg2))
                    {
                        freezeDuration = uint32(atoi(arg2));
                        canApplyFreeze = true;
                    }
                    else
                        getDurationFromConfig = true;
                }
            }
        }

        // Check if duration needs to be retrieved from config
        if (getDurationFromConfig)
        {
            freezeDuration = sWorld->getIntConfig(CONFIG_GM_FREEZE_DURATION);
            canApplyFreeze = true;
        }

        // Player and duration retrieval is over
        if (canApplyFreeze)
        {
            if (!player) // can be null if some previous selection failed
            {
                handler->SendSysMessage(LANG_COMMAND_FREEZE_WRONG);
                return true;
            }
            else if (player == handler->GetSession()->GetPlayer())
            {
                // Can't freeze himself
                handler->SendSysMessage(LANG_COMMAND_FREEZE_ERROR);
                return true;
            }
            else // Apply the effect
            {
                // Add the freeze aura and set the proper duration
                // Player combat status and flags are now handled
                // in Freeze Spell AuraScript (OnApply)
                Aura* freeze = player->AddAura(9454, player);
                if (freeze)
                {
                    if (freezeDuration)
                        freeze->SetDuration(freezeDuration * IN_MILLISECONDS);
                    handler->PSendSysMessage(LANG_COMMAND_FREEZE, player->GetName().c_str());
                    // save player
                    player->SaveToDB();
                    return true;
                }
            }
        }
        return false;
    }

    bool HandleUnFreezeCommand(ChatHandler* handler, char const*args)
    {
        std::string name;
        Player* player;
        char* targetName = strtok((char*)args, " "); // Get entered name

        if (targetName)
        {
            name = targetName;
            normalizePlayerName(name);
            player = sObjectAccessor->FindPlayerByName(name);
        }
        else // If no name was entered - use target
        {
            player = handler->getSelectedPlayer();
            if (player)
                name = player->GetName();
        }

        if (player)
        {
            handler->PSendSysMessage(LANG_COMMAND_UNFREEZE, name.c_str());

            // Remove Freeze spell (allowing movement and spells)
            // Player Flags + Neutral faction removal is now
            // handled on the Freeze Spell AuraScript (OnRemove)
            player->RemoveAurasDueToSpell(9454);
        }
        else
        {
            if (targetName)
            {
                // Check for offline players
                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_GUID_BY_NAME);
                stmt->setString(0, name);
                PreparedQueryResult result = CharacterDatabase.Query(stmt);

                if (!result)
                {
                    handler->SendSysMessage(LANG_COMMAND_FREEZE_WRONG);
                    return true;
                }

                // If player found: delete his freeze aura
                Field* fields = result->Fetch();
                uint32 lowGuid = fields[0].GetUInt32();

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_AURA_FROZEN);
                stmt->setUInt32(0, lowGuid);
                CharacterDatabase.Execute(stmt);

                handler->PSendSysMessage(LANG_COMMAND_UNFREEZE, name.c_str());
                return true;
            }
            else
            {
                handler->SendSysMessage(LANG_COMMAND_FREEZE_WRONG);
                return true;
            }
        }

        return true;
    }

    bool HandleListFreezeCommand(ChatHandler* handler, char const* /*args*/)
    {
        // Get names from DB
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURA_FROZEN);
        PreparedQueryResult result = CharacterDatabase.Query(stmt);
        if (!result)
        {
            handler->SendSysMessage(LANG_COMMAND_NO_FROZEN_PLAYERS);
            return true;
        }

        // Header of the names
        handler->PSendSysMessage(LANG_COMMAND_LIST_FREEZE);

        // Output of the results
        do
        {
            Field* fields = result->Fetch();
            std::string player = fields[0].GetString();
            int32 remaintime = fields[1].GetInt32();
            // Save the frozen player to update remaining time in case of future .listfreeze uses
            // before the frozen state expires
            if (Player* frozen = sObjectAccessor->FindPlayerByName(player))
                frozen->SaveToDB();
            // Notify the freeze duration
            if (remaintime == -1) // Permanent duration
                handler->PSendSysMessage(LANG_COMMAND_PERMA_FROZEN_PLAYER, player.c_str());
            else
                // show time left (seconds)
                handler->PSendSysMessage(LANG_COMMAND_TEMP_FROZEN_PLAYER, player.c_str(), remaintime / IN_MILLISECONDS);
        } while (result->NextRow());

        return true;
    }

    bool HandlePlayAllCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 soundId = atoi((char*)args);

        if (!sSoundEntriesStore.LookupEntry(soundId))
        {
            handler->PSendSysMessage(LANG_SOUND_NOT_EXIST, soundId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        WorldPacket data(SMSG_PLAY_SOUND, 4);
        data << uint32(soundId) << handler->GetSession()->GetPlayer()->GetGUID();
        sWorld->SendGlobalMessage(data);

        handler->PSendSysMessage(LANG_COMMAND_PLAYED_TO_ALL, soundId);
        return true;
    }

    bool HandlePossessCommand(ChatHandler* handler, char const* /*args*/)
    {
        Unit* unit = handler->getSelectedUnit();
        if (!unit)
            return false;

        handler->GetSession()->GetPlayer()->CastSpell(unit, 530, true);
        return true;
    }

    bool HandleUnPossessCommand(ChatHandler* handler, char const* /*args*/)
    {
        Unit* unit = handler->getSelectedUnit();
        if (!unit)
            unit = handler->GetSession()->GetPlayer();

        unit->RemoveCharmAuras();

        return true;
    }

    bool HandleBindSightCommand(ChatHandler* handler, char const* /*args*/)
    {
        Unit* unit = handler->getSelectedUnit();
        if (!unit)
            return false;

        handler->GetSession()->GetPlayer()->CastSpell(unit, 6277, true);
        return true;
    }

    bool HandleUnbindSightCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();

        if (player->isPossessing())
            return false;

        player->StopCastingBindSight();
        return true;
    }

    bool HandleMailBoxCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();

        handler->GetSession()->SendShowMailBox(player->GetGUID());
        return true;
    }

    bool HandleQuestXPCommand(ChatHandler* handler, char const* args)
    {
        Player* target = handler->getSelectedPlayer();
        std::string argstr{ args };

        if (argstr == "normal")
            target->SetCustomFlag(Player::XP_NORMAL);
        else if (argstr == "double")
            target->RemoveCustomFlag(Player::XP_NORMAL);
        else if (!argstr.empty())
            return false;

        // show current status
        handler->PSendSysMessage(
            target->HasCustomFlag(Player::XP_NORMAL) ? LANG_HAS_NORMAL_QUEST_XP : LANG_HAS_DOUBLE_QUEST_XP,
            target->GetName().c_str()
        );
        return true;
    }

    enum TCG {
        TCG_HORDE = 66812,
        TCG_ALLIANZ = 66813
    };

    bool HandleTcgCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        if (handler->getSelectedUnit()->ToPlayer())
            target = handler->getSelectedPlayer();
        else
            target = handler->GetSession()->GetPlayer();

        int32 count = 1;
        if (*args)
            count = atoi(args);
        std::ostringstream os;
        os << (target->GetTeamId() ? TCG_HORDE : TCG_ALLIANZ);
        os << " ";
        os << count;
        return HandleAddItemCommand(handler, os.str().c_str());

    }

    bool HandlePrintFishersCommand(ChatHandler* handler, char const* /*args*/)
    {
        enum
        {
            SPELL_FISHING = 7620,
        };

        uint32 count = 0;

        boost::shared_lock<boost::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
        HashMapHolder<Player>::MapType const& m = sObjectAccessor->GetPlayers();
        for (HashMapHolder<Player>::MapType::const_iterator itr = m.begin(); itr != m.end(); ++itr)
        {
            Player* player = itr->second;
            if (Spell* spell = player->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
            {
                uint32 id = sSpellMgr->GetFirstSpellInChain(spell->GetSpellInfo()->Id);
                if (id != SPELL_FISHING)
                    continue;

                const AreaTableEntry* area = sAreaTableStore.LookupEntry(player->GetAreaId());

                handler->PSendSysMessage("Player %s (Area: %s)", player->GetName().c_str(), area ? area->area_name[sWorld->GetDefaultDbcLocale()] : "<unknown>");
                ++count;
            }
        }

        handler->PSendSysMessage("Found %u potential fishers", count);

        return true;
    }

    bool HandleIdInfoCommand(ChatHandler* handler, char const* /*args*/)
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_TEMP_INSTANCE);
        stmt->setUInt32(0, handler->GetSession()->GetPlayer()->GetGUID().GetCounter());
        PreparedQueryResult result = CharacterDatabase.Query(stmt);
        if (!result)
        {
            handler->PSendSysMessage("Keine ID gefunden");
            return true;
        }

        handler->PSendSysMessage("Folgende IDs wurden gefunden:");
        do
        {
            Field* fields = result->Fetch();
            uint16 mapId = fields[0].GetUInt16();
            std::string mode = fields[1].GetString();
            MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
            handler->PSendSysMessage("%s - %s", (mapEntry ? mapEntry->name[handler->GetSessionDbcLocale()] : ("<unknown:" + std::to_string(mapId) + ">").c_str()), mode.c_str());
        } while (result->NextRow());

        return true;
    }

    bool HandleBgInfoCommand(ChatHandler* handler, char const* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        BattlegroundQueueTypeId queueTypeID = player->GetBattlegroundQueueTypeId(0);
        BattlegroundTypeId bgTypeId = BattlegroundMgr::BGTemplateId(queueTypeID);
        Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);

        if (!bg) // if the player is NOT in a bg queue
        {
            handler->PSendSysMessage(LANG_COMMAND_BGINFO_NOT_IN_QUEUE);
            return true;
        }

        char const* bgName = bg->GetName();

        PvPDifficultyEntry const* difficultyEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), player->getLevel());
        uint32 q_min_level = difficultyEntry->minLevel;
        uint32 q_max_level = difficultyEntry->maxLevel;

        BattlegroundBracketId bracketId = (BattlegroundBracketId)difficultyEntry->bracketId;

        if (sWorld->getBoolConfig(BATTLEGROUND_CROSSFACTION_ENABLED))
        {
            uint32 minPlayers = bg->GetMinPlayersPerTeam() * 2;
            uint32 playerCount = sBattlegroundMgr->GetPlayerCountInQueue(queueTypeID, bracketId, BG_QUEUE_MIXED);

            handler->PSendSysMessage(
                LANG_BG_QUEUE_ANNOUNCE_CROSSFACTION_SELF,
                bgName,
                q_min_level,
                q_max_level,
                playerCount,
                minPlayers > playerCount ? minPlayers - playerCount : uint32(0)
            );
        }
        else
        {
            uint32 minPlayers = bg->GetMinPlayersPerTeam();
            uint32 playerCountAlliance = sBattlegroundMgr->GetPlayerCountInQueue(queueTypeID, bracketId, BG_QUEUE_NORMAL_ALLIANCE);
            uint32 playerCountHorde = sBattlegroundMgr->GetPlayerCountInQueue(queueTypeID, bracketId, BG_QUEUE_NORMAL_HORDE);

            handler->PSendSysMessage(
                LANG_BG_QUEUE_ANNOUNCE_SELF,
                bgName,
                q_min_level,
                q_max_level,
                playerCountAlliance,
                (minPlayers > playerCountAlliance) ? minPlayers - playerCountAlliance : uint32(0),
                playerCountHorde,
                (minPlayers > playerCountHorde) ? minPlayers - playerCountHorde : uint32(0)
            );
        }

        return true;
    }

    bool HandlePrintAttackersCommand(ChatHandler* handler, const char* args)
    {
        Player* player = handler->GetSession()->GetPlayer();

        Unit* target = handler->getSelectedUnit();
        if (!target)
            target = player;

        if (!target)
            return false;

        handler->PSendSysMessage("Listing attackers for: %s (GUID (low): %u, Entry: %u):", target->GetName(), target->GetGUID().GetCounter(), target->GetEntry());
        const Unit::AttackerSet& attackers = target->getAttackers();
        for (auto attacker : attackers)
        {
            switch (attacker->GetTypeId())
            {
            case TYPEID_PLAYER:
                handler->PSendSysMessage("- Player: %s (GUID (low): %u", attacker->GetName(), attacker->GetGUID().GetCounter());
                break;
            default:
                bool handled = false;
                if (Unit* owner = attacker->GetCharmerOrOwner())
                {
                    handled = true;

                    if (attacker->IsPet())
                        handler->PSendSysMessage("- Pet   : %s (Entry: %u) (Owner: %s (GUID (low): %u, Entry: %u))",
                            attacker->GetName(), attacker->GetEntry(), owner->GetName(), owner->GetGUID().GetCounter(), owner->GetEntry());
                    else if (attacker->IsTotem())
                        handler->PSendSysMessage("- Totem : %s (Entry: %u) (Owner: %s (GUID (low): %u, Entry: %u))",
                            attacker->GetName(), attacker->GetEntry(), owner->GetName(), owner->GetGUID().GetCounter(), owner->GetEntry());
                    else
                        handled = false;
                }

                if (!handled)
                    handler->PSendSysMessage("- Unit: %s (GUID (low): %u, Entry: %u)", attacker->GetName(), attacker->GetGUID().GetCounter(), attacker->GetEntry());
                break;
            }
        }

        return true;
    }

    bool HandleBgRating(ChatHandler* handler, const char* args)
    {
        Player* player = handler->GetSession()->GetPlayer();

        Unit* target = nullptr;
        if (AccountMgr::IsStaffAccount(handler->GetSession()->GetSecurity()))
            target = handler->getSelectedUnit();

        if (!target)
            target = player;

        if (!target)
            return false;

        auto targetPlr = target->ToPlayer();

        if (!targetPlr)
            return false;

        float rating = targetPlr->pvpData.GetRating().rating;
        float rd = targetPlr->pvpData.GetRating().rd;
        int32 lowerBound = int32(rating - (rd * 1.96f) + 0.5f);
        int32 upperBound = int32(rating + (rd * 1.96f) + 0.5f);

        if (!AccountMgr::IsStaffAccount(handler->GetSession()->GetSecurity()))
        {
            handler->PSendSysMessage("Current Rating: %i-%i", lowerBound, upperBound);
            return true;
        }

        handler->PSendSysMessage("BG-Rating Player: %s (Guid: %u, Crossfaction: %u, Item-Level: %f):", target->GetName(), 
            target->GetGUID().GetCounter(), !targetPlr->HasCustomFlag(Player::PlayerCustomFlags::CROSSFACTION_DISABLED), targetPlr->GetPvPItemLevel());
        handler->PSendSysMessage("Rating: %f, RD: %f, Total: %i-%i", rating, rd, lowerBound, upperBound);
        if (auto battleground = targetPlr->GetBattleground())
        {
            auto players = battleground->GetPlayers();

            const uint32 bgTeams[BG_TEAMS_COUNT] = { HORDE, ALLIANCE };
            float currentTeamRating[BG_TEAMS_COUNT];
            float currentTeamRD[BG_TEAMS_COUNT];
            int count[BG_TEAMS_COUNT];

            for (uint32 i = 0; i < BG_TEAMS_COUNT; i++)
            {
                count[i] = 0;
                currentTeamRating[i] = 0;
                currentTeamRD[i] = 0;
                for (auto itr = players.begin(); itr != players.end(); ++itr)
                {
                    if (itr->second.Team == bgTeams[i])
                    {
                        Player* player = ObjectAccessor::FindPlayer(itr->first);
                        if (player)
                        {
                            currentTeamRating[i] += player->pvpData.GetRating().rating;
                            currentTeamRD[i] += player->pvpData.GetRating().rd;
                            ++count[i];
                        }
                    }
                }
            }

            int maxcount = std::max(count[0], count[1]);

            if (maxcount)
                handler->PSendSysMessage("Battleground Info: (%f, %f) - (%f, %f)", currentTeamRating[0] / maxcount, currentTeamRD[0] / maxcount, currentTeamRating[1] / maxcount, currentTeamRD[1] / maxcount);
        }

        return true;
    }

    bool HandleFlushPvPTitlesCommand(ChatHandler* handler, char const* /*args*/)
    {
        PvPData::CalculateRankUpdate();
        handler->SendSysMessage("Calculated PvP title rank update");
        return true;
    }

    bool HandleToggleCrossfactionCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();

        if (player->HasCustomFlag(Player::PlayerCustomFlags::CROSSFACTION_DISABLED))
        {
            handler->SendSysMessage(LANG_CROSSFACTION_ENABLED);
            player->RemoveCustomFlag(Player::PlayerCustomFlags::CROSSFACTION_DISABLED);
        }
        else
        {
            handler->SendSysMessage(LANG_CROSSFACTION_DISABLED);
            player->SetCustomFlag(Player::PlayerCustomFlags::CROSSFACTION_DISABLED);
        }

        return true;
    }

    static bool HandleFMInselCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (player->IsInCombat())
        {
            handler->SendSysMessage("Cannot teleport during combat.");
            return false;
        }
        
        auto fm_start_guid = static_cast<uint32>(sConfigMgr->GetIntDefault("RG.Fusion.Frostmourne.CharacterGuid.Start", 0));
        auto fm_end_guid = static_cast<uint32>(sConfigMgr->GetIntDefault("RG.Fusion.Frostmourne.CharacterGuid.End", 0));

        if (!(player->GetGUID().GetCounter() >= fm_start_guid && player->GetGUID().GetCounter() <= fm_end_guid)
            && !player->IsGameMaster())
        {
            handler->SendSysMessage("This feature is only available to former Frostmourne players.");
            return false;
        }
        
        WorldLocation target;
        target.m_mapId = 1;
        switch (player->GetOTeamId())
        {
            default:
            case TEAM_ALLIANCE:
                target.Relocate(-11367.099609f, -4717.062988f, 7.0f);
                break;
            case TEAM_HORDE:
                target.Relocate(-11858.933594f, -4717.062988f, 7.0f);
                break;
        }

        player->TeleportTo(target);

        return true;
    }

    class ReservedNameAddCommand : public SimpleCommand
    {
    public:
        explicit ReservedNameAddCommand(CommandDefinition definition) : SimpleCommand(std::move(definition), true) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
            if (args.empty())
                return ExecutionResult::FAILURE_MISSING_ARGUMENT;

            std::string nameString = args[0].GetString();

            if (!sObjectMgr->AddReservedName(nameString))
            {
                context.SendSystemMessage(nameString + " couldn't be added. Either bad encoding or name already exists.");
                return ExecutionResult::FAILURE_ERROR_MESSAGE_ALREADY_SENT;
            }

            context.SendSystemMessage(nameString + " successfully added.");
            return ExecutionResult::SUCCESS;
        };
    };

    class ReservedNameDeleteCommand : public SimpleCommand
    {
    public:
        explicit ReservedNameDeleteCommand(CommandDefinition definition) : SimpleCommand(std::move(definition), true) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
            if (args.empty())
                return ExecutionResult::FAILURE_MISSING_ARGUMENT;

            std::string nameString = args[0].GetString();

            if (!sObjectMgr->DeleteReservedName(nameString))
            {
                context.SendSystemMessage(nameString + " couldn't be deleted. Either bad encoding or name doesn't exist.");
                return ExecutionResult::FAILURE_ERROR_MESSAGE_ALREADY_SENT;
            }

            context.SendSystemMessage(nameString + " successfully deleted.");
            return ExecutionResult::SUCCESS;
        };
    };

}}}

void AddSC_misc_commandscript()
{
    using namespace chat::command;
    using namespace chat::command::handler;

    new LegacyCommandScript("cmd_additem",              HandleAddItemCommand,            false);
    new LegacyCommandScript("cmd_additemset",           HandleAddItemSetCommand,         false);
    new LegacyCommandScript("cmd_appear",               HandleAppearCommand,             false);
    new LegacyCommandScript("cmd_aura",                 HandleAuraCommand,               false);
    new LegacyCommandScript("cmd_bank",                 HandleBankCommand,               false);
    new LegacyCommandScript("cmd_bginfo",               HandleBgInfoCommand,             false);
    new LegacyCommandScript("cmd_bindsight",            HandleBindSightCommand,          false);
    new LegacyCommandScript("cmd_combatstop",           HandleCombatStopCommand,         true);
    new LegacyCommandScript("cmd_cometome",             HandleComeToMeCommand,           false);
    new LegacyCommandScript("cmd_cooldown",             HandleCooldownCommand,           false);
    new LegacyCommandScript("cmd_damage",               HandleDamageCommand,             false);
    new LegacyCommandScript("cmd_dev",                  HandleDevCommand,                false);
    new LegacyCommandScript("cmd_die",                  HandleDieCommand,                false);
    new LegacyCommandScript("cmd_dismount",             HandleDismountCommand,           false);
    new LegacyCommandScript("cmd_distance",             HandleGetDistanceCommand,        false);
    new LegacyCommandScript("cmd_flusharenapoints",     HandleFlushArenaPointsCommand,   true);
    new LegacyCommandScript("cmd_freeze",               HandleFreezeCommand,             false);
    new LegacyCommandScript("cmd_gps",                  HandleGPSCommand,                false);
    new LegacyCommandScript("cmd_guid",                 HandleGUIDCommand,               false);
    new LegacyCommandScript("cmd_hidearea",             HandleHideAreaCommand,           false);
    new LegacyCommandScript("cmd_itemmove",             HandleItemMoveCommand,           false);
    new LegacyCommandScript("cmd_kick",                 HandleKickPlayerCommand,         true);
    new LegacyCommandScript("cmd_linkgrave",            HandleLinkGraveCommand,          false);
    new LegacyCommandScript("cmd_listfreeze",           HandleListFreezeCommand,         false);
    new LegacyCommandScript("cmd_maxskill",             HandleMaxSkillCommand,           false);
    new LegacyCommandScript("cmd_movegens",             HandleMovegensCommand,           false);
    new LegacyCommandScript("cmd_mute",                 HandleMuteCommand,               true);
    new LegacyCommandScript("cmd_mutehistory",          HandleMuteInfoCommand,           true);
    new LegacyCommandScript("cmd_neargrave",            HandleNearGraveCommand,          false);
    new LegacyCommandScript("cmd_pinfo",                HandlePInfoCommand,              false);
    new LegacyCommandScript("cmd_playall",              HandlePlayAllCommand,            false);
    new LegacyCommandScript("cmd_possess",              HandlePossessCommand,            false);
    new LegacyCommandScript("cmd_pvpstats",             HandlePvPstatsCommand,           true);
    new LegacyCommandScript("cmd_recall",               HandleRecallCommand,             false);
    new LegacyCommandScript("cmd_repairitems",          HandleRepairitemsCommand,        true);
    new LegacyCommandScript("cmd_respawn",              HandleRespawnCommand,            false);
    new LegacyCommandScript("cmd_revive",               HandleReviveCommand,             true);
    new LegacyCommandScript("cmd_saveall",              HandleSaveAllCommand,            true);
    new LegacyCommandScript("cmd_save",                 HandleSaveCommand,               false);
    new LegacyCommandScript("cmd_setskill",             HandleSetSkillCommand,           false);
    new LegacyCommandScript("cmd_showarea",             HandleShowAreaCommand,           false);
    new LegacyCommandScript("cmd_summon",               HandleSummonCommand,             false);
    new LegacyCommandScript("cmd_unaura",               HandleUnAuraCommand,             false);
    new LegacyCommandScript("cmd_unbindsight",          HandleUnbindSightCommand,        false);
    new LegacyCommandScript("cmd_unfreeze",             HandleUnFreezeCommand,           false);
    new LegacyCommandScript("cmd_unmute",               HandleUnmuteCommand,             true);
    new LegacyCommandScript("cmd_unpossess",            HandleUnPossessCommand,          false);
    new LegacyCommandScript("cmd_unstuck",              HandleUnstuckCommand,            true);
    new LegacyCommandScript("cmd_wchange",              HandleChangeWeather,             false);
    new LegacyCommandScript("cmd_mailbox",              HandleMailBoxCommand,            false);
    new LegacyCommandScript("cmd_questxp",              HandleQuestXPCommand,            false);
    new LegacyCommandScript("cmd_tcg",                  HandleTcgCommand,                false);
    new LegacyCommandScript("cmd_printfishers",         HandlePrintFishersCommand,       true);
    new LegacyCommandScript("cmd_idinfo",               HandleIdInfoCommand,             false);
    new LegacyCommandScript("cmd_printattackers",       HandlePrintAttackersCommand,     false);
    new LegacyCommandScript("cmd_bgrating",             HandleBgRating,                  false);
    new LegacyCommandScript("cmd_flushpvptitles",       HandleFlushPvPTitlesCommand,     true);
    new LegacyCommandScript("cmd_togglecrossfaction",   HandleToggleCrossfactionCommand, false);
    new LegacyCommandScript("cmd_fminsel",              HandleFMInselCommand,            false);

    new GenericCommandScriptLoader<ReservedNameAddCommand>("cmd_reserved_name_add");
    new GenericCommandScriptLoader<ReservedNameDeleteCommand>("cmd_reserved_name_delete");
}
