/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Npc_Taxi
SD%Complete: 0%
SDComment: To be used for taxi NPCs that are located globally.
SDCategory: NPCs
EndScriptData
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "WorldSession.h"

enum GossipsTaxi
{
    GOSSIP_TAXI             = 60194,
    GOSSIP_SUSURRUS         = 0, // "I am ready."
    GOSSIP_NETHER_DRAKE     = 1, // "I'm ready to fly! Take me up, dragon!"
    GOSSIP_BRAZEN           = 2, // "I am ready to go to Durnholde Keep."
    GOSSIP_IRONWING         = 3, // "I'd like to take a flight around Stormwind Harbor."
    GOSSIP_DABIREE1         = 4, // "Fly me to Murketh and Shaadraz Gateways"
    GOSSIP_DABIREE2         = 5, // "Fly me to Shatter Point"
    GOSSIP_WINDBELLOW1      = 6, // "Fly me to The Abyssal Shelf"
    GOSSIP_WINDBELLOW2      = 7, // "Fly me to Honor Point"
    GOSSIP_BRACK1           = 8, // "Fly me to Murketh and Shaadraz Gateways"
    GOSSIP_BRACK2           = 9, // "Fly me to The Abyssal Shelf"
    GOSSIP_BRACK3           = 10, // "Fly me to Spinebreaker Post"
    GOSSIP_IRENA            = 11, // "Fly me to Skettis please"
    GOSSIP_CLOUDBREAKER1    = 12, // "Speaking of action, I've been ordered to undertake an air strike."
    GOSSIP_CLOUDBREAKER2    = 13, // I need to intercept the Dawnblade reinforcements."
    GOSSIP_DRAGONHAWK       = 14, // "<Ride the dragonhawk to Sun's Reach>"
    GOSSIP_VERONIA          = 15, // "Fly me to Manaforge Coruu please"
    GOSSIP_DEESAK           = 16, // "Fly me to Ogri'la please"
    GOSSIP_AFRASASTRASZ1    = 17, // "I would like to take a flight to the ground, Lord Of Afrasastrasz."
    GOSSIP_AFRASASTRASZ2    = 18, // "My Lord, I must go to the upper floor of the temple."
    GOSSIP_TARIOLSTRASZ1    = 19, // "My Lord, I must go to the upper floor of the temple."
    GOSSIP_TARIOLSTRASZ2    = 20, // "Can you spare a drake to travel to Lord Of Afrasastrasz, in the middle of the temple?"
    GOSSIP_TORASTRASZA1     = 21, // "I would like to see Lord Of Afrasastrasz, in the middle of the temple."
    GOSSIP_TORASTRASZA2     = 22, // Yes, Please. I would like to return to the ground floor of the temple."
    GOSSIP_CRIMSONWING      = 23, // "<Ride the gryphons to Survey Alcaz Island>"
    GOSSIP_WILLIAMKEILAR1   = 24, // "Take me to Northpass Tower."
    GOSSIP_WILLIAMKEILAR2   = 25, // "Take me to Eastwall Tower."
    GOSSIP_WILLIAMKEILAR3   = 26 // "Take me to Crown Guard Tower."
};

class npc_taxi : public CreatureScript
{
public:
    npc_taxi() : CreatureScript("npc_taxi") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        switch (creature->GetEntry())
        {
        case 17435: // Azuremyst Isle - Susurrus
            if (player->HasItemCount(23843, 1, true))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_SUSURRUS), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
            break;
        case 20903: // Netherstorm - Protectorate Nether Drake
            if (player->GetQuestStatus(10438) == QUEST_STATUS_INCOMPLETE && player->HasItemCount(29778))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_NETHER_DRAKE), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            break;
        case 18725: // Old Hillsbrad Foothills - Brazen
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_BRAZEN), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            break;
        case 29154: // Stormwind City - Thargold Ironwing
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_IRONWING), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            break;
        case 19409: // Hellfire Peninsula - Wing Commander Dabir'ee
            //Mission: The Murketh and Shaadraz Gateways
            if (player->GetQuestStatus(10146) == QUEST_STATUS_INCOMPLETE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_DABIREE1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

            //Shatter Point
            if (!player->GetQuestRewardStatus(10340))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_DABIREE2), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
            break;
        case 20235: // Hellfire Peninsula - Gryphoneer Windbellow
            //Mission: The Abyssal Shelf || Return to the Abyssal Shelf
            if (player->GetQuestStatus(10163) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(10346) == QUEST_STATUS_INCOMPLETE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_WINDBELLOW1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);

            //Go to the Front
            if (player->GetQuestStatus(10382) != QUEST_STATUS_NONE && !player->GetQuestRewardStatus(10382))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_WINDBELLOW2), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);
            break;
        case 19401: // Hellfire Peninsula - Wing Commander Brack
            //Mission: The Murketh and Shaadraz Gateways
            if (player->GetQuestStatus(10129) == QUEST_STATUS_INCOMPLETE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_BRACK1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 8);

            //Mission: The Abyssal Shelf || Return to the Abyssal Shelf
            if (player->GetQuestStatus(10162) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(10347) == QUEST_STATUS_INCOMPLETE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_BRACK2), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 9);

            //Spinebreaker Post
            if (player->GetQuestStatus(10242) == QUEST_STATUS_COMPLETE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_BRACK3), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 10);
            break;
        case 23413: // Blade's Edge Mountains - Skyguard Handler Irena
            if (player->GetReputationRank(1031) >= REP_HONORED)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_IRENA), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 11);
            break;
        case 25059: // Isle of Quel'Danas - Ayren Cloudbreaker
            if (player->GetQuestStatus(11532) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(11533) == QUEST_STATUS_INCOMPLETE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_CLOUDBREAKER1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 12);

            if (player->GetQuestStatus(11542) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(11543) == QUEST_STATUS_INCOMPLETE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_CLOUDBREAKER2), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 13);
            break;
        case 25236: // Isle of Quel'Danas - Unrestrained Dragonhawk
            if (player->GetQuestStatus(11542) == QUEST_STATUS_COMPLETE || player->GetQuestStatus(11543) == QUEST_STATUS_COMPLETE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_DRAGONHAWK), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 14);
            break;
        case 20162: // Netherstorm - Veronia
            //Behind Enemy Lines
            if (player->GetQuestStatus(10652) != QUEST_STATUS_REWARDED)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_VERONIA), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 15);
            break;
        case 23415: // Terokkar Forest - Skyguard Handler Deesak
            if (player->GetReputationRank(1031) >= REP_HONORED)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_DEESAK), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 16);
            break;
        case 27575: // Dragonblight - Lord Afrasastrasz
            // middle -> ground
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_AFRASASTRASZ1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 17);
            // middle -> top
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_AFRASASTRASZ2), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 18);
            break;
        case 26443: // Dragonblight - Tariolstrasz //need to check if quests are required before gossip available (12123, 12124)
            // ground -> top
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_TARIOLSTRASZ1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 19);
            // ground -> middle
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_TARIOLSTRASZ2), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 20);
            break;
        case 26949: // Dragonblight - Torastrasza
            // top -> middle
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_TORASTRASZA1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 21);
            // top -> ground
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_TORASTRASZA2), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 22);
            break;
        case 23704: // Dustwallow Marsh - Cassa Crimsonwing
            if (player->GetQuestStatus(11142) == QUEST_STATUS_INCOMPLETE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_CRIMSONWING), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 25);
            break;
        case 17209:
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_WILLIAMKEILAR1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 26);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_WILLIAMKEILAR2), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 27);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_TAXI, GOSSIP_WILLIAMKEILAR3), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 28);
            break;
        }

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        switch (action)
        {
        case GOSSIP_ACTION_INFO_DEF:
            //spellId is correct, however it gives flight a somewhat funny effect //TaxiPath 506.
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 32474, true);
            break;
        case GOSSIP_ACTION_INFO_DEF + 1:
            player->CLOSE_GOSSIP_MENU();
            player->ActivateTaxiPathTo(627);                  //TaxiPath 627 (possibly 627+628(152->153->154->155))
            break;
        case GOSSIP_ACTION_INFO_DEF + 2:
            if (!player->HasItemCount(25853))
                player->SEND_GOSSIP_MENU(9780, creature->GetGUID());
            else
            {
                player->CLOSE_GOSSIP_MENU();
                player->ActivateTaxiPathTo(534);              //TaxiPath 534
            }
            break;
        case GOSSIP_ACTION_INFO_DEF + 3:
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 53335, true);               //TaxiPath 1041 (Stormwind Harbor)
            break;
        case GOSSIP_ACTION_INFO_DEF + 4:
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 33768, true);               //TaxiPath 585 (Gateways Murket and Shaadraz)
            break;
        case GOSSIP_ACTION_INFO_DEF + 5:
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 35069, true);               //TaxiPath 612 (Taxi - Hellfire Peninsula - Expedition Point to Shatter Point)
            break;
        case GOSSIP_ACTION_INFO_DEF + 6:
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 33899, true);               //TaxiPath 589 (Aerial Assault Flight (Alliance))
            break;
        case GOSSIP_ACTION_INFO_DEF + 7:
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 35065, true);               //TaxiPath 607 (Taxi - Hellfire Peninsula - Shatter Point to Beach Head)
            break;
        case GOSSIP_ACTION_INFO_DEF + 8:
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 33659, true);               //TaxiPath 584 (Gateways Murket and Shaadraz)
            break;
        case GOSSIP_ACTION_INFO_DEF + 9:
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 33825, true);               //TaxiPath 587 (Aerial Assault Flight (Horde))
            break;
        case GOSSIP_ACTION_INFO_DEF + 10:
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 34578, true);               //TaxiPath 604 (Taxi - Reaver's Fall to Spinebreaker Ridge)
            break;
        case GOSSIP_ACTION_INFO_DEF + 11:
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 41278, true);               //TaxiPath 706
            break;
        case GOSSIP_ACTION_INFO_DEF + 12:
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 45071, true);               //TaxiPath 779
            break;
        case GOSSIP_ACTION_INFO_DEF + 13:
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 45113, true);               //TaxiPath 784
            break;
        case GOSSIP_ACTION_INFO_DEF + 14:
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 45353, true);               //TaxiPath 788
            break;
        case GOSSIP_ACTION_INFO_DEF + 15:
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 34905, true);               //TaxiPath 606
            break;
        case GOSSIP_ACTION_INFO_DEF + 16:
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 41279, true);               //TaxiPath 705 (Taxi - Skettis to Skyguard Outpost)
            break;
        case GOSSIP_ACTION_INFO_DEF + 17:
            player->CLOSE_GOSSIP_MENU();
            player->ActivateTaxiPathTo(882);
            break;
        case GOSSIP_ACTION_INFO_DEF + 18:
            player->CLOSE_GOSSIP_MENU();
            player->ActivateTaxiPathTo(881);
            break;
        case GOSSIP_ACTION_INFO_DEF + 19:
            player->CLOSE_GOSSIP_MENU();
            player->ActivateTaxiPathTo(878);
            break;
        case GOSSIP_ACTION_INFO_DEF + 20:
            player->CLOSE_GOSSIP_MENU();
            player->ActivateTaxiPathTo(883);
            break;
        case GOSSIP_ACTION_INFO_DEF + 21:
            player->CLOSE_GOSSIP_MENU();
            player->ActivateTaxiPathTo(880);
            break;
        case GOSSIP_ACTION_INFO_DEF + 22:
            player->CLOSE_GOSSIP_MENU();
            player->ActivateTaxiPathTo(879);
            break;
        case GOSSIP_ACTION_INFO_DEF + 23:
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 43074, true);               //TaxiPath 736
            break;
        case GOSSIP_ACTION_INFO_DEF + 24:
            player->CLOSE_GOSSIP_MENU();
            //player->ActivateTaxiPathTo(738);
            player->CastSpell(player, 43136, false);
            break;
        case GOSSIP_ACTION_INFO_DEF + 25:
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, 42295, true);
            break;
        case GOSSIP_ACTION_INFO_DEF + 26:
            player->CLOSE_GOSSIP_MENU();
            player->ActivateTaxiPathTo(494);
            break;
        case GOSSIP_ACTION_INFO_DEF + 27:
            player->CLOSE_GOSSIP_MENU();
            player->ActivateTaxiPathTo(495);
            break;
        case GOSSIP_ACTION_INFO_DEF + 28:
            player->CLOSE_GOSSIP_MENU();
            player->ActivateTaxiPathTo(496);
            break;
        }

        return true;
    }
};

void AddSC_npc_taxi()
{
    new npc_taxi;
}
