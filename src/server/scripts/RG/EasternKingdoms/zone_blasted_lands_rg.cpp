/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

enum ServantOfRazelikhMisc
{
    // Spells
    SPELL_GHOST_SHOCK     = 10794,

    // Gameobjects
    GO_STONE_OF_BINDING_1 = 141812,
    GO_STONE_OF_BINDING_2 = 141857, 
    GO_STONE_OF_BINDING_3 = 141858, 
    GO_STONE_OF_BINDING_4 = 141859,

    // Texts
    SAY_LOW_HP            = 0
};

class npc_servant_of_razelikh : public CreatureScript
{
    public:
        npc_servant_of_razelikh() : CreatureScript("npc_servant_of_razelikh") { }

        struct npc_servant_of_razelikhAI : public ScriptedAI
        {
            npc_servant_of_razelikhAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                below_5_percent = false;
            }

            void DamageTaken(Unit* /*who*/, uint32& damage) override
            {
                if (me->HealthBelowPctDamaged(5, damage) && !below_5_percent)
                {
                    below_5_percent = true;
                    damage = 0;
                    me->InterruptNonMeleeSpells(false);
                    me->AddAura(SPELL_GHOST_SHOCK, me);
                    Talk(SAY_LOW_HP);
                    
                    std::list<GameObject*> StoneList;
                    me->GetGameObjectListWithEntryInGrid(StoneList, GO_STONE_OF_BINDING_1, 30.0f);
                    me->GetGameObjectListWithEntryInGrid(StoneList, GO_STONE_OF_BINDING_2, 30.0f);
                    me->GetGameObjectListWithEntryInGrid(StoneList, GO_STONE_OF_BINDING_3, 30.0f);
                    me->GetGameObjectListWithEntryInGrid(StoneList, GO_STONE_OF_BINDING_4, 30.0f);
                    for (std::list<GameObject*>::const_iterator itr = StoneList.begin(); itr != StoneList.end(); ++itr)
                        if (GameObject* stone = (*itr))
                            stone->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE | GO_FLAG_INTERACT_COND);
                }

                if (damage > me->GetHealth() && me->HasAura(SPELL_GHOST_SHOCK))
                    damage = 0;
            }
            
        private:
            bool below_5_percent;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_servant_of_razelikhAI(creature);
    }
};

void AddSC_blasted_lands_rg()
{
    new npc_servant_of_razelikh();
}
