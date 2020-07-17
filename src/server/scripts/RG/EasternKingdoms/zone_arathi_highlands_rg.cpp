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

enum RouschEvents
{
    EVENT_SALUT       = 1,
    EVENT_STAND_ONE   = 2,
    EVENT_KNEEL       = 3,
    EVENT_STAND_TWO   = 4
};

class npc_rousch : public CreatureScript
{
    public:
        npc_rousch() : CreatureScript("npc_rousch") { }

        struct npc_rouschAI : public ScriptedAI
        {
            npc_rouschAI(Creature* creature) : ScriptedAI(creature) { }
       
            void Reset() override
            {
                events.ScheduleEvent(EVENT_KNEEL, 3 * IN_MILLISECONDS);
            }
        
            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);
            
                switch (events.ExecuteEvent()) 
                {
                    case EVENT_SALUT:
                        me->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
                        events.ScheduleEvent(EVENT_STAND_ONE, 1 * IN_MILLISECONDS);
                        break;
                    case EVENT_STAND_ONE:
                        me->HandleEmoteCommand(EMOTE_STATE_STAND);
                        events.ScheduleEvent(EVENT_KNEEL, 7 * IN_MILLISECONDS);
                        break;
                    case EVENT_KNEEL:
                        me->SetSheath(SHEATH_STATE_UNARMED);
                        me->SetStandState(UNIT_STAND_STATE_KNEEL); 
                        events.ScheduleEvent(EVENT_STAND_TWO, 20 * IN_MILLISECONDS);
                        break;
                    case EVENT_STAND_TWO:
                        me->HandleEmoteCommand(EMOTE_STATE_STAND);
                        events.ScheduleEvent(EVENT_SALUT, 3 * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }

        private:
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_rouschAI(creature);
        }
};

/*######
## npc_kinelory
######*/
enum
{
    // Texts
    SAY_START               = 9,
    SAY_REACH_BOTTOM        = 8,
    SAY_AGGRO_KINELORY      = 7,
    SAY_AGGRO_JORELL        = 6,
    SAY_WATCH_BACK          = 5,
    EMOTE_BELONGINGS        = 4,
    SAY_DATA_FOUND          = 3,
    SAY_ESCAPE              = 2,
    SAY_FINISH              = 1,
    EMOTE_HAND_PACK         = 0,
    
    // Spells
    SPELL_REJUVENATION      = 3627,
    SPELL_BEAR_FORM         = 4948,
    
    // Creatures
    NPC_JORELL              = 2733,
    NPC_QUAE                = 2712,
    
    // Quests
    QUEST_HINTS_NEW_PLAGUE  = 660
};

class npc_kinelory : public CreatureScript
{
    public:
        npc_kinelory() : CreatureScript("npc_kinelory") { }
    
        struct npc_kineloryAI : public npc_escortAI
        {
            npc_kineloryAI(Creature* creature) : npc_escortAI(creature) { }
            
            void Reset() override
            {
                BearFormTimer = urand(5, 7) * IN_MILLISECONDS;
                HealTimer     = urand(2, 5) * IN_MILLISECONDS;
            }
            
            void WaypointReached(uint32 waypointId) override
            {
                Player* player = GetPlayerForEscort();
                if (!player)
                    return;
    
                switch (waypointId)
                {
                    case 9:
                        Talk(SAY_REACH_BOTTOM);
                        break;
                    case 16:
                        Talk(SAY_WATCH_BACK);
                        Talk(EMOTE_BELONGINGS);
                        break;
                    case 17:
                        Talk(SAY_DATA_FOUND);
                        break;
                    case 18:
                        Talk(SAY_ESCAPE);
                        if (Player* player = GetPlayerForEscort())
                            me->SetFacingToObject(player);
                        SetRun(true);
                        break;
                    case 35:
                        Talk(SAY_FINISH);
                        if (Creature* quae = me->FindNearestCreature(NPC_QUAE, 200.0f, true))
                        {
                            Talk(EMOTE_HAND_PACK);
                            me->SetFacingToObject(quae);
                        }
                        break;
                    case 36:
                        if (Player* player = GetPlayerForEscort())
                            player->GroupEventHappens(QUEST_HINTS_NEW_PLAGUE, me);
                        me->SetImmuneToNPC(true);
                        me->SetRespawnTime(1); 
                        me->SetCorpseDelay(1);
                        SetRun(false);
                        break;
                }
            }
    
            void JustDied(Unit* /*killer*/) override
            {
                if (Player* player = GetPlayerForEscort())
                    player->FailQuest(QUEST_HINTS_NEW_PLAGUE);
            }
    
            void EnterCombat(Unit* who) override
            {
                if (roll_chance_i(30))
                    Talk(SAY_AGGRO_KINELORY);
            }
    
            void JustSummoned(Creature* summoned) override
            {
                if (Player* player = GetPlayerForEscort())
                    summoned->AI()->AttackStart(player);
            }
    
            void UpdateAI(uint32 diff) override
            {
                npc_escortAI::UpdateAI(diff);
    
                //Combat check
                if (me->GetVictim())
                {
                    if (BearFormTimer <= diff)
                    {
                        DoCastSelf(SPELL_BEAR_FORM, true);
                        BearFormTimer = urand(25, 30) * IN_MILLISECONDS;
                    }
                    else BearFormTimer -= diff;
    
                    if (HealTimer < diff)
                    {
                        if (Unit* target = DoSelectLowestHpFriendly(40.0f))
                        {
                            DoCast(target, SPELL_REJUVENATION, true);
                            HealTimer = urand(15, 25) * IN_MILLISECONDS;
                        }
                    }  
                    else HealTimer -= diff;
    
                    DoMeleeAttackIfReady();
                }
            }
    
        private:
            uint32 BearFormTimer;
            uint32 HealTimer;
    
        };
    
        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
        {
            if (quest->GetQuestId() == QUEST_HINTS_NEW_PLAGUE)
            {
                if (npc_escortAI* pEscortAI = CAST_AI(npc_kinelory::npc_kineloryAI, creature->AI()))
                    pEscortAI->Start(true, false, player->GetGUID());
    
                creature->SetImmuneToAll(false);
                creature->AI()->Talk(SAY_START, player);
            }
            return true;
        }
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_kineloryAI(creature);
        }
};

void AddSC_arathi_highlands_rg()
{
    new npc_rousch();
    new npc_kinelory();
}
