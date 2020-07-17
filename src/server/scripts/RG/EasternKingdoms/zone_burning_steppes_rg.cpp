/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * Copyright (C) 2010-2014 Rising Gods <http://www.rising-gods.de/>
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
#include "ScriptedGossip.h"
#include "SpellInfo.h"

/*######
## npc_grark_lorkrub
######*/

enum LorkrubMisc
{
    // Texts
    SAY_START                       = 17,
    SAY_PAY                         = 16,
    SAY_FIRST_AMBUSH_START          = 15,
    SAY_FIRST_AMBUSH_END            = 14,
    SAY_SEC_AMBUSH_START            = 13,
    SAY_SEC_AMBUSH_END              = 12,
    SAY_THIRD_AMBUSH_START          = 11,
    SAY_THIRD_AMBUSH_END            = 10,
    EMOTE_LAUGH                     = 9,
    SAY_LAST_STAND                  = 8,
    SAY_LEXLORT_1                   = 7,
    SAY_LEXLORT_2                   = 6,
    EMOTE_RAISE_AXE                 = 5,
    EMOTE_LOWER_HAND                = 4,
    SAY_LEXLORT_3                   = 3,
    SAY_LEXLORT_4                   = 2,
    EMOTE_SUBMIT                    = 1,
    SAY_AGGRO                       = 0,
    
    // Creatures
    NPC_BLACKROCK_AMBUSHER          = 9522,
    NPC_BLACKROCK_RAIDER            = 9605,
    NPC_FLAMESCALE_DRAGONSPAWN      = 7042,
    NPC_SEARSCALE_DRAKE             = 7046,
    NPC_GRARK_LORKRUB               = 9520,
    NPC_HIGH_EXECUTIONER_NUZARK     = 9538,
    NPC_SHADOW_OF_LEXLORT           = 9539,
    
    // Quests
    QUEST_ID_PRECARIOUS_PREDICAMENT = 4121,
    
    // Spells
    SPELL_CAPTURE_GRARK             = 14250,

    // Factions
    FACTION_HOSTILE_GRARK           = 40,
    FACTION_NEUTRAL_GRARK           = 29
};

class npc_grark_lorkrub : public CreatureScript
{
public:
    npc_grark_lorkrub() : CreatureScript("npc_grark_lorkrub") { }

    struct npc_grark_lorkrubAI : public npc_escortAI
    {
        npc_grark_lorkrubAI(Creature* creature) : npc_escortAI(creature) { }
        
        void Reset() override
        {
            me->SetReactState(REACT_AGGRESSIVE);           
        }

        void WaypointReached(uint32 waypointId) override
        {
            Player* player = GetPlayerForEscort();
            if (!player)
                return;

            switch (waypointId)
            {
                case 1:
                    Talk(SAY_START);
                    break;
                case 7:
                    Talk(SAY_PAY);
                    break;
                case 12:
                    Talk(SAY_FIRST_AMBUSH_START);
                    me->SummonCreature(NPC_BLACKROCK_AMBUSHER, -7844.3f, -1521.6f, 139.2f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 20*IN_MILLISECONDS);
                    me->SummonCreature(NPC_BLACKROCK_AMBUSHER, -7860.4f, -1507.8f, 141.0f, 6.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 20*IN_MILLISECONDS);
                    me->SummonCreature(NPC_BLACKROCK_RAIDER,   -7845.6f, -1508.1f, 138.8f, 6.1f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 20*IN_MILLISECONDS);
                    me->SummonCreature(NPC_BLACKROCK_RAIDER,   -7859.8f, -1521.8f, 139.2f, 6.2f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 20*IN_MILLISECONDS);
                    break;
                case 14:
                    Talk(SAY_FIRST_AMBUSH_END);
                    break;
                case 24:
                    Talk(SAY_SEC_AMBUSH_START);
                    me->SummonCreature(NPC_BLACKROCK_AMBUSHER,     -8035.3f, -1222.2f, 135.5f, 5.1f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 20*IN_MILLISECONDS);
                    me->SummonCreature(NPC_FLAMESCALE_DRAGONSPAWN, -8037.5f, -1216.9f, 135.8f, 5.1f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 20*IN_MILLISECONDS);
                    me->SummonCreature(NPC_BLACKROCK_AMBUSHER,     -8009.5f, -1222.1f, 139.2f, 3.9f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 20*IN_MILLISECONDS);
                    me->SummonCreature(NPC_FLAMESCALE_DRAGONSPAWN, -8007.1f, -1219.4f, 140.1f, 3.9f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 20*IN_MILLISECONDS);
                    break;
                case 26:
                    Talk(SAY_SEC_AMBUSH_END);
                    break;
                case 28:
                    me->SummonCreature(NPC_SEARSCALE_DRAKE, -7897.8f, -1123.1f, 233.4f, 3.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60*IN_MILLISECONDS);
                    me->SummonCreature(NPC_SEARSCALE_DRAKE, -7898.8f, -1125.1f, 193.9f, 3.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60*IN_MILLISECONDS);
                    me->SummonCreature(NPC_SEARSCALE_DRAKE, -7895.6f, -1119.5f, 194.5f, 3.1f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60*IN_MILLISECONDS);
                    break;
                case 30:
                    Talk(SAY_THIRD_AMBUSH_START);
                    break;
                case 33:
                    Talk(SAY_THIRD_AMBUSH_END);
                    break;
                case 36:
                    Talk(EMOTE_LAUGH);
                    break;
                case 45:
                    Talk(SAY_LAST_STAND);
                    me->SummonCreature(NPC_HIGH_EXECUTIONER_NUZARK, -7532.3f, -1029.4f, 258.0f, 2.7f, TEMPSUMMON_TIMED_DESPAWN, 40*IN_MILLISECONDS);
                    me->SummonCreature(NPC_SHADOW_OF_LEXLORT,       -7532.8f, -1032.9f, 258.2f, 2.5f, TEMPSUMMON_TIMED_DESPAWN, 40*IN_MILLISECONDS);
                    break;
                case 46:
                    me->SetStandState(UNIT_STAND_STATE_KNEEL);
                    break;
            }
        }

        void JustDied(Unit* /*killer*/) override
        {
            me->DespawnOrUnsummon();
            me->SetRespawnTime(540);
            me->SetCorpseDelay(1);
            me->setFaction(FACTION_HOSTILE_GRARK);
            me->SetReactState(REACT_AGGRESSIVE);
            if (Player* player = GetPlayerForEscort())
                player->GroupEventHappens(QUEST_ID_PRECARIOUS_PREDICAMENT, me);
        }

        void EnterCombat(Unit* /*who*/) override
        {
            if (!HasEscortState(STATE_ESCORT_ESCORTING))
                Talk(SAY_AGGRO);
        }

        void JustSummoned(Creature* summoned) override
        {
            if (Player* player = GetPlayerForEscort())
                summoned->AI()->AttackStart(player);
        }

        void SpellHit(Unit* /*caster*/, SpellInfo const* spell) override
        {
            if (spell->Id == SPELL_CAPTURE_GRARK)
            {
                if (HealthAbovePct(25))
                    return;
                me->SetReactState(REACT_PASSIVE);
                me->setFaction(FACTION_NEUTRAL_GRARK);
                me->AttackStop();
                ResetThreatList();
                EnterEvadeMode();
            }
        }

    };

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_ID_PRECARIOUS_PREDICAMENT)
        {
            if (npc_escortAI* pEscortAI = CAST_AI(npc_grark_lorkrub::npc_grark_lorkrubAI, creature->AI()))
                pEscortAI->Start(true, false, player->GetGUID());

            creature->SetImmuneToPC(true);
            creature->SetImmuneToNPC(true);
            creature->SetReactState(REACT_PASSIVE);
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_grark_lorkrubAI(creature);
    }
};

enum CyrusMisc
{
    NPC_FRENZIED_BLACK_DRAKE = 9461,
};

class npc_cyrus_therepentous : public CreatureScript
{
public:
    npc_cyrus_therepentous() : CreatureScript("npc_cyrus_therepentous") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 /*action*/) override
    {
        if (player && creature)
        {
            player->PlayerTalkClass->ClearMenus();

            if (!creature->FindNearestCreature(NPC_FRENZIED_BLACK_DRAKE, 150.0f))
                creature->SummonCreature(NPC_FRENZIED_BLACK_DRAKE, -7656.94f, -3009.47f, 133.206f, 4.83028f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 180 * IN_MILLISECONDS);
            player->CLOSE_GOSSIP_MENU();
        }
        return true;
    }
};

void AddSC_burning_steppes_rg()
{
    new npc_grark_lorkrub();
    new npc_cyrus_therepentous();
}
