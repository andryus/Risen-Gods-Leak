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

/*######
## npc_thrall_warchief_rg
######*/

enum ThrallWarchiefSpell
{
    // Questrelated
    SPELL_WARCHIEFS_BLESSING     = 16609,

    SPELL_CHAIN_LIGHTNING        = 16033,
    SPELL_SHOCK                  = 16034
};

enum ThrallWarchiefText
{
    SAY_HORDE_1                  = 0,
    SAY_HORDE_2                  = 1,
    SAY_HORDE_3                  = 2,
    SAY_HORDE_4                  = 3,
    SAY_HORDE_5                  = 4,
    SAY_HORDE_6                  = 5,
    SAY_HORDE_7                  = 6,
    SAY_HORDE_8                  = 7,

    SAY_ETRRIGG_0                = 0,
    SAY_ETRRIGG_1                = 1,
    SAY_ETRRIGG_2                = 2,

    SAY_BLESSING_1               = 9,
    SAY_BLESSING_2               = 10,
    SAY_BLESSING_3               = 11,
    SAY_BLESSING_4               = 12

}; 

enum ThrallWarchiefQuests
{
    QUEST_WHAT_THE_WIND_CARRIES  = 6566,
    QUEST_FOR_THE_HORDE          = 4974,
    QUEST_MESSAGE_TO_THRALL      = 9438,
    QUEST_WARCHIEFS_BLESSING     = 13189,
    QUEST_RUBY_PRE_EVENT         = 110201,
    QUEST_RUBY_PRE_EVENT_2       = 110197,
};

enum ThrallWarchiefEvents
{
    EVENT_CHAIN_LIGHTNING        = 1,
    EVENT_SHOCK                  = 2,
    EVENT_FOR_THE_HORDE_1        = 3,
    EVENT_FOR_THE_HORDE_2        = 4,
    EVENT_FOR_THE_HORDE_3        = 5,
    EVENT_FOR_THE_HORDE_4        = 6,
    EVENT_FOR_THE_HORDE_5        = 7,
    EVENT_FOR_THE_HORDE_6        = 8,

    EVENT_MESSAGE_TO_THRALL_1    = 9,
    EVENT_MESSAGE_TO_THRALL_2    = 10,
    EVENT_MESSAGE_TO_THRALL_3    = 11,
    EVENT_MESSAGE_TO_THRALL_4    = 12,
    EVENT_MESSAGE_TO_THRALL_5    = 13,
    EVENT_MESSAGE_TO_THRALL_6    = 14,
    EVENT_MESSAGE_TO_THRALL_7    = 15,
    EVENT_MESSAGE_TO_THRALL_8    = 16,
    EVENT_MESSAGE_TO_THRALL_9    = 17,
    EVENT_MESSAGE_TO_THRALL_10   = 18,
    EVENT_MESSAGE_TO_THRALL_11   = 19,
    EVENT_MESSAGE_TO_THRALL_12   = 20,
    EVENT_MESSAGE_TO_THRALL_13   = 21,
    EVENT_MESSAGE_TO_THRALL_14   = 22,
    EVENT_MESSAGE_TO_THRALL_15   = 23,
    EVENT_MESSAGE_TO_THRALL_16   = 24,
    EVENT_MESSAGE_TO_THRALL_17   = 25,
    EVENT_MESSAGE_TO_THRALL_18   = 26,
    EVENT_MESSAGE_TO_THRALL_19   = 27,
    EVENT_MESSAGE_TO_THRALL_20   = 28,

    EVENT_BLESSING_1             = 29,
    EVENT_BLESSING_2             = 30,
    EVENT_BLESSING_3             = 31,
    EVENT_BLESSING_4             = 32
};

enum ThrallWarchiefMisc
{
    ACTION_START_EVENT           = 1,
    ACTION_START_MESSAGE         = 2,
    ACTION_START_BLESSING        = 3,
    GO_UNDORNED_STAKE            = 175787,
    NPC_ETRIGG                   = 3144
};

enum ThrallWarchiefGossip
{
    GOSSIP_MENU_OPTION_ID_ALL    = 0,

    OPTION_PLEASE_SHARE_YOUR     = 3664,
    OPTION_WHAT_DISCOVERIES      = 3665,
    OPTION_USURPER               = 3666,
    OPTION_WITH_ALL_DUE_RESPECT  = 3667,
    OPTION_I_I_DID_NOT_THINK_OF  = 3668,
    OPTION_I_LIVE_ONLY_TO_SERVE  = 3669,
    OPTION_OF_COURSE_WARCHIEF    = 3670,

    GOSSIP_MEMBERS_OF_THE_HORDE  = 4477,
    GOSSIP_THE_SHATTERED_HAND    = 5733,
    GOSSIP_IT_WOULD_APPEAR_AS    = 5734,
    GOSSIP_THE_BROOD_MOTHER      = 5735,
    GOSSIP_SO_MUCH_TO_LEARN      = 5736,
    GOSSIP_I_DO_NOT_FAULT_YOU    = 5737,
    GOSSIP_NOW_PAY_ATTENTION     = 5738,
};

class DelayedQuestCompleteEvent : public BasicEvent
{
    public:
        DelayedQuestCompleteEvent(Player& owner, uint32 questId) : BasicEvent(), _owner(owner), _quest(questId) { }

        bool Execute(uint64 /*eventTime*/, uint32 /*diff*/)
        {
            _owner.AreaExploredOrEventHappens(_quest);
            return true;
        }

    private:
        uint32 _quest;
        Player& _owner;
};

class npc_thrall_warchief_rg : public CreatureScript
{
    public:
        npc_thrall_warchief_rg() : CreatureScript("npc_thrall_warchief_rg") { }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
        {
            player->PlayerTalkClass->ClearMenus();
            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF + 1:
                    player->ADD_GOSSIP_ITEM_DB(OPTION_WHAT_DISCOVERIES, GOSSIP_MENU_OPTION_ID_ALL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                    player->SEND_GOSSIP_MENU(GOSSIP_THE_SHATTERED_HAND, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF + 2:
                    player->ADD_GOSSIP_ITEM_DB(OPTION_USURPER, GOSSIP_MENU_OPTION_ID_ALL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                    player->SEND_GOSSIP_MENU(GOSSIP_IT_WOULD_APPEAR_AS, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF + 3:
                    player->ADD_GOSSIP_ITEM_DB(OPTION_WITH_ALL_DUE_RESPECT, GOSSIP_MENU_OPTION_ID_ALL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
                    player->SEND_GOSSIP_MENU(GOSSIP_THE_BROOD_MOTHER, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF + 4:
                    player->ADD_GOSSIP_ITEM_DB(OPTION_I_I_DID_NOT_THINK_OF, GOSSIP_MENU_OPTION_ID_ALL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
                    player->SEND_GOSSIP_MENU(GOSSIP_SO_MUCH_TO_LEARN, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF + 5:
                    player->ADD_GOSSIP_ITEM_DB(OPTION_I_LIVE_ONLY_TO_SERVE, GOSSIP_MENU_OPTION_ID_ALL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);
                    player->SEND_GOSSIP_MENU(GOSSIP_I_DO_NOT_FAULT_YOU, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF + 6:
                    player->ADD_GOSSIP_ITEM_DB(OPTION_OF_COURSE_WARCHIEF, GOSSIP_MENU_OPTION_ID_ALL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);
                    player->SEND_GOSSIP_MENU(GOSSIP_NOW_PAY_ATTENTION, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF + 7:
                    player->CLOSE_GOSSIP_MENU();
                    player->AreaExploredOrEventHappens(QUEST_WHAT_THE_WIND_CARRIES);
                    break;
            }
            return true;
        }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            if (creature->IsQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());

            if (player->GetQuestStatus(QUEST_WHAT_THE_WIND_CARRIES) == QUEST_STATUS_INCOMPLETE)
                player->ADD_GOSSIP_ITEM_DB(OPTION_PLEASE_SHARE_YOUR, GOSSIP_MENU_OPTION_ID_ALL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

            player->SEND_GOSSIP_MENU(GOSSIP_MEMBERS_OF_THE_HORDE, creature->GetGUID());
            return true;
        }       

        bool OnQuestReward(Player* player, Creature* creature, Quest const* quest, uint32 /*opt*/) override
        {
            switch (quest->GetQuestId())
            {
                case QUEST_FOR_THE_HORDE:
                    creature->AI()->DoAction(ACTION_START_EVENT);
                    break;
                case QUEST_MESSAGE_TO_THRALL:
                    creature->AI()->DoAction(ACTION_START_MESSAGE);
                    break;
                case QUEST_WARCHIEFS_BLESSING:
                    creature->AI()->DoAction(ACTION_START_BLESSING);
                    break;
                case QUEST_RUBY_PRE_EVENT:
                {
                    const uint32 CITY_GUARD = 1010791;
                    if (creature->FindNearestCreature(CITY_GUARD, 30.f))
                        break;
                    float x, y, z;
                    creature->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE, 20.f, 0.1f);
                    if (auto guard = creature->SummonCreature(CITY_GUARD, x, y, z, 5.f, TEMPSUMMON_TIMED_DESPAWN, 15 * IN_MILLISECONDS))
                        guard->AI()->SetData(0, 0);
                    creature->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE, 20.f, -0.1f);
                    if (auto guard = creature->SummonCreature(CITY_GUARD, x, y, z, 5.f, TEMPSUMMON_TIMED_DESPAWN, 15 * IN_MILLISECONDS))
                        guard->AI()->SetData(0, 1);
                }
                default:
                    break;
            }

            return false;
        }

        bool OnQuestAccept(Player* player, Creature* /*creature*/, Quest const* quest) override
        { 
            switch (quest->GetQuestId())
            {
                case QUEST_RUBY_PRE_EVENT_2:
                {
                    player->m_Events.AddEvent(new DelayedQuestCompleteEvent(*player, QUEST_RUBY_PRE_EVENT_2), player->m_Events.CalculateTime(25 * IN_MILLISECONDS));
                    break;
                }
                default:
                    break;
            }

            return false; 
        }

        struct npc_thrall_warchief_rgAI : public ScriptedAI
        {
            npc_thrall_warchief_rgAI(Creature* creature) : ScriptedAI(creature) { }

            void DoAction(int32 action) override
            {
                if (action == ACTION_START_EVENT)
                    events.ScheduleEvent(EVENT_FOR_THE_HORDE_1, 1 * IN_MILLISECONDS);

                if (action == ACTION_START_MESSAGE)
                    events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_1, 1 * IN_MILLISECONDS);

                if (action == ACTION_START_BLESSING)
                    events.ScheduleEvent(EVENT_BLESSING_1, 1 * IN_MILLISECONDS);
            }

            void EnterCombat(Unit* /*who*/) override
            {
                events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, 2 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SHOCK,           8 * IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_FOR_THE_HORDE_1:
                            me->setActive(true);
                            me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                            events.ScheduleEvent(EVENT_FOR_THE_HORDE_2, 1 * IN_MILLISECONDS);
                            break;
                        case EVENT_FOR_THE_HORDE_2:
                            if (GameObject * skull = me->FindNearestGameObject(GO_UNDORNED_STAKE, 50.0f))
                                skull->UseDoorOrButton();
                            events.ScheduleEvent(EVENT_FOR_THE_HORDE_3, 1 * IN_MILLISECONDS);
                            break;
                        case EVENT_FOR_THE_HORDE_3:
                            Talk(SAY_HORDE_1);
                            events.ScheduleEvent(EVENT_FOR_THE_HORDE_4, 5 * IN_MILLISECONDS);
                            break;
                        case EVENT_FOR_THE_HORDE_4:
                            DoCastSelf(SPELL_WARCHIEFS_BLESSING);
                            events.ScheduleEvent(EVENT_FOR_THE_HORDE_5, 3 * IN_MILLISECONDS);
                            break;
                        case EVENT_FOR_THE_HORDE_5:
                            Talk(SAY_HORDE_2);
                            events.ScheduleEvent(EVENT_FOR_THE_HORDE_6, 4 * IN_MILLISECONDS);
                            break;
                        case EVENT_FOR_THE_HORDE_6:
                            me->setActive(false);
                            me->HandleEmoteCommand(EMOTE_STATE_SIT_CHAIR_MED);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_1:
                            me->setActive(true);
                            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_STAND);
                            me->SetByteValue(UNIT_FIELD_BYTES_1, 0, UNIT_STAND_STATE_STAND);
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_2, 2 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_2:
                            Talk(SAY_HORDE_3);
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_3, 1 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_3:
                            me->GetMotionMaster()->MovePoint(1, 1923.52f, -4136.32f, 40.574f);
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_4, 2 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_4:
                            Talk(SAY_HORDE_4);
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_5, 4 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_5:
                            me->SetSheath(SHEATH_STATE_UNARMED);
                            me->SetStandState(UNIT_STAND_STATE_KNEEL);
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_6, 2 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_6:
                            Talk(SAY_HORDE_5);
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_7, 1 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_7:
                            if (Creature* etrigg = me->FindNearestCreature(NPC_ETRIGG, 100.0f))
                                etrigg->GetMotionMaster()->MovePoint(1, 1923.41f, -4138.83f, 40.608f);
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_8, 5 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_8:
                            if (Creature* etrigg = me->FindNearestCreature(NPC_ETRIGG, 100.0f))
                                etrigg->SetFacingToObject(me);
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_9, 2 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_9:
                            if (Creature* etrigg = me->FindNearestCreature(NPC_ETRIGG, 100.0f))
                                etrigg->AI()->Talk(SAY_ETRRIGG_0);
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_10, 5 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_10:
                            Talk(SAY_HORDE_6);
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_11, 5 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_11:
                            if (Creature* etrigg = me->FindNearestCreature(NPC_ETRIGG, 100.0f))
                            {
                                etrigg->SetSheath(SHEATH_STATE_UNARMED);
                                etrigg->SetStandState(UNIT_STAND_STATE_KNEEL);
                            }
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_12, 5 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_12:
                            if (Creature* etrigg = me->FindNearestCreature(NPC_ETRIGG, 100.0f))
                                etrigg->AI()->Talk(SAY_ETRRIGG_1);
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_13, 5 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_13:
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_STAND);
                            me->GetMotionMaster()->MovePoint(1, 1920.33f, -4126.62f, 43.1443f);
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_14, 6 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_14:
                            me->SetFacingTo(4.86366f);
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_15, 3 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_15:
                            Talk(SAY_HORDE_7);
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_16, 5 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_16:
                            if (Creature* etrigg = me->FindNearestCreature(NPC_ETRIGG, 100.0f))
                                etrigg->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_STAND);
                            me->GetMotionMaster()->MovePoint(1, 1923.25f, -4136.01f, 40.5651f);
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_17, 4 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_17:
                            if (Creature* etrigg = me->FindNearestCreature(NPC_ETRIGG, 100.0f))
                            {
                                etrigg->AI()->Talk(SAY_ETRRIGG_2);
                                etrigg->SetByteValue(UNIT_FIELD_BYTES_1, 0, UNIT_STAND_STATE_STAND);
                            }
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_18, 2 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_18:
                            Talk(SAY_HORDE_8);
                            if (Creature* etrigg = me->FindNearestCreature(NPC_ETRIGG, 100.0f))
                                etrigg->GetMotionMaster()->MovePoint(1, 1905.75f, -4161.37f, 41.0035f);
                            me->GetMotionMaster()->MovePoint(1, 1920.01f, -4124.28f, 43.6129f);
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_19, 3 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_19:
                            me->SetFacingTo(4.79965f);
                            me->SetByteValue(UNIT_FIELD_BYTES_1, 0, UNIT_STAND_STATE_SIT_MEDIUM_CHAIR);
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_SIT_CHAIR_MED);
                            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);                           
                            events.ScheduleEvent(EVENT_MESSAGE_TO_THRALL_20, 10 * IN_MILLISECONDS);
                            break;
                        case EVENT_MESSAGE_TO_THRALL_20:
                            if (Creature* etrigg = me->FindNearestCreature(NPC_ETRIGG, 100.0f))
                                etrigg->SetFacingTo(0.488692f);
                            me->setActive(false);
                            break;
                        case EVENT_BLESSING_1:
                            Talk(SAY_BLESSING_1);
                            events.ScheduleEvent(EVENT_BLESSING_2, 4 * IN_MILLISECONDS);
                            break;
                        case EVENT_BLESSING_2:
                            Talk(SAY_BLESSING_2);
                            events.ScheduleEvent(EVENT_BLESSING_3, 9 * IN_MILLISECONDS);
                            break;
                        case EVENT_BLESSING_3:
                            Talk(SAY_BLESSING_3);
                            events.ScheduleEvent(EVENT_BLESSING_4, 6 * IN_MILLISECONDS);
                            break;
                        case EVENT_BLESSING_4:
                            Talk(SAY_BLESSING_4);
                            break;
                        default:
                            break;
                    }
                }

                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CHAIN_LIGHTNING:
                            DoCastVictim(SPELL_CHAIN_LIGHTNING);
                            events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, 9 * IN_MILLISECONDS);
                            break;
                        case EVENT_SHOCK:
                            DoCastVictim(SPELL_SHOCK);
                            events.ScheduleEvent(EVENT_SHOCK, 15 * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_thrall_warchief_rgAI(creature);
    }
};

void AddSC_orgrimmar_rg()
{
    new npc_thrall_warchief_rg();
}
