/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2012-2015 Rising-Gods <http://www.rising-gods.de/>
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
#include "ScriptedGossip.h"
#include "Vehicle.h"
#include "trial_of_the_champion.h"
#include "Player.h"
#include "SpellScript.h"

enum Yells
{
    // Arelas Brightstar / Jaeren Sunsworn
    SAY_STAGE_1_000             = 0,
    SAY_STAGE_1_050             = 1,
    SAY_STAGE_1_060             = 2,
    SAY_STAGE_1_070             = 3,
    SAY_STAGE_1_080             = 4,
    SAY_STAGE_1_090             = 5,
    SAY_STAGE_2_010             = 6,
    SAY_STAGE_2_020             = 7,
    SAY_STAGE_3_010             = 8,

    // Tirion
    SAY_STAGE_1_010             = 50,
    SAY_STAGE_1_040             = 51,
    SAY_STAGE_1_100             = 52,
    SAY_STAGE_2_000             = 53,
    SAY_STAGE_2_060             = 54,
    SAY_STAGE_3_000             = 55,
    SAY_STAGE_3_020             = 56,
    SAY_STAGE_3_030             = 57,
    SAY_STAGE_3_040             = 58,

    // Eadric
    SAY_STAGE_2_030             = 0,
    // Paletress
    SAY_STAGE_2_040             = 0,
    SAY_STAGE_2_050             = 1,

    // Trall
    SAY_STAGE_1_015             = 0,
    SAY_STAGE_3_050h            = 1,
    // Garrosh
    SAY_STAGE_1_020             = 50,
    SAY_STAGE_3_025h            = 51,
    // Varian
    SAY_STAGE_1_030             = 50,
    SAY_STAGE_3_025a            = 51,
    SAY_STAGE_3_050a            = 52
};

const Position SpawnPosition = {746.261f, 657.401f, 411.681f, 4.65f};

enum ToC5AnnouncerGossipItems
{
    MENU_ID                     = 60073,

    GOSSIP_ITEM_STAGE_1          = 1,
    GOSSIP_ITEM_STAGE_SKIP_INTRO = 2,
    GOSSIP_ITEM_STAGE_2          = 3,
    GOSSIP_ITEM_STAGE_3          = 4
};

enum ToC5AnnouncerGossipActions
{
    G_ACTION_CHAMPIONS           = 1000,
    G_ACTION_SKIP_INTRO          = 1001,
    G_ACTION_ARGENT_CHALLANGE    = 1002,
    G_ACTION_BLACK_KNIGHT        = 1003
};

enum ToC5AnnouncerMessages
{
    MSG_FIRST           = 100,
    MSG_SECOND          = 101
};

struct Messages
{
    ToC5AnnouncerMessages message;
    uint32 actionId;
    ToC5AnnouncerGossipItems item;
    uint32 encounter;
};

#define NUM_MESSAGES    3

static Messages GossipMessage[NUM_MESSAGES] =
{
//  1 - message  2 - actionId               3 - item             4 - encounter
    {MSG_SECOND, G_ACTION_CHAMPIONS,        GOSSIP_ITEM_STAGE_1, BOSS_GRAND_CHAMPIONS},
    {MSG_SECOND, G_ACTION_ARGENT_CHALLANGE, GOSSIP_ITEM_STAGE_2, BOSS_ARGENT_CHALLENGE},
    {MSG_SECOND, G_ACTION_BLACK_KNIGHT,     GOSSIP_ITEM_STAGE_3, BOSS_BLACK_KNIGHT},
};

static const Position PositionBossSpawn = { 746.396667f, 687.246277f, 412.374481f, 1.5f * static_cast<float>(M_PI) };

struct ChampionWave
{
    uint32 boss;
    uint32 bossVehicle;
    uint32 champion;
    uint32 championVehicle;
    uint32 fxTriggerEntry;
    uint32 announcerIntroduceYell;
};

static const ChampionWave ChampionWavesHorde[] =
{
    { NPC_MOKRA,   VEHICLE_MOKRA_SKILLCRUSHER_MOUNT,   NPC_ORGRIMAR_CHAMPION,   VEHICLE_ORGRIMMAR_WOLF,         NPC_FX_TRIGGER_ORC,      SAY_STAGE_1_070 },
    { NPC_ERESSEA, VEHICLE_ERESSEA_DAWNSINGER_MOUNT,   NPC_SILVERMOON_CHAMPION, VEHICLE_SILVERMOON_HAWKSTRIDER, NPC_FX_TRIGGER_BLOODELF, SAY_STAGE_1_050 },
    { NPC_RUNOK,   VEHICLE_RUNOK_WILDMANE_MOUNT,       NPC_THUNDER_CHAMPION,    VEHICLE_THUNDER_BLUFF_KODO,     NPC_FX_TRIGGER_TAURE,    SAY_STAGE_1_090 },
    { NPC_ZULTORE, VEHICLE_ZUL_TORE_MOUNT,             NPC_TROLL_CHAMPION,      VEHICLE_DARKSPEAR_RAPTOR,       NPC_FX_TRIGGER_TROLL,    SAY_STAGE_1_060 },
    { NPC_VISCERI, VEHICLE_DEATHSTALKER_VESCERI_MOUNT, NPC_UNDERCITY_CHAMPION,  VEHICLE_FORSAKE_WARHORSE,       NPC_FX_TRIGGER_UNDEAD,   SAY_STAGE_1_080 },
};

static const ChampionWave ChampionWavesAlliance[] =
{
    { NPC_JACOB,   VEHICLE_MARSHAL_JACOB_ALERIUS_MOUNT, NPC_STORMWIND_CHAMPION,  VEHICLE_STORMWIND_STEED,           NPC_FX_TRIGGER_HUMAN,    SAY_STAGE_1_080 },
    { NPC_AMBROSE, VEHICLE_AMBROSE_BOLTSPARK_MOUNT,     NPC_GNOMERAGN_CHAMPION,  VEHICLE_GNOMEREGAN_MECHANOSTRIDER, NPC_FX_TRIGGER_GNOME,    SAY_STAGE_1_070 },
    { NPC_COLOSOS, VEHICLE_COLOSOS_MOUNT,               NPC_EXODAR_CHAMPION,     VEHICLE_EXODAR_ELEKK,              NPC_FX_TRIGGER_DRAENEI,  SAY_STAGE_1_050 },
    { NPC_JAELYNE, VEHICLE_JAELYNES_MOUNT,              NPC_DRNASSUS_CHAMPION,   VEHICLE_DARNASSIA_NIGHTSABER,      NPC_FX_TRIGGER_NIGHTELF, SAY_STAGE_1_060 },
    { NPC_LANA,    VEHICLE_LANA_STOUTHAMMER_MOUNT,      NPC_IRONFORGE_CHAMPION,  VEHICLE_IRONFORGE_RAM,             NPC_FX_TRIGGER_DWARF,    SAY_STAGE_1_090 },
};

class npc_announcer_toc5 : public CreatureScript
{
public:
    npc_announcer_toc5() : CreatureScript("npc_announcer_toc5") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        InstanceScript* instance = creature->GetInstanceScript();

        if (!instance)
            return true;

        if (instance->GetBossState(GossipMessage[0].encounter) != DONE)
        {
            if (player->HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT))
            {
                if (player->GetTeam() == HORDE)
                {
                    if (player->HasAchieved(instance->instance->IsHeroic() ? ACHIEVEMENT_TOC5_HERO_H : ACHIEVEMENT_TOC5_H))
                        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(MENU_ID, GOSSIP_ITEM_STAGE_SKIP_INTRO), GOSSIP_SENDER_MAIN, G_ACTION_SKIP_INTRO);
                }
                else if (player->GetTeam() == ALLIANCE)
                {
                    if (player->HasAchieved(instance->instance->IsHeroic() ? ACHIEVEMENT_TOC5_HERO_A : ACHIEVEMENT_TOC5_A))
                        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(MENU_ID, GOSSIP_ITEM_STAGE_SKIP_INTRO), GOSSIP_SENDER_MAIN, G_ACTION_SKIP_INTRO);
                }

                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(MENU_ID, GossipMessage[0].item), GOSSIP_SENDER_MAIN, GossipMessage[0].actionId);
                player->SEND_GOSSIP_MENU(GossipMessage[0].message, creature->GetGUID());
                return true;
            }
            else
            {
                player->SEND_GOSSIP_MENU(MSG_FIRST, creature->GetGUID());
                return true;
            }
        }

        uint8 i = 0;
        for (; i < NUM_MESSAGES; ++i)
        {
            if (instance->GetBossState(GossipMessage[i].encounter) != DONE)
            {
                if (i == 1) // Argent Challenge
                    if (Creature* argentchampion = ObjectAccessor::GetCreature(*creature, ObjectGuid(instance->GetGuidData(NPC_EADRIC))))
                        if (argentchampion->IsAlive())
                            break;
                
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(MENU_ID, GossipMessage[i].item), GOSSIP_SENDER_MAIN, GossipMessage[i].actionId);
                break;
            }
        }
        player->SEND_GOSSIP_MENU(GossipMessage[i].message, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        InstanceScript* instance = creature->GetInstanceScript();

        if (!instance)
            return true;

        switch (action)
        {
            case G_ACTION_CHAMPIONS:
                instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_CHAMPIONS_000);
                creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                break;
            case G_ACTION_SKIP_INTRO:
                instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_CHAMPIONS_040);
                creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                break;
            case G_ACTION_ARGENT_CHALLANGE:
                instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_ARGENT_CHALLENGE_000);
                break;
            case G_ACTION_BLACK_KNIGHT:
                instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_BLACK_KNIGHT_000);
                break;
        }
        player->CLOSE_GOSSIP_MENU();

        creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        return true;
    }

    struct npc_announcer_toc5AI : public ScriptedAI
    {
        npc_announcer_toc5AI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            instance = me->GetInstanceScript();
        }

        void UpdateAI(uint32 diff)
        {
            if (!instance)
                return;

            if (instance->GetData(DATA_EVENT_NPC) != me->GetEntry())
                return;

            UpdateTimer = instance->GetData(DATA_EVENT_TIMER);

            if (UpdateTimer <= diff)
            {
                switch (instance->GetData(DATA_EVENT_STAGE))
                {
                    case EVENT_STAGE_CHAMPIONS_000:
                        Talk(SAY_STAGE_1_000);
                        UpdateTimer = 5*IN_MILLISECONDS;
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_CHAMPIONS_010);
                        break;
                    case EVENT_STAGE_ARGENT_CHALLENGE_020:
                        Talk(instance->GetData(DATA_ARGENT_CHAMPION) == NPC_EADRIC ? SAY_STAGE_2_010 : SAY_STAGE_2_020);
                        UpdateTimer = 5*IN_MILLISECONDS;
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_ARGENT_CHALLENGE_030);
                        break;
                    case EVENT_STAGE_BLACK_KNIGHT_020:
                        if (Creature* knight = ObjectAccessor::GetCreature(*me, instance ? instance->GetGuidData(NPC_BLACK_KNIGHT): ObjectGuid::Empty))
                        {
                            me->HandleEmoteCommand(EMOTE_ONESHOT_POINT);
                            me->SetUInt64Value(UNIT_FIELD_TARGET, knight->GetGUID().GetRawValue());
                        }
                        Talk(SAY_STAGE_3_010);
                        UpdateTimer = 2*IN_MILLISECONDS;
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_BLACK_KNIGHT_030);
                        break;
                    default:
                        break;
                }
            } else UpdateTimer -= diff;

            instance->SetData(DATA_EVENT_TIMER, UpdateTimer);
        }

    private:
        InstanceScript* instance;
        uint32 UpdateTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_announcer_toc5AI(creature);
    }
};

class npc_tirion_toc5 : public CreatureScript
{
public:
    npc_tirion_toc5() : CreatureScript("npc_tirion_toc5") { }

    struct npc_tirion_toc5AI : public ScriptedAI
    {
        npc_tirion_toc5AI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void UpdateAI(uint32 diff)
        {
            if (!instance)
                return;

            if (instance->GetData(DATA_EVENT_NPC) != me->GetEntry())
                return;

            UpdateTimer = instance->GetData(DATA_EVENT_TIMER);

            if (UpdateTimer <= diff)
            {
                switch (instance->GetData(DATA_EVENT_STAGE))
                {
                    case EVENT_STAGE_CHAMPIONS_010:
                        Talk(SAY_STAGE_1_010);
                        UpdateTimer = 5*IN_MILLISECONDS;
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_CHAMPIONS_015);
                        break;
                    case EVENT_STAGE_CHAMPIONS_040:
                        ChampionsInit();
                        Talk(SAY_STAGE_1_040);
                        UpdateTimer = 5*IN_MILLISECONDS;
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_CHAMPIONS_WAVE_1);
                        break;
                    case EVENT_STAGE_CHAMPIONS_WAVE_1:
                        ChampionSummon();
                        if (instance)
                        {
                            instance->SetBossState(BOSS_GRAND_CHAMPIONS, IN_PROGRESS);
                            if (Creature* announcer = ObjectAccessor::GetCreature(*me, ObjectGuid((uint64)instance->GetData(DATA_ANNOUNCER))))
                                announcer->AI()->Talk(AnnouncerIntroduceYell);
                        }
                        UpdateTimer = 5*IN_MILLISECONDS;
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_CHAMPIONS_WAVE_2);
                        break;
                    case EVENT_STAGE_CHAMPIONS_WAVE_2:
                        ChampionSummon();
                        if (Creature* announcer = ObjectAccessor::GetCreature(*me, ObjectGuid(instance ? instance->GetData(DATA_ANNOUNCER) : (uint64)0)))
                            announcer->AI()->Talk(AnnouncerIntroduceYell);
                        UpdateTimer = 5*IN_MILLISECONDS;
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_CHAMPIONS_WAVE_3);
                        break;
                    case EVENT_STAGE_CHAMPIONS_WAVE_3:
                        ChampionSummon();
                        if (Creature* announcer = ObjectAccessor::GetCreature(*me, ObjectGuid(instance ? instance->GetData(DATA_ANNOUNCER) : (uint64)0)))
                            announcer->AI()->Talk(AnnouncerIntroduceYell);
                        UpdateTimer = 18*IN_MILLISECONDS;
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_CHAMPIONS_050);
                        break;
                    case EVENT_STAGE_CHAMPIONS_050:
                        Talk(SAY_STAGE_1_100);
                        UpdateTimer = 2*IN_MILLISECONDS;
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_CHAMPIONS_WAVE_1_START);
                        break;
                    case EVENT_STAGE_CHAMPIONS_WAVE_1_START:
                    case EVENT_STAGE_CHAMPIONS_WAVE_2_START:
                    case EVENT_STAGE_CHAMPIONS_WAVE_3_START:
                    case EVENT_STAGE_CHAMPIONS_WAVE_BOSS:
                    case EVENT_STAGE_CHAMPIONS_WAVE_BOSS_P2:
                        ChampionActivate();
                        instance->SetData(DATA_EVENT_STAGE, 0);
                        break;
                    case EVENT_STAGE_ARGENT_CHALLENGE_000:
                        Talk(SAY_STAGE_2_000);
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_ARGENT_CHALLENGE_010);
                        UpdateTimer = 5*IN_MILLISECONDS;
                        break;
                    case EVENT_STAGE_ARGENT_CHALLENGE_010:
                        DoStartArgentChampionEncounter();
                        if (instance)
                            instance->SetBossState(BOSS_ARGENT_CHALLENGE, IN_PROGRESS);
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_ARGENT_CHALLENGE_020);
                        break;
                    case EVENT_STAGE_ARGENT_CHALLENGE_030:
                        if (Creature* argentChampion = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(instance->GetData(DATA_ARGENT_CHAMPION)))))
                            argentChampion->AI()->Talk(instance->GetData(DATA_ARGENT_CHAMPION) == NPC_EADRIC ? SAY_STAGE_2_030 : SAY_STAGE_2_040);
                        UpdateTimer = 7*IN_MILLISECONDS;
                        if (instance->GetData(DATA_ARGENT_CHAMPION) == NPC_EADRIC)
                            instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_ARGENT_CHALLENGE_050);
                        else
                            instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_ARGENT_CHALLENGE_040);
                        break;
                    case EVENT_STAGE_ARGENT_CHALLENGE_040:
                        if (Creature* argentChampion = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(instance->GetData(DATA_ARGENT_CHAMPION)))))
                            argentChampion->AI()->Talk(SAY_STAGE_2_050);
                        UpdateTimer = 7*IN_MILLISECONDS;
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_ARGENT_CHALLENGE_050);
                        break;
                    case EVENT_STAGE_ARGENT_CHALLENGE_050:
                        Talk(SAY_STAGE_2_060);
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_NULL);
                        break;
                    case EVENT_STAGE_BLACK_KNIGHT_000:
                        Talk(SAY_STAGE_3_000);
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_BLACK_KNIGHT_010);
                        UpdateTimer = 4*IN_MILLISECONDS;
                        break;
                    case EVENT_STAGE_BLACK_KNIGHT_010:
                        me->SummonCreature(VEHICLE_BLACK_KNIGHT, BlackKnight[0]);
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_BLACK_KNIGHT_020);
                        UpdateTimer = 2*IN_MILLISECONDS;
                        break;
                    case EVENT_STAGE_BLACK_KNIGHT_030:
                        Talk(SAY_STAGE_3_020);
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_NULL);
                        break;
                    case EVENT_STAGE_BLACK_KNIGHT_040:
                        Talk(SAY_STAGE_3_030);
                        UpdateTimer = 5*IN_MILLISECONDS;
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_BLACK_KNIGHT_050);
                        break;
                    case EVENT_STAGE_BLACK_KNIGHT_050:
                        Talk(SAY_STAGE_3_040);
                        UpdateTimer = 5*IN_MILLISECONDS;
                        if (instance->GetData(DATA_TEAM_IN_INSTANCE) == HORDE)
                            instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_BLACK_KNIGHT_060);
                        else
                            instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_BLACK_KNIGHT_070);
                        break;

                }
            } else UpdateTimer -= diff;

            instance->SetData(DATA_EVENT_TIMER, UpdateTimer);
        }

        void AggroAllPlayers(Creature* temp)
        {
            temp->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            temp->SetImmuneToAll(false);
            temp->SetReactState(REACT_AGGRESSIVE);
            
            for (Player& player : me->GetMap()->GetPlayers())
            {
                Unit* base = &player;
                if (player.HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT))
                    base = player.GetVehicleBase();

                AddThreat(base, frand(0.0f, 10.0f), temp);
                if (base) // player has MOVEMENTFLAG_ONTRANSPORT but have no VehicleBase
                    temp->SetInCombatWith(base);
                player.SetInCombatWith(temp);
            }
        }

        void ChampionsInit()
        {
            ChampionBossIds[0] = BOSS_ID_NONE;
            ChampionBossIds[1] = BOSS_ID_NONE;
            ChampionBossIds[2] = BOSS_ID_NONE;
            
            // roll 3 out of 5 factions
            ChampionBossIds[0] = urand(BOSS_ID_FACTION_1, BOSS_ID_FACTION_5);

            do
            {
                 ChampionBossIds[1] = urand(BOSS_ID_FACTION_1, BOSS_ID_FACTION_5);
            } while (ChampionBossIds[1] == ChampionBossIds[0]);

            do
            {
                ChampionBossIds[2] = urand(BOSS_ID_FACTION_1, BOSS_ID_FACTION_5);
            } while (ChampionBossIds[2] == ChampionBossIds[0] || ChampionBossIds[2] == ChampionBossIds[1]);

            ChampionSummonStage = 0;
            ChampionActivationStage = 0;
        }

        void ChampionSummon()
        {
            if (ChampionSummonStage >= 3)
                return;

            uint8 bossId = ChampionBossIds[ChampionSummonStage];

            const ChampionWave& wave = instance->GetData(DATA_TEAM_IN_INSTANCE) == HORDE ? ChampionWavesAlliance[bossId - BOSS_ID_FACTION_1] : ChampionWavesHorde[bossId - BOSS_ID_FACTION_1];

            if (Creature* vehicle = me->SummonCreature(wave.bossVehicle, SpawnPosition))
            {
                if (Creature* boss = me->SummonCreature(wave.boss, SpawnPosition))
                {
                    boss->EnterVehicle(vehicle, 0);
                    boss->AI()->SetGuidData(vehicle->GetGUID());

                    for (uint8 i = 0; i < 3; ++i)
                    {
                        if (Creature* championVehicle = me->SummonCreature(wave.championVehicle, SpawnPosition, TEMPSUMMON_CORPSE_DESPAWN))
                        {
                            if (Creature* champion = me->SummonCreature(wave.champion, SpawnPosition, TEMPSUMMON_CORPSE_DESPAWN))
                            {
                                champion->EnterVehicle(championVehicle, 0);

                                champion->AI()->SetGuidData(championVehicle->GetGUID());

                                ChampionGUIDs[ChampionSummonStage][i][1] = championVehicle->GetGUID();
                                ChampionGUIDs[ChampionSummonStage][i][0] = champion->GetGUID();
                            
                                championVehicle->GetMotionMaster()->MoveFollow(vehicle, 3.5f, M_PI / 2.0f * (i + 1));
                            }
                        }
                    }

                    ChampionBossGUIDs[ChampionSummonStage][0] = boss->GetGUID();
                    ChampionBossGUIDs[ChampionSummonStage][1] = vehicle->GetGUID();
                    AnnouncerIntroduceYell = wave.announcerIntroduceYell;

                    if (Creature* fxTrigger = me->SummonCreature(wave.fxTriggerEntry, ToCCommonPos[0], TEMPSUMMON_TIMED_DESPAWN, IN_MILLISECONDS))
                        fxTrigger->AI()->Talk(0, boss);

                    if (instance)
                        instance->SetGuidData(DATA_GRAND_CHAMPION_1 + ChampionSummonStage, boss->GetGUID());
                    boss->AI()->DoAction(ACTION_GRAND_CHAMPIONS_MOVE_1 + ChampionSummonStage);
                }
            }    

            ++ChampionSummonStage;
        }

        void ChampionActivate()
        {
            if (ChampionActivationStage < 3)
            {
                for (uint8 i = 0; i < 3; ++i)
                    if (Creature* champion = ObjectAccessor::GetCreature(*me, ChampionGUIDs[ChampionActivationStage][i][0]))
                        AggroAllPlayers(champion);
            }
            else if (ChampionActivationStage == 3)
            {
                for (uint8 i = 0; i < 3; ++i)
                    if (Creature* bossVehicle = ObjectAccessor::GetCreature(*me, ChampionBossGUIDs[i][0]))
                        AggroAllPlayers(bossVehicle);
            }
            else if (ChampionActivationStage == 4)
            {
                for (uint8 i=0; i<3; i++)
                    if (Creature* boss = ObjectAccessor::GetCreature(*me, ChampionBossGUIDs[i][0]))
                    {
                        if (Creature* replacement = me->SummonCreature(boss->GetEntry(), *boss, TEMPSUMMON_CORPSE_DESPAWN))
                        {
                            boss->DespawnOrUnsummon();
                            replacement->SetInCombatWithZone();
                            replacement->SetHomePosition(replacement->GetPositionX(), replacement->GetPositionY(), replacement->GetPositionZ(), 4.7f);
                            if (instance)
                                instance->SetGuidData(DATA_GRAND_CHAMPION_1 + i, replacement->GetGUID());
                        }
                    }
            }
            ++ChampionActivationStage;
        }

        void DoStartArgentChampionEncounter()
        {
            if (me->SummonCreature(instance ? instance->GetData(DATA_ARGENT_CHAMPION) : 0, SpawnPosition))
            {
                for (uint8 i = 0; i < 3; ++i)
                {
                    if (Creature* trash = me->SummonCreature(NPC_ARGENT_LIGHWIELDER, SpawnPosition))
                        trash->AI()->SetData(i, 0);
                    if (Creature* trash = me->SummonCreature(NPC_ARGENT_MONK, SpawnPosition))
                        trash->AI()->SetData(i, 0);
                    if (Creature* trash = me->SummonCreature(NPC_PRIESTESS, SpawnPosition))
                        trash->AI()->SetData(i, 0);
                }
            }
        }

    private:
        InstanceScript* instance;
        uint32 UpdateTimer;

        //! Grand Champions
        uint8 ChampionBossIds[3];
        uint8 ChampionSummonStage;
        uint8 ChampionActivationStage;
        uint32 AnnouncerIntroduceYell;
        ObjectGuid ChampionBossGUIDs[3][2];
        ObjectGuid ChampionGUIDs[3][3][2];
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_tirion_toc5AI(creature);
    }
};

class npc_thrall_toc5 : public CreatureScript
{
public:
    npc_thrall_toc5() : CreatureScript("npc_thrall_toc5") { }

    struct npc_thrall_toc5AI : public ScriptedAI
    {
        npc_thrall_toc5AI(Creature* creature) : ScriptedAI(creature)
        {
            instance = me->GetInstanceScript();
        }

        void UpdateAI(uint32 diff)
        {
            if (!instance)
                return;

            if (instance->GetData(DATA_EVENT_NPC) != me->GetEntry())
                return;

            UpdateTimer = instance->GetData(DATA_EVENT_TIMER);

            if (UpdateTimer <= diff)
            {
                switch (instance->GetData(DATA_EVENT_STAGE))
                {
                    case EVENT_STAGE_CHAMPIONS_015:
                        Talk(SAY_STAGE_1_015);
                        UpdateTimer = 5*IN_MILLISECONDS;
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_CHAMPIONS_020);
                        break;
                    case EVENT_STAGE_BLACK_KNIGHT_060:
                        Talk(SAY_STAGE_3_050h);
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_NULL);
                        break;
                    default:
                        break;
                }
            } else UpdateTimer -= diff;

            instance->SetData(DATA_EVENT_TIMER, UpdateTimer);
        }

    private:
        InstanceScript* instance;
        uint32 UpdateTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_thrall_toc5AI(creature);
    }
};

class npc_garrosh_toc5 : public CreatureScript
{
public:
    npc_garrosh_toc5() : CreatureScript("npc_garrosh_toc5") { }

    struct npc_garrosh_toc5AI : public ScriptedAI
    {
        npc_garrosh_toc5AI(Creature* creature) : ScriptedAI(creature)
        {
            instance = me->GetInstanceScript();
        }

        void UpdateAI(uint32 diff)
        {
            if (!instance)
                return;

            if (instance->GetData(DATA_EVENT_NPC) != me->GetEntry())
                return;

            UpdateTimer = instance->GetData(DATA_EVENT_TIMER);

            if (UpdateTimer <= diff)
            {
                switch (instance->GetData(DATA_EVENT_STAGE))
                {
                    case EVENT_STAGE_CHAMPIONS_020:
                        Talk(SAY_STAGE_1_020);
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_CHAMPIONS_030);
                        UpdateTimer = 1*IN_MILLISECONDS;
                        break;
                    case EVENT_STAGE_BLACK_KNIGHT_035:
                        Talk(SAY_STAGE_3_025h);
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_NULL);
                        break;
                    default:
                        break;
                }
            } else UpdateTimer -= diff;

            instance->SetData(DATA_EVENT_TIMER, UpdateTimer);
        }

    private:
        InstanceScript* instance;
        uint32 UpdateTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_garrosh_toc5AI(creature);
    }
};

class npc_varian_toc5 : public CreatureScript
{
public:
    npc_varian_toc5() : CreatureScript("npc_varian_toc5") { }

    struct npc_varian_toc5AI : public ScriptedAI
    {
        npc_varian_toc5AI(Creature* creature) : ScriptedAI(creature)
        {
            instance = me->GetInstanceScript();
        }

        void UpdateAI(uint32 diff)
        {
            if (!instance)
                return;

            if (instance->GetData(DATA_EVENT_NPC) != me->GetEntry())
                return;

            UpdateTimer = instance->GetData(DATA_EVENT_TIMER);

            if (UpdateTimer <= diff)
            {
                switch (instance->GetData(DATA_EVENT_STAGE))
                {
                    case EVENT_STAGE_CHAMPIONS_030:
                        Talk(SAY_STAGE_1_030);
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_CHAMPIONS_040);
                        UpdateTimer = 5*IN_MILLISECONDS;
                        break;
                    case EVENT_STAGE_BLACK_KNIGHT_036:
                        Talk(SAY_STAGE_3_025a);
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_NULL);
                        break;
                    case EVENT_STAGE_BLACK_KNIGHT_070:
                        Talk(SAY_STAGE_3_050a);
                        instance->SetData(DATA_EVENT_STAGE, EVENT_STAGE_NULL);
                        break;
                    default:
                        break;
                }
            } else UpdateTimer -= diff;

            instance->SetData(DATA_EVENT_TIMER, UpdateTimer);
        }

    private:
        InstanceScript* instance;
        uint32 UpdateTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_varian_toc5AI(creature);
    }
};

class spell_use_argent_mount_toc5 : public SpellScriptLoader
{
    public:
        spell_use_argent_mount_toc5() : SpellScriptLoader("spell_use_argent_mount_toc5") { }

        class spell_use_argent_mount_toc5_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_use_argent_mount_toc5_SpellScript);

            SpellCastResult CheckRequirement()
            {
                if (Player* player = GetCaster()->ToPlayer())
                {
                    if (!player->HasItemOrGemWithIdEquipped(ITEM_ARGENT_LANCE, 1))
                    {
                        player->MonsterWhisper("Ich brauche eine Argentumlanze", player);
                        return SPELL_FAILED_EQUIPPED_ITEM;
                    }
                }
                return SPELL_CAST_OK;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_use_argent_mount_toc5_SpellScript::CheckRequirement);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_use_argent_mount_toc5_SpellScript();
        }
};

class spell_toc5_firework_controller : public SpellScriptLoader
{
    public:
        spell_toc5_firework_controller() : SpellScriptLoader("spell_toc5_firework_controller") { }

        class spell_toc5_firework_controller_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_toc5_firework_controller_SpellScript);

            void HandleScriptEffect()
            {
                uint32 firworkSpellId = RAND(SPELL_FIREWORK_RED, SPELL_FIREWORK_BLUE, SPELL_FIREWORK_GOLD);

                if (Unit* caster = GetCaster())
                    caster->CastSpell(caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ(), firworkSpellId, false);
            }

            void Register()
            {
                OnCast += SpellCastFn(spell_toc5_firework_controller_SpellScript::HandleScriptEffect);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_toc5_firework_controller_SpellScript();
        }
};

void AddSC_trial_of_the_champion()
{
    new npc_announcer_toc5();
    new npc_tirion_toc5();

    new npc_thrall_toc5();
    new npc_garrosh_toc5();
    new npc_varian_toc5();

    new spell_use_argent_mount_toc5();
    new spell_toc5_firework_controller();
}
