/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2012 TrinityCore <http://www.rising-gods.de/>
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
#include "ScriptedFollowerAI.h"
#include "Player.h"

/*#####
# npc_dorius
######*/
enum EdorData
{
    QUEST_SUNTARA_STONES    = 3367,
    NPC_LATHORIC_THE_BLACK  = 8391,
    GO_QUEST_ROOL           = 175704,
    SPELL_SHOT              = 51502,
};

class npc_dorius : public CreatureScript
{
public:
    npc_dorius() : CreatureScript("npc_dorius") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest)
    {
        if (quest->GetQuestId() == QUEST_SUNTARA_STONES)
        {
            if (npc_doriusAI* escortAI = CAST_AI(npc_dorius::npc_doriusAI, creature->AI()))
            {
                creature->setFaction(player->getFaction());
                creature->SetStandState(UNIT_STAND_STATE_STAND);
                escortAI->Start(false, false, player->GetGUID(), quest, true);
                creature->MonsterSay("Bravo, bravo! Good show. What? You thought I was dead?",0,player);
            }
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_doriusAI(creature);
    }

    struct npc_doriusAI : public npc_escortAI
    {
        npc_doriusAI(Creature* creature) : npc_escortAI(creature) { }

        ObjectGuid LathoricGUID;

        void Reset()
        {
            LathoricGUID.Clear();
        }

        void WaypointReached(uint32 uiPointId)
        {
            switch(uiPointId)
            {
                case 0:
                    me->MonsterSay("Fools, I knew that if I played on my brother's feeble emotions, he would send 'rescuers'.",0,me);
                    break;
                case 1:
                    me->MonsterSay("How easy it was to manipulate you into recovering the last Suntara stone from those imbeciles of the Twilight Hammer When I stumbled on the Suntara stones at the Grimesilt digsite, the power of Ragnaros surged through my being.",0,me);
                    break;
                case 2:
                    SetRun(true);
                    break;
                case 20:
                    SetRun(false);
                    me->MonsterSay("It was Ragnaros that gave me a purpose.",0,me);
                    break;
                case 21:
                    me->MonsterSay("It was the will of Ragnaros that Obsidion be built. Obsidion will destroy the Blackrock Orcs of Blackrock Spire, uniting us with our brethren in the fiery depths.",0,me);
                    break;
                case 22:
                    me->MonsterSay("And, ultimately, it was Ragnaros that named me when I was reborn as an acolyte of fire: Lathoric... Lathoric the Black.",0,me);

                    if (Unit* black = me->SummonCreature(NPC_LATHORIC_THE_BLACK,-6370.267578f,-1975.036499f,256.774353f,3.740159f,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 10000))
                        LathoricGUID = black->GetGUID();
                    break;
                case 23:
                    me->SummonGameObject(GO_QUEST_ROOL,-6386.89f,-1984.05f,246.73f,-1.27409f,0,0,0.785556f,0.61879f,0);

                    if (Player* player = GetPlayerForEscort())
                        player->GroupEventHappens(QUEST_SUNTARA_STONES, me);

                    if (Creature* black = ObjectAccessor::GetCreature(*me, LathoricGUID))
                    {
                        black->MonsterSay("Your task is complete. Prepare to meet your doom. Obsidion, Rise and Serve your Master!",0,me);
                        black->CastSpell(me, SPELL_SHOT, false);
                        black->DealDamage(me, 10000000);
                    }
                    break;
            }
        }
    };

};


void AddSC_searing_gorge()
{
    new npc_dorius();
}
