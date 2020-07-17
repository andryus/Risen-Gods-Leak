/*
 * Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
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

#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "SmartAI.h"

enum DefiasTraitorMisc
{
    SAY_AGGRO = 3
};

class npc_the_defias_traitor : public CreatureScript
{
    public:
        npc_the_defias_traitor() : CreatureScript("npc_the_defias_traitor") { }

        struct npc_the_defias_traitorAI : public SmartAI
        {
            npc_the_defias_traitorAI(Creature* creature) : SmartAI(creature) { }

            void EnterCombat(Unit* who) override
            {
                Talk(SAY_AGGRO, who);
                SmartAI::EnterCombat(who);
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_the_defias_traitorAI(creature);
        }
};

/*######
## npc_daphne_stilwell
######*/

enum DaphneStilwell
{
    // Yells
    SAY_DS_START          = 0,
    SAY_DS_DOWN_1         = 1,
    SAY_DS_DOWN_2         = 2,
    SAY_DS_DOWN_3         = 3,
    SAY_DS_PROLOGUE       = 4,

    // Spells
    SPELL_SHOOT           = 6660,

    // Quests
    QUEST_TOME_VALOR      = 1651,

    // Creatures
    NPC_DEFIAS_RAIDER     = 6180,

    // Equips
    EQUIP_ID_RIFLE        = 2511
};

class npc_daphne_stilwell : public CreatureScript
{
    public:
        npc_daphne_stilwell() : CreatureScript("npc_daphne_stilwell") { }
    
        bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest) override
        {
            if (quest->GetQuestId() == QUEST_TOME_VALOR)
            {
                npc_escortAI* pEscortAI = CAST_AI(npc_daphne_stilwell::npc_daphne_stilwellAI, creature->AI());
                if (pEscortAI && !pEscortAI->HasEscortState(STATE_ESCORT_PAUSED))
                {
                    creature->AI()->Talk(SAY_DS_START);
                    pEscortAI->Start(true, true, player->GetGUID());
                }
                else
                    player->FailQuest(QUEST_TOME_VALOR);
            }
    
            return true;
        }
            
        struct npc_daphne_stilwellAI : public npc_escortAI
        {
            npc_daphne_stilwellAI(Creature* creature) : npc_escortAI(creature), summons(me) { }
    
            SummonList summons;
            uint8 textCounter;
    
            void Reset() override
            {
                summons.DespawnAll();
                textCounter = SAY_DS_DOWN_1;
            }
    
            void WaypointReached(uint32 waypointId) override
            {
                Player* player = GetPlayerForEscort();
                if (!player)
                {
                    me->DespawnOrUnsummon(1);
                    return;
                }
    
                switch (waypointId)
                {
                    case 4:
                        SetEquipmentSlots(false, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE, EQUIP_ID_RIFLE);
                        me->SetSheath(SHEATH_STATE_RANGED);
                        me->HandleEmoteCommand(EMOTE_STATE_USE_STANDING_NO_SHEATHE);
                        break;
                    case 7:
                        me->SummonCreature(NPC_DEFIAS_RAIDER, -11435.52f, 1601.26f, 68.06f, 4.1f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        me->SummonCreature(NPC_DEFIAS_RAIDER, -11440.96f, 1599.69f, 66.35f, 4.09f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        me->SummonCreature(NPC_DEFIAS_RAIDER, -11433.44f, 1594.24f, 66.99f, 4.05f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        break;
                    case 8:
                        me->SetSheath(SHEATH_STATE_RANGED);
                        me->SummonCreature(NPC_DEFIAS_RAIDER, -11435.52f, 1601.26f, 68.06f, 4.1f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        me->SummonCreature(NPC_DEFIAS_RAIDER, -11440.96f, 1599.69f, 66.35f, 4.09f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        me->SummonCreature(NPC_DEFIAS_RAIDER, -11433.44f, 1594.24f, 66.99f, 4.05f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        me->SummonCreature(NPC_DEFIAS_RAIDER, -11428.29f, 1598.37f, 70.90f, 3.9f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        break;
                    case 9:
                        me->SetSheath(SHEATH_STATE_RANGED);
                        me->SummonCreature(NPC_DEFIAS_RAIDER, -11435.52f, 1601.26f, 68.06f, 4.1f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        me->SummonCreature(NPC_DEFIAS_RAIDER, -11440.96f, 1599.69f, 66.35f, 4.09f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        me->SummonCreature(NPC_DEFIAS_RAIDER, -11433.44f, 1594.24f, 66.99f, 4.05f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        me->SummonCreature(NPC_DEFIAS_RAIDER, -11428.29f, 1598.37f, 70.90f, 3.9f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        me->SummonCreature(NPC_DEFIAS_RAIDER, -11438.14f, 1607.6f, 70.94f, 4.38f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        break;
                    case 10:
                        SetRun(false);
                        break;
                    case 11:
                        Talk(SAY_DS_PROLOGUE);
                        break;
                    case 13:
                        SetEquipmentSlots(true);
                        me->SetSheath(SHEATH_STATE_UNARMED);
                        me->HandleEmoteCommand(EMOTE_STATE_USE_STANDING_NO_SHEATHE);
                        break;
                    case 17:
                        SetEscortPaused(true);
                        player->GroupEventHappens(QUEST_TOME_VALOR, me);
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                        me->DespawnOrUnsummon(10000);
                        break;
                }
            }
    
            void AttackStart(Unit* who) override
            {
                if (me->Attack(who, false))
                {
                    me->SetInCombatWith(who);
                    who->SetInCombatWith(me);
                }
            }
    
            void JustSummoned(Creature* creature) override
            {
                creature->SetHomePosition(me->GetHomePosition());
                creature->GetMotionMaster()->MoveChase(me);
                creature->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
                creature->AI()->AttackStart(me);
                AddThreat(me, 0.0f, creature);
                summons.Summon(creature);
            }
    
            void JustDied(Unit* /*killer*/) override
            {
                Player* player = GetPlayerForEscort();
                if (player)
                    player->FailQuest(QUEST_TOME_VALOR);
                Reset();
            }
    
            void CorpseRemoved(uint32& /*whatever*/) override
            {
                Reset();
            }
    
            void SummonedCreatureDies(Creature* creature, Unit*) override
            {
                summons.Despawn(creature);
                if (summons.empty())
                    Talk(textCounter++, GetPlayerForEscort());
            }
    
            void UpdateAI(uint32 diff) override
            {
                npc_escortAI::UpdateAI(diff);
    
                if (!UpdateVictim())
                    return;
    
                if (me->isAttackReady(BASE_ATTACK))
                {
                    if (!me->IsWithinDist(me->GetVictim(), ATTACK_DISTANCE))
                        DoCastVictim(SPELL_SHOOT, true);
                    else
                    {
                        me->SetSheath(SHEATH_STATE_MELEE);
                        me->AttackerStateUpdate(me->GetVictim());
                    }
    
                    me->resetAttackTimer();
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_daphne_stilwellAI(creature);
        }
};

enum DuskwoodChest
{
    ITEM_KLAVEN_JOURNAL     = 7908,
    ITEM_DEFIAS_KEY         = 7923,
    SPELL_TOUCH_OF_ZANZLI   = 9991,
};

class go_duskwood_chest : public GameObjectScript
{
public:
    go_duskwood_chest() : GameObjectScript("go_duskwood_chest") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (player && player->HasItemCount(ITEM_DEFIAS_KEY))
        {
            player->AddItem(ITEM_KLAVEN_JOURNAL, 1);
            player->CastSpell(player, SPELL_TOUCH_OF_ZANZLI);
        }
        return false;
    }
};

void AddSC_westfall_rg()
{
    new npc_the_defias_traitor();
    new npc_daphne_stilwell();
    new go_duskwood_chest();
}
