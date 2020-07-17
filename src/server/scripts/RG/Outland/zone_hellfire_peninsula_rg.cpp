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
#include "GameEventMgr.h"

/*#####
# npc_grillok
######*/

enum eGrillok
{
    SPELL_FIRE_TOTEM              = 32062,
    SPELL_GROUNDING_TOTEM         = 34079,
    SPELL_SHIELD                  = 12550,
    SPELL_CHAIN_LIGHTNING         = 12058
};

class npc_grillok : public CreatureScript
{
    public:
        npc_grillok() : CreatureScript("npc_grillok") { }

        struct npc_grillokAI: public ScriptedAI
        {
            npc_grillokAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                _events.Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                _events.ScheduleEvent(SPELL_FIRE_TOTEM, 7*IN_MILLISECONDS);
                _events.ScheduleEvent(SPELL_GROUNDING_TOTEM, 5*IN_MILLISECONDS);
                _events.ScheduleEvent(SPELL_SHIELD, 2*IN_MILLISECONDS);
                _events.ScheduleEvent(SPELL_CHAIN_LIGHTNING, 9*IN_MILLISECONDS);

            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case SPELL_FIRE_TOTEM:
                            DoCastSelf(SPELL_FIRE_TOTEM);
                            _events.RescheduleEvent(SPELL_FIRE_TOTEM, 11*IN_MILLISECONDS);
                            break;
                        case SPELL_GROUNDING_TOTEM:
                            DoCastSelf(SPELL_GROUNDING_TOTEM);                            
                            _events.RescheduleEvent(SPELL_GROUNDING_TOTEM, 20*IN_MILLISECONDS);
                            break;
                        case SPELL_SHIELD:
                            DoCastSelf(SPELL_SHIELD);
                            _events.RescheduleEvent(SPELL_SHIELD, 15*IN_MILLISECONDS);
                            break;
                        case SPELL_CHAIN_LIGHTNING:
                            DoCastVictim(SPELL_CHAIN_LIGHTNING);
                            _events.RescheduleEvent(SPELL_CHAIN_LIGHTNING, 30*IN_MILLISECONDS);
                            break;
                        default:
                           break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_grillokAI(creature);
        }
};

enum GenericQuestTriggerEvents
{
    EVENT_CHECK_NPC            = 1,
    NPC_UNSTABLE_LIVING_FLARE  = 24958,
    QUEST_BLAST_THE_GATEWAY    = 11516,
    GO_EXPLOSION               = 185319
};

class npc_generic_quest_trigger : public CreatureScript
{
public:
    npc_generic_quest_trigger() : CreatureScript("npc_generic_quest_trigger") { }

    struct npc_generic_quest_triggerAI : public ScriptedAI
    {
        npc_generic_quest_triggerAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            events.ScheduleEvent(EVENT_CHECK_NPC, 3*IN_MILLISECONDS);
        }

        void CompleteQuest()
        {
            if (Map *map = me->GetMap())
            {
                for (Player& player : map->GetPlayers())
                {
                    if (me->IsInRange(&player, 0.0f, 50.0f))
                        player.CompleteQuest(QUEST_BLAST_THE_GATEWAY);
                }
            }
        }
        
        void UpdateAI(uint32 diff)
        {
            events.Update(diff);
            
            switch (events.ExecuteEvent()) 
            {
                case EVENT_CHECK_NPC:
                    if (Creature* fire = me->FindNearestCreature(NPC_UNSTABLE_LIVING_FLARE, 15.0f, true))
                        if (me->FindNearestPlayer(20.0f))
                        {
                            fire->DespawnOrUnsummon();
                            CompleteQuest();
                            me->SummonGameObject(GO_EXPLOSION, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 60);
                            me->DespawnOrUnsummon(1*IN_MILLISECONDS);
                        }
                    events.ScheduleEvent(EVENT_CHECK_NPC, 2*IN_MILLISECONDS);
                    break;
                default:
                    break;
            }        
        }
        private:
            EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_generic_quest_triggerAI(creature);
    }
};

/*######
## npc_anchorite_barada
######*/
enum
{
    SAY_EXORCISM_1                   = 0,
    SAY_EXORCISM_2                   = 1,
    SAY_EXORCISM_3                   = 2,
    SAY_EXORCISM_4                   = 3,
    SAY_EXORCISM_5                   = 4,
    SAY_EXORCISM_6                   = 5,
    ANCHORITE_TEXTS                  = 6,
    CONEL_TEXTS                      = 8,
    SPELL_JULES_GOES_PRONE           = 39283,
    SPELL_BARADA_COMMANDS            = 39277,
    SPELL_BARADA_FALTERS             = 39278,
    SPELL_JULES_THREATENS            = 39284,
    SPELL_JULES_GOES_UPRIGHT         = 39294,
    SPELL_JULES_VOMITS               = 39295,
    SPELL_JULES_RELEASE_DARKNESS     = 39306, // periodic trigger missing spell 39305
    NPC_ANCHORITE_BARADA             = 22431,
    NPC_COLONEL_JULES                = 22432,
    NPC_DARKNESS_RELEASED            = 22507, // summoned by missing spell 39305
    QUEST_ID_EXORCISM                = 10935,
    TEXT_ID_CLEANSED                 = 10706,
    TEXT_ID_POSSESSED                = 10707,
    TEXT_ID_ANCHORITE                = 10683,
    EVENT_SAY_EXORCISM_1             = 1,
    EVENT_SAY_EXORCISM_2             = 2,
    EVENT_QUEST_ID_EXORCISM          = 3,
    EVENT_KNEEL_DOWN                 = 5,
    EVENT_SAY_EXORCISM_3             = 6,
    EVENT_SPELL_BARADA_COMMANDS      = 7,
    EVENT_SAY_EXORCISM_4             = 8,
    EVENT_SAY_EXORCISM_5             = 9,
    EVENT_SPELL_BARADA_FALTERS       = 10,
    EVENT_SPELL_JULES_THREATENS      = 11,
    EVENT_NPC_COLONEL_JULES1         = 12,
    EVENT_NPC_ANCHORITE_BARADA1      = 13,
    EVENT_NPC_COLONEL_JULES2         = 14,
    EVENT_NPC_ANCHORITE_BARADA2      = 15,
    EVENT_SPELL_JULES_GOES_UPRIGHT   = 16,
    EVENT_SPELL_JULES_VOMITS         = 17,
    EVENT_NPC_COLONEL_JULES3         = 18,
    EVENT_NPC_ANCHORITE_BARADA3      = 19,
    EVENT_NPC_COLONEL_JULES4         = 20,
    EVENT_NPC_ANCHORITE_BARADA4      = 21,
    EVENT_NPC_COLONEL_JULES5         = 22,
    EVENT_NPC_ANCHORITE_BARADA5      = 23,
    EVENT_NPC_COLONEL_JULES6         = 24,
    EVENT_NPC_ANCHORITE_BARADA6      = 25,
    EVENT_NPC_DARKNESS_RELEASED      = 26,
    EVENT_SAY_EXORCISM_6             = 27,
    EVENT_TEXT_ID_CLEANSED           = 28,
    EVENT_SUMMON_SKULL               = 29,
    HOME_REACHED                     = 30,
    ACTION_START_EVENT               = 0,
    ACTION_JULES_HOVER               = 1,
    ACTION_JULES_FLIGH               = 2,
    ACTION_JULES_MOVE_HOME           = 3,
    NPC_FOUL_PURGE                   = 22506,
};

Position const exorcismPos[11] =
{
    { -707.123f, 2751.686f, 101.592f, 4.577416f }, //Barada Waypoint-1 0
    { -710.731f, 2749.075f, 101.592f, 1.513286f }, //Barada Castposition 1
    { -710.332f, 2754.394f, 102.948f, 3.207566f }, //Jules Schwebeposition 2
    { -714.261f, 2747.754f, 103.391f, 0.0f },      //Jules Waypoint-1 3
    { -713.113f, 2750.194f, 103.391f, 0.0f },      //Jules Waypoint-2 4
    { -710.385f, 2750.896f, 103.391f, 0.0f },      //Jules Waypoint-3 5
    { -708.309f, 2750.062f, 103.391f, 0.0f },      //Jules Waypoint-4 6
    { -707.401f, 2747.696f, 103.391f, 0.0f },      //Jules Waypoint-5 7 uebergeben
    { -708.591f, 2745.266f, 103.391f, 0.0f },      //Jules Waypoint-6 8
    { -710.597f, 2744.035f, 103.391f, 0.0f },      //Jules Waypoint-7 9
    { -713.089f, 2745.302f, 103.391f, 0.0f },      //Jules Waypoint-8 10
};

class npc_anchorite_barada : public CreatureScript
{
public:
    npc_anchorite_barada() : CreatureScript("npc_anchorite_barada") { }

    struct npc_anchorite_baradaAI : public ScriptedAI
    {
        npc_anchorite_baradaAI(Creature* creature) : ScriptedAI(creature)
        {
            Reset();
        }

        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_NONE);
            events.Reset();
        }

        void CompleteQuest()
        {
            if (Map *map = me->GetMap())
            {
                for (Player& player : map->GetPlayers())
                {
                    if (me->IsInRange(&player, 0.0f, 30.0f))
                        player.CompleteQuest(QUEST_ID_EXORCISM);
                }
            }
        }

        void sGossipSelect(Player* player, uint32 sender, uint32 action)
        {
            if (player->GetQuestStatus(QUEST_ID_EXORCISM) == QUEST_STATUS_INCOMPLETE)
            {
                if (me->GetCreatureTemplate()->GossipMenuId == sender && !action)
                {
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    me->SetStandState(UNIT_STAND_STATE_STAND);
                    me->AI()->DoAction(ACTION_START_EVENT);
                    me->setActive(true);
                }
            }
        }

        void DoAction(int32 action)
        {
            if (action == ACTION_START_EVENT)
            {
                events.ScheduleEvent(EVENT_SAY_EXORCISM_1, 2 * IN_MILLISECONDS);
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            if (Creature * Colonel = me->FindNearestCreature(NPC_COLONEL_JULES, 50.0f, true))
            {
                Colonel->AI()->DoAction(ACTION_JULES_MOVE_HOME);
                Colonel->RemoveAllAuras();
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SAY_EXORCISM_1:
                        Talk(SAY_EXORCISM_1);
                        events.ScheduleEvent(EVENT_SAY_EXORCISM_2, 3 * IN_MILLISECONDS);
                        events.CancelEvent(EVENT_SAY_EXORCISM_1);
                        break;
                    case EVENT_SAY_EXORCISM_2:
                        Talk(SAY_EXORCISM_2);
                        events.ScheduleEvent(EVENT_QUEST_ID_EXORCISM, 2 * IN_MILLISECONDS);
                        events.CancelEvent(EVENT_SAY_EXORCISM_2);
                        break;
                    case EVENT_QUEST_ID_EXORCISM:
                        me->GetMotionMaster()->MovePath(NPC_ANCHORITE_BARADA * 10, false);
                        events.ScheduleEvent(EVENT_KNEEL_DOWN, 5 * IN_MILLISECONDS);
                        events.CancelEvent(EVENT_QUEST_ID_EXORCISM);
                        break;
                    case EVENT_KNEEL_DOWN:
                        me->SetStandState(UNIT_STAND_STATE_KNEEL);
                        if (Creature* Colonel = me->FindNearestCreature(NPC_COLONEL_JULES, 50.0f, true))
                            me->SetFacingToObject(Colonel);
                        events.ScheduleEvent(EVENT_SAY_EXORCISM_3, 1 * IN_MILLISECONDS);
                        events.CancelEvent(EVENT_KNEEL_DOWN);
                        break;
                    case EVENT_SAY_EXORCISM_3:
                        if (Creature* Colonel = me->FindNearestCreature(NPC_COLONEL_JULES, 50.0f, true))
                            Colonel->AI()->Talk(SAY_EXORCISM_3);
                        events.ScheduleEvent(EVENT_SPELL_BARADA_COMMANDS, 3 * IN_MILLISECONDS);
                        break;
                    case EVENT_SPELL_BARADA_COMMANDS:
                        DoCastSelf(SPELL_BARADA_COMMANDS, true);
                        events.ScheduleEvent(EVENT_SAY_EXORCISM_4, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_SAY_EXORCISM_4:
                        Talk(SAY_EXORCISM_4);
                        events.ScheduleEvent(EVENT_SAY_EXORCISM_5, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_SAY_EXORCISM_5:
                        if (Creature* Colonel = me->FindNearestCreature(NPC_COLONEL_JULES, 50.0f, true))
                            Colonel->AI()->Talk(SAY_EXORCISM_5);
                        events.ScheduleEvent(EVENT_SPELL_BARADA_FALTERS, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_SPELL_BARADA_FALTERS:
                        DoCastSelf(SPELL_BARADA_FALTERS, true);
                        // start levitating
                        if (Creature* Colonel = me->FindNearestCreature(NPC_COLONEL_JULES, 50.0f, true))
                        {
                            Colonel->SetCanFly(true);
                            Colonel->SetDisableGravity(true);
                            Colonel->GetAI()->DoAction(ACTION_JULES_HOVER);
                        }
                        events.ScheduleEvent(EVENT_SPELL_JULES_THREATENS, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_SPELL_JULES_THREATENS:
                        if (Creature* Colonel = me->FindNearestCreature(NPC_COLONEL_JULES, 50.0f, true))
                        {
                            Colonel->CastSpell(Colonel, SPELL_JULES_THREATENS, true);
                            Colonel->CastSpell(Colonel, SPELL_JULES_RELEASE_DARKNESS, true);
                            Colonel->SetFacingTo(0);
                        }
                        events.ScheduleEvent(EVENT_NPC_COLONEL_JULES1, 15 * IN_MILLISECONDS);
                        break;
                    case EVENT_NPC_COLONEL_JULES1:
                        if (Creature* Colonel = me->FindNearestCreature(NPC_COLONEL_JULES, 50.0f, true))
                            Colonel->AI()->Talk(CONEL_TEXTS);
                        events.ScheduleEvent(EVENT_NPC_ANCHORITE_BARADA1, 15 * IN_MILLISECONDS);
                        break;
                    case EVENT_NPC_ANCHORITE_BARADA1:
                        Talk(ANCHORITE_TEXTS);
                        events.ScheduleEvent(EVENT_NPC_COLONEL_JULES2, 15 * IN_MILLISECONDS);
                        break;
                    case EVENT_NPC_COLONEL_JULES2:
                        if (Creature* Colonel = me->FindNearestCreature(NPC_COLONEL_JULES, 50.0f, true))
                            Colonel->AI()->Talk(CONEL_TEXTS);
                        events.ScheduleEvent(EVENT_NPC_ANCHORITE_BARADA2, 15 * IN_MILLISECONDS);
                        break;
                    case EVENT_NPC_ANCHORITE_BARADA2:
                        Talk(ANCHORITE_TEXTS);
                        events.ScheduleEvent(EVENT_SPELL_JULES_GOES_UPRIGHT, 15 * IN_MILLISECONDS);
                        break;
                    case EVENT_SPELL_JULES_GOES_UPRIGHT:
                        if (Creature* Colonel = me->FindNearestCreature(NPC_COLONEL_JULES, 50.0f, true))
                        {
                            Colonel->InterruptNonMeleeSpells(false);
                            Colonel->CastSpell(Colonel, SPELL_JULES_GOES_UPRIGHT, false);
                        }
                        events.ScheduleEvent(EVENT_SPELL_JULES_VOMITS, 3 * IN_MILLISECONDS);
                        break;
                    case EVENT_SPELL_JULES_VOMITS:
                        if (Creature* Colonel = me->FindNearestCreature(NPC_COLONEL_JULES, 50.0f, true))
                        {
                            Colonel->CastSpell(Colonel, SPELL_JULES_VOMITS, true);
                            //Colonel->GetMotionMaster()->MovePath(NPC_COLONEL_JULES * 10, true); -- alternativ way
                            Colonel->AI()->DoAction(ACTION_JULES_FLIGH);
                        }
                        events.ScheduleEvent(EVENT_NPC_COLONEL_JULES3, 7 * IN_MILLISECONDS);
                        break;
                    case EVENT_NPC_COLONEL_JULES3:
                        if (Creature* Colonel = me->FindNearestCreature(NPC_COLONEL_JULES, 50.0f, true))
                            Colonel->AI()->Talk(CONEL_TEXTS);
                        events.ScheduleEvent(EVENT_NPC_ANCHORITE_BARADA3, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_NPC_ANCHORITE_BARADA3:
                        Talk(ANCHORITE_TEXTS);
                        events.ScheduleEvent(EVENT_NPC_COLONEL_JULES4, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_NPC_COLONEL_JULES4:
                        if (Creature* Colonel = me->FindNearestCreature(NPC_COLONEL_JULES, 50.0f, true))
                            Colonel->AI()->Talk(CONEL_TEXTS);
                        events.ScheduleEvent(EVENT_NPC_ANCHORITE_BARADA4, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_NPC_ANCHORITE_BARADA4:
                        Talk(ANCHORITE_TEXTS);
                        events.ScheduleEvent(EVENT_NPC_COLONEL_JULES5, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_NPC_COLONEL_JULES5:
                        if (Creature* Colonel = me->FindNearestCreature(NPC_COLONEL_JULES, 50.0f, true))
                            Colonel->AI()->Talk(CONEL_TEXTS);
                        events.ScheduleEvent(EVENT_NPC_ANCHORITE_BARADA5, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_NPC_ANCHORITE_BARADA5:
                        Talk(ANCHORITE_TEXTS);
                        events.ScheduleEvent(EVENT_NPC_COLONEL_JULES6, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_NPC_COLONEL_JULES6:
                        if (Creature* Colonel = me->FindNearestCreature(NPC_COLONEL_JULES, 50.0f, true))
                            Colonel->AI()->Talk(CONEL_TEXTS);
                        events.ScheduleEvent(EVENT_NPC_ANCHORITE_BARADA6, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_NPC_ANCHORITE_BARADA6:
                        Talk(ANCHORITE_TEXTS);
                        events.ScheduleEvent(EVENT_NPC_DARKNESS_RELEASED, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_NPC_DARKNESS_RELEASED:
                        if (Creature* Colonel = me->FindNearestCreature(NPC_COLONEL_JULES, 50.0f, true))
                        {
                            Colonel->RemoveAurasDueToSpell(SPELL_JULES_THREATENS);
                            Colonel->RemoveAurasDueToSpell(SPELL_JULES_RELEASE_DARKNESS);
                            Colonel->RemoveAurasDueToSpell(SPELL_JULES_VOMITS);
                            Colonel->GetMotionMaster()->MoveTargetedHome();
                            Colonel->AI()->DoAction(ACTION_JULES_MOVE_HOME);
                            Colonel->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        }
                        events.ScheduleEvent(EVENT_SAY_EXORCISM_6, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_SAY_EXORCISM_6:
                        Talk(SAY_EXORCISM_6);
                        events.ScheduleEvent(EVENT_TEXT_ID_CLEANSED, 3 * IN_MILLISECONDS);
                        break;
                    case EVENT_TEXT_ID_CLEANSED:
                        if (Creature* Colonel = me->FindNearestCreature(NPC_COLONEL_JULES, 50.0f, true))
                        {
                            Colonel->RemoveAurasDueToSpell(SPELL_JULES_GOES_UPRIGHT);
                            Colonel->SetCanFly(false);
                            Colonel->SetDisableGravity(false);
                        }
                        // resume wp movemnet
                        me->RemoveAllAuras();
                        me->SetStandState(UNIT_STAND_STATE_STAND);
                        EnterEvadeMode();
                        CompleteQuest();
                        me->GetMotionMaster()->MovePath(NPC_ANCHORITE_BARADA * 100, false);
                        events.ScheduleEvent(HOME_REACHED, 6 * IN_MILLISECONDS);
                        break;
                    case HOME_REACHED:
                        me->SetStandState(UNIT_STAND_STATE_KNEEL);
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        break;
                    default:
                        break;
                }
            }
            // does not melee
        }
    private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_anchorite_baradaAI(creature);
    }
};

class npc_colonel_jules : public CreatureScript
{
public:
    npc_colonel_jules() : CreatureScript("npc_colonel_jules") { }

    struct npc_colonel_julesAI : public ScriptedAI
    {
        npc_colonel_julesAI(Creature* creature) : ScriptedAI(creature), summons(me) { }

        EventMap events;
        SummonList summons;

        uint8 SpawnCount;
        uint8 point;

        bool wpreached;

        void Reset()
        {
            events.Reset();

            summons.DespawnAll();
            SpawnCount = 2;
            point = 3;
            wpreached = false;
        }

        void DoAction(int32 action)
        {
            switch (action)
            {
                case ACTION_JULES_HOVER:
                    me->AddAura(SPELL_JULES_GOES_PRONE, me);
                    me->SetCanFly(true);
                    me->SetSpeedRate(MOVE_RUN, 0.2f);
                    me->SetFacingTo(3.207566f);
                    me->GetMotionMaster()->MoveJump(exorcismPos[2], 2.0f, 2.0f);
                    events.ScheduleEvent(EVENT_SUMMON_SKULL, 10 * IN_MILLISECONDS);
                    break;
                case ACTION_JULES_FLIGH:
                    me->RemoveAura(SPELL_JULES_GOES_PRONE);
                    me->AddAura(SPELL_JULES_GOES_UPRIGHT, me);
                    me->AddAura(SPELL_JULES_VOMITS, me);
                    wpreached = true;
                    me->GetMotionMaster()->MovePoint(point, exorcismPos[point]);
                    break;
                case ACTION_JULES_MOVE_HOME:
                    wpreached = false;
                    me->SetSpeedRate(MOVE_RUN, 1.0f);
                    me->GetMotionMaster()->MovePoint(11, exorcismPos[2]);
                    events.CancelEvent(EVENT_SUMMON_SKULL);
                    break;
            }
        }

        void JustSummoned(Creature* summon)
        {
            summons.Summon(summon);

            if (summon->GetAI())
                summon->GetMotionMaster()->MoveRandom(0.2f);
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id < 10)
                wpreached = true;

            if (id == 8)
            {
                for (uint8 i = 0; i < SpawnCount; i++)
                    DoSummon(NPC_FOUL_PURGE, exorcismPos[8], 60000, TEMPSUMMON_TIMED_DESPAWN);
            }

            if (id == 10)
            {
                wpreached = true;
                point = 3;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (wpreached)
            {
                me->GetMotionMaster()->MovePoint(point, exorcismPos[point]);
                point++;
                wpreached = false;
            }

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SUMMON_SKULL:
                        uint8 summonCount = urand(1, 2);
                        for (uint8 i = 0; i < summonCount; i++)
                            me->SummonCreature(NPC_DARKNESS_RELEASED, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 1.5f, 0, TEMPSUMMON_MANUAL_DESPAWN);
                        events.ScheduleEvent(EVENT_SUMMON_SKULL, urand(10, 20)*IN_MILLISECONDS);
                        break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_colonel_julesAI(creature);
    }
};

enum WarpRifts
{
    SPELL_SUMMON_VOIDWALKER = 34943,
    DISPLAY_ID_WARP_RIFT    = 18684,
};

class npc_void_spawner_quest_warp_rifts : public CreatureScript
{
public:
    npc_void_spawner_quest_warp_rifts() : CreatureScript("npc_void_spawner_quest_warp_rifts") { }

    struct npc_void_spawner_quest_warp_riftsAI : public ScriptedAI
    {
        npc_void_spawner_quest_warp_riftsAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _spawnVoidWalkerTimer = urand(2000, 5000);
            me->SetDisplayId(DISPLAY_ID_WARP_RIFT);
            me->DespawnOrUnsummon(120000);
        }

        void JustSummoned(Creature* summon) override 
        {
            if (summon)
            {
                summon->GetMotionMaster()->MoveRandom(25.0f);

                if (Player* target = me->FindNearestPlayer(35.0f))
                    summon->GetAI()->AttackStart(target);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (_spawnVoidWalkerTimer <= diff)
            {
                DoCastSelf(SPELL_SUMMON_VOIDWALKER, true);
                _spawnVoidWalkerTimer = urand(15000, 23000);
            }
            else _spawnVoidWalkerTimer -= diff;
        }

    private:
        uint32 _spawnVoidWalkerTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_void_spawner_quest_warp_riftsAI(creature);
    }
};

enum TheFootOfTheCitadel
{
    SAY_AGGRO                   = 0,
    NPC_COMMANDER_GORAX         = 19264,
    NPC_HAND_OF_KARGATH         = 22374,
    NPC_HAND_BERSERKER          = 16878,

    SPELL_TOUGHEN               = 33962,
    SEPLL_HAMSTRING             = 9080,
    SPELL_BLADE_FURRY           = 33735,
    SPELL_CHARGE                = 24193,

    EVENT_HAMSTRING             = 1,
    EVENT_BLADE_FURRY           = 2,
    EVENT_CHARGE_KARGATH        = 3,
};

const Position BerserkerPos[3] =
{
    { -252.792f, 3030.24f, -65.7743f, 1.309f },
    { -246.012f, 3027.31f, -65.4702f, 1.309f },
    { -248.426f, 3032.92f, -65.5952f, 1.309f },
};

const Position BerserkerHomePos[3] =
{
    { -228.660f, 3078.85f, -62.0914f, 1.368f },
    { -227.936f, 3077.15f, -62.0005f, 1.669f },
    { -230.333f, 3077.69f, -62.5202f, 1.194f },
};

class npc_hand_of_kargath : public CreatureScript
{
public:
    npc_hand_of_kargath() : CreatureScript("npc_hand_of_kargath") { }

    struct npc_hand_of_kargathAI : public ScriptedAI
    {
        npc_hand_of_kargathAI(Creature* creature) : ScriptedAI(creature) { }

        void InitializeAI() override
        {
            _wasInCombat = false;
        }

        void Reset() override
        {
            _lastHealth = 10;

            if (_wasInCombat)
            {
                uint32 y = 0;
                for (uint32 i = 1; i < 3; ++i)
                {
                    if (Creature* add = ObjectAccessor::GetCreature(*me, _beserkerGUID[y]))
                        add->DespawnOrUnsummon();

                    if (Creature* add = me->SummonCreature(NPC_HAND_BERSERKER, BerserkerHomePos[i], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 3 * MINUTE * IN_MILLISECONDS))
                        _beserkerGUID[y++] = add->GetGUID();
                }

                _wasInCombat = false;
            }
        }

        void ForceCombat(Unit* caster)
        {
            if (caster)
            {
                me->AI()->AttackStart(caster);
                me->SetHomePosition(BerserkerHomePos[0]);

                uint32 y = 0;
                for (uint32 i = 1; i < 3; ++i)
                {
                    if (Creature* add = ObjectAccessor::GetCreature(*me, _beserkerGUID[i]))
                        add->DespawnOrUnsummon();

                    if (Creature* add = me->SummonCreature(NPC_HAND_BERSERKER, BerserkerPos[i], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 3 * MINUTE * IN_MILLISECONDS))
                    {
                        add->GetMotionMaster()->MoveFollow(me, 1.0f, (y == 0 ? -3 : 3) * M_PI_F / 4);

                        add->SetHomePosition(BerserkerHomePos[1]);
                        add->AI()->AttackStart(caster);
                        _beserkerGUID[y++] = add->GetGUID();
                    }
                }
            }
        }

        void EnterCombat(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);
            me->CallForHelp(15.0f);
            DoCastSelf(SPELL_TOUGHEN);
            _wasInCombat = true;

            _events.Reset();
            _events.ScheduleEvent(EVENT_BLADE_FURRY,    26 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_HAMSTRING,      52 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_CHARGE_KARGATH, 15 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (_lastHealth > static_cast<uint8>(ceil(me->GetHealthPct() / 10)))
            {
                _lastHealth = static_cast<uint8>(ceil(me->GetHealthPct() / 10));
                DoCastSelf(SPELL_TOUGHEN);
            }

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_HAMSTRING:
                        DoCastSelf(SEPLL_HAMSTRING);
                        _events.ScheduleEvent(EVENT_HAMSTRING, urand(4, 6) * IN_MILLISECONDS);
                        break;
                    case EVENT_BLADE_FURRY:
                        DoCastSelf(SPELL_BLADE_FURRY);
                        _events.ScheduleEvent(EVENT_BLADE_FURRY, urand(30, 40) * IN_MILLISECONDS);
                        break;
                    case EVENT_CHARGE_KARGATH:
                        if (Unit* victim = SelectTarget(SELECT_TARGET_MAXDISTANCE, 0, 40.0f))
                            DoCast(victim, SPELL_CHARGE);
                        _events.ScheduleEvent(EVENT_CHARGE_KARGATH, urand(12, 16) * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        EventMap    _events;
        uint8       _lastHealth;
        ObjectGuid  _beserkerGUID[2];
        bool        _wasInCombat;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_hand_of_kargathAI(creature);
    }
};

class spell_challenge_the_horde : public SpellScriptLoader
{
public:
    spell_challenge_the_horde() : SpellScriptLoader("spell_challenge_the_horde") { }

    class spell_challenge_the_horde_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_challenge_the_horde_SpellScript)

        SpellCastResult CheckCast()
        {
            if (GetCaster() && GetCaster()->FindNearestCreature(NPC_HAND_OF_KARGATH, 150.0f))
                return SPELL_FAILED_ALREADY_HAVE_SUMMON;

            return SPELL_CAST_OK;
        }

        void HandleDummy(SpellEffIndex /*effindex*/)
        {
            if (Unit* caster = GetCaster())
                if (Creature* kargath = caster->SummonCreature(NPC_HAND_OF_KARGATH, BerserkerPos[2], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 3 * MINUTE * IN_MILLISECONDS))
                    ENSURE_AI(npc_hand_of_kargath::npc_hand_of_kargathAI, kargath->AI())->ForceCombat(caster);
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_challenge_the_horde_SpellScript::CheckCast);
            OnEffectLaunch += SpellEffectFn(spell_challenge_the_horde_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_SUMMON_OBJECT_WILD);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_challenge_the_horde_SpellScript();
    }
};

class spell_magtheridon_reputation_bonus : public SpellScriptLoader
{
    public:
        spell_magtheridon_reputation_bonus(char const* name, uint16 eventId) : SpellScriptLoader(name), eventId(eventId) { }

        class spell_magtheridon_reputation_bonus_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_magtheridon_reputation_bonus_AuraScript);

        public:
            spell_magtheridon_reputation_bonus_AuraScript(uint16 eventId) : eventId(eventId) { }


            // Add Update for Areacheck and BF-Timercheck
            void CalcPeriodic(AuraEffect const* /*effect*/, bool& isPeriodic, int32& amplitude)
            {
                isPeriodic = true;
                amplitude = 1 * IN_MILLISECONDS;
            }

            void Update(AuraEffect* /*effect*/)
            {
                if (!IsEventActive(eventId))
                    GetUnitOwner()->RemoveAura(GetSpellInfo()->Id);
            }

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                if (!IsEventActive(eventId))
                    caster->RemoveAura(GetSpellInfo()->Id);
            }

            void Register()
            {
               DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_magtheridon_reputation_bonus_AuraScript::CalcPeriodic, EFFECT_0, SPELL_AURA_MOD_FACTION_REPUTATION_GAIN);
               OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_magtheridon_reputation_bonus_AuraScript::Update, EFFECT_0, SPELL_AURA_MOD_FACTION_REPUTATION_GAIN);
               OnEffectApply += AuraEffectApplyFn(spell_magtheridon_reputation_bonus_AuraScript::OnApply, EFFECT_0, SPELL_AURA_MOD_FACTION_REPUTATION_GAIN, AURA_EFFECT_HANDLE_REAL);
            }

        private:
            uint16 eventId;
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_magtheridon_reputation_bonus_AuraScript(eventId);
        }

    private:
        uint16 eventId;
};

void AddSC_hellfire_peninsula_rg()
{
    new npc_grillok();
    new npc_generic_quest_trigger();
    new npc_anchorite_barada();
    new npc_colonel_jules();
    new npc_void_spawner_quest_warp_rifts();
    new spell_challenge_the_horde();
    new npc_hand_of_kargath();
    new spell_magtheridon_reputation_bonus("spell_trollbanes_command", 80);
    new spell_magtheridon_reputation_bonus("spell_nazgrels_fervor", 81);
}
