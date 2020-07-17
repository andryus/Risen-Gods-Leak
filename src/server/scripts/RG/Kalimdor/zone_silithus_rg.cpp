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

/*###
## npc_prince_thunderaan
###*/

enum ThunderfuryMisc
{
    // Items
    ITEM_THUNDERFURY                    = 19019,
    ITEM_VESSEL_OF_REBIRTH              = 19016,
    
    // Quests
    QUEST_THUNDERAAN_THE_WINDSEEKER     = 7786,
    
    // Creatures
    NPC_THUNDERAAN                      = 14435,

    // Spells
    SPELL_TENDRILS_OF_AIR               = 23009,
    SPELL_TEARS_OF_THE_WIND_SEEKER      = 23011,
    
    // Texts
    SAY_AGGRO                           = 0,
};

class npc_prince_thunderaan : public CreatureScript
{
    public:
        npc_prince_thunderaan() : CreatureScript("npc_prince_thunderaan") { }
    
        struct npc_prince_thunderaanAI : public ScriptedAI
        {
            npc_prince_thunderaanAI(Creature* creature) : ScriptedAI(creature) { }
    
            void Reset() override
            {
                tendrilsOfAirTimer        = 17 * IN_MILLISECONDS;
                tearsOfTheWindSeekerTimer =  5 * IN_MILLISECONDS;
            }
    
            void EnterCombat(Unit* /*who*/) override
            {
                Talk(SAY_AGGRO);
            }
    
            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;
    
                if (tendrilsOfAirTimer <= diff)
                {
                    DoCastVictim(SPELL_TENDRILS_OF_AIR);
                    tendrilsOfAirTimer = 17 * IN_MILLISECONDS;
                }
                else
                    tendrilsOfAirTimer -= diff;
    
                if (tearsOfTheWindSeekerTimer <= diff)
                {
                    DoCastVictim(SPELL_TEARS_OF_THE_WIND_SEEKER);
                    tearsOfTheWindSeekerTimer = urand(7, 10) * IN_MILLISECONDS;
                }
                else
                    tearsOfTheWindSeekerTimer -= diff;
    
                DoMeleeAttackIfReady();
            }
            
        private:
            uint32 tendrilsOfAirTimer;
            uint32 tearsOfTheWindSeekerTimer;
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_prince_thunderaanAI (creature);
        }
};

void AddSC_silithus_rg()
{
    new npc_prince_thunderaan();
}
