/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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

#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "icecrown_citadel.h"
#include "Spell.h"
#include "Player.h"

#define GOSSIP_OPTION_TEXT 11014

enum ICCTeleportOptions
{
    LIGHT           = 0,
    ORATORY         = 1,
    RAMPART         = 2,
    DEATHBRINGER    = 3,
    UPPER           = 4,
    SINDRAGOSA      = 5,
};

class icecrown_citadel_teleport : public GameObjectScript
{
    public:
        icecrown_citadel_teleport() : GameObjectScript("icecrown_citadel_teleport") { }

        bool OnGossipHello(Player* player, GameObject* go)
        {
            player->AddGossipItem(GOSSIP_OPTION_TEXT, LIGHT, GOSSIP_SENDER_MAIN, LIGHT_S_HAMMER_TELEPORT);
            if (InstanceScript* instance = go->GetInstanceScript())
            {
                if (instance->GetBossState(DATA_LORD_MARROWGAR) == DONE || player->IsGameMaster())
                    player->AddGossipItem(GOSSIP_OPTION_TEXT, ORATORY, GOSSIP_SENDER_MAIN, ORATORY_OF_THE_DAMNED_TELEPORT);
                if (instance->GetBossState(DATA_LADY_DEATHWHISPER) == DONE || player->IsGameMaster())
                    player->AddGossipItem(GOSSIP_OPTION_TEXT, RAMPART, GOSSIP_SENDER_MAIN, RAMPART_OF_SKULLS_TELEPORT);
                if (instance->GetBossState(DATA_ICECROWN_GUNSHIP_BATTLE) == DONE || player->IsGameMaster())
                    player->AddGossipItem(GOSSIP_OPTION_TEXT, DEATHBRINGER, GOSSIP_SENDER_MAIN, DEATHBRINGER_S_RISE_TELEPORT);
                if (instance->GetData(DATA_COLDFLAME_JETS) == DONE || player->IsGameMaster())
                    player->AddGossipItem(GOSSIP_OPTION_TEXT, UPPER, GOSSIP_SENDER_MAIN, UPPER_SPIRE_TELEPORT);
                if (instance->GetData(DATA_SINDRAGOSA_SPAWN) == 1 || player->IsGameMaster())
                    player->AddGossipItem(GOSSIP_OPTION_TEXT, SINDRAGOSA, GOSSIP_SENDER_MAIN, SINDRAGOSA_S_LAIR_TELEPORT);
            }

            player->SEND_GOSSIP_MENU(go->GetGOInfo()->GetGossipMenuId(), go->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, GameObject* /*go*/, uint32 sender, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();
            SpellInfo const* spell = sSpellMgr->GetSpellInfo(action);
            if (!spell)
                return false;

            if (InstanceScript* instance = player->GetInstanceScript())
                if (instance->IsEncounterInProgress())
                {
                    Spell::SendCastResult(player, spell, 0, SPELL_FAILED_AFFECTING_COMBAT);
                    return true;
                }

            player->ClearInCombat();

            if (sender == GOSSIP_SENDER_MAIN)
                player->CastSpell(player, spell, true);

            return true;
        }
};

class at_frozen_throne_teleport : public AreaTriggerScript
{
    public:
        at_frozen_throne_teleport() : AreaTriggerScript("at_frozen_throne_teleport") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            if (player->IsInCombat())
            {
                if (SpellInfo const* spell = sSpellMgr->GetSpellInfo(FROZEN_THRONE_TELEPORT))
                    Spell::SendCastResult(player, spell, 0, SPELL_FAILED_AFFECTING_COMBAT);
                return true;
            }

            if (InstanceScript* instance = player->GetInstanceScript())
            {
                if (player->GetMap()->IsHeroic())
                {
                    if (instance->GetData(DATA_PROFESSOR_PUTRICIDE_HC) == 1 &&
                        instance->GetData(DATA_BLOOD_QUEEN_LANA_THEL_HC) == 1 &&
                        instance->GetData(DATA_SINDRAGOSA_HC) == 1 &&
                        instance->GetBossState(DATA_THE_LICH_KING) != IN_PROGRESS)

                        player->CastSpell(player, FROZEN_THRONE_TELEPORT, true);
                }
                else // if (!player->GetMap()->IsHeroic())
                {
                    if (instance->GetData(DATA_MAX_ATTEMPTS) > 20 &&
                        instance->GetBossState(DATA_PROFESSOR_PUTRICIDE) == DONE &&
                        instance->GetBossState(DATA_BLOOD_QUEEN_LANA_THEL) == DONE &&
                        instance->GetBossState(DATA_SINDRAGOSA) == DONE &&
                        instance->GetBossState(DATA_THE_LICH_KING) != IN_PROGRESS)

                        player->CastSpell(player, FROZEN_THRONE_TELEPORT, true);
                }                
            }

            return true;
        }
};

void AddSC_icecrown_citadel_teleport() 
{
    new icecrown_citadel_teleport();
    new at_frozen_throne_teleport();
}
