/*
* Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
* Copyright (C) 2014-2016 Rising Gods <http://www.rising-gods.de/>
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

enum DruidSpells
{
    SPELL_DRUID_DRAMBLES_RANK_1     = 16836,
    SPELL_DRUID_DRAMBLES_RANK_2     = 16839,
    SPELL_DRUID_DRAMBLES_RANK_3     = 16840,
    SPELL_DRUID_TREANT_DAZE         = 50411
};

enum DruidEvents
{
    EVENT_DRUID_SELECT_TARGET       = 1
};

class npc_pet_druid_treant : public CreatureScript
{
    public:
        npc_pet_druid_treant() : CreatureScript("npc_pet_druid_treant") { }

        struct npc_pet_druid_treantAI : public ScriptedAI
        {
            npc_pet_druid_treantAI(Creature* creature) : ScriptedAI(creature) { }

            void InitializeAI() override
            {
                if (Player* player = me->GetOwner() ? me->GetOwner()->ToPlayer() : nullptr)
                    if (Unit* target = player->GetSelectedUnit())
                        if (me->IsValidAttackTarget(target))
                            AttackStart(target);

                if (Unit* owner = me->GetOwner())
                {
                    if (owner->HasAura(SPELL_DRUID_DRAMBLES_RANK_1))
                        modifier = 5.0f;
                    else if (owner->HasAura(SPELL_DRUID_DRAMBLES_RANK_2))
                        modifier = 10.0f;
                    else if (owner->HasAura(SPELL_DRUID_DRAMBLES_RANK_3))
                        modifier = 15.0f;
                    else
                        modifier = 0.0f;
                }

                _events.ScheduleEvent(EVENT_DRUID_SELECT_TARGET, 0);
                me->SetInCombatWithZone();
            }

            void Reset() override
            {
                if (Player* player = me->GetOwner() ? me->GetOwner()->ToPlayer() : nullptr)
                    if (Unit* target = player->GetSelectedUnit())
                        if (me->IsValidAttackTarget(target))
                            AttackStart(target);
            }

            void DoMeleeAttackIfReady()
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (Unit* victim = me->GetVictim())
                {
                    if (me->isAttackReady() && me->IsWithinMeleeRange(victim))
                    {
                        me->AttackerStateUpdate(victim);
                        me->resetAttackTimer();
                        uint16 chance = urand(0, 99);
                        if (chance < modifier)
                            DoCastVictim(SPELL_DRUID_TREANT_DAZE);
                    }
                }
            }

            void UpdateAI(uint32 /*diff*/) override
            {
                if (!UpdateVictim())
                    return;

                if (me->GetVictim()->HasBreakableByDamageCrowdControlAura(me))
                {
                    me->InterruptNonMeleeSpells(false);
                    return;
                }

                if (_events.ExecuteEvent() == EVENT_DRUID_SELECT_TARGET)
                {
                    if (Player* player = me->GetOwner() ? me->GetOwner()->ToPlayer() : nullptr)
                        if (Unit* target = player->GetSelectedUnit())
                            if (me->IsValidAttackTarget(target))
                                AttackStart(target);
                    _events.ScheduleEvent(EVENT_DRUID_SELECT_TARGET, 1 * IN_MILLISECONDS);
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
            float modifier;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pet_druid_treantAI(creature);
        }
};

void AddSC_druid_pet_scripts()
{
    new npc_pet_druid_treant();
}
