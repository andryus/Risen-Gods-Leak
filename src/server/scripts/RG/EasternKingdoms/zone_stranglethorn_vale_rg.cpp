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
#include "ScriptedGossip.h"
#include "GameEventMgr.h"

enum Says
{
    SAY_START               = 0,
    SAY_WINNER              = 1,
    SAY_END                 = 2,
    QUEST_MASTER_ANGLER     = 8193,
};

/*######
## npc_riggle_bassbait
######*/
class npc_riggle_bassbait : public CreatureScript
{
public:
    npc_riggle_bassbait() : CreatureScript("npc_riggle_bassbait") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->IsQuestGiver()) // If the quest is still running.
        {
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());
            pPlayer->SEND_GOSSIP_MENU(7614, pCreature->GetGUID());
            return true;
        }
        // The Quest is not there anymore
        // There is a winner!
        pPlayer->SEND_GOSSIP_MENU(7714, pCreature->GetGUID());
        return true;
    }

    bool OnQuestReward(Player* pPlayer, Creature* pCreature, const Quest* pQuest, uint32 /*opt*/)
    {
        // TODO: check if this can only be called if NPC has QUESTGIVER flag.
        if (pQuest->GetQuestId() == QUEST_MASTER_ANGLER && ((npc_riggle_bassbaitAI*)(pCreature->AI()))->bEventWinnerFound == false)
        {
            pCreature->AI()->Talk(SAY_WINNER);
            pCreature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            ((npc_riggle_bassbaitAI*)(pCreature->AI()))->bEventWinnerFound = true;
            Creature* creature2 = pCreature->FindNearestCreature(15087,60.0f);
            if (creature2)
            {
                creature2->SetFlag(UNIT_NPC_FLAGS,UNIT_NPC_FLAG_QUESTGIVER);
            }

            return true;
        }
    return true;
}


    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_riggle_bassbaitAI (creature);
    }

    struct npc_riggle_bassbaitAI: public ScriptedAI
    {
        npc_riggle_bassbaitAI(Creature* c) : ScriptedAI(c)
        {
            // This will keep the NPC active even if there are no players around!
            c->setActive(true);
            bEventAnnounced = bEventIsOver = bEventWinnerFound = false;
            Reset();
        }
        /**
        *  Flag to check if event was announced. True if event was announced.
        */
        bool bEventAnnounced;
        /**
         *  Flag to check if event is over. True if event is over.
         */
        bool bEventIsOver;
        /**
         *  Flag to check if someone won the event. True if someone has won.
         */
        bool bEventWinnerFound;

        void Reset() { }
        void EnterCombat(Unit* /*who*/) {}

        void UpdateAI(uint32 /*uiDiff*/)
        {
            // Announce the event max 1 minute after being spawned. But only if Fishing extravaganza is running.
            if (!bEventAnnounced && time(NULL) % 60 == 0 && IsHolidayActive(HOLIDAY_FISHING_EXTRAVAGANZA))
            {
                Talk(SAY_START);
                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER); //Quest&Gossip are now active
                bEventAnnounced = true;
            }
            // The Event was started (announced) & It was not yet ended & One minute passed & the Fish are gone
            if ( bEventAnnounced && !bEventIsOver && time(NULL) % 60 == 0 && !IsHolidayActive(HOLIDAY_FISHING_EXTRAVAGANZA))
            {
                Talk(SAY_END);
                bEventIsOver = true;

            }
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
    }
    };
};

void AddSC_stranglethorn_vale_rg()
{
    new npc_riggle_bassbait();
}
