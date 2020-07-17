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
#include "ScriptedEscortAI.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "Group.h"

/*#######
## npc_captain_saeed
######*/

enum Quest_Data
{
    //Captain Saeed's helper
    NPC_AVENGER                = 21805,
    NPC_DEFENDOR               = 20984,
    NPC_REGENERATOR            = 21783,
    NPC_DIMENSIUS              = 19554,

    EVENT_START_WALK           = 1,
    EVENT_START_FIGHT1         = 2,
    EVENT_START_FIGHT2         = 3,
    EVENT_CLEAVE               = 4,
    EVENT_TRIGGER_ATTACK       = 5,

    DATA_START_ENCOUNTER       = 1,
    DATA_START_FIGHT           = 2,

    SAY_SAEED_0                = 0,
    SAY_SAEED_1                = 1,
    SAY_SAEED_2                = 2,
    SAY_SAEED_3                = 3,
    SAY_DIMENSISIUS_1          = 1,

    QUEST_DIMENSIUS_DEVOURING  = 10439,

    SPELL_DIMENSIUS_TRANSFORM  = 35939,
    SPELL_CLEAVE               = 15496,
};

#define GOSSIP_SAEED_1 "Mir wurde gesagt ihr leitet den Angriff gegen Dimensius."
#define GOSSIP_SAEED_2 "Sagt mir Bescheid, wenn ihr bereit seid. Wir werden auf euer Kommando hin angreifen!"

class npc_captain_saeed : public CreatureScript
{
    public:
        npc_captain_saeed() : CreatureScript("npc_captain_saeed") { }
    
        struct npc_captain_saeedAI : public npc_escortAI
        {
            npc_captain_saeedAI(Creature* creature) : npc_escortAI(creature) { }
            
            void Reset() override
            {
                events.Reset();
                started = false;
                fight = false;
                me->RestoreFaction();
            }

            void MoveInLineOfSight(Unit* who) override
            {
                if (Player* player = GetPlayerForEscort())
                    if (me->GetDistance(who) < 10.0f && !me->GetVictim())
                        if (player->IsValidAttackTarget(who))
                        {
                            AttackStart(who);
                            return;
                        }

                npc_escortAI::MoveInLineOfSight(who);
            }

            void SetGuidData(ObjectGuid playerGUID, uint32 type) override
            {
                if (type == DATA_START_ENCOUNTER)
                {
                    Start(true, true, playerGUID);
                    SetEscortPaused(true);
                    started = true;
                    me->setFaction(FACTION_HORDE_ALLIANCE);
                    Talk(SAY_SAEED_0);
                    events.ScheduleEvent(EVENT_START_WALK, 3 * IN_MILLISECONDS);
                }
                else if (type == DATA_START_FIGHT)
                {
                    Talk(SAY_SAEED_2);
                    SetEscortPaused(false);
                    me->SetUInt32Value(UNIT_NPC_FLAGS, 0);
                }
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                if (fight)
                    SetEscortPaused(false);

                for (uint8 i = 0; i < 2; ++i)
                {
                    if (Creature* Regenerator = ObjectAccessor::GetCreature(*me, RegeneratorGUID[i]))
                    {
                        Regenerator->GetMotionMaster()->Clear(false);
                        Regenerator->GetMotionMaster()->MoveFollow(me, 1.0f, -M_PI / 5 + M_PI / 5 * i);
                    }                        
                }
                for (uint8 i = 0; i < 3; ++i)
                {
                    if (Creature* Defendor = ObjectAccessor::GetCreature(*me, DefendorGUID[i]))
                    {
                        Defendor->GetMotionMaster()->Clear(false);
                        Defendor->GetMotionMaster()->MoveFollow(me, 1.0f, -M_PI / 5 + M_PI / 5 * i);
                    }                        
                }
                for (uint8 i = 0; i < 4; ++i)
                {
                    if (Creature* Avenger = ObjectAccessor::GetCreature(*me, AvengerGUID[i]))
                    {
                        Avenger->GetMotionMaster()->Clear(false);
                        Avenger->GetMotionMaster()->MoveFollow(me, 1.0f, -M_PI / 5 + M_PI / 5 * i);
                    }                       
                }

                npc_escortAI::EnterEvadeMode();
            }

            void WaypointReached(uint32 i) override
            {
                Player* player = GetPlayerForEscort();
                if (!player)
                    return;

                switch (i)
                {
                    case 2:
                        for (uint8 i = 0; i < 2; ++i)
                            if (Creature* Regenerator = me->SummonCreature(NPC_REGENERATOR, me->GetPositionX() + 5, me->GetPositionY() - 5, me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 240000))
                            {
                                RegeneratorGUID[i] = Regenerator->GetGUID();
                                Regenerator->GetMotionMaster()->MoveFollow(me, 1.0f, -M_PI / 5 + M_PI / 5 * i);
                                Regenerator->setFaction(FACTION_HORDE_ALLIANCE);
                            }//Regenerator
                        for (uint8 i = 0; i < 3; ++i)
                            if (Creature* Defendor = me->SummonCreature(NPC_DEFENDOR, me->GetPositionX() - 6, me->GetPositionY() + 6, me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 240000))
                            {
                                DefendorGUID[i] = Defendor->GetGUID();
                                Defendor->GetMotionMaster()->MoveFollow(me, 1.0f, -M_PI / 5 + M_PI / 5 * i);
                                Defendor->setFaction(FACTION_HORDE_ALLIANCE);
                            }//Defendor
                        for (uint8 i = 0; i < 4; ++i)
                            if (Creature* Avenger = me->SummonCreature(NPC_AVENGER, me->GetPositionX() - 1, me->GetPositionY() + 6, me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 240000))
                            {
                                AvengerGUID[i] = Avenger->GetGUID();
                                Avenger->GetMotionMaster()->MoveFollow(me, 1.0f, -M_PI / 5 + M_PI / 5 * i);
                                Avenger->setFaction(FACTION_HORDE_ALLIANCE);
                            }//Avenger
                        break;
                    case 16:
                        if (Player* player = me->FindNearestPlayer(50.0f))
                            Talk(SAY_SAEED_1, player);
                        me->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        SetEscortPaused(true);
                        break;
                    case 18:
                        events.ScheduleEvent(EVENT_START_FIGHT1, 0);
                        SetEscortPaused(true);
                        break;
                    case 19:
                        DespawnHelper();
                        break;
                }
            }

            void EnterCombat(Unit* who) override
            {
                events.ScheduleEvent(EVENT_CLEAVE, 3.5 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_TRIGGER_ATTACK, 3.5 * IN_MILLISECONDS);       
                AttackStartHelper();
            }

            void AttackStartHelper()
            {
                std::list<Creature*> HelperList;
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_REGENERATOR, 50.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_DEFENDOR,    50.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_AVENGER,     50.0f);
                if (!HelperList.empty())
                    for (std::list<Creature*>::iterator itr = HelperList.begin(); itr != HelperList.end(); itr++)
                    {
                        (*itr)->SetHomePosition((*itr)->GetPositionX(), (*itr)->GetPositionY(), (*itr)->GetPositionZ(), (*itr)->GetOrientation());
                        (*itr)->AI()->AttackStart(me->GetVictim());
                    }                        
            }

            void DespawnHelper()
            {
                std::list<Creature*> HelperList;
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_REGENERATOR, 50.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_DEFENDOR,    50.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_AVENGER,     50.0f);
                if (!HelperList.empty())
                    for (std::list<Creature*>::iterator itr = HelperList.begin(); itr != HelperList.end(); itr++)
                        (*itr)->DespawnOrUnsummon();
            }

            void JustDied(Unit* /*killer*/) override
            {
                Player* player = GetPlayerForEscort();
                if (player)
                    player->FailQuest(QUEST_DIMENSIUS_DEVOURING);

                DespawnHelper();
            }

            void CorpseRemoved(uint32&)
            {
                DespawnHelper();
            }

            uint32 GetData(uint32 data) const
            {
                if (data == 1)
                    return (uint32)started;

                return 0;
            }

            void UpdateAI(uint32 diff)
            {
                npc_escortAI::UpdateAI(diff);

                events.Update(diff);

                switch (events.ExecuteEvent())
                {
                    case EVENT_START_WALK:
                        SetEscortPaused(false);
                        break;
                    case EVENT_START_FIGHT1:
                    {
                        Talk(SAY_SAEED_3);
                        me->SummonCreature(NPC_DIMENSIUS, 3936.810f, 2003.839f, 257.528f, 1.0f, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 280000);                                              
                        events.ScheduleEvent(EVENT_START_FIGHT2, 3 * IN_MILLISECONDS);
                        break;
                    }
                    case EVENT_START_FIGHT2:
                        if (Creature* dimensius = me->FindNearestCreature(NPC_DIMENSIUS, 50.0f))
                        {
                            dimensius->RemoveAurasDueToSpell(SPELL_DIMENSIUS_TRANSFORM);
                            dimensius->SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);
                            AttackStart(dimensius);
                            fight = true;
                        }
                        break;
                    case EVENT_CLEAVE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_MINDISTANCE))
                            me->CastSpell(target, SPELL_CLEAVE, true);
                        events.ScheduleEvent(EVENT_CLEAVE, 3.5 * IN_MILLISECONDS);
                        break;
                    case EVENT_TRIGGER_ATTACK:
                        AttackStartHelper();
                        events.ScheduleEvent(EVENT_TRIGGER_ATTACK, 3.5 * IN_MILLISECONDS);
                        break;
                }

                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
            bool started, fight;
            ObjectGuid RegeneratorGUID[2];
            ObjectGuid DefendorGUID[3];
            ObjectGuid AvengerGUID[4];
    };


    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
    {
        player->PlayerTalkClass->ClearMenus();

        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            creature->AI()->SetGuidData(player->GetGUID(), DATA_START_ENCOUNTER);

            if (Group* group = player->GetGroup())
            {
                for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
                    if (Player* member = itr->GetSource())
                        if (member->GetQuestStatus(QUEST_DIMENSIUS_DEVOURING) == QUEST_STATUS_INCOMPLETE)
                            member->KilledMonsterCredit(creature->GetEntry());
            }
            else
            {
                if (player->GetQuestStatus(QUEST_DIMENSIUS_DEVOURING) == QUEST_STATUS_INCOMPLETE)
                    player->KilledMonsterCredit(creature->GetEntry());
            }
        }
        else if (uiAction == GOSSIP_ACTION_INFO_DEF + 2)
        {
            creature->AI()->SetGuidData(player->GetGUID(), DATA_START_FIGHT);
        }

        player->CLOSE_GOSSIP_MENU();
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (player->GetQuestStatus(QUEST_DIMENSIUS_DEVOURING) == QUEST_STATUS_INCOMPLETE)
        {
            if (!creature->AI()->GetData(1))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SAEED_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            else
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SAEED_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        }

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_captain_saeedAI(creature);
    }
};

/*#######
## npc_drijya
######*/

enum DrijyaMisc
{
    // Quests
    QUEST_SABOTAGE_THE_WARP_GATE = 10310,

    // Creatures
    NPC_FELBLADE_DOOMGUARD       = 19853,
    NPC_EXPLODE_TRIGGER          = 20296,
    NPC_TERROR_IMP               = 20399,
    NPC_LEGION_TROOPER           = 20402,
    NPC_LEGION_DESTROYER         = 20403,

    // Texts
    SAY_DRIJYA_START             = 0,
    SAY_DRIJYA_1                 = 1,
    SAY_DRIJYA_2                 = 2,
    SAY_DRIJYA_3                 = 3,
    SAY_DRIJYA_4                 = 4,
    SAY_DRIJYA_5                 = 5,
    SAY_DRIJYA_6                 = 6,
    SAY_DRIJYA_7                 = 7,
    SAY_DRIJYA_COMPLETE          = 8,

    // Spells
    SPELL_SUMMON_SMOKE           = 42456,   // summon temp GO 185318
    SPELL_SUMMON_FIRE            = 42467,   // summon temp GO 185319
    SPELL_EXPLOSION_VISUAL       = 42458,

    // GO_SMOKE                  = 185318,
    // GO_FIRE                   = 185317, // not sure if this one is used
    // GO_BIG_FIRE               = 185319,

    // Counts
    MAX_TROOPERS                 = 9,
    MAX_IMPS                     = 6,
};

class npc_drijya : public CreatureScript
{
    public:
        npc_drijya() : CreatureScript("npc_drijya") { }
    
        bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest)
        {
            if (quest->GetQuestId() == QUEST_SABOTAGE_THE_WARP_GATE)
            {
                if (npc_drijyaAI* EscortAI = CAST_AI(npc_drijya::npc_drijyaAI, creature->AI()))
                {
                    creature->setFaction(FACTION_ESCORT_N_NEUTRAL_ACTIVE);
                    EscortAI->Start(false, false, player->GetGUID());
                }
            }
            return true;
        }

        struct npc_drijyaAI : public npc_escortAI
        {
            npc_drijyaAI(Creature* creature) : npc_escortAI(creature) { }

            void Reset() override
            {
                if (!HasEscortState(STATE_ESCORT_ESCORTING))
                {
                    SpawnCount          = 0;
                    SpawnImpTimer       = 0 * IN_MILLISECONDS;
                    SpawnTrooperTimer   = 0 * IN_MILLISECONDS;
                    SpawnDestroyerTimer = 0 * IN_MILLISECONDS;
                    DestroyingTimer     = 0 * IN_MILLISECONDS;
                }
            }

            void EnterCombat(Unit* who) override
            {
                switch (who->GetEntry())
                {
                   case NPC_TERROR_IMP:
                   case NPC_LEGION_TROOPER:
                   case NPC_LEGION_DESTROYER:
                       if (urand(0, 1))
                           Talk(SAY_DRIJYA_3);
                    break;
                default:
                    break;
                }
            }

            void JustSummoned(Creature* summoned) override
            {
                switch (summoned->GetEntry())
                {
                    case NPC_TERROR_IMP:
                    case NPC_LEGION_TROOPER:
                    case NPC_LEGION_DESTROYER:
                        summoned->setFaction(FACTION_HOSTILE);
                        summoned->AI()->AttackStart(me);
                        break;
                }
            }

            void WaypointReached(uint32 waypointId) override
            {
                Player* player = GetPlayerForEscort();
                if (!player)
                    return;

                switch (waypointId)
                {
                    case 0:
                        Talk(SAY_DRIJYA_START);
                        SetRun(true);
                        break;
                    case 1:
                        Talk(SAY_DRIJYA_1);
                        break;                    
                    case 5:
                        Talk(SAY_DRIJYA_2);
                        break;
                    case 7:
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
                        SetEscortPaused(true);
                        DestroyingTimer = 60 * IN_MILLISECONDS;
                        SpawnImpTimer = 15 * IN_MILLISECONDS;
                        SpawnCount = 0;
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_WORK);
                        break;
                    case 8:
                        DoCastSelf(SPELL_SUMMON_SMOKE);
                        if (Player* player = GetPlayerForEscort())
                            me->SetFacingToObject(player);
                        Talk(SAY_DRIJYA_4);
                        break;
                    case 12:
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
                        SetEscortPaused(true);
                        DestroyingTimer = 60 * IN_MILLISECONDS;
                        SpawnTrooperTimer = 15 * IN_MILLISECONDS;
                        SpawnCount = 0;
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_WORK);
                        break;
                    case 13:
                        DoCastSelf(SPELL_SUMMON_SMOKE);
                        if (Player* player = GetPlayerForEscort())
                            me->SetFacingToObject(player);
                        Talk(SAY_DRIJYA_5);
                        break;
                    case 17:
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
                        SetEscortPaused(true);
                        DestroyingTimer = 60 * IN_MILLISECONDS;
                        SpawnDestroyerTimer = 15 * IN_MILLISECONDS;
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_WORK);
                        break;
                    case 18:
                        DoCastSelf(SPELL_SUMMON_SMOKE);
                        if (Creature* trigger = me->FindNearestCreature(NPC_EXPLODE_TRIGGER, 100.0f))
                            me->SetFacingToObject(trigger);
                        Talk(SAY_DRIJYA_6);
                        break;
                    case 19:
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
                        if (Creature* trigger = me->FindNearestCreature(NPC_EXPLODE_TRIGGER, 100.0f))
                        {
                            trigger->CastSpell(trigger, SPELL_SUMMON_FIRE, true);
                            trigger->CastSpell(trigger, SPELL_EXPLOSION_VISUAL, true);
                        }
                        break;
                    case 20:
                        Talk(SAY_DRIJYA_7);
                        break;
                    case 23:
                        SetRun(false);
                        break;                   
                    case 27:
                        if (Player* player = GetPlayerForEscort())
                        {
                            Talk(SAY_DRIJYA_COMPLETE, player);
                            player->GroupEventHappens(QUEST_SABOTAGE_THE_WARP_GATE, me);
                        }
                        SpawnCount = 0;
                        me->SetRespawnTime(5);
                        me->SetCorpseDelay(0);
                        break;
                    default:
                        break;                
                }
            }

        void JustDied(Unit* /*killer*/)
        {
            if (Player* player = GetPlayerForEscort())
                player->FailQuest(QUEST_SABOTAGE_THE_WARP_GATE);
        }

        void UpdateAI(uint32 diff) override
        {
            npc_escortAI::UpdateAI(diff);

            if (SpawnImpTimer)
            {
                if (SpawnImpTimer <= diff)
                {
                    me->SummonCreature(NPC_TERROR_IMP, 3024.821533f, 2727.803955f, 113.682465f, 0.184732f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 600000);
                    ++SpawnCount;

                    if (SpawnCount == MAX_IMPS)
                        SpawnImpTimer = 0;
                    else
                        SpawnImpTimer = 3.5 * IN_MILLISECONDS;
                }
                else
                    SpawnImpTimer -= diff;
            }

            if (SpawnTrooperTimer)
            {
                if (SpawnTrooperTimer <= diff)
                {
                    me->SummonCreature(NPC_LEGION_TROOPER, 3029.321777f, 2713.804688f, 113.991241f, 4.6889f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 600000);
                    ++SpawnCount;

                    if (SpawnCount == MAX_TROOPERS)
                        SpawnTrooperTimer = 0;
                    else
                        SpawnTrooperTimer = 3.5 * IN_MILLISECONDS;
                }
                else
                    SpawnTrooperTimer -= diff;
            }

            if (SpawnDestroyerTimer)
            {
                if (SpawnDestroyerTimer <= diff)
                {
                    me->SummonCreature(NPC_LEGION_DESTROYER, 3018.869629f, 2721.437256f, 114.945526f, 2.391695f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 600000);
                    SpawnDestroyerTimer = 0;
                }
                else
                    SpawnDestroyerTimer -= diff;
            }

            if (DestroyingTimer)
            {
                if (DestroyingTimer <= diff)
                {
                    SetEscortPaused(false);
                    me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_NONE);
                    DestroyingTimer = 0;
                }
                else
                    DestroyingTimer -= diff;
            }

            DoMeleeAttackIfReady();
        }
        private:
            uint8 SpawnCount;
            uint32 SpawnImpTimer;
            uint32 SpawnTrooperTimer;
            uint32 SpawnDestroyerTimer;
            uint32 DestroyingTimer;
    };  

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_drijyaAI(creature);
    }
};

enum TriangulationPoints
{
    // Creatures
    NPC_TRIANGULATION_DUMMY_ONE = 20086,
    NPC_TRIANGULATION_DUMMY_TWO = 20114,

    // Spells
    SPELL_MARK                  = 1130
};

class spell_triangulation_point_one : public SpellScriptLoader
{
    public:
        spell_triangulation_point_one() : SpellScriptLoader("spell_triangulation_point_one") { }

        class spell_triangulation_point_one_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_triangulation_point_one_SpellScript);

            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Creature* dummy = caster->FindNearestCreature(NPC_TRIANGULATION_DUMMY_ONE, 100.0f))
                    {
                        dummy->AddAura(SPELL_MARK, dummy);
                        caster->SetFacingToObject(dummy);
                    }
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_triangulation_point_one_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_triangulation_point_one_SpellScript();
        }
};

class spell_triangulation_point_two : public SpellScriptLoader
{
    public:
        spell_triangulation_point_two() : SpellScriptLoader("spell_triangulation_point_two") { }

        class spell_triangulation_point_two_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_triangulation_point_two_SpellScript);

            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Creature* dummy = caster->FindNearestCreature(NPC_TRIANGULATION_DUMMY_TWO, 100.0f))
                    {
                        dummy->AddAura(SPELL_MARK, dummy);
                        caster->SetFacingToObject(dummy);
                    }
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_triangulation_point_two_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_triangulation_point_two_SpellScript();
        }
};

class npc_scrap_reaver : public CreatureScript
{
    public:
        npc_scrap_reaver() : CreatureScript("npc_scrap_reaver") { }

        struct npc_scrap_reaverAI : public ScriptedAI
        {
            npc_scrap_reaverAI(Creature* creature) : ScriptedAI(creature) { }

            void OnCharmed(bool apply) override
            {
                if (!apply)
                {
                    me->DespawnOrUnsummon();
                    me->SetRespawnTime(1);
                    me->SetCorpseDelay(1);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new  npc_scrap_reaverAI(creature);
        }
};

enum FelReaverWithHeart
{
    SPELL_TRANSMORPH        = 39311,
    SPELL_FEL_ZAPPER        = 35282,

    NPC_STAXIS_STALKER      = 19642,
    NPC_ZAXXIS_RAIDER       = 18875,

    SAY_START               = 0,
    EVENT_SUMMON_ADD        = 1,
};

Position const addSpawnPositions[2] =
{
    { 2505.380f, 3951.330f, 123.541f, 0.0f },
    { 2544.995f, 4034.498f, 137.039f, 0.0f },
};

class npc_scrapped_fel_reaver : public CreatureScript
{
public:
    npc_scrapped_fel_reaver() : CreatureScript("npc_scrapped_fel_reaver") { }

    struct npc_scrapped_fel_reaverAI : public ScriptedAI
    {
        npc_scrapped_fel_reaverAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->SetImmuneToPC(true);
            me->SetImmuneToNPC(true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            DoCastSelf(SPELL_TRANSMORPH);
        }

        void SpellHit(Unit* caster, const SpellInfo* spell) override
        {
            if (caster && spell && spell->Id == SPELL_FEL_ZAPPER)
            {
                me->SetImmuneToAll(false);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetInCombatWith(caster);
                _events.ScheduleEvent(EVENT_SUMMON_ADD, 5 * IN_MILLISECONDS);
                Talk(SAY_START);
            }
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
        {
            if (summon)
            {
                summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                summon->DespawnOrUnsummon(1000);
            }
        }

        void HandleAdd(Creature* add, Unit* target)
        {
            if (add && target)
            {
                add->GetMotionMaster()->MovePoint(1, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());
                add->SetHomePosition(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), target->GetOrientation());
                add->SetWalk(false);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SUMMON_ADD:
                        x = urand(1, 2);
                        y = urand(1, 2);
                        if (Unit* target = me->GetVictim())
                        {
                            if (x == 1)
                            {
                                if (y == 1)
                                {
                                    if (Creature* add = me->SummonCreature(NPC_STAXIS_STALKER, addSpawnPositions[0], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 40 * IN_MILLISECONDS))
                                        HandleAdd(add, target);
                                }
                                else
                                    if (Creature* add = me->SummonCreature(NPC_ZAXXIS_RAIDER, addSpawnPositions[1], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 40 * IN_MILLISECONDS))
                                        HandleAdd(add, target);
                            }
                            else
                            {
                                if (y == 1)
                                {
                                    if (Creature* add = me->SummonCreature(NPC_ZAXXIS_RAIDER, addSpawnPositions[0], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 40 * IN_MILLISECONDS))
                                        HandleAdd(add, target);
                                }
                                else
                                    if (Creature* add = me->SummonCreature(NPC_STAXIS_STALKER, addSpawnPositions[1], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 40 * IN_MILLISECONDS))
                                        HandleAdd(add, target);
                            }
                        }
                        _events.ScheduleEvent(EVENT_SUMMON_ADD, (urand(15, 25) * IN_MILLISECONDS));
                        break;
                    default:
                        break;
                }
            }
        }
    private:
        EventMap _events;
        uint8    x;
        uint8    y;
    };

    CreatureAI* GetAI(Creature* creature) const override	
    {
        return new npc_scrapped_fel_reaverAI(creature);	
    }	
};

void AddSC_netherstorm_rg()
{
    new npc_captain_saeed();
    new npc_drijya();
    new spell_triangulation_point_one();
    new spell_triangulation_point_two();
    new npc_scrap_reaver();
    new npc_scrapped_fel_reaver();
}
