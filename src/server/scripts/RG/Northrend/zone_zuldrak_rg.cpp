/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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
#include "ScriptedEscortAI.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"

/*####
## Commander Kunz
####*/

enum NPCCommanderKunz
{
    QUEST_PA_TROLL                          = 12596,
    QUEST_CREATURE_COMFORTS                 = 12599,
    QUEST_THROWING_DOWN                     = 12598,
    QUEST_SOMETHING_FOR_THE_PAIN            = 12597,
    QUEST_LAB_WORK                          = 12557,
    QUEST_CONGRATULATIONS                   = 12604,
    QUEST_TROLL_PATROL                      = 12587,
    AURA_ON_PATROL                          = 51573,
    SPELL_CAPTAIN_GRONDEL_KILL_CREDIT       = 51233,
    SPELL_CAPTAIN_RUPERT_KILL_CREDIT        = 50634,
    SPELL_ALCHEMIST_FINKLESTEIN_KILL_CREDIT = 51232,
    NPC_CAPTAIN_BRANDON                     = 28042 // there is no spell for this KC
};

class npc_commander_kunz : public CreatureScript
{
public:
    npc_commander_kunz() : CreatureScript("npc_commander_kunz") { }

    bool OnGossipHello(Player* player, Creature* me)
    {
        player->PrepareQuestMenu(me->GetGUID());

        //Copy current quest menu without "Congratulations" - this one will be added later (if player fulfills the requirements)
        QuestMenu &qm = player->PlayerTalkClass->GetQuestMenu();
        QuestMenu qm2;

        for (uint32 i = 0; i<qm.GetMenuItemCount(); ++i)
        {
            if (qm.GetItem(i).QuestId == QUEST_CONGRATULATIONS)
                continue;

            qm2.AddMenuItem(qm.GetItem(i).QuestId, qm.GetItem(i).QuestIcon);
        }

        //The Requirements for "Congratulations" are:
        //Player has aura "On Patrol" AND
        //The daily quest "Trol Patrol" has been done today AND
        //"Congratulations" has not been done today

        if(player->HasAura(AURA_ON_PATROL)){
            if(player->SatisfyQuestDay(sObjectMgr->GetQuestTemplate(QUEST_TROLL_PATROL),false) == false){
                if(player->SatisfyQuestDay(sObjectMgr->GetQuestTemplate(QUEST_CONGRATULATIONS),false)){
                    qm2.AddMenuItem(QUEST_CONGRATULATIONS,4);
                }
            }
        }

        //copy the modified quest menu back
        qm.ClearMenu();
        
        for (uint32 i = 0; i<qm2.GetMenuItemCount(); ++i)
                qm.AddMenuItem(qm2.GetItem(i).QuestId, qm2.GetItem(i).QuestIcon);

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(me), me->GetGUID());
        
        return true;
    }

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_PA_TROLL)
        {
            if (player->IsQuestRewarded(QUEST_CREATURE_COMFORTS))
            {
                creature->CastSpell(player, SPELL_CAPTAIN_GRONDEL_KILL_CREDIT);
            }
            if (player->IsQuestRewarded(QUEST_THROWING_DOWN))
            {
                creature->CastSpell(player, SPELL_CAPTAIN_RUPERT_KILL_CREDIT);
            }
            if (player->IsQuestRewarded(QUEST_SOMETHING_FOR_THE_PAIN))
            {
                player->KilledMonsterCredit(NPC_CAPTAIN_BRANDON);
            }
            if (player->IsQuestRewarded(QUEST_LAB_WORK))
            {
                creature->CastSpell(player, SPELL_ALCHEMIST_FINKLESTEIN_KILL_CREDIT);
            }
        }

        return true;
    }
};

/*######
## Quest 12527: Gorged Lurker
## npc_gorged_lurking_basilisk
######*/
enum LurkingBasilisk
{
  SPELL_QUEST                   = 50918,
  ITEM_QUEST_BASILISK_CRYSTAL   = 38382,

};

class npc_gorged_lurking_basilisk : public CreatureScript
{
public:
    npc_gorged_lurking_basilisk() : CreatureScript("npc_gorged_lurking_basilisk") {}

    struct npc_gorged_lurking_basiliskAI : public ScriptedAI
    {
        npc_gorged_lurking_basiliskAI(Creature* creature) : ScriptedAI(creature) {}
        void SpellHit(Unit* caster, SpellInfo const* spell)
        {
            if (spell->Id == SPELL_QUEST)
            {
                caster->ToPlayer()->AddItem(ITEM_QUEST_BASILISK_CRYSTAL, 1);
                me->DespawnOrUnsummon();
                return;
            }

        }
    };

    CreatureAI *GetAI(Creature* creature) const
    {
        return new npc_gorged_lurking_basiliskAI(creature);
    }

};

/*####
## Quest Feedin' Da Goolz
####*/

enum ghoulfeeding
{
  OBJECT_BOWELS_AND_BRAINS_BOWL = 190656,
  NPC_DECAYING_GHOUL            = 28565,
  NPC_GHOUL_FEEDING_KC_BUNNY    = 28591,
  SPELL_SCOURGE_DISGUISE        = 51966
};

class npc_ghoul_feeding_kc_bunny : public CreatureScript
{
    public:
        npc_ghoul_feeding_kc_bunny() : CreatureScript("npc_ghoul_feeding_kc_bunny") { }

    struct npc_ghoul_feeding_kc_bunnyAI : public ScriptedAI
    {
        npc_ghoul_feeding_kc_bunnyAI(Creature* c) : ScriptedAI(c) { }

        bool spawn;
        uint32 uidespawnTimer;
        ObjectGuid playerGUID;
        ObjectGuid ghoulGUID;

        void Reset()
        {
            spawn = false;
            uidespawnTimer = 0;
            playerGUID.Clear();
            ghoulGUID.Clear();
        }

        void UpdateAI(uint32 diff)
        {
            if (!spawn)
            {
                if (Creature* ghoul = me->FindNearestCreature(NPC_DECAYING_GHOUL, 10.0f, true))
                {
                    ghoulGUID = ghoul->GetGUID();
                    if (!ghoul->IsInCombat())
                    {
                        ghoul->GetMotionMaster()->MoveChase(me, 1.0f, 0);
                        ghoul->HandleEmoteCommand(EMOTE_ONESHOT_EAT);
                        spawn = true;
                        uidespawnTimer = 3000;
                        playerGUID = me->ToTempSummon()->GetSummonerGUID();
                    }
                }
            }
            else
            {
                if (uidespawnTimer <= diff)
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
                        if (player->HasAura(SPELL_SCOURGE_DISGUISE))
                            player->KilledMonsterCredit(NPC_GHOUL_FEEDING_KC_BUNNY);

                    if (GameObject* bowl = me->FindNearestGameObject(OBJECT_BOWELS_AND_BRAINS_BOWL, 10.0f))
                        bowl->RemoveFromWorld();

                    me->DespawnOrUnsummon();
                    if (Creature* ghoul = ObjectAccessor::GetCreature(*me, ghoulGUID))
                        ghoul->DespawnOrUnsummon();

                }else uidespawnTimer -= diff;

            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ghoul_feeding_kc_bunnyAI(creature);
    }
};

/*######
## Quest 12606: Cocooned!
## npc_nerubian_cocoon
######*/

enum Cocooned
{
    QUEST_COCOONED          = 12606,

    NPC_DRAKKARI_CAPTIVE    = 28414,
    NPC_CAPTIVE_FOOTMAN     = 28415
};

struct npc_nerubian_cocoonAI : public ScriptedAI
{
    npc_nerubian_cocoonAI(Creature *Creature) : ScriptedAI(Creature) {}

    void Reset()
    {

        me->SetCorpseDelay(0);
    }

    void JustDied(Unit* killer)
    {
        // Pet 
        if (killer->GetTypeId() == TYPEID_UNIT)
        {
            killer = killer->GetCharmerOrOwnerPlayerOrPlayerItself();
        }

        if (killer)
        {
            if (killer->ToPlayer()->GetQuestStatus(QUEST_COCOONED) == QUEST_STATUS_INCOMPLETE && urand(0, 100) <= 40)
            {

                    me->SummonCreature(NPC_CAPTIVE_FOOTMAN, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 60000);
                    if (killer->ToPlayer())
                        killer->ToPlayer()->KilledMonsterCredit(NPC_CAPTIVE_FOOTMAN);
            }
            
            else if (Creature* creature = me->SummonCreature(NPC_DRAKKARI_CAPTIVE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000))
            {
                creature->AI()->AttackStart(killer);
            }
        }
    }

    void UpdateAI(uint32 diff) override { } // do nothing
};

/*####
 ## npc_stefan_vadu
 ####*/

enum StefanVaduMisc
{
    SPELL_SUMMON_NASS                   = 51889,
    QUEST_KICKIN_NASS_AND_TAKIN_MANES   = 12630,

    SPELL_PUSH_ENSORCELLED_CHOKER       = 53810,
    ITEM_ENSORCELLED_CHOKER             = 38699,
    QUEST_DRESSING_DOWN                 = 12648,
    QUEST_SUIT_UP                       = 12649,
    QUEST_BETRAYAL                      = 12713,
    QUEST_SABOTAGE                      = 12676,
    SPELL_KILL_CREDIT_A                 = 52680,
    SPELL_KILL_CREDIT_B                 = 52675
};

enum StefanVaduTexts
{
    VADU                                = 0
};

class npc_stefan_vadu : public CreatureScript
{
    public:
        npc_stefan_vadu() : CreatureScript("npc_stefan_vadu") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == QUEST_KICKIN_NASS_AND_TAKIN_MANES)
            {
                creature->CastSpell(player, SPELL_SUMMON_NASS);
                creature->AI()->Talk(VADU, player);
                player->RemoveAurasByType(SPELL_AURA_MOUNTED);
            }
        return true;
        }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            if (creature->IsQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());

            if (player->GetQuestStatus(QUEST_KICKIN_NASS_AND_TAKIN_MANES) == QUEST_STATUS_INCOMPLETE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(9709, 0), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 0);

            if (!player->HasItemCount(ITEM_ENSORCELLED_CHOKER, 1, true)
                && (player->GetQuestStatus(QUEST_DRESSING_DOWN) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(QUEST_DRESSING_DOWN) == QUEST_STATUS_REWARDED
                || player->GetQuestStatus(QUEST_SUIT_UP) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(QUEST_SUIT_UP) == QUEST_STATUS_REWARDED)
                && !player->GetQuestRewardStatus(QUEST_BETRAYAL))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(9709, 1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            player->RemoveAurasByType(SPELL_AURA_MOUNTED);
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*uiSender*/, uint32 uiAction)
        {
            switch (uiAction)
            {
                case GOSSIP_ACTION_INFO_DEF + 0:
                    player->CLOSE_GOSSIP_MENU();
                    player->CastSpell(player, SPELL_SUMMON_NASS, false);
                    player->RemoveAurasByType(SPELL_AURA_MOUNTED);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 1:
                    player->CLOSE_GOSSIP_MENU();
                    player->CastSpell(player, SPELL_PUSH_ENSORCELLED_CHOKER, false);
                    break;
            }
            return true;
        }
};

/*####
 ## Quest 12707: Wooly Justice
 ## npc_enraged_mammoth
 ## npc_mamtoth_disciple
 ####*/

enum WoolyJustice
{
    SPELL_WOOLY_JUSTICE_KILL_CREDIT_AURA = 52607,
    NPC_WOOLY_JUSTICE_KILL_CREDIT        = 28876
};

class npc_mamtoth_disciple : public CreatureScript
{
    public:
        npc_mamtoth_disciple() : CreatureScript("npc_mamtoth_disciple") { }

    struct npc_mamtoth_discipleAI : public ScriptedAI
    {
        npc_mamtoth_discipleAI(Creature* creature) : ScriptedAI(creature) { }

        void JustDied(Unit* killer)
        {
            if (me->HasAura(SPELL_WOOLY_JUSTICE_KILL_CREDIT_AURA))
                if (Unit* charmerowner = killer->GetCharmerOrOwner())
                    if (Player* player = charmerowner->ToPlayer())
                        player->KilledMonsterCredit(NPC_WOOLY_JUSTICE_KILL_CREDIT);
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_mamtoth_discipleAI(creature);
    }
};

/*####
 ## Quest 12710: Disclosure
 ## spell_q12710_summon_escort_aura
 ## npc_malmortis
 ####*/

enum QuestDisclosure
{
    NPC_MALMORTIS               = 28948,
    NPC_DISCLOSURE_KC_BUNNY     = 28929,
    QUEST_CLEANSING_DRAK_THARON = 12238,
    EVENT_SAY_INTRO_1           = 1,
    EVENT_SAY_INTRO_2           = 2,
    EVENT_START_WALKING         = 3,
    EVENT_SAY_4                 = 4,
    EVENT_SAY_5                 = 5,
    EVENT_SAY_7                 = 6,
    EVENT_SAY_8                 = 7,
    EVENT_SAY_9                 = 8,
    EVENT_SAY_11                = 9,
    EVENT_SAY_12                = 10,
    EVENT_SAY_13                = 11,
    EVENT_SAY_14                = 12,
    EVENT_SAY_15                = 13,
    EVENT_SAY_16                = 14,
    MALMORTIS_SAY_1             = 0,
    MALMORTIS_SAY_2             = 1,
    MALMORTIS_SAY_3             = 2,
    MALMORTIS_SAY_4             = 3,
    MALMORTIS_SAY_5             = 4,
    MALMORTIS_SAY_6             = 5,
    MALMORTIS_SAY_7             = 6,
    MALMORTIS_SAY_8             = 7,
    MALMORTIS_SAY_9             = 8,
    MALMORTIS_SAY_10             = 9,
    MALMORTIS_SAY_11             = 10,
    MALMORTIS_SAY_12             = 11,
    MALMORTIS_SAY_13             = 12,
    MALMORTIS_SAY_14             = 13,
    MALMORTIS_SAY_15             = 14,
    MALMORTIS_SAY_16             = 15,
};

class spell_q12710_summon_escort_aura : public SpellScriptLoader
{
public:
    spell_q12710_summon_escort_aura() : SpellScriptLoader("spell_q12710_summon_escort_aura") { }

    class spell_q12710_summon_escort_aura_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_q12710_summon_escort_aura_SpellScript);

        void HandleSpell(SpellEffIndex /*effIndex*/)
        {   
            if (Unit* caster = GetCaster())
            {
                if (!caster->FindNearestCreature(NPC_MALMORTIS, 20.0f))
                {
                    caster->SummonCreature(NPC_MALMORTIS, 6255.0f, -1959.27f, 484.783f, 4.359f,TEMPSUMMON_MANUAL_DESPAWN, 600000);
                }
            }
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_q12710_summon_escort_aura_SpellScript::HandleSpell, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_q12710_summon_escort_aura_SpellScript();
    }
};

class npc_malmortis : public CreatureScript
{
    public:
        npc_malmortis() : CreatureScript("npc_malmortis") { }

    struct npc_malmortisAI : public npc_escortAI
    {
        npc_malmortisAI(Creature* creature) : npc_escortAI(creature) { }

        bool intro;
        uint32 introtimer;
        ObjectGuid playerGUID;
        EventMap events;

        void Reset()
        {
            intro = true;
            introtimer = 5000;
            events.Reset();
            if (TempSummon* temp = me->ToTempSummon())
            {
                if (Unit* summoner = temp->GetSummoner())
                {
                    if (Player* player = summoner->ToPlayer())
                    {
                        playerGUID = player->GetGUID();
                        events.ScheduleEvent(EVENT_SAY_INTRO_1, 3000);
                    }
                }
            }
        }

        void WaypointReached(uint32 waypointId)
        {
            Player* player = GetPlayerForEscort();
            if (!player)
                return;

            switch (waypointId)
            {
                case 1:
                    if (player->GetQuestRewardStatus(QUEST_CLEANSING_DRAK_THARON))
                    {
                        Talk(MALMORTIS_SAY_3);
                    }
                    events.ScheduleEvent(EVENT_SAY_4, 5000);
                    break;
                case 10: // in front of crystals
                    Talk(MALMORTIS_SAY_6);
                    events.ScheduleEvent(EVENT_SAY_7, 5000);
                    break;
                case 12: // in front of troll
                    Talk(MALMORTIS_SAY_10);
                    events.ScheduleEvent(EVENT_SAY_11, 5000);
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);

            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case EVENT_SAY_INTRO_1:
                    Talk(MALMORTIS_SAY_1);
                    events.ScheduleEvent(EVENT_SAY_INTRO_2, 5000);
                    break;
                case EVENT_SAY_INTRO_2:
                    if (Player* player = GetPlayerForEscort())
                        Talk(MALMORTIS_SAY_2, player);
                    events.ScheduleEvent(EVENT_START_WALKING, 5000);
                    break;
                case EVENT_START_WALKING:
                    npc_escortAI::Start(false, false, playerGUID);
                    break;
                case EVENT_SAY_4:
                    Talk(MALMORTIS_SAY_4);
                    events.ScheduleEvent(EVENT_SAY_5, 5000);
                    break;
                case EVENT_SAY_5:
                    Talk(MALMORTIS_SAY_5);
                    break;
                case EVENT_SAY_7:
                    Talk(MALMORTIS_SAY_7);
                    events.ScheduleEvent(EVENT_SAY_8, 5000);
                    break;
                case EVENT_SAY_8:
                    Talk(MALMORTIS_SAY_8);
                    events.ScheduleEvent(EVENT_SAY_9, 5000);
                    break;
                case EVENT_SAY_9:
                    Talk(MALMORTIS_SAY_9);
                    break;
                case EVENT_SAY_11:
                    Talk(MALMORTIS_SAY_11);
                    events.ScheduleEvent(EVENT_SAY_12, 5000);
                    break;
                case EVENT_SAY_12:
                    Talk(MALMORTIS_SAY_12);
                    events.ScheduleEvent(EVENT_SAY_13, 5000);
                    break;
                case EVENT_SAY_13:
                    Talk(MALMORTIS_SAY_13);
                    events.ScheduleEvent(EVENT_SAY_14, 5000);
                    break;
                case EVENT_SAY_14:
                    if (Player* player = GetPlayerForEscort())
                        Talk(MALMORTIS_SAY_14, player);
                    events.ScheduleEvent(EVENT_SAY_15, 5000);
                    break;
                case EVENT_SAY_15:
                    Talk(MALMORTIS_SAY_15);
                    events.ScheduleEvent(EVENT_SAY_16, 5000);
                    break;
                case EVENT_SAY_16:
                    Talk(MALMORTIS_SAY_16);
                    if (Player* player = GetPlayerForEscort())
                    {
                        player->KilledMonsterCredit(NPC_DISCLOSURE_KC_BUNNY);
                        me->DespawnOrUnsummon(5000);
                    }
                    break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_malmortisAI(creature);
    }
};

/*####
 ## Quest 12713: Betrayal
 ## npc_overlord_drakuru_enemy
 ## npc_lich_king_betrayal_quest
 ####*/

 enum QuestBetrayal
 {
     NPC_ENEMY_DRAKURU             = 28998,
     NPC_BLIGHTBLOOD_TROLL         = 28931,
     NPC_LICH_KING                 = 1010656,
     NPC_TOTALLY_GENERIC_BUNNY     = 28960,
     NPC_DRAKURUS_SKULL_TRIGGER    = 1010657,
     GO_DRAKURUS_LAST_WISH         = 202357,
     GO_DRAKURUS_SKULL             = 191458,
     SPELL_DRAKURU_SHADOW_BOLT     = 54113,
     SPELL_THROW_BLIGHT_CRYSTAL    = 54087,
     SPELL_DRAKURUS_DEATH          = 54248,
     // SPELL_SPAWN_DRAKURUS_SKULL = 54253,
     // SPELL_SCOURGE_DISGUISE     = 51966
     EVENT_TALK_1                  = 1,
     EVENT_TALK_2                  = 2,
     EVENT_SPAWN_TROLLS            = 3,
     EVENT_FAIL_DISGUISE_WARNING   = 4,
     EVENT_FAIL_DISGUISE           = 5,
     EVENT_TALK_3                  = 6,
     EVENT_TALK_4                  = 7,
     // EVENT_TALK_5               = 8,
     EVENT_TALK_6                  = 9,
     EVENT_TALK_7                  = 10,
     EVENT_CAST_SPELL1             = 11,
     EVENT_LICHKING_WALK           = 12,
     EVENT_LICHKING_TALK_1         = 13,
     EVENT_LICHKING_TALK_2         = 14,
     EVENT_LICHKING_KILL_DRAKURU   = 15,
     EVENT_LICHKING_TALK_3         = 16,
     EVENT_LICHKING_TALK_4         = 17,
     EVENT_LICHKING_TALK_5         = 18,
     EVENT_LICHKING_TALK_6         = 19,
     EVENT_CAST_SPELL2             = 20,
     EVENT_FAIL_DISGUISE_ALWAYS    = 21,
     SAY_FAIL_DISGUISE_WARNING     = 0,
     DRAKURU_SAY_1                 = 1,
     DRAKURU_SAY_2                 = 2,
     DRAKURU_SAY_3                 = 3,
     DRAKURU_SAY_4                 = 4,
     DRAKURU_SAY_5                 = 5,
     DRAKURU_SAY_6                 = 6,
     DRAKURU_SAY_7                 = 7,
     LICHKING_SAY_1                = 0,
     LICHKING_SAY_2                = 1,
     LICHKING_SAY_3                = 2,
     LICHKING_SAY_4                = 3,
     LICHKING_SAY_5                = 4,
     LICHKING_SAY_6                = 5
 };

Position BlightbloodTrollSpawnPos[] =
{
    {6222.93f, -2026.46f, 586.76f, 2.938f},
    {6166.21f, -2065.61f, 586.76f, 1.367f},
    {6127.46f, -2008.76f, 586.76f, 6.126f},
    {6184.25f, -1969.81f, 586.76f, 4.579f},
};

class npc_overlord_drakuru_enemy : public CreatureScript
{
    public:
        npc_overlord_drakuru_enemy() : CreatureScript("npc_overlord_drakuru_enemy") { }

    struct npc_overlord_drakuru_enemyAI : public ScriptedAI
    {
        npc_overlord_drakuru_enemyAI(Creature* creature) : ScriptedAI(creature) { }
        
        EventMap events;

        void InitializeAI()
        {
            // there should only be one enemy Drakuru at a time
            std::list<Creature*> creaturelist;
            me->GetCreatureListWithEntryInGrid(creaturelist, NPC_ENEMY_DRAKURU, 1000.0f);
            if (creaturelist.size() > 1)
            {
                me->DespawnOrUnsummon(0);
                return;
            }
            
            // despawn eventually existing skull and last wish when starting a new event
            if (Creature* trigger = me->FindNearestCreature(NPC_DRAKURUS_SKULL_TRIGGER, 1000.0f))
            {
                trigger->DespawnOrUnsummon();
            }
            if (GameObject* skull = me->FindNearestGameObject(GO_DRAKURUS_SKULL, 1000.0f))
            {
                skull->Delete();
            }
            if (GameObject* wish = me->FindNearestGameObject(GO_DRAKURUS_LAST_WISH, 1000.0f))
            {
                wish->Delete();
            }

            events.ScheduleEvent(EVENT_TALK_1, 10000);
        }

        void KilledUnit(Unit* victim)
            {
              if (victim->GetTypeId() == TYPEID_PLAYER)
              me->DespawnOrUnsummon(0); // only darkuru despawn cuz trolls despawn if a new event starts
            }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);
            
            switch (events.ExecuteEvent())
            {
                case EVENT_TALK_1:
                    Talk(DRAKURU_SAY_1);
                    events.ScheduleEvent(EVENT_TALK_2, 10000);
                    events.ScheduleEvent(EVENT_SPAWN_TROLLS, 3000);
                    break;
                case EVENT_TALK_2:
                    Talk(DRAKURU_SAY_2);
                    events.ScheduleEvent(EVENT_FAIL_DISGUISE_WARNING, 7000);
                    events.ScheduleEvent(EVENT_FAIL_DISGUISE_ALWAYS, 10000);
                    break;
                case EVENT_TALK_3:
                    Talk(DRAKURU_SAY_3);
                    break;
                case EVENT_SPAWN_TROLLS:
                    SpawnTrolls();
                    break;
                case EVENT_FAIL_DISGUISE_WARNING:
                    HandleDisguise(true);
                    events.ScheduleEvent(EVENT_TALK_3, 1500);
                    events.ScheduleEvent(EVENT_FAIL_DISGUISE, 3000);
                    break;
                case EVENT_FAIL_DISGUISE:
                    HandleDisguise(false);
                    if (Player* player = me->FindNearestPlayer(90.0f))
                    {
                        me->SetInCombatWith(player);
                        AddThreat(player, 1.0f);
                    }
                    events.ScheduleEvent(EVENT_CAST_SPELL1, urand(3000, 5000));
                    events.ScheduleEvent(EVENT_CAST_SPELL2, urand(5000, 10000));
                    events.ScheduleEvent(EVENT_TALK_4, 15000);
                    break;
                case EVENT_FAIL_DISGUISE_ALWAYS:
                    HandleDisguise2(true);
                    events.ScheduleEvent(EVENT_FAIL_DISGUISE_ALWAYS, 10000);
                case EVENT_TALK_4:
                    Talk(DRAKURU_SAY_4);
                    events.RescheduleEvent(EVENT_TALK_4, 15000);
                    break;
                case EVENT_TALK_6:
                    Talk(DRAKURU_SAY_6);
                    me->SummonCreature(NPC_LICH_KING, 6169.84f, -2035.72f, 590.88f, 1.25f);
                    events.ScheduleEvent(EVENT_TALK_7, 5000);
                    break;
                case EVENT_TALK_7:
                    Talk(DRAKURU_SAY_7);
                    break;
                case EVENT_CAST_SPELL1:
                    if (Unit* victim = me->GetVictim())
                    {
                        if (Player* player = victim->ToPlayer()) // if victim is player: shadow bolt
                        {
                            DoCast(player, SPELL_DRAKURU_SHADOW_BOLT);
                        }
                    }
                    events.RescheduleEvent(EVENT_CAST_SPELL1, urand(4, 5)*IN_MILLISECONDS);
                    break;
                case EVENT_CAST_SPELL2:
                    if (Unit* victim = me->GetVictim())
                    {
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 90, true)) // if victim = troll -> cast crystalspell on rndmplayer in range
                        {
                            DoCast(target, SPELL_THROW_BLIGHT_CRYSTAL);
                        }
                    }
                    events.RescheduleEvent(EVENT_CAST_SPELL1, urand(3000, 5000));
                    events.RescheduleEvent(EVENT_CAST_SPELL2, urand(5, 15)*IN_MILLISECONDS);
                    break;
            }

            if (UpdateVictim())
            {
                DoMeleeAttackIfReady();
            }
        }

        void DamageTaken(Unit* /*who*/, uint32 &damage)
        {
            if (damage >= me->GetHealth())
            {
                damage = 0;
                if (!me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                {
                    Talk(DRAKURU_SAY_5);
                    events.ScheduleEvent(EVENT_TALK_6, 5000);
                }
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                events.CancelEvent(EVENT_CAST_SPELL1);
                events.CancelEvent(EVENT_CAST_SPELL2);
                events.CancelEvent(EVENT_TALK_4);
                me->setFaction(35);
                DespawnTrolls();
            }
        }

        void HandleDisguise(bool warning)
        {
            std::list<Player*> players;
            me->GetPlayerListInGrid(players, 100.f);
            for (std::list<Player*>::const_iterator pitr = players.begin(); pitr != players.end(); ++pitr)
            {
                Player* player = *pitr;
                if (player->GetPositionZ() > 580.0f) // only players on the roof
                {
                    if (player->HasAura(SPELL_SCOURGE_DISGUISE))
                    {
                        if (warning)
                        {
                            Talk(SAY_FAIL_DISGUISE_WARNING, player);
                        }
                        else
                        {
                            player->RemoveAura(SPELL_SCOURGE_DISGUISE);
                        }
                    }
                }
            }
        }

        void HandleDisguise2(bool warning)
        {
            std::list<Player*> players;
            me->GetPlayerListInGrid(players, 100.f);
            for (std::list<Player*>::const_iterator pitr = players.begin(); pitr != players.end(); ++pitr)
            {
                Player* player = *pitr;
                if (player->GetPositionZ() > 580.0f) // only players on the roof
                {
                    if (player->HasAura(SPELL_SCOURGE_DISGUISE))
                    {
                        player->RemoveAura(SPELL_SCOURGE_DISGUISE);
                    }
                }
            }
        }
        

        void DespawnTrolls()
        {
            std::list<Creature*> creaturelist;
            me->GetCreatureListWithEntryInGrid(creaturelist, NPC_BLIGHTBLOOD_TROLL, 1000.0f);
            for (std::list<Creature*>::const_iterator itr = creaturelist.begin(); itr != creaturelist.end(); ++itr)
            {
                Creature* troll = *itr;
                if (troll->GetPositionZ() > 580.0f)
                {
                    troll->DespawnOrUnsummon();
                }
            }
        }

        void SpawnTrolls()
        {
            // first despawn old trolls on the roof
            DespawnTrolls();

            // now spawn new trolls
            for (uint8 i = 0; i < 4; ++i)
            {
                me->SummonCreature(NPC_BLIGHTBLOOD_TROLL, BlightbloodTrollSpawnPos[i]);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_overlord_drakuru_enemyAI(creature);
    }
};

class npc_lich_king_betrayal_quest : public CreatureScript
{
    public:
        npc_lich_king_betrayal_quest() : CreatureScript("npc_lich_king_betrayal_quest") { }

    struct npc_lich_king_betrayal_questAI : public ScriptedAI
    {
        npc_lich_king_betrayal_questAI(Creature* creature) : ScriptedAI(creature) { }

        EventMap events;
        
        void Reset()
        {
            events.ScheduleEvent(EVENT_LICHKING_WALK, 3000);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);
            
            switch (events.ExecuteEvent())
            {
                case EVENT_LICHKING_WALK:
                    if (Creature* drakuru = me->FindNearestCreature(NPC_ENEMY_DRAKURU, 1000.0f))
                    {
                        me->GetMotionMaster()->MoveFollow(drakuru, 5.0f, me->GetAngle(drakuru->GetPositionX(), drakuru->GetPositionY()));
                    }
                    events.ScheduleEvent(EVENT_LICHKING_TALK_1, 10000);
                    break;
                case EVENT_LICHKING_TALK_1:
                    Talk(LICHKING_SAY_1);
                    events.ScheduleEvent(EVENT_LICHKING_TALK_2, 5000);
                    break;
                case EVENT_LICHKING_TALK_2:
                    Talk(LICHKING_SAY_2);
                    events.ScheduleEvent(EVENT_LICHKING_KILL_DRAKURU, 8000);
                    break;
                case EVENT_LICHKING_KILL_DRAKURU:
                    if (Creature* drakuru = me->FindNearestCreature(NPC_ENEMY_DRAKURU, 1000.0f))
                    {
                        drakuru->CastSpell(drakuru, SPELL_DRAKURUS_DEATH);
                        //drakuru->CastSpell(drakuru, SPELL_SPAWN_DRAKURUS_SKULL);
                        if (Creature* bunny = me->FindNearestCreature(NPC_TOTALLY_GENERIC_BUNNY, 200.0f))
                        {
                            // the GO must be spawned by a permanent trigger NPC, because it will despawn when the summoner despawns
                            bunny->SummonGameObject(GO_DRAKURUS_LAST_WISH, drakuru->GetPositionX(), drakuru->GetPositionY(), drakuru->GetPositionZ(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 180000);
                            bunny->SummonCreature(NPC_DRAKURUS_SKULL_TRIGGER, drakuru->GetPositionX(), drakuru->GetPositionY(), drakuru->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 190000);
                        }
                        me->Kill(drakuru);
                    }
                    events.ScheduleEvent(EVENT_LICHKING_TALK_3, 4000);
                    break;
                case EVENT_LICHKING_TALK_3:
                    Talk(LICHKING_SAY_3);
                    events.ScheduleEvent(EVENT_LICHKING_TALK_4, 5000);
                    break;
                case EVENT_LICHKING_TALK_4:
                    Talk(LICHKING_SAY_4);
                    events.ScheduleEvent(EVENT_LICHKING_TALK_5, 12000);
                    break;
                case EVENT_LICHKING_TALK_5:
                    Talk(LICHKING_SAY_5);
                    events.ScheduleEvent(EVENT_LICHKING_TALK_6, 5000);
                    break;
                case EVENT_LICHKING_TALK_6:
                    Talk(LICHKING_SAY_6);
                    me->DespawnOrUnsummon(10000);
                    break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_lich_king_betrayal_questAI(creature);
    }
};

class npc_drakurus_skull_trigger : public CreatureScript
{
    public:
        npc_drakurus_skull_trigger() : CreatureScript("npc_drakurus_skull_trigger") { }

    struct npc_drakurus_skull_triggerAI : public ScriptedAI
    {
        npc_drakurus_skull_triggerAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 time;
        bool done;
        
        void Reset()
        {
            time = 0;
            done = false;
        }

        void UpdateAI(uint32 diff)
        {
            time += diff;
            if (time < 180000)
            {
                if (!done)
                {
                    if (!me->FindNearestGameObject(GO_DRAKURUS_SKULL, 100.0f))
                    {
                        me->SummonGameObject(GO_DRAKURUS_SKULL, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 180000);
                        done = false; // loot available for everyone for 3 min 
                    }
                }
            }
            else
            {
                if (GameObject* skull = me->FindNearestGameObject(GO_DRAKURUS_SKULL, 100.0f))
                {
                    skull->Delete();
                    done = false;
                }
                me->DespawnOrUnsummon();
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_drakurus_skull_triggerAI(creature);
    }
};

class spell_q12512_crusader_kill_credit : public SpellScriptLoader
{
    public:
        spell_q12512_crusader_kill_credit() : SpellScriptLoader("spell_q12512_crusader_kill_credit") { }

        class spell_q12512_crusader_kill_credit_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q12512_crusader_kill_credit_SpellScript);

            void HandleScript(SpellEffIndex effIndex)
            {
                if (Unit* caster = GetCaster())
                    if (Unit* owner = caster->ToTempSummon()->GetSummoner())
                        owner->CastSpell(owner, GetSpellInfo()->Effects[0].BasePoints + 1, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_q12512_crusader_kill_credit_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_q12512_crusader_kill_credit_SpellScript();
        }
};

/*####
## Quest 12690: Fuel for the Fire
## spell_q12690_burst_at_the_seams
## npc_drakkari_chieftain
####*/

enum QuestFuelForTheFire
{
    NPC_DRAKKARI_SKULLCRUSHER = 28844,
    NPC_DRAKKARI_SKULLCRUSHER_KC_BUNNY = 29099,
    NPC_SHALEWING = 28875,
    SPELL_SUMMON_DRAKKARI_CHIEFTAIN = 52616,
    SPELL_FUEL_FOR_THE_FIRE_TROLL_EXPLOSION = 52575
};

class spell_q12690_burst_at_the_seams : public SpellScriptLoader
{
public:
    spell_q12690_burst_at_the_seams() : SpellScriptLoader("spell_q12690_burst_at_the_seams") { }

    class spell_q12690_burst_at_the_seams_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_q12690_burst_at_the_seams_SpellScript);

        void HandleSpell(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
            {
                if (Unit* charmer = caster->GetCharmerOrOwner())
                {
                    if (Player* player = charmer->ToPlayer())
                    {
                        std::list<Creature*> targetlist;
                        uint32 trolls;
                        trolls = 0;
                        caster->GetCreatureListWithEntryInGrid(targetlist, NPC_DRAKKARI_SKULLCRUSHER, 15.0f);
                        for (std::list<Creature*>::const_iterator itr = targetlist.begin(); itr != targetlist.end(); ++itr)
                        {
                            Creature* target = *itr;
                            if (target->IsAlive())
                            {
                                trolls++;
                                player->KilledMonsterCredit(NPC_DRAKKARI_SKULLCRUSHER_KC_BUNNY);
                                target->SetCorpseDelay(10);
                                target->SetRespawnDelay(50);
                                caster->Kill(target);
                            }
                        }

                        // 5% chance per hit troll to summon chieftain
                        if (urand(1, 20) <= trolls)
                        {
                            player->CastSpell(player, SPELL_SUMMON_DRAKKARI_CHIEFTAIN);
                        }
                    }
                }
                if (Creature* creature = caster->ToCreature())
                {
                    creature->SetCorpseDelay(0);
                    creature->SetRespawnDelay(60);
                }
                caster->CastSpell(caster, SPELL_FUEL_FOR_THE_FIRE_TROLL_EXPLOSION);
                caster->KillSelf();
            }
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_q12690_burst_at_the_seams_SpellScript::HandleSpell, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_q12690_burst_at_the_seams_SpellScript();
    }
};

class npc_drakkari_chieftain : public CreatureScript
{
public:
    npc_drakkari_chieftain() : CreatureScript("npc_drakkari_chieftain") { }

    struct npc_drakkari_chieftainAI : public ScriptedAI
    {
        npc_drakkari_chieftainAI(Creature* creature) : ScriptedAI(creature) { }

        ObjectGuid shalewingGUID;
        bool flyingAway;

        void Reset()
        {
            shalewingGUID.Clear();
            flyingAway = false;
        }

        void UpdateAI(uint32 diff)
        {
            if (!shalewingGUID)
            {
                Position randomPosOnRadius;
                randomPosOnRadius.m_positionZ = (me->GetPositionZ() + 30.0f);
                me->GetNearPoint2D(randomPosOnRadius.m_positionX, randomPosOnRadius.m_positionY, 30.0f, me->GetAngle(me));
                if (Creature* shalewing = me->SummonCreature(NPC_SHALEWING, randomPosOnRadius))
                {
                    shalewingGUID = shalewing->GetGUID();
                    shalewing->GetMotionMaster()->MovePoint(1, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
                }
            }
            else
            {
                if (Creature* shalewing = ObjectAccessor::GetCreature(*me, shalewingGUID))
                {
                    if ((shalewing->GetDistance(me) < 3.0f) && !flyingAway)
                    {
                        flyingAway = true;
                        me->EnterVehicle(shalewing, 0);
                        Position randomPosOnRadius;
                        randomPosOnRadius.m_positionZ = (me->GetPositionZ() + 30.0f);
                        me->GetNearPoint2D(randomPosOnRadius.m_positionX, randomPosOnRadius.m_positionY, 30.0f, me->GetAngle(me) + M_PI);
                        shalewing->GetMotionMaster()->MovePoint(1, randomPosOnRadius);
                        shalewing->DespawnOrUnsummon(5000);
                        me->DespawnOrUnsummon(5000);
                    }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_drakkari_chieftainAI(creature);
    }
};

// 50927 - Create Zul'Drak Rat
class spell_create_zuldrak_rat : public SpellScriptLoader
{
public:
    spell_create_zuldrak_rat() : SpellScriptLoader("spell_create_zuldrak_rat") { }

    class spell_create_zuldrak_rat_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_create_zuldrak_rat_SpellScript);

        SpellCastResult CheckRequirement()
        {
            if (Unit* target = GetExplTargetUnit())
                if (target->IsMounted())
                    return SPELL_FAILED_NOT_ON_MOUNTED;
            return SPELL_CAST_OK;
        }

        void Register()
        {
            OnCheckCast += SpellCheckCastFn(spell_create_zuldrak_rat_SpellScript::CheckRequirement);
        }

    };

    SpellScript* GetSpellScript() const
    {
        return new spell_create_zuldrak_rat_SpellScript();
    }
};

class npc_blight_geist : public CreatureScript
{
    public:
        npc_blight_geist() : CreatureScript("npc_blight_geist") { }

        struct npc_blight_geistAI : public ScriptedAI
        {
            npc_blight_geistAI(Creature* creature) : ScriptedAI(creature) { }

            void OnCharmed(bool apply) override
            {
                if (apply)
                    if (Player* player = me->FindNearestPlayer(50.0f))
                        me->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, player->GetFollowAngle());
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new  npc_blight_geistAI(creature);
        }
};

enum Tangled_Skein 
{
	SPELL_TANGLED_SKEIN_THROWER			= 51165,
	SPELL_TANGLED_SKEIN_ENCASING_WEBS	= 51173,
	SPELL_SUMMON_PLAGUE_SPRAY			= 54496,
	SPELL_PLAGUE_SPRAYER_EXPLOSION		= 53236,
	SPELL_ENCASING_WEBS					= 51168,

	NPC_PLAGUE_SPRAYER_KILL_CREDIT		= 28289,

	EVENT_FALL_TO_GROUND				= 1,
	EVENT_DESPAWN						= 2
};

struct npc_plague_sprayerAI : public ScriptedAI 
{
	npc_plague_sprayerAI(Creature* creature) : ScriptedAI(creature) {}

	void Reset()
	{
		me->SetReactState(REACT_PASSIVE);
		me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
		me->SetDisableGravity(true);
		me->SetCanFly(true);
	}

	void SpellHit(Unit* caster, SpellInfo const* spell) 
	{
		if (spell->Id == SPELL_TANGLED_SKEIN_THROWER)
		{
			if (caster->ToPlayer())
				playerGUID = caster->ToPlayer()->GetGUID();
		}

		if (spell->Id == SPELL_TANGLED_SKEIN_ENCASING_WEBS) 
		{
			DoCastSelf(SPELL_SUMMON_PLAGUE_SPRAY);
			DoCastSelf(SPELL_PLAGUE_SPRAYER_EXPLOSION);
			DoCastSelf(SPELL_ENCASING_WEBS);
			events.ScheduleEvent(EVENT_FALL_TO_GROUND, 0s);
		}
	}

	void UpdateAI(uint32 diff) 
	{
		events.Update(diff);
		switch (events.ExecuteEvent()) 
		{
		case EVENT_FALL_TO_GROUND: 
		{
			me->SetDisableGravity(true);
			me->SetCanFly(false);
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
			me->GetMotionMaster()->MoveFall();
			events.ScheduleEvent(EVENT_DESPAWN, 2s);
		}
		break;
		case EVENT_DESPAWN: 
		{
			if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
			{
				player->KilledMonsterCredit(NPC_PLAGUE_SPRAYER_KILL_CREDIT);
			}
			DoCastSelf(SPELL_PLAGUE_SPRAYER_EXPLOSION);
			me->DespawnOrUnsummon(2000);
		}
		break;
		default: break;
		}
	}
private:
	ObjectGuid playerGUID;
	EventMap events;
};

void AddSC_zuldrak_rg()
{
    new npc_commander_kunz();
    new npc_gorged_lurking_basilisk;
    new npc_ghoul_feeding_kc_bunny();
    new CreatureScriptLoaderEx<npc_nerubian_cocoonAI>("npc_nerubian_cocoon");
    new npc_stefan_vadu();
    new npc_mamtoth_disciple();
    new spell_q12710_summon_escort_aura();
    new npc_malmortis();
    new npc_overlord_drakuru_enemy();
    new npc_lich_king_betrayal_quest();
    new npc_drakurus_skull_trigger();
    new spell_q12512_crusader_kill_credit();
    new spell_q12690_burst_at_the_seams();
    new npc_drakkari_chieftain();
    new spell_create_zuldrak_rat();
    new npc_blight_geist();
	new CreatureScriptLoaderEx<npc_plague_sprayerAI>("npc_plague_sprayer");
}
