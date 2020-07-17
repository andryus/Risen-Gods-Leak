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

/*######
# npc_miran
######*/

enum MiranMisc
{
    // Texts
    SAY_START           =   0,
    SAY_AGGRO           =   1,
    SAY_END             =   2,
    
    // Quests
    QUEST_PROTECT       =  309,
    
    // Creatures
    NPC_AMBUSHER        = 1981
};

class npc_miran : public CreatureScript
{
public:
    npc_miran() : CreatureScript("npc_miran") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest)
    {
        if (quest->GetQuestId() == QUEST_PROTECT)
        {
            if (npc_miranAI* pEscortAI = CAST_AI(npc_miran::npc_miranAI, creature->AI()))
                pEscortAI->Start(false, false, player->GetGUID(), quest);
        }
        return true;
    }

    struct npc_miranAI : public npc_escortAI
    {
        npc_miranAI(Creature* creature) : npc_escortAI(creature) { }

        void WaypointReached(uint32 waypointId) override
        {
            switch (waypointId)
            {
                case 21:          
                    Talk(SAY_START);
                    me->SummonCreature(NPC_AMBUSHER, -5692.58f, -3597.50f, 315.79f, 3.25f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,  30 * IN_MILLISECONDS);
                    me->SummonCreature(NPC_AMBUSHER, -5693.89f, -3604.78f, 315.43f, 2.38f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,  30 * IN_MILLISECONDS);
                    me->SummonCreature(NPC_AMBUSHER, -5699.26f, -3605.054f, 315.18f, 1.59f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30 * IN_MILLISECONDS);
                    break;
                case 23:
                    Talk(SAY_AGGRO);
                    break;
                case 39:
                    Talk(SAY_END);
                    if (Player* player = GetPlayerForEscort())
                        player->GroupEventHappens(QUEST_PROTECT, me);
                    SetRun(true);
                    break;
            }
        }

      void JustSummoned(Creature* summoned) override
      {
           summoned->AI()->AttackStart(me);
      }

    };
    
    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_miranAI(creature);
    }
};

void AddSC_loch_modan_rg()
{
    new npc_miran();
}
