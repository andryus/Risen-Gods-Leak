/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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

#include "event.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "ScriptedGossip.h"
#include "GameEventMgr.h"
#include "WorldSession.h"

class npc_firework_trigger : public CreatureScript
{
public:
    npc_firework_trigger() : CreatureScript("npc_firework_trigger") { }

    struct npc_firework_triggerAI : public ScriptedAI
    {
        npc_firework_triggerAI(Creature* c) : ScriptedAI(c) {}

        uint32 castTimer;
        uint32 deathTimer;

        void Reset()
        {
            castTimer = 1000;
            deathTimer = 2000;
        }

        void UpdateAI(uint32 diff)
        {
            if (deathTimer <= diff)
            {
                me->DisappearAndDie();

            }deathTimer -= diff;

            if (castTimer < diff)
            {
                switch(me->GetEntry())
                {
                    case NPC_CLUSTER_BLUE_SMALL:
                        DoCastSelf(SPELL_CLUSTER_BLUE_SMALL, true);
                        break;
                    case NPC_CLUSTER_GREEN_SMALL:
                        DoCastSelf(SPELL_CLUSTER_GREEN_SMALL, true);
                        break;
                    case NPC_CLUSTER_RED_SMALL:
                        DoCastSelf(SPELL_CLUSTER_RED_SMALL, true);
                        break;
                    case NPC_CLUSTER_WHITE_SMALL:
                        DoCastSelf(SPELL_CLUSTER_WHITE_SMALL, true);
                        break;
                    case NPC_CLUSTER_BLUE_BIG:
                        DoCastSelf(SPELL_CLUSTER_BLUE_BIG, true);
                        break;
                    case NPC_CLUSTER_GREEN_BIG:
                        DoCastSelf(SPELL_CLUSTER_GREEN_BIG, true);
                        break;
                    case NPC_CLUSTER_PURPLE_BIG:
                        DoCastSelf(SPELL_CLUSTER_PURPLE_BIG, true);
                        break;
                    case NPC_CLUSTER_RED_BIG:
                        DoCastSelf(SPELL_CLUSTER_RED_BIG, true);
                        break;
                    case NPC_CLUSTER_WHITE_BIG:
                        DoCastSelf(SPELL_CLUSTER_WHITE_BIG, true);
                        break;
                    case NPC_CLUSTER_YELLOW_BIG:
                        DoCastSelf(SPELL_CLUSTER_YELLOW_BIG, true);
                        break;
                    case NPC_RED_YELLOW:
                        DoCastSelf(SPELL_RED_YELLOW, true);
                        break;
                    case NPC_BLUE_SMALL:
                        DoCastSelf(SPELL_BLUE_SMALL, true);
                        break;
                    case NPC_BLUE:
                        DoCastSelf(SPELL_BLUE, true);
                        break;
                    case NPC_RED_SMALL:
                        DoCastSelf(SPELL_RED_SMALL, true);
                        break;
                    case NPC_RED:
                        DoCastSelf(SPELL_RED, true);
                        break;
                    case NPC_RED_VERY_SMALL:
                        DoCastSelf(SPELL_RED_VERY_SMALL, true);
                        break;
                    case NPC_GREEN:
                        DoCastSelf(SPELL_GREEN, true);
                        break;
                    case NPC_PURPLE_BIG:
                        DoCastSelf(SPELL_PURPLE_BIG, true);
                        break;
                    case NPC_FIREWORK_RANDOM:
                        DoCastSelf(SPELL_FIREWORK_RANDOM, true);
                        break;
                    case NPC_FIRWORK_BLUE_FLAMES:
                        DoCastSelf(SPELL_FIRWORK_BLUE_FLAMES, true);
                        break;
                    case NPC_FIREWORK_RED_FLAMES:
                        DoCastSelf(SPELL_FIREWORK_RED_FLAMES, true);
                        break;
                    case NPC_EXPLOSION:
                        DoCastSelf(SPELL_EXPLOSION, true);
                        break;
                    case NPC_EXPLOSION_LARGE:
                        DoCastSelf(SPELL_EXPLOSION_LARGE, true);
                        break;
                    case NPC_EXPLOSION_MEDIUM:
                        DoCastSelf(SPELL_EXPLOSION_MEDIUM, true);
                        break;
                    case NPC_EXPLOSION_SMALL:
                        DoCastSelf(SPELL_EXPLOSION_SMALL, true);
                        break;
                    case NPC_RED_STREAKS:
                        DoCastSelf(SPELL_RED_STREAKS, true);
                        break;
                    case NPC_RED_BLUE_WHITE:
                        DoCastSelf(SPELL_RED_BLUE_WHITE, true);
                        break;
                    case NPC_YELLOW_ROSE:
                        DoCastSelf(SPELL_YELLOW_ROSE, true);
                        break;
                    case NPC_DALARAN_FIRE:
                        DoCastSelf(SPELL_DALARAN_FIRE, true);
                        break;
                    case NPC_FIREWORK:
                        DoCastSelf(SPELL_FIREWORK, true);
                        break;
                    default:
                        break;
                }
            }castTimer -= diff;
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_firework_triggerAI(creature);
    }
};

#define SAY_WRONG "Du hast keinen Mistelzweig"
#define GOSSIP_ITEM_MISTLETOE "Bewerft mich mit einem Mistelzweig!"


class gossip_winter_veil : public CreatureScript
{
public:
    gossip_winter_veil() : CreatureScript("gossip_winter_veil")  {}

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->IsVendor())
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, player->GetOptionTextWithEntry(GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_TEXT_BROWSE_GOODS_ID), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);

        if (creature->IsTrainer())
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, GOSSIP_TEXT_TRAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);

        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (player->HasItemCount(21519, 1, false))
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_ITEM_MISTLETOE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        player->PlayerTalkClass->SendGossipMenu(player->GetGossipTextId(creature),creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
    {
        player->PlayerTalkClass->ClearMenus();

        switch(uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
            {
                switch (creature->GetEntry())
                {
                    case 5661:
                    case 26044:
                    case 31261:
                    case 927:
                    case 1444:
                    case 5484:
                    case 5489:
                    case 1351:
                    case 12336:
                    case 739:
                    case 8140:
                    case 1182:
                        player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2, 26004, 0, creature);
                        break;
                    default:
                        break;
                }
                player->DestroyItemCount(21519, 1, true);
                player->CastSpell(creature, 26004, true);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 2:
                player->GetSession()->SendListInventory(creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF + 3:
                player->GetSession()->SendTrainerList(creature->GetGUID());
                break;
            default:
                player->CLOSE_GOSSIP_MENU();
                break;
        }
        player->CLOSE_GOSSIP_MENU();
        return true;
    }
};

#define SPELL_QCOM_H        71539
#define SPELL_QCOM_A        71522
#define SPELL_CRATE         71459
#define SPELL_COSTUME_DUMMY 71450
#define SPELL_COSTUME_MODEL 34850
#define QUEST_HORDE         24541
#define QUEST_ALLIANCE      24656

class npc_crown_supply_guard : public CreatureScript
{
public:
    npc_crown_supply_guard() : CreatureScript("npc_crown_supply_guard") { }

    struct npc_crown_supply_guardAI : public ScriptedAI
    {
        npc_crown_supply_guardAI(Creature* creature) : ScriptedAI(creature) {}

        void MoveInLineOfSight(Unit* who){
            if(who && who->GetTypeId()==TYPEID_PLAYER){
                Player* player = who->ToPlayer();
                if(player && me->GetDistance2d(player)<=10.0f && player->HasAura(SPELL_COSTUME_DUMMY) && player->HasAura(SPELL_COSTUME_MODEL) && (player->GetQuestStatus(QUEST_HORDE)==QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(QUEST_ALLIANCE)==QUEST_STATUS_INCOMPLETE)){
                    me->CastSpell(player,SPELL_CRATE,true);
                    player->GetTeam() == HORDE ? me->CastSpell(player,SPELL_QCOM_H,true) :  me->CastSpell(player,SPELL_QCOM_A,true);
                }
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_crown_supply_guardAI(creature);
    }
};
#define QUEST_PIFFERING_PARFUME_HORDE       24541
#define QUEST_PIFFERING_PARFUME_ALLIANCE    24656
#define SPELL_COSTUME_DUMMY                 71450
#define SPELL_CRATE                         71459

class npc_piffering_parfume_q_giver : public CreatureScript
{
public:
    npc_piffering_parfume_q_giver() : CreatureScript("npc_piffering_parfume_q_giver") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest){
        if(player && creature && (quest->GetQuestId()==QUEST_PIFFERING_PARFUME_HORDE || quest->GetQuestId()==QUEST_PIFFERING_PARFUME_ALLIANCE)){
            if(player->IsMounted()){
                //Unmount
                player->Dismount();
                player->RemoveAurasByType(SPELL_AURA_MOUNTED);
            }
            //Demorph
            player->DeMorph();
            player->RemoveAurasByType(SPELL_AURA_TRANSFORM);
            player->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);
            //Gnome custome and dummy spell
            player->CastSpell(player,SPELL_COSTUME_DUMMY,true);
        }
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 sender, uint32 action) 
    { 
        if (sender == 0 && action == 1 && (player->GetQuestStatus(QUEST_PIFFERING_PARFUME_HORDE) == QUEST_STATUS_INCOMPLETE 
            || player->GetQuestStatus(QUEST_PIFFERING_PARFUME_HORDE) == QUEST_STATUS_INCOMPLETE))
        {
            if (player->IsMounted())
            {
                //Unmount
                player->Dismount();
                player->RemoveAurasByType(SPELL_AURA_MOUNTED);
            }
            //Demorph
            player->DeMorph();
            player->RemoveAurasByType(SPELL_AURA_TRANSFORM);
            player->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);
            //Gnome custome and dummy spell
            player->CastSpell(player, SPELL_COSTUME_DUMMY, true);
        }
        return false;
    }
};

enum CrashinTrashinRacer
{
    SPELL_RACER_KILL_COUNTER        = 49444,
    SPELL_RACER_SLAM_SLAMMING       = 49440,
    SPELL_COSMETIC_EXPLOSION        = 46419,
    NPC_CRASHIN_THRASHIN_RACER      = 27664,
    NPC_CRASHIN_THRASHIN_RACER_BLUE = 40281,
    NPC_RACER_SLAM_BUNNY            = 27674,
    NPC_KRACHBUMM_TRIGGER           = 1010691
};

class npc_crashin_thrashin_racer : public CreatureScript
{
public:
    npc_crashin_thrashin_racer() : CreatureScript("npc_crashin_thrashin_racer") { }

    struct npc_crashin_thrashin_racerAI : public npc_escortAI
    {
        npc_crashin_thrashin_racerAI(Creature* creature) : npc_escortAI(creature) {}

        ObjectGuid TriggerGUID;

        void Reset()
        {
            TriggerGUID.Clear();

            if (TempSummon* temp = me->ToTempSummon())
            {
                if (Unit* summoner = temp->GetSummoner())
                {
                    if (Player* player = summoner->ToPlayer())
                    {
                        player->RemoveAurasDueToSpell(SPELL_RACER_KILL_COUNTER);
                    }
                }
            }

            if (Creature* trigger = me->SummonCreature(NPC_KRACHBUMM_TRIGGER, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()))
            {
                TriggerGUID = trigger->GetGUID();
                trigger->AI()->SetGuidData(me->GetGUID());
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (Creature* trigger = ObjectAccessor::GetCreature(*me, TriggerGUID))
            {
                trigger->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f);
            }

            std::list<Creature*> racerList;
            me->GetCreatureListWithEntryInGrid(racerList, NPC_KRACHBUMM_TRIGGER, 1.5f);

            if (me->HasAura(SPELL_RACER_SLAM_SLAMMING))
            {
                for (std::list<Creature*>::const_iterator itr = racerList.begin(); itr != racerList.end(); ++itr)
                {
                    if (((*itr)->GetGUID() != TriggerGUID) && (*itr)->IsAlive())
                    {
                        if (Creature* racer = ObjectAccessor::GetCreature(*me, (*itr)->AI()->GetGuidData()))
                        {
                            me->CastSpell(racer, SPELL_COSMETIC_EXPLOSION);
                            me->Kill(racer);
                            racer->DespawnOrUnsummon();
                            me->Kill(*itr);
                            (*itr)->DespawnOrUnsummon();
                            if (TempSummon* temp = me->ToTempSummon())
                            {
                                if (Unit* summoner = temp->GetSummoner())
                                {
                                    if (Player* player = summoner->ToPlayer())
                                    {
                                        player->AddAura(SPELL_RACER_KILL_COUNTER, player);
                                        if (IsHolidayActive(HOLIDAY_FEAST_OF_WINTER_VEIL))
                                        {
                                            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, SPELL_RACER_KILL_COUNTER, 0U, me);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        void JustSummoned(Creature* Summon)
        {
            if (!Summon)
                return;

            if (Summon->GetEntry() == NPC_RACER_SLAM_BUNNY)
            {
                me->GetMotionMaster()->MoveCharge(Summon->GetPositionX(), Summon->GetPositionY(), Summon->GetPositionZ());
            }
        }

        void WaypointReached(uint32 /*waypointId*/) {}
        void AttackStart(Unit* /*who*/) {}
        void EnterCombat(Unit* /*who*/) {}
        void EnterEvadeMode(EvadeReason /*why*/) override {}
        void JustDied(Unit* /*killer*/) {}
        void OnCharmed(bool /*apply*/) {}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_crashin_thrashin_racerAI(creature);
    }

};

class npc_krachbumm_trigger : public CreatureScript
{
public:
    npc_krachbumm_trigger() : CreatureScript("npc_krachbumm_trigger") { }

    struct npc_krachbumm_triggerAI : public ScriptedAI
    {
        npc_krachbumm_triggerAI(Creature* creature) : ScriptedAI(creature) {}

        ObjectGuid RacerGUID;

        void Reset()
        {
            RacerGUID.Clear();
        }

        void SetGuidData(ObjectGuid guid, uint32 /*id*/) override
        {
            RacerGUID = guid;
        }

        ObjectGuid GetGuidData(uint32 /*id*/) const override
        {
            return RacerGUID;
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_krachbumm_triggerAI(creature);
    }
};

void AddSC_event_scripts()
{
    AddSC_event_brewfest();
    AddSC_event_hallows_end();
    AddSC_event_pilgrims_bounty();
    AddSC_event_lunar_festival();
    AddSC_event_childrens_week();
    AddSC_midsummer_fire_festival();
    AddSC_event_love_is_in_the_air();

    new npc_firework_trigger();
    new gossip_winter_veil();
    new npc_crown_supply_guard();
    new npc_piffering_parfume_q_giver();
    new npc_crashin_thrashin_racer();
    new npc_krachbumm_trigger();
}
