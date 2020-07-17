/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * Copyright (C) 2010-2017 Rising Gods <http://www.rising-gods.de/>
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
#include "Vehicle.h"

enum FrostbroodSpawnEvent
{
    NPC_FROSTBROOD_SPAWN = 31702,
    NPC_INV_TRIGGER      = 31245,
    NPC_SPAWN_TRIGGER    = 27047,
    NPC_TARGET_TRIGGER   = 15384,
    NPC_WYRM_REANIMATOR  = 31731,

    EVENT_SEARCH_TARGET  = 1,
    EVENT_FLY_ABROAD     = 2,
    EVENT_BECOME_MURDER  = 3,
};

class npc_frostbrood_spawn : public CreatureScript
{
public:
    npc_frostbrood_spawn() : CreatureScript("npc_frostbrood_spawn") { }

    struct npc_frostbrood_spawnAI : public ScriptedAI
    {
        npc_frostbrood_spawnAI(Creature* creature) : ScriptedAI(creature) { }

        void InitializeAI() override
        {
            if (me->FindNearestCreature(NPC_SPAWN_TRIGGER, 15.0f))
                me->DespawnOrUnsummon(120 * IN_MILLISECONDS);
            Reset();
        }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_SEARCH_TARGET, 8 * IN_MILLISECONDS);
        }

        void MovementInform(uint32 /*type*/, uint32 id) override
        {
            if (id == 1)
            {
                if (Creature * target = me->FindNearestCreature(NPC_WYRM_REANIMATOR, 10.0f))
                {
                    target->EnterVehicle(me, 0);
                    target->DespawnOrUnsummon(14 * IN_MILLISECONDS);
                    _events.ScheduleEvent(EVENT_FLY_ABROAD, 5 * IN_MILLISECONDS);
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SEARCH_TARGET:
                        if (Creature* trigger = me->FindNearestCreature(NPC_TARGET_TRIGGER, 50.0f))
                        {
                            if (Creature* target = trigger->FindNearestCreature(NPC_WYRM_REANIMATOR, 10.0f))
                            {
                                me->GetMotionMaster()->MovePoint(1, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());
                                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                                me->SetReactState(REACT_PASSIVE);
                                me->setActive(true);

                                target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                                target->SetReactState(REACT_PASSIVE);
                                target->setActive(true);
                            }

                            else
                                me->DespawnOrUnsummon(15 * IN_MILLISECONDS);
                        }
                        break;
                    case EVENT_FLY_ABROAD:
                        me->SetCanFly(true);
                        me->SetDisableGravity(true);
                        me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                        me->SetSpeedRate(MOVE_FLIGHT, 4.0f);

                        me->GetNearPoint(me, x, y, z, 1, 100, float(M_PI * 2 * rand_norm()));
                        me->GetMotionMaster()->MovePoint(2, x, y, z + float(urand(55, 80)));

                        _events.ScheduleEvent(EVENT_BECOME_MURDER, 8 * IN_MILLISECONDS);
                        break;
                    case EVENT_BECOME_MURDER:
                        if (me->IsVehicle() || me->GetVehicleKit())
                        {
                            if (Unit* target = me->GetVehicleKit()->GetPassenger(0))
                            {
                                target->SetVisible(false);
                                target->KillSelf();

                                me->DespawnOrUnsummon(1 * IN_MILLISECONDS);
                            }
                        }
                        break;
                    default:
                        break;
                }
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        float x, y, z;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_frostbrood_spawnAI(creature);
    }
};

void AddSC_area_sindragosas_fall_rg()
{
    new npc_frostbrood_spawn();
}
