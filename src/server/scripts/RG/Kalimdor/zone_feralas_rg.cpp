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
#include "ScriptedGossip.h"
#include "SpellScript.h"

/*######
## npc_shayleafrunner
######*/

enum SHAY 
{
    NPC_ROCKBITER           = 7765,

    QUEST_WANDERING_SHAY    = 2845,
    FACTION_ESCORTEE        = 774,
};


class npc_shayleafrunner : public CreatureScript
{
    public: 
        npc_shayleafrunner() : CreatureScript("npc_shayleafrunner") { }
    
        bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest) override
        {
            if(quest->GetQuestId() == QUEST_WANDERING_SHAY)
            {
                if (npc_shayleafrunnerAI* shayleafrunnerAI = CAST_AI(npc_shayleafrunner::npc_shayleafrunnerAI, creature->AI()))
                    shayleafrunnerAI->StartFollow(player, FACTION_ESCORTEE, quest);
            }
            return true;
        }
    
        struct npc_shayleafrunnerAI : public FollowerAI
        {
            npc_shayleafrunnerAI(Creature* creature) : FollowerAI(creature) { }
    
            void MoveInLineOfSight(Unit* who) override
            {
                FollowerAI::MoveInLineOfSight(who);
    
                if (!me->GetVictim() && !HasFollowState(STATE_FOLLOW_COMPLETE) && who->GetEntry() == NPC_ROCKBITER)
                {
                    if (me->IsWithinDistInMap(who, INTERACTION_DISTANCE))
                    {
                        if (Player* player = GetLeaderForFollower())
                        {
                            if (player->GetQuestStatus(QUEST_WANDERING_SHAY) == QUEST_STATUS_INCOMPLETE)
                                player->GroupEventHappens(QUEST_WANDERING_SHAY, me);
                        }
    
                        SetFollowComplete(true);
                    }
                }
            }
    
            void UpdateFollowerAI(uint32 Diff) override
            {
                if(!UpdateVictim())
                    return;
    
                DoMeleeAttackIfReady();
            }
        };
        
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_shayleafrunnerAI(creature);
        }
};

/*####
 ## Quest 3520: Screecher Spirits
 ## spell_q3520_summon_screecher_spirit
 ####*/

class spell_q3520_summon_screecher_spirit : public SpellScriptLoader
{
   public:
       spell_q3520_summon_screecher_spirit() : SpellScriptLoader("spell_q3520_summon_screecher_spirit") { }
   
       class spell_q3520_summon_screecher_spirit_SpellScript : public SpellScript
       {
           PrepareSpellScript(spell_q3520_summon_screecher_spirit_SpellScript);
   
           void HandleSpell(SpellEffIndex /*effIndex*/)
           {   
               if (Unit* target = GetHitUnit())
               {
                   if (Creature* creature = target->ToCreature())
                       creature->DespawnOrUnsummon(0);
               }
           }
   
           void Register() override
           {
               OnEffectHitTarget += SpellEffectFn(spell_q3520_summon_screecher_spirit_SpellScript::HandleSpell, EFFECT_0, SPELL_EFFECT_SUMMON);
           }
       };
   
       SpellScript* GetSpellScript() const override
       {
           return new spell_q3520_summon_screecher_spirit_SpellScript();
       }
};

/*####
 ## Quest 3909: The Videre Elixir
 ## npc_miblon_snarltooth
 ####*/

enum MiblonSnarltoothMisc
{
    // Gameobjects
    GO_MIBLONS_BAIT = 164758,
    GO_MIBLONS_DOOR = 164729,
};

class npc_miblon_snarltooth : public CreatureScript
{
    public:
        npc_miblon_snarltooth() : CreatureScript("npc_miblon_snarltooth") { }

    struct npc_miblon_snarltoothAI : public ScriptedAI
    {
        npc_miblon_snarltoothAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            checkTimer = 1 * IN_MILLISECONDS;
        }

        void UpdateAI(uint32 diff) override
        {
            if (checkTimer <= diff)
            {
                if (me->FindNearestGameObject(GO_MIBLONS_BAIT, 10.0f))
                {
                    if (GameObject* door = me->FindNearestGameObject(GO_MIBLONS_DOOR, 50.0f))
                        door->Delete();
                }
                else
                {
                    if (!me->FindNearestGameObject(GO_MIBLONS_DOOR, 50.0f))
                        me->SummonGameObject(GO_MIBLONS_DOOR, -2849.0f, 2339.68f, 40.262f, 1.85f, 0.0f, 0.0f, 0.798636f, 0.601815f, 120);
                }
                
                checkTimer = urand(1, 2) * IN_MILLISECONDS;
            }
            else
                checkTimer -= diff;
        }
        
    private:
        uint32 checkTimer;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_miblon_snarltoothAI(creature);
    }
};

enum KnotMisc
{
    QUEST_THE_GORDOK_OGRE_SUIT = 5518,
    QUEST_FREE_KNOT            = 5525,
    QUEST_FREE_KNOT_DAILY      = 7429,
    GO_CHEST                   = 179501,
    GOSSIP_KNOT                = 60088,
    SPELL_LEATHERWORKING       = 22816,
    SPELL_TAILORING            = 22814

};

class npc_knot_thimblejack : public CreatureScript
{
    public:
        npc_knot_thimblejack() : CreatureScript("npc_knot_thimblejack") { }

        bool OnQuestReward(Player* /*player*/, Creature* creature, Quest const* quest, uint32 /*opt*/) override
        {
            switch (quest->GetQuestId())
            {
                case QUEST_FREE_KNOT:
                    creature->DespawnOrUnsummon();
                    break;
                case QUEST_FREE_KNOT_DAILY:
                    creature->SummonGameObject(GO_CHEST, 594.802f, 609.988f, -4.75476f, 3.9156f, 0.0f, 0.0f, 0.798636f, 0.601815f, 120);
                    creature->DespawnOrUnsummon();
                    break;
            }

            return false;
        }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            if (creature->IsQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());

            if ((player->GetQuestStatus(QUEST_THE_GORDOK_OGRE_SUIT) == QUEST_STATUS_REWARDED) && player->HasSkill(SKILL_LEATHERWORKING) && !player->HasSpell(SPELL_LEATHERWORKING))
                player->AddGossipItem(GOSSIP_KNOT, 0, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 0);

            if ((player->GetQuestStatus(QUEST_THE_GORDOK_OGRE_SUIT) == QUEST_STATUS_REWARDED) && player->HasSkill(SKILL_TAILORING) && !player->HasSpell(SPELL_TAILORING))
                player->AddGossipItem(GOSSIP_KNOT, 1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*uiSender*/, uint32 uiAction) override
        {
            switch (uiAction)
            {
                case GOSSIP_ACTION_INFO_DEF + 0:
                    player->CLOSE_GOSSIP_MENU();
                    player->CastSpell(player, SPELL_LEATHERWORKING, true);               
                    break;
                case GOSSIP_ACTION_INFO_DEF + 1:
                    player->CLOSE_GOSSIP_MENU();
                    player->CastSpell(player, SPELL_TAILORING, true);
                    break;
            }
            return true;
        }
};

/*######
## npc_oox22fe_rg
######*/

enum OOX_rg
{
    // Texts
    SAY_OOX_START        = 0,
    SAY_OOX_AGGRO        = 1,
    SAY_OOX_AMBUSH       = 2,
    SAY_OOX_END          = 3,
    
    // Creatures
    NPC_YETI             = 7848,
    
    // Quests
    QUEST_RESCUE_OOX22FE = 2767,

    // Spells
    SPELL_OOX_OFF        = 68499
};

class npc_oox22fe_rg : public CreatureScript
{
    public:
        npc_oox22fe_rg() : CreatureScript("npc_oox22fe_rg") { }
    
        bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest) override
        {
            if (quest->GetQuestId() == QUEST_RESCUE_OOX22FE)
            {
                creature->AI()->Talk(SAY_OOX_START);
                creature->SetImmuneToAll(false);
                //change that the npc is not lying dead on the ground
                creature->SetStandState(UNIT_STAND_STATE_STAND);
                creature->setFaction(FACTION_FRIENDLY_TO_ALL);
    
                if (npc_escortAI* pEscortAI = CAST_AI(npc_oox22fe_rg::npc_oox22fe_rgAI, creature->AI()))
                    pEscortAI->Start(true, false, player->GetGUID());
            }
            return true;
        }
        
        struct npc_oox22fe_rgAI : public npc_escortAI
        {
            npc_oox22fe_rgAI(Creature* creature) : npc_escortAI(creature) { }
    
            void WaypointReached(uint32 waypointId) override
            {
                switch (waypointId)
                {
                    case 1:
                        me->SetImmuneToAll(false);
                        break;
                    // First Ambush(3 Yetis)
                    case 11:
                        Talk(SAY_OOX_AMBUSH);
                        me->SummonCreature(NPC_YETI, -4841.01f, 1593.91f, 73.42f, 3.98f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 25 * IN_MILLISECONDS);
                        me->SummonCreature(NPC_YETI, -4837.61f, 1568.58f, 78.21f, 3.13f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 25 * IN_MILLISECONDS);
                        me->SummonCreature(NPC_YETI, -4841.89f, 1569.95f, 76.53f, 0.68f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 25 * IN_MILLISECONDS);
                        break;
                    case 15:
                        Talk(SAY_OOX_END);                    
                        DoCastSelf(SPELL_OOX_OFF, true);
                        // Award quest credit
                        if (Player* player = GetPlayerForEscort())
                            player->GroupEventHappens(QUEST_RESCUE_OOX22FE, me);
                        me->SetImmuneToAll(true);
                        break;
                }
            }
    
            void Reset() override
            {
                if (!HasEscortState(STATE_ESCORT_ESCORTING))
                    me->SetStandState(UNIT_STAND_STATE_DEAD);
            }
    
            void EnterCombat(Unit* /*who*/) override
            {
                //For an small probability the npc says something when he get aggro
                if (urand(0, 9) > 7)
                    Talk(SAY_OOX_AGGRO);
            }
    
            void JustSummoned(Creature* summoned) override
            {
                summoned->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
                summoned->AI()->AttackStart(me);
            }
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_oox22fe_rgAI(creature);
        }
};

void AddSC_feralas_rg()
{
    new npc_shayleafrunner();
    new spell_q3520_summon_screecher_spirit();
    new npc_miblon_snarltooth();
    new npc_knot_thimblejack();
    new npc_oox22fe_rg();
}
