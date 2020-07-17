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

/*####
## npc_varian_wrynn_sw
###*/

enum VarianMisc
{
    // Quests
    QUEST_THE_KILLING_TIME      = 13371,
    QUEST_WHERE_KINGS_WALK      = 13188,
    QUEST_RUBY_PRE_EVENT        = 110200,
    QUEST_RUBY_PRE_EVENT_2      = 110197,

    // Gameobjects
    GO_PORTAL_TO_UC             = 193425,

    // Spells
    SPELL_HEROIC_LEAP           = 59688,
    SPELL_WHIRLWIND             = 41056,

    // Events
    EVENT_HEROIC_LEAP           = 1,
    EVENT_WHIRLWIND             = 2,
    EVENT_WHERE_KINGS_WALK_1    = 3,
    EVENT_WHERE_KINGS_WALK_2    = 4,
    EVENT_WHERE_KINGS_WALK_3    = 5,
    EVENT_WHERE_KINGS_WALK_4    = 6,

    // Actions
    ACTION_START_KINGS_WALK     = 1,

    // Texts
    SAY_WHERE_KINGS_WALK_1      = 0,
    SAY_WHERE_KINGS_WALK_2      = 1,
    SAY_WHERE_KINGS_WALK_3      = 2,
    SAY_WHERE_KINGS_WALK_4      = 3,
};

#define POS_PORTAL_TO_UC        -8449.009f, 323.303f, 121.329f

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

class npc_varian_wrynn_sw : public CreatureScript
{
    public:
        npc_varian_wrynn_sw() : CreatureScript("npc_varian_wrynn_sw") { }

        bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest)
        {
            
            switch (quest->GetQuestId())
            {
                case QUEST_THE_KILLING_TIME:
                    creature->SummonGameObject(GO_PORTAL_TO_UC, POS_PORTAL_TO_UC, 0, 0, 0, 0, 0, 60 * IN_MILLISECONDS);
                    break;
                case QUEST_RUBY_PRE_EVENT_2:
                    player->m_Events.AddEvent(new DelayedQuestCompleteEvent(*player, QUEST_RUBY_PRE_EVENT_2), player->m_Events.CalculateTime(25 * IN_MILLISECONDS));
                    break;
                default:
                    break;
            }

            return true;
        }

        bool OnQuestReward(Player* player, Creature* creature, Quest const* quest, uint32 /*opt*/) override
        {
            switch (quest->GetQuestId())
            {
                case QUEST_WHERE_KINGS_WALK:
                    creature->AI()->DoAction(ACTION_START_KINGS_WALK);
                    break;
                case QUEST_RUBY_PRE_EVENT:
                {
                    const uint32 CITY_GUARD = 1010790;
                    if (creature->FindNearestCreature(CITY_GUARD, 30.f))
                        break;
                    float x, y, z;
                    creature->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE, 20.f, 0.1f);
                    if (auto guard = creature->SummonCreature(CITY_GUARD, x, y, z, 5.f, TEMPSUMMON_TIMED_DESPAWN, 15 * IN_MILLISECONDS))
                        guard->AI()->SetData(0, 0);
                    creature->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE, 20.f, -0.1f);
                    if (auto guard = creature->SummonCreature(CITY_GUARD, x, y, z, 5.f, TEMPSUMMON_TIMED_DESPAWN, 15 * IN_MILLISECONDS))
                        guard->AI()->SetData(0, 1);
                    break;
                }
                default:
                    break;
            }

            return false;
        }

        struct npc_varian_wrynn_swAI : public ScriptedAI
        {
            npc_varian_wrynn_swAI(Creature* creature) : ScriptedAI(creature) { }

            void DoAction(int32 action) override
            {
                if (action == ACTION_START_KINGS_WALK)
                    events.ScheduleEvent(EVENT_WHERE_KINGS_WALK_1, 1 * IN_MILLISECONDS);
            }

            void Reset() override
            {
                events.Reset();
            }

            void EnterCombat(Unit* /*who*/) override
            {
                events.ScheduleEvent(EVENT_HEROIC_LEAP, 10 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_WHIRLWIND,   12 * IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_WHERE_KINGS_WALK_1:
                            Talk(SAY_WHERE_KINGS_WALK_1);
                            events.ScheduleEvent(EVENT_WHERE_KINGS_WALK_2, 4 * IN_MILLISECONDS);
                            break;
                        case EVENT_WHERE_KINGS_WALK_2:
                            Talk(SAY_WHERE_KINGS_WALK_2);
                            events.ScheduleEvent(EVENT_WHERE_KINGS_WALK_3, 9 * IN_MILLISECONDS);
                            break;
                        case EVENT_WHERE_KINGS_WALK_3:
                            Talk(SAY_WHERE_KINGS_WALK_3);
                            events.ScheduleEvent(EVENT_WHERE_KINGS_WALK_4, 6 * IN_MILLISECONDS);
                            break;
                        case EVENT_WHERE_KINGS_WALK_4:
                            Talk(SAY_WHERE_KINGS_WALK_4);
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
                        case EVENT_HEROIC_LEAP:
                        {
                            Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 25.0f, true);
                            if (target)
                            {
                                me->CastSpell(target, SPELL_HEROIC_LEAP, false);
                                me->JumpTo(target, 4.0f);
                            }
                            me->resetAttackTimer();
                            events.ScheduleEvent(EVENT_HEROIC_LEAP, 45 * IN_MILLISECONDS);
                            break;
                        }
                        case EVENT_WHIRLWIND:
                            DoCastVictim(SPELL_WHIRLWIND);                            
                            me->resetAttackTimer();
                            events.ScheduleEvent(EVENT_WHIRLWIND, 12 * IN_MILLISECONDS);
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
            return new npc_varian_wrynn_swAI(creature);
        }
};

void AddSC_stormwind_city_rg()
{
    new npc_varian_wrynn_sw();
}
