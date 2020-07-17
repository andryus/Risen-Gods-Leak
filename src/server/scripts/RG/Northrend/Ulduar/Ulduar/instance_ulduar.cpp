/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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
#include "ScriptedCreature.h"
#include "InstanceScript.h"
#include "ObjectMgr.h"
#include "ulduar.h"
#include "Player.h"
#include "WorldPacket.h"
#include "Vehicle.h"

static DoorData const BossDoorData[] =
{
    { GO_LEVIATHAN_DOOR_S,             BOSS_LEVIATHAN, DOOR_TYPE_ROOM       },
    { GO_LEVIATHAN_DOOR_N,             BOSS_LEVIATHAN, DOOR_TYPE_ROOM       },
    // Ignis no door
    // Razorscale no door
    { GO_XT002_DOOR,                   BOSS_XT002,     DOOR_TYPE_ROOM       },
    { GO_ARCHIVUM_DOOR,                BOSS_ASSEMBLY,  DOOR_TYPE_PASSAGE    },
    { GO_IRON_COUNCIL_ENTRANCE,        BOSS_ASSEMBLY,  DOOR_TYPE_ROOM       },
//    { GO_KOLOGARN_BRIDGE,              BOSS_KOLOGARN,  DOOR_TYPE_PASSAGE  }, // Manually cause bridge handles differently
    // Auriaya no doof
    { GO_HODIR_IN_DOOR_STONE,          BOSS_HODIR,     DOOR_TYPE_ROOM       },
    { GO_HODIR_OUT_DOOR_ICE,           BOSS_HODIR,     DOOR_TYPE_PASSAGE    },
    { GO_HODIR_OUT_DOOR_STONE,         BOSS_HODIR,     DOOR_TYPE_PASSAGE    },
    { GO_THORIM_ENCOUNTER_DOOR,        BOSS_THORIM,    DOOR_TYPE_ROOM       },
    // Freya no door
    { GO_MIMIRON_DOOR_1,               BOSS_MIMIRON,   DOOR_TYPE_ROOM       },
    { GO_MIMIRON_DOOR_2,               BOSS_MIMIRON,   DOOR_TYPE_ROOM       },
    { GO_MIMIRON_DOOR_3,               BOSS_MIMIRON,   DOOR_TYPE_ROOM       },
    { GO_VEZAX_DOOR,                   BOSS_VEZAX,     DOOR_TYPE_PASSAGE    },
    { GO_YOGGSARON_DOOR,               BOSS_VEZAX,     DOOR_TYPE_PASSAGE    },
    { GO_YOGGSARON_DOOR,               BOSS_YOGGSARON, DOOR_TYPE_ROOM       },
    { GO_DOODAD_UL_SIGILDOOR_03,       BOSS_ALGALON,   DOOR_TYPE_ROOM       },
    { GO_DOODAD_UL_UNIVERSEFLOOR_01,   BOSS_ALGALON,   DOOR_TYPE_ROOM       },
    { GO_DOODAD_UL_UNIVERSEFLOOR_02,   BOSS_ALGALON,   DOOR_TYPE_SPAWN_HOLE },
    { GO_DOODAD_UL_UNIVERSEGLOBE01,    BOSS_ALGALON,   DOOR_TYPE_SPAWN_HOLE },
    { GO_DOODAD_UL_ULDUAR_TRAPDOOR_03, BOSS_ALGALON,   DOOR_TYPE_SPAWN_HOLE },
    { 0,                               0,              DOOR_TYPE_ROOM       }
};

static MinionData const BossMinionData[] =
{
    { NPC_SANCTUM_SENTRY, BOSS_AURIAYA },
    { 0,                  0            },
};

class instance_ulduar : public InstanceMapScript
{
public:
    instance_ulduar() : InstanceMapScript("instance_ulduar", 603) { }

    struct instance_ulduar_InstanceMapScript : public InstanceScript
    {
        instance_ulduar_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(MAX_ENCOUNTER);
            LoadDoorData(BossDoorData);
            LoadMinionData(BossMinionData);

            // Leviathan
            DwarfCount = 0;
            dalaranPortalGUID.Clear();
            LeviathanTowerState = 0;
            ColossusCount = 0;
            LeviathanEncountered = false;
            // Ignis
            IgnisGUID.Clear();
            // Razorscale
            RazorscaleGUID.Clear();
            ExpeditionCommanderGUID.Clear();
            // XT-002
            Xt002GUID.Clear();
            // Kologarn
            KologarnGUID.Clear();
            KologarnBridgeGUID.Clear();
            KologarnChestGUID.Clear();
            // Auriaya
            AuriayaGUID.Clear();
            // Mimiron
            MimironTrainGUID.Clear();
            MimironElevatorGUID.Clear();
            MimironGUID.Clear();
            LeviathanMKIIGUID.Clear();
            MimironTeleporterActivated = false;
            // Hodir
            HodirGUID.Clear();
            // Thorim
            ThorimGUID.Clear();
            RunicDoorGUID.Clear();
            StoneDoorGUID.Clear();
            // Freya
            FreyaGUID.Clear();
            // Vezax
            VezaxGUID.Clear();
            // Yogg Saron
            YoggSaronGUID.Clear();
            SaraGUID.Clear();
            SupportKeeperFlag = 0;
            FreyaWalkwayGUID.Clear();
            MimironWalkwayGUID.Clear();
            ThorimWalkwayGUID.Clear();
            HodirWalkwayGUID.Clear();
            // Algalon
            AlgalonUniverseGUID.Clear();
            AlgalonTrapdoorGUID.Clear();
            BrannBronzebeardAlgGUID.Clear();
            AlgalonTimer = 61;
            HasOnlyAllowedItems = instance->Is25ManRaid() ? false : true;
            AlgalonSummoned = false;

            // Gates
            WayToYoggGUID.Clear();
            LeviathanGateGUID.Clear();
            YoggSaronBrainDoor1GUID.Clear();
            YoggSaronBrainDoor2GUID.Clear();
            YoggSaronBrainDoor3GUID.Clear();

            // Misc
            conSpeedAtory = false;
            IsAllianz = false;
            SanctumSentryCounter = 0;
            AssemlyDeadCount = 0;
            PlayerEncounterDeaths = 0;
        }

        // Leviathan
        ObjectGuid LeviathanGUID;
        ObjectGuid LeviathanGateGUID;
        std::list<ObjectGuid> VehicleGUIDs;
        bool LeviathanEncountered;
        uint32 DwarfCount;
        ObjectGuid dalaranPortalGUID; //! Todo
        // Tower States
        uint32 LeviathanTowerState;
        ObjectGuid LeviathanTowerGUIDs[4][2];
        uint8 ColossusCount;

        // Ignis
        ObjectGuid IgnisGUID;

        // Razorscale
        ObjectGuid RazorscaleGUID;
        ObjectGuid ExpeditionCommanderGUID;

        // XT-002
        ObjectGuid Xt002GUID;

        // Assembly of Iron
        ObjectGuid AssemblyGUIDs[3];
        uint8 AssemlyDeadCount;

        // Kologarn
        ObjectGuid KologarnGUID;
        ObjectGuid KologarnChestGUID;
        ObjectGuid KologarnBridgeGUID;

        // Auriaya
        ObjectGuid AuriayaGUID;
        ObjectGuid SanctumSentryGUIDs[4];

        // Hodir
        ObjectGuid HodirGUID;
        ObjectGuid HodirCacheGUID;
        ObjectGuid HodirRareCacheGUID;
        ObjectGuid HodirCacheHeroGUID;
        ObjectGuid HodirRareCacheHeroGUID;

        // Mimiron
        ObjectGuid MimironTrainGUID;
        ObjectGuid MimironGUID;
        ObjectGuid LeviathanMKIIGUID;
        ObjectGuid Vx001GUID;
        ObjectGuid AerialUnitGUID;
        ObjectGuid MagneticCoreGUID;
        ObjectGuid MimironElevatorGUID;
        bool MimironTeleporterActivated;

        // Thorim
        ObjectGuid ThorimGUID;
        ObjectGuid RunicColossusGUID;
        ObjectGuid RuneGiantGUID;
        ObjectGuid RunicDoorGUID;
        ObjectGuid StoneDoorGUID;
        ObjectGuid SifBlizzardGUID;
        ObjectGuid ThorimLeverGUID;

        // Freya
        ObjectGuid FreyaGUID;
        ObjectGuid ElderBrightleafGUID;
        ObjectGuid ElderIronbranchGUID;
        ObjectGuid ElderStonebarkGUID;
        ObjectGuid FreyaAchieveTriggerGUID;
        bool conSpeedAtory;

        // Vezax
        ObjectGuid WayToYoggGUID;
        ObjectGuid VezaxGUID;

        // Yogg-Saron
        uint32  SupportKeeperFlag;
        ObjectGuid YoggSaronGUID;
        ObjectGuid SaraGUID;
        ObjectGuid YoggSaronBrainDoor1GUID;
        ObjectGuid YoggSaronBrainDoor2GUID;
        ObjectGuid YoggSaronBrainDoor3GUID;
        ObjectGuid FreyaWalkwayGUID;
        ObjectGuid MimironWalkwayGUID;
        ObjectGuid ThorimWalkwayGUID;
        ObjectGuid HodirWalkwayGUID;

        // Algalon
        ObjectGuid AlgalonGUID;
        ObjectGuid AlgalonSigilDoorGUID[3];
        ObjectGuid AlgalonFloorGUID[2];
        ObjectGuid AlgalonUniverseGUID;
        ObjectGuid AlgalonTrapdoorGUID;
        ObjectGuid BrannBronzebeardAlgGUID;
        uint32 AlgalonTimer;
        bool AlgalonSummoned;
        bool HasOnlyAllowedItems;

        // Misc
        EventMap _events;
        uint8 SanctumSentryCounter;
        bool IsAllianz;
        uint32 PlayerEncounterDeaths;

        void OnPlayerEnter(Player* player)
        {
            if (player && !player->IsGameMaster())
                IsAllianz = player->GetTeam() == ALLIANCE;
        }
        
        void OnPlayerRemove(Player* /*player*/)
        {
            ResetInstance();
        }

        bool CheckAchievementCriteriaMeet(uint32 criteria_id, Player const* /*source*/, Unit const* /*target = NULL*/, uint32 /*miscvalue1 = 0*/)
        {
            // Yogg-Saron
            switch (criteria_id)
            {
                case CRITERIA_THE_ASSASSINATION_OF_KING_LLANE_10:
                case CRITERIA_THE_ASSASSINATION_OF_KING_LLANE_25:
                {
                    if (GetBossState(BOSS_YOGGSARON) != IN_PROGRESS)
                        return false;

                    if (Creature* Sara = instance->GetCreature(SaraGUID))
                        return (Sara->AI()->GetData(DATA_PORTAL_PHASE) == 0);

                    return false;
                }
                case CRITERIA_THE_TORTURED_CHAMPION_10:
                case CRITERIA_THE_TORTURED_CHAMPION_25:
                {
                    if (GetBossState(BOSS_YOGGSARON) != IN_PROGRESS)
                        return false;

                    if (Creature* Sara = instance->GetCreature(SaraGUID))
                        return (Sara->AI()->GetData(DATA_PORTAL_PHASE) == 2);

                    return false;
                }
                case CRITERIA_FORGING_OF_THE_DEMON_SOUL_10:
                case CRITERIA_FORGING_OF_THE_DEMON_SOUL_25:
                {
                    if (GetBossState(BOSS_YOGGSARON) != IN_PROGRESS)
                        return false;

                    if (Creature* Sara = instance->GetCreature(SaraGUID))
                        return (Sara->AI()->GetData(DATA_PORTAL_PHASE) == 1);

                    return false;
                }
                // not working for unknown reason... :/
                //case CRITERIA_HERALD_OF_TITANS:
                //    return HasOnlyAllowedItems && instance->GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL;
                case CRITERIA_KILL_WITHOUT_DEATHS_FLAMELEVIATAN_10:
                case CRITERIA_KILL_WITHOUT_DEATHS_FLAMELEVIATAN_25:
                    return !(PlayerEncounterDeaths & (1 << BOSS_LEVIATHAN));
                case CRITERIA_KILL_WITHOUT_DEATHS_RAZORSCALE_10:
                case CRITERIA_KILL_WITHOUT_DEATHS_RAZORSCALE_25:
                    return !(PlayerEncounterDeaths & (1 << BOSS_RAZORSCALE));
                case CRITERIA_KILL_WITHOUT_DEATHS_IGNIS_10:
                case CRITERIA_KILL_WITHOUT_DEATHS_IGNIS_25:
                    return !(PlayerEncounterDeaths & (1 << BOSS_IGNIS));
                case CRITERIA_KILL_WITHOUT_DEATHS_XT002_10:
                case CRITERIA_KILL_WITHOUT_DEATHS_XT002_25:
                    return !(PlayerEncounterDeaths & (1 << BOSS_XT002));
                case CRITERIA_KILL_WITHOUT_DEATHS_ASSEMBLY_10:
                case CRITERIA_KILL_WITHOUT_DEATHS_ASSEMBLY_25:
                    return !(PlayerEncounterDeaths & (1 << BOSS_ASSEMBLY));
                case CRITERIA_KILL_WITHOUT_DEATHS_KOLOGARN_10:
                case CRITERIA_KILL_WITHOUT_DEATHS_KOLOGARN_25:
                    return !(PlayerEncounterDeaths & (1 << BOSS_KOLOGARN));
                case CRITERIA_KILL_WITHOUT_DEATHS_AURIAYA_10:
                case CRITERIA_KILL_WITHOUT_DEATHS_AURIAYA_25:
                    return !(PlayerEncounterDeaths & (1 << BOSS_AURIAYA));
                case CRITERIA_KILL_WITHOUT_DEATHS_HODIR_10:
                case CRITERIA_KILL_WITHOUT_DEATHS_HODIR_25:
                    return !(PlayerEncounterDeaths & (1 << BOSS_HODIR));
                case CRITERIA_KILL_WITHOUT_DEATHS_THORIM_10:
                case CRITERIA_KILL_WITHOUT_DEATHS_THORIM_25:
                    return !(PlayerEncounterDeaths & (1 << BOSS_THORIM));
                case CRITERIA_KILL_WITHOUT_DEATHS_FREYA_10:
                case CRITERIA_KILL_WITHOUT_DEATHS_FREYA_25:
                    return !(PlayerEncounterDeaths & (1 << BOSS_FREYA));
                case CRITERIA_KILL_WITHOUT_DEATHS_MIMIRON_10:
                case CRITERIA_KILL_WITHOUT_DEATHS_MIMIRON_25:
                    return !(PlayerEncounterDeaths & (1 << BOSS_MIMIRON));
                case CRITERIA_KILL_WITHOUT_DEATHS_VEZAX_10:
                case CRITERIA_KILL_WITHOUT_DEATHS_VEZAX_25:
                    return !(PlayerEncounterDeaths & (1 << BOSS_VEZAX));
                case CRITERIA_KILL_WITHOUT_DEATHS_YOGGSARON_10:
                case CRITERIA_KILL_WITHOUT_DEATHS_YOGGSARON_25:
                    return !(PlayerEncounterDeaths & (1 << BOSS_YOGGSARON));
            }
            return false;
        }

        void FillInitialWorldStates(WorldPacket& packet)
        {
            packet << uint32(WORLD_STATE_ALGALON_TIMER_ENABLED) << uint32(AlgalonTimer && AlgalonTimer <= 60);
            packet << uint32(WORLD_STATE_ALGALON_DESPAWN_TIMER) << uint32(std::min<uint32>(AlgalonTimer, 60));
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                // Flame Leviathan
                case NPC_LEVIATHAN:
                    if (!creature->IsAlive())
                        SetBossState(BOSS_LEVIATHAN, DONE);
                    LeviathanGUID = creature->GetGUID();
                    break;
                case NPC_COLOSSUS:
                    if (creature->GetCreatureData()->spawndist == 0)     // Colossus near Leviathan
                        if (!creature->IsAlive() && GetBossState(BOSS_LEVIATHAN) != DONE)
                            creature->Respawn();
                    break;
                case NPC_VEHICLE_SIEGE:
                case NPC_VEHICLE_DEMOLISHER:
                case NPC_VEHICLE_CHOPPER:
                    if (GetBossState(BOSS_LEVIATHAN) == DONE)
                        creature->DespawnOrUnsummon();
                    VehicleGUIDs.push_back(creature->GetGUID());
                    break;
                // Ignis
                case NPC_IGNIS:
                    IgnisGUID = creature->GetGUID();
                    break;
                // Razorscale
                case NPC_RAZORSCALE:
                    RazorscaleGUID = creature->GetGUID();
                    break;
                case NPC_EXPEDITION_COMMANDER:
                    ExpeditionCommanderGUID = creature->GetGUID();
                    return;
                // XT-002
                case NPC_XT002:
                    Xt002GUID = creature->GetGUID();
                    break;
                // Assembly of Iron
                case NPC_STEELBREAKER:
                    AssemblyGUIDs[0] = creature->GetGUID();
                    break;
                case NPC_MOLGEIM:
                    AssemblyGUIDs[1] = creature->GetGUID();
                    break;
                case NPC_BRUNDIR:
                    AssemblyGUIDs[2] = creature->GetGUID();
                    break;
                // Kologarn
                case NPC_KOLOGARN:
                    KologarnGUID = creature->GetGUID();
                    break;
                // Auriaya
                case NPC_AURIAYA:
                    AuriayaGUID = creature->GetGUID();
                    break;
                case NPC_SANCTUM_SENTRY:
                     SanctumSentryGUIDs[SanctumSentryCounter++] = creature->GetGUID();
                     AddMinion(creature, true);
                     break;
                // Mimiron
                case NPC_MIMIRON:
                    MimironGUID = creature->GetGUID();
                    break;
                case NPC_LEVIATHAN_MKII:
                    LeviathanMKIIGUID = creature->GetGUID();
                    break;
                case NPC_VX_001:
                    Vx001GUID = creature->GetGUID();
                    break;
                case NPC_AERIAL_COMMAND_UNIT:
                    AerialUnitGUID = creature->GetGUID();
                    break;
                case NPC_MAGNETIC_CORE:
                    MagneticCoreGUID = creature->GetGUID();
                    break;
                // Hodir
                case NPC_HODIR:
                    HodirGUID = creature->GetGUID();
                    break;
                // Thorim
                case NPC_THORIM:
                    ThorimGUID = creature->GetGUID();
                    break;
                case NPC_THORIM_EVENT_BUNNY:
                    if (creature->GetWaypointPath())
                        SifBlizzardGUID = creature->GetGUID();
                    break;
                case NPC_RUNIC_COLOSSUS:
                    RunicColossusGUID = creature->GetGUID();
                    break;
                case NPC_RUNE_GIANT:
                    RuneGiantGUID = creature->GetGUID();
                    break;
                // Freya
                case NPC_FREYA:
                    FreyaGUID = creature->GetGUID();
                    break;
                case NPC_ELDER_BRIGHTLEAF:
                    ElderBrightleafGUID = creature->GetGUID();
                    creature->setActive(true);
                    break;
                case NPC_ELDER_IRONBRANCH:
                    ElderIronbranchGUID = creature->GetGUID();
                    creature->setActive(true);
                    break;
                case NPC_ELDER_STONEBARK:
                    ElderStonebarkGUID = creature->GetGUID();
                    creature->setActive(true);
                    break;
                case NPC_FREYA_ACHIEVE_TRIGGER:
                    FreyaAchieveTriggerGUID = creature->GetGUID();
                    creature->setActive(true);
                    break;
                // Vezax
                case NPC_VEZAX:
                    VezaxGUID = creature->GetGUID();
                    break;
                // Yogg-Saron
                case NPC_YOGGSARON:
                    YoggSaronGUID = creature->GetGUID();
                    break;
                case NPC_SARA:
                    SaraGUID = creature->GetGUID();
                    break;
                case NPC_KEEPER_WALKWAY_FREYA:
                    FreyaWalkwayGUID = creature->GetGUID();
                    break;
                case NPC_KEEPER_WALKWAY_MIMIRON:
                    MimironWalkwayGUID = creature->GetGUID();
                    break;
                case NPC_KEEPER_WALKWAY_THORIM:
                    ThorimWalkwayGUID = creature->GetGUID();
                    break;
                case NPC_KEEPER_WALKWAY_HODIR:
                    HodirWalkwayGUID = creature->GetGUID();
                    break;
                // Algalon
                case NPC_ALGALON:
                    if (AlgalonTimer > 0)
                        AlgalonGUID = creature->GetGUID();
                    else
                        creature->DespawnOrUnsummon();
                    break;
                case NPC_BRANN_BRONZBEARD_ALG:
                    BrannBronzebeardAlgGUID = creature->GetGUID();
                    break;
                //! These creatures are summoned by something else than Algalon
                //! but need to be controlled/despawned by him - so they need to be
                //! registered in his summon list
                case NPC_ALGALON_VOID_ZONE_VISUAL_STALKER:
                case NPC_ALGALON_STALKER_ASTEROID_TARGET_01:
                case NPC_ALGALON_STALKER_ASTEROID_TARGET_02:
                case NPC_UNLEASHED_DARK_MATTER:
                    if (Creature* algalon = instance->GetCreature(AlgalonGUID))
                        algalon->AI()->JustSummoned(creature);
                    break;
            }

        }

        void OnCreatureRemove(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case NPC_BRANN_BRONZBEARD_ALG:
                    if (BrannBronzebeardAlgGUID == creature->GetGUID())
                        BrannBronzebeardAlgGUID.Clear();
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GO_KOLOGARN_DOOR:
                case GO_THORIM_ENCOUNTER_DOOR:
                case GO_IRON_COUNCIL_ENTRANCE:
                case GO_ARCHIVUM_DOOR:
                case GO_HODIR_OUT_DOOR_ICE:
                case GO_HODIR_OUT_DOOR_STONE:
                case GO_HODIR_IN_DOOR_STONE:
                case GO_XT002_DOOR:
                case GO_MIMIRON_DOOR_1:
                case GO_MIMIRON_DOOR_2:
                case GO_MIMIRON_DOOR_3:
                case GO_VEZAX_DOOR:
                case GO_YOGGSARON_DOOR:
                case GO_LEVIATHAN_DOOR_N:
                case GO_LEVIATHAN_DOOR_S:
                    AddDoor(go, true);
                    break;
                case GO_KOLOGARN_CHEST:
                case GO_KOLOGARN_CHEST_HERO:
                    KologarnChestGUID = go->GetGUID();
                    break;
                case GO_KOLOGARN_BRIDGE:
                    KologarnBridgeGUID = go->GetGUID();
                    break;
                case GO_THORIM_STONE_DOOR:
                    StoneDoorGUID = go->GetGUID();
                    break;
                case GO_THORIM_RUNIC_DOOR:
                    RunicDoorGUID = go->GetGUID();
                    break;
                case GO_THORIM_LEVER:
                    ThorimLeverGUID = go->GetGUID();
                    break;
                case GO_LEVIATHAN_GATE:
                    LeviathanGateGUID = go->GetGUID();
                    if (GetBossState(BOSS_LEVIATHAN) == DONE)
                        go->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                    else
                        HandleGameObject(ObjectGuid::Empty, false, go);
                    break;
                case GO_PORTAL_DALARAN:
                     dalaranPortalGUID = go->GetGUID();
                    break;
                case GO_HODIR_RARE_CACHE_HERO:
                    HodirRareCacheHeroGUID = go->GetGUID();
                    if (GetBossState(BOSS_HODIR) == DONE)
                        go->SetPhaseMask(2, true);
                    break;
                case GO_HODIR_RARE_CACHE:
                    HodirRareCacheGUID = go->GetGUID();
                    if (GetBossState(BOSS_HODIR) == DONE)
                        go->SetPhaseMask(2, true);
                    break;
                case GO_HODIR_CHEST_HERO:
                    HodirCacheHeroGUID = go->GetGUID();
                    if (GetBossState(BOSS_HODIR) == DONE)
                        go->SetPhaseMask(2, true);
                    break;
                case GO_HODIR_CHEST:
                    HodirCacheGUID = go->GetGUID();
                    if (GetBossState(BOSS_HODIR) == DONE)
                        go->SetPhaseMask(2, true);
                    break;
                case GO_MIMIRON_TRAIN:
                    go->setActive(true);
                    MimironTrainGUID = go->GetGUID();
                    break;
                case GO_MIMIRON_ELEVATOR:
                    go->setActive(true);
                    MimironElevatorGUID = go->GetGUID();
                    break;
                case GO_WAY_TO_YOGG:
                    WayToYoggGUID = go->GetGUID();
                    if (CheckRequiredBosses(BOSS_VEZAX))
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED);
                    break;
                case GO_YOGGBRAIN_DOOR_1:
                    YoggSaronBrainDoor1GUID = go->GetGUID();
                    HandleGameObject(ObjectGuid::Empty, false, go);
                    break;
                case GO_YOGGBRAIN_DOOR_2:
                    YoggSaronBrainDoor2GUID = go->GetGUID();
                    HandleGameObject(ObjectGuid::Empty, false, go);
                    break;
                case GO_YOGGBRAIN_DOOR_3:
                    YoggSaronBrainDoor3GUID = go->GetGUID();
                    HandleGameObject(ObjectGuid::Empty, false, go);
                    break;
                case GO_MOLE_MACHINE:
                    if (GetBossState(BOSS_RAZORSCALE) == IN_PROGRESS)
                        go->SetGoState(GO_STATE_ACTIVE);
                    break;
                case GO_HODIR_STORM_GEN:
                    LeviathanTowerGUIDs[HODIR][0] = go->GetGUID();
                    break;
                case GO_FREYA_STORM_GEN:
                    LeviathanTowerGUIDs[FREYA][0] = go->GetGUID();
                    break;
                case GO_MIMIRON_STORM_GEN:
                    LeviathanTowerGUIDs[MIMIRON][0] = go->GetGUID();
                    break;
                case GO_THORIM_STORM_GEN:
                    LeviathanTowerGUIDs[THORIM][0] = go->GetGUID();
                    break;
                case GO_HODIR_TAR_CRYST:
                    LeviathanTowerGUIDs[HODIR][1] = go->GetGUID();
                    break;
                case GO_FREYA_TAR_CRYST:
                    LeviathanTowerGUIDs[FREYA][1] = go->GetGUID();
                    break;
                case GO_MIMIRON_TAR_CRYST:
                    LeviathanTowerGUIDs[MIMIRON][1] = go->GetGUID();
                    break;
                case GO_THORIM_TAR_CRYST:
                    LeviathanTowerGUIDs[THORIM][1] = go->GetGUID();
                    break;
                case GO_CELESTIAL_PLANETARIUM_ACCESS_10:
                case GO_CELESTIAL_PLANETARIUM_ACCESS_25:
                    if (AlgalonSummoned)
                        go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
                    break;
                case GO_DOODAD_UL_SIGILDOOR_01:
                    AlgalonSigilDoorGUID[0] = go->GetGUID();
                    if (AlgalonSummoned)
                        go->SetGoState(GO_STATE_ACTIVE);
                    break;
                case GO_DOODAD_UL_SIGILDOOR_02:
                    AlgalonSigilDoorGUID[1] = go->GetGUID();
                    break;
                case GO_DOODAD_UL_SIGILDOOR_03:
                    AlgalonSigilDoorGUID[2] = go->GetGUID();
                    AddDoor(go, true);
                    break;
                case GO_DOODAD_UL_UNIVERSEFLOOR_01:
                    AlgalonFloorGUID[0] = go->GetGUID();
                    AddDoor(go, true);
                    break;
                case GO_DOODAD_UL_UNIVERSEFLOOR_02:
                    AlgalonFloorGUID[1] = go->GetGUID();
                    AddDoor(go, true);
                    break;
                case GO_DOODAD_UL_UNIVERSEGLOBE01:
                    AlgalonUniverseGUID = go->GetGUID();
                    AddDoor(go, true);
                    break;
                case GO_DOODAD_UL_ULDUAR_TRAPDOOR_03:
                    AlgalonTrapdoorGUID = go->GetGUID();
                    AddDoor(go, true);
                    break;
                case GO_GIFT_OF_THE_OBSERVER_10:
                case GO_GIFT_OF_THE_OBSERVER_25:
                    break;
            }
        }

        void OnGameObjectRemove(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GO_KOLOGARN_BRIDGE:
                case GO_KOLOGARN_DOOR:
                case GO_THORIM_ENCOUNTER_DOOR:
                case GO_IRON_COUNCIL_ENTRANCE:
                case GO_ARCHIVUM_DOOR:
                case GO_HODIR_OUT_DOOR_ICE:
                case GO_HODIR_OUT_DOOR_STONE:
                case GO_HODIR_IN_DOOR_STONE:
                case GO_XT002_DOOR:
                case GO_MIMIRON_DOOR_1:
                case GO_MIMIRON_DOOR_2:
                case GO_MIMIRON_DOOR_3:
                case GO_VEZAX_DOOR:
                case GO_YOGGSARON_DOOR:
                case GO_LEVIATHAN_DOOR_N:
                case GO_LEVIATHAN_DOOR_S:
                case GO_DOODAD_UL_SIGILDOOR_03:
                case GO_DOODAD_UL_UNIVERSEFLOOR_01:
                case GO_DOODAD_UL_UNIVERSEFLOOR_02:
                case GO_DOODAD_UL_UNIVERSEGLOBE01:
                case GO_DOODAD_UL_ULDUAR_TRAPDOOR_03:
                    AddDoor(go, false);
                    break;
            }
        }

        void ProcessEvent(WorldObject* /*go*/, uint32 eventId)
        {
            if (eventId < EVENT_TOWER_OF_LIFE_DESTROYED || eventId > EVENT_TOWER_OF_FLAMES_DESTROYED)
                return;

            if (GetBossState(BOSS_LEVIATHAN) == DONE)
                return;

            uint8 index = eventId - EVENT_TOWER_OF_LIFE_DESTROYED;

            for (uint8 i = 0; i < 2; ++i)
                if (GameObject* go = instance->GetGameObject(LeviathanTowerGUIDs[index][i]))
                    go->UseDoorOrButton();
            LeviathanTowerState &= ~(1 << index);
        }

        bool SetBossState(uint32 type, EncounterState state)
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;

            switch (type)
            {
                case BOSS_LEVIATHAN:
                    if (state == DONE)
                    {
                        SummonLeviFinishNpc();
                        _events.ScheduleEvent(EVENT_DESPAWN_LEVIATHAN_VEHICLES, 5 * IN_MILLISECONDS);
                    }
                    else if (state == FAIL)
                    {
                        LeviathanEncountered = true;
                        uint8 count = instance->GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL? 2 : 5;
                        for (uint8 i = GetVehiclesInUse(NPC_VEHICLE_SIEGE, false); i < count; ++i)
                            instance->SummonCreature(NPC_VEHICLE_SIEGE, SiegeRespawnPositions[i]);
                        for (uint8 i = GetVehiclesInUse(NPC_VEHICLE_CHOPPER, false); i < count; ++i)
                            instance->SummonCreature(NPC_VEHICLE_CHOPPER, ChopperRespawnPositions[i]);
                        for (uint8 i = GetVehiclesInUse(NPC_VEHICLE_DEMOLISHER, false); i < count; ++i)
                            instance->SummonCreature(NPC_VEHICLE_DEMOLISHER, DemolisherRespawnPositions[i]);
                    }
                    break;
                case BOSS_ASSEMBLY:
                    if (state == NOT_STARTED)
                    {
                        SetData(DATA_ASSEMBLY_DEAD, 0);

                        for (uint8 i = 0; i < 3; ++i)
                            if (Creature* assemblyMember = instance->GetCreature(AssemblyGUIDs[i]))
                            {
                                assemblyMember->Respawn();
                                assemblyMember->GetGUID().GetCounter();

                                if (!assemblyMember->IsInCombat())
                                    assemblyMember->GetMotionMaster()->MoveTargetedHome();
                            }
                    }
                    else if (state == IN_PROGRESS)
                    {
                        SetData(DATA_ASSEMBLY_DEAD, 0);

                        for (uint8 i = 0; i < 3; ++i)
                            if (Creature* assemblyMember = instance->GetCreature(AssemblyGUIDs[i]))
                                if (assemblyMember->isDead())
                                    assemblyMember->Respawn();
                    }
                    break;
                case BOSS_KOLOGARN:
                    if (GameObject* bridge = instance->GetGameObject(KologarnBridgeGUID))
                        bridge->SetGoState(state == DONE ? GO_STATE_READY : GO_STATE_ACTIVE);

                    if (state == DONE)
                        DoRespawnGameObject(KologarnChestGUID, 0);  // Spawn chest
                    break;  
                // make helper NPCs on walkway visible if boss is done
                case BOSS_FREYA:
                    if (state == DONE)
                        instance->SummonCreature(NPC_KEEPER_WALKWAY_FREYA, walkwayKeeperSpawnLocation[1]);
                    break;
                case BOSS_MIMIRON:
                    if (state == DONE)
                        instance->SummonCreature(NPC_KEEPER_WALKWAY_MIMIRON, walkwayKeeperSpawnLocation[0]);
                    break;
                case BOSS_THORIM:
                    if (state == DONE)
                        instance->SummonCreature(NPC_KEEPER_WALKWAY_THORIM, walkwayKeeperSpawnLocation[2]);
                    break;
                case BOSS_HODIR:
                    if (state == DONE)
                        instance->SummonCreature(NPC_KEEPER_WALKWAY_HODIR, walkwayKeeperSpawnLocation[3]);
                    break;
                case BOSS_ALGALON:
                    switch (state)
                    {
                        case IN_PROGRESS:
                            if (!instance->Is25ManRaid())
                            {
                                HasOnlyAllowedItems = true;
                                DoSendNotifyToInstance("|cFFFFFC00 [Herald of the Titans!] |cFF00FFFF Enabled");
                                CheckForItemLevel();
                            }
                            if (AlgalonTimer >= 60) // only execute the following on the first pull
                            {
                                DoUpdateWorldState(WORLD_STATE_ALGALON_TIMER_ENABLED, 1);
                                AlgalonTimer = 60;
                                DoUpdateWorldState(WORLD_STATE_ALGALON_DESPAWN_TIMER, AlgalonTimer);
                                _events.ScheduleEvent(EVENT_UPDATE_ALGALON_TIMER, 60*IN_MILLISECONDS);
                                SaveToDB();
                            }
                            break;
                        case DONE:
                            _events.CancelEvent(EVENT_UPDATE_ALGALON_TIMER);
                            DoUpdateWorldState(WORLD_STATE_ALGALON_TIMER_ENABLED, 0);
                            AlgalonTimer = 0;
                            CheckForItemLevel();
                            if (HasOnlyAllowedItems && instance->GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL)
                            {
                                DoCompleteCriteria(CRITERIA_HERALD_OF_TITANS);
                                DoSendNotifyToInstance("|cFFFFFC00 [Herald of the Titans!] |cFF00FFFF Succesful");
                            }
                            break;
                        case NOT_STARTED:
                            if (!instance->Is25ManRaid())
                            {
                                DoSendNotifyToInstance("|cFFFFFC00 [Herald of the Titans!] |cFF00FFFF Enabled");
                                HasOnlyAllowedItems = true;
                            }
                            break;
                        default:
                            break;
                    }
                    break;
            }

            if (CheckRequiredBosses(BOSS_VEZAX))
                 if (GameObject* go = instance->GetGameObject(WayToYoggGUID))
                     go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED);

            SaveToDB();
            return true;
        }

        bool CheckRequiredBosses(uint32 bossId, Player const* /*player*/ = NULL) const
        {
            switch (bossId)
            {
                case BOSS_LEVIATHAN:
                    return true;
                case BOSS_IGNIS:
                case BOSS_XT002:
                case BOSS_RAZORSCALE:
                    return GetBossState(BOSS_LEVIATHAN) == DONE;
                case BOSS_ASSEMBLY:
                case BOSS_KOLOGARN:
                    return GetBossState(BOSS_XT002) == DONE;
                case BOSS_AURIAYA:
                case BOSS_FREYA:
                case BOSS_ELDER_BRIGHTLEAF:
                case BOSS_ELDER_IRONBRANCH:
                case BOSS_ELDER_STONEBARK:
                case BOSS_HODIR:
                case BOSS_THORIM:
                case BOSS_MIMIRON:
                    return GetBossState(BOSS_KOLOGARN) == DONE;
                case BOSS_VEZAX:
                    return GetBossState(BOSS_FREYA) == DONE && GetBossState(BOSS_HODIR) == DONE &&
                        GetBossState(BOSS_THORIM) == DONE &&  GetBossState(BOSS_MIMIRON) == DONE;
                case BOSS_YOGGSARON:
                    return GetBossState(BOSS_VEZAX) == DONE;
                case BOSS_ALGALON:
                    return GetBossState(BOSS_XT002) == DONE;
                default:
                    return false;
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_LEVIATHAN_HARDMODE:
                    LeviathanTowerState = THORIM_TOWER | HODIR_TOWER | FREYA_TOWER | MIMIRON_TOWER;
                    break;
                case DATA_COLOSSUS:
                    if (++ColossusCount >= 2)
                        _events.ScheduleEvent(EVENT_LEVIATHAN_DRIVE_OUT, 10*IN_MILLISECONDS);
                    break;
                case DATA_ASSEMBLY_DEAD:
                    if (data == 1)
                        AssemlyDeadCount++;
                    else if (data == 0)
                        AssemlyDeadCount = 0;
                    break;
                case DATA_CALL_TRAM:
                    if (GameObject* go = instance->GetGameObject(MimironTrainGUID))
                        go->UseDoorOrButton();
                    break;
                case DATA_MIMIRON_ELEVATOR:
                    if (GameObject* go = instance->GetGameObject(MimironElevatorGUID))
                        go->SetGoState(GOState(data));
                    break;
                case DATA_RUNIC_DOOR:
                    if (GameObject* go = instance->GetGameObject(RunicDoorGUID))
                        go->SetGoState(GOState(data));
                    break;
                case DATA_STONE_DOOR:
                    if (GameObject* go = instance->GetGameObject(StoneDoorGUID))
                        go->SetGoState(GOState(data));
                    break;
                case DATA_ADD_HELP_FLAG:
                    SupportKeeperFlag = SupportKeeperFlag | (1 << data);
                    SaveToDB();
                    break;
                case DATA_MIMIRON_TELEPORTER:
                    MimironTeleporterActivated = true;
                    break;
                case DATA_ALGALON_SUMMONED:
                    AlgalonSummoned = true;
                    SaveToDB();
                    break;
                case DATA_DWARF_COUNT:
                    if (DwarfCount == 0)
                        _events.ScheduleEvent(EVENT_DWARFAGEDDON_TIMER, 10*IN_MILLISECONDS);
                    DwarfCount += data;
                    break;
                default:
                    break;
            }
        }

        ObjectGuid GetGuidData(uint32 data) const
        {
            switch (data)
            {
                case BOSS_LEVIATHAN:            return LeviathanGUID;
                case BOSS_IGNIS:                return IgnisGUID;
                case BOSS_RAZORSCALE:           return RazorscaleGUID;
                case BOSS_XT002:                return Xt002GUID;
                case BOSS_KOLOGARN:             return KologarnGUID;
                // Auriaya
                case BOSS_AURIAYA:              return AuriayaGUID;
                case DATA_SANCTUM_SENTRY_0:
                case DATA_SANCTUM_SENTRY_1:
                case DATA_SANCTUM_SENTRY_2:
                case DATA_SANCTUM_SENTRY_3:
                    return SanctumSentryGUIDs[data - DATA_SANCTUM_SENTRY_0];

                // Mimiron
                case BOSS_MIMIRON:              return MimironGUID;
                case DATA_LEVIATHAN_MK_II:      return LeviathanMKIIGUID;
                case DATA_VX_001:               return Vx001GUID;
                case DATA_AERIAL_UNIT:          return AerialUnitGUID;
                case DATA_MAGNETIC_CORE:        return MagneticCoreGUID;

                case BOSS_HODIR:                return HodirGUID;
                case DATA_HODIR_RARE_CACHE_HERO:return HodirRareCacheHeroGUID;
                case DATA_HODIR_RARE_CACHE:     return HodirRareCacheGUID;
                case DATA_HODIR_CHEST_HERO:     return HodirCacheHeroGUID;
                case DATA_HODIR_CHEST:          return HodirCacheGUID;

                // Thorim
                case BOSS_THORIM:               return ThorimGUID;
                case DATA_RUNIC_COLOSSUS:       return RunicColossusGUID;
                case DATA_RUNE_GIANT:           return RuneGiantGUID;
                case DATA_SIF_BLIZZARD:         return SifBlizzardGUID;
                case DATA_THORIM_LEVER:         return ThorimLeverGUID;

                // Freya
                case BOSS_FREYA:                return FreyaGUID;
                case BOSS_ELDER_BRIGHTLEAF:     return ElderBrightleafGUID;
                case BOSS_ELDER_IRONBRANCH:     return ElderIronbranchGUID;
                case BOSS_ELDER_STONEBARK:      return ElderStonebarkGUID;

                case BOSS_VEZAX:                return VezaxGUID;

                // Yogg-Saron
                case BOSS_YOGGSARON:            return YoggSaronGUID;
                case DATA_SARA:                 return SaraGUID;
                case GO_YOGGBRAIN_DOOR_1:       return YoggSaronBrainDoor1GUID;
                case GO_YOGGBRAIN_DOOR_2:       return YoggSaronBrainDoor2GUID;
                case GO_YOGGBRAIN_DOOR_3:       return YoggSaronBrainDoor3GUID;

                // Algalon
                case BOSS_ALGALON:
                    return AlgalonGUID;
                case DATA_SIGILDOOR_01:
                    return AlgalonSigilDoorGUID[0];
                case DATA_SIGILDOOR_02:
                    return AlgalonSigilDoorGUID[1];
                case DATA_SIGILDOOR_03:
                    return AlgalonSigilDoorGUID[2];
                case DATA_UNIVERSE_FLOOR_01:
                    return AlgalonFloorGUID[0];
                case DATA_UNIVERSE_FLOOR_02:
                    return AlgalonFloorGUID[1];
                case DATA_UNIVERSE_GLOBE:
                    return AlgalonUniverseGUID;
                case DATA_ALGALON_TRAPDOOR:
                    return AlgalonTrapdoorGUID;
                case DATA_BRANN_BRONZEBEARD_ALG:
                    return BrannBronzebeardAlgGUID;
                // razorscale expedition commander
                case DATA_EXP_COMMANDER:        return ExpeditionCommanderGUID;
                // Assembly of Iron
                case DATA_STEELBREAKER:         return AssemblyGUIDs[0];
                case DATA_MOLGEIM:              return AssemblyGUIDs[1];
                case DATA_BRUNDIR:              return AssemblyGUIDs[2];

                case DATA_BRAIN_DOOR_1 :        return YoggSaronBrainDoor1GUID;
                case DATA_BRAIN_DOOR_2 :        return YoggSaronBrainDoor2GUID;
                case DATA_BRAIN_DOOR_3 :        return YoggSaronBrainDoor3GUID;
            }

            return ObjectGuid::Empty;
        }

        uint32 GetData(uint32 type) const
        {
            switch (type)
            {
                case DATA_LEVIATHAN_HARDMODE:
                    return LeviathanTowerState;
                case DATA_COLOSSUS:
                    return ColossusCount;
                case DATA_ASSEMBLY_DEAD:
                    return AssemlyDeadCount;
                case DATA_KEEPER_SUPPORT_YOGG:
                    return SupportKeeperFlag;
                case DATA_PLAYER_DEATH_MASK:
                    return PlayerEncounterDeaths;
                case DATA_DWARF_COUNT:
                    return DwarfCount;
                case DATA_RAID_IS_ALLIANZ:
                    return IsAllianz ? 1 : 0;
                case DATA_MIMIRON_TELEPORTER:
                    return (uint32) MimironTeleporterActivated;
                case DATA_ALGALON_SUMMONED:
                    return (uint32) AlgalonSummoned;
            }

            return 0;
        }

        void OnPlayerEquipItem(Player* player, Item* item, EquipmentSlots slot)
        {
            if (!HasOnlyAllowedItems || instance->Is25ManRaid() || player->IsGameMaster() || !IsEncounterInProgress() || instance->GetDifficulty() != RAID_DIFFICULTY_10MAN_NORMAL)
                return;

            // only weapons can be changed infight
            if (slot != EQUIPMENT_SLOT_MAINHAND && slot != EQUIPMENT_SLOT_OFFHAND && slot != EQUIPMENT_SLOT_RANGED)
                return;

            if (item->GetTemplate()->ItemLevel > MAX_HERALD_WEAPON_ITEMLEVEL)
            {
                HasOnlyAllowedItems = false;
                DoSendNotifyToInstance("|cFFFFFC00 [Herald of the Titans!] |cFF00FFFF Failed item: %u.", item->GetEntry());
            }
        }

        void CheckForItemLevel()
        {
            for (Player& player : instance->GetPlayers())
            {
                for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
                {
                    if (slot == EQUIPMENT_SLOT_TABARD || slot == EQUIPMENT_SLOT_BODY)
                        continue;

                    if (Item* item = player.GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                    {
                        if (slot >= EQUIPMENT_SLOT_MAINHAND && slot <= EQUIPMENT_SLOT_RANGED)
                        {
                            if (item->GetTemplate()->ItemLevel > MAX_HERALD_WEAPON_ITEMLEVEL)
                            {
                                HasOnlyAllowedItems = false;
                                DoSendNotifyToInstance("|cFFFFFC00 [Herald of the Titans!] |cFF00FFFF Failed item: %u.", item->GetEntry());
                            }
                        }
                        else if (item->GetTemplate()->ItemLevel > MAX_HERALD_ARMOR_ITEMLEVEL)
                        {
                            HasOnlyAllowedItems = false;
                            DoSendNotifyToInstance("|cFFFFFC00 [Herald of the Titans!] |cFF00FFFF Failed item: %u.", item->GetEntry());
                        }
                    }
                }
            }
        }

        void ResetInstance()
        {
            const ObjectGuid bossGUID[18] = { LeviathanGUID, IgnisGUID, RazorscaleGUID, Xt002GUID, AssemblyGUIDs[0],
                                          AssemblyGUIDs[1], AssemblyGUIDs[2], KologarnGUID,
                                          AuriayaGUID, SanctumSentryGUIDs[0], SanctumSentryGUIDs[1],
                                          SanctumSentryGUIDs[2], SanctumSentryGUIDs[3], HodirGUID, MimironGUID,
                                          ThorimGUID, FreyaGUID, VezaxGUID };

            uint8 counter = 0;
            for (Player& player : instance->GetPlayers())
            {
                if (player.IsAlive() && !player.IsGameMaster())
                    ++counter;
            }

            // Called when player leaves the instance, executed before actual removal
            // So when the last player leaves the counter is 1 and becomes 0 later
            if (counter > 1)
                return;

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                uint32 tmpState = GetBossState(i);
                if (tmpState == IN_PROGRESS || tmpState > SPECIAL)
                    SetBossState(i, NOT_STARTED);
            }

            for (uint8 i = 0; i < 18; ++i)
            {
                if (Creature* boss = instance->GetCreature(bossGUID[i]))
                    boss->AI()->EnterEvadeMode();
            }
        }

        void WriteSaveDataMore(std::ostringstream& data) override
        {
            data << PlayerEncounterDeaths  << ' ' // Achievement
                << LeviathanTowerState     << ' ' // Leviathan HM mode
                << SupportKeeperFlag       << ' ' // Yogg HM mode
                << AlgalonTimer            << ' '
                << (uint32)AlgalonSummoned << ' '
                << (uint32)MimironTeleporterActivated;
        }

        void ReadSaveDataMore(std::istringstream& data) override
        {
            data >> PlayerEncounterDeaths;
            data >> LeviathanTowerState;

            // boss done and talked with -> spawn in yogg room
            // boss done, but not used -> spawn in walkway
            data >> SupportKeeperFlag;
            if (GetBossState(BOSS_FREYA) == DONE) 
            {
                if (SupportKeeperFlag & (1 << FREYA))
                    instance->SummonCreature(NPC_KEEPER_YOGGROOM_FREYA, yoggroomKeeperSpawnLocation[1]);
                else
                    instance->SummonCreature(NPC_KEEPER_WALKWAY_FREYA, walkwayKeeperSpawnLocation[1]);
            }
            if (GetBossState(BOSS_MIMIRON) == DONE) 
            {
                if (SupportKeeperFlag & (1 << MIMIRON))
                    instance->SummonCreature(NPC_KEEPER_YOGGROOM_MIMIRON, yoggroomKeeperSpawnLocation[0]);
                else
                    instance->SummonCreature(NPC_KEEPER_WALKWAY_MIMIRON, walkwayKeeperSpawnLocation[0]);
            }
            if (GetBossState(BOSS_THORIM) == DONE) 
            {
                if (SupportKeeperFlag & (1 << THORIM))
                    instance->SummonCreature(NPC_KEEPER_YOGGROOM_THORIM, yoggroomKeeperSpawnLocation[2]);
                else
                    instance->SummonCreature(NPC_KEEPER_WALKWAY_THORIM, walkwayKeeperSpawnLocation[2]);
            }
            if (GetBossState(BOSS_HODIR) == DONE) 
            {
                if (SupportKeeperFlag & (1 << HODIR))
                    instance->SummonCreature(NPC_KEEPER_YOGGROOM_HODIR, yoggroomKeeperSpawnLocation[3]);
                else
                    instance->SummonCreature(NPC_KEEPER_WALKWAY_HODIR, walkwayKeeperSpawnLocation[3]);
            }

            data >> AlgalonTimer;
            uint32 tmp;
            data >> tmp;
            AlgalonSummoned = (bool)tmp;

            if (AlgalonSummoned && AlgalonTimer) 
            {   // algalon was summoned, is not failed, is not defeated
                if (Creature* algalon = instance->SummonCreature(NPC_ALGALON, algalonSpawnLocation))
                    algalon->AI()->DoAction(ACTION_ALGALON_SPAWN_INTRO);

                if (AlgalonTimer <= 60)  {
                    _events.ScheduleEvent(EVENT_UPDATE_ALGALON_TIMER, 60000);
                    DoUpdateWorldState(WORLD_STATE_ALGALON_TIMER_ENABLED, 1);
                    DoUpdateWorldState(WORLD_STATE_ALGALON_DESPAWN_TIMER, AlgalonTimer);
                }
                else {
                    DoUpdateWorldState(WORLD_STATE_ALGALON_TIMER_ENABLED, 0);
                }
            }
            else {
                DoUpdateWorldState(WORLD_STATE_ALGALON_TIMER_ENABLED, 0);
            }

            tmp = 0;
            data >> tmp;
            MimironTeleporterActivated = (bool)tmp;
        }

        void Update(uint32 diff)
        {
            if (_events.Empty())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_UPDATE_ALGALON_TIMER:
                        AlgalonTimer--;
                        DoUpdateWorldState(WORLD_STATE_ALGALON_DESPAWN_TIMER, AlgalonTimer);
                        if (AlgalonTimer)
                            _events.RescheduleEvent(EVENT_UPDATE_ALGALON_TIMER, 60*IN_MILLISECONDS);
                        else
                        {
                            DoUpdateWorldState(WORLD_STATE_ALGALON_TIMER_ENABLED, 0);
                            if (Creature* algalon = instance->GetCreature(AlgalonGUID))
                                algalon->AI()->DoAction(ACTION_ALGALON_SEND_REPLYCODE);
                        }
                        SaveToDB();
                        break;
                    case EVENT_LEVIATHAN_DRIVE_OUT:
                        if (Creature* leviathan = instance->GetCreature(LeviathanGUID))
                            leviathan->AI()->DoAction(ACTION_DRIVE_TO_CENTER);
                        break;
                    case EVENT_DWARFAGEDDON_TIMER:
                        if (DwarfCount >= 100)
                            DoCompleteAchievement(instance->GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL ? ACHIEVEMENT_DWARFAGEDDON_10 : ACHIEVEMENT_DWARFAGEDDON_25 );
                        DwarfCount = 0;
                        break;
                    case EVENT_LEVIATHAN_VEHICLE_CHECK:
                        if (!IsEncounterInProgress())
                        {
                            uint8 count = instance->GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL? 2 : 5;
                            if (LeviathanEncountered)
                            {
                                for (uint8 i = GetVehiclesInUse(NPC_VEHICLE_SIEGE, true); i < count; ++i)
                                    instance->SummonCreature(NPC_VEHICLE_SIEGE, SiegeRespawnPositions[i]);
                                for (uint8 i = GetVehiclesInUse(NPC_VEHICLE_CHOPPER, true); i < count; ++i)
                                    instance->SummonCreature(NPC_VEHICLE_CHOPPER, ChopperRespawnPositions[i]);
                                for (uint8 i = GetVehiclesInUse(NPC_VEHICLE_DEMOLISHER, true); i < count; ++i)
                                    instance->SummonCreature(NPC_VEHICLE_DEMOLISHER, DemolisherRespawnPositions[i]);
                            }
                            else
                            {
                                for (uint8 i = GetVehiclesInUse(NPC_VEHICLE_SIEGE, true); i < count; ++i)
                                    instance->SummonCreature(NPC_VEHICLE_SIEGE, SiegePositions[i]);
                                for (uint8 i = GetVehiclesInUse(NPC_VEHICLE_CHOPPER, true); i < count; ++i)
                                    instance->SummonCreature(NPC_VEHICLE_CHOPPER, ChopperPositions[i]);
                                for (uint8 i = GetVehiclesInUse(NPC_VEHICLE_DEMOLISHER, true); i < count; ++i)
                                    instance->SummonCreature(NPC_VEHICLE_DEMOLISHER, DemolisherPositions[i]);
                            }
                        }
                        if (GetBossState(BOSS_XT002) != DONE)
                            _events.ScheduleEvent(EVENT_LEVIATHAN_VEHICLE_CHECK, 60*IN_MILLISECONDS);
                        break;
                    case EVENT_DESPAWN_LEVIATHAN_VEHICLES:
                        // Eject all players from vehicles and make them untargetable.
                        // They will be despawned after a while
                        for (auto const& vehicleGuid : VehicleGUIDs)
                        {
                            if (Creature* vehicleCreature = instance->GetCreature(vehicleGuid))
                            {
                                if (Vehicle* vehicle = vehicleCreature->GetVehicleKit())
                                {
                                    vehicle->RemoveAllPassengers();
                                    vehicleCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                    vehicleCreature->DespawnOrUnsummon(5 * MINUTE * IN_MILLISECONDS);
                                }
                            }
                        }
                        break;
                }
            }
        }

        void OnUnitDeath(Unit* unit)
        {
            if (unit->GetTypeId() == TYPEID_PLAYER)
            {
                //! Assembly HM needs spezial handling here
                if (GetBossState(BOSS_ASSEMBLY) == IN_PROGRESS)
                    if (Creature* steelbreaker = instance->GetCreature(ObjectGuid(GetGuidData(DATA_STEELBREAKER))))
                        if (steelbreaker->IsAlive())
                            steelbreaker->AI()->DoAction(ACTION_ASSEMBLY_PLAYER_DIED);

                //! No Death in first try
                for (uint8 i = BOSS_LEVIATHAN; i < MAX_ENCOUNTER; ++i)
                    if (!(PlayerEncounterDeaths & ( 1 << i)) && GetBossState(i) == IN_PROGRESS)
                    {
                        PlayerEncounterDeaths |= 1 << i;
                        SaveToDB();
                        break;
                    }
            }
            else if (Creature* creature = unit->ToCreature())
            {
                switch (creature->GetEntry())
                {
                    case NPC_CORRUPTED_SERVITOR:
                    case NPC_MISGUIDED_NYMPH:
                    case NPC_GUARDIAN_LASHER:
                    case NPC_FOREST_SWARMER:
                    case NPC_MANGROVE_ENT:
                    case NPC_IRONROOT_LASHER:
                    case NPC_NATURES_BLADE:
                    case NPC_GUARDIAN_OF_LIFE:
                        if (!conSpeedAtory)
                        {
                            DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, CRITERIA_CON_SPEED_ATORY);
                            conSpeedAtory = true;
                        }
                        break;
                    case NPC_STEELFORGED_DEFFENDER:
                        SetData(DATA_DWARF_COUNT, 1);
                        break;
                    case NPC_ELDER_IRONBRANCH:
                    case NPC_ELDER_STONEBARK:
                    case NPC_ELDER_BRIGHTLEAF:
                        if (Creature* trigger = instance->GetCreature(FreyaAchieveTriggerGUID))
                            trigger->AI()->SetData(1, 1);
                        break;
                    default:
                        break;
                }
            }
        }

        uint8 GetVehiclesInUse(uint32 VehicleId, bool OnlyDespawnDead)
        {
            uint8 VehicleCount = 0;
            std::list<Creature*> Remove;
            for (std::list<ObjectGuid>::iterator it = VehicleGUIDs.begin(); it != VehicleGUIDs.end(); ++it)
            {
                Creature* creature = instance->GetCreature(*it);
                if (!creature)
                    continue;

                if (creature->GetEntry() == VehicleId)
                {
                    if (((OnlyDespawnDead && creature->IsAlive()) || creature->IsControlledByPlayer()))
                        VehicleCount++;
                    else
                    {
                        creature->DisappearAndDie();
                        Remove.push_back(creature);
                    }
                }
            }
            for (std::list<Creature*>::iterator it = Remove.begin(); it != Remove.end(); ++it)
                VehicleGUIDs.remove((*it)->GetGUID());
            return VehicleCount;
        }

        void SummonLeviFinishNpc()
        {
            for (uint8 i = 0; i < 6; ++i)
                instance->SummonCreature(NPC_EXPEDITION_MERCENARY, LeviathanFinishPositions[i]);
            for (uint8 i = 6; i < 12; ++i)
                instance->SummonCreature(NPC_EXPEDITION_ENGINEER, LeviathanFinishPositions[i]);

            instance->SummonCreature(NPC_MAGE, LeviathanFinishPositions[12]);
            instance->SummonCreature(NPC_MAGE, LeviathanFinishPositions[13]);

            if (GameObject* obj = instance->GetGameObject(dalaranPortalGUID))
                obj->SetPhaseMask(PHASEMASK_NORMAL, true);
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_ulduar_InstanceMapScript(map);
    }
};

void AddSC_instance_ulduar()
{
    new instance_ulduar();
}
