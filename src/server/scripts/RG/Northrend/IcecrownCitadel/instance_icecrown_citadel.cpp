/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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
#include "InstanceScript.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Pet.h"
#include "PoolMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Transport.h"
#include "TransportMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "icecrown_citadel.h"
#include "area_plague_works.hpp"

enum EventIds
{
    EVENT_PLAYERS_GUNSHIP_SPAWN     = 22663,
    EVENT_PLAYERS_GUNSHIP_COMBAT    = 22664,
    EVENT_PLAYERS_GUNSHIP_SAURFANG  = 22665,
    EVENT_ENEMY_GUNSHIP_COMBAT      = 22860,
    EVENT_ENEMY_GUNSHIP_DESPAWN     = 22861,
    EVENT_FILL_GREEN_TUBES          = 23426,
    EVENT_QUAKE                     = 23437,
    EVENT_FILL_ORANGE_TUBES         = 23438,
    EVENT_SECOND_REMORSELESS_WINTER = 23507,
    EVENT_TELEPORT_TO_FROSMOURNE    = 23617,
};

enum TimedEvents
{
    EVENT_UPDATE_EXECUTION_TIME = 1,
    EVENT_QUAKE_SHATTER         = 2,
    EVENT_REBUILD_PLATFORM      = 3,
    EVENT_RESPAWN_GUNSHIP       = 4,
    EVENT_FILL_GREEN_DOOR       = 5,
    EVENT_FILL_ORANGE_DOOR      = 6,
    EVENT_OPEN_PUTRICIDE_DOOR   = 7,
    EVENT_FADE_LEFT_ATTEMTS     = 8,
    EVENT_SPAWN_DAMNED          = 9,
    // SpiderEvent
    EVENT_SPIDER_ATTACK         = 10,
    EVENT_RESPAWN_SAURFANG      = 11,
    EVENT_BLOOD_WING_ENTRANCE   = 12
};

enum WPDatas
{
    WP_THE_DAMNED_RIGHT         = 3701100,
    WP_THE_DAMNED_LEFT          = 3701101
};

enum Spells
{
    SPELL_MARK_OF_THE_FALLEN_CHAMPION = 72293,
    SPELL_ESSENCE_OF_THE_BLOODQUEEN   = 70871,
    SPELL_SHADOW_RESONANCE_RESIST     = 71822,
    SPELL_ANTI_MAGIC_SHELL            = 48707,
    SPELL_FRENZY                      = 70923
};

DoorData const doorData[] =
{
    {GO_LORD_MARROWGAR_S_ENTRANCE,           DATA_LORD_MARROWGAR,        DOOR_TYPE_ROOM       },
    {GO_ICEWALL,                             DATA_LORD_MARROWGAR,        DOOR_TYPE_PASSAGE    },
    {GO_DOODAD_ICECROWN_ICEWALL02,           DATA_LORD_MARROWGAR,        DOOR_TYPE_PASSAGE    },
    {GO_ORATORY_OF_THE_DAMNED_ENTRANCE,      DATA_LADY_DEATHWHISPER,     DOOR_TYPE_ROOM       },
    {GO_SAURFANG_S_DOOR,                     DATA_DEATHBRINGER_SAURFANG, DOOR_TYPE_PASSAGE    },
    {GO_ORANGE_PLAGUE_MONSTER_ENTRANCE,      DATA_FESTERGUT,             DOOR_TYPE_ROOM       },
    {GO_GREEN_PLAGUE_MONSTER_ENTRANCE,       DATA_ROTFACE,               DOOR_TYPE_ROOM       },
    {GO_SCIENTIST_ENTRANCE,                  DATA_PROFESSOR_PUTRICIDE,   DOOR_TYPE_ROOM       },
    {GO_CRIMSON_HALL_DOOR,                   DATA_BLOOD_PRINCE_COUNCIL,  DOOR_TYPE_ROOM       },
    {GO_BLOOD_ELF_COUNCIL_DOOR,              DATA_BLOOD_PRINCE_COUNCIL,  DOOR_TYPE_PASSAGE    },
    {GO_BLOOD_ELF_COUNCIL_DOOR_RIGHT,        DATA_BLOOD_PRINCE_COUNCIL,  DOOR_TYPE_PASSAGE    },
    {GO_DOODAD_ICECROWN_BLOODPRINCE_DOOR_01, DATA_BLOOD_QUEEN_LANA_THEL, DOOR_TYPE_ROOM       },
    {GO_DOODAD_ICECROWN_GRATE_01,            DATA_BLOOD_QUEEN_LANA_THEL, DOOR_TYPE_PASSAGE    },
    {GO_GREEN_DRAGON_BOSS_ENTRANCE,          DATA_SISTER_SVALNA,         DOOR_TYPE_PASSAGE    },
    {GO_GREEN_DRAGON_BOSS_ENTRANCE,          DATA_VALITHRIA_DREAMWALKER, DOOR_TYPE_ROOM       },
    {GO_GREEN_DRAGON_BOSS_EXIT,              DATA_VALITHRIA_DREAMWALKER, DOOR_TYPE_PASSAGE    },
    {GO_DOODAD_ICECROWN_ROOSTPORTCULLIS_01,  DATA_VALITHRIA_DREAMWALKER, DOOR_TYPE_SPAWN_HOLE },
    {GO_DOODAD_ICECROWN_ROOSTPORTCULLIS_02,  DATA_VALITHRIA_DREAMWALKER, DOOR_TYPE_SPAWN_HOLE },
    {GO_DOODAD_ICECROWN_ROOSTPORTCULLIS_03,  DATA_VALITHRIA_DREAMWALKER, DOOR_TYPE_SPAWN_HOLE },
    {GO_DOODAD_ICECROWN_ROOSTPORTCULLIS_04,  DATA_VALITHRIA_DREAMWALKER, DOOR_TYPE_SPAWN_HOLE },
    {GO_SINDRAGOSA_ENTRANCE_DOOR,            DATA_SINDRAGOSA,            DOOR_TYPE_ROOM       },
    {GO_SINDRAGOSA_SHORTCUT_ENTRANCE_DOOR,   DATA_SINDRAGOSA,            DOOR_TYPE_PASSAGE    },
    {GO_SINDRAGOSA_SHORTCUT_EXIT_DOOR,       DATA_SINDRAGOSA,            DOOR_TYPE_PASSAGE    },
    {GO_ICE_WALL,                            DATA_SINDRAGOSA,            DOOR_TYPE_ROOM       },
    {GO_ICE_WALL,                            DATA_SINDRAGOSA,            DOOR_TYPE_ROOM       },
    {0,                                      0,                          DOOR_TYPE_ROOM       }, // END
};

// this doesnt have to only store questgivers, also can be used for related quest spawns
struct WeeklyQuest
{
    uint32 creatureEntry;
    uint32 questId[2];  // 10 and 25 man versions
};

// when changing the content, remember to update SetData, DATA_BLOOD_QUICKENING_STATE case for NPC_ALRIN_THE_AGILE index
WeeklyQuest const WeeklyQuestData[WeeklyNPCs] =
{
    {NPC_INFILTRATOR_MINCHAR,         {QUEST_DEPROGRAMMING_10,                 QUEST_DEPROGRAMMING_25                }}, // Deprogramming
    {NPC_KOR_KRON_LIEUTENANT,         {QUEST_SECURING_THE_RAMPARTS_10,         QUEST_SECURING_THE_RAMPARTS_25        }}, // Securing the Ramparts
    {NPC_ROTTING_FROST_GIANT_10,      {QUEST_SECURING_THE_RAMPARTS_10,         QUEST_SECURING_THE_RAMPARTS_25        }}, // Securing the Ramparts
    {NPC_ROTTING_FROST_GIANT_25,      {QUEST_SECURING_THE_RAMPARTS_10,         QUEST_SECURING_THE_RAMPARTS_25        }}, // Securing the Ramparts
    {NPC_ALCHEMIST_ADRIANNA,          {QUEST_RESIDUE_RENDEZVOUS_10,            QUEST_RESIDUE_RENDEZVOUS_25           }}, // Residue Rendezvous
    {NPC_ALRIN_THE_AGILE,             {QUEST_BLOOD_QUICKENING_10,              QUEST_BLOOD_QUICKENING_25             }}, // Blood Quickening
    {NPC_INFILTRATOR_MINCHAR_BQ,      {QUEST_BLOOD_QUICKENING_10,              QUEST_BLOOD_QUICKENING_25             }}, // Blood Quickening
    {NPC_MINCHAR_BEAM_STALKER,        {QUEST_BLOOD_QUICKENING_10,              QUEST_BLOOD_QUICKENING_25             }}, // Blood Quickening
    {NPC_VALITHRIA_DREAMWALKER_QUEST, {QUEST_RESPITE_FOR_A_TORNMENTED_SOUL_10, QUEST_RESPITE_FOR_A_TORNMENTED_SOUL_25}}, // Respite for a Tormented Soul
};

// NPCs spawned at Light's Hammer on Lich King dead
Position const JainaSpawnPos    = { -48.65278f, 2211.026f, 27.98586f, 3.124139f };
Position const MuradinSpawnPos  = { -47.34549f, 2208.087f, 27.98586f, 3.106686f };
Position const UtherSpawnPos    = { -26.58507f, 2211.524f, 30.19898f, 3.124139f };
Position const SylvanasSpawnPos = { -41.45833f, 2222.891f, 27.98586f, 3.647738f };

// SpiderEvent
Position const BroodlingLoc[8] =
{
    { 4158.76f, 2526.06f, 271.2867f, 4.94079f },
    { 4137.13f, 2463.34f, 271.2867f, 0.329325f },
    { 4202.79f, 2528.59f, 271.2867f, 4.22608f },
    { 4225.8f, 2460.41f, 271.2867f, 2.56957f },
    { 4136.23f, 2507.93f, 271.2867f, 5.81415f },
    { 4157.82f, 2439.42f, 271.2867f, 1.08448f },
    { 4203.65f, 2438.18f, 271.2867f, 1.64054f },
    { 4226.21f, 2506.58f, 271.2867f, 3.72107f }
};

Position const ChampionLoc[8] =
{
    { 4158.76f, 2526.06f, 271.2867f, 4.94079f },
    { 4137.13f, 2463.34f, 271.2867f, 0.329325f },
    { 4202.79f, 2528.59f, 271.2867f, 4.22608f },
    { 4225.8f, 2460.41f, 271.2867f, 2.56957f },
    { 4136.23f, 2507.93f, 271.2867f, 5.81415f },
    { 4157.82f, 2439.42f, 271.2867f, 1.08448f },
    { 4203.65f, 2438.18f, 271.2867f, 1.64054f },
    { 4226.21f, 2506.58f, 271.2867f, 3.72107f }
};

Position const FrostwardenLoc[6] =
{
    { 4172.535f, 2555.645f, 211.1156f, 5.009095f },
    { 4181.684f, 2411.439f, 210.9789f, 1.466077f },
    { 4174.768f, 2413.653f, 210.9789f, 1.413717f },
    { 4186.672f, 2559.176f, 211.1156f, 4.555309f },
    { 4180.163f, 2560.032f, 211.1156f, 4.747295f },
    { 4188.872f, 2413.849f, 210.9789f, 1.710423f }
};

class instance_icecrown_citadel : public InstanceMapScript
{
    public:
        instance_icecrown_citadel() : InstanceMapScript(ICCScriptName, 631) { }

        struct instance_icecrown_citadel_InstanceMapScript : public InstanceScript
        {
            instance_icecrown_citadel_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
            {
                SetHeaders(DataHeader);
                SetBossNumber(EncounterCount);
                LoadDoorData(doorData);
                TeamInInstance = 0;
                MaxAttempts = 30;
                LeftAttempts = MaxAttempts;
                PutricidePipeGreenFilled = false;
                PutricidePipeOrangeFilled = false;
                PutricideTrap = 0;
                IsBonedEligible = true;
                IsOozeDanceEligible = true;
                IsNauseaEligible = true;
                IsOrbWhispererEligible = true;
                IsBuffAvailable = true;
                IsArthasTeleporterUnlocked = (GetBossState(DATA_PROFESSOR_PUTRICIDE) == DONE && GetBossState(DATA_BLOOD_QUEEN_LANA_THEL) == DONE && GetBossState(DATA_SINDRAGOSA) == DONE);
                ColdflameJetsState = NOT_STARTED;
                PutricideHCState = 0;
                BloodQueenHCState = 0;
                SindragosaHCState = 0;
                SaurfangDone = 0;
                BloodQuickeningState = NOT_STARTED;
                BloodQuickeningMinutes = 0;
                TheDamned = 0;
                SpiderAttack = 0;
                SpiderCounter = 0;
                FrostwyrmLeft = 0;
                FrostwyrmRight = 0;
                SindragosaSpawn = 0;
                SindragosaRespawnDelay = time(nullptr) + 60;
                BloodWingEntrance = 0;
                Events.ScheduleEvent(EVENT_SPAWN_DAMNED, 2 * IN_MILLISECONDS);
                Events.ScheduleEvent(EVENT_BLOOD_WING_ENTRANCE, 2 * IN_MILLISECONDS);
            }

            ~instance_icecrown_citadel_InstanceMapScript()
            {
                delete PutricidePreAI;
            }

            void FillInitialWorldStates(WorldPacket& data)
            {
                data << uint32(WORLDSTATE_SHOW_TIMER)         << uint32(BloodQuickeningState == IN_PROGRESS);
                data << uint32(WORLDSTATE_EXECUTION_TIME)     << uint32(BloodQuickeningMinutes);
                data << uint32(WORLDSTATE_SHOW_ATTEMPTS)      << uint32(!(bool(LeftAttempts)));
                data << uint32(WORLDSTATE_ATTEMPTS_REMAINING) << uint32(LeftAttempts);
                data << uint32(WORLDSTATE_ATTEMPTS_MAX)       << uint32(MaxAttempts);
            }

            void OnPlayerAreaUpdate(Player* player, uint32 oldArea, uint32 newArea)
            {
                if (newArea == 4890 /*Putricide's Laboratory of Alchemical Horrors and Fun*/ ||
                    newArea == 4891 /*The Sanctum of Blood*/ ||
                    newArea == 4889 /*The Frost Queen's Lair*/ ||
                    newArea == 4859 /*The Frozen Throne*/ ||
                    newArea == 4910 /*Frostmourne*/)
                {
                    player->SendInitWorldStates(player->GetZoneId(), player->GetAreaId());
                }
                else
                    player->SendUpdateWorldState(WORLDSTATE_SHOW_ATTEMPTS, 0);
            }

            void ShowAttempts(bool show)
            {
                if (show)
                    DoUpdateWorldState(WORLDSTATE_SHOW_ATTEMPTS, 1);
                else
                {
                    if (!Events.GetNextEventTime(EVENT_FADE_LEFT_ATTEMTS) && LeftAttempts)
                        Events.ScheduleEvent(EVENT_FADE_LEFT_ATTEMTS, MINUTE * IN_MILLISECONDS);
                }
            }

            void HandleAttempts(EncounterState state, ObjectGuid BossGUID)
            {
                // show attemnts when encounter starts
                if (state == IN_PROGRESS)
                {
                    ShowAttempts(true);
                    return;
                }
                // decrease attemts when encounter fails
                else if (state == FAIL && LeftAttempts)
                {
                    if (instance->IsHeroic())
                    {
                        --LeftAttempts;
                        DoUpdateWorldState(WORLDSTATE_ATTEMPTS_REMAINING, LeftAttempts);
                        if (!LeftAttempts)
                            if (Creature* boss = instance->GetCreature(BossGUID))
                                boss->DespawnOrUnsummon();
                    }                    
                }
                
                // hide attemts when encounter is done or resets
                if (state != SPECIAL)
                    ShowAttempts(false);
            }

            void HandleFactionBuff(Unit* unit, bool apply)
            {
                //                                               5%     10%    15%    20%    25%    30%
                static const std::vector<uint32> HORDE_IDS    = {73816, 73818, 73819, 73820, 73821, 73822};
                static const std::vector<uint32> ALLIANCE_IDS = {73762, 73824, 73825, 73826, 73827, 73828};

                static const uint32 NM_INDEX = 5;
                static const uint32 HC_INDEX = 2;

                if (GetData(DATA_BUFF_AVAILABLE) && apply)
                {
                    bool is_hc = unit->GetMap()->IsHeroic();
                    uint32 buff_id = 0;

                    if (TeamInInstance == ALLIANCE)
                        buff_id = ALLIANCE_IDS[is_hc ? HC_INDEX : NM_INDEX];
                    else
                        buff_id = HORDE_IDS[is_hc ? HC_INDEX : NM_INDEX];

                    if (!unit->HasAura(buff_id))
                        unit->CastSpell(unit, buff_id, true);
                }
                else
                {

                    auto remove_aura = [&unit](uint32 id) { unit->RemoveAura(id); };

                    std::for_each(HORDE_IDS.begin(), HORDE_IDS.end(), remove_aura);
                    std::for_each(ALLIANCE_IDS.begin(), ALLIANCE_IDS.end(), remove_aura);
                }
            }

            void OnPlayerEnter(Player* player)
            {
                if (!TeamInInstance && !player->IsGameMaster())
                    TeamInInstance = player->GetTeam();

                if (GetBossState(DATA_LADY_DEATHWHISPER) == DONE && GetBossState(DATA_ICECROWN_GUNSHIP_BATTLE) != DONE)
                    SpawnGunship();

                if (GetBossState(DATA_DEATHBRINGER_SAURFANG) != IN_PROGRESS)
                    player->RemoveAurasDueToSpell(SPELL_MARK_OF_THE_FALLEN_CHAMPION);

                player->SendUpdateWorldState(WORLDSTATE_SHOW_ATTEMPTS, uint32(false));

                player->RemoveAura(SPELL_ESSENCE_OF_THE_BLOODQUEEN);
                player->RemoveAura(SPELL_SHADOW_RESONANCE_RESIST);
                player->RemoveAura(SPELL_ANTI_MAGIC_SHELL);
                if (GetBossState(DATA_BLOOD_QUEEN_LANA_THEL) != IN_PROGRESS)
                    player->RemoveAura(SPELL_FRENZY);

                HandleFactionBuff(player, true);
            }

            void OnPlayerRemove(Player* player) 
            {
                HandleFactionBuff(player, false);
                player->RemoveAura(SPELL_ESSENCE_OF_THE_BLOODQUEEN);
                player->RemoveAura(SPELL_SHADOW_RESONANCE_RESIST);
                player->RemoveAurasDueToSpell(SPELL_MARK_OF_THE_FALLEN_CHAMPION);
            }

            void OnPetEnter(Pet* pet) override
            {
                HandleFactionBuff(pet, true);
            }

            void OnPetRemove(Pet* pet) override
            {
                HandleFactionBuff(pet, false);
            }

            void OnCreatureCreate(Creature* creature)
            {
                if (!TeamInInstance)
                {
                    if (!instance->GetPlayers().empty())
                        TeamInInstance = instance->GetPlayers().begin()->GetTeam();
                }

                // apply ICC buff to pets/summons
                if (creature->IsCharmedOwnedByPlayerOrPlayer() && creature->HasUnitTypeMask(UNIT_MASK_MINION | UNIT_MASK_GUARDIAN | UNIT_MASK_CONTROLABLE_GUARDIAN))
                    HandleFactionBuff(creature, true);

                switch (creature->GetEntry())
                {
                    case NPC_LORD_MARROWGAR:
                        MarrowGarGUID = creature->GetGUID();
                        if (GetBossState(DATA_THE_LICH_KING) == DONE)
                        {
                            instance->SummonCreature(NPC_LADY_JAINA_PROUDMOORE_QUEST, JainaSpawnPos);
                            instance->SummonCreature(NPC_MURADIN_BRONZEBEARD_QUEST, MuradinSpawnPos);
                            instance->SummonCreature(NPC_UTHER_THE_LIGHTBRINGER_QUEST, UtherSpawnPos);
                            instance->SummonCreature(NPC_LADY_SYLVANAS_WINDRUNNER_QUEST, SylvanasSpawnPos);
                        }
                        break;
                    case NPC_KOR_KRON_GENERAL:
                        if (TeamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_ALLIANCE_COMMANDER);
                        break;
                    case NPC_KOR_KRON_LIEUTENANT:
                        if (TeamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_SKYBREAKER_LIEUTENANT);
                        break;
                    case NPC_TORTUNOK:
                        if (TeamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_ALANA_MOONSTRIKE);
                        break;
                    case NPC_GERARDO_THE_SUAVE:
                        if (TeamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_TALAN_MOONSTRIKE);
                        break;
                    case NPC_UVLUS_BANEFIRE:
                        if (TeamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_MALFUS_GRIMFROST);
                        break;
                    case NPC_IKFIRUS_THE_VILE:
                        if (TeamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_YILI);
                        break;
                    case NPC_VOL_GUK:
                        if (TeamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_JEDEBIA);
                        break;
                    case NPC_HARAGG_THE_UNSEEN:
                        if (TeamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_NIBY_THE_ALMIGHTY);
                        break;
                    case NPC_GARROSH_HELLSCREAM:
                        if (TeamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_KING_VARIAN_WRYNN);
                        break;
                    case NPC_DEATHBRINGER_SAURFANG:
                        DeathbringerSaurfangGUID = creature->GetGUID();
                        break;
                    case NPC_ALLIANCE_GUNSHIP_CANNON:
                    case NPC_HORDE_GUNSHIP_CANNON:
                        creature->SetControlled(true, UNIT_STATE_ROOT);
                        break;
                    case NPC_SE_HIGH_OVERLORD_SAURFANG:
                        if (TeamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_SE_MURADIN_BRONZEBEARD, creature->GetCreatureData());
                        // no break;
                    case NPC_SE_MURADIN_BRONZEBEARD:
                        DeathbringerSaurfangEventGUID = creature->GetGUID();
                        creature->LastUsedScriptID = creature->GetScriptId();
                        break;
                    case NPC_SE_KOR_KRON_REAVER:
                        if (TeamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_SE_SKYBREAKER_MARINE);
                        break;
                    case NPC_MARTYR_STALKER_IGB_SAURFANG:
                        DeathbringerEndEventTriggerGUID = creature->GetGUID();
                        break;
                    case NPC_FESTERGUT:
                        FestergutGUID = creature->GetGUID();
                        break;
                    case NPC_ROTFACE:
                        RotfaceGUID = creature->GetGUID();
                        break;
                    case NPC_PROFESSOR_PUTRICIDE:
                        ProfessorPutricideGUID = creature->GetGUID();
                        break;
                    case NPC_PRINCE_KELESETH:
                        BloodCouncilGUIDs[0] = creature->GetGUID();
                        break;
                    case NPC_PRINCE_TALDARAM:
                        BloodCouncilGUIDs[1] = creature->GetGUID();
                        break;
                    case NPC_PRINCE_VALANAR:
                        BloodCouncilGUIDs[2] = creature->GetGUID();
                        break;
                    case NPC_BLOOD_ORB_CONTROLLER:
                        BloodCouncilControllerGUID = creature->GetGUID();
                        break;
                    case NPC_BLOOD_QUEEN_LANA_THEL_TRIGGER:
                        BloodQueenLanaThelTriggerGUID = creature->GetGUID();
                        break;
                    case NPC_BLOOD_QUEEN_LANA_THEL:
                        BloodQueenLanaThelGUID = creature->GetGUID();
                        break;
                    case NPC_CROK_SCOURGEBANE:
                        CrokScourgebaneGUID = creature->GetGUID();
                        break;
                    // we can only do this because there are no gaps in their entries
                    case NPC_CAPTAIN_ARNATH:
                    case NPC_CAPTAIN_BRANDON:
                    case NPC_CAPTAIN_GRONDEL:
                    case NPC_CAPTAIN_RUPERT:
                        CrokCaptainGUIDs[creature->GetEntry()-NPC_CAPTAIN_ARNATH] = creature->GetGUID();
                        break;
                    case NPC_SISTER_SVALNA:
                        SisterSvalnaGUID = creature->GetGUID();
                        break;
                    case NPC_VALITHRIA_DREAMWALKER:
                        ValithriaDreamwalkerGUID = creature->GetGUID();
                        break;
                    case NPC_THE_LICH_KING_VALITHRIA:
                        ValithriaLichKingGUID = creature->GetGUID();
                        break;
                    case NPC_GREEN_DRAGON_COMBAT_TRIGGER:
                        ValithriaTriggerGUID = creature->GetGUID();
                        break;
                    case NPC_SINDRAGOSA:
                        SindragosaGUID = creature->GetGUID();
                        break;
                    case NPC_SPINESTALKER:
                        SpinestalkerGUID = creature->GetGUID();
                        break;
                    case NPC_RIMEFANG:
                        RimefangGUID = creature->GetGUID();
                        break;
                    case NPC_INVISIBLE_STALKER:
                        // Teleporter visual at center
                        if (creature->GetExactDist2d(4357.052f, 2769.421f) < 10.0f)
                            creature->CastSpell(creature, SPELL_ARTHAS_TELEPORTER_CEREMONY, false);
                        break;
                    case NPC_THE_LICH_KING:
                        TheLichKingGUID = creature->GetGUID();
                        break;
                    case NPC_HIGHLORD_TIRION_FORDRING_LK:
                        HighlordTirionFordringGUID = creature->GetGUID();
                        break;
                    case NPC_TERENAS_MENETHIL_FROSTMOURNE:
                    case NPC_TERENAS_MENETHIL_FROSTMOURNE_H:
                        TerenasMenethilGUID = creature->GetGUID();
                        break;
                    case NPC_WICKED_SPIRIT:
                        // Remove corpse as soon as it dies (and respawn 10 seconds later)
                        creature->SetCorpseDelay(0);
                        creature->SetReactState(REACT_PASSIVE);
                        break;
                    case NPC_DEATHBOUND_WARD:
                        creature->AddAura(SPELL_STONEFORM, creature);
                        break;
                    case NPC_NERUBER_BROODKEEPER:
                    case NPC_NERUBER_BROODKEEPER_H:
                        creature->AddAura(SPELL_FLIGHT, creature);
                        break;
                    case NPC_DARKFALLEN_NOBLE:
                    case NPC_DARKFALLEN_ARCHMAGE:
                    case NPC_DARKFALLEN_BLOOD_KNIGHT:
                    case NPC_DARKFALLEN_ADVISOR:
                        if (creature->IsAlive() && creature->GetPositionZ() < 352.0f)
                            DarkfallenCreaturesGuids.insert(creature->GetGUID());
                        break;
                    default:
                        break;
                }
            }

            void OnCreatureRemove(Creature* creature)
            {
                if (creature->GetEntry() == NPC_SINDRAGOSA)
                    SindragosaGUID.Clear();
            }

            // Spider Event in front of Sindragosa
            void SummonBroodling()
            {
                if (Creature* summoner = instance->GetCreature(SpinestalkerGUID))
                {
                    for (int i = 0; i < 8; ++i)
                        summoner->SummonCreature(NPC_NERUBAR_BROODLING, BroodlingLoc[i], TEMPSUMMON_DEAD_DESPAWN, 300000);
                }
            }

            void SummonChampion()
            {
                if (Creature* summoner = instance->GetCreature(SpinestalkerGUID))
                {
                    for (int i = 0; i < 8; ++i)
                        summoner->SummonCreature(NPC_NERUBAR_CHAMPION, ChampionLoc[i], TEMPSUMMON_DEAD_DESPAWN, 300000);
                }
            }

            void SummonFrostwarden()
            {
                if (Creature* summoner = instance->GetCreature(SpinestalkerGUID))
                {
                    for (int i = 0; i < 2; ++i)
                        summoner->SummonCreature(NPC_FROSTWARDEN_SORCERESS, FrostwardenLoc[i], TEMPSUMMON_DEAD_DESPAWN, 300000);
                    for (int i = 3; i < 6; ++i)
                        summoner->SummonCreature(NPC_FROSTWARDEN_WARRIOR, FrostwardenLoc[i], TEMPSUMMON_DEAD_DESPAWN, 300000);
                }
            }

            // Weekly quest spawn prevention
            uint32 GetCreatureEntry(uint32 /*guidLow*/, CreatureData const* data)
            {
                uint32 entry = data->id;
                switch (entry)
                {
                    // This hack binds the weekly quest to the instance id. This isn't really random, but random enough.
                    case NPC_INFILTRATOR_MINCHAR:
                        if (instance->GetInstanceId() % 5 != 0)
                            entry = 0;
                        break;
                    case NPC_KOR_KRON_LIEUTENANT:
                        if (instance->GetInstanceId() % 5 != 1)
                            entry = 0;
                        break;
                    case NPC_ALCHEMIST_ADRIANNA:
                        if (instance->GetInstanceId() % 5 != 2)
                            entry = 0;
                        break;
                    case NPC_ALRIN_THE_AGILE:
                    case NPC_INFILTRATOR_MINCHAR_BQ:
                    case NPC_MINCHAR_BEAM_STALKER:
                        if (instance->GetInstanceId() % 5 != 3)
                            entry = 0;
                        break;
                    case NPC_VALITHRIA_DREAMWALKER_QUEST:
                        if (instance->GetInstanceId() % 5 != 4)
                            entry = 0;
                        break;

                    case NPC_HORDE_GUNSHIP_CANNON:
                    case NPC_ORGRIMS_HAMMER_CREW:
                    case NPC_SKY_REAVER_KORM_BLACKSCAR:
                        if (TeamInInstance == ALLIANCE)
                            return 0;
                        break;
                    case NPC_ALLIANCE_GUNSHIP_CANNON:
                    case NPC_SKYBREAKER_DECKHAND:
                    case NPC_HIGH_CAPTAIN_JUSTIN_BARTLETT:
                        if (TeamInInstance == HORDE)
                            return 0;
                        break;
                    case NPC_ZAFOD_BOOMBOX:
                        if (GameObjectTemplate const* go = sObjectMgr->GetGameObjectTemplate(GO_THE_SKYBREAKER_A))
                            if ((TeamInInstance == ALLIANCE && data->mapid == go->moTransport.mapID) ||
                                (TeamInInstance == HORDE && data->mapid != go->moTransport.mapID))
                                return entry;
                        return 0;
                    case NPC_IGB_MURADIN_BRONZEBEARD:
                        if ((TeamInInstance == ALLIANCE && data->posX > 10.0f) ||
                            (TeamInInstance == HORDE && data->posX < 10.0f))
                            return entry;
                        return 0;
                    default:
                        break;
                }

                return entry;
            }

            uint32 GetGameObjectEntry(uint32 /*guidLow*/, uint32 entry) override
            {
                switch (entry)
                {
                case GO_GUNSHIP_ARMORY_H_10N:
                case GO_GUNSHIP_ARMORY_H_25N:
                case GO_GUNSHIP_ARMORY_H_10H:
                case GO_GUNSHIP_ARMORY_H_25H:
                    if (TeamInInstance == ALLIANCE)
                        return 0;
                    break;
                case GO_GUNSHIP_ARMORY_A_10N:
                case GO_GUNSHIP_ARMORY_A_25N:
                case GO_GUNSHIP_ARMORY_A_10H:
                case GO_GUNSHIP_ARMORY_A_25H:
                    if (TeamInInstance == HORDE)
                        return 0;
                    break;
                default:
                    break;
                }

                return entry;
            }

            void OnUnitDeath(Unit* unit)
            {
                Creature* creature = unit->ToCreature();
                if (!creature)
                    return;

                switch (creature->GetEntry())
                {
                    case NPC_YMIRJAR_BATTLE_MAIDEN:
                    case NPC_YMIRJAR_DEATHBRINGER:
                    case NPC_YMIRJAR_FROSTBINDER:
                    case NPC_YMIRJAR_HUNTRESS:
                    case NPC_YMIRJAR_WARLORD:
                        if (Creature* crok = instance->GetCreature(CrokScourgebaneGUID))
                            crok->AI()->SetGuidData(creature->GetGUID(), ACTION_VRYKUL_DEATH);
                        break;
                    case NPC_FROSTWING_WHELP:
                        if (FrostwyrmGUIDs.empty())
                            return;

                        if (creature->AI()->GetData(1/*DATA_FROSTWYRM_OWNER*/) == DATA_SPINESTALKER)
                        {
                            SpinestalkerTrash.erase(creature->GetSpawnId());
                            if (SpinestalkerTrash.empty())
                                if (Creature* spinestalk = instance->GetCreature(SpinestalkerGUID))
                                    spinestalk->AI()->DoAction(ACTION_START_FROSTWYRM);
                        }
                        else
                        {
                            RimefangTrash.erase(creature->GetSpawnId());
                            if (RimefangTrash.empty())
                                if (Creature* spinestalk = instance->GetCreature(RimefangGUID))
                                    spinestalk->AI()->DoAction(ACTION_START_FROSTWYRM);
                        }
                        break;
                    case NPC_RIMEFANG:
                    case NPC_SPINESTALKER:
                    {
                        if (instance->IsHeroic() && !LeftAttempts)
                            return;

                        if (GetBossState(DATA_SINDRAGOSA) == DONE)
                            return;

                        FrostwyrmGUIDs.erase(creature->GetSpawnId());
                        if (FrostwyrmGUIDs.empty())
                        {
                            instance->LoadGrid(SindragosaSpawnPos.GetPositionX(), SindragosaSpawnPos.GetPositionY());
                            if (Creature* boss = instance->SummonCreature(NPC_SINDRAGOSA, SindragosaSpawnPos))
                                boss->AI()->DoAction(ACTION_START_FROSTWYRM);
                        }
                        break;
                    }
                    case NPC_DARKFALLEN_NOBLE:
                    case NPC_DARKFALLEN_ARCHMAGE:
                    case NPC_DARKFALLEN_BLOOD_KNIGHT:
                    case NPC_DARKFALLEN_ADVISOR:
                        SetGuidData(DATA_DARKFALLEN_DIED, creature->GetGUID());
                        if (GetData(DATA_DARKFALLEN_ALIVE) == 0)
                        {
                            if (GetData(DATA_BLOOD_WING_ENTRANCE) == 0)
                                SetData(DATA_BLOOD_WING_ENTRANCE, 1);
                            if (GameObject* door = instance->GetGameObject(CrimsonHallDoorGUID))
                                HandleGameObject(CrimsonHallDoorGUID, true, door);
                        }
                        break;
                    default:
                        break;
                }
            }

            void OnGameObjectCreate(GameObject* go)
            {
                switch (go->GetEntry())
                {
                    case GO_SCIENTIST_ENTRANCE:
                        PutricideDoorGUID = go->GetGUID();
                        if (GetData(DATA_PUTRICIDE_TRAP) != 0)
                            HandleGameObject(PutricideDoorGUID, true, go);
                        else
                            HandleGameObject(PutricideDoorGUID, false, go);
                        break;
                    case GO_SINDRAGOSA_ENTRANCE_DOOR:
                        SindragosaDoorGUID = go->GetGUID();
                        AddDoor(go, true);
                        if (GetData(DATA_SPIDER_ATTACK) != DONE)
                            HandleGameObject(SindragosaDoorGUID, false, go);
                        break;
                    case GO_DOODAD_ICECROWN_ICEWALL02:
                    case GO_ICEWALL:
                    case GO_LORD_MARROWGAR_S_ENTRANCE:
                    case GO_ORATORY_OF_THE_DAMNED_ENTRANCE:
                    case GO_ORANGE_PLAGUE_MONSTER_ENTRANCE:
                    case GO_GREEN_PLAGUE_MONSTER_ENTRANCE:
                    case GO_BLOOD_ELF_COUNCIL_DOOR:
                    case GO_BLOOD_ELF_COUNCIL_DOOR_RIGHT:
                    case GO_DOODAD_ICECROWN_BLOODPRINCE_DOOR_01:
                    case GO_DOODAD_ICECROWN_GRATE_01:
                    case GO_GREEN_DRAGON_BOSS_ENTRANCE:
                    case GO_GREEN_DRAGON_BOSS_EXIT:
                    case GO_DOODAD_ICECROWN_ROOSTPORTCULLIS_02:
                    case GO_DOODAD_ICECROWN_ROOSTPORTCULLIS_03:
                    case GO_SINDRAGOSA_SHORTCUT_ENTRANCE_DOOR:
                    case GO_SINDRAGOSA_SHORTCUT_EXIT_DOOR:
                    case GO_ICE_WALL:
                        AddDoor(go, true);
                        break;
                    case GO_CRIMSON_HALL_DOOR:
                        CrimsonHallDoorGUID = go->GetGUID();
                        AddDoor(go, true);
                        if (GetData(DATA_BLOOD_WING_ENTRANCE) == 1)
                            HandleGameObject(CrimsonHallDoorGUID, true, go);
                        else
                            HandleGameObject(CrimsonHallDoorGUID, false, go);
                        break;
                    // these 2 gates are functional only on 25man modes
                    case GO_DOODAD_ICECROWN_ROOSTPORTCULLIS_01:
                    case GO_DOODAD_ICECROWN_ROOSTPORTCULLIS_04:
                        if (instance->Is25ManRaid())
                            AddDoor(go, true);
                        break;
                    case GO_LADY_DEATHWHISPER_ELEVATOR:
                        LadyDeathwisperElevatorGUID = go->GetGUID();
                        if (GetBossState(DATA_LADY_DEATHWHISPER) == DONE)
                        {
                            go->SetUInt32Value(GAMEOBJECT_LEVEL, 0);
                            go->SetGoState(GO_STATE_READY);
                        }
                        break;
                    case GO_THE_SKYBREAKER_H:
                    case GO_ORGRIMS_HAMMER_A:
                        EnemyGunshipGUID = go->GetGUID();
                        break;
                    case GO_GUNSHIP_ARMORY_H_10N:
                    case GO_GUNSHIP_ARMORY_H_25N:
                    case GO_GUNSHIP_ARMORY_H_10H:
                    case GO_GUNSHIP_ARMORY_H_25H:
                    case GO_GUNSHIP_ARMORY_A_10N:
                    case GO_GUNSHIP_ARMORY_A_25N:
                    case GO_GUNSHIP_ARMORY_A_10H:
                    case GO_GUNSHIP_ARMORY_A_25H:
                        GunshipArmoryGUID = go->GetGUID();
                        break;
                    case GO_SAURFANG_S_DOOR:
                        DeathbringerSaurfangDoorGUID = go->GetGUID();
                        AddDoor(go, true);
                        break;
                    case GO_DEATHBRINGER_S_CACHE_10N:
                    case GO_DEATHBRINGER_S_CACHE_25N:
                    case GO_DEATHBRINGER_S_CACHE_10H:
                    case GO_DEATHBRINGER_S_CACHE_25H:
                        DeathbringersCacheGUID = go->GetGUID();
                        break;
                    case GO_SCOURGE_TRANSPORTER_SAURFANG:
                        SaurfangTeleportGUID = go->GetGUID();
                        break;
                    case GO_PLAGUE_SIGIL:
                        PlagueSigilGUID = go->GetGUID();
                        if (GetBossState(DATA_PROFESSOR_PUTRICIDE) == DONE)
                            HandleGameObject(PlagueSigilGUID, false, go);
                        break;
                    case GO_BLOODWING_SIGIL:
                        BloodwingSigilGUID = go->GetGUID();
                        if (GetBossState(DATA_BLOOD_QUEEN_LANA_THEL) == DONE)
                            HandleGameObject(BloodwingSigilGUID, false, go);
                        break;
                    case GO_SIGIL_OF_THE_FROSTWING:
                        FrostwingSigilGUID = go->GetGUID();
                        if (GetBossState(DATA_SINDRAGOSA) == DONE)
                            HandleGameObject(FrostwingSigilGUID, false, go);
                        break;
                    case GO_SCIENTIST_AIRLOCK_DOOR_COLLISION:
                        PutricideCollisionGUID = go->GetGUID();
                        break;
                    case GO_SCIENTIST_AIRLOCK_DOOR_ORANGE:
                        PutricideGateGUIDs[0] = go->GetGUID();
                        break;
                    case GO_SCIENTIST_AIRLOCK_DOOR_GREEN:
                        PutricideGateGUIDs[1] = go->GetGUID();
                        break;
                    case GO_DOODAD_ICECROWN_ORANGETUBES02:
                        PutricidePipeGUIDs[0] = go->GetGUID();
                        break;
                    case GO_DOODAD_ICECROWN_GREENTUBES02:
                        PutricidePipeGUIDs[1] = go->GetGUID();
                        break;
                    case GO_DRINK_ME:
                        PutricideTableGUID = go->GetGUID();
                        break;
                    case GO_CACHE_OF_THE_DREAMWALKER_10N:
                    case GO_CACHE_OF_THE_DREAMWALKER_25N:
                    case GO_CACHE_OF_THE_DREAMWALKER_10H:
                    case GO_CACHE_OF_THE_DREAMWALKER_25H:
                        if (Creature* valithria = instance->GetCreature(ValithriaDreamwalkerGUID))
                            go->SetLootRecipient(valithria->GetLootRecipient());
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED | GO_FLAG_NOT_SELECTABLE | GO_FLAG_NODESPAWN);
                        break;
                    case GO_SCOURGE_TRANSPORTER_LK:
                        TheLichKingTeleportGUID = go->GetGUID();
                        if (GetBossState(DATA_PROFESSOR_PUTRICIDE) == DONE && GetBossState(DATA_BLOOD_QUEEN_LANA_THEL) == DONE && GetBossState(DATA_SINDRAGOSA) == DONE)
                            go->SetGoState(GO_STATE_ACTIVE);
                        break;
                    case GO_ARTHAS_PLATFORM:
                        // this enables movement at The Frozen Throne, when printed this value is 0.000000f
                        // however, when represented as integer client will accept only this value
                        go->SetUInt32Value(GAMEOBJECT_PARENTROTATION, 5535469);
                        ArthasPlatformGUID = go->GetGUID();
                        break;
                    case GO_ARTHAS_PRECIPICE:
                        go->SetUInt32Value(GAMEOBJECT_PARENTROTATION, 4178312);
                        ArthasPrecipiceGUID = go->GetGUID();
                        break;
                    case GO_DOODAD_ICECROWN_THRONEFROSTYEDGE01:
                        FrozenThroneEdgeGUID = go->GetGUID();
                        break;
                    case GO_DOODAD_ICECROWN_THRONEFROSTYWIND01:
                        FrozenThroneWindGUID = go->GetGUID();
                        break;
                    case GO_DOODAD_ICECROWN_SNOWEDGEWARNING01:
                        FrozenThroneWarningGUID = go->GetGUID();
                        break;
                    case GO_FROZEN_LAVAMAN:
                        FrozenBolvarGUID = go->GetGUID();
                        if (GetBossState(DATA_THE_LICH_KING) == DONE)
                            go->SetRespawnTime(7 * DAY);
                        break;
                    case GO_LAVAMAN_PILLARS_CHAINED:
                        PillarsChainedGUID = go->GetGUID();
                        if (GetBossState(DATA_THE_LICH_KING) == DONE)
                            go->SetRespawnTime(7 * DAY);
                        break;
                    case GO_LAVAMAN_PILLARS_UNCHAINED:
                        PillarsUnchainedGUID = go->GetGUID();
                        if (GetBossState(DATA_THE_LICH_KING) == DONE)
                            go->SetRespawnTime(7 * DAY);
                        break;
                    case GO_RELEASE_DOOR_FROST_WING:
                        if (MaxAttempts <= 19)
                            HandleGameObject(ObjectGuid::Empty, false, go);
                        break;
                    case GO_RELEASE_DOOR_BLOOD_WING:
                        if (MaxAttempts <= 10)
                            HandleGameObject(ObjectGuid::Empty, false, go);
                        break;
                    default:
                        break;
                }
            }

            void OnGameObjectRemove(GameObject* go)
            {
                switch (go->GetEntry())
                {
                    case GO_DOODAD_ICECROWN_ICEWALL02:
                    case GO_ICEWALL:
                    case GO_LORD_MARROWGAR_S_ENTRANCE:
                    case GO_ORATORY_OF_THE_DAMNED_ENTRANCE:
                    case GO_SAURFANG_S_DOOR:
                    case GO_ORANGE_PLAGUE_MONSTER_ENTRANCE:
                    case GO_GREEN_PLAGUE_MONSTER_ENTRANCE:
                    case GO_SCIENTIST_ENTRANCE:
                    case GO_CRIMSON_HALL_DOOR:
                    case GO_BLOOD_ELF_COUNCIL_DOOR:
                    case GO_BLOOD_ELF_COUNCIL_DOOR_RIGHT:
                    case GO_DOODAD_ICECROWN_BLOODPRINCE_DOOR_01:
                    case GO_DOODAD_ICECROWN_GRATE_01:
                    case GO_GREEN_DRAGON_BOSS_ENTRANCE:
                    case GO_GREEN_DRAGON_BOSS_EXIT:
                    case GO_DOODAD_ICECROWN_ROOSTPORTCULLIS_01:
                    case GO_DOODAD_ICECROWN_ROOSTPORTCULLIS_02:
                    case GO_DOODAD_ICECROWN_ROOSTPORTCULLIS_03:
                    case GO_DOODAD_ICECROWN_ROOSTPORTCULLIS_04:
                    case GO_SINDRAGOSA_ENTRANCE_DOOR:
                    case GO_SINDRAGOSA_SHORTCUT_ENTRANCE_DOOR:
                    case GO_SINDRAGOSA_SHORTCUT_EXIT_DOOR:
                    case GO_ICE_WALL:
                        AddDoor(go, false);
                        break;
                    case GO_THE_SKYBREAKER_A:
                    case GO_ORGRIMS_HAMMER_H:
                        GunshipGUID.Clear();
                        break;
                    default:
                        break;
                }
            }

            uint32 GetData(uint32 type) const
            {
                switch (type)
                {
                    case DATA_SINDRAGOSA_FROSTWYRMS:
                        return FrostwyrmGUIDs.size();
                    case DATA_SPINESTALKER:
                        return SpinestalkerTrash.size();
                    case DATA_RIMEFANG:
                        return RimefangTrash.size();
                    case DATA_SINDRAGOSA_CAN_SPAWN:
                        return SindragosaRespawnDelay + 30 <= time(nullptr) ? 1 : 0;
                    case DATA_COLDFLAME_JETS:
                        return ColdflameJetsState;
                    case DATA_TEAM_IN_INSTANCE:
                        return TeamInInstance;
                    case DATA_BLOOD_QUICKENING_STATE:
                        return BloodQuickeningState;
                    case DATA_HEROIC_ATTEMPTS:
                        return LeftAttempts;
                    case DATA_PUTRICIDE_TRAP:
                        return PutricideTrap;
                    case DATA_THE_DAMNED:
                        return TheDamned;
                    case DATA_SPIDER_ATTACK:
                        return SpiderAttack;
                    case DATA_FROSTWYRM_RIGHT:
                        return FrostwyrmRight;
                    case DATA_FROSTWYRM_LEFT:
                        return FrostwyrmLeft;
                    case DATA_MAX_ATTEMPTS:
                        return MaxAttempts;
                    case DATA_SINDRAGOSA_SPAWN:
                        return SindragosaSpawn;
                    case DATA_PROFESSOR_PUTRICIDE_HC:
                        return PutricideHCState;
                    case DATA_BLOOD_QUEEN_LANA_THEL_HC:
                        return BloodQueenHCState;
                    case DATA_SINDRAGOSA_HC:
                        return SindragosaHCState;
                    case DATA_SAURFANG_DONE:
                        return SaurfangDone;
                    case DATA_DARKFALLEN_ALIVE:
                        return DarkfallenCreaturesGuids.size();
                    case DATA_BLOOD_WING_ENTRANCE:
                        return BloodWingEntrance;
                    case DATA_BUFF_AVAILABLE:
                        return (IsBuffAvailable ? 1 : 0);
                    default:
                        break;
                }

                return 0;
            }

            ObjectGuid GetGuidData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_LORD_MARROWGAR:
                        return MarrowGarGUID;
                    case DATA_ICECROWN_GUNSHIP_BATTLE:
                        return GunshipGUID;
                    case DATA_ENEMY_GUNSHIP:
                        return EnemyGunshipGUID;
                    case DATA_DEATHBRINGER_SAURFANG:
                        return DeathbringerSaurfangGUID;
                    case DATA_SAURFANG_EVENT_NPC:
                        return DeathbringerSaurfangEventGUID;
                    case NPC_MARTYR_STALKER_IGB_SAURFANG:
                        return DeathbringerEndEventTriggerGUID;
                    case GO_SAURFANG_S_DOOR:
                        return DeathbringerSaurfangDoorGUID;
                    case GO_SCOURGE_TRANSPORTER_SAURFANG:
                        return SaurfangTeleportGUID;
                    case DATA_FESTERGUT:
                        return FestergutGUID;
                    case DATA_ROTFACE:
                        return RotfaceGUID;
                    case GO_SCIENTIST_AIRLOCK_DOOR_COLLISION:
                        return PutricideCollisionGUID;
                    case GO_SCIENTIST_AIRLOCK_DOOR_ORANGE:
                        return PutricideGateGUIDs[0];
                    case GO_SCIENTIST_AIRLOCK_DOOR_GREEN:
                        return PutricideGateGUIDs[1];
                    case DATA_PUTRICIDE_DOOR:
                        return PutricideDoorGUID;
                    case DATA_PROFESSOR_PUTRICIDE:
                        return ProfessorPutricideGUID;
                    case DATA_PUTRICIDE_TABLE:
                        return PutricideTableGUID;
                    case DATA_PRINCE_KELESETH_GUID:
                        return BloodCouncilGUIDs[0];
                    case DATA_PRINCE_TALDARAM_GUID:
                        return BloodCouncilGUIDs[1];
                    case DATA_PRINCE_VALANAR_GUID:
                        return BloodCouncilGUIDs[2];
                    case DATA_BLOOD_PRINCES_CONTROL:
                        return BloodCouncilControllerGUID;
                    case DATA_BLOOD_QUEEN_LANATHEL_TRIGGER:
                        return BloodQueenLanaThelTriggerGUID;
                    case DATA_BLOOD_QUEEN_LANA_THEL:
                        return BloodQueenLanaThelGUID;
                    case DATA_CROK_SCOURGEBANE:
                        return CrokScourgebaneGUID;
                    case DATA_CAPTAIN_ARNATH:
                    case DATA_CAPTAIN_BRANDON:
                    case DATA_CAPTAIN_GRONDEL:
                    case DATA_CAPTAIN_RUPERT:
                        return CrokCaptainGUIDs[type - DATA_CAPTAIN_ARNATH];
                    case DATA_SISTER_SVALNA:
                        return SisterSvalnaGUID;
                    case DATA_VALITHRIA_DREAMWALKER:
                        return ValithriaDreamwalkerGUID;
                    case DATA_VALITHRIA_LICH_KING:
                        return ValithriaLichKingGUID;
                    case DATA_VALITHRIA_TRIGGER:
                        return ValithriaTriggerGUID;
                    case DATA_SINDRAGOSA_DOOR:
                        return SindragosaDoorGUID;
                    case DATA_SINDRAGOSA:
                        return SindragosaGUID;
                    case DATA_SPINESTALKER:
                        return SpinestalkerGUID;
                    case DATA_RIMEFANG:
                        return RimefangGUID;
                    case DATA_THE_LICH_KING:
                        return TheLichKingGUID;
                    case DATA_HIGHLORD_TIRION_FORDRING:
                        return HighlordTirionFordringGUID;
                    case DATA_ARTHAS_PLATFORM:
                        return ArthasPlatformGUID;
                    case DATA_TERENAS_MENETHIL:
                        return TerenasMenethilGUID;
                    default:
                        break;
                }

                return ObjectGuid::Empty;
            }

            bool SetBossState(uint32 type, EncounterState state)
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;

                switch (type)
                {
                    case DATA_LADY_DEATHWHISPER:
                    {
                        if (state == DONE)
                        {
                            if (GameObject* elevator = instance->GetGameObject(LadyDeathwisperElevatorGUID))
                            {
                                elevator->SetUInt32Value(GAMEOBJECT_LEVEL, 0);
                                elevator->SetGoState(GO_STATE_READY);
                            }
                            SaveToDB();

                            SpawnGunship();
                        }
                        break;
                    }
                    case DATA_ICECROWN_GUNSHIP_BATTLE:
                        if (state == DONE)
                        {
                            if (GameObject* loot = instance->GetGameObject(GunshipArmoryGUID))
                            {
                                loot->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED | GO_FLAG_NOT_SELECTABLE | GO_FLAG_NODESPAWN);
                                loot->SetPhaseMask(1, true);
                            }
                        }
                        else if (state == FAIL)
                            Events.ScheduleEvent(EVENT_RESPAWN_GUNSHIP, 30000);
                        break;
                    case DATA_DEATHBRINGER_SAURFANG:
                        switch (state)
                        {
                            case DONE:
                                if (GameObject* loot = instance->GetGameObject(DeathbringersCacheGUID))
                                {
                                    if (Creature* deathbringer = instance->GetCreature(DeathbringerSaurfangGUID))
                                        loot->SetLootRecipient(deathbringer->GetLootRecipient());
                                    loot->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED | GO_FLAG_NOT_SELECTABLE | GO_FLAG_NODESPAWN);
                                }
                                // no break
                            case NOT_STARTED:
                                if (GameObject* teleporter = instance->GetGameObject(SaurfangTeleportGUID))
                                {
                                    HandleGameObject(SaurfangTeleportGUID, true, teleporter);
                                    teleporter->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
                                }
                            case FAIL:
                                Events.ScheduleEvent(EVENT_RESPAWN_SAURFANG, 30 * IN_MILLISECONDS);
                                break;
                            default:
                                break;
                        }
                        break;
                    case DATA_FESTERGUT:
                    case DATA_ROTFACE:
                        if (state == IN_PROGRESS)
                        {
                            if (Creature* prof = instance->GetCreature(ProfessorPutricideGUID))
                            {
                                if (!PutricideNormalAI)
                                    PutricideNormalAI = dynamic_cast<boss_professor_putricideAI*>(prof->AI());

                                if (PutricidePreAI)
                                    prof->SetAI(PutricidePreAI);
                                else
                                {
                                    PutricidePreAI = new boss_professor_putricide_dummyAI(prof);
                                    prof->SetAI(PutricidePreAI);
                                }
                            }
                        }
                        else
                        {
                            if (Creature* prof = instance->GetCreature(ProfessorPutricideGUID); PutricideNormalAI != nullptr && prof)
                                prof->SetAI(PutricideNormalAI);
                        }
                        break;
                    case DATA_PROFESSOR_PUTRICIDE:
                        HandleGameObject(PlagueSigilGUID, state != DONE);
                        HandleAttempts(state, ProfessorPutricideGUID);
                        break;
                    case DATA_BLOOD_QUEEN_LANA_THEL:
                        HandleGameObject(BloodwingSigilGUID, state != DONE);
                        HandleAttempts(state, BloodQueenLanaThelGUID);
                        break;
                    case DATA_VALITHRIA_DREAMWALKER:
                        if (state == DONE && (instance->GetInstanceId() % 5 == 4))
                            instance->SummonCreature(NPC_VALITHRIA_DREAMWALKER_QUEST, ValithriaSpawnPos);
                        break;
                    case DATA_SINDRAGOSA:
                        HandleGameObject(FrostwingSigilGUID, state != DONE);
                        HandleAttempts(state, SindragosaGUID);
                        if (state == FAIL)
                            SindragosaRespawnDelay = time(nullptr);
                        break;
                    case DATA_THE_LICH_KING:
                    {
                        // set the platform as active object to dramatically increase visibility range
                        // note: "active" gameobjects do not block grid unloading
                        if (GameObject* precipice = instance->GetGameObject(ArthasPrecipiceGUID))
                            precipice->setActive(state == IN_PROGRESS);
                        if (GameObject* platform = instance->GetGameObject(ArthasPlatformGUID))
                            platform->setActive(state == IN_PROGRESS);
                        HandleAttempts(state, TheLichKingGUID);

                        if (state == DONE)
                        {
                            if (GameObject* bolvar = instance->GetGameObject(FrozenBolvarGUID))
                                bolvar->SetRespawnTime(7 * DAY);
                            if (GameObject* pillars = instance->GetGameObject(PillarsChainedGUID))
                                pillars->SetRespawnTime(7 * DAY);
                            if (GameObject* pillars = instance->GetGameObject(PillarsUnchainedGUID))
                                pillars->SetRespawnTime(7 * DAY);

                            instance->SummonCreature(NPC_LADY_JAINA_PROUDMOORE_QUEST, JainaSpawnPos);
                            instance->SummonCreature(NPC_MURADIN_BRONZEBEARD_QUEST, MuradinSpawnPos);
                            instance->SummonCreature(NPC_UTHER_THE_LIGHTBRINGER_QUEST, UtherSpawnPos);
                            instance->SummonCreature(NPC_LADY_SYLVANAS_WINDRUNNER_QUEST, SylvanasSpawnPos);
                            SaveToDB();
                        }
                        break;
                    }
                    default:
                        break;
                 }

                 return true;
            }

            void SpawnGunship()
            {
                if (!GunshipGUID)
                {
                    SetBossState(DATA_ICECROWN_GUNSHIP_BATTLE, NOT_STARTED);
                    uint32 gunshipEntry = TeamInInstance == HORDE ? GO_ORGRIMS_HAMMER_H : GO_THE_SKYBREAKER_A;
                    if (Transport* gunship = sTransportMgr->CreateTransport(gunshipEntry, 0, instance))
                        GunshipGUID = gunship->GetGUID();
                }
            }

            void SetGuidData(uint32 type, ObjectGuid data) override
            {
                if (type == DATA_DARKFALLEN_DIED)
                    DarkfallenCreaturesGuids.erase(data);
            }

            void SetData(uint32 type, uint32 data)
            {
                switch (type)
                {
                    case DATA_BONED_ACHIEVEMENT:
                        IsBonedEligible = data ? true : false;
                        break;
                    case DATA_OOZE_DANCE_ACHIEVEMENT:
                        IsOozeDanceEligible = data ? true : false;
                        break;
                    case DATA_NAUSEA_ACHIEVEMENT:
                        IsNauseaEligible = data ? true : false;
                        break;
                    case DATA_ORB_WHISPERER_ACHIEVEMENT:
                        IsOrbWhispererEligible = data ? true : false;
                        break;
                    case DATA_SINDRAGOSA_FROSTWYRMS:
                        FrostwyrmGUIDs.insert(data);
                        break;
                    case DATA_SPINESTALKER:
                        SpinestalkerTrash.insert(data);
                        break;
                    case DATA_RIMEFANG:
                        RimefangTrash.insert(data);
                        break;
                    case DATA_THE_DAMNED:
                        TheDamned = data;
                        SaveToDB();
                        break;
                    case DATA_SPIDER_ATTACK:
                        SpiderAttack = data;
                        if (data == IN_PROGRESS)
                        {
                            Events.ScheduleEvent(EVENT_SPIDER_ATTACK, 3 * IN_MILLISECONDS);
                            SpiderCounter = 0;
                        }
                        else
                            Events.CancelEvent(EVENT_SPIDER_ATTACK);
                        SaveToDB();
                        break;
                    case DATA_FROSTWYRM_RIGHT:
                        FrostwyrmRight = data;
                        SaveToDB();
                        break;
                    case DATA_FROSTWYRM_LEFT:
                        FrostwyrmLeft = data;
                        SaveToDB();
                        break;
                    case DATA_COLDFLAME_JETS:
                        ColdflameJetsState = data;
                        if (ColdflameJetsState == DONE)
                            SaveToDB();
                        break;
                    case DATA_PUTRICIDE_TRAP:
                        PutricideTrap = data;
                        SaveToDB();
                        break;
                    case DATA_SINDRAGOSA_SPAWN:
                        SindragosaSpawn = data;
                        SaveToDB();
                        break;
                    case DATA_BLOOD_QUICKENING_STATE:
                    {
                        // skip if nothing changes
                        if (BloodQuickeningState == data)
                            break;

                        if (instance->GetInstanceId() % 5 != 3)
                            break;

                        switch (data)
                        {
                            case IN_PROGRESS:
                                Events.ScheduleEvent(EVENT_UPDATE_EXECUTION_TIME, 60000);
                                BloodQuickeningMinutes = 30;
                                DoUpdateWorldState(WORLDSTATE_SHOW_TIMER, 1);
                                DoUpdateWorldState(WORLDSTATE_EXECUTION_TIME, BloodQuickeningMinutes);
                                break;
                            case DONE:
                                Events.CancelEvent(EVENT_UPDATE_EXECUTION_TIME);
                                BloodQuickeningMinutes = 0;
                                DoUpdateWorldState(WORLDSTATE_SHOW_TIMER, 0);
                                break;
                            default:
                                break;
                        }

                        BloodQuickeningState = data;
                        SaveToDB();
                        break;
                    }
                    case DATA_PROFESSOR_PUTRICIDE_HC:
                        PutricideHCState = data;
                        SaveToDB();
                        break;
                    case DATA_BLOOD_QUEEN_LANA_THEL_HC:
                        BloodQueenHCState = data;
                        SaveToDB();
                        break;
                    case DATA_SINDRAGOSA_HC:
                        SindragosaHCState = data;
                        SaveToDB();
                        break;
                    case DATA_SAURFANG_DONE:
                        SaurfangDone = data;
                        SaveToDB();
                        break;
                    case DATA_BLOOD_WING_ENTRANCE:
                        BloodWingEntrance = data;
                        SaveToDB();
                        break;
                    case DATA_BUFF_AVAILABLE:
                    {
                        IsBuffAvailable = (data ? true : false);

                        for (Player& player : instance->GetPlayers())
                        {
                            HandleFactionBuff(&player, IsBuffAvailable);
                            if (Pet* pet = player.GetPet())
                                if (pet->IsPet() || pet->HasUnitTypeMask(UNIT_MASK_MINION | UNIT_MASK_GUARDIAN | UNIT_MASK_CONTROLABLE_GUARDIAN))
                                    HandleFactionBuff(pet, IsBuffAvailable);
                        }
                        break;
                    }
                    case DATA_ARTHAS_TELEPORTER_UNLOCK:
                        CheckLichKingAvailability();
                        break;
                    default:
                        break;
                }
            }

            bool CheckAchievementCriteriaMeet(uint32 criteria_id, Player const* /*source*/, Unit const* /*target*/, uint32 /*miscvalue1*/)
            {
                switch (criteria_id)
                {
                    case CRITERIA_BONED_10N:
                    case CRITERIA_BONED_25N:
                    case CRITERIA_BONED_10H:
                    case CRITERIA_BONED_25H:
                        return IsBonedEligible;
                    case CRITERIA_DANCES_WITH_OOZES_10N:
                    case CRITERIA_DANCES_WITH_OOZES_25N:
                    case CRITERIA_DANCES_WITH_OOZES_10H:
                    case CRITERIA_DANCES_WITH_OOZES_25H:
                        return IsOozeDanceEligible;
                    case CRITERIA_NAUSEA_10N:
                    case CRITERIA_NAUSEA_25N:
                    case CRITERIA_NAUSEA_10H:
                    case CRITERIA_NAUSEA_25H:
                        return IsNauseaEligible;
                    case CRITERIA_ORB_WHISPERER_10N:
                    case CRITERIA_ORB_WHISPERER_25N:
                    case CRITERIA_ORB_WHISPERER_10H:
                    case CRITERIA_ORB_WHISPERER_25H:
                        return IsOrbWhispererEligible;
                    // Only one criteria for both modes, need to do it like this
                    case CRITERIA_KILL_LANA_THEL_10M:
                    case CRITERIA_ONCE_BITTEN_TWICE_SHY_10N:
                    case CRITERIA_ONCE_BITTEN_TWICE_SHY_10V:
                        return instance->ToInstanceMap()->GetMaxPlayers() == 10;
                    case CRITERIA_KILL_LANA_THEL_25M:
                    case CRITERIA_ONCE_BITTEN_TWICE_SHY_25N:
                    case CRITERIA_ONCE_BITTEN_TWICE_SHY_25V:
                        return instance->ToInstanceMap()->GetMaxPlayers() == 25;
                    default:
                        break;
                }

                return false;
            }

            bool CheckRequiredBosses(uint32 bossId, Player const* player = NULL) const
            {
                if (player && player->GetSession()->HasPermission(rbac::RBAC_PERM_SKIP_CHECK_INSTANCE_REQUIRED_BOSSES))
                    return true;

                switch (bossId)
                {
                    case DATA_THE_LICH_KING:
                        if (!CheckPlagueworks(bossId))
                            return false;
                        if (!CheckCrimsonHalls(bossId))
                            return false;
                        if (!CheckFrostwingHalls(bossId))
                            return false;
                        if (!LeftAttempts)
                            return false;
                        break;
                    case DATA_SINDRAGOSA:
                        if (!LeftAttempts)
                            return false;
                    case DATA_VALITHRIA_DREAMWALKER:
                        if (!CheckFrostwingHalls(bossId))
                            return false;
                        break;
                    case DATA_BLOOD_QUEEN_LANA_THEL:
                        if (!LeftAttempts)
                            return false;
                    case DATA_BLOOD_PRINCE_COUNCIL:
                        if (!CheckCrimsonHalls(bossId))
                            return false;
                        break;
                    case DATA_PROFESSOR_PUTRICIDE:
                        if (!LeftAttempts)
                            return false;
                    case DATA_FESTERGUT:
                    case DATA_ROTFACE:
                        if (!CheckPlagueworks(bossId))
                            return false;
                        break;
                    default:
                        break;
                }

                if (!CheckLowerSpire(bossId))
                    return false;

                return true;
            }

            bool CheckPlagueworks(uint32 bossId) const
            {
                switch (bossId)
                {
                    case DATA_THE_LICH_KING:
                        if (GetBossState(DATA_PROFESSOR_PUTRICIDE) != DONE)
                            return false;
                        // no break
                    case DATA_PROFESSOR_PUTRICIDE:
                        if (GetBossState(DATA_FESTERGUT) != DONE || GetBossState(DATA_ROTFACE) != DONE)
                            return false;
                        break;
                    default:
                        break;
                }

                return true;
            }

            bool CheckCrimsonHalls(uint32 bossId) const
            {
                switch (bossId)
                {
                    case DATA_THE_LICH_KING:
                        if (GetBossState(DATA_BLOOD_QUEEN_LANA_THEL) != DONE)
                            return false;
                        // no break
                    case DATA_BLOOD_QUEEN_LANA_THEL:
                        if (GetBossState(DATA_BLOOD_PRINCE_COUNCIL) != DONE)
                            return false;
                        break;
                    default:
                        break;
                }

                return true;
            }

            bool CheckFrostwingHalls(uint32 bossId) const
            {
                switch (bossId)
                {
                    case DATA_THE_LICH_KING:
                        if (GetBossState(DATA_SINDRAGOSA) != DONE)
                            return false;
                        // no break
                    case DATA_SINDRAGOSA:
                        if (GetBossState(DATA_VALITHRIA_DREAMWALKER) != DONE)
                            return false;
                        break;
                    default:
                        break;
                }

                return true;
            }

            bool CheckLowerSpire(uint32 bossId) const
            {
                switch (bossId)
                {
                    case DATA_THE_LICH_KING:
                    case DATA_SINDRAGOSA:
                    case DATA_BLOOD_QUEEN_LANA_THEL:
                    case DATA_PROFESSOR_PUTRICIDE:
                    case DATA_VALITHRIA_DREAMWALKER:
                    case DATA_BLOOD_PRINCE_COUNCIL:
                    case DATA_ROTFACE:
                    case DATA_FESTERGUT:
                    //    if (GetBossState(DATA_DEATHBRINGER_SAURFANG) != DONE)
                    //        return false;
                        // no break
                    case DATA_DEATHBRINGER_SAURFANG:
                        if (GetBossState(DATA_ICECROWN_GUNSHIP_BATTLE) != DONE)
                            return false;
                        // no break
                    case DATA_ICECROWN_GUNSHIP_BATTLE:
                        if (GetBossState(DATA_LADY_DEATHWHISPER) != DONE)
                            return false;
                        // no break
                    case DATA_LADY_DEATHWHISPER:
                        if (GetBossState(DATA_LORD_MARROWGAR) != DONE)
                            return false;
                        // no break
                    case DATA_LORD_MARROWGAR:
                    default:
                        break;
                }

                return true;
            }

            void CheckLichKingAvailability()
            {
                if (IsArthasTeleporterUnlocked)
                    return;

                if (GetBossState(DATA_PROFESSOR_PUTRICIDE) == DONE && GetBossState(DATA_BLOOD_QUEEN_LANA_THEL) == DONE && GetBossState(DATA_SINDRAGOSA) == DONE && GetBossState(DATA_THE_LICH_KING) != DONE)
                {
                    IsArthasTeleporterUnlocked = true;

                    const uint32 SOUND_EVENT_ARTHAS_TELEPORT_UNLOCK = 17329;

                    for (Player& player : instance->GetPlayers())
                        player.PlayDirectSound(SOUND_EVENT_ARTHAS_TELEPORT_UNLOCK);

                    if (GameObject* teleporter = instance->GetGameObject(TheLichKingTeleportGUID))
                    {
                        teleporter->SetGoState(GO_STATE_ACTIVE);

                        std::list<Creature*> stalkers;
                        teleporter->GetCreatureListWithEntryInGrid(stalkers, NPC_INVISIBLE_STALKER, 100.0f);
                        if (stalkers.empty())
                            return;

                        stalkers.sort(Trinity::ObjectDistanceOrderPred(teleporter));
                        stalkers.front()->CastSpell((Unit*)NULL, SPELL_ARTHAS_TELEPORTER_CEREMONY, false);
                        stalkers.pop_front();
                        for (std::list<Creature*>::iterator itr = stalkers.begin(); itr != stalkers.end(); ++itr)
                            (*itr)->AI()->Reset();
                    }
                }
            }

            void WriteSaveDataMore(std::ostringstream& data) override
            {
                data << LeftAttempts          << ' '
                    << ColdflameJetsState     << ' '
                    << BloodQuickeningState   << ' '
                    << BloodQuickeningMinutes << ' '
                    << PutricideTrap          << ' '
                    << TheDamned              << ' '
                    << SpiderAttack           << ' '
                    << FrostwyrmLeft          << ' '
                    << FrostwyrmRight         << ' '
                    << SindragosaSpawn        << ' '
                    << PutricideHCState       << ' '
                    << BloodQueenHCState      << ' '
                    << SindragosaHCState      << ' '
                    << SaurfangDone           << ' '
                    << BloodWingEntrance;
            }

            void ReadSaveDataMore(std::istringstream& data) override
            {
                data >> LeftAttempts;

                uint32 temp = 0;
                data >> temp;
                ColdflameJetsState = temp ? DONE : NOT_STARTED;

                data >> temp;
                BloodQuickeningState = temp ? DONE : NOT_STARTED;   // DONE means finished (not success/fail)
                data >> BloodQuickeningMinutes;
                data >> PutricideTrap;
                data >> TheDamned;
                data >> SpiderAttack;
                if (SpiderAttack != DONE)
                    SpiderAttack = NOT_STARTED;
                data >> FrostwyrmLeft;
                data >> FrostwyrmRight;
                data >> SindragosaSpawn;
                data >> PutricideHCState;
                data >> BloodQueenHCState;
                data >> SindragosaHCState;
                data >> SaurfangDone;
                data >> BloodWingEntrance;
            }

            void Update(uint32 diff)
            {
                if (Events.Empty())
                    return;

                Events.Update(diff);

                while (uint32 eventId = Events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_UPDATE_EXECUTION_TIME:
                        {
                            --BloodQuickeningMinutes;
                            if (BloodQuickeningMinutes)
                            {
                                Events.ScheduleEvent(EVENT_UPDATE_EXECUTION_TIME, 60000);
                                DoUpdateWorldState(WORLDSTATE_SHOW_TIMER, 1);
                                DoUpdateWorldState(WORLDSTATE_EXECUTION_TIME, BloodQuickeningMinutes);
                            }
                            else
                            {
                                BloodQuickeningState = DONE;
                                DoUpdateWorldState(WORLDSTATE_SHOW_TIMER, 0);
                                if (Creature* bq = instance->GetCreature(BloodQueenLanaThelGUID))
                                    bq->AI()->DoAction(ACTION_KILL_MINCHAR);
                            }
                            SaveToDB();
                            break;
                        }
                        case EVENT_QUAKE_SHATTER:
                        {
                            if (GameObject* platform = instance->GetGameObject(ArthasPlatformGUID))
                                platform->SetDestructibleState(GO_DESTRUCTIBLE_DAMAGED);
                            if (GameObject* edge = instance->GetGameObject(FrozenThroneEdgeGUID))
                                edge->SetGoState(GO_STATE_ACTIVE);
                            if (GameObject* wind = instance->GetGameObject(FrozenThroneWindGUID))
                                wind->SetGoState(GO_STATE_READY);
                            if (GameObject* warning = instance->GetGameObject(FrozenThroneWarningGUID))
                                warning->SetGoState(GO_STATE_READY);
                            if (Creature* theLichKing = instance->GetCreature(TheLichKingGUID))
                                theLichKing->AI()->DoAction(ACTION_RESTORE_LIGHT);
                            break;
                        }
                        case EVENT_REBUILD_PLATFORM:
                            if (GameObject* platform = instance->GetGameObject(ArthasPlatformGUID))
                                platform->SetDestructibleState(GO_DESTRUCTIBLE_REBUILDING);
                            if (GameObject* edge = instance->GetGameObject(FrozenThroneEdgeGUID))
                                edge->SetGoState(GO_STATE_READY);
                            if (GameObject* wind = instance->GetGameObject(FrozenThroneWindGUID))
                                wind->SetGoState(GO_STATE_ACTIVE);
                            break;
                        case EVENT_RESPAWN_GUNSHIP:
                            SpawnGunship();
                            break;
                        case EVENT_FILL_GREEN_DOOR:
                            HandleGameObject(PutricideGateGUIDs[1], false);
                            PutricidePipeGreenFilled = true;
                            if (PutricidePipeGreenFilled && PutricidePipeOrangeFilled)
                                Events.ScheduleEvent(EVENT_OPEN_PUTRICIDE_DOOR, 5*IN_MILLISECONDS);
                            break;
                        case EVENT_FILL_ORANGE_DOOR:
                            HandleGameObject(PutricideGateGUIDs[0], false);
                            PutricidePipeOrangeFilled = true;
                            if (PutricidePipeGreenFilled && PutricidePipeOrangeFilled)
                                Events.ScheduleEvent(EVENT_OPEN_PUTRICIDE_DOOR, 5*IN_MILLISECONDS);
                            break;
                        case EVENT_OPEN_PUTRICIDE_DOOR:
                            if (GameObject* go = instance->GetGameObject(PutricideGateGUIDs[0]))
                                go->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                            if (GameObject* go = instance->GetGameObject(PutricideGateGUIDs[1]))
                                go->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                            HandleGameObject(PutricideCollisionGUID, true);
                            break;
                        case EVENT_FADE_LEFT_ATTEMTS:
                            DoUpdateWorldState(WORLDSTATE_SHOW_ATTEMPTS, 0);
                            break;
                        case EVENT_SPAWN_DAMNED:
                            if (Creature* summoner = instance->GetCreature(MarrowGarGUID))
                            {
                                if (InstanceScript* instance = summoner->GetInstanceScript())
                                { 
                                    if (instance->GetData(DATA_THE_DAMNED) == 0)
                                    {
                                        if (Creature* spawner1 = summoner->SummonCreature(NPC_THE_DAMNED, -147.040848f, 2211.435303f, 35.233490f))
                                        {
                                            spawner1->SetWalk(false);
                                            spawner1->setActive(true);
                                            spawner1->GetMotionMaster()->MovePath(WP_THE_DAMNED_RIGHT, false);
                                            spawner1->RemoveLootMode(1);
                                            spawner1->DespawnOrUnsummon(25 * IN_MILLISECONDS);
                                            spawner1->SetDisableReputationGain(true);
                                        }
                                        if (Creature* spawner2 = summoner->SummonCreature(NPC_THE_DAMNED, -147.040848f, 2211.435303f, 35.233490f))
                                        {
                                            spawner2->SetWalk(false);
                                            spawner2->setActive(true);
                                            spawner2->GetMotionMaster()->MovePath(WP_THE_DAMNED_LEFT, false);
                                            spawner2->RemoveLootMode(1);
                                            spawner2->DespawnOrUnsummon(25 * IN_MILLISECONDS);
                                            spawner2->SetDisableReputationGain(true);
                                        }
                                        Events.ScheduleEvent(EVENT_SPAWN_DAMNED, urand (10, 15) * IN_MILLISECONDS);
                                    }
                                }                                                                    
                            }                            
                            break;
                            // Spider Attack Event in front of Sindragosa
                        case EVENT_SPIDER_ATTACK:
                            // 0:15
                            if (SpiderCounter == 0)                                                           
                                SummonChampion();
                            // 0:30
                            if (SpiderCounter == 1) 
                                SummonBroodling();
                            // 0:45
                            if (SpiderCounter == 2) 
                                SummonFrostwarden();
                            // 1:00
                            if (SpiderCounter == 3) 
                                SummonBroodling();
                            // 1:15
                            if (SpiderCounter == 4) 
                                SummonChampion();
                            // 1:30
                            if (SpiderCounter == 5) 
                                SummonBroodling();
                            // 1:45
                            if (SpiderCounter == 6)
                                SummonBroodling();
                            // 2:00
                            if (SpiderCounter == 7)
                                if (Creature* summoner = instance->GetCreature(SpinestalkerGUID))
                                {
                                    if (GameObject* go = summoner->FindNearestGameObject(GO_SINDRAGOSA_ENTRANCE_DOOR, 1000.0f))
                                        go->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                                    SetData(DATA_SPIDER_ATTACK, DONE);
                                }
                            if (++SpiderCounter <= 7)
                                Events.ScheduleEvent(EVENT_SPIDER_ATTACK, urand(13, 15) * IN_MILLISECONDS);
                            break;
                        case EVENT_RESPAWN_SAURFANG:
                            if (Creature* deathbringer = instance->GetCreature(DeathbringerSaurfangGUID))
                            { 
                                deathbringer->SetVisible(true);
                                deathbringer->setFaction(FACTION_HOSTILE);
                            }
                            break;
                        case EVENT_BLOOD_WING_ENTRANCE:
                            instance->LoadGrid(4580.881f, 2770.451f);
                            if (GetData(DATA_BLOOD_WING_ENTRANCE) == 1)
                            {
                                if (GameObject* door = instance->GetGameObject(CrimsonHallDoorGUID))
                                    HandleGameObject(CrimsonHallDoorGUID, true, door);
                            }                                
                            else
                            {
                                if (GameObject* door = instance->GetGameObject(CrimsonHallDoorGUID))
                                    HandleGameObject(CrimsonHallDoorGUID, false, door);
                            }
                            break;
                        default:
                            break;
                    }
                }
            }

            void ProcessEvent(WorldObject* source, uint32 eventId)
            {
                switch (eventId)
                {
                    case EVENT_ENEMY_GUNSHIP_DESPAWN:
                        if (GetBossState(DATA_ICECROWN_GUNSHIP_BATTLE) == DONE)
                            source->AddObjectToRemoveList();
                        break;
                    case EVENT_ENEMY_GUNSHIP_COMBAT:
                        if (Creature* captain = source->FindNearestCreature(TeamInInstance == HORDE ? NPC_IGB_HIGH_OVERLORD_SAURFANG : NPC_IGB_MURADIN_BRONZEBEARD, 100.0f))
                            captain->AI()->DoAction(ACTION_ENEMY_GUNSHIP_TALK);
                        // no break;
                    case EVENT_PLAYERS_GUNSHIP_SPAWN:
                    case EVENT_PLAYERS_GUNSHIP_COMBAT:
                        if (GameObject* go = source->ToGameObject())
                            if (MotionTransport* transport = go->ToMotionTransport())
                                transport->EnableMovement(false);
                        break;
                    case EVENT_PLAYERS_GUNSHIP_SAURFANG:
                    {
                        if (Creature* captain = source->FindNearestCreature(TeamInInstance == HORDE ? NPC_IGB_HIGH_OVERLORD_SAURFANG : NPC_IGB_MURADIN_BRONZEBEARD, 100.0f))
                            captain->AI()->DoAction(ACTION_EXIT_SHIP);
                        if (GameObject* go = source->ToGameObject())
                            if (MotionTransport* transport = go->ToMotionTransport())
                                transport->EnableMovement(false);
                        break;
                    }
                    case EVENT_FILL_GREEN_TUBES:
                        if (GetBossState(DATA_ROTFACE) != DONE)
                            break;
                        HandleGameObject(PutricidePipeGUIDs[1], true);
                        source->ToGameObject()->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        Events.ScheduleEvent(EVENT_FILL_GREEN_DOOR, 9*IN_MILLISECONDS);
                        break;
                    case EVENT_FILL_ORANGE_TUBES:
                        if (GetBossState(DATA_FESTERGUT) != DONE)
                            break;
                        HandleGameObject(PutricidePipeGUIDs[0], true);
                        source->ToGameObject()->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        Events.ScheduleEvent(EVENT_FILL_ORANGE_DOOR, 9*IN_MILLISECONDS);
                        break;
                    case EVENT_QUAKE:
                        if (GameObject* warning = instance->GetGameObject(FrozenThroneWarningGUID))
                            warning->SetGoState(GO_STATE_ACTIVE);
                        Events.ScheduleEvent(EVENT_QUAKE_SHATTER, 5000);
                        break;
                    case EVENT_SECOND_REMORSELESS_WINTER:
                        if (GameObject* platform = instance->GetGameObject(ArthasPlatformGUID))
                        {
                            platform->SetDestructibleState(GO_DESTRUCTIBLE_DESTROYED);
                            Events.ScheduleEvent(EVENT_REBUILD_PLATFORM, 1500);
                        }
                        break;
                    case EVENT_TELEPORT_TO_FROSMOURNE: // Harvest Soul (normal mode)
                        if (Creature* terenas = instance->SummonCreature(NPC_TERENAS_MENETHIL_FROSTMOURNE, TerenasSpawn, NULL, 70000))
                        {
                            terenas->AI()->DoAction(ACTION_FROSTMOURNE_INTRO);
                            std::list<Creature*> triggers;
                            terenas->GetCreatureListWithEntryInGrid(triggers, NPC_WORLD_TRIGGER_INFINITE_AOI, 100.0f);
                            if (!triggers.empty())
                            {
                                triggers.sort(Trinity::ObjectDistanceOrderPred(terenas, false));
                                Unit* visual = triggers.front();
                                visual->CastSpell(visual, SPELL_FROSTMOURNE_TELEPORT_VISUAL, true);
                            }

                            if (Creature* warden = instance->SummonCreature(NPC_SPIRIT_WARDEN, SpiritWardenSpawn, NULL, 63000))
                            {
                                warden->EngageWithTarget(terenas);
                                warden->GetThreatManager().AddThreat(terenas, 300000.0f);
                            }
                        }
                        break;
                }
            }

        protected:
            EventMap Events;
            ObjectGuid MarrowGarGUID;
            ObjectGuid LadyDeathwisperElevatorGUID;
            ObjectGuid GunshipGUID;
            ObjectGuid EnemyGunshipGUID;
            ObjectGuid GunshipArmoryGUID;
            ObjectGuid DeathbringerSaurfangGUID;
            ObjectGuid DeathbringerSaurfangDoorGUID;
            ObjectGuid DeathbringerSaurfangEventGUID;   // Muradin Bronzebeard or High Overlord Saurfang
            ObjectGuid DeathbringersCacheGUID;
            ObjectGuid DeathbringerEndEventTriggerGUID;
            ObjectGuid SaurfangTeleportGUID;
            ObjectGuid PlagueSigilGUID;
            ObjectGuid BloodwingSigilGUID;
            ObjectGuid FrostwingSigilGUID;
            ObjectGuid CrimsonHallDoorGUID;
            ObjectGuid PutricidePipeGUIDs[2];
            ObjectGuid PutricideGateGUIDs[2];
            bool PutricidePipeGreenFilled;
            bool PutricidePipeOrangeFilled;
            ObjectGuid PutricideCollisionGUID;
            uint32 PutricideTrap;
            ObjectGuid FestergutGUID;
            ObjectGuid RotfaceGUID;
            ObjectGuid PutricideDoorGUID;
            ObjectGuid ProfessorPutricideGUID;
            ObjectGuid PutricideTableGUID;
            boss_professor_putricideAI* PutricideNormalAI = nullptr;
            boss_professor_putricide_dummyAI* PutricidePreAI = nullptr;
            ObjectGuid BloodCouncilGUIDs[3];
            ObjectGuid BloodCouncilControllerGUID;
            ObjectGuid BloodQueenLanaThelTriggerGUID;
            ObjectGuid BloodQueenLanaThelGUID;
            ObjectGuid CrokScourgebaneGUID;
            ObjectGuid CrokCaptainGUIDs[4];
            ObjectGuid SisterSvalnaGUID;
            ObjectGuid ValithriaDreamwalkerGUID;
            ObjectGuid ValithriaLichKingGUID;
            ObjectGuid ValithriaTriggerGUID;
            ObjectGuid SindragosaDoorGUID;
            ObjectGuid SindragosaGUID;
            ObjectGuid SpinestalkerGUID;
            ObjectGuid RimefangGUID;
            time_t SindragosaRespawnDelay;
            ObjectGuid TheLichKingTeleportGUID;
            ObjectGuid TheLichKingGUID;
            ObjectGuid HighlordTirionFordringGUID;
            ObjectGuid TerenasMenethilGUID;
            ObjectGuid ArthasPlatformGUID;
            ObjectGuid ArthasPrecipiceGUID;
            ObjectGuid FrozenThroneEdgeGUID;
            ObjectGuid FrozenThroneWindGUID;
            ObjectGuid FrozenThroneWarningGUID;
            ObjectGuid FrozenBolvarGUID;
            ObjectGuid PillarsChainedGUID;
            ObjectGuid PillarsUnchainedGUID;
            uint32 TeamInInstance;
            uint32 ColdflameJetsState;
            uint32 PutricideHCState;
            uint32 BloodQueenHCState;
            uint32 SindragosaHCState;
            uint32 SaurfangDone;
            std::set<uint32> FrostwyrmGUIDs;
            std::set<uint32> SpinestalkerTrash;
            std::set<uint32> RimefangTrash;
            std::set<ObjectGuid> DarkfallenCreaturesGuids;
            uint32 BloodQuickeningState;
            uint32 MaxAttempts;
            uint32 LeftAttempts;
            uint16 BloodQuickeningMinutes;
            uint32 TheDamned;
            uint32 SpiderAttack;
            uint32 FrostwyrmLeft;
            uint32 FrostwyrmRight;
            uint32 SindragosaSpawn;
            uint32 BloodWingEntrance;
            uint8 SpiderCounter;
            bool IsBonedEligible;
            bool IsOozeDanceEligible;
            bool IsNauseaEligible;
            bool IsOrbWhispererEligible;
            bool IsBuffAvailable;
            bool IsArthasTeleporterUnlocked;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_icecrown_citadel_InstanceMapScript(map);
        }
};

void AddSC_instance_icecrown_citadel()
{
    new instance_icecrown_citadel();
}
