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

/* ScriptData
SDName: Boss Mr.Smite
SD%Complete:
SDComment: Timers and say taken from acid script
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "deadmines.h"

enum Spels
{
    SPELL_SMITE_STOMP            = 6432,
    SPELL_SMITE_SLAM             = 6435,

    EQUIP_SWORD                  = 1,
    EQUIP_TWO_SWORDS             = 2,
    EQUIP_MACE                   = 3,

    EVENT_CHECK_HEALTH1          = 1,
    EVENT_CHECK_HEALTH2          = 2,
    EVENT_SMITE_SLAM             = 3,
    EVENT_SWAP_WEAPON1           = 4,
    EVENT_SWAP_WEAPON2           = 5,
    EVENT_RESTORE_COMBAT         = 6,
    EVENT_KNEEL                  = 7,

    SAY_SWAP1                    = 2,
    SAY_SWAP2                    = 3
};

class boss_mr_smite : public CreatureScript
{
    public:
        boss_mr_smite() : CreatureScript("boss_mr_smite") { }
        
        struct boss_mr_smiteAI : public ScriptedAI
        {
            boss_mr_smiteAI(Creature* creature) : ScriptedAI(creature) { }
            
            void Reset() override
            {
                me->LoadEquipment(EQUIP_SWORD);
                me->SetCanDualWield(false);
                me->SetStandState(UNIT_STAND_STATE_STAND);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
                me->SetReactState(REACT_AGGRESSIVE);
            }

            void EnterCombat(Unit* /*who*/) override
            {
                events.ScheduleEvent(EVENT_CHECK_HEALTH1, 500);
                events.ScheduleEvent(EVENT_CHECK_HEALTH2, 500);
                events.ScheduleEvent(EVENT_SMITE_SLAM, 3000);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                switch (events.ExecuteEvent())
                {
                    case EVENT_SMITE_SLAM:
                        me->CastSpell(me->GetVictim(), SPELL_SMITE_SLAM, false);
                        events.ScheduleEvent(EVENT_SMITE_SLAM, 15000);
                        break;
                    case EVENT_CHECK_HEALTH1:
                        if (me->HealthBelowPct(67))
                        {
                            DoCastSelf(SPELL_SMITE_STOMP, false);
                            events.DelayEvents(10000);
                            me->GetMotionMaster()->Clear();
                            me->GetMotionMaster()->MovePoint(EQUIP_TWO_SWORDS, 1.859f, -780.72f, 9.831f);
                            Talk(SAY_SWAP1);
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
                            me->SetReactState(REACT_PASSIVE);
                            break;
                        }
                        events.ScheduleEvent(EVENT_CHECK_HEALTH1, 500);
                        break;
                    case EVENT_CHECK_HEALTH2:
                        if (me->HealthBelowPct(34))
                        {
                            DoCastSelf(SPELL_SMITE_STOMP, false);
                            events.DelayEvents(10000);
                            me->GetMotionMaster()->Clear();
                            me->GetMotionMaster()->MovePoint(EQUIP_MACE, 1.859f, -780.72f, 9.831f);
                            Talk(SAY_SWAP2);
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
                            me->SetReactState(REACT_PASSIVE);
                            break;
                        }
                        events.ScheduleEvent(EVENT_CHECK_HEALTH2, 500);
                        break;
                    case EVENT_SWAP_WEAPON1:
                        me->LoadEquipment(EQUIP_TWO_SWORDS);
                        me->SetCanDualWield(true);
                        break;
                    case EVENT_SWAP_WEAPON2:
                        me->LoadEquipment(EQUIP_MACE);
                        me->SetCanDualWield(false);
                        break;
                    case EVENT_RESTORE_COMBAT:
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
                        me->SetStandState(UNIT_STAND_STATE_STAND);
                        if (me->GetVictim())
                        {
                            me->GetMotionMaster()->MoveChase(me->GetVictim());
                            me->SetTarget(me->GetVictim()->GetGUID());
                        }
                        break;
                    case EVENT_KNEEL:
                        me->SendMeleeAttackStop(me->GetVictim());
                        me->SetStandState(UNIT_STAND_STATE_KNEEL);
                        break;
                }

                DoMeleeAttackIfReady();
            }

            void MovementInform(uint32 type, uint32 point) override
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                me->SetTarget(ObjectGuid::Empty);
                me->SetFacingTo(5.558f);
                me->SetStandState(UNIT_STAND_STATE_KNEEL);
                events.ScheduleEvent(point == EQUIP_TWO_SWORDS ? EVENT_SWAP_WEAPON1 : EVENT_SWAP_WEAPON2, 1500);
                events.ScheduleEvent(EVENT_RESTORE_COMBAT, 3000);
                events.ScheduleEvent(EVENT_KNEEL, 0);
            }

        private:
            EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<boss_mr_smiteAI>(creature);
    }
};

void AddSC_boss_mr_smite()
{
    new boss_mr_smite();
}
