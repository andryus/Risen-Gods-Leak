/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2010-2016 Rising-Gods <https://www.rising-gods.de/>
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

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ruins_of_ahnqiraj.h"
#include "ScriptedEscortAI.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"

enum Yells
{
    // The time of our retribution is at hand! Let darkness reign in the hearts of our enemies! Sound: 8645 Emote: 35
    SAY_WAVE3                 = 0,
    SAY_WAVE4                 = 1,
    SAY_WAVE5                 = 2,
    SAY_WAVE6                 = 3,
    SAY_WAVE7                 = 4,
    SAY_INTRO                 = 5,
    SAY_UNK1                  = 6,
    SAY_UNK2                  = 7,
    SAY_UNK3                  = 8,
    SAY_DEATH                 = 9,
    SAY_CHANGEAGGRO           = 10,
    SAY_KILLS_ANDOROV         = 11,
    SAY_COMPLETE_QUEST        = 12    // Yell when realm complete quest 8743 for world event
    // Warriors, Captains, continue the fight! Sound: 8640
};

enum Spells
{
    SPELL_DISARM            = 6713,
    SPELL_FRENZY            = 8269,
    SPELL_THUNDERCRASH      = 25599
};

enum Events
{
    EVENT_DISARM            = 1,        // 03:58:27, 03:58:49
    EVENT_THUNDERCRASH      = 2,        // 03:58:29, 03:58:50
    EVENT_CHANGE_AGGRO      = 3,
};

class boss_rajaxx : public CreatureScript
{
    public:
        boss_rajaxx() : CreatureScript("boss_rajaxx") { }

        struct boss_rajaxxAI : public BossAI
        {
            boss_rajaxxAI(Creature* creature) : BossAI(creature, DATA_RAJAXX) { }

            void Reset() override
            {
                BossAI::Reset();
                enraged = false;
                events.ScheduleEvent(EVENT_DISARM,       10 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_THUNDERCRASH, 12 * IN_MILLISECONDS);
            }

            void JustDied(Unit* killer) override
            {
                //SAY_DEATH
                BossAI::JustDied(killer);
                instance->SetBossState(DATA_RAJAXX, DONE);
                Talk(SAY_DEATH);
                if (Creature* andronov = me->FindNearestCreature(NPC_ANDRONOV, 400.0f, true))
                {
                    andronov->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    andronov->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_VENDOR);
                }                    
            }

            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
                me->CallForHelp(500.0f);
            }

            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/)
            {
                if (!enraged && HealthBelowPct(30))
                {
                    DoCastVictim(SPELL_FRENZY);
                    enraged = true;
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_DISARM:
                            DoCastVictim(SPELL_DISARM);
                            events.ScheduleEvent(EVENT_DISARM, 22 * IN_MILLISECONDS);
                            break;
                        case EVENT_THUNDERCRASH:
                            DoCastSelf(SPELL_THUNDERCRASH);
                            events.ScheduleEvent(EVENT_THUNDERCRASH, 21 * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            private:
                bool enraged;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_rajaxxAI(creature);
        }
};

/*####
## npc_general_andronov
####*/

static Position KaldoreiEliteLoc[] =
{
    { -8880.787109f, 1643.450195f, 21.386433f }, // 1 
    { -8879.288086f, 1648.774658f, 21.386433f }, // 2 
    { -8875.350586f, 1648.345581f, 21.386433f }, // 3 
    { -8873.777344f, 1644.600830f, 21.386433f }  // 4 
};

enum AndronovMisc
{
    // Factions
    FACTION_ANDRONOV_ESCORT  = 250,

    // Spells
    SPELL_AURA_OF_COMMAND    = 25516,
    SPELL_BASH               = 25515,
    SPELL_STRIKE             = 22591,

    // Texts
    SAY_ANDOROV_INTRO        = 0,   // Before for the first wave
    SAY_ANDOROV_ATTACK       = 1,   // Beginning the event

    // Gossips
    GOSSIP_ANDRNOV           = 7047,

    // Events
    EVENT_WAVE_QEEZ          = 1,
    EVENT_WAVE_TUUBID,
    EVENT_WAVE_DRENN,
    EVENT_WAVE_XURREM,
    EVENT_WAVE_YEGGETH,
    EVENT_WAVE_PAKKON,
    EVENT_WAVE_ZERRAN,
    EVENT_WAVE_RAJAXX,
    EVENT_BASH,
    EVENT_COMMAND_AURA,
    EVENT_STRIKE,

    // Datatypes
    DATA_RECIVE_KILL          = 6
};

class npc_general_andronov : public CreatureScript
{
public:
    npc_general_andronov() : CreatureScript("npc_general_andronov") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        player->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            player->CLOSE_GOSSIP_MENU();

            if (npc_escortAI* pEscortAI = CAST_AI(npc_general_andronov::npc_general_andronovAI, creature->AI()))
                pEscortAI->Start(false, true, ObjectGuid::Empty, NULL, false, false);
        }
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 2)
        {
            player->CLOSE_GOSSIP_MENU();
            player->GetSession()->SendListInventory(creature->GetGUID());
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        InstanceScript* instance = creature->GetInstanceScript();

        if (instance->GetBossState(DATA_RAJAXX) != DONE)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_ANDRNOV, 1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

        if (instance->GetBossState(DATA_RAJAXX) == DONE)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_ANDRNOV, 0), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);


        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    }

    struct npc_general_andronovAI : public npc_escortAI
    {
        npc_general_andronovAI(Creature* creature) : npc_escortAI(creature)
        {
            instance = creature->GetInstanceScript();
            SetDespawnAtEnd(false);
            SetDespawnAtFar(false);
        }
       
        void InitializeAI() override
        {
            for (uint8 i = 0; i < 4; i++)
                if (Creature* kaldoreielitist = me->SummonCreature(NPC_KALDOREI_ELITE, KaldoreiEliteLoc[i]))
                {
                   kaldoreielitist->SetOwnerGUID(me->GetGUID());
                   kaldoreielitist->GetMotionMaster()->MoveFollow(me, urand(1.0f, 3.0f), urand(1.0f, 5.0f) + i);
                   kaldoreielitist->SetReactState(REACT_AGGRESSIVE);
                   kaldoreielitist->setFaction(FACTION_ESCORT_H_ACTIVE);
                   kaldoreielitist->SetFacingTo(kaldoreielitist->GetAngle(me));
                }

            me->setFaction(FACTION_ANDRONOV_ESCORT);
            Endwaypoint = false;
        }

        void EnterCombat(Unit* /*who*/) override
        {
            events.ScheduleEvent(EVENT_BASH,          urand(8, 11) * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_COMMAND_AURA,  urand(1, 3)  * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_STRIKE,        urand(2, 5)  * IN_MILLISECONDS);
        }

        void WaypointReached(uint32 waypointId) override
        {
            switch (waypointId)
            {
                case 0:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_CHEER);
                    SetRun(true);
                    break;
                case 8:
                    SetEscortPaused(true);
                    if (!Endwaypoint)
                    {
                        events.ScheduleEvent(EVENT_WAVE_QEEZ, 5 * IN_MILLISECONDS);
                        Endwaypoint = true;
                        Talk(SAY_ANDOROV_INTRO);
                    }
                    break;
            }
        }

        void SetData(uint32 type, uint32 data) override
        {
            if (type == DATA_RECIVE_KILL && data == DATA_RECIVE_KILL)
                events.RescheduleEvent(EVENT_WAVE_QEEZ, 0.1 * IN_MILLISECONDS);
        }

        void JustDied(Unit* /*killer*/) override
        {
            me->DespawnOrUnsummon();
            std::list<Creature*> HelperList;
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_KALDOREI_ELITE, 200.0f);
            if (!HelperList.empty())
                for (std::list<Creature*>::iterator itr = HelperList.begin(); itr != HelperList.end(); itr++)
                    (*itr)->DespawnOrUnsummon();

            if (instance)
            {
                if (Creature* rajaxx = me->FindNearestCreature(NPC_RAJAXX, 200.0f))
                    rajaxx->AI()->Talk(SAY_KILLS_ANDOROV);
            }
        }

        void KilledUnit(Unit* victim) override 
        {
            if (victim->GetEntry() == NPC_RAJAXX)
               Talk(SAY_ANDOROV_ATTACK);
        }

        void MoveInLineOfSight(Unit* who) override
        {
            // If Rajaxx is in range attack him
            if (who->GetEntry() == NPC_RAJAXX && me->IsWithinDistInMap(who, 50.0f))
                AttackStart(who);

            ScriptedAI::MoveInLineOfSight(who);
        }

        void UpdateAI(uint32 diff) override
        {
            npc_escortAI::UpdateAI(diff);

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_WAVE_QEEZ:
                        if (Creature* queez = me->FindNearestCreature(NPC_QEEZ, 400.0f, true))
                        {
                            queez->AI()->AttackStart(me);
                            if (Creature* rajaxx = me->FindNearestCreature(NPC_RAJAXX, 400.0f, true))
                                rajaxx->AI()->Talk(SAY_WAVE3);
                            events.ScheduleEvent(EVENT_WAVE_TUUBID, 60 * IN_MILLISECONDS);
                        }
                        else
                            events.ScheduleEvent(EVENT_WAVE_TUUBID, 0.1 * IN_MILLISECONDS);
                        break;
                    case EVENT_WAVE_TUUBID:
                        if (Creature* tuubid = me->FindNearestCreature(NPC_TUUBID, 400.0f, true))
                        {
                            tuubid->AI()->AttackStart(me);
                            if (Creature* rajaxx = me->FindNearestCreature(NPC_RAJAXX, 400.0f, true))
                                rajaxx->AI()->Talk(SAY_WAVE4);
                            events.ScheduleEvent(EVENT_WAVE_DRENN, 60 * IN_MILLISECONDS);
                        }
                        else
                            events.ScheduleEvent(EVENT_WAVE_DRENN, 0.1 * IN_MILLISECONDS);
                        break;
                    case EVENT_WAVE_DRENN:
                        if (Creature* dreen = me->FindNearestCreature(NPC_DRENN, 400.0f, true))
                        {
                            dreen->AI()->AttackStart(me);
                            if (Creature* rajaxx = me->FindNearestCreature(NPC_RAJAXX, 400.0f, true))
                                rajaxx->AI()->Talk(SAY_WAVE5);
                            events.ScheduleEvent(EVENT_WAVE_XURREM, 60 * IN_MILLISECONDS);
                        }
                        else
                            events.ScheduleEvent(EVENT_WAVE_XURREM, 0.1 * IN_MILLISECONDS);
                        break;
                    case EVENT_WAVE_XURREM:
                        if (Creature* xurrem = me->FindNearestCreature(NPC_XURREM, 400.0f, true))
                        {
                            xurrem->AI()->AttackStart(me);
                            if (Creature* rajaxx = me->FindNearestCreature(NPC_RAJAXX, 400.0f, true))
                                rajaxx->AI()->Talk(SAY_WAVE7);
                            events.ScheduleEvent(EVENT_WAVE_YEGGETH, 60 * IN_MILLISECONDS);
                        }
                        else
                            events.ScheduleEvent(EVENT_WAVE_YEGGETH, 0.1 * IN_MILLISECONDS);
                        break;
                    case EVENT_WAVE_YEGGETH:
                        if (Creature* yeggeth = me->FindNearestCreature(NPC_YEGGETH, 400.0f, true))
                        {
                            yeggeth->AI()->AttackStart(me);
                            if (Creature* rajaxx = me->FindNearestCreature(NPC_RAJAXX, 400.0f, true))
                                rajaxx->AI()->Talk(SAY_UNK1);
                            events.ScheduleEvent(EVENT_WAVE_PAKKON, 60 * IN_MILLISECONDS);
                        }
                        else
                            events.ScheduleEvent(EVENT_WAVE_PAKKON, 0.1 * IN_MILLISECONDS);
                        break;
                    case EVENT_WAVE_PAKKON:
                        if (Creature* pakkon = me->FindNearestCreature(NPC_PAKKON, 400.0f, true))
                        {
                            pakkon->AI()->AttackStart(me);
                            if (Creature* rajaxx = me->FindNearestCreature(NPC_RAJAXX, 400.0f, true))
                                rajaxx->AI()->Talk(SAY_UNK2);
                            events.ScheduleEvent(EVENT_WAVE_ZERRAN, 60 * IN_MILLISECONDS);
                        }
                        else
                            events.ScheduleEvent(EVENT_WAVE_ZERRAN, 0.1 * IN_MILLISECONDS);
                        break;
                    case EVENT_WAVE_ZERRAN:
                        if (Creature* zerran = me->FindNearestCreature(NPC_ZERRAN, 400.0f, true))
                        {
                            zerran->AI()->AttackStart(me);
                            if (Creature* rajaxx = me->FindNearestCreature(NPC_RAJAXX, 400.0f, true))
                                rajaxx->AI()->Talk(SAY_UNK3);
                            events.ScheduleEvent(EVENT_WAVE_RAJAXX, 60 * IN_MILLISECONDS);
                        }
                        else
                            events.ScheduleEvent(EVENT_WAVE_RAJAXX, 0.1 * IN_MILLISECONDS);
                        break;
                    case EVENT_WAVE_RAJAXX:
                        if (Creature* rajaxx = me->FindNearestCreature(NPC_RAJAXX, 400.0f, true))
                        {
                            rajaxx->AI()->AttackStart(me);
                            rajaxx->AI()->Talk(SAY_INTRO);
                        }
                        break;
                                   
                        if (!UpdateVictim())
                            return;

                        events.Update(diff);

                        if (me->HasUnitState(UNIT_STATE_CASTING))
                            return;

                        while (uint32 eventId = events.ExecuteEvent())
                        {
                            switch (eventId)
                            {
                                case EVENT_BASH:
                                    DoCastVictim(SPELL_BASH);
                                    events.ScheduleEvent(EVENT_BASH, urand(12, 15) * IN_MILLISECONDS);
                                    break;
                                case EVENT_COMMAND_AURA:
                                    DoCastSelf(SPELL_AURA_OF_COMMAND, true);
                                    events.ScheduleEvent(EVENT_COMMAND_AURA, urand(10, 20) * IN_MILLISECONDS);
                                    break;
                                case EVENT_STRIKE:
                                    DoCastVictim(SPELL_STRIKE);
                                    events.ScheduleEvent(EVENT_STRIKE, urand(4, 6) * IN_MILLISECONDS);
                                    break;
                                default:
                                    break;
                                           
                            }
                        }

                        DoMeleeAttackIfReady();
                    }
                }
            }
        private:
            InstanceScript* instance;
            EventMap events;
            bool Endwaypoint;
    };
    
    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_general_andronovAI(creature);
    }
};

void AddSC_boss_rajaxx()
{
    new boss_rajaxx();
    new npc_general_andronov();
}
