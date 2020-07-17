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
#include "ScriptedFollowerAI.h"
#include "ScriptedEscortAI.h"

enum OneShotOneKillMisc
{
    // Quests
    QUEST_ONESHOTONEKILL         = 5713,
    
    // Creatures
    NPC_BLACKWOOD_TRACKER        = 11713,
    NPC_MAROSH_THE_DEVIOUS       = 11714,
    
    // Actions
    ACTION_ONESHOTONEKILL_START  = 1,
    
    // Spells
    SPELL_SHOOT                  = 19767,
    
    // Items
    ITEM_EQUIP                   = 22969,
    
    // Texts
    SAY_START                    = 0,
    SAY_OUT_OF_ARROWS            = 1,
    SAY_END                      = 2,
    SAY_END2                     = 3,
    SAY_END3                     = 4
};

class npc_aynasha : public CreatureScript
{
    public:
        npc_aynasha() : CreatureScript("npc_aynasha") { }
    
        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
        {
            if (quest->GetQuestId() == QUEST_ONESHOTONEKILL)
            {
                CAST_AI(npc_aynasha::npc_aynashaAI, creature->AI())->StartEvent();
                CAST_AI(npc_aynasha::npc_aynashaAI, creature->AI())->PlayerGUID = (player->GetGUID());
    
                if (npc_escortAI* pEscortAI = CAST_AI(npc_aynasha::npc_aynashaAI, creature->AI()))
                    pEscortAI->Start(true, true, player->GetGUID());
    
            }
            return true;
        }
    
        struct npc_aynashaAI : public npc_escortAI
        {
            npc_aynashaAI(Creature* creature) : npc_escortAI(creature)
            {
                bArrow_said = 0;
                Emote_done  = 0;
                uiWait_Controller = 0;
            }
            
            ObjectGuid PlayerGUID;
    
            void Reset() override
            {
                uiShoot_Timer = 1 * IN_MILLISECONDS;
            }
    
            void WaypointReached(uint32 waypointId) override { }
    
            void JustSummoned(Creature* summoned) override
            {
                summoned->AI()->AttackStart(me);
            }
    
            void JustDied(Unit* /*killer*/) override
            {
                if (PlayerGUID)
                    if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                        player->FailQuest(QUEST_ONESHOTONEKILL);
            }
    
            void UpdateAI(uint32 diff) override
            {
                if(uiWait_Controller)
                {
                    if(uiWait_Timer <= diff)
                    {
                        switch(uiWait_Controller)
                        {
                            case 1: 
                                me->SummonCreature(NPC_BLACKWOOD_TRACKER, 4374.425781f, -55.801338f, 87.653648f, 5.664735f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10 * IN_MILLISECONDS);
                                me->SummonCreature(NPC_BLACKWOOD_TRACKER, 4377.024414f, -53.647842f, 86.534828f, 5.495090f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10 * IN_MILLISECONDS);
                                me->SummonCreature(NPC_BLACKWOOD_TRACKER, 4379.248535f, -50.534988f, 85.865311f, 5.363142f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10 * IN_MILLISECONDS);
                                ++uiWait_Controller;
                                uiWait_Timer = 30 * IN_MILLISECONDS;
                                break;
                            case 2: 
                                me->SummonCreature(NPC_BLACKWOOD_TRACKER, 4374.425781f, -55.801338f, 87.653648f, 5.664735f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10 * IN_MILLISECONDS);
                                me->SummonCreature(NPC_BLACKWOOD_TRACKER, 4377.024414f, -53.647842f, 86.534828f, 5.495090f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10 * IN_MILLISECONDS);
                                me->SummonCreature(NPC_BLACKWOOD_TRACKER, 379.248535f, -50.534988f, 85.865311f, 5.363142f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10 * IN_MILLISECONDS);
                                me->SummonCreature(NPC_BLACKWOOD_TRACKER, 4370.991211f, -50.038376f, 86.439560f, 5.530042f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10 * IN_MILLISECONDS);
                                ++uiWait_Controller;
                                uiWait_Timer = 30 * IN_MILLISECONDS;
                                break;
                            case 3:
                                me->SummonCreature(NPC_MAROSH_THE_DEVIOUS, 4377.024414f, -53.647842f, 86.534828f, 5.495090f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10 * IN_MILLISECONDS);
                                ++uiWait_Controller;
                                uiWait_Timer = 20 * IN_MILLISECONDS;
                                break;
                            case 4:
                                if (!UpdateVictim())            
                                {
                                    Talk(SAY_END);
                                    Emote_done = 1;
                                    uiWait_Timer = 10 * IN_MILLISECONDS;
                                    ++uiWait_Controller;
                                }
                                break;
                            case 5:
                                if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                                     player->GroupEventHappens(QUEST_ONESHOTONEKILL,me);
                                Talk(SAY_END2);
                                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL); 
                                bArrow_said = 0;
                                ++uiWait_Controller;
                                uiWait_Timer = 5 * IN_MILLISECONDS;
                                break;
                            case 6:
                                Talk(SAY_END3);                            
                                uiWait_Controller = 0;
                                me->DespawnOrUnsummon();
                                me->SetRespawnTime(1);
                                me->SetCorpseDelay(1);
                                break;
                            default:
                                break;
                        }
                    }else uiWait_Timer -= diff;
                }
    
                //if(!UpdateVictim())           
                //   return;           
    
                if(uiArrow_Timer <= diff && !bArrow_said && uiWait_Controller)
                {
                    Talk(SAY_OUT_OF_ARROWS);
                    bArrow_said = 1;
                }else uiArrow_Timer -= diff;
    
                if(uiEmote_Timer <= diff && !Emote_done && uiWait_Controller)
                {
                    me->HandleEmoteCommand(EMOTE_ONESHOT_USE_STANDING);
                    uiEmote_Timer = 1 * IN_MILLISECONDS;
                }else uiEmote_Timer -= diff;
    
                if(uiShoot_Timer <= diff && !bArrow_said)
                {
                    DoCast(me->GetVictim(),SPELL_SHOOT);
                    uiShoot_Timer = 1 * IN_MILLISECONDS;
                }else uiShoot_Timer -= diff;
            }
    
            void StartEvent()
            {
                uiWait_Controller = 1;
                Emote_done = 0;
                uiWait_Timer = 5 * IN_MILLISECONDS;
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);
                Talk(SAY_START);
                uiArrow_Timer = 40 * IN_MILLISECONDS;
                uiEmote_Timer =  2 * IN_MILLISECONDS;
            }
            
        private:
            uint32 uiWait_Controller;
            uint32 uiWait_Timer;
            uint32 uiShoot_Timer;
            uint32 uiArrow_Timer;
            uint32 uiEmote_Timer;
            bool bArrow_said;
            bool Emote_done;
        };
        
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_aynashaAI(creature);
        }
};

void AddSC_darkshore_rg()
{
    new npc_aynasha();
}
