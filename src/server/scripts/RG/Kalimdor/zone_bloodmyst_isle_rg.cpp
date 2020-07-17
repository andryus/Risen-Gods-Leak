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
#include "ScriptedEscortAI.h"
#include "Player.h"
#include "Group.h"

enum Spells
{
    SPELL_FLAMEBREATH                     = 8873,
    SPELL_SWIPE                           = 31279,
    SPELL_TERRIFYING_ROAR                 = 14100,
};

enum Events
{
    EVENT_FLAMEBREATH                     = 1,
    EVENT_SWIPE                           = 2,
    EVENT_TERRIFYING_ROAR                 = 3,
};

const Position RazormawFly[7] =
{
    {-1244.46f, -12490.5f, 111.481f},
    {-1259.88f, -12459.4f, 110.485f},
    {-1171.06f, -12429.4f, 114.808f},
    {-1189.19f, -12504.3f, 111.894f},
    {-1209.29f, -12499.7f, 110.000f},
    {-1209.96f, -12481.4f, 101.971f},
    {-1210.65f, -12469.6f, 94.713f}
};

class npc_razormaw : public CreatureScript
{
public:
    npc_razormaw() : CreatureScript("npc_razormaw") { }

    struct npc_razormawAI : public ScriptedAI
    {
        npc_razormawAI(Creature* creature) : ScriptedAI(creature) 
        {
           waypointReached = false;
           movePhase = 0;
           introDone = false;
           me->SetReactState(REACT_PASSIVE);
           me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
           me->SetCanFly(true);
           me->SetDisableGravity(false);
           me->GetMotionMaster()->MovePoint(0, RazormawFly[0]);
        }

        void EnterCombat(Unit* /*who*/) override
        {
          events.ScheduleEvent(EVENT_FLAMEBREATH,     26 * IN_MILLISECONDS);
          events.ScheduleEvent(EVENT_SWIPE,            6 * IN_MILLISECONDS);
          events.ScheduleEvent(EVENT_TERRIFYING_ROAR, 12 * IN_MILLISECONDS);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            switch(id)
            {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                    movePhase++;
                    waypointReached = true;
                    break;
                case 6:
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->SetCanFly(false);
                    me->SetDisableGravity(true);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                    introDone = true;
                    break;
             }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!introDone && waypointReached)
            {
                if (movePhase <= 5)
                    me->GetMotionMaster()->MovePoint(movePhase, RazormawFly[movePhase]);
                else
                    me->GetMotionMaster()->MoveLand(movePhase, RazormawFly[movePhase]);
                waypointReached = false;
            }
           
           if (!UpdateVictim() || !introDone)
              return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_FLAMEBREATH:
                        events.ScheduleEvent(EVENT_FLAMEBREATH, 30 * IN_MILLISECONDS);
                        DoCastVictim(SPELL_FLAMEBREATH);
                        break;
                    case EVENT_SWIPE:
                        events.ScheduleEvent(EVENT_SWIPE, 9 * IN_MILLISECONDS);
                        DoCastVictim(SPELL_SWIPE);
                        break;
                    case EVENT_TERRIFYING_ROAR:
                        events.ScheduleEvent(EVENT_TERRIFYING_ROAR, 12 * IN_MILLISECONDS);
                        DoCastVictim(SPELL_TERRIFYING_ROAR);
                        break;
                }
            }
            
            DoMeleeAttackIfReady();
        }

        private:
            bool waypointReached;
            uint32 movePhase;
            bool introDone;
            EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_razormawAI(creature);
    }
};

void AddSC_bloodmyst_isle_rg()
{
    new npc_razormaw();
}
