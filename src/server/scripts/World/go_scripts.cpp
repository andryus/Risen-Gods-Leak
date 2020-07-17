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

/* ContentData
go_cat_figurine (the "trap" version of GO, two different exist)
go_barov_journal
go_ethereum_prison
go_ethereum_stasis
go_sacred_fire_of_life
go_shrine_of_the_birds
go_southfury_moonstone
go_orb_of_command
go_resonite_cask
go_tablet_of_madness
go_tablet_of_the_seven
go_tele_to_dalaran_crystal
go_tele_to_violet_stand
go_scourge_cage
go_jotunheim_cage
go_table_theka
go_soulwell
go_bashir_crystalforge
go_soulwell
go_dragonflayer_cage
go_amberpine_outhouse
go_hive_pod
go_gjalerbron_cage
go_large_gjalerbron_cage
go_veil_skith_cage
go_toy_train_set
go_bells
EndContentData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "GameObjectAI.h"
#include "Spell.h"
#include "Player.h"
#include "WorldSession.h"
#include "Group.h"
#include "GameTime.h"

/*######
## go_cat_figurine
######*/

enum CatFigurine
{
    SPELL_SUMMON_GHOST_SABER    = 5968,
};

class go_cat_figurine : public GameObjectScript
{
public:
    go_cat_figurine() : GameObjectScript("go_cat_figurine") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/) override
    {
        player->CastSpell(player, SPELL_SUMMON_GHOST_SABER, true);
        return false;
    }
};

/*######
## go_barov_journal
######*/

class go_barov_journal : public GameObjectScript
{
public:
    go_barov_journal() : GameObjectScript("go_barov_journal") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/) override
    {
        if (player->HasSkill(SKILL_TAILORING) && player->GetBaseSkillValue(SKILL_TAILORING) >= 280 && !player->HasSpell(26086))
            player->CastSpell(player, 26095, false);

        return true;
    }
};

/*######
## go_gilded_brazier (Paladin First Trail quest (9678))
######*/

enum GildedBrazier
{
    NPC_STILLBLADE        = 17716,
    QUEST_THE_FIRST_TRIAL = 9678
};

class go_gilded_brazier : public GameObjectScript
{
public:
    go_gilded_brazier() : GameObjectScript("go_gilded_brazier") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (go->GetGoType() == GAMEOBJECT_TYPE_GOOBER)
        {
            if (player->GetQuestStatus(QUEST_THE_FIRST_TRIAL) == QUEST_STATUS_INCOMPLETE)
            {
                if (Creature* Stillblade = player->SummonCreature(NPC_STILLBLADE, 8106.11f, -7542.06f, 151.775f, 3.02598f, TEMPSUMMON_DEAD_DESPAWN, 60000))
                    Stillblade->AI()->AttackStart(player);
            }
        }
        return true;
    }
};

/*######
## go_orb_of_command
######*/

class go_orb_of_command : public GameObjectScript
{
public:
    go_orb_of_command() : GameObjectScript("go_orb_of_command") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/) override
    {
        if (player->GetQuestRewardStatus(7761))
            player->TeleportTo(469, -7673.029785f, -1106.079956f, 396.651001f, 0.703353f);

        return true;
    }
};

/*######
## go_tablet_of_madness
######*/

class go_tablet_of_madness : public GameObjectScript
{
public:
    go_tablet_of_madness() : GameObjectScript("go_tablet_of_madness") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/) override
    {
        if (player->HasSkill(SKILL_ALCHEMY) && player->GetSkillValue(SKILL_ALCHEMY) >= 300 && !player->HasSpell(24266))
            player->CastSpell(player, 24267, false);

        return true;
    }
};

/*######
## go_tablet_of_the_seven
######*/

class go_tablet_of_the_seven : public GameObjectScript
{
public:
    go_tablet_of_the_seven() : GameObjectScript("go_tablet_of_the_seven") { }

    /// @todo use gossip option ("Transcript the Tablet") instead, if Trinity adds support.
    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (go->GetGoType() != GAMEOBJECT_TYPE_QUESTGIVER)
            return true;

        if (player->GetQuestStatus(4296) == QUEST_STATUS_INCOMPLETE)
            player->CastSpell(player, 15065, false);

        return true;
    }
};

/*#####
## go_jump_a_tron
######*/

class go_jump_a_tron : public GameObjectScript
{
public:
    go_jump_a_tron() : GameObjectScript("go_jump_a_tron") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/) override
    {
        if (player->GetQuestStatus(10111) == QUEST_STATUS_INCOMPLETE)
            player->CastSpell(player, 33382, true);

        return true;
    }
};

/*######
## go_ethereum_prison
######*/

enum EthereumPrison
{
    SPELL_REP_LC        = 39456,
    SPELL_REP_SHAT      = 39457,
    SPELL_REP_CE        = 39460,
    SPELL_REP_CON       = 39474,
    SPELL_REP_KT        = 39475,
    SPELL_REP_SPOR      = 39476
};

const uint32 NpcPrisonEntry[] =
{
    22810, 22811, 22812, 22813, 22814, 22815,               //good guys
    20783, 20784, 20785, 20786, 20788, 20789, 20790         //bad guys
};

class go_ethereum_prison : public GameObjectScript
{
public:
    go_ethereum_prison() : GameObjectScript("go_ethereum_prison") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        go->UseDoorOrButton();
        int Random = rand32() % (sizeof(NpcPrisonEntry) / sizeof(uint32));

        if (Creature* creature = player->SummonCreature(NpcPrisonEntry[Random], go->GetPositionX(), go->GetPositionY(), go->GetPositionZ(), go->GetAngle(player),
            TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000))
        {
            if (!creature->IsHostileTo(player))
            {
                if (FactionTemplateEntry const* pFaction = creature->GetFactionTemplateEntry())
                {
                    uint32 Spell = 0;

                    switch (pFaction->faction)
                    {
                        case 1011: Spell = SPELL_REP_LC; break;
                        case 935: Spell = SPELL_REP_SHAT; break;
                        case 942: Spell = SPELL_REP_CE; break;
                        case 933: Spell = SPELL_REP_CON; break;
                        case 989: Spell = SPELL_REP_KT; break;
                        case 970: Spell = SPELL_REP_SPOR; break;
                    }

                    if (Spell)
                        player->CastSpell(player, Spell, false);
                    else
                        TC_LOG_ERROR("scripts", "go_ethereum_prison summoned Creature (entry %u) but faction (%u) are not expected by script.", creature->GetEntry(), creature->getFaction());
                }
            }
        }

        return false;
    }
};

/*######
## go_ethereum_stasis
######*/

const uint32 NpcStasisEntry[] =
{
    22825, 20888, 22827, 22826, 22828
};

class go_ethereum_stasis : public GameObjectScript
{
public:
    go_ethereum_stasis() : GameObjectScript("go_ethereum_stasis") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        go->UseDoorOrButton();
        int Random = rand32() % (sizeof(NpcStasisEntry) / sizeof(uint32));

        player->SummonCreature(NpcStasisEntry[Random], go->GetPositionX(), go->GetPositionY(), go->GetPositionZ(), go->GetAngle(player),
            TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);

        return false;
    }
};

/*######
## go_resonite_cask
######*/

enum ResoniteCask
{
    NPC_GOGGEROC    = 11920
};

class go_resonite_cask : public GameObjectScript
{
public:
    go_resonite_cask() : GameObjectScript("go_resonite_cask") { }

    bool OnGossipHello(Player* /*player*/, GameObject* go) override
    {
        if (go->GetGoType() == GAMEOBJECT_TYPE_GOOBER)
            go->SummonCreature(NPC_GOGGEROC, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 300000);

        return false;
    }
};

/*######
## go_sacred_fire_of_life
######*/

enum SacredFireOfLife
{
    NPC_ARIKARA     = 10882
};

class go_sacred_fire_of_life : public GameObjectScript
{
public:
    go_sacred_fire_of_life() : GameObjectScript("go_sacred_fire_of_life") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (go->GetGoType() == GAMEOBJECT_TYPE_GOOBER)
            player->SummonCreature(NPC_ARIKARA, -5008.338f, -2118.894f, 83.657f, 0.874f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);

        return true;
    }
};

/*######
## go_shrine_of_the_birds
######*/

enum ShrineOfTheBirds
{
    NPC_HAWK_GUARD      = 22992,
    NPC_EAGLE_GUARD     = 22993,
    NPC_FALCON_GUARD    = 22994,
    GO_SHRINE_HAWK      = 185551,
    GO_SHRINE_EAGLE     = 185547,
    GO_SHRINE_FALCON    = 185553
};

class go_shrine_of_the_birds : public GameObjectScript
{
public:
    go_shrine_of_the_birds() : GameObjectScript("go_shrine_of_the_birds") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        uint32 BirdEntry = 0;

        float fX, fY, fZ;
        go->GetClosePoint(fX, fY, fZ, go->GetObjectSize(), INTERACTION_DISTANCE);

        switch (go->GetEntry())
        {
            case GO_SHRINE_HAWK:
                BirdEntry = NPC_HAWK_GUARD;
                break;
            case GO_SHRINE_EAGLE:
                BirdEntry = NPC_EAGLE_GUARD;
                break;
            case GO_SHRINE_FALCON:
                BirdEntry = NPC_FALCON_GUARD;
                break;
        }

        if (BirdEntry)
            player->SummonCreature(BirdEntry, fX, fY, fZ, go->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);

        return false;
    }
};

/*######
## go_southfury_moonstone
######*/

enum Southfury
{
    NPC_RIZZLE                  = 23002,
    SPELL_BLACKJACK             = 39865, //stuns player
    SPELL_SUMMON_RIZZLE         = 39866
};

class go_southfury_moonstone : public GameObjectScript
{
public:
    go_southfury_moonstone() : GameObjectScript("go_southfury_moonstone") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/) override
    {
        //implicitTarget=48 not implemented as of writing this code, and manual summon may be just ok for our purpose
        //player->CastSpell(player, SPELL_SUMMON_RIZZLE, false);

        if (Creature* creature = player->SummonCreature(NPC_RIZZLE, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0))
            creature->CastSpell(player, SPELL_BLACKJACK, false);

        return false;
    }
};

/*######
## go_tele_to_dalaran_crystal
######*/

enum DalaranCrystal
{
    QUEST_LEARN_LEAVE_RETURN    = 12790,
    QUEST_TELE_CRYSTAL_FLAG     = 12845
};

#define GO_TELE_TO_DALARAN_CRYSTAL_FAILED   "This teleport crystal cannot be used until the teleport crystal in Dalaran has been used at least once."

class go_tele_to_dalaran_crystal : public GameObjectScript
{
public:
    go_tele_to_dalaran_crystal() : GameObjectScript("go_tele_to_dalaran_crystal") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/) override
    {
        if (player->HasAuraType(SPELL_AURA_FLY))
            player->RemoveAurasByType(SPELL_AURA_FLY);
        
        if (player->HasAuraType(SPELL_AURA_MOUNTED))
            player->RemoveAurasByType(SPELL_AURA_MOUNTED);

        if (player->GetQuestRewardStatus(QUEST_TELE_CRYSTAL_FLAG))
            return false;

        player->GetSession()->SendNotification(GO_TELE_TO_DALARAN_CRYSTAL_FAILED);

        return true;
    }
};

/*######
## go_portal_shatt_quel
######*/

enum PortalspellQuel
{
    SPELL_PORT_QUEL       = 44876,
};

class go_portal_shatt_quel : public GameObjectScript
{
public:
    go_portal_shatt_quel() : GameObjectScript("go_portal_shatt_quel") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/)
    {
        if (player->HasAuraType(SPELL_AURA_FLY))
            player->RemoveAurasByType(SPELL_AURA_FLY);
        
        if (player->HasAuraType(SPELL_AURA_MOUNTED))
            player->RemoveAurasByType(SPELL_AURA_MOUNTED);
        
        player->CastSpell(player, SPELL_PORT_QUEL, true);
        return true;
    }
};

/*######
## go_portal_shatt_exo
######*/

enum PortalspellExodar
{
    SPELL_PORT_EXODAR       = 32268,
};

class go_portal_shatt_exo : public GameObjectScript
{
public:
    go_portal_shatt_exo() : GameObjectScript("go_portal_shatt_exo") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/)
    {
        if (player->HasAuraType(SPELL_AURA_FLY))
            player->RemoveAurasByType(SPELL_AURA_FLY);
        
        if (player->HasAuraType(SPELL_AURA_MOUNTED))
            player->RemoveAurasByType(SPELL_AURA_MOUNTED);
        
        player->CastSpell(player, SPELL_PORT_EXODAR, true);
        return true;
    }
};

/*######
## go_portal_shatt_silvermoon
######*/

enum PortalspellSilvermoon
{
    SPELL_PORT_SILVERMOON       = 32270,
};

class go_portal_shatt_silvermoon : public GameObjectScript
{
public:
    go_portal_shatt_silvermoon() : GameObjectScript("go_portal_shatt_silvermoon") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/)
    {
        if (player->HasAuraType(SPELL_AURA_FLY))
            player->RemoveAurasByType(SPELL_AURA_FLY);
        
        if (player->HasAuraType(SPELL_AURA_MOUNTED))
            player->RemoveAurasByType(SPELL_AURA_MOUNTED);

        player->CastSpell(player, SPELL_PORT_SILVERMOON, true);
        return true;
    }
};

/*######
## go_tele_to_violet_stand
######*/

class go_tele_to_violet_stand : public GameObjectScript
{
public:
    go_tele_to_violet_stand() : GameObjectScript("go_tele_to_violet_stand") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/) override
    {
        if (player->GetQuestRewardStatus(QUEST_LEARN_LEAVE_RETURN) || player->GetQuestStatus(QUEST_LEARN_LEAVE_RETURN) == QUEST_STATUS_INCOMPLETE)
            return false;

        return true;
    }
};

/*######
## go_fel_crystalforge
######*/

#define GOSSIP_FEL_CRYSTALFORGE_TEXT 31000
#define GOSSIP_FEL_CRYSTALFORGE_ITEM_TEXT_RETURN 31001

enum GossipMisc
{
    GOSSIP_FEL_FORGE = 60087
};

enum FelCrystalforge
{
    SPELL_CREATE_1_FLASK_OF_BEAST   = 40964,
    SPELL_CREATE_5_FLASK_OF_BEAST   = 40965,
};

class go_fel_crystalforge : public GameObjectScript
{
public:
    go_fel_crystalforge() : GameObjectScript("go_fel_crystalforge") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (go->GetGoType() == GAMEOBJECT_TYPE_QUESTGIVER) /* != GAMEOBJECT_TYPE_QUESTGIVER) */
            player->PrepareQuestMenu(go->GetGUID()); /* return true*/

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_FEL_FORGE, 0), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_FEL_FORGE, 1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

        player->SEND_GOSSIP_MENU(GOSSIP_FEL_CRYSTALFORGE_TEXT, go->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* player, GameObject* go, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF:
                player->CastSpell(player, SPELL_CREATE_1_FLASK_OF_BEAST, false);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_FEL_FORGE, 2), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                player->SEND_GOSSIP_MENU(GOSSIP_FEL_CRYSTALFORGE_ITEM_TEXT_RETURN, go->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF + 1:
                player->CastSpell(player, SPELL_CREATE_5_FLASK_OF_BEAST, false);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_FEL_FORGE, 2), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                player->SEND_GOSSIP_MENU(GOSSIP_FEL_CRYSTALFORGE_ITEM_TEXT_RETURN, go->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF + 2:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_FEL_FORGE, 0), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_FEL_FORGE, 1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                player->SEND_GOSSIP_MENU(GOSSIP_FEL_CRYSTALFORGE_TEXT, go->GetGUID());
                break;
        }
        return true;
    }
};

/*######
## matrix_punchograph
######*/

enum MatrixPunchograph
{
    ITEM_WHITE_PUNCH_CARD = 9279,
    ITEM_YELLOW_PUNCH_CARD = 9280,
    ITEM_BLUE_PUNCH_CARD = 9282,
    ITEM_RED_PUNCH_CARD = 9281,
    ITEM_PRISMATIC_PUNCH_CARD = 9316,
    SPELL_YELLOW_PUNCH_CARD = 11512,
    SPELL_BLUE_PUNCH_CARD = 11525,
    SPELL_RED_PUNCH_CARD = 11528,
    SPELL_PRISMATIC_PUNCH_CARD = 11545,
    MATRIX_PUNCHOGRAPH_3005_A = 142345,
    MATRIX_PUNCHOGRAPH_3005_B = 142475,
    MATRIX_PUNCHOGRAPH_3005_C = 142476,
    MATRIX_PUNCHOGRAPH_3005_D = 142696,
};

class go_matrix_punchograph : public GameObjectScript
{
public:
    go_matrix_punchograph() : GameObjectScript("go_matrix_punchograph") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        switch (go->GetEntry())
        {
            case MATRIX_PUNCHOGRAPH_3005_A:
                if (player->HasItemCount(ITEM_WHITE_PUNCH_CARD))
                {
                    player->DestroyItemCount(ITEM_WHITE_PUNCH_CARD, 1, true);
                    player->CastSpell(player, SPELL_YELLOW_PUNCH_CARD, true);
                }
                break;
            case MATRIX_PUNCHOGRAPH_3005_B:
                if (player->HasItemCount(ITEM_YELLOW_PUNCH_CARD))
                {
                    player->DestroyItemCount(ITEM_YELLOW_PUNCH_CARD, 1, true);
                    player->CastSpell(player, SPELL_BLUE_PUNCH_CARD, true);
                }
                break;
            case MATRIX_PUNCHOGRAPH_3005_C:
                if (player->HasItemCount(ITEM_BLUE_PUNCH_CARD))
                {
                    player->DestroyItemCount(ITEM_BLUE_PUNCH_CARD, 1, true);
                    player->CastSpell(player, SPELL_RED_PUNCH_CARD, true);
                }
                break;
            case MATRIX_PUNCHOGRAPH_3005_D:
                if (player->HasItemCount(ITEM_RED_PUNCH_CARD))
                {
                    player->DestroyItemCount(ITEM_RED_PUNCH_CARD, 1, true);
                    player->CastSpell(player, SPELL_PRISMATIC_PUNCH_CARD, true);
                }
                break;
            default:
                break;
        }
        return false;
    }
};

/*######
## go_scourge_cage
######*/

enum ScourgeCage
{
    NPC_SCOURGE_PRISONER = 25610
};

class go_scourge_cage : public GameObjectScript
{
public:
    go_scourge_cage() : GameObjectScript("go_scourge_cage") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        go->UseDoorOrButton();
        if (Creature* pNearestPrisoner = go->FindNearestCreature(NPC_SCOURGE_PRISONER, 5.0f, true))
        {
            player->KilledMonsterCredit(NPC_SCOURGE_PRISONER, pNearestPrisoner->GetGUID());
            pNearestPrisoner->DisappearAndDie();
        }

        return true;
    }
};

/*######
## go_arcane_prison
######*/

enum ArcanePrison
{
    QUEST_PRISON_BREAK                  = 11587,
    SPELL_ARCANE_PRISONER_KILL_CREDIT   = 45456
};

class go_arcane_prison : public GameObjectScript
{
public:
    go_arcane_prison() : GameObjectScript("go_arcane_prison") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (player->GetQuestStatus(QUEST_PRISON_BREAK) == QUEST_STATUS_INCOMPLETE)
        {
            go->SummonCreature(25318, 3485.089844f, 6115.7422188f, 70.966812f, 0, TEMPSUMMON_TIMED_DESPAWN, 60000);
            player->CastSpell(player, SPELL_ARCANE_PRISONER_KILL_CREDIT, true);
            return true;
        }
        return false;
    }
};

/*######
## go_blood_filled_orb
######*/

enum BloodFilledOrb
{
    NPC_ZELEMAR     = 17830

};

class go_blood_filled_orb : public GameObjectScript
{
public:
    go_blood_filled_orb() : GameObjectScript("go_blood_filled_orb") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (go->GetGoType() == GAMEOBJECT_TYPE_GOOBER)
            player->SummonCreature(NPC_ZELEMAR, -369.746f, 166.759f, -21.50f, 5.235f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);

        return true;
    }
};

/*######
## go_jotunheim_cage
######*/

enum JotunheimCage
{
    NPC_EBON_BLADE_PRISONER_HUMAN   = 30186,
    NPC_EBON_BLADE_PRISONER_NE      = 30194,
    NPC_EBON_BLADE_PRISONER_TROLL   = 30196,
    NPC_EBON_BLADE_PRISONER_ORC     = 30195,

    SPELL_SUMMON_BLADE_KNIGHT_H     = 56207,
    SPELL_SUMMON_BLADE_KNIGHT_NE    = 56209,
    SPELL_SUMMON_BLADE_KNIGHT_ORC   = 56212,
    SPELL_SUMMON_BLADE_KNIGHT_TROLL = 56214
};

class go_jotunheim_cage : public GameObjectScript
{
public:
    go_jotunheim_cage() : GameObjectScript("go_jotunheim_cage") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        go->UseDoorOrButton();
        Creature* pPrisoner = go->FindNearestCreature(NPC_EBON_BLADE_PRISONER_HUMAN, 5.0f, true);
        if (!pPrisoner)
        {
            pPrisoner = go->FindNearestCreature(NPC_EBON_BLADE_PRISONER_TROLL, 5.0f, true);
            if (!pPrisoner)
            {
                pPrisoner = go->FindNearestCreature(NPC_EBON_BLADE_PRISONER_ORC, 5.0f, true);
                if (!pPrisoner)
                    pPrisoner = go->FindNearestCreature(NPC_EBON_BLADE_PRISONER_NE, 5.0f, true);
            }
        }
        if (!pPrisoner || !pPrisoner->IsAlive())
            return false;

        pPrisoner->DisappearAndDie();
        player->KilledMonsterCredit(NPC_EBON_BLADE_PRISONER_HUMAN, ObjectGuid::Empty);
        switch (pPrisoner->GetEntry())
        {
            case NPC_EBON_BLADE_PRISONER_HUMAN:
                player->CastSpell(player, SPELL_SUMMON_BLADE_KNIGHT_H, true);
                break;
            case NPC_EBON_BLADE_PRISONER_NE:
                player->CastSpell(player, SPELL_SUMMON_BLADE_KNIGHT_NE, true);
                break;
            case NPC_EBON_BLADE_PRISONER_TROLL:
                player->CastSpell(player, SPELL_SUMMON_BLADE_KNIGHT_TROLL, true);
                break;
            case NPC_EBON_BLADE_PRISONER_ORC:
                player->CastSpell(player, SPELL_SUMMON_BLADE_KNIGHT_ORC, true);
                break;
        }
        return true;
    }
};

enum TableTheka
{
    GOSSIP_TABLE_THEKA = 1653,

    QUEST_SPIDER_GOLD = 2936
};

class go_table_theka : public GameObjectScript
{
public:
    go_table_theka() : GameObjectScript("go_table_theka") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (player->GetQuestStatus(QUEST_SPIDER_GOLD) == QUEST_STATUS_INCOMPLETE)
            player->AreaExploredOrEventHappens(QUEST_SPIDER_GOLD);

        player->SEND_GOSSIP_MENU(GOSSIP_TABLE_THEKA, go->GetGUID());

        return true;
    }
};

/*######
## go_inconspicuous_landmark
######*/

enum InconspicuousLandmark
{
    SPELL_SUMMON_PIRATES_TREASURE_AND_TRIGGER_MOB    = 11462,
    ITEM_CUERGOS_KEY                                 = 9275,
};

class go_inconspicuous_landmark : public GameObjectScript
{
public:
    go_inconspicuous_landmark() : GameObjectScript("go_inconspicuous_landmark") { }

    void OnLootStateChanged(GameObject* /*go*/, uint32 state, Unit* unit) override
    {
        if (state != GO_ACTIVATED || !unit)
            return;

        if (Player* player = unit->ToPlayer())
        {
            if (player->HasItemCount(ITEM_CUERGOS_KEY))
                return;

            player->CastSpell(player, SPELL_SUMMON_PIRATES_TREASURE_AND_TRIGGER_MOB, true);

            return;
        }
    }
};

/*######
## go_soulwell
######*/

enum SoulWellData
{
    GO_SOUL_WELL_R1                     = 181621,
    GO_SOUL_WELL_R2                     = 193169,

    SPELL_IMPROVED_HEALTH_STONE_R1      = 18692,
    SPELL_IMPROVED_HEALTH_STONE_R2      = 18693,

    SPELL_CREATE_MASTER_HEALTH_STONE_R0 = 34130,
    SPELL_CREATE_MASTER_HEALTH_STONE_R1 = 34149,
    SPELL_CREATE_MASTER_HEALTH_STONE_R2 = 34150,

    SPELL_CREATE_FEL_HEALTH_STONE_R0    = 58890,
    SPELL_CREATE_FEL_HEALTH_STONE_R1    = 58896,
    SPELL_CREATE_FEL_HEALTH_STONE_R2    = 58898,
};

class go_soulwell : public GameObjectScript
{
    public:
        go_soulwell() : GameObjectScript("go_soulwell") { }

        struct go_soulwellAI : public GameObjectAI
        {
            go_soulwellAI(GameObject* go) : GameObjectAI(go)
            {
                _stoneSpell = 0;
                _stoneId = 0;
                switch (go->GetEntry())
                {
                    case GO_SOUL_WELL_R1:
                        _stoneSpell = SPELL_CREATE_MASTER_HEALTH_STONE_R0;
                        if (Unit* owner = go->GetOwner())
                        {
                            if (owner->HasAura(SPELL_IMPROVED_HEALTH_STONE_R1))
                                _stoneSpell = SPELL_CREATE_MASTER_HEALTH_STONE_R1;
                            else if (owner->HasAura(SPELL_CREATE_MASTER_HEALTH_STONE_R2))
                                _stoneSpell = SPELL_CREATE_MASTER_HEALTH_STONE_R2;
                        }
                        break;
                    case GO_SOUL_WELL_R2:
                        _stoneSpell = SPELL_CREATE_FEL_HEALTH_STONE_R0;
                        if (Unit* owner = go->GetOwner())
                        {
                            if (owner->HasAura(SPELL_IMPROVED_HEALTH_STONE_R1))
                                _stoneSpell = SPELL_CREATE_FEL_HEALTH_STONE_R1;
                            else if (owner->HasAura(SPELL_CREATE_MASTER_HEALTH_STONE_R2))
                                _stoneSpell = SPELL_CREATE_FEL_HEALTH_STONE_R2;
                        }
                        break;
                }
                if (_stoneSpell == 0) // Should never happen
                    return;

                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(_stoneSpell);
                if (!spellInfo)
                    return;

                _stoneId = spellInfo->Effects[EFFECT_0].ItemType;
            }

            /// Due to the fact that this GameObject triggers CMSG_GAMEOBJECT_USE
            /// _and_ CMSG_GAMEOBJECT_REPORT_USE, this GossipHello hook is called
            /// twice. The script's handling is fine as it won't remove two charges
            /// on the well. We have to find how to segregate REPORT_USE and USE.
            bool GossipHello(Player* player, bool /*reportUse*/) override
            {
                Unit* owner = go->GetOwner();
                if (_stoneSpell == 0 || _stoneId == 0)
                    return true;

                if (!owner || owner->GetTypeId() != TYPEID_PLAYER || !player->IsInSameRaidWith(owner->ToPlayer()))
                    return true;

                // Don't try to add a stone if we already have one.
                if (player->HasItemCount(_stoneId))
                {
                    if (SpellInfo const* spell = sSpellMgr->GetSpellInfo(_stoneSpell))
                        Spell::SendCastResult(player, spell, 0, SPELL_FAILED_TOO_MANY_OF_ITEM);
                    return true;
                }

                owner->CastSpell(player, _stoneSpell, true);
                // Item has to actually be created to remove a charge on the well.
                if (player->HasItemCount(_stoneId))
                    go->AddUse();

                return false;
            }

        private:
            uint32 _stoneSpell;
            uint32 _stoneId;
        };

        GameObjectAI* GetAI(GameObject* go) const override
        {
            return new go_soulwellAI(go);
        }
};



/*######
## go_amberpine_outhouse
######*/

enum AmberpineOuthouse
{
    ITEM_ANDERHOLS_SLIDER_CIDER     = 37247,
    NPC_OUTHOUSE_BUNNY              = 27326,
    QUEST_DOING_YOUR_DUTY           = 12227,
    QUEST_OLYMPIA_EVENT             = 110169,
    SPELL_INDISPOSED                = 53017,
    SPELL_INDISPOSED_III            = 48341,
    SPELL_CREATE_AMBERSEEDS         = 48330,
    GOSSIP_OUTHOUSE_INUSE           = 12775,
    GOSSIP_OUTHOUSE_VACANT          = 12779
};

class go_amberpine_outhouse : public GameObjectScript
{
public:
    go_amberpine_outhouse() : GameObjectScript("go_amberpine_outhouse") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        QuestStatus status = player->GetQuestStatus(QUEST_DOING_YOUR_DUTY);
        QuestStatus status2 = player->GetQuestStatus(QUEST_OLYMPIA_EVENT);
        if ((status == QUEST_STATUS_INCOMPLETE && player->HasItemCount(ITEM_ANDERHOLS_SLIDER_CIDER))
            || status2 == QUEST_STATUS_INCOMPLETE)
        {
            player->ADD_GOSSIP_ITEM_DB(Player::GetDefaultGossipMenuForSource(go), 0, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            player->SEND_GOSSIP_MENU(GOSSIP_OUTHOUSE_VACANT, go->GetGUID());
        }
        else
            player->SEND_GOSSIP_MENU(GOSSIP_OUTHOUSE_INUSE, go->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* player, GameObject* go, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF + 1)
        {
            player->CLOSE_GOSSIP_MENU();
            player->AreaExploredOrEventHappens(QUEST_OLYMPIA_EVENT);
            Creature* target = player->FindNearestCreature(NPC_OUTHOUSE_BUNNY, 3.0f);
            if (target)
            {
                target->AI()->SetData(1, player->getGender());
                go->CastSpell(target, SPELL_INDISPOSED_III);
                go->CastSpell(player, SPELL_INDISPOSED);
            }
            else // Seeds normally created by spell
                go->CastSpell(player, SPELL_CREATE_AMBERSEEDS);

            return true;
        }

        return false;
    }
};

/*######
## Quest 1126: Hive in the Tower
## go_hive_pod
######*/

enum Hives
{
    QUEST_HIVE_IN_THE_TOWER                       = 9544,
    NPC_HIVE_AMBUSHER                             = 13301
};

class go_hive_pod : public GameObjectScript
{
public:
    go_hive_pod() : GameObjectScript("go_hive_pod") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        player->SendLoot(go->GetGUID(), LOOT_CORPSE);

        if (go->FindNearestCreature(NPC_HIVE_AMBUSHER, 20.0f))
            return true;

        go->SummonCreature(NPC_HIVE_AMBUSHER, go->GetPositionX()+1, go->GetPositionY(), go->GetPositionZ(), go->GetAngle(player), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000);
        go->SummonCreature(NPC_HIVE_AMBUSHER, go->GetPositionX(), go->GetPositionY()+1, go->GetPositionZ(), go->GetAngle(player), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000);
        return true;
    }
};

class go_massive_seaforium_charge : public GameObjectScript
{
    public:
        go_massive_seaforium_charge() : GameObjectScript("go_massive_seaforium_charge") { }

        bool OnGossipHello(Player* /*player*/, GameObject* go) override
        {
            go->DespawnOrUnsummon();
            return true;
        }
};

/*######
## go_gjalerbron_cage
######*/

enum OfKeysAndCages
{
    QUEST_ALLIANCE_OF_KEYS_AND_CAGES    = 11231,
    QUEST_HORDE_OF_KEYS_AND_CAGES       = 11265,
    NPC_GJALERBRON_PRISONER             = 24035,
    SAY_FREE                            = 0,
};

class go_gjalerbron_cage : public GameObjectScript
{
    public:
        go_gjalerbron_cage() : GameObjectScript("go_gjalerbron_cage") { }

        bool OnGossipHello(Player* player, GameObject* go) override
        {
            go->UseDoorOrButton();
            if ((player->GetTeamId() == TEAM_ALLIANCE && player->GetQuestStatus(QUEST_ALLIANCE_OF_KEYS_AND_CAGES) == QUEST_STATUS_INCOMPLETE) ||
                (player->GetTeamId() == TEAM_HORDE && player->GetQuestStatus(QUEST_HORDE_OF_KEYS_AND_CAGES) == QUEST_STATUS_INCOMPLETE))
            {
                if (Creature* prisoner = go->FindNearestCreature(NPC_GJALERBRON_PRISONER, 5.0f))
                {
                    player->KilledMonsterCredit(NPC_GJALERBRON_PRISONER, ObjectGuid::Empty);

                    prisoner->AI()->Talk(SAY_FREE);
                    prisoner->DespawnOrUnsummon(6000);
                }
            }
            return true;
        }
};

/*########
## go_large_gjalerbron_cage
#####*/

class go_large_gjalerbron_cage : public GameObjectScript
{
    public:
        go_large_gjalerbron_cage() : GameObjectScript("go_large_gjalerbron_cage") { }

        bool OnGossipHello(Player* player, GameObject* go) override
        {
            go->UseDoorOrButton();
            if ((player->GetTeamId() == TEAM_ALLIANCE && player->GetQuestStatus(QUEST_ALLIANCE_OF_KEYS_AND_CAGES) == QUEST_STATUS_INCOMPLETE) ||
                (player->GetTeamId() == TEAM_HORDE && player->GetQuestStatus(QUEST_HORDE_OF_KEYS_AND_CAGES) == QUEST_STATUS_INCOMPLETE))
            {
                std::list<Creature*> prisonerList;
                go->GetCreatureListWithEntryInGrid(prisonerList, NPC_GJALERBRON_PRISONER, INTERACTION_DISTANCE);
                for (std::list<Creature*>::const_iterator itr = prisonerList.begin(); itr != prisonerList.end(); ++itr)
                {
                    player->KilledMonsterCredit(NPC_GJALERBRON_PRISONER, (*itr)->GetGUID());
                    (*itr)->DespawnOrUnsummon(6000);
                    (*itr)->AI()->Talk(SAY_FREE);
                }
            }
            return false;
        }
};

/*########
#### go_veil_skith_cage
#####*/

enum MissingFriends
{
   QUEST_MISSING_FRIENDS    = 10852,
   NPC_CAPTIVE_CHILD        = 22314,
   NPC_CAPTIVE_CHILD_CREDIT = 22365,
   SAY_FREE_0               = 0,
};

class go_veil_skith_cage : public GameObjectScript
{
    public:
       go_veil_skith_cage() : GameObjectScript("go_veil_skith_cage") { }

       bool OnGossipHello(Player* player, GameObject* go) override
       {
           go->UseDoorOrButton();
           if (player->GetQuestStatus(QUEST_MISSING_FRIENDS) == QUEST_STATUS_INCOMPLETE)
           {
               std::list<Creature*> childrenList;
               go->GetCreatureListWithEntryInGrid(childrenList, NPC_CAPTIVE_CHILD, INTERACTION_DISTANCE);
               for (std::list<Creature*>::const_iterator itr = childrenList.begin(); itr != childrenList.end(); ++itr)
               {
                   player->KilledMonsterCredit(NPC_CAPTIVE_CHILD_CREDIT, ObjectGuid::Empty);
                   (*itr)->DespawnOrUnsummon(5000);
                   (*itr)->GetMotionMaster()->MovePoint(1, go->GetPositionX()+5, go->GetPositionY(), go->GetPositionZ());
                   (*itr)->AI()->Talk(SAY_FREE_0);
                   (*itr)->GetMotionMaster()->Clear();
               }
           }
           return false;
       }
};

/*######
## go_frostblade_shrine
######*/

enum TheCleansing
{
   QUEST_THE_CLEANSING_HORDE      = 11317,
   QUEST_THE_CLEANSING_ALLIANCE   = 11322,
   SPELL_CLEANSING_SOUL           = 43351,
   SPELL_RECENT_MEDITATION        = 61720,
};

class go_frostblade_shrine : public GameObjectScript
{
public:
    go_frostblade_shrine() : GameObjectScript("go_frostblade_shrine") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        go->UseDoorOrButton(10);
        if (!player->HasAura(SPELL_RECENT_MEDITATION))
            if (player->GetQuestStatus(QUEST_THE_CLEANSING_HORDE) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(QUEST_THE_CLEANSING_ALLIANCE) == QUEST_STATUS_INCOMPLETE)
            {
                player->CastSpell(player, SPELL_CLEANSING_SOUL);
                player->SetStandState(UNIT_STAND_STATE_SIT);
            }
            return true;
    }
};

#define NPC_PROUDDUSK 9136
#define QUEST_DREADMAUL_ROCK 3821
#define NPC_PROUDTUSK_GOSSIP_MENU 2850
class go_proudtusk_gravestone: public GameObjectScript
{
    public:

        go_proudtusk_gravestone()
            : GameObjectScript("go_proudtusk_gravestone")
        {
        }
        bool OnGossipHello(Player* player, GameObject* go)
        {
            player->SEND_GOSSIP_MENU(NPC_PROUDTUSK_GOSSIP_MENU,go->GetGUID());
            if(!(go->FindNearestCreature(NPC_PROUDDUSK,5.0f))){
                go->SummonCreature(NPC_PROUDDUSK,go->GetPositionX()+1,go->GetPositionY()+1,go->GetPositionZ(),0.0F,TEMPSUMMON_TIMED_DESPAWN,120000);
            }
            return true;
        }
};

/*######
## go_tcg_picnic_grill
######*/

#define GRILLED_PICINIC_TREAT 32563

class go_tcg_picnic_grill : public GameObjectScript
{
public:
    go_tcg_picnic_grill() : GameObjectScript("go_tcg_picnic_grill") { }


    bool OnGossipHello(Player* player, GameObject* pGO)
    {
        if (player && pGO)
        {
            // Add item manually because otherwise picnic grill despawns after all group members looted the grill
            player->AddItem(GRILLED_PICINIC_TREAT, 1);
            // Send loot release manually because otherwise the empty loot leads to nasty bugs with the loot animation
            player->SendLootRelease(pGO->GetGUID());
        }
        return true;
    }
};

/*######
## go_despawn_on_activate_object
##
## simple script for spells with Effect (86) Activate Object (15)
## after activate object a OnGossipHello will called, following by an instant despawn
######*/
class go_despawn_on_activate_object : public GameObjectScript
{
public:
    go_despawn_on_activate_object() : GameObjectScript("go_despawn_on_activate_object") { }

    bool OnGossipHello(Player* /*player*/, GameObject* go)
    {
        go->DespawnOrUnsummon();
        return false;
    }
};

/*
* go_testing_script
*
* before overwrite plz check if #9312 is pending/live
*/

class go_testing_script : public GameObjectScript
{
public:
    go_testing_script() : GameObjectScript("go_testing_script") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Despawn instant", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Despawn after 15 seconds", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "spawn temp GO with 30 sec respawntimer", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
            player->SEND_GOSSIP_MENU(GOSSIP_OUTHOUSE_VACANT, go->GetGUID());
            return true;
    }

    bool OnGossipSelect(Player* player, GameObject* go, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();
        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF+1:
                go->Delete();
                break;
            case GOSSIP_ACTION_INFO_DEF+2:
                go->Delete();
                break;
            case GOSSIP_ACTION_INFO_DEF+4:
                go->SummonGameObject(go->GetEntry(), go->GetPositionX()+5,go->GetPositionY(),go->GetPositionZ(),0,0,0,0,0,30);
                break;
        }
        return true;
    }
};

/*######
## go_brazier_of_madness
######*/

const uint32 NpcMadnessEntry[] =
{
    15083, 15085, 15084, 15082
};

class go_brazier_of_madness : public GameObjectScript
{
public:
    go_brazier_of_madness() : GameObjectScript("go_brazier_of_madness") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        go->UseDoorOrButton();
        int Random = rand32() % (sizeof(NpcMadnessEntry) / sizeof(uint32));

        player->SummonCreature(NpcMadnessEntry[Random], -11901.229f, -1906.366f, 65.358f, 0.942f, TEMPSUMMON_DEAD_DESPAWN, 120000);

        return false;
    }
};

enum PostboxesMisc
{
    GO_CRUSADER_POSTBOX   =  176349,
    GO_ELDER_POSTBOX      =  176351,
    GO_KING_POSTBOX       =  176352,
    NPC_UNDEAD_POSTMAN    =  11142,
    NPC_MALOWN            =  11143,
    GO_POSTBOX            =  176360,
    SPELL_SUMMON_MALOWN   =  24627
};

bool   crusader_postbox, elder_postbox, king_postbox;

class go_stratholme_postbox : public GameObjectScript
{
public:
    go_stratholme_postbox() : GameObjectScript("go_stratholme_postbox") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
         switch (go->GetEntry())
         {
             case GO_CRUSADER_POSTBOX:
                 go->SummonCreature(NPC_UNDEAD_POSTMAN, 3664.55f, -3176.47f, 126.42f, 2.2f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                 go->SummonCreature(NPC_UNDEAD_POSTMAN, 3656.82f, -3160.63f, 129.03f, 4.84f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                 go->SummonCreature(NPC_UNDEAD_POSTMAN, 3644.62f, -3168.25f, 128.52f, 5.93f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                 go->SummonGameObject(GO_POSTBOX, 3651.69f, -3166.39f, 129.24f, 5.03f, 0.0f, 0.0f, 0.0f, 0.0f, 600000000);
                 crusader_postbox = true;
                 break;
             case GO_ELDER_POSTBOX:
                 go->SummonCreature(NPC_UNDEAD_POSTMAN, 3659.46f, -3634.96f, 138.33f, 1.28f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                 go->SummonCreature(NPC_UNDEAD_POSTMAN, 3656.25f, -3635.08f, 138.36f, 1.02f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                 go->SummonCreature(NPC_UNDEAD_POSTMAN, 3661.24f, -3621.96f, 138.4f, 3.58f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                 go->SummonGameObject(GO_POSTBOX, 3661.24f, -3621.96f, 138.4f, 3.58f, 0.0f, 0.0f, 0.0f, 0.0f, 600000000);
                 elder_postbox = true;
                 break;
             case GO_KING_POSTBOX:
                 go->SummonCreature(NPC_UNDEAD_POSTMAN, 3568.5f, -3356.91f, 131.06f, 2.07f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                 go->SummonCreature(NPC_UNDEAD_POSTMAN, 3570.91f, -3351.01f, 130.57f, 2.71f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                 go->SummonCreature(NPC_UNDEAD_POSTMAN, 3562.79f, -3353.38f, 130.78f, 0.81f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
                 go->SummonGameObject(GO_POSTBOX, 3639.44f, -3640.59f, 139.58f, 2.06f, 0.0f, 0.0f, 0.0f, 0.0f, 600000000);
                 king_postbox = true; 
                 break;
            }  

            if (crusader_postbox == true && elder_postbox == true && king_postbox == true)
            {
                go->CastSpell(player, SPELL_SUMMON_MALOWN);                
            }        

        return true;
    }
};

/*######
## go_defener_portal
######*/

enum PortalspellTW
{
    SPELL_PORT_TW       = 54640
};

class go_defender_portal : public GameObjectScript
{
public:
    go_defender_portal() : GameObjectScript("go_defender_portal") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/)
    {
        if (player->HasAuraType(SPELL_AURA_FLY))
            player->RemoveAurasByType(SPELL_AURA_FLY);
        
        if (player->HasAuraType(SPELL_AURA_MOUNTED))
            player->RemoveAurasByType(SPELL_AURA_MOUNTED);
        
        player->CastSpell(player, SPELL_PORT_TW, true);
        return true;
    }
};

enum ToyTrainSpells
{
    SPELL_TOY_TRAIN_PULSE       = 61551,
};

class go_toy_train_set : public GameObjectScript
{
    public:
        go_toy_train_set() : GameObjectScript("go_toy_train_set") { }

        struct go_toy_train_setAI : public GameObjectAI
        {
            go_toy_train_setAI(GameObject* go) : GameObjectAI(go), _pulseTimer(3 * IN_MILLISECONDS) { }
        
            void UpdateAI(uint32 diff) override
            {
                if (diff < _pulseTimer)
                    _pulseTimer -= diff;
                else
                {
                    go->CastSpell(nullptr, SPELL_TOY_TRAIN_PULSE);
                    _pulseTimer = 6 * IN_MILLISECONDS;
                }
            }

            // triggered on wrecker'd
            void DoAction(int32 /*action*/) override
            {
                go->Delete();
            }

        private:
            uint32 _pulseTimer;
        };

        GameObjectAI* GetAI(GameObject* go) const override
        {
            return new go_toy_train_setAI(go);
        }
};

/*####
## go_bells
####*/

enum BellHourlySoundFX
{
    BELLTOLLHORDE          = 6595, // Horde
    BELLTOLLTRIBAL         = 6675,
    BELLTOLLALLIANCE       = 6594, // Alliance
    BELLTOLLNIGHTELF       = 6674,
    BELLTOLLDWARFGNOME     = 7234,
    BELLTOLLKHARAZHAN      = 9154 // Karazhan
};

enum BellHourlySoundAreas
{
    UNDERCITY_AREA         = 1497,
    IRONFORGE_1_AREA       = 809,
    IRONFORGE_2_AREA       = 1,
    DARNASSUS_AREA         = 1657,
    TELDRASSIL_ZONE        = 141,
    KHARAZHAN_MAPID        = 532
};

enum BellHourlyObjects
{
    GO_HORDE_BELL          = 175885,
    GO_ALLIANCE_BELL       = 176573,
    GO_KHARAZHAN_BELL      = 182064
};

enum BellHourlyMisc
{
    GAME_EVENT_HOURLY_BELLS = 73,
    EVENT_RING_BELL         = 1
};

class go_bells : public GameObjectScript
{
public:
    go_bells() : GameObjectScript("go_bells") { }

    struct go_bellsAI : public GameObjectAI
    {
        go_bellsAI(GameObject* go) : GameObjectAI(go) { }

        void InitializeAI() override
        {
            switch (go->GetEntry())
            {
                case GO_HORDE_BELL:
                    _soundId = go->GetAreaId() == UNDERCITY_AREA ? BELLTOLLHORDE : BELLTOLLTRIBAL;
                    break;
                case GO_ALLIANCE_BELL:
                {
                    if (go->GetAreaId() == IRONFORGE_1_AREA || go->GetAreaId() == IRONFORGE_2_AREA)
                        _soundId = BELLTOLLDWARFGNOME;
                    else if (go->GetAreaId() == DARNASSUS_AREA || go->GetZoneId() == TELDRASSIL_ZONE)
                        _soundId = BELLTOLLNIGHTELF;
                    else
                        _soundId = BELLTOLLALLIANCE;

                    break;
                }
                case GO_KHARAZHAN_BELL:
                    _soundId = BELLTOLLKHARAZHAN;
                    break;
            }
        }

        void OnGameEvent(bool start, uint16 eventId) override
        {
            if (eventId == GAME_EVENT_HOURLY_BELLS && start)
            {
                time_t time = game::time::GetGameTime();
                tm localTm;
                localtime_r(&time, &localTm);
                uint8 _rings = (localTm.tm_hour - 1) % 12 + 1;

                for (auto i = 0; i < _rings; ++i)
                    _events.ScheduleEvent(EVENT_RING_BELL, Seconds(i * 4 + 1));
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_RING_BELL:
                        go->PlayDirectSound(_soundId);
                        break;
                    default:
                        break;
                }
            }
        }
    private:
        EventMap _events;
        uint32 _soundId;
    };

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return new go_bellsAI(go);
    }
};

void AddSC_go_scripts()
{
    new go_cat_figurine();
    new go_barov_journal();
    new go_gilded_brazier();
    new go_orb_of_command();
    new go_shrine_of_the_birds();
    new go_southfury_moonstone();
    new go_tablet_of_madness();
    new go_tablet_of_the_seven();
    new go_jump_a_tron();
    new go_ethereum_prison();
    new go_ethereum_stasis();
    new go_resonite_cask();
    new go_sacred_fire_of_life();
    new go_tele_to_dalaran_crystal();
    new go_tele_to_violet_stand();
    new go_fel_crystalforge();
    new go_matrix_punchograph();
    new go_scourge_cage();
    new go_arcane_prison();
    new go_blood_filled_orb();
    new go_jotunheim_cage();
    new go_table_theka();
    new go_inconspicuous_landmark();
    new go_soulwell();
    new go_amberpine_outhouse();
    new go_hive_pod();
    new go_massive_seaforium_charge();
    new go_gjalerbron_cage();
    new go_large_gjalerbron_cage();
    new go_veil_skith_cage();
    new go_frostblade_shrine();
    new go_toy_train_set();
    new go_proudtusk_gravestone;
    new go_tcg_picnic_grill();
    new go_despawn_on_activate_object();
    new go_portal_shatt_quel();
    new go_portal_shatt_exo();
    new go_portal_shatt_silvermoon();
    new go_brazier_of_madness();
    new go_stratholme_postbox();
    new go_defender_portal();
    new go_bells();
}
