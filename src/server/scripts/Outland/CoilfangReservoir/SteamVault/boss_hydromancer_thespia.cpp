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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "steam_vault.h"

enum Yells
{
    SAY_SUMMON = 0,
    SAY_AGGRO = 1,
    SAY_SLAY = 2,
    SAY_DEAD = 3,
};

enum Spells
{
    SPELL_LIGHTNING_CLOUD  = 25033,
    SPELL_LUNG_BURST       = 31481,
    SPELL_ENVELOPING_WINDS = 31718
};

enum Events
{
    EVENT_LIGHTNING_CLOUD  = 1,
    EVENT_LUNG_BURST,
    EVENT_ENVELOPING_WINDS,
    EVENT_CHECK_HOME
};

enum Creatures
{
    NPC_ADD_WATER_ELEMENTAL   = 17917
};

class boss_hydromancer_thespia : public CreatureScript
{
    public:
        boss_hydromancer_thespia() : CreatureScript("boss_hydromancer_thespia") { }

        struct boss_thespiaAI : public BossAI
        {
            boss_thespiaAI(Creature* creature) : BossAI(creature, DATA_HYDROMANCER_THESPIA) { }

            void JustDied(Unit* killer) override
            {
                Talk(SAY_DEAD);
                BossAI::JustDied(killer);
            }

            void KilledUnit(Unit* who) override
            {
                if (who->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustReachedHome() override
            {
                BossAI::JustReachedHome();
                me->SummonCreature(NPC_ADD_WATER_ELEMENTAL, 90.0419f, -326.115f, -7.78746f, 3.00197f, TEMPSUMMON_DEAD_DESPAWN, 60000);
                me->SummonCreature(NPC_ADD_WATER_ELEMENTAL, 91.3224f, -306.508f, -7.78735f, 3.21141f, TEMPSUMMON_DEAD_DESPAWN, 60000);
            }

            void EnterCombat(Unit* who) override
            {
                Talk(SAY_AGGRO);
                BossAI::EnterCombat(who);

                events.ScheduleEvent(EVENT_LIGHTNING_CLOUD, 15000);
                events.ScheduleEvent(EVENT_LUNG_BURST, 7000);
                events.ScheduleEvent(EVENT_ENVELOPING_WINDS, 9000);
                events.ScheduleEvent(EVENT_CHECK_HOME, 2000);
            }

            void ExecuteEvent(uint32 eventId) override
            {
                switch (eventId)
                {
                    case EVENT_LIGHTNING_CLOUD:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30.0f, true))
                            DoCast(target, SPELL_LIGHTNING_CLOUD);
                        // cast twice in Heroic mode
                        if (IsHeroic())
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30.0f, true))
                                DoCast(target, SPELL_LIGHTNING_CLOUD);
                        events.ScheduleEvent(EVENT_LIGHTNING_CLOUD, urand(15000, 25000));
                        break;
                    case EVENT_LUNG_BURST:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true))
                            DoCast(target, SPELL_LUNG_BURST);
                        events.ScheduleEvent(EVENT_LUNG_BURST, urand(7000, 12000));
                        break;
                    case EVENT_ENVELOPING_WINDS:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 35.0f, true))
                            DoCast(target, SPELL_ENVELOPING_WINDS);
                        // cast twice in Heroic mode
                        if (IsHeroic())
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 35.0f, true))
                                DoCast(target, SPELL_ENVELOPING_WINDS);

                        events.ScheduleEvent(EVENT_ENVELOPING_WINDS, urand(10000, 15000));
                        break;
                    case EVENT_CHECK_HOME:
                        if (me->GetDistance(me->GetHomePosition()) > 80.0f)
                            EnterEvadeMode();
                        events.ScheduleEvent(EVENT_CHECK_HOME, 2000);
                        break;
                    default:
                        break;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_thespiaAI>(creature);

        }
};

enum CoilfangWaterElemental
{
    EVENT_WATER_BOLT_VOLLEY = 1,
    SPELL_WATER_BOLT_VOLLEY = 34449
};

class npc_coilfang_waterelemental : public CreatureScript
{
    public:
        npc_coilfang_waterelemental() : CreatureScript("npc_coilfang_waterelemental") { }

        struct npc_coilfang_waterelementalAI : public ScriptedAI
        {
            npc_coilfang_waterelementalAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                _events.Reset();
            }

            void EnterCombat(Unit* /*who*/) override
            {
                _events.ScheduleEvent(EVENT_WATER_BOLT_VOLLEY, urand(3000, 6000));
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_WATER_BOLT_VOLLEY:
                            DoCastSelf(SPELL_WATER_BOLT_VOLLEY);
                            _events.ScheduleEvent(EVENT_WATER_BOLT_VOLLEY, urand(7000, 12000));
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

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_coilfang_waterelementalAI(creature);
        }
};

void AddSC_boss_hydromancer_thespia()
{
    new boss_hydromancer_thespia();
    new npc_coilfang_waterelemental();
}
