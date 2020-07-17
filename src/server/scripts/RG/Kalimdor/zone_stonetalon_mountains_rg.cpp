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
#include "SpellScript.h"
#include "Group.h"

/*######
## npc_piznik
######*/

enum PinzikMisc
{
    // Quests
    QUEST_GERENZOS_ORDER    = 1090,
    
    // Creatures
    NPC_WINDSHEAR_DIGGER    = 3999,
};

Position const PinzikSummonPositions[3] =
{
    {954.349976f, -257.600006f, -3.477842f, 5.753643f},
    {956.172607f, -254.613449f, -3.107884f, 5.408071f},
    {953.306152f, -261.492126f, -3.711920f, 5.973548f},
};

class npc_piznik : public CreatureScript
{
    public:
        npc_piznik() : CreatureScript("npc_piznik") { }
    
        bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest) override
        {
            if (quest->GetQuestId() == QUEST_GERENZOS_ORDER)
            {
                creature->AI()->SetGuidData(player->GetGUID(), 0);
            }
            
            return true;
        }
    
        struct npc_piznikAI : public ScriptedAI
        {
            npc_piznikAI(Creature* c) : ScriptedAI(c), summons(me) {}
    
            void SetGuidData(ObjectGuid guid, uint32 /*type*/) override
            {
                start = true;
                playerGUID = guid;
            }
    
            void Reset() override
            {
                step = 0;
                start = false;
                waitTimer = 1 * IN_MILLISECONDS;
                playerGUID.Clear();
                summons.DespawnAll();
            }
    
            void JustDied(Unit* /*killer*/) override
            {
                SetGroupQuestStatus(false);
                Reset();
            }
    
            void JustSummoned(Creature* summoned) override
            {
                summons.Summon(summoned);
            }
    
            void SetGroupQuestStatus(bool complete) 
            {
                Player* player = ObjectAccessor::GetPlayer(*me, playerGUID);
                if (!player)
                    return;
    
                if (Group* group = player->GetGroup())
                {
                    for (GroupReference* groupRef = group->GetFirstMember(); groupRef != NULL; groupRef = groupRef->next())
                        if (Player* member = groupRef->GetSource())
                            if (member->GetQuestStatus(QUEST_GERENZOS_ORDER) == QUEST_STATUS_INCOMPLETE)
                            {
                                if (complete)
                                    member->CompleteQuest(QUEST_GERENZOS_ORDER);
                                else
                                    member->FailQuest(QUEST_GERENZOS_ORDER);
                            }
                }
                else
                {
                    if (player->GetQuestStatus(QUEST_GERENZOS_ORDER) == QUEST_STATUS_INCOMPLETE)
                    {
                        if (complete)
                            player->CompleteQuest(QUEST_GERENZOS_ORDER);
                        else
                            player->FailQuest(QUEST_GERENZOS_ORDER);
                    }
                }
            }
    
            void MustResetCheck()
            {
                bool allDeadOrNoPlayer = true;
    
                std::list<Player*> players;
                me->GetPlayerListInGrid(players, 100.f);
                for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    Player* player = *itr;
                    if (player->GetQuestStatus(QUEST_GERENZOS_ORDER) != QUEST_STATUS_INCOMPLETE)
                        return;
    
                    float dist = me->GetDistance2d(player);
                    if (dist <= 50.0f)
                        allDeadOrNoPlayer = false;
                    else
                        player->FailQuest(QUEST_GERENZOS_ORDER);
                }
    
                if (allDeadOrNoPlayer)
                    Reset();
            }
    
            void UpdateAI(uint32 diff) override
            {
                if (!start || !playerGUID)
                    return;
    
                if (waitTimer <= diff)
                {
                    switch (step)
                    {
                        case 0:
                        case 1:
                            for (uint8 i = 0; i < 3; ++i)
                                me->SummonCreature(NPC_WINDSHEAR_DIGGER, PinzikSummonPositions[i], TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 120 * IN_MILLISECONDS);
                            waitTimer = step ? 20 * IN_MILLISECONDS : 25 * IN_MILLISECONDS;
                            ++step;
                        case 2:
                            summons.RemoveNotExisting();
                            if (summons.empty())
                            {
                                SetGroupQuestStatus(true);
                                Reset();
                            }
                            waitTimer = 2 * IN_MILLISECONDS;
                    }
                }
                else
                    waitTimer -= diff;
            }
            
        private:
            bool start;
            uint32 waitTimer;
            uint8 step;
            ObjectGuid playerGUID;
            SummonList summons;
        };
        
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_piznikAI (creature);
        }
};

enum CovertOpsAlphaBetaMisc
{
    // Spells
    SPELL_SET_NG_5_CHARGE_RED   = 6630,
    SPELL_SET_NG_5_CHARGE_BLUE  = 6626,
    SPELL_REMOTE_DETONATOR_RED  = 6627,
    SPELL_REMOTE_DETONATOR_BLUE = 6656,

    // Gameobjects
    GO_NG_5_EXPLOSIVES_RED      = 19592,
    GO_NG_5_EXPLOSIVES_BLUE     = 19601,
    GO_SPELLFOCUS_RED           = 19600,
    GO_SPELLFOCUS_BLUE          = 19591,
    VENTURE_WAGON_1             = 20899,
    VENTURE_WAGON_2             = 19547,
};

class spell_set_ng_5_charge : public SpellScriptLoader
{
    public:
        spell_set_ng_5_charge() : SpellScriptLoader("spell_set_ng_5_charge") { }

        class spell_set_ng_5_charge_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_set_ng_5_charge_SpellScript);

            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                { 
                    if (caster->FindNearestGameObject(GO_SPELLFOCUS_RED, 100.0f))
                        caster->SummonGameObject(GO_NG_5_EXPLOSIVES_RED, caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ(), caster->GetOrientation(), 0, 0, 0, 0, 300 * IN_MILLISECONDS);
                    else if (caster->FindNearestGameObject(GO_SPELLFOCUS_BLUE, 100.0f))
                        caster->SummonGameObject(GO_NG_5_EXPLOSIVES_BLUE, caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ(), caster->GetOrientation(), 0, 0, 0, 0, 300 * IN_MILLISECONDS);
                }
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_set_ng_5_charge_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_set_ng_5_charge_SpellScript();
        }
};

class spell_remote_detonator : public SpellScriptLoader
{
    public:
        spell_remote_detonator() : SpellScriptLoader("spell_remote_detonator") { }

        class spell_remote_detonator_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_remote_detonator_SpellScript);

            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->FindNearestGameObject(GO_SPELLFOCUS_RED, 100.0f))
                    { 
                        if (GameObject* trap = caster->FindNearestGameObject(VENTURE_WAGON_1, 100.0f))
                            trap->UseDoorOrButton();
                    }
                    else if (caster->FindNearestGameObject(GO_SPELLFOCUS_BLUE, 100.0f))
                    {
                        if (GameObject* trap = caster->FindNearestGameObject(VENTURE_WAGON_2, 100.0f))
                            trap->UseDoorOrButton();
                    }
                }
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_remote_detonator_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_remote_detonator_SpellScript();
        }
};

void AddSC_stonetalon_mountains_rg()
{
    new npc_piznik();
    new spell_set_ng_5_charge();
    new spell_remote_detonator();
}
