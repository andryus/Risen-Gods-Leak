/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2014 ScriptDev2 <https://rising-gods.de/>
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
#include "ArenaTeam.h"
#include "ArenaTeamMgr.h"
#include "Battleground.h"
#include "GameObjectAI.h"
#include "Player.h"
#include "Pet.h"
#include "SpellInfo.h"
#include "SpellHistory.h"

#ifdef __ITEM_TRANSFER__

bool IsT2Item(uint32 item)
{
    switch (item)
    {
        case 45967:
        case 45964:
        case 45950:
        case 45938:
        case 45971:
        case 45955:
        case 45969:
        case 45952:
        case 45957:
        case 45951:
        case 45962:
        case 45960:
        case 45956:
        case 45968:
        case 45963:
        case 45970:
        case 45939:
        case 45954:
        case 45953:
        case 45959:
        case 45961:
        case 45965:
        case 45937:
        case 45958:
        case 45949:
        case 45948:
        case 45966:
            return true;
        default:
            return false;
    }
}

uint32 GetT1Equivalent(uint32 T2Item)
{
    switch (T2Item)
    {
        case 45967: return 42256;
        case 45964: return 42281;
        case 45950: return 42333;
        case 45938: return 42491;
        case 45971: return 42353;
        case 45955: return 42364;
        case 45969: return 42261;
        case 45952: return 42391;
        case 45957: return 42209;
        case 45951: return 42328;
        case 45962: return 42249;
        case 45960: return 42286;
        case 45956: return 42385;
        case 45968: return 42266;
        case 45963: return 42271;
        case 45970: return 42347;
        case 45939: return 42496;
        case 45954: return 44422;
        case 45953: return 44421;
        case 45959: return 42276;
        case 45961: return 42228;
        case 45965: return 42291;
        case 45937: return 42486;
        case 45958: return 42243;
        case 45949: return 42323;
        case 45948: return 42318;
        case 45966: return 42233;
        default: return 0;
    }
}

#define GOSSIP_ADD_ITEMS "Die Items vom Arenarealm zu meinem Inventar hinreturnfuegen"

class npc_arenarealm_transfer_items : public CreatureScript
{
public:
    npc_arenarealm_transfer_items() : CreatureScript("npc_arenarealm_transfer_items") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (player->getLevel() == 80)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ADD_ITEMS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        player->SEND_GOSSIP_MENU(13583, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        // gossip stuff
        player->PlayerTalkClass->ClearMenus();

        if (action != GOSSIP_ACTION_INFO_DEF)
            return true;

        if (player->getLevel() < 80)
            return false;

        uint32 accountId = player->GetSession()->GetAccountId();
        uint32 classId = player->getClass();

        // select the items from rg database
        QueryResult result = RGDatabase.PQuery("SELECT itemId FROM rg_arenarealm_items WHERE accountId = %u AND class = %u", accountId, classId);

        if (!result)
        {
            // player has no items from arenarealm or already got his items
            player->MonsterWhisper("Euch stehen leider keine Gegenstaende zur Verfuegung.", player->GetGUID());
            return true;
        }

        uint32 count = 0;
        std::list<uint32> items;
        items.clear();
        do
        {
            Field* field = result->Fetch();
            uint32 item = field[0].GetInt32();
            // check for t2 weapon, if yes get t1 equivalent
            items.push_back(item);

            ++count;

        } while (result->NextRow());

        // check if player has enought space in inventory
        uint32 noSpaceForCount = 0;
        ItemPosCountVec dest;
        // take any random item. we just want to check if character has free slots
        InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, 33371, count, &noSpaceForCount);
        if (msg != EQUIP_ERR_OK)
        {
            player->MonsterWhisper("Ihr braucht mehr Platz in eurem Inventar.", player->GetGUID());
            return true;
        }

        // everything ok, add the items to player
        for (std::list<uint32>::iterator itr = items.begin(); itr != items.end(); ++itr)
        {
            RGDatabase.DirectPExecute("DELETE FROM rg_arenarealm_items WHERE accountId = %u AND class = %u AND itemId = %u", accountId, classId, (*itr));
            RGDatabase.DirectPExecute("INSERT INTO rg_arenarealm_items_distibute VALUES ('%u', '%u', '%u')", accountId, classId, (*itr));
            player->AddItem(IsT2Item(*itr) ? GetT1Equivalent(*itr) : (*itr), 1);
        }

        // save equipment
        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        player->SaveInventoryAndGoldToDB(trans);
        CharacterDatabase.CommitTransaction(trans);

        // close gossip menu
        if (roll_chance_i(10))
            player->AddAura(30298, player);
        player->CLOSE_GOSSIP_MENU();
        return true;
    }

    struct npc_arenarealm_transfer_itemsAI : public ScriptedAI
    {
        npc_arenarealm_transfer_itemsAI(Creature* creature) : ScriptedAI(creature)
        {
            DoCastSelf(48044, true);
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_arenarealm_transfer_itemsAI(creature);
    }
};

#endif // __ITEM_TRANSFER__

enum DuelScriptData
{
    SPELL_HEAL                  = 22806,

    ZONE_DALARAN                = 4395
};

void RemoveDuelSpellCooldowns(Player* player)
{
    bool hasPet = false;
    // pet cooldowns
    if (Pet* pet = player->GetPet())
    {
        hasPet = true;

        // actually clear cooldowns
        pet->GetSpellHistory()->ResetAllCooldowns();
    }

    // remove cooldowns on spells that have <= 5 min CD
    player->GetSpellHistory()->ResetCooldowns([hasPet](SpellHistory::CooldownStorageType::iterator itr) -> bool
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itr->first);
        if (!spellInfo)
            return false;

        // check special conditions
        if (hasPet && spellInfo->Id == 46584)
            return false;

        // jeeves, Mr. Pinchy, MOLL-E, wormhole, stone of elune, Lament of the Highborne
        if (spellInfo->Id == 67826 || spellInfo->Id == 33060 || spellInfo->Id == 54710 || spellInfo->Id == 67833 || spellInfo->Id == 26265 ||
            spellInfo->Id == 73331)
            return false;

        // check if spellentry is present and if the cooldown is less or equal to 10 min
        if (spellInfo->RecoveryTime <= 10 * MINUTE * IN_MILLISECONDS &&
            spellInfo->CategoryRecoveryTime <= 10 * MINUTE * IN_MILLISECONDS)
            return true;
        return false;
    }, true);

    if (player->getClass() == CLASS_PALADIN)
    {
        player->GetSpellHistory()->ResetCooldown(66235); // argent defender heal
        player->RemoveAura(66233); // argent defender debuff
        player->RemoveAura(25771); // Forbearance
    }
}

class DuelEndPlayerScript : public PlayerScript
{
public:
    DuelEndPlayerScript() : PlayerScript("duel_end_script") {}

    void OnDuelEnd(Player* winner, Player* loser, DuelCompleteType /*type*/)
    {
        // Only in Dalaran
        if (winner->GetZoneId() != ZONE_DALARAN)
            return;

        winner->ResetAllPowers();
        winner->CastSpell(winner, SPELL_HEAL, true);
        winner->SetHealth(winner->GetMaxHealth());
        winner->ModifyPower(POWER_MANA, 50000);
        if (Pet* pet = winner->GetPet())
        {
            pet->SetHealth(pet->GetMaxHealth());
            pet->ModifyPower(POWER_MANA, 50000);
        }
        RemoveDuelSpellCooldowns(winner);

        loser->ResetAllPowers();
        loser->CastSpell(loser, SPELL_HEAL, true);
        loser->SetHealth(loser->GetMaxHealth());
        loser->ModifyPower(POWER_MANA, 50000);
        if (Pet* pet = loser->GetPet())
        {
            pet->SetHealth(loser->GetMaxHealth());
            pet->ModifyPower(POWER_MANA, 50000);
        }
        RemoveDuelSpellCooldowns(loser);
    }
};

class go_ready_marker : public GameObjectScript
{
public:
    go_ready_marker() : GameObjectScript("go_ready_marker") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/)
    {
        if (Battleground* bg = player->GetBattleground())
            bg->EventPlayerUsedReadyMarker(player);

        return true;
    }

    struct go_ready_markerAI : public GameObjectAI
    {
        go_ready_markerAI(GameObject* go) : GameObjectAI(go)
        {
            active = true;
            sayTimer = 5*IN_MILLISECONDS;
        }

        void UpdateAI(uint32 diff)
        {
            if (!active)
                return;

            if (sayTimer <= diff)
            {
                go->MonsterSay("Wenn ihr den Arenakampf fruehzeitig starten moechtet, klickt auf den Kristall und die Schlacht wird in 15 Sekunden starten.", 0, 0);
                active = false;
            }
            else
                sayTimer -= diff;
        }

    private:
        bool active;
        uint32 sayTimer;
    };

    GameObjectAI* GetAI(GameObject* go) const
    {
        return new go_ready_markerAI(go);
    }
};

enum T2VendorData
{
    ARENA_SLOT_2V2                  = 0,
    ARENA_SLOT_3V3                  = 1,
    ARENA_SLOT_5V5                  = 2,        // unused

    MIN_T2_ITEM_RATING_2V2          = 2450,
    MIN_T2_ITEM_RATING_3V3          = 2150,

    MIN_SHOULDER_ITEM_RATING_2V2    = 2200,
    MIN_SHOULDER_ITEM_RATING_3V3    = 1900

};

class npc_arena_t2_vendor : public CreatureScript
{
public:
    npc_arena_t2_vendor() : CreatureScript("npc_arena_t2_vendor") { }

    bool OnSendListInventory(Player* player, Creature* /*creature*/)
    {
        uint32 p_rating = 0;
        uint32 t_rating = 0;
        bool canTalk = false;

        if (ArenaTeam* at = sArenaTeamMgr->GetArenaTeamById(player->GetArenaTeamId(ARENA_SLOT_2V2)))
        {
            p_rating = player->GetArenaPersonalRating(ARENA_SLOT_2V2);
            t_rating = at->GetRating();
            p_rating = p_rating < t_rating ? p_rating : t_rating;

            if (p_rating >= MIN_T2_ITEM_RATING_2V2)
                canTalk = true;
        }

        if (ArenaTeam* at = sArenaTeamMgr->GetArenaTeamById(player->GetArenaTeamId(ARENA_SLOT_3V3)))
        {
            p_rating = player->GetArenaPersonalRating(ARENA_SLOT_3V3);
            t_rating = at->GetRating();
            p_rating = p_rating < t_rating ? p_rating : t_rating;

            if (p_rating >= MIN_T2_ITEM_RATING_3V3)
                canTalk = true;
        }

        if (!canTalk)
            player->SendBuyError(BUY_ERR_RANK_REQUIRE, 0, 0, 0);
        return canTalk;
    }
};

class npc_arena_shoulder_vendor : public CreatureScript
{
public:
    npc_arena_shoulder_vendor() : CreatureScript("npc_arena_shoulder_vendor") { }

    bool OnSendListInventory(Player* player, Creature* /*creature*/)
    {
        uint32 p_rating = 0;
        uint32 t_rating = 0;
        bool canTalk = false;

        if (ArenaTeam* at = sArenaTeamMgr->GetArenaTeamById(player->GetArenaTeamId(ARENA_SLOT_2V2)))
        {
            p_rating = player->GetArenaPersonalRating(ARENA_SLOT_2V2);
            t_rating = at->GetRating();
            p_rating = p_rating < t_rating ? p_rating : t_rating;

            if (p_rating >= MIN_SHOULDER_ITEM_RATING_2V2)
                canTalk = true;
        }

        if (ArenaTeam* at = sArenaTeamMgr->GetArenaTeamById(player->GetArenaTeamId(ARENA_SLOT_3V3)))
        {
            p_rating = player->GetArenaPersonalRating(ARENA_SLOT_3V3);
            t_rating = at->GetRating();
            p_rating = p_rating < t_rating ? p_rating : t_rating;

            if (p_rating >= MIN_SHOULDER_ITEM_RATING_3V3)
                canTalk = true;
        }

        if (!canTalk)
            player->SendBuyError(BUY_ERR_RANK_REQUIRE, 0, 0, 0);
        return canTalk;
    }
};

void AddSC_arena_scripts()
{
  //new npc_arenarealm_transfer_items();
    new DuelEndPlayerScript();
    new go_ready_marker();
    new npc_arena_t2_vendor();
    new npc_arena_shoulder_vendor();
}
