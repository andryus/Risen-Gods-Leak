/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * Copyright (C) 2010-2015 Rising Gods <http://www.rising-gods.de/>
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

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "trial_of_the_crusader.h"
#include "ScriptedCreature.h"
#include "Opcodes.h"
#include "Packets/WorldPacket.h"

class instance_trial_of_the_crusader : public InstanceMapScript
{
    public:
        instance_trial_of_the_crusader() : InstanceMapScript("instance_trial_of_the_crusader", 649) { }

        struct instance_trial_of_the_crusader_InstanceMapScript : public InstanceScript
        {
            instance_trial_of_the_crusader_InstanceMapScript(Map* map) : InstanceScript(map)
            {
                SetBossNumber(MAX_ENCOUNTER);
                SetHeaders(DataHeader);
                // General Instance Event
                EventTimer = 1000;
                EventStage = 0;

                // Genral NPCs
                BarrentGUID.Clear();
                TirionGUID.Clear();
                FizzlebangGUID.Clear();
                GarroshGUID.Clear();
                VarianGUID.Clear();

                // Northend Beasts
                GormokGUID.Clear();
                SnoboldCount = 0;
                AcidmawGUID.Clear();
                DreadscaleGUID.Clear();
                NotOneButTwoJormungarsTimer = 0;
                IcehowlGUID.Clear();

                // Lord Jaraxxus
                JaraxxusGUID.Clear();
                MistressOfPainCount = 0;

                // Faction Champins
                ChampionsControllerGUID.Clear();
                CrusadersCacheGUID.Clear();
                ResilienceWillFixItTimer = 0;

                // Twin Valkyries
                DarkbaneGUID.Clear();
                LightbaneGUID.Clear();

                // Anub'arak
                AnubarakGUID.Clear();

                // Tribute Run
                TributeChestGUID.Clear();
                TirionFordringGUID.Clear(); // Tirion at the end of the instance
                TrialCounter = 50;
                TeamInInstance = HORDE; // make sure TeamInInstance has always a correct value, to prevent bugs with stupid gms
                                        // On Heroic
                NoPlayerDied = instance->IsHeroic() ? true : false;
                HasOnlyAllowedItems = instance->IsHeroic() && !instance->Is25ManRaid() ? true : false;

                // General GameObjects
                MainGateDoorGUID.Clear();
                EastPortcullisGUID.Clear();
                FloorGUID.Clear();
                WebDoorGUID.Clear();

                // Misc
                NorthrendBeasts = NOT_STARTED;
                NeedGossipFlag = false;
            }

            bool IsEncounterInProgress() const
            {
                for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                {
                    if (i == BOSS_LICH_KING)
                        continue;

                    if (GetBossState(i) == IN_PROGRESS)
                        return true;
                }

                // Special state is set at Faction Champions after first champ dead, encounter is still in combat
                if (GetBossState(BOSS_CRUSADERS) == SPECIAL)
                    return true;

                return false;
            }

            void OnPlayerEnter(Player* player)
            {
                if (player && !player->IsGameMaster())
                    TeamInInstance = player->GetTeam();

                if (instance->IsHeroic())
                {
                    player->SendUpdateWorldState(UPDATE_STATE_UI_SHOW, 1);
                    player->SendUpdateWorldState(UPDATE_STATE_UI_COUNT, GetData(TYPE_COUNTER));
                }
                else
                    player->SendUpdateWorldState(UPDATE_STATE_UI_SHOW, 0);

                // make sure Anub'arak isnt missing and floor is destroyed after a crash
                if (GetBossState(BOSS_LICH_KING) == DONE)
                {
                    if (GameObject* floor = GameObject::GetGameObject(*player, ObjectGuid(GetGuidData(GO_ARGENT_COLISEUM_FLOOR))))
                    {
                        floor->SetDestructibleState(GO_DESTRUCTIBLE_DAMAGED);
                        floor->DestroyForPlayer(player);
                    }

                    if (Creature* announcer = instance->GetCreature(ObjectGuid(GetGuidData(NPC_BARRENT))))
                        announcer->DespawnOrUnsummon();

                    if (TrialCounter && GetBossState(BOSS_ANUBARAK) != DONE)
                    {
                        Creature* anubArak = ObjectAccessor::GetCreature(*player, ObjectGuid(GetGuidData(NPC_ANUBARAK)));
                        if (!anubArak)
                            anubArak = player->SummonCreature(NPC_ANUBARAK, AnubarakLoc[0].GetPositionX(), AnubarakLoc[0].GetPositionY(), AnubarakLoc[0].GetPositionZ(), 3, TEMPSUMMON_CORPSE_TIMED_DESPAWN, DESPAWN_TIME);
                    }
                }

                if (Creature* tirion = instance->GetCreature(ObjectGuid(GetGuidData(NPC_TIRION))))
                {
                    Creature* announcer = tirion->FindNearestCreature(NPC_BARRENT, 100.0f);
                    if (!announcer && GetBossState(BOSS_LICH_KING) != DONE)
                        instance->SummonCreature(NPC_BARRENT, ToCCommonLoc[0]);
                }
            }

            void OnPlayerRemove(Player* player)
            {
                if (player && player->GetVehicleKit())
                {
                    player->RemoveAura(66406);
                    player->RemoveVehicleKit();

                    WorldPacket data(SMSG_PLAYER_VEHICLE_DATA, player->GetPackGUID().size()+4);
                    data << player->GetPackGUID();
                    data << uint32(0);
                    player->SendMessageToSet(data, true);
                }
            }

            void OnPlayerLoggedIn(Player* player)
            {
                // only remove debufs out of a fight, eg after a serverdowns
                if (IsEncounterInProgress())
                    return;

                // ordered by raid difficulty              10man  25man  10manH 25manH ...
                static const uint32 ValkyrieSpells[20] = { 65808, 67172, 67173, 67174, 65686, 67222, 67223, 67224, 67590, 67602, 67603, 67604, 65808, 67172, 67173, 67174, 65795, 67238, 67239, 67240 };

                for (uint8 i = instance->GetSpawnMode(); i<20;)
                {
                    i += 4;
                    player->RemoveAura(ValkyrieSpells[i]);
                }
            }

            void OpenDoor(ObjectGuid guid)
            {
                if (!guid)
                    return;

                if (GameObject* go = instance->GetGameObject(guid))
                    go->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
            }

            void CloseDoor(ObjectGuid guid)
            {
                if (!guid)
                    return;

                if (GameObject* go = instance->GetGameObject(guid))
                    go->SetGoState(GO_STATE_READY);
            }

            void OnCreatureCreate(Creature* creature)
            {
                switch (creature->GetEntry())
                {
                    case NPC_BARRENT:
                        BarrentGUID = creature->GetGUID();
                        if (!TrialCounter)
                            creature->DespawnOrUnsummon();
                        break;
                    case NPC_TIRION:
                        TirionGUID = creature->GetGUID();
                        break;
                    case NPC_TIRION_FORDRING:
                        TirionFordringGUID = creature->GetGUID();
                        break;
                    case NPC_FIZZLEBANG:
                        FizzlebangGUID = creature->GetGUID();
                        break;
                    case NPC_GARROSH:
                        GarroshGUID = creature->GetGUID();
                        break;
                    case NPC_VARIAN:
                        VarianGUID = creature->GetGUID();
                        break;

                    case NPC_GORMOK:
                        GormokGUID = creature->GetGUID();
                        break;
                    case NPC_SNOBOLD_VASSAL:
                        ++SnoboldCount;
                        break;
                    case NPC_ACIDMAW:
                        AcidmawGUID = creature->GetGUID();
                        break;
                    case NPC_DREADSCALE:
                        DreadscaleGUID = creature->GetGUID();
                        break;
                    case NPC_ICEHOWL:
                        IcehowlGUID = creature->GetGUID();
                        break;

                    case NPC_JARAXXUS:
                        JaraxxusGUID = creature->GetGUID();
                        break;
                    case NPC_MISTRESS_OF_PAIN:
                        ++MistressOfPainCount;
                    case NPC_FEL_INFERNAL:
                        if (Creature* jaraxxus = instance->GetCreature(JaraxxusGUID))
                            jaraxxus->AI()->JustSummoned(creature);
                        break;

                    case NPC_CHAMPIONS_CONTROLLER:
                        ChampionsControllerGUID = creature->GetGUID();
                        break;

                    case NPC_DARKBANE:
                        DarkbaneGUID = creature->GetGUID();
                        break;
                    case NPC_LIGHTBANE:
                        LightbaneGUID = creature->GetGUID();
                        break;

                    case NPC_ANUBARAK:
                        AnubarakGUID = creature->GetGUID();
                        creature->SetRespawnDelay(7 * DAY);
                        if (GetBossState(BOSS_ANUBARAK) == DONE)
                            creature->DisappearAndDie();
                        break;
                    default:
                        break;
                }
            }

            void OnGameObjectCreate(GameObject* go)
            {
                switch (go->GetEntry())
                {
                    case GO_CRUSADERS_CACHE_10:
                        if (instance->GetSpawnMode() == RAID_DIFFICULTY_10MAN_NORMAL)
                            CrusadersCacheGUID = go->GetGUID();
                        break;
                    case GO_CRUSADERS_CACHE_25:
                        if (instance->GetSpawnMode() == RAID_DIFFICULTY_25MAN_NORMAL)
                            CrusadersCacheGUID = go->GetGUID();
                        break;
                    case GO_CRUSADERS_CACHE_10_H:
                        if (instance->GetSpawnMode() == RAID_DIFFICULTY_10MAN_HEROIC)
                            CrusadersCacheGUID = go->GetGUID();
                        break;
                    case GO_CRUSADERS_CACHE_25_H:
                        if (instance->GetSpawnMode() == RAID_DIFFICULTY_25MAN_HEROIC)
                            CrusadersCacheGUID = go->GetGUID();
                        break;
                    case GO_ARGENT_COLISEUM_FLOOR:
                        FloorGUID = go->GetGUID();
                        break;
                    case GO_MAIN_GATE_DOOR:
                        MainGateDoorGUID = go->GetGUID();
                        break;
                    case GO_EAST_PORTCULLIS:
                        EastPortcullisGUID = go->GetGUID();
                        break;
                    case GO_WEB_DOOR:
                        WebDoorGUID = go->GetGUID();
                        break;

                    case GO_TRIBUTE_CHEST_10H_25:
                    case GO_TRIBUTE_CHEST_10H_45:
                    case GO_TRIBUTE_CHEST_10H_50:
                    case GO_TRIBUTE_CHEST_10H_99:
                    case GO_TRIBUTE_CHEST_25H_25:
                    case GO_TRIBUTE_CHEST_25H_45:
                    case GO_TRIBUTE_CHEST_25H_50:
                    case GO_TRIBUTE_CHEST_25H_99:
                        TributeChestGUID = go->GetGUID();
                        break;
                    default:
                        break;
                }
            }

            bool SetBossState(uint32 type, EncounterState state)
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;

                switch (type)
                {
                    case BOSS_BEASTS:
                        if (state == FAIL)
                            SnoboldCount = 0;
                        break;
                    case BOSS_JARAXXUS:
                        // Cleanup Icehowl
                        if (Creature* icehowl = instance->GetCreature(IcehowlGUID))
                            icehowl->DespawnOrUnsummon();
                        if (state == DONE)
                            EventStage = 2000;
                        if (state == NOT_STARTED)
                            MistressOfPainCount = 0;
                        break;
                    case BOSS_CRUSADERS:
                        // Cleanup Jaraxxus
                        if (Creature* jaraxxus = instance->GetCreature(JaraxxusGUID))
                            jaraxxus->DespawnOrUnsummon();
                        if (Creature* fizzlebang = instance->GetCreature(FizzlebangGUID))
                            fizzlebang->DespawnOrUnsummon();
                        switch (state)
                        {
                            case IN_PROGRESS:
                                ResilienceWillFixItTimer = 0;
                                break;
                            case SPECIAL: // Means the first blood
                                ResilienceWillFixItTimer = 60*IN_MILLISECONDS;
                                state = IN_PROGRESS;
                                break;
                            case DONE:
                                if (ResilienceWillFixItTimer > 0)
                                    DoUpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, SPELL_CHAMPIONS_KILLED_IN_MINUTE);
                                DoRespawnGameObject(CrusadersCacheGUID, 7*DAY);
                                if (GameObject* chest = instance->GetGameObject(CrusadersCacheGUID))
                                    chest->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                                EventStage = 3100;
                                break;
                            default:
                                break;
                        }
                        break;
                    case BOSS_VALKIRIES:
                        // Cleanup chest
                        if (GameObject* cache = instance->GetGameObject(CrusadersCacheGUID))
                            cache->Delete();
                        switch (state)
                        {
                            case SPECIAL:
                                if (GetBossState(BOSS_VALKIRIES) == SPECIAL)
                                    state = DONE;
                                break;
                            case DONE:
                                if (!instance->GetPlayers().empty() && instance->GetPlayers().begin()->GetTeam() == ALLIANCE)
                                    EventStage = 4020;
                                else
                                    EventStage = 4030;
                                break;
                            default:
                                break;
                        }
                        break;
                    case BOSS_LICH_KING:
                        break;
                    case BOSS_ANUBARAK:
                        switch (state)
                        {
                            case DONE:
                            {
                                EventStage = 6000;
                                uint32 tributeChest = 0;
                                if (instance->GetSpawnMode() == RAID_DIFFICULTY_10MAN_HEROIC)
                                {
                                    if (TrialCounter >= 50)
                                    {
                                        CheckForItemLevel();
                                        tributeChest = GO_TRIBUTE_CHEST_10H_99;
                                    }
                                    else if (TrialCounter >= 45)
                                        tributeChest = GO_TRIBUTE_CHEST_10H_50;
                                    else if (TrialCounter >= 25)
                                        tributeChest = GO_TRIBUTE_CHEST_10H_45;
                                    else
                                        tributeChest = GO_TRIBUTE_CHEST_10H_25;
                                }
                                else if (instance->GetSpawnMode() == RAID_DIFFICULTY_25MAN_HEROIC)
                                {
                                    if (TrialCounter >= 50)
                                        tributeChest = GO_TRIBUTE_CHEST_25H_99;
                                    else if (TrialCounter >= 45)
                                        tributeChest = GO_TRIBUTE_CHEST_25H_50;
                                    else if (TrialCounter >= 25)
                                        tributeChest = GO_TRIBUTE_CHEST_25H_45;
                                    else
                                        tributeChest = GO_TRIBUTE_CHEST_25H_25;
                                }

                                if (tributeChest)
                                    if (Creature* tirion =  instance->GetCreature(TirionGUID))
                                        if (GameObject* chest = tirion->SummonGameObject(tributeChest, 650.47f, 123.69f, 141.60f, 0.15f, 0, 0, 0, 0, WEEK))
                                            chest->SetRespawnTime(chest->GetRespawnDelay());
                                break;
                            }
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }

                if (state == IN_PROGRESS)
                {
                    CloseDoor(ObjectGuid(GetGuidData(GO_EAST_PORTCULLIS)));
                    CloseDoor(ObjectGuid(GetGuidData(GO_WEB_DOOR)));
                    CheckForItemLevel();
                }
                else
                {
                    OpenDoor(ObjectGuid(GetGuidData(GO_EAST_PORTCULLIS)));
                    OpenDoor(ObjectGuid(GetGuidData(GO_WEB_DOOR)));
                }

                if (type < MAX_ENCOUNTER)
                {
                    TC_LOG_INFO("scripts", "[ToCr] BossState(type %u) %u = state %u;", type, GetBossState(type), state);
                    if (state == FAIL)
                    {
                        if (instance->IsHeroic())
                        {
                            --TrialCounter;
                            // decrease attempt counter at wipe
                            for (Player& player : instance->GetPlayers())
                                player.SendUpdateWorldState(UPDATE_STATE_UI_COUNT, TrialCounter);

                            // if theres no more attemps allowed
                            if (!TrialCounter)
                            {
                                if (Creature* announcer = instance->GetCreature(ObjectGuid(GetGuidData(NPC_BARRENT))))
                                    announcer->DespawnOrUnsummon();

                                if (Creature* jaraxxus = instance->GetCreature(JaraxxusGUID))
                                    jaraxxus->DespawnOrUnsummon();

                                if (Creature* anubArak = instance->GetCreature(ObjectGuid(GetGuidData(NPC_ANUBARAK))))
                                    anubArak->DespawnOrUnsummon();
                            }
                        }
                        NeedGossipFlag = true;
                        EventStage = (type == BOSS_BEASTS ? 666 : 0);
                        state = NOT_STARTED;
                    }

                    if (state == DONE || NeedGossipFlag)
                    {
                        if (Creature* tirion = instance->GetCreature(ObjectGuid(GetGuidData(NPC_TIRION))))
                        {
                            Creature* announcer = tirion->FindNearestCreature(NPC_BARRENT, 100.0f);
                            if (!announcer)
                            {
                                if (GetBossState(BOSS_LICH_KING) != DONE)
                                    instance->SummonCreature(NPC_BARRENT, ToCCommonLoc[0]);
                            }
                            else
                                announcer->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                            NeedGossipFlag = false;
                        }
                        SaveToDB();
                    }
                }
                return true;
            }

            void OnPlayerEquipItem(Player* player, Item* item, EquipmentSlots slot)
            {
                if (!HasOnlyAllowedItems || TrialCounter != 50 || player->IsGameMaster() || !IsEncounterInProgress() ||
                    instance->GetDifficulty() != RAID_DIFFICULTY_10MAN_HEROIC)
                    return;

                // only weapons can be changed infight
                if (slot != EQUIPMENT_SLOT_MAINHAND && slot != EQUIPMENT_SLOT_OFFHAND && slot != EQUIPMENT_SLOT_RANGED)
                    return;

                if (item->GetTemplate()->ItemLevel > MAX_ALLOWED_ITEMLEVEL)
                {
                    DoSendNotifyToInstance("|cFFFFFC00 [A Tribute to Dedicated Insanity!] |cFF00FFFF Failed item: %u.", item->GetEntry());
                    HasOnlyAllowedItems = false;
                    return;
                }

                // some 245 parts are also forbidden
                for (uint32 i=0; i < sizeof(ForbiddenWeapons)/sizeof(uint32); ++i)
                {
                    if (item->GetEntry() == ForbiddenWeapons[i])
                    {
                        DoSendNotifyToInstance("|cFFFFFC00 [A Tribute to Dedicated Insanity!] |cFF00FFFF Failed item: %u.", item->GetEntry());
                        HasOnlyAllowedItems = false;
                        return;
                    }
                }
            }

            void CheckForItemLevel()
            {
                if (!HasOnlyAllowedItems || TrialCounter != 50 || instance->GetDifficulty() != RAID_DIFFICULTY_10MAN_HEROIC)
                    return;

                for (Player& player : instance->GetPlayers()) // check all slots on all player
                {
                    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
                    {
                        if (slot == EQUIPMENT_SLOT_TABARD || slot == EQUIPMENT_SLOT_BODY)
                            continue;

                        if (Item* item = player.GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                        {
                            if (item->GetTemplate()->ItemLevel > MAX_ALLOWED_ITEMLEVEL) // to hight itemlevel (ilvl > 245)
                            {
                                if (slot == EQUIPMENT_SLOT_BACK) // some cloacks(ilvl 258) are allowed
                                {
                                    bool isAllowedCloack = false;
                                    for (uint8 i=0; i<10; i++)
                                    {
                                        if (item->GetEntry() == AllowedItem[i])
                                            isAllowedCloack = true;
                                    }
                                    if (!isAllowedCloack) // return false if no special cloack
                                    {
                                            DoSendNotifyToInstance("|cFFFFFC00 [A Tribute to Dedicated Insanity!] |cFF00FFFF Failed item: %u.", item->GetEntry());
                                        HasOnlyAllowedItems = false;
                                        return;
                                    }
                                }
                                else
                                {
                                        DoSendNotifyToInstance("|cFFFFFC00 [A Tribute to Dedicated Insanity!] |cFF00FFFF Failed item: %u.", item->GetEntry());
                                    HasOnlyAllowedItems = false; // no 258er cloack
                                    return;
                                }
                            }
                            else // item level below 245
                            {
                                // some 245 parts are also forbidden
                                for (uint32 i=0; i < sizeof(ForbiddenItems)/sizeof(uint32); ++i)
                                {
                                    if (item->GetEntry() == ForbiddenItems[i])
                                    {
                                            DoSendNotifyToInstance("|cFFFFFC00 [A Tribute to Dedicated Insanity!] |cFF00FFFF Failed item: %u.", item->GetEntry());
                                        HasOnlyAllowedItems = false;
                                        return;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            void SetData(uint32 type, uint32 data)
            {
                switch (type)
                {
                    case TYPE_COUNTER:
                        TrialCounter = data;
                        break;
                    case TYPE_EVENT:
                        EventStage = data;
                        break;
                    case TYPE_EVENT_TIMER:
                        EventTimer = data;
                        break;
                    case TYPE_NORTHREND_BEASTS:
                        NorthrendBeasts = data;
                        switch (data)
                        {
                            case GORMOK_DONE:
                                EventStage = 200;
                                SetData(TYPE_NORTHREND_BEASTS, IN_PROGRESS);
                                break;
                            case SNAKES_IN_PROGRESS:
                                NotOneButTwoJormungarsTimer = 0;
                                break;
                            case SNAKES_SPECIAL:
                                NotOneButTwoJormungarsTimer = 10*IN_MILLISECONDS;
                                break;
                            case SNAKES_DONE:
                                if (NotOneButTwoJormungarsTimer > 0)
                                    DoUpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, SPELL_WORMS_KILLED_IN_10_SECONDS);
                                EventStage = 300;
                                SetData(TYPE_NORTHREND_BEASTS, IN_PROGRESS);
                                break;
                            case ICEHOWL_DONE:
                                EventStage = 400;
                                SetData(TYPE_NORTHREND_BEASTS, DONE);
                                SetBossState(BOSS_BEASTS, DONE);
                                break;
                            case FAIL:
                                SetBossState(BOSS_BEASTS, FAIL);
                                break;
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }
            }

            ObjectGuid GetGuidData(uint32 type) const
            {
                switch (type)
                {
                    case NPC_BARRENT:
                        return BarrentGUID;
                    case NPC_TIRION:
                        return TirionGUID;
                    case NPC_TIRION_FORDRING:
                        return TirionFordringGUID;
                    case NPC_FIZZLEBANG:
                        return FizzlebangGUID;
                    case NPC_GARROSH:
                        return GarroshGUID;
                    case NPC_VARIAN:
                        return VarianGUID;

                    case NPC_GORMOK:
                        return GormokGUID;
                    case NPC_ACIDMAW:
                        return AcidmawGUID;
                    case NPC_DREADSCALE:
                        return DreadscaleGUID;
                    case NPC_ICEHOWL:
                        return IcehowlGUID;

                    case NPC_JARAXXUS:
                        return JaraxxusGUID;

                    case NPC_CHAMPIONS_CONTROLLER:
                        return ChampionsControllerGUID;

                    case NPC_DARKBANE:
                        return DarkbaneGUID;
                    case NPC_LIGHTBANE:
                        return LightbaneGUID;

                    case NPC_ANUBARAK:
                        return AnubarakGUID;

                    case GO_ARGENT_COLISEUM_FLOOR:
                        return FloorGUID;
                    case GO_MAIN_GATE_DOOR:
                        return MainGateDoorGUID;
                    case GO_EAST_PORTCULLIS:
                        return EastPortcullisGUID;
                    case GO_WEB_DOOR:
                        return WebDoorGUID;
                    default:
                        break;
                }
                return ObjectGuid::Empty;
            }

            uint32 GetData(uint32 type) const
            {
                switch (type)
                {
                    case TYPE_COUNTER:
                        return TrialCounter;
                    case TYPE_EVENT:
                        return EventStage;
                    case TYPE_NORTHREND_BEASTS:
                        return NorthrendBeasts;
                    case TYPE_EVENT_TIMER:
                        return EventTimer;
                    case TYPE_TEAM_IN_INSTANCE:
                        return TeamInInstance;
                    case TYPE_EVENT_NPC:
                        switch (EventStage)
                        {
                            case 110:
                            case 140:
                            case 150:
                            case 155:
                            case 200:
                            case 205:
                            case 210:
                            case 220:
                            case 300:
                            case 305:
                            case 310:
                            case 315:
                            case 400:
                            case 666:
                            case 1010:
                            case 1180:
                            case 2000:
                            case 2030:
                            case 3000:
                            case 3001:
                            case 3060:
                            case 3061:
                            case 3090:
                            case 3091:
                            case 3092:
                            case 3100:
                            case 3110:
                            case 4000:
                            case 4010:
                            case 4015:
                            case 4016:
                            case 4040:
                            case 4050:
                            case 5000:
                            case 5005:
                            case 5020:
                            case 6000:
                            case 6005:
                            case 6010:
                                return NPC_TIRION;
                                break;
                            case 5010:
                            case 5030:
                            case 5040:
                            case 5050:
                            case 5060:
                            case 5070:
                            case 5080:
                                return NPC_LICH_KING;
                                break;
                            case 120:
                            case 122:
                            case 2020:
                            case 3080:
                            case 3051:
                            case 3071:
                            case 4020:
                                return NPC_VARIAN;
                                break;
                            case 130:
                            case 132:
                            case 2010:
                            case 3050:
                            case 3070:
                            case 3081:
                            case 4030:
                                return NPC_GARROSH;
                                break;
                            case 1110:
                            case 1120:
                            case 1130:
                            case 1132:
                            case 1134:
                            case 1135:
                            case 1140:
                            case 1142:
                            case 1144:
                            case 1150:
                                return NPC_FIZZLEBANG;
                                break;
                            default:
                                return NPC_TIRION;
                                break;
                        };
                    default:
                        break;
                }
                return 0;
            }

            void OnUnitDeath(Unit* unit)
            {
                if (unit->GetTypeId() == TYPEID_PLAYER && NoPlayerDied)
                {
                    NoPlayerDied = false;
                    SaveToDB();
                }
                else if (Creature* creature = unit->ToCreature())
                {
                    switch (creature->GetEntry())
                    {
                        case NPC_SNOBOLD_VASSAL:
                            if (SnoboldCount > 0)
                                --SnoboldCount;
                            break;
                        case NPC_MISTRESS_OF_PAIN:
                            if (MistressOfPainCount > 0)
                                --MistressOfPainCount;
                            break;
                        default:
                            break;
                    }
                    if (creature->GetCreatureTemplate()->rank == 3) //! boss
                        creature->SetRespawnTime(608400);           //! one week
                }

                if (GetBossState(BOSS_BEASTS) == IN_PROGRESS)
                    if (unit->GetTypeId() == TYPEID_PLAYER)
                        if (unit->GetVehicleKit())
                        {
                            unit->RemoveAura(66406);
                            unit->RemoveVehicleKit();

                            WorldPacket data(SMSG_PLAYER_VEHICLE_DATA, unit->ToPlayer()->GetPackGUID().size()+4);
                            data << unit->GetPackGUID();
                            data << uint32(0);
                            unit->ToPlayer()->SendMessageToSet(data, true);
                        }
            }

            void Update(uint32 diff)
            {
                if (GetData(TYPE_NORTHREND_BEASTS) == SNAKES_SPECIAL && NotOneButTwoJormungarsTimer)
                {
                    if (NotOneButTwoJormungarsTimer <= diff)
                        NotOneButTwoJormungarsTimer = 0;
                    else
                        NotOneButTwoJormungarsTimer -= diff;
                }

                if (GetBossState(BOSS_CRUSADERS) == SPECIAL && ResilienceWillFixItTimer)
                {
                    if (ResilienceWillFixItTimer <= diff)
                        ResilienceWillFixItTimer = 0;
                    else
                        ResilienceWillFixItTimer -= diff;
                }
            }

            void WriteSaveDataMore(std::ostringstream& data) override
            {
                data << TrialCounter          << ' '         // Trial counter
                     << (uint32)NoPlayerDied  << ' '         // Tribut to immortality achivement
                     << (uint32)HasOnlyAllowedItems;         // Tribut to dedicated insanity achivement
            }

            void Load(const char* strIn)
            {
                if (!strIn)
                {
                    OUT_LOAD_INST_DATA_FAIL;
                    return;
                }

                OUT_LOAD_INST_DATA(strIn);

                std::istringstream loadStream(strIn);

                char dataHead1, dataHead2, dataHead3;
                loadStream >> dataHead1 >> dataHead2 >> dataHead3;

                if (dataHead1 == 'T' && dataHead2 == 'C' && dataHead3 == 'R')
                {
                    for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                    {
                        uint32 tmpState;
                        loadStream >> tmpState;

                        if (tmpState == IN_PROGRESS || tmpState > SPECIAL)
                            tmpState = NOT_STARTED;

                        if (i == BOSS_CRUSADERS)
                        {
                            // SPECIAL should only be set by champions. i am not sure if this will brak something else
                            if (tmpState == IN_PROGRESS || tmpState >= SPECIAL)
                                tmpState = NOT_STARTED;
                        }

                        SetBossState(i, EncounterState(tmpState));
                    }

                    loadStream >> TrialCounter;
                    loadStream >> NoPlayerDied;
                    loadStream >> HasOnlyAllowedItems;
                    EventStage = 0;
                }
                else
                {
                    for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                    {
                        SetBossState(i, DONE);
                    }
                }

                OUT_LOAD_INST_DATA_COMPLETE;
            }

            bool CheckAchievementCriteriaMeet(uint32 criteria_id, Player const* /*source*/, Unit const* /*target*/, uint32 /*miscvalue1*/)
            {
                switch (criteria_id)
                {
                    case UPPER_BACK_PAIN_10_PLAYER:
                    case UPPER_BACK_PAIN_10_PLAYER_HEROIC:
                        return SnoboldCount >= 2;
                    case UPPER_BACK_PAIN_25_PLAYER:
                    case UPPER_BACK_PAIN_25_PLAYER_HEROIC:
                        return SnoboldCount >= 4;
                    case THREE_SIXTY_PAIN_SPIKE_10_PLAYER:
                    case THREE_SIXTY_PAIN_SPIKE_10_PLAYER_HEROIC:
                    case THREE_SIXTY_PAIN_SPIKE_25_PLAYER:
                    case THREE_SIXTY_PAIN_SPIKE_25_PLAYER_HEROIC:
                        return MistressOfPainCount >= 2;
                    case A_TRIBUTE_TO_SKILL_10_PLAYER:
                    case A_TRIBUTE_TO_SKILL_25_PLAYER:
                        return TrialCounter >= 25;
                    case A_TRIBUTE_TO_MAD_SKILL_10_PLAYER:
                    case A_TRIBUTE_TO_MAD_SKILL_25_PLAYER:
                        return TrialCounter >= 45;
                    case A_TRIBUTE_TO_INSANITY_10_PLAYER:
                    case A_TRIBUTE_TO_INSANITY_25_PLAYER:
                    case REALM_FIRST_GRAND_CRUSADER:
                        return TrialCounter == 50;
                    case A_TRIBUTE_TO_IMMORTALITY_HORDE:
                    case A_TRIBUTE_TO_IMMORTALITY_ALLIANCE:
                        return TrialCounter == 50 && NoPlayerDied;
                    case A_TRIBUTE_TO_DEDICATED_INSANITY:
                        return TrialCounter == 50 && HasOnlyAllowedItems;
                    default:
                        break;
                }

                return false;
            }

            protected:
                // General Instance Event
                uint32 EventTimer;
                uint32 EventStage;
                uint32 TrialCounter; // on heroic
                uint32 TeamInInstance;

                // Genral NPCs
                ObjectGuid BarrentGUID;
                ObjectGuid TirionGUID;
                ObjectGuid FizzlebangGUID;
                ObjectGuid GarroshGUID;
                ObjectGuid VarianGUID;

                // Northend Beasts
                ObjectGuid GormokGUID;
                uint8  SnoboldCount;
                ObjectGuid AcidmawGUID;
                ObjectGuid DreadscaleGUID;
                uint32 NotOneButTwoJormungarsTimer;
                ObjectGuid IcehowlGUID;

                // Lord Jaraxxus
                ObjectGuid JaraxxusGUID;
                uint8  MistressOfPainCount;

                // Faction Champins
                ObjectGuid ChampionsControllerGUID;
                ObjectGuid CrusadersCacheGUID;
                uint32 ResilienceWillFixItTimer;

                // Twin Valkyries
                ObjectGuid DarkbaneGUID;
                ObjectGuid LightbaneGUID;

                // Anub'arak
                ObjectGuid AnubarakGUID;

                // Tribute Run
                ObjectGuid TributeChestGUID;
                ObjectGuid TirionFordringGUID;
                bool   NoPlayerDied;
                bool   HasOnlyAllowedItems;

                // General GameObjects
                ObjectGuid MainGateDoorGUID;
                ObjectGuid EastPortcullisGUID;
                ObjectGuid FloorGUID;
                ObjectGuid WebDoorGUID;

                // Misc
                uint32 NorthrendBeasts;
                bool NeedGossipFlag;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_trial_of_the_crusader_InstanceMapScript(map);
        }
};

void AddSC_instance_trial_of_the_crusader()
{
    new instance_trial_of_the_crusader();
}
