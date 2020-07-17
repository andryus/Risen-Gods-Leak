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
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"
#include "halls_of_stone.h"
#include "Player.h"

enum Texts
{
    SAY_KILL                            = 0,
    SAY_LOW_HEALTH                      = 1,
    SAY_DEATH                           = 2,
    SAY_PLAYER_DEATH                    = 3,
    SAY_ESCORT_START                    = 4,

    SAY_SPAWN_DWARF                     = 5,
    SAY_SPAWN_TROGG                     = 6,
    SAY_SPAWN_OOZE                      = 7,
    SAY_SPAWN_EARTHEN                   = 8,

    SAY_EVENT_INTRO_1                   = 9,
    SAY_EVENT_INTRO_2                   = 10,
    SAY_EVENT_A_1                       = 11,
    SAY_EVENT_A_3                       = 12,
    SAY_EVENT_B_1                       = 13,
    SAY_EVENT_B_3                       = 14,
    SAY_EVENT_C_1                       = 15,
    SAY_EVENT_C_3                       = 16,
    SAY_EVENT_D_1                       = 17,
    SAY_EVENT_D_3                       = 18,

    SAY_EVENT_END_01                    = 19,
    SAY_EVENT_END_02                    = 20,
    SAY_EVENT_END_04                    = 21,
    SAY_EVENT_END_06                    = 22,
    SAY_EVENT_END_08                    = 23,
    SAY_EVENT_END_10                    = 24,
    SAY_EVENT_END_12                    = 25,
    SAY_EVENT_END_14                    = 26,
    SAY_EVENT_END_16                    = 27,
    SAY_EVENT_END_18                    = 28,
    SAY_EVENT_END_20                    = 29,

    SAY_VICTORY_SJONNIR_1               = 30,
    SAY_VICTORY_SJONNIR_2               = 31,
    SAY_ENTRANCE_MEET                   = 32,

    SAY_EVENT_INTRO_3_ABED              = 0,
    SAY_EVENT_C_2_ABED                  = 1,
    SAY_EVENT_D_2_ABED                  = 2,
    SAY_EVENT_D_4_ABED                  = 3,
    SAY_EVENT_END_03_ABED               = 4,
    SAY_EVENT_END_05_ABED               = 5,
    SAY_EVENT_END_07_ABED               = 6,
    SAY_EVENT_END_21_ABED               = 7,

    SAY_EVENT_A_2_KADD                  = 0,
    SAY_EVENT_END_09_KADD               = 1,
    SAY_EVENT_END_11_KADD               = 2,
    SAY_EVENT_END_13_KADD               = 3,

    SAY_EVENT_B_2_MARN                  = 0,
    SAY_EVENT_END_15_MARN               = 1,
    SAY_EVENT_END_17_MARN               = 2,
    SAY_EVENT_END_19_MARN               = 3,

    TEXT_ID_START                       = 13100,
    TEXT_ID_PROGRESS                    = 13101
};

enum BrannCreatures
{
    CREATURE_TRIBUNAL_OF_THE_AGES       = 28234,
    CREATURE_BRANN_BRONZEBEARD          = 28070,
    CREATURE_DARK_MATTER_TARGET         = 28237,
    CREATURE_DARK_MATTER                = 28235,
    CREATURE_SEARING_GAZE_TARGET        = 28265,
    CREATURE_DARK_RUNE_PROTECTOR        = 27983,
    CREATURE_DARK_RUNE_STORMCALLER      = 27984,
    CREATURE_IRON_GOLEM_CUSTODIAN       = 27985
};

enum SjonnirCreatures
{
    CREATURE_FORGED_IRON_TROGG                             = 27979,
    CREATURE_MALFORMED_OOZE                                = 27981,
    CREATURE_FORGED_IRON_DWARF                             = 27982,
    CREATURE_IRON_SLUDGE                                   = 28165
};

enum BrannGameObjects
{
    GAMEOBJECT_KADDRAK                  = 191671,
    GAMEOBJECT_MARNAK                   = 191670,
    GAMEOBJECT_ABEDNEUM                 = 191669
};

enum Spells
{
    SPELL_STEALTH                       = 58506,
    //Kadrak
    SPELL_GLARE_OF_THE_TRIBUNAL         = 50988,
    H_SPELL_GLARE_OF_THE_TRIBUNAL       = 59870,
    //Marnak
    SPELL_DARK_MATTER                   = 51012,
    H_SPELL_DARK_MATTER                 = 59868,
    //Abedneum
    SPELL_SEARING_GAZE                  = 51136,
    H_SPELL_SEARING_GAZE                = 59867,

    SPELL_REWARD_ACHIEVEMENT            = 59046,
};

enum Quests
{
    QUEST_HALLS_OF_STONE                = 13207
};

#define GOSSIP_ITEM_START               "Brann, es waere uns eine Ehre!"
#define GOSSIP_ITEM_PROGRESS            "Bewegung Brann, genug Geschichtsunterricht!"
#define GOSSIP_ITEM_PROGRESS2           "Brann, lasst uns weiter gehen..."
#define GOSSIP_ITEM_SJONNIR             "Wir stehen an Eurer Seite, Brann! Oeffnet sie!"
#define DATA_BRANN_SPARKLIN_NEWS        1

static Position SpawnLocations[]=
{
    {946.992f, 397.016f, 208.374f, 0.0f},
    {960.748f, 382.944f, 208.374f, 0.0f},
};

class npc_tribuna_controller : public CreatureScript
{
public:
    npc_tribuna_controller() : CreatureScript("npc_tribuna_controller") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_tribuna_controllerAI(creature);
    }

    struct npc_tribuna_controllerAI : public ScriptedAI
    {
        npc_tribuna_controllerAI(Creature* c) : ScriptedAI(c)
        {
            pInstance = c->GetInstanceScript();
            SetCombatMovement(false);
        }

        InstanceScript* pInstance;

        uint32 uiKaddrakEncounterTimer;
        uint32 uiMarnakEncounterTimer;
        uint32 uiAbedneumEncounterTimer;

        bool bKaddrakActivated;
        bool bMarnakActivated;
        bool bAbedneumActivated;

        std::list<uint64> KaddrakGUIDList;
        uint32 checkTimer;

        void Reset()
        {
            uiKaddrakEncounterTimer = 1500;
            uiMarnakEncounterTimer = 10000;
            uiAbedneumEncounterTimer = 10000;

            bKaddrakActivated = false;
            bMarnakActivated = false;
            bAbedneumActivated = false;

            if (pInstance)
            {
                pInstance->HandleGameObject(ObjectGuid(pInstance->GetGuidData(DATA_GO_KADDRAK)), false);
                pInstance->HandleGameObject(ObjectGuid(pInstance->GetGuidData(DATA_GO_MARNAK)), false);
                pInstance->HandleGameObject(ObjectGuid(pInstance->GetGuidData(DATA_GO_ABEDNEUM)), false);
                pInstance->HandleGameObject(ObjectGuid(pInstance->GetGuidData(DATA_GO_SKY_FLOOR)), false);

                if(GameObject* KaddrakGO = me->FindNearestGameObject(GAMEOBJECT_KADDRAK,100.f))
                    KaddrakGO->SetGoState(GO_STATE_READY);
                if(GameObject* MarnakGO = me->FindNearestGameObject(GAMEOBJECT_MARNAK,100.f))
                    MarnakGO->SetGoState(GO_STATE_READY);
                if(GameObject* AbedneumGO = me->FindNearestGameObject(GAMEOBJECT_ABEDNEUM,100.f))
                    AbedneumGO->SetGoState(GO_STATE_READY);
                pInstance->HandleGameObject(ObjectGuid(pInstance->GetGuidData(DATA_GO_TRIBUNAL_CONSOLE)), false);
            }

            KaddrakGUIDList.clear();

            checkTimer = 2000;
        }

        void UpdateFacesList()
        {
            /*me->GetCreatureListWithEntryInGrid(lKaddrakGUIDList, CREATURE_KADDRAK, 50.0f);
            if (!lKaddrakGUIDList.empty())
            {
                uint32 uiPositionCounter = 0;
                for (std::list<Creature*>::const_iterator itr = lKaddrakGUIDList.begin(); itr != lKaddrakGUIDList.end(); ++itr)
                {
                    if ((*itr)->IsAlive())
                    {
                        if (uiPositionCounter == 0)
                        {
                            (*itr)->GetMap()->CreatureRelocation((*itr), 927.265f, 333.200f, 218.780f, (*itr)->GetOrientation());
                            (*itr)->SendMonsterMove(927.265f, 333.200f, 218.780f, 0, (*itr)->GetMovementFlags(), 1);
                        }
                        else
                        {
                            (*itr)->GetMap()->CreatureRelocation((*itr), 921.745f, 328.076f, 218.780f, (*itr)->GetOrientation());
                            (*itr)->SendMonsterMove(921.745f, 328.076f, 218.780f, 0, (*itr)->GetMovementFlags(), 1);
                        }
                    }
                    ++uiPositionCounter;
                }
            }*/
        }

        void UpdateAI(uint32 diff)
        {
            if (checkTimer <= diff)
            {
                GameObject* KaddrakGO = me->FindNearestGameObject(GAMEOBJECT_KADDRAK,100.f);
                GameObject* MarnakGO = me->FindNearestGameObject(GAMEOBJECT_MARNAK,100.f);
                if (bKaddrakActivated)
                    KaddrakGO->SetGoState(GO_STATE_ACTIVE);

                if (bMarnakActivated)
                     MarnakGO->SetGoState(GO_STATE_ACTIVE);

                checkTimer = 3000;
            }
            else
                checkTimer -= diff;

//            GameObject* AbedneumGO = me->FindNearestGameObject(GAMEOBJECT_ABEDNEUM,100.f);

            if (bKaddrakActivated)
            {
                if (uiKaddrakEncounterTimer <= diff)
                {
                    if (Creature* pKaddrak = me->FindNearestCreature(CREATURE_KADDRAK,100.f,true))
                    {
                        if (Creature* pKaddrak1 = me->FindNearestCreature(CREATURE_KADDRAK1,100.f,true))
                        {
                            if (pKaddrak->IsAlive() && pKaddrak1->IsAlive())
                            {
                                Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true);
                                pKaddrak->CastSpell(target, DUNGEON_MODE(SPELL_GLARE_OF_THE_TRIBUNAL, H_SPELL_GLARE_OF_THE_TRIBUNAL), true);
                                pKaddrak1->CastSpell(target, DUNGEON_MODE(SPELL_GLARE_OF_THE_TRIBUNAL, H_SPELL_GLARE_OF_THE_TRIBUNAL), true);
                            }
                        }
                    }
                    uiKaddrakEncounterTimer = 3000;
                } else uiKaddrakEncounterTimer -= diff;
            }
            if (bMarnakActivated)
            {
                if (uiMarnakEncounterTimer <= diff)
                {
                    // summon matter and matter target. move matter to matter target. see extra npc_script for matter
                    if (Creature* pMarnak = me->FindNearestCreature(CREATURE_MARNAK,100.f,true))
                    {
                        if (Unit* target = pMarnak->FindNearestPlayer(100.f))
                        {
                            if (Creature* summon = me->SummonCreature(CREATURE_DARK_MATTER_TARGET, target->GetPositionX(),target->GetPositionY(),target->GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN))
                            {
                                summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                summon->SetDisplayId(11686);
                                if(Creature* summon2 = me->SummonCreature(CREATURE_DARK_MATTER, 902.25f, 353.444f, 215.95f, 0.0f, TEMPSUMMON_MANUAL_DESPAWN))
                                {
                                    summon2->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                    summon2->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                    summon2->SetDisplayId(11686);
                                }
                            }
                        }
                    }
                    uiMarnakEncounterTimer = 25000 + rand32()%2000;
                } else uiMarnakEncounterTimer -= diff;
            }
            if (bAbedneumActivated)
            {
                if (uiAbedneumEncounterTimer <= diff)
                {
                    if (Creature* pAbedneum = me->FindNearestCreature(CREATURE_ABEDNEUM,100.f,true))
                    {
                        if (Unit* target = pAbedneum->FindNearestPlayer(100.f))
                        {
                            if (Creature* summon = me->SummonCreature(CREATURE_SEARING_GAZE_TARGET, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 10000))
                            {
                                summon->SetDisplayId(11686);
                                summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                summon->CastSpell(summon, DUNGEON_MODE(SPELL_SEARING_GAZE, H_SPELL_SEARING_GAZE), true);
                                summon->AddAura(DUNGEON_MODE(SPELL_SEARING_GAZE, H_SPELL_SEARING_GAZE),summon);
                            }
                        }
                    }
                    uiAbedneumEncounterTimer = 16000 + rand32()%2000;
                } else uiAbedneumEncounterTimer -= diff;
            }
        }
    };

};

class npc_brann_hos : public CreatureScript
{
public:
    npc_brann_hos() : CreatureScript("npc_brann_hos") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
    {
        player->PlayerTalkClass->ClearMenus();

        npc_brann_hosAI* EscortAI = CAST_AI(npc_brann_hos::npc_brann_hosAI, creature->AI());
        if (!EscortAI)
            return false;

        switch (uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF:
            {
                EscortAI->Start(true, true, player->GetGUID());
                Position pos; 
                creature->GetPosition(&pos);
                creature->SetHomePosition(pos);
                player->CLOSE_GOSSIP_MENU();
                break;
            }
            case GOSSIP_ACTION_INFO_DEF+1:
                EscortAI->Start(true, true, player->GetGUID());
                EscortAI->SetNextWaypoint(14, true,false);
                EscortAI->SetEscortPaused(false);
                creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                break;
            case GOSSIP_ACTION_INFO_DEF+2:
                {
                EscortAI->Start(true, true, player->GetGUID());
                EscortAI->SetNextWaypoint(19, true);
                EscortAI->SetEscortPaused(false);
                EscortAI->binterrupted = true;
                creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                EscortAI->uiStep = 51;

                float x,y,z;
                EscortAI->GetWaypointPosition(30, x, y, z);
                Position possec = {x,y,z};
                creature->SetHomePosition(possec);
                creature->UpdatePosition(possec, true);

                break;
                }
            case GOSSIP_ACTION_INFO_DEF+3:
                creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                creature->RemoveAura(SPELL_STEALTH);
                creature->SetOrientation(6.175f);
                EscortAI->Start(true, true, player->GetGUID());
                EscortAI->SetNextWaypoint(30,true,false);
                EscortAI->SetEscortPaused(false);
                EscortAI->uiStep = 52;
                break;
        }
        EscortAI->SetDespawnAtFar(false);
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());
        npc_brann_hosAI* EscortAI = CAST_AI(npc_brann_hos::npc_brann_hosAI, creature->AI());

        switch(EscortAI->GossipStep)
        {
            case 0:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_START, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
                break;
            case 1:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_PROGRESS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
                player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
                break;
            case 2:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_PROGRESS2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
                break;
            case 3:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_SJONNIR, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
                break;
        }
        return true;
    }


    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_brann_hosAI(creature);
    }

    struct npc_brann_hosAI : public npc_escortAI
    {
        npc_brann_hosAI(Creature* c) : npc_escortAI(c)
        {
            pInstance = c->GetInstanceScript();
        }

        uint32 uiStep;
        uint32 GossipStep;
        uint32 uiPhaseTimer;

        ObjectGuid uiControllerGUID;
        std::list<ObjectGuid> lDwarfGUIDList;

        InstanceScript* pInstance;

        bool bIsBattle;
        bool bIsLowHP;
        bool brannSparklinNews;
        bool AllowDamage;

        bool binterrupted;

        void Reset()
        {
            if (pInstance && pInstance->GetData(DATA_BRANN_EVENT) == NOT_STARTED)
            {
                bIsLowHP = false;
                bIsBattle = false;
                uiStep = 0;
                GossipStep = 0;
                uiPhaseTimer = 0;
                uiControllerGUID.Clear();
                brannSparklinNews = true;
                AllowDamage = false;
                binterrupted = false; 

                DespawnDwarf();
            }
            if (pInstance && pInstance->GetData(DATA_BRANN_EVENT) == DONE)
            {
                GossipStep = 3;
                uiStep = 54;
                bIsLowHP = false;
                bIsBattle = false;
                uiPhaseTimer = 0;
                me->SetStandState(UNIT_STAND_STATE_STAND);
            }
            me->setActive(true);
            me->GetMotionMaster()->MoveTargetedHome();
        }

        void DespawnDwarf()
        {
            if (lDwarfGUIDList.empty())
                return;
            for (std::list<ObjectGuid>::const_iterator itr = lDwarfGUIDList.begin(); itr != lDwarfGUIDList.end(); ++itr)
            {
                Creature* pTemp = ObjectAccessor::GetCreature(*me, pInstance ? (*itr) : ObjectGuid::Empty);
                if (pTemp && pTemp->IsAlive())
                    pTemp->DespawnOrUnsummon();
            }
            lDwarfGUIDList.clear();
        }

        void WaypointReached(uint32 uiPointId)
        {
            switch(uiPointId)
            {
                case 7:
                    if (Creature* creature = me->FindNearestCreature(CREATURE_TRIBUNAL_OF_THE_AGES, 100.0f))
                    {
                        if (!creature->IsAlive())
                            creature->Respawn();
                        CAST_AI(npc_tribuna_controller::npc_tribuna_controllerAI, creature->AI())->UpdateFacesList();
                        uiControllerGUID = creature->GetGUID();
                    }
                    break;
                case 13:
                    Talk(SAY_EVENT_INTRO_1);
                    SetEscortPaused(true);
                    GossipStep=1;
                    uiStep=2;
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    break;
                case 17:
                    Talk(SAY_EVENT_INTRO_2);
                    if (pInstance)
                        pInstance->HandleGameObject(ObjectGuid(pInstance->GetGuidData(DATA_GO_TRIBUNAL_CONSOLE)), true);
                    if (Creature* pTemp = ObjectAccessor::GetCreature(*me, uiControllerGUID))
                        pTemp->SetInCombatWithZone();
                    me->SetStandState(UNIT_STAND_STATE_KNEEL);
                    SetEscortPaused(true);
                    JumpToNextStep(8500);
                    break;
                case 18:
                    GossipStep=2;
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    me->setActive(true);
                    SetEscortPaused(true);
                    break;
                case 20:
                    Talk(SAY_ENTRANCE_MEET);
                    DoCastSelf(SPELL_STEALTH,true);
                    SetDespawnAtEnd(false);
                    break;
                case 21:
                    SetDespawnAtFar(false);
                    SetEscortPaused(false);
                    me->SetVisible(false);
                    SetRun(true);
                    me->RemoveAura(SPELL_STEALTH);
                    me->SetSpeedRate(MOVE_RUN, 7.0f);
                    break;
                case 29:
                    GossipStep = 3;
                    break;
                case 30:
                    me->SetVisible(true);
                    DoCastSelf(SPELL_STEALTH,true);
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    me->setFaction(35);
                    me->SetOrientation(3.059f);
                    me->SetSpeedRate(MOVE_RUN, 1.0f);
                    SetRun(false);
                    SetEscortPaused(true);
                    break;
                case 31:
                    me->HandleEmoteCommand(69);
                    SetRun(true);
                    break;
                case 32:
                    SetEscortPaused(true);
                    Talk(SAY_SPAWN_DWARF);
                    uiStep=54;
                    break;
                case 33:
                    SetEscortPaused(true);
                    me->SetStandState(UNIT_STAND_STATE_KNEEL);
                    if(GameObject* SJONNIR_CONSOLE = me->FindNearestGameObject(GO_SJONNIR_CONSOLE,10.f))
                        SJONNIR_CONSOLE->SetGoState(GO_STATE_ACTIVE);
                    uiStep=55;
                    break;
                case 34:
                    SetEscortPaused(true);
                    SetDespawnAtEnd(false);
                    SetDespawnAtFar(false);
                    Talk(SAY_VICTORY_SJONNIR_1);
                    uiStep=60;
                    break;

            }
         }

         void SpawnDwarf(uint32 uiType)
         {
           switch(uiType)
           {
               case 1:
               {
                   uint32 uiSpawnNumber = DUNGEON_MODE(2, 3);
                   for (uint8 i = 0; i < uiSpawnNumber; ++i)
                   {
                        Creature* summon = me->SummonCreature(CREATURE_DARK_RUNE_PROTECTOR, SpawnLocations[0], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000);
                        summon->SetInCombatWithZone();
                   }
                   break;
               }
               case 2:
                   for (uint8 i = 0; i < 2; ++i)
                   {
                       Creature* summon = me->SummonCreature(CREATURE_DARK_RUNE_STORMCALLER, SpawnLocations[1], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000);
                       summon->SetInCombatWithZone();
                   }
                   break;
               case 3:
                   Creature* summon = me->SummonCreature(CREATURE_IRON_GOLEM_CUSTODIAN, SpawnLocations[1], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000);
                   summon->SetInCombatWithZone();
                   break;
           }
         }

        void JustSummoned(Creature* summoned)
        {
            lDwarfGUIDList.push_back(summoned->GetGUID());
            AddThreat(me,1.0f, summoned);
            summoned->AI()->AttackStart(me);
        }

        void JumpToNextStep(uint32 uiTimer)
        {
          uiPhaseTimer = uiTimer;
          ++uiStep;
        }

        void StartWP()
        {
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            SetEscortPaused(false);
            uiStep = 1;
            Start();
        }

        void DamageTaken(Unit* /*done_by*/, uint32 & /*damage*/)
        {
            if (!AllowDamage)
                return;

            if (brannSparklinNews)
                brannSparklinNews = false;
        }

        uint32 GetData(uint32 type) const
        {
            if (type == DATA_BRANN_SPARKLIN_NEWS)
                return brannSparklinNews ? 1 : 0;

            return 0;
        }
        void JustDied(Unit* /*who*/)
        {
            if (Creature* pTemp = ObjectAccessor::GetCreature(*me, uiControllerGUID))
            {
                pTemp->CombatStop(true);
                pTemp->AI()->Reset();
                CAST_AI(npc_tribuna_controller::npc_tribuna_controllerAI, pTemp->AI())->bMarnakActivated = false;
                CAST_AI(npc_tribuna_controller::npc_tribuna_controllerAI, pTemp->AI())->bAbedneumActivated = false;
                CAST_AI(npc_tribuna_controller::npc_tribuna_controllerAI, pTemp->AI())->bKaddrakActivated = false;

            }
            if(pInstance && pInstance->GetData(DATA_BRANN_EVENT)==IN_PROGRESS)
                pInstance->SetData(DATA_BRANN_EVENT, NOT_STARTED);
            if(pInstance && pInstance->GetData(DATA_BRANN_EVENT)==DONE)
                pInstance->SetData(DATA_SJONNIR_EVENT, NOT_STARTED);
            me->Respawn(true);
        }

        void UpdateEscortAI(const uint32 uiDiff)
        {
            if (uiPhaseTimer <= uiDiff)
            {
                switch(uiStep)
                {
                    case 1:
                        if (pInstance)
                        {
                            if (pInstance->GetData(DATA_BRANN_EVENT) != NOT_STARTED)
                                return;
                            pInstance->SetData(DATA_BRANN_EVENT, IN_PROGRESS);
                        }
                        bIsBattle = false;
                        Talk(SAY_ESCORT_START);
                        SetRun(true);
                        JumpToNextStep(0);
                        break;
                    case 3:
                        uiStep++;
                        JumpToNextStep(0);
                        break;
                    case 5:
                        if (pInstance)
                            if (Creature* pTemp = (ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_ABEDNEUM)))))
                                pTemp->AI()->Talk(SAY_EVENT_INTRO_3_ABED);
                            JumpToNextStep(8500);
                        break;
                    case 6:
                        Talk(SAY_EVENT_A_1);
                        JumpToNextStep(6500);
                        break;
                    case 7:
                        if (pInstance)
                            if (Creature* pTemp = ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_KADDRAK))))
                                pTemp->AI()->Talk(SAY_EVENT_A_2_KADD);
                            JumpToNextStep(10500);
                        break;
                    case 8:
                        Talk(SAY_EVENT_A_3);
                        if (pInstance)
                            pInstance->HandleGameObject(ObjectGuid(pInstance->GetGuidData(DATA_GO_KADDRAK)), true);
                        if (Creature* pTemp = ObjectAccessor::GetCreature(*me, uiControllerGUID))
                            CAST_AI(npc_tribuna_controller::npc_tribuna_controllerAI, pTemp->AI())->bKaddrakActivated = true;
                        JumpToNextStep(5000);
                        break;
                    case 9:
                        me->SetReactState(REACT_PASSIVE);
                        AllowDamage = true;
                        SpawnDwarf(1);
                        JumpToNextStep(20000);
                        break;
                    case 10:
                        Talk(SAY_EVENT_B_1);
                        JumpToNextStep(5000);
                        break;
                    case 11:
                        if (pInstance)
                            if (Creature* pTemp = ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_MARNAK))))
                                pTemp->AI()->Talk(SAY_EVENT_B_2_MARN);
                        SpawnDwarf(1);
                        JumpToNextStep(10000);
                        break;
                    case 12:
                        Talk(SAY_EVENT_B_3);
                        if (pInstance)
                            pInstance->HandleGameObject(ObjectGuid(pInstance->GetGuidData(DATA_GO_MARNAK)), true);
                        if (Creature* pTemp = ObjectAccessor::GetCreature(*me, uiControllerGUID))
                            CAST_AI(npc_tribuna_controller::npc_tribuna_controllerAI, pTemp->AI())->bMarnakActivated = true;
                        JumpToNextStep(10000);
                        break;
                    case 13:
                        SpawnDwarf(1);
                        JumpToNextStep(10000);
                        break;
                    case 14:
                        SpawnDwarf(2);
                        JumpToNextStep(20000);
                        break;
                    case 15:
                        Talk(SAY_EVENT_C_1);
                        SpawnDwarf(1);
                        JumpToNextStep(10000);
                        break;
                    case 16:
                        SpawnDwarf(1);
                        SpawnDwarf(2);
                        JumpToNextStep(20000);
                        break;
                    case 17:
                        if (pInstance)
                            if (Creature* pTemp = ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_ABEDNEUM))))
                                pTemp->AI()->Talk(SAY_EVENT_C_2_ABED);
                        SpawnDwarf(1);
                        JumpToNextStep(8500);
                        break;
                    case 18:
                        Talk(SAY_EVENT_C_3);
                        if (pInstance)
                            pInstance->HandleGameObject(ObjectGuid(pInstance->GetGuidData(DATA_GO_ABEDNEUM)), true);
                        if (Creature* pTemp = ObjectAccessor::GetCreature(*me, uiControllerGUID))
                            CAST_AI(npc_tribuna_controller::npc_tribuna_controllerAI, pTemp->AI())->bAbedneumActivated = true;
                        JumpToNextStep(5000);
                        break;
                    case 19:
                        SpawnDwarf(2);
                        JumpToNextStep(10000);
                        break;
                    case 20:
                        SpawnDwarf(1);
                        JumpToNextStep(15000);
                        break;
                    case 21:
                        Talk(SAY_EVENT_D_1);
                        SpawnDwarf(3);
                        JumpToNextStep(6000);
                        break;
                    case 22:
                        if (pInstance)
                            if (Creature* pTemp = ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_ABEDNEUM))))
                                pTemp->AI()->Talk(SAY_EVENT_D_2_ABED);
                        SpawnDwarf(1);
                        JumpToNextStep(5000);
                        break;
                    case 23:
                        SpawnDwarf(2);
                        JumpToNextStep(15000);
                        break;
                    case 24:
                        Talk(SAY_EVENT_D_3);
                        SpawnDwarf(3);
                        JumpToNextStep(5000);
                        break;
                    case 25:
                        SpawnDwarf(1);
                        JumpToNextStep(5000);
                        break;
                    case 26:
                        SpawnDwarf(2);
                        JumpToNextStep(10000);
                        break;
                    case 27:
                        if (pInstance)
                            if (Creature* pTemp = ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_ABEDNEUM))))
                                pTemp->AI()->Talk(SAY_EVENT_D_4_ABED);
                        SpawnDwarf(1);
                        JumpToNextStep(6000);
                        break;
                    case 28:
                        me->SetReactState(REACT_DEFENSIVE);
                        Talk(SAY_EVENT_END_01);
                        me->SetStandState(UNIT_STAND_STATE_STAND);
                        if (pInstance)
                            pInstance->HandleGameObject(ObjectGuid(pInstance->GetGuidData(DATA_GO_SKY_FLOOR)), true);
                        if (Creature* pTemp = ObjectAccessor::GetCreature(*me, uiControllerGUID))
                            pTemp->DealDamage(pTemp, pTemp->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                        bIsBattle = true;
                        SetEscortPaused(false);
                        SetRun(false);
                        JumpToNextStep(6500);
                        break;
                    case 29:
                        Talk(SAY_EVENT_END_02);
                        if (pInstance)
                            pInstance->SetData(DATA_BRANN_EVENT, DONE);
                        if (Creature* sjonnir = ObjectAccessor::GetCreature((*me), ObjectGuid(pInstance->GetGuidData(DATA_SJONNIR))))
                            sjonnir->setFaction(FACTION_HOSTILE);
                        DoCastSelf(SPELL_REWARD_ACHIEVEMENT, true);
                        JumpToNextStep(5500);
                        break;
                    case 30:
                        if (pInstance  && !binterrupted)
                            if (Creature* pTemp = ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_ABEDNEUM))))
                                pTemp->AI()->Talk(SAY_EVENT_END_03_ABED);
                        JumpToNextStep(5500);
                        break;
                    case 31:
                        Talk(SAY_EVENT_END_04);
                        JumpToNextStep(11500);
                        break;
                    case 32:
                        if (pInstance && !binterrupted)
                            if (Creature* pTemp = ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_ABEDNEUM))))
                                pTemp->AI()->Talk(SAY_EVENT_END_05_ABED);
                            JumpToNextStep(9500);
                        break;
                    case 33:
                        Talk(SAY_EVENT_END_06);
                        JumpToNextStep(4500);
                        break;
                    case 34:
                        if (pInstance && !binterrupted)
                            if (Creature* pTemp = ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_ABEDNEUM))))
                                pTemp->AI()->Talk(SAY_EVENT_END_07_ABED);
                            JumpToNextStep(21500);
                        break;
                    case 35:
                        Talk(SAY_EVENT_END_08);
                        JumpToNextStep(7500);
                        break;
                    case 36:
                        if (pInstance && !binterrupted)
                            if (Creature* pTemp = ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_KADDRAK))))
                                pTemp->AI()->Talk(SAY_EVENT_END_09_KADD);
                        JumpToNextStep(17000);
                        break;
                    case 37:
                        Talk(SAY_EVENT_END_10);
                        JumpToNextStep(5500);
                        break;
                    case 38:
                        if (pInstance && !binterrupted)
                            if (Creature* pTemp = ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_KADDRAK))))
                                pTemp->AI()->Talk(SAY_EVENT_END_11_KADD);
                            JumpToNextStep(19500);
                        break;
                    case 39:
                        Talk(SAY_EVENT_END_12);
                        JumpToNextStep(2500);
                        break;
                    case 40:
                        if (pInstance && !binterrupted)
                            if (Creature* pTemp = ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_KADDRAK))))
                                pTemp->AI()->Talk(SAY_EVENT_END_13_KADD);
                        JumpToNextStep(19500);
                        break;
                    case 41:
                        Talk(SAY_EVENT_END_14);
                        JumpToNextStep(10500);
                        break;
                    case 42:
                        if (pInstance && !binterrupted)
                            if (Creature* pTemp = ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_MARNAK))))
                                pTemp->AI()->Talk(SAY_EVENT_END_15_MARN);
                        JumpToNextStep(6500);
                        break;
                    case 43:
                        Talk(SAY_EVENT_END_16);
                        JumpToNextStep(6500);
                        break;
                    case 44:
                        if (pInstance && !binterrupted)
                            if (Creature* pTemp = ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_MARNAK))))
                                pTemp->AI()->Talk(SAY_EVENT_END_17_MARN);
                            JumpToNextStep(25500);
                        break;
                    case 45:
                        Talk(SAY_EVENT_END_18);
                        JumpToNextStep(23500);
                        break;
                    case 46:
                        if (pInstance && !binterrupted)
                            if (Creature* pTemp = ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_MARNAK))))
                                pTemp->AI()->Talk(SAY_EVENT_END_19_MARN);
                            JumpToNextStep(3500);
                        break;
                    case 47:
                        Talk(SAY_EVENT_END_20);
                        JumpToNextStep(8500);
                        break;
                    case 48:
                        if (pInstance && !binterrupted)
                            if (Creature* pTemp = ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_ABEDNEUM))))
                                pTemp->AI()->Talk(SAY_EVENT_END_21_ABED);
                        JumpToNextStep(5500);
                        break;
                    case 49:
                    {
                        if (pInstance)
                        {
                            pInstance->HandleGameObject(ObjectGuid(pInstance->GetGuidData(DATA_GO_ABEDNEUM)), false);
                            pInstance->HandleGameObject(ObjectGuid(pInstance->GetGuidData(DATA_GO_SKY_FLOOR)), false);
                            if(GameObject* KaddrakGO = me->FindNearestGameObject(GAMEOBJECT_KADDRAK,100.f))
                                KaddrakGO->SetGoState(GO_STATE_READY);
                            if(GameObject* MarnakGO = me->FindNearestGameObject(GAMEOBJECT_MARNAK,100.f))
                                MarnakGO->SetGoState(GO_STATE_READY);
                        }
                        Player* player = GetPlayerForEscort();
                        if (player)
                            player->GroupEventHappens(QUEST_HALLS_OF_STONE, me);
                        JumpToNextStep(180000);
                        break;
                    }
                    case 50:
                        JumpToNextStep(0);
                        break;
                    case 51:
                        SetDespawnAtEnd(false);
                        break;
                    case 52:
                        JumpToNextStep(4000);
                        break;
                    case 53:
                        if(InstanceScript* pInstance = me->GetInstanceScript())
                            pInstance->HandleGameObject(ObjectGuid(pInstance->GetGuidData(DATA_SJONNIR_DOOR)), true);
                        break;
                    case 54:
                        if (Creature* pTemp = (ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_SJONNIR)))))
                        {
                            if (pTemp->IsInCombat())
                            {
                                Start(true, true, me->GetGUID());
                                SetNextWaypoint(32,true);
                                me->SetSpeedRate(MOVE_RUN, 1.0f);
                                SetRun(true);
                                SetEscortPaused(false);
                            }
                        }
                        break;
                    case 55:
                        JumpToNextStep(10000);
                        break;
                    case 56:
                        if (me->FindNearestCreature(CREATURE_FORGED_IRON_TROGG,100.f,true))
                        {
                            Talk(SAY_SPAWN_TROGG);
                            JumpToNextStep(4000);
                        }
                        break;
                    case 57:
                        if (me->FindNearestCreature(CREATURE_MALFORMED_OOZE,100.f,true))
                        {
                            Talk(SAY_SPAWN_OOZE);
                            JumpToNextStep(6000);
                        }
                        break;
                    case 58:
                        if (Creature* pTemp = me->FindNearestCreature(CREATURE_FORGED_IRON_TROGG,100.f,true))
                        {
                            if (pTemp->getFaction() == 113)
                            {
                                Talk(SAY_SPAWN_EARTHEN); // must be triggered by sjonnir
                                JumpToNextStep(3000);
                            }
                        }
                        break;
                    case 59:
                        if (Creature* pTemp = (ObjectAccessor::GetCreature(*me, ObjectGuid(pInstance->GetGuidData(DATA_SJONNIR)))))
                        {
                            if (!pTemp->IsAlive())
                            {
                                SetRun(false);
                                SetEscortPaused(false);
                                JumpToNextStep(1000);
                            }
                        }
                        break;
                    case 60:
                        JumpToNextStep(9000);
                        break;
                    case 61:
                        Talk(SAY_VICTORY_SJONNIR_2);
                        JumpToNextStep(180000);
                        break;
                }
            } else uiPhaseTimer -= uiDiff;

            if (!bIsLowHP && HealthBelowPct(30))
            {
                Talk(SAY_LOW_HEALTH);
                bIsLowHP = true;
            }
            else if (bIsLowHP && !HealthBelowPct(30))
                bIsLowHP = false;

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };



};

class npc_hos_brann_dark_matter : public CreatureScript
{
    public:
        npc_hos_brann_dark_matter() : CreatureScript("npc_hos_brann_dark_matter") { }

        struct npc_hos_brann_dark_matterAI : public ScriptedAI
        {
            npc_hos_brann_dark_matterAI(Creature* creature) : ScriptedAI(creature)
            {}

            uint32 uiMoveTimer;
            uint32 uiDespTimer;
            void Reset()
            {
                uiMoveTimer = 5000;
            }

            void JustDied(Unit* /*who*/)
            {}

            void EnterCombat(Unit* /*who*/)
            {}

            void UpdateAI(uint32 diff)
            {
                me->AddAura(DUNGEON_MODE(SPELL_DARK_MATTER, H_SPELL_DARK_MATTER),me);
                

                if (uiMoveTimer <= diff)
                {
                    if (Creature* target = me->FindNearestCreature(CREATURE_DARK_MATTER_TARGET,150.f,true))
                    {
                        if (me->GetExactDist2d(target) < 0.01f)
                        {
                            if (me->HasAura(DUNGEON_MODE(SPELL_DARK_MATTER, H_SPELL_DARK_MATTER)))
                                me->RemoveAura(DUNGEON_MODE(SPELL_DARK_MATTER, H_SPELL_DARK_MATTER));

                            if (!target->HasAura(DUNGEON_MODE(SPELL_DARK_MATTER, H_SPELL_DARK_MATTER)))
                                target->AddAura(DUNGEON_MODE(SPELL_DARK_MATTER, H_SPELL_DARK_MATTER),me);

                            if (me->IsAlive() && target->IsAlive())
                            {
                                if(Player* PlayerTarget = me->FindNearestPlayer(1.f))
                                    me->AddAura(DUNGEON_MODE(SPELL_DARK_MATTER, H_SPELL_DARK_MATTER),PlayerTarget);
                            }

                            me->DespawnOrUnsummon(1);
                            target->DespawnOrUnsummon(1);
                        }
                        else
                        {
                            if (me->IsAlive())
                                me->GetMotionMaster()->MovePoint(1,target->GetPositionX(),target->GetPositionY(),target->GetPositionZ());
                        }
                    }

                    uiMoveTimer = 500;
                }
                else
                    uiMoveTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_hos_brann_dark_matterAI(creature);
        }
};

class achievement_brann_spankin_new : public AchievementCriteriaScript
{
    public:
        achievement_brann_spankin_new() : AchievementCriteriaScript("achievement_brann_spankin_new")
        {
        }

        bool OnCheck(Player* /*player*/, Unit* target)
        {
            if (!target)
                return false;

            if (Creature* Brann = target->ToCreature())
                if (Brann->AI()->GetData(DATA_BRANN_SPARKLIN_NEWS))
                    return true;

            return false;
        }
};

void AddSC_halls_of_stone()
{
    new npc_brann_hos();
    new npc_tribuna_controller();
    new npc_hos_brann_dark_matter();
    new achievement_brann_spankin_new();
}
