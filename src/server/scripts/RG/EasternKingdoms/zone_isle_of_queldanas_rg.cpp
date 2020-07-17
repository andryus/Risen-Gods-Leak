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
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "PassiveAI.h"
#include "Chat.h"
#include "Group.h"

enum ThalorienDawnseekerSays
{
    SAY_THALORIEN_1          = 0,
    SAY_THALORIEN_2          = 1,
    SAY_THALORIEN_3          = 2,
    SAY_THALORIEN_4          = 3,
    SAY_THALORIEN_5          = 4,
    SAY_THALORIEN_6          = 5,
    SAY_THALORIEN_7          = 6,
    SAY_THALORIEN_8          = 7,
    SAY_THALORIEN_9          = 8,
    SAY_THALORIEN_10         = 9,

    SAY_MORLEN_GOLDGRIP_1    = 0,
    SAY_MORLEN_GOLDGRIP_2    = 1,
    SAY_MORLEN_GOLDGRIP_3    = 2,
    SAY_MORLEN_GOLDGRIP_4    = 3,
    SAY_MORLEN_GOLDGRIP_5    = 4
};

enum ThalorienDawnseekerCreatures
{
    NPC_THALORIEN_DAWNSEEKER = 37205,
    NPC_MORLEN_GOLDGRIP      = 37542,
    NPC_QUEST_CREDIT         = 37601,
    NPC_SUNWELL_DEFENDER     = 37211,
    NPC_SCOURGE_ZOMBIE       = 37538,
    NPC_GHOUL_INVADER        = 37539,
    NPC_CRYPT_RAIDER         = 37541
};

enum ThalorienDawnseekerActions
{
    ACTION_START_QUEST       = 1
};

enum ThalorienDawnseekerEvents
{
    EVENT_STEP_1             = 1,
    EVENT_STEP_2             = 2,
    EVENT_STEP_3             = 3,
    EVENT_STEP_4             = 4,
    EVENT_STEP_5             = 5,
    EVENT_STEP_6             = 6,
    EVENT_STEP_7             = 7,
    EVENT_STEP_8             = 8,
    EVENT_STEP_9             = 9,
    EVENT_STEP_10            = 10,
    EVENT_STEP_11            = 11,
    EVENT_STEP_12            = 12,
    EVENT_STEP_13            = 13,
    EVENT_STEP_14            = 14,
    EVENT_STEP_15            = 15,
    EVENT_STEP_16            = 16,
    EVENT_STEP_17            = 17,
    EVENT_STEP_18            = 18,
};

enum ThalorienDawnseekerMisc
{
    DISPLAY_MOUNT            = 25678,
    QUEST_THALORIEN_A        = 24535,
    QUEST_THALORIEN_H        = 24563,
    SPELL_BLOOD_PRESENCE     = 50689
};

Position const ZombieLoc[5] =
{
    { 11768.7f, -7065.83f, 24.0971f, 0.125877f   },
    { 11769.5f, -7063.83f, 24.1399f, 6.06035f    },
    { 11769.8f, -7061.41f, 24.2536f, 6.06035f    },
    { 11769.4f, -7057.86f, 24.4624f, 0.00335493f },
    { 11769.4f, -7054.56f, 24.6869f, 0.00335493f }
};

Position const GuardLoc[4] =
{
    { 11807.0f, -7070.37f, 25.372f, 3.218f  },
    { 11805.7f, -7073.64f, 25.5694f, 3.218f },
    { 11805.6f, -7077.0f, 25.9643f, 3.218f  },
    { 11806.0f, -7079.71f, 26.2067f, 3.218f }
};

class npc_thalorien_dawnseeker : public CreatureScript
{
public:
    npc_thalorien_dawnseeker() : CreatureScript("npc_thalorien_dawnseeker") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        player->PrepareGossipMenu(creature, 0);

        if ((player->GetQuestStatus(QUEST_THALORIEN_A) == QUEST_STATUS_INCOMPLETE) || (player->GetQuestStatus(QUEST_THALORIEN_H) == QUEST_STATUS_INCOMPLETE))
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Examine the corpse.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        player->SendPreparedGossip(creature);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();

        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
                player->CLOSE_GOSSIP_MENU();
                creature->AI()->SetGuidData(player->GetGUID());
                creature->SetPhaseMask(2, true);
                player->SetPhaseMask(2, true); // Better handling if we find the correct Phasing-Spell
                creature->AI()->DoAction(ACTION_START_QUEST);
                break;
            default:
                return false;
        }
        return true;
    }

    struct npc_thalorien_dawnseekerAI : public ScriptedAI
    {
        npc_thalorien_dawnseekerAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            if (Player* player = ObjectAccessor::GetPlayer(*me, uiPlayer))
                player->SetPhaseMask(1, true);
            if (Creature* Morlen = ObjectAccessor::GetCreature(*me, uiMorlen))
                Morlen->DisappearAndDie();
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            me->SetPhaseMask(1, true);
            events.Reset();
            uiThalorien.Clear();
            uiMorlen.Clear();
            uiPlayer.Clear();
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case EVENT_STEP_1:
                    if (Creature* Thalorien = me->SummonCreature(NPC_THALORIEN_DAWNSEEKER, 11797.0f, -7074.06f, 26.379f, 0.242908f, TEMPSUMMON_CORPSE_DESPAWN, 0))
                    {
                        Thalorien->SetPhaseMask(2, true);
                        uiThalorien = Thalorien->GetGUID();
                    }

                    for (int i = 0; i < 4; ++i)
                        if (Creature* Guard = me->SummonCreature(NPC_SUNWELL_DEFENDER, GuardLoc[i], TEMPSUMMON_TIMED_DESPAWN, 30000))
                            Guard->SetPhaseMask(2, true);

                    if (Creature* Morlen = me->SummonCreature(NPC_MORLEN_GOLDGRIP, 11776.8f, -7050.72f, 25.2394f, 5.13752f, TEMPSUMMON_CORPSE_DESPAWN, 0))
                    {
                        Morlen->Mount(DISPLAY_MOUNT);
                        Morlen->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        Morlen->SetPhaseMask(2, true);
                        Morlen->SetReactState(REACT_PASSIVE);
                        uiMorlen = Morlen->GetGUID();
                    }
                    events.ScheduleEvent(EVENT_STEP_2, 0.1 * IN_MILLISECONDS);
                    break;
                case EVENT_STEP_2:
                    if (Creature* Thalorien = ObjectAccessor::GetCreature(*me, uiThalorien))
                        Thalorien->AI()->Talk(SAY_THALORIEN_1);
                    events.ScheduleEvent(EVENT_STEP_3, 5 * IN_MILLISECONDS);
                    break;
                case EVENT_STEP_3:
                    if (Creature* Thalorien = ObjectAccessor::GetCreature(*me, uiThalorien))
                        Thalorien->AI()->Talk(SAY_THALORIEN_2);
                    events.ScheduleEvent(EVENT_STEP_4, 5 * IN_MILLISECONDS);
                    break;
                case EVENT_STEP_4:
                    if (Creature* Thalorien = ObjectAccessor::GetCreature(*me, uiThalorien))
                        Thalorien->AI()->Talk(SAY_THALORIEN_3);
                    events.ScheduleEvent(EVENT_STEP_5, 10 * IN_MILLISECONDS);
                    break;
                case EVENT_STEP_5:
                    if (Creature* Thalorien = ObjectAccessor::GetCreature(*me, uiThalorien))
                        Thalorien->AI()->Talk(SAY_THALORIEN_4);
                    events.ScheduleEvent(EVENT_STEP_6, 6 * IN_MILLISECONDS);
                    break;
                case EVENT_STEP_6:
                    if (Creature* Thalorien = ObjectAccessor::GetCreature(*me, uiThalorien))
                        Thalorien->GetMotionMaster()->MovePoint(0, 11787.3f, -7064.11f, 25.8395f);
                    events.ScheduleEvent(EVENT_STEP_7, 6 * IN_MILLISECONDS);
                    break;
                case EVENT_STEP_7:
                    if (Creature* Thalorien = ObjectAccessor::GetCreature(*me, uiThalorien))
                        Thalorien->AI()->Talk(SAY_THALORIEN_5);
                    events.ScheduleEvent(EVENT_STEP_8, 9 * IN_MILLISECONDS);
                    break;
                case EVENT_STEP_8:
                    if (Creature* Thalorien = ObjectAccessor::GetCreature(*me, uiThalorien))
                    { 
                        Thalorien->AI()->Talk(SAY_THALORIEN_6);
                        Thalorien->AI()->Talk(SAY_THALORIEN_7);
                    }                        
                    if (Creature* Morlen = ObjectAccessor::GetCreature(*me, uiMorlen))
                    { 
                        Morlen->CastSpell(Morlen, SPELL_BLOOD_PRESENCE, true);
                        Morlen->AI()->Talk(SAY_MORLEN_GOLDGRIP_1);
                    }                        
                    events.ScheduleEvent(EVENT_STEP_9, 9 * IN_MILLISECONDS);
                    break;
                    // Intro 
                case EVENT_STEP_9:
                    if (Creature* Thalorien = ObjectAccessor::GetCreature(*me, uiThalorien))
                        Thalorien->SetHomePosition(Thalorien->GetPositionX(), Thalorien->GetPositionY(), Thalorien->GetPositionZ(), Thalorien->GetOrientation());
                    SummonWave(NPC_SCOURGE_ZOMBIE);
                    if (Creature* Morlen = ObjectAccessor::GetCreature(*me, uiMorlen))
                        Morlen->AI()->Talk(SAY_MORLEN_GOLDGRIP_2);
                    events.ScheduleEvent(EVENT_STEP_10, 30 * IN_MILLISECONDS);
                    break;
                case EVENT_STEP_10:
                    SummonWave(NPC_GHOUL_INVADER);
                    if (Creature* Morlen = ObjectAccessor::GetCreature(*me, uiMorlen))
                        Morlen->AI()->Talk(SAY_MORLEN_GOLDGRIP_3);
                    events.ScheduleEvent(EVENT_STEP_11, 30 * IN_MILLISECONDS);
                    break;
                case EVENT_STEP_11:
                    SummonWave(NPC_CRYPT_RAIDER);
                    if (Creature* Morlen = ObjectAccessor::GetCreature(*me, uiMorlen))
                        Morlen->AI()->Talk(SAY_MORLEN_GOLDGRIP_4);
                    events.ScheduleEvent(EVENT_STEP_12, 30 * IN_MILLISECONDS);
                    break;
                case EVENT_STEP_12:
                    if (Creature* Thalorien = ObjectAccessor::GetCreature(*me, uiThalorien))
                    {
                        if (Creature* Morlen = ObjectAccessor::GetCreature(*me, uiMorlen))
                        {
                            Morlen->SetReactState(REACT_AGGRESSIVE);
                            AddThreat(Thalorien, 100.0f, Morlen);
                            Morlen->AI()->AttackStart(Thalorien);
                            Morlen->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                            Morlen->AI()->Talk(SAY_MORLEN_GOLDGRIP_5);
                        }
                    }
                    break;                    
                case EVENT_STEP_13:
                    // Outro - Visuals Missing
                    events.ScheduleEvent(EVENT_STEP_14, 3.5 * IN_MILLISECONDS);
                    break;
                case EVENT_STEP_14:
                    if (Creature* Thalorien = ObjectAccessor::GetCreature(*me, uiThalorien))
                        Thalorien->AI()->Talk(SAY_THALORIEN_8);
                    events.ScheduleEvent(EVENT_STEP_15, 3.5 * IN_MILLISECONDS);
                    break;
                case EVENT_STEP_15:
                    if (Creature* Thalorien = ObjectAccessor::GetCreature(*me, uiThalorien))
                        Thalorien->AI()->Talk(SAY_THALORIEN_9);
                    events.ScheduleEvent(EVENT_STEP_16, 3 * IN_MILLISECONDS);
                    break;
                case EVENT_STEP_16:
                    if (Creature* Thalorien = ObjectAccessor::GetCreature(*me, uiThalorien))
                        Thalorien->AI()->Talk(SAY_THALORIEN_10);
                    events.ScheduleEvent(EVENT_STEP_17, 12 * IN_MILLISECONDS);
                    break;
                case EVENT_STEP_17:
                    if (Creature* Thalorien = ObjectAccessor::GetCreature(*me, uiThalorien))
                        Thalorien->DisappearAndDie();
                    if (Player* player = ObjectAccessor::GetPlayer(*me, uiPlayer))
                    {
                        player->KilledMonsterCredit(NPC_QUEST_CREDIT, player->GetGUID());
                        if (player->GetTeam() == ALLIANCE)
                            player->CompleteQuest(QUEST_THALORIEN_A);
                        else
                            player->CompleteQuest(QUEST_THALORIEN_H);
                    }              
                    Reset();
                    break;
                default:
                    break;
          } 

          DoMeleeAttackIfReady();
       }

        void SummonWave(uint32 entry)
        {
            if (Creature* Thalorien = ObjectAccessor::GetCreature(*me, uiThalorien))
            {
                for (int i = 0; i < 5; ++i)
                    if (Creature* Zombie = me->SummonCreature(entry, ZombieLoc[i], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000))
                    {
                        Zombie->SetPhaseMask(2, true);
                        AddThreat(Thalorien, 100.0f, Zombie);
                        Zombie->AI()->AttackStart(Thalorien);
                    }
            }
        }

        void SummonedCreatureDespawn(Creature* summon) override
        {
            if (summon->GetEntry() == NPC_THALORIEN_DAWNSEEKER)
            {
                Reset();
                return;
            }

            if (summon->GetEntry() == NPC_MORLEN_GOLDGRIP)
                events.ScheduleEvent(EVENT_STEP_13, 3 * IN_MILLISECONDS);
        }

        void SetGuidData(ObjectGuid guid, uint32 /*id*/) override
        {
            uiPlayer = guid;
        }

        void DoAction(int32 action) override
        {
            switch (action)
            {
                case ACTION_START_QUEST:
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    events.ScheduleEvent(EVENT_STEP_1, 0);
                    break;
                default:
                    break;
            }
        }
        
    private:
        EventMap events;
        ObjectGuid uiThalorien;
        ObjectGuid uiPlayer;
        ObjectGuid uiMorlen;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_thalorien_dawnseekerAI(creature);
    }
};

enum PurificationIdMisc
{
    // Gameobjects
    GO_QUEL_DELAR            = 201794,

    // Creatures
    NPC_SUNWELL_VISUAL_BUNNY = 37000,
    NPC_SUNWELL_HONOR_GUARD  = 37781,
    NPC_ROMMATH              = 37763,
    NPC_GALIROS              = 38056,
    NPC_THERON               = 37764,
    NPC_AURIC                = 37765,

    // Texts
    SAY_TEXT_0               = 0,
    SAY_TEXT_1               = 1,
    SAY_TEXT_2               = 2,
    SAY_TEXT_3               = 3,
    SAY_TEXT_4               = 4,
    SAY_TEXT_5               = 5,

    // Spells
    SPELL_TELEPORT_SUNWELL   = 70746
};

class spell_bh_cleanse_quel_delar : public SpellScriptLoader
{
    public:
        spell_bh_cleanse_quel_delar() : SpellScriptLoader("spell_bh_cleanse_quel_delar") { }

        class spell_bh_cleanse_quel_delar_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_bh_cleanse_quel_delar_SpellScript);

            void OnEffect(SpellEffIndex effIndex)
            {
                if (Unit* caster = GetCaster())
                    if (Creature* c = caster->FindNearestCreature(NPC_ROMMATH, 50.0f, true))
                        c->AI()->DoAction(-1);
            }

            void Register() override
            {
                OnEffectLaunch += SpellEffectFn(spell_bh_cleanse_quel_delar_SpellScript::OnEffect, EFFECT_0, SPELL_EFFECT_SEND_EVENT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_bh_cleanse_quel_delar_SpellScript();
        }
};

class npc_grand_magister_rommath : public CreatureScript
{
    public:
        npc_grand_magister_rommath() : CreatureScript("npc_grand_magister_rommath") { }
        
        struct npc_grand_magister_rommathAI : public NullCreatureAI
        {
            npc_grand_magister_rommathAI(Creature* creature) : NullCreatureAI(creature)
            {
                announced = false;
                playerGUID.Clear();
                me->SetReactState(REACT_AGGRESSIVE);
            }
            
            void MoveInLineOfSight(Unit* who) override
            {
                if (!announced && who->GetTypeId() == TYPEID_PLAYER && who->GetPositionZ() < 30.0f)
                {
                    announced = true;
                    playerGUID = who->GetGUID();
                    if (Creature* creature = me->FindNearestCreature(NPC_GALIROS, 100.0f, true))
                        creature->AI()->Talk(SAY_TEXT_0, who);
                }
            }

            void DoAction(int32 action) override
            {
                if (action == -1 && !me->isActiveObject())
                {
                    me->SummonGameObject(GO_QUEL_DELAR, 1688.24f, 621.769f, 29.1745f, 0.523177f, 0.0f, 0.0f, 0.0f, 0.0f, 86400);
                    me->SummonCreature(NPC_SUNWELL_VISUAL_BUNNY, 1688.24f, 621.769f, 29.1745f, 0.523177f, TEMPSUMMON_MANUAL_DESPAWN);
                    me->setActive(true);
                    events.Reset();
                    events.ScheduleEvent(1, 1 * IN_MILLISECONDS);   // guard talk
                    events.ScheduleEvent(2, 4 * IN_MILLISECONDS);   // theron talk
                    events.ScheduleEvent(3, 10 * IN_MILLISECONDS);  // npcs walk
                    events.ScheduleEvent(4, 17 * IN_MILLISECONDS);  // rommath talk
                    events.ScheduleEvent(5, 20 * IN_MILLISECONDS);  // theron talk
                    events.ScheduleEvent(6, 28 * IN_MILLISECONDS);  // theron talk
                    events.ScheduleEvent(7, 37 * IN_MILLISECONDS);  // rommath talk
                    events.ScheduleEvent(8, 44 * IN_MILLISECONDS);  // rommath talk
                    events.ScheduleEvent(9, 52 * IN_MILLISECONDS);  // rommath talk
                    events.ScheduleEvent(10, 60 * IN_MILLISECONDS); // auric talk
                    events.ScheduleEvent(11, 66 * IN_MILLISECONDS); // auric talk
                    events.ScheduleEvent(12, 76 * IN_MILLISECONDS); // rommath talk
                    events.ScheduleEvent(13, 80 * IN_MILLISECONDS); // move home
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (!me->isActiveObject())
                    return;

                events.Update(diff);

                switch (events.ExecuteEvent())
                {
                    case 1:
                        if (Creature* creature = me->FindNearestCreature(NPC_SUNWELL_HONOR_GUARD, 60.0f, true))
                            creature->AI()->Talk(SAY_TEXT_0);
                        break;
                    case 2:
                        if (Creature* creature = me->FindNearestCreature(NPC_THERON, 60.0f, true))
                            creature->AI()->Talk(SAY_TEXT_0);
                        break;
                    case 3:
                        me->SetWalk(true);
                        me->GetMotionMaster()->MovePath(me->GetEntry() * 100, false);
                        if (Creature* creature = me->FindNearestCreature(NPC_THERON, 60.0f, true))
                        {
                            creature->SetWalk(true);
                            creature->GetMotionMaster()->MovePath(creature->GetEntry() * 100, false);
                        }
                        if (Creature* creature = me->FindNearestCreature(NPC_AURIC, 60.0f, true))
                        {
                            creature->SetWalk(true);
                            creature->GetMotionMaster()->MovePath(creature->GetEntry() * 100, false);
                        }
                        break;
                    case 4:
                        Talk(SAY_TEXT_0);
                        break;
                    case 5:
                        if (Creature* creature = me->FindNearestCreature(NPC_THERON, 60.0f, true))
                            creature->AI()->Talk(SAY_TEXT_1);
                        break;
                    case 6:
                        if (Creature* creature = me->FindNearestCreature(NPC_THERON, 60.0f, true))
                            creature->AI()->Talk(SAY_TEXT_2, ObjectAccessor::GetPlayer(*me, playerGUID));
                        break;
                    case 7:
                        Talk(SAY_TEXT_1, ObjectAccessor::GetPlayer(*me, playerGUID));
                        break;
                    case 8:
                        Talk(SAY_TEXT_2);
                        break;
                    case 9:
                        Talk(SAY_TEXT_3);
                        break;
                    case 10:
                        if (Creature* creature = me->FindNearestCreature(NPC_AURIC, 60.0f, true))
                            creature->AI()->Talk(SAY_TEXT_0);
                        break;
                    case 11:
                        if (Creature* creature = me->FindNearestCreature(NPC_AURIC, 60.0f, true))
                            creature->AI()->Talk(SAY_TEXT_1);
                        break;
                    case 12:
                        if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
                            Talk(player->GetTeamId() == TEAM_ALLIANCE ? SAY_TEXT_5 : SAY_TEXT_4, player);
                        break;
                    case 13:
                        me->setActive(false);
                        if (Creature* creature = me->FindNearestCreature(NPC_SUNWELL_VISUAL_BUNNY, 60.0f, true))
                            creature->DespawnOrUnsummon(1);
                        if (GameObject* go = me->FindNearestGameObject(GO_QUEL_DELAR, 60.0f))
                            go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        me->SetWalk(true);
                        if (me->GetCreatureData())
                            me->GetMotionMaster()->MovePoint(0, me->GetCreatureData()->posX, me->GetCreatureData()->posY, me->GetCreatureData()->posZ);
                        if (Creature* creature = me->FindNearestCreature(NPC_THERON, 60.0f, true))
                        {
                            creature->SetWalk(true);
                            if (creature->GetCreatureData())
                                creature->GetMotionMaster()->MovePoint(0, creature->GetCreatureData()->posX, creature->GetCreatureData()->posY, creature->GetCreatureData()->posZ);
                        }
                        if (Creature* creature = me->FindNearestCreature(NPC_AURIC, 60.0f, true))
                        {
                            creature->SetWalk(true);
                            if (creature->GetCreatureData())
                                creature->GetMotionMaster()->MovePoint(0, creature->GetCreatureData()->posX, creature->GetCreatureData()->posY, creature->GetCreatureData()->posZ);
                        }
                        break;
                }
            }

            private:
                EventMap events;
                bool announced;
                ObjectGuid playerGUID;
    };

    CreatureAI *GetAI(Creature *creature) const override
    {
        return new npc_grand_magister_rommathAI(creature);
    }
};

class npc_sunwell_warder : public CreatureScript
{
    public:
        npc_sunwell_warder() : CreatureScript("npc_sunwell_warder") { }
    
        struct npc_sunwell_warderAI : public ScriptedAI
        {
            npc_sunwell_warderAI(Creature* creature) : ScriptedAI(creature) { }

            void sGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
            {
                if (gossipListId == 0)
                {
                    player->CLOSE_GOSSIP_MENU();
                    Group* group = player->GetGroup();
                    if (group && group->GetMembersCount() >= 1)
                        group->Disband();
                    ChatHandler(player->GetSession()).PSendSysMessage("Fuer den Abschluss der Quest duerft ihr euch nicht in einer Gruppe oder einem Schlachtzug befinden! Die Aufgabe muss im Sonnenbrunnenplateau ohne einen Schlachtzug abgeschlossen werden.");

                    player->SetGameMaster(true); // Player on that Quest can enter the Instance solo
                    player->CastSpell(player, SPELL_TELEPORT_SUNWELL);
                    player->SetGameMaster(false);
                    player->SetPhaseMask(2, true);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_sunwell_warderAI(creature);
        }
};

void AddSC_isle_of_queldanas_rg()
{
    new npc_thalorien_dawnseeker();
    new npc_grand_magister_rommath();
    new spell_bh_cleanse_quel_delar();
    new npc_sunwell_warder();
}
