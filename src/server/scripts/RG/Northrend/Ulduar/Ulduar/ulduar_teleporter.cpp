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
#include "ScriptedGossip.h"
#include "ulduar.h"
#include "InstanceScript.h"
#include "Player.h"
#include "GossipDef.h"

/*
The teleporter appears to be active and stable.

- Expedition Base Camp
- Formation Grounds
- Colossal Forge
- Scrapyard
- Antechamber of Ulduar
- Shattered Walkway
- Conservatory of Life
*/

#define MAP_ULDUAR 603
#define GOSSIP_OPTION_TEXT 10389

enum UlduarTeleportTargets
{
    BASE_CAMP       = 0,
    GROUNDS         = 1,
    FORGE           = 2,
    SCRAPYARD       = 3,
    ANTECHAMBER     = 4,
    WALKWAY         = 5,
    CONSERVATORY    = 6,
    SPARK           = 7,
    MADNESS         = 8,
};

Position const teleportPointsUlduarGOs[9] =
{
    {-706.122f, -92.6024f, 429.876f},   // Base Camp
    {131.248f, -35.3802f, 409.804f},    // Grounds
    {553.233f, -12.3247f, 409.679f},    // Forge
    {926.292f, -11.4635f, 418.595f},    // Scrapyard
    {1498.09f, -24.246f, 420.967f},     // Antechamber
    {1859.45f, -24.1f, 448.9f},         // Walkway
    {2086.27f, -24.3134f, 421.239f},    // Conservatory
    {2517.97f, 2569.10f, 412.69f},      // Spark
    {1855.03f, -11.629f, 334.58f},      // Descent into Madness
};

// Targets nearby entry, possibly the trigger.
enum TeleportSpells
{
    SPELL_EXPEDITION_BASE_CAMP_TELEPORT = 64014,
    SPELL_CONSERVATORY_TELEPORT = 64024,
    SPELL_HALLS_OF_INVENTIONS_TELEPORT = 64025,
    SPELL_HALLS_OF_WINTER_TELEPORT = 64026, //TODO: missing DB row 
    SPELL_DESCEND_INTO_MADNESS_TELEPORT = 64027, //TODO: missing DB row 
    SPELL_COLOSSAL_FORGE_TELEPORT = 64028,
    SPELL_SHATTERED_WALKWAY_TELEPORT = 64029,
    SPELL_ANTECHAMBER_TELEPORT = 64030,
    SPELL_SCRAPYARD_TELEPORT = 64031,
    SPELL_FORMATIONS_GROUNDS_TELEPORT = 64032,
    SPELL_PRISON_OF_YOGG_SARON_TELEPORT = 65042,
    SPELL_ULDUAR_TRAM_TELEPORT = 65061, //TODO: missing DB row 
    SPELL_TELEPORT_VISUAL = 51347,
};

/*
 - English:
 * Teleport to the Expedition Base Camp
 * Teleport to the Formation Grounds
 * Teleport to the Colossal Forge
 * Teleport to the Scrapyard
 * Teleport to the Antechamber of Ulduar
 * Teleport to the Shattered Walkway
 * Teleport to the Conservatory of Life
 * Teleport to the Spark of Imagination
 * Teleport to Descent into Madness
*/

class ulduar_teleporter : public GameObjectScript
{
public:
    ulduar_teleporter() : GameObjectScript("ulduar_teleporter") { }

    bool OnGossipSelect(Player* player, GameObject* /*go*/, uint32 sender, uint32 action)
    {
        if (!player)
            return false;

        player->PlayerTalkClass->ClearMenus();
        if (sender != GOSSIP_SENDER_MAIN)
            return false;

        if (InstanceScript* instance = player->GetInstanceScript())
            if (instance->IsEncounterInProgress())
                return false;

        player->ClearInCombat();

        int pos = action - GOSSIP_ACTION_INFO_DEF;
        if (pos < 0 && pos > MADNESS)
            return false;

        if (pos > 3) // dismount player manually to avoid display bugs
            player->Dismount();

        player->TeleportTo(MAP_ULDUAR, teleportPointsUlduarGOs[pos].GetPositionX(), teleportPointsUlduarGOs[pos].GetPositionY(), teleportPointsUlduarGOs[pos].GetPositionZ(), 0.0f);
        player->CastSpell(player, SPELL_TELEPORT_VISUAL);
        player->CLOSE_GOSSIP_MENU();
        return true;
    }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        player->AddGossipItem(GOSSIP_OPTION_TEXT, BASE_CAMP, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + BASE_CAMP);
        if (InstanceScript* instance = go->GetInstanceScript())
        {
            //count of 2 Collossus death
            if (instance->GetData(DATA_COLOSSUS) >= 2 || player->IsGameMaster())
                player->AddGossipItem(GOSSIP_OPTION_TEXT, GROUNDS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + GROUNDS);

            // Leviathan death
            if (instance->GetBossState(BOSS_LEVIATHAN) == DONE || player->IsGameMaster())
                player->AddGossipItem(GOSSIP_OPTION_TEXT, FORGE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + FORGE);

            // XT002 death
            if (instance->GetBossState(BOSS_XT002) == DONE || player->IsGameMaster())
            {
                player->AddGossipItem(GOSSIP_OPTION_TEXT, SCRAPYARD, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + SCRAPYARD);
                player->AddGossipItem(GOSSIP_OPTION_TEXT, ANTECHAMBER, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + ANTECHAMBER);
            }

            //Kologan death
            if (instance->GetBossState(BOSS_KOLOGARN) == DONE || player->IsGameMaster())
                player->AddGossipItem(GOSSIP_OPTION_TEXT, WALKWAY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + WALKWAY);

            //Auriaya death
            if (instance->GetBossState(BOSS_AURIAYA) == DONE || player->IsGameMaster())
                player->AddGossipItem(GOSSIP_OPTION_TEXT, CONSERVATORY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + CONSERVATORY);

            //Mimiron death or after first time attacked
            if (instance->GetBossState(BOSS_MIMIRON) == DONE || instance->GetData(DATA_MIMIRON_TELEPORTER) || player->IsGameMaster())
                player->AddGossipItem(GOSSIP_OPTION_TEXT, SPARK, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + SPARK);

            //Vezax death
            if (instance->GetBossState(BOSS_VEZAX) == DONE || player->IsGameMaster())
                player->AddGossipItem(GOSSIP_OPTION_TEXT, MADNESS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + MADNESS);
        }
        player->SEND_GOSSIP_MENU(go->GetGOInfo()->GetGossipMenuId(), go->GetGUID());
        return true;
    }

};

void AddSC_ulduar_teleporter()
{
    new ulduar_teleporter();
}
