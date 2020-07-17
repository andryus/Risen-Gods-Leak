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
#include "SpellScript.h"

enum EpicHunterQuestMisc
{
    // Quests
    QUEST_HUNTER_BOW_QUEST      = 7636,
    
    // Creatures
    NPC_SIMONE                  = 14527,
    NPC_FRANKLIN                = 14529,
    NPC_NELSON                  = 14536,
    NPC_ARTORIUS                = 14531,
    NPC_BAD_SIMONE              = 14533,
    NPC_BAD_FRANKLIN            = 14534,
    NPC_BAD_NELSON              = 14530,
    NPC_BAD_ARTORIUS            = 14535,
};

#define GOSSIP_ITEM_HUNTER   "Ah, edler Waldläufer. Ihr seid alleine gekommen?"

class npc_hunter_epic_quest_general : public CreatureScript
{
    public:
        npc_hunter_epic_quest_general() : CreatureScript("npc_hunter_epic_quest_general") { }
    
        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
        {
            player->PlayerTalkClass->ClearMenus();
    
            if (action == GOSSIP_ACTION_INFO_DEF)
            {
                    switch (creature->GetEntry())
                    {
                        case NPC_SIMONE:
                            creature->UpdateEntry(NPC_BAD_SIMONE);
                            break;
                        case NPC_FRANKLIN:
                            creature->UpdateEntry(NPC_BAD_FRANKLIN);
                            break;
                        case NPC_NELSON:
                            creature->UpdateEntry(NPC_BAD_NELSON);
                            break;
                        case NPC_ARTORIUS:
                            creature->UpdateEntry(NPC_BAD_ARTORIUS);
                            break;
                    }
                    player->CLOSE_GOSSIP_MENU();
            }
            return true;
        }
    
        bool OnGossipHello(Player* player, Creature* creature) override
        {
            if (player->GetQuestStatus(QUEST_HUNTER_BOW_QUEST) == QUEST_STATUS_INCOMPLETE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_HUNTER, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
    
            player->SEND_GOSSIP_MENU(68, creature->GetGUID());
    
            return true;
        }
};

enum TheBaitforLarkorwiMisc
{
    GO_PRESERVED_THRESHADON_CARCASS = 169216
};

class spell_place_threshadon_carcass : public SpellScriptLoader
{
    public:
        spell_place_threshadon_carcass() : SpellScriptLoader("spell_place_threshadon_carcass") { }

        class spell_place_threshadon_carcass_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_place_threshadon_carcass_SpellScript);

            void HandleAfterCast()
            {
                GetCaster()->SummonGameObject(GO_PRESERVED_THRESHADON_CARCASS, -7198.09f, -2425.0f, -216.693f, 4.30284f, 0, 0, 0, 0, 300 * IN_MILLISECONDS);
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_place_threshadon_carcass_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_place_threshadon_carcass_SpellScript();
        }
};

void AddSC_ungoro_crater_rg()
{
    new npc_hunter_epic_quest_general();
    new spell_place_threshadon_carcass();
}
