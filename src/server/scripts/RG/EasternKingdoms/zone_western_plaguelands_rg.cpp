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

/*####
 ## Quest 5096: Scarlet Diversions
 ## npc_scarlet_diversions_trigger
 ####*/

 enum QuestScarletDiversionsMisc
 {
     // Gameobjects
     GO_COMMAND_TENT                 = 176210,
     GO_SCARLET_CRUSADE_FORWARD_CAMP = 176088
 };

class npc_scarlet_diversions_trigger : public CreatureScript
{
    public:
        npc_scarlet_diversions_trigger() : CreatureScript("npc_scarlet_diversions_trigger") { }

    struct npc_scarlet_diversions_triggerAI : public ScriptedAI
    {
        npc_scarlet_diversions_triggerAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            checkTimer = 1 * IN_MILLISECONDS;
        }

        void UpdateAI(uint32 diff) override
        {
            if (checkTimer <= diff)
            {
                if (GameObject* tent = me->FindNearestGameObject(GO_COMMAND_TENT, 20.0f))
                {
                    if (tent->GetGoState() == GO_STATE_ACTIVE) // Tent is burned down
                    {
                        if (!me->FindNearestGameObject(GO_SCARLET_CRUSADE_FORWARD_CAMP, 20.0f))
                            me->SummonGameObject(GO_SCARLET_CRUSADE_FORWARD_CAMP, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
                    }
                    else
                    {
                        if (GameObject* spellfocus = me->FindNearestGameObject(GO_SCARLET_CRUSADE_FORWARD_CAMP, 20.0f))
                            spellfocus->Delete();
                    }
                }

                checkTimer = urand(1000, 1500);
            }
            else
                checkTimer -= diff;
        }
        
        private:
            uint32 checkTimer;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_scarlet_diversions_triggerAI(creature);
    }
};

void AddSC_western_plaguelands_rg()
{
    new npc_scarlet_diversions_trigger();
}
