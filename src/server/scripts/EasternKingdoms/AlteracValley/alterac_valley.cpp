/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

#define RESET_RANGE 30.0f

enum Spells
{
    SPELL_CHARGE                                  = 22911,
    SPELL_CLEAVE                                  = 40504,
    SPELL_DEMORALIZING_SHOUT                      = 23511,
    SPELL_ENRAGE                                  = 8599,
    SPELL_WHIRLWIND                               = 13736,

    SPELL_NORTH_MARSHAL                           = 45828,
    SPELL_SOUTH_MARSHAL                           = 45829,
    SPELL_STONEHEARTH_MARSHAL                     = 45830,
    SPELL_ICEWING_MARSHAL                         = 45831,
    SPELL_ICEBLOOD_WARMASTER                      = 45822,
    SPELL_TOWER_POINT_WARMASTER                   = 45823,
    SPELL_WEST_FROSTWOLF_WARMASTER                = 45824,
    SPELL_EAST_FROSTWOLF_WARMASTER                = 45826
};

enum Creatures
{
    NPC_VANNDAR                                   = 11948,
    NPC_NORTH_MARSHAL                             = 14762,
    NPC_SOUTH_MARSHAL                             = 14763,
    NPC_ICEWING_MARSHAL                           = 14764,
    NPC_STONEHEARTH_MARSHAL                       = 14765,

    NPC_DREK_THAR                                 = 11946,
    NPC_EAST_FROSTWOLF_WARMASTER                  = 14772,
    NPC_ICEBLOOD_WARMASTER                        = 14773,
    NPC_TOWER_POINT_WARMASTER                     = 14776,
    NPC_WEST_FROSTWOLF_WARMASTER                  = 14777
};

uint32 allianceGroup[5] = 
{
    NPC_VANNDAR,
    NPC_NORTH_MARSHAL,
    NPC_SOUTH_MARSHAL,
    NPC_ICEWING_MARSHAL,
    NPC_STONEHEARTH_MARSHAL
};

uint32 hordeGroup[5] =
{
    NPC_DREK_THAR,
    NPC_EAST_FROSTWOLF_WARMASTER,
    NPC_ICEBLOOD_WARMASTER,
    NPC_TOWER_POINT_WARMASTER,
    NPC_WEST_FROSTWOLF_WARMASTER
};

enum Events
{
    EVENT_CHARGE_TARGET        = 1,
    EVENT_CLEAVE               = 2,
    EVENT_DEMORALIZING_SHOUT   = 3,
    EVENT_WHIRLWIND            = 4,
    EVENT_ENRAGE               = 5,
    EVENT_CHECK_RESET          = 6
};

struct SpellPair
{
    uint32 npcEntry;
    uint32 spellId;
};

uint8 const MAX_SPELL_PAIRS = 8;
SpellPair const _auraPairs[MAX_SPELL_PAIRS] =
{
    { NPC_NORTH_MARSHAL,            SPELL_NORTH_MARSHAL },
    { NPC_SOUTH_MARSHAL,            SPELL_SOUTH_MARSHAL },
    { NPC_STONEHEARTH_MARSHAL,      SPELL_STONEHEARTH_MARSHAL },
    { NPC_ICEWING_MARSHAL,          SPELL_ICEWING_MARSHAL },
    { NPC_EAST_FROSTWOLF_WARMASTER, SPELL_EAST_FROSTWOLF_WARMASTER },
    { NPC_WEST_FROSTWOLF_WARMASTER, SPELL_WEST_FROSTWOLF_WARMASTER },
    { NPC_TOWER_POINT_WARMASTER,    SPELL_TOWER_POINT_WARMASTER },
    { NPC_ICEBLOOD_WARMASTER,       SPELL_ICEBLOOD_WARMASTER }
};

class npc_av_marshal_or_warmaster : public CreatureScript
{
    public:
        npc_av_marshal_or_warmaster() : CreatureScript("npc_av_marshal_or_warmaster") { }

        struct npc_av_marshal_or_warmasterAI : public ScriptedAI
        {
            npc_av_marshal_or_warmasterAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                events.Reset();
                events.ScheduleEvent(EVENT_CHARGE_TARGET, urand(2 * IN_MILLISECONDS, 12 * IN_MILLISECONDS));
                events.ScheduleEvent(EVENT_CLEAVE, urand(1 * IN_MILLISECONDS, 11 * IN_MILLISECONDS));
                events.ScheduleEvent(EVENT_DEMORALIZING_SHOUT, 2000);
                events.ScheduleEvent(EVENT_WHIRLWIND, urand(5 * IN_MILLISECONDS, 20 * IN_MILLISECONDS));
                events.ScheduleEvent(EVENT_ENRAGE, urand(5 * IN_MILLISECONDS, 20 * IN_MILLISECONDS));
                events.ScheduleEvent(EVENT_CHECK_RESET, 5000);

                //Helperlist 
                std::list<Creature*> HelperList;
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_VANNDAR,                  40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_DREK_THAR,                40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_EAST_FROSTWOLF_WARMASTER, 40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_ICEBLOOD_WARMASTER,       40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_TOWER_POINT_WARMASTER,    40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_WEST_FROSTWOLF_WARMASTER, 40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_NORTH_MARSHAL,            40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_SOUTH_MARSHAL,            40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_ICEWING_MARSHAL,          40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_STONEHEARTH_MARSHAL,      40.0f);
                if (!HelperList.empty())
                    for (std::list<Creature*>::iterator itr = HelperList.begin(); itr != HelperList.end(); itr++)
                    {
                        (*itr)->AI()->EnterEvadeMode();
                    }

                _hasAura = false;
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, true);
            }

            void JustRespawned() override
            {
                Reset();
            }

            void EnterCombat(Unit* who) override
            {
                for (uint32 i = 0; i < 5; i++)
                {
                    if (Player* player = who->GetCharmerOrOwnerPlayerOrPlayerItself())
                        if (Creature* creature = me->FindNearestCreature(player->GetTeamId() == TEAM_ALLIANCE ? hordeGroup[i] : allianceGroup[i], 30.0f))
                        {
                            creature->Attack(who, false);
                            AddThreat(who, 1.0f, creature);
                            AddThreat(creature, 1.0f, who);
                            creature->SetInCombatWithZone();
                        }
                }

                //Helperlist 
                std::list<Creature*> HelperList;
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_VANNDAR,                  40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_DREK_THAR,                40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_EAST_FROSTWOLF_WARMASTER, 40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_ICEBLOOD_WARMASTER,       40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_TOWER_POINT_WARMASTER,    40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_WEST_FROSTWOLF_WARMASTER, 40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_NORTH_MARSHAL,            40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_SOUTH_MARSHAL,            40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_ICEWING_MARSHAL,          40.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_STONEHEARTH_MARSHAL,      40.0f);
                if (me->GetVictim())
                    for (std::list<Creature*>::iterator itr = HelperList.begin(); itr != HelperList.end(); itr++)
                    {
                        (*itr)->EngageWithTarget(me->GetVictim());
                        (*itr)->SetInCombatWithZone();
                    }
            }

            void UpdateAI(uint32 diff) override
            {
                // I have a feeling this isn't blizzlike, but owell, I'm only passing by and cleaning up.
                if (!_hasAura)
                {
                    for (uint8 i = 0; i < MAX_SPELL_PAIRS; ++i)
                        if (_auraPairs[i].npcEntry == me->GetEntry())
                            DoCastSelf(_auraPairs[i].spellId);

                    _hasAura = true;
                }

                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CHARGE_TARGET:
                            DoCastVictim(SPELL_CHARGE);
                            events.ScheduleEvent(EVENT_CHARGE, urand(10 * IN_MILLISECONDS, 25 * IN_MILLISECONDS));
                            break;
                        case EVENT_CLEAVE:
                            DoCastVictim(SPELL_CLEAVE);
                            events.ScheduleEvent(EVENT_CLEAVE, urand(10 * IN_MILLISECONDS, 16 * IN_MILLISECONDS));
                            break;
                        case EVENT_DEMORALIZING_SHOUT:
                            DoCastSelf(SPELL_DEMORALIZING_SHOUT);
                            events.ScheduleEvent(EVENT_DEMORALIZING_SHOUT, urand(10 * IN_MILLISECONDS, 15 * IN_MILLISECONDS));
                            break;
                        case EVENT_WHIRLWIND:
                            DoCastSelf(SPELL_WHIRLWIND);
                            events.ScheduleEvent(EVENT_WHIRLWIND, urand(10 * IN_MILLISECONDS, 25 * IN_MILLISECONDS));
                            break;
                        case EVENT_ENRAGE:
                            DoCastSelf(SPELL_ENRAGE);
                            events.ScheduleEvent(EVENT_ENRAGE, urand(10 * IN_MILLISECONDS, 30 * IN_MILLISECONDS));
                            break;
                        case EVENT_CHECK_RESET:
                        {
                            Position const& _homePosition = me->GetHomePosition();
                            if (me->GetDistance2d(_homePosition.GetPositionX(), _homePosition.GetPositionY()) > RESET_RANGE)
                            {
                                EnterEvadeMode();
                                return;
                            }
                            events.ScheduleEvent(EVENT_CHECK_RESET, 5000);
                            break;
                        }
                    }
                    if (me->HasUnitState(UNIT_STATE_CASTING))
                        return;
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
            bool _hasAura;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_av_marshal_or_warmasterAI(creature);
        }
};

enum DrektharSpells
{
    SPELL_WHIRLWIND_DREKTHAR                      = 15589,
    SPELL_WHIRLWIND2                              = 13736,
    SPELL_KNOCKDOWN                               = 19128,
    SPELL_FRENZY                                  = 8269,
    SPELL_SWEEPING_STRIKES                        = 18765, // not sure
    SPELL_WINDFURY                                = 35886, // not sure
    SPELL_STORMPIKE                               = 51876  // not sure
};

enum DrektharYells
{
    YELL_AGGRO                                    = 0,
    YELL_EVADE                                    = 1,
    YELL_RESPAWN                                  = 2,
    YELL_RANDOM                                   = 3
};

enum DrektharEvents
{
    EVENT_WHIRLWIND1_DREKTHAR = 1,
    EVENT_WHIRLWIND2_DREKTHAR,
    EVENT_KNOCKDOWN,
    EVENT_FRENZY,
    EVENT_YELL,
    EVENT_CHECK_COMBAT_AREA
};

class boss_drekthar : public CreatureScript
{
public:
    boss_drekthar() : CreatureScript("boss_drekthar") { }

    struct boss_drektharAI : public ScriptedAI
    {
        boss_drektharAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            events.Reset();
            //Helperlist 
            std::list<Creature*> HelperList;
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_EAST_FROSTWOLF_WARMASTER, 40.0f);
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_ICEBLOOD_WARMASTER,       40.0f);
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_TOWER_POINT_WARMASTER,    40.0f);
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_WEST_FROSTWOLF_WARMASTER, 40.0f);
            if (!HelperList.empty())
                for (std::list<Creature*>::iterator itr = HelperList.begin(); itr != HelperList.end(); itr++)
                {
                    (*itr)->AI()->EnterEvadeMode();
                }
        }

        void EnterCombat(Unit* who)
        {
            Talk(YELL_AGGRO);

            for (uint32 i = 0; i < 5; i++)
                if (Creature* creature = me->FindNearestCreature(hordeGroup[i], 20.0f))
                    creature->SetInCombatWith(who);
            //Helperlist 
            std::list<Creature*> HelperList;
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_EAST_FROSTWOLF_WARMASTER, 40.0f);
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_ICEBLOOD_WARMASTER,       40.0f);
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_TOWER_POINT_WARMASTER,    40.0f);
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_WEST_FROSTWOLF_WARMASTER, 40.0f);
            if (me->GetVictim())
                for (std::list<Creature*>::iterator itr = HelperList.begin(); itr != HelperList.end(); itr++)
                {
                    (*itr)->EngageWithTarget(me->GetVictim());
                    (*itr)->SetInCombatWithZone();
                }

            events.ScheduleEvent(EVENT_WHIRLWIND1_DREKTHAR, urand(1, 20)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_WHIRLWIND2_DREKTHAR, urand(1, 20)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_KNOCKDOWN, 12*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_FRENZY, 6*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_YELL, urand(20, 30)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_CHECK_COMBAT_AREA, 2*IN_MILLISECONDS);
        }

        void JustRespawned()
        {
            Reset();
            Talk(YELL_RESPAWN);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            if (!UpdateVictim())
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_WHIRLWIND1_DREKTHAR:
                        DoCastVictim(SPELL_WHIRLWIND_DREKTHAR);
                        events.ScheduleEvent(EVENT_WHIRLWIND1_DREKTHAR, urand(8, 18)*IN_MILLISECONDS);
                        break;
                    case EVENT_WHIRLWIND2_DREKTHAR:
                        DoCastVictim(SPELL_WHIRLWIND2);
                        events.ScheduleEvent(EVENT_WHIRLWIND2_DREKTHAR, urand(7, 25)*IN_MILLISECONDS);
                        break;
                    case EVENT_KNOCKDOWN:
                        DoCastVictim(SPELL_KNOCKDOWN);
                        events.ScheduleEvent(EVENT_KNOCKDOWN, urand(10, 15)*IN_MILLISECONDS);
                        break;
                    case EVENT_FRENZY:
                        DoCastVictim(SPELL_FRENZY);
                        events.ScheduleEvent(EVENT_FRENZY, urand(20, 30)*IN_MILLISECONDS);
                        break;
                    case EVENT_YELL:
                        Talk(YELL_RANDOM);
                        events.ScheduleEvent(EVENT_YELL, urand(20, 30)*IN_MILLISECONDS);
                        break;
                    case EVENT_CHECK_COMBAT_AREA:
                        if (me->GetDistance2d(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY()) > RESET_RANGE)
                        {
                            EnterEvadeMode();
                            Talk(YELL_EVADE);
                        }
                        events.ScheduleEvent(EVENT_CHECK_COMBAT_AREA, 2*IN_MILLISECONDS);
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_drektharAI(creature);
    }
};

enum VanndarYells
{
    YELL_AGGRO_VANNDAR                            = 0,
    YELL_EVADE_VANNDAR                            = 1,
  //YELL_RESPAWN1                                 = -1810010, // Missing in database
  //YELL_RESPAWN2                                 = -1810011, // Missing in database
    YELL_RANDOM_VANNDAR                           = 2,
    YELL_SPELL_VANNDAR                            = 3,
};

enum VanndarSpells
{
    SPELL_AVATAR                                  = 19135,
    SPELL_THUNDERCLAP                             = 15588,
    SPELL_STORMBOLT                               = 20685 // not sure
};

enum VanndarEvents
{
    EVENT_AVATAR = 1,
    EVENT_THUNDERCLAP,
    EVENT_STORMBOLT,
  //EVENT_YELL,
  //EVENT_CHECK_COMBAT_AREA
};

class boss_vanndar : public CreatureScript
{
public:
    boss_vanndar() : CreatureScript("boss_vanndar") { }

    struct boss_vanndarAI : public ScriptedAI
    {
        boss_vanndarAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            events.Reset();
            //Helperlist 
            std::list<Creature*> HelperList;
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_NORTH_MARSHAL,       40.0f);
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_SOUTH_MARSHAL,       40.0f);
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_ICEWING_MARSHAL,     40.0f);
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_STONEHEARTH_MARSHAL, 40.0f);
            if (!HelperList.empty())
                for (std::list<Creature*>::iterator itr = HelperList.begin(); itr != HelperList.end(); itr++)
                {
                    (*itr)->AI()->EnterEvadeMode();
                }
        }

        void EnterCombat(Unit* who)
        {
            Talk(YELL_AGGRO_VANNDAR);
            
            for (uint32 i = 0; i < 5; i++)
                if (Creature* creature = me->FindNearestCreature(allianceGroup[i], 20.0f))
                    creature->SetInCombatWith(who);

            //Helperlist 
            std::list<Creature*> HelperList;
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_NORTH_MARSHAL,       40.0f);
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_SOUTH_MARSHAL,       40.0f);
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_ICEWING_MARSHAL,     40.0f);
            me->GetCreatureListWithEntryInGrid(HelperList, NPC_STONEHEARTH_MARSHAL, 40.0f);
            if (me->GetVictim())
                for (std::list<Creature*>::iterator itr = HelperList.begin(); itr != HelperList.end(); itr++)
                {
                    (*itr)->EngageWithTarget(me->GetVictim());
                    (*itr)->SetInCombatWithZone();
                }

            events.ScheduleEvent(EVENT_AVATAR, 3*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_THUNDERCLAP, 4*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_STORMBOLT, 6*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_YELL, urand(20, 30)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_CHECK_COMBAT_AREA, 2*IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            if (!UpdateVictim())
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_AVATAR:
                        DoCastVictim(SPELL_AVATAR);
                        events.ScheduleEvent(EVENT_AVATAR, urand(15, 20)*IN_MILLISECONDS);
                        break;
                    case EVENT_THUNDERCLAP:
                        DoCastVictim(SPELL_THUNDERCLAP);
                        events.ScheduleEvent(EVENT_AVATAR, urand(5, 15)*IN_MILLISECONDS);
                        break;
                    case EVENT_STORMBOLT:
                        DoCastVictim(SPELL_STORMBOLT);
                        Talk(YELL_SPELL_VANNDAR);
                        events.ScheduleEvent(EVENT_STORMBOLT, urand(10, 25)*IN_MILLISECONDS);
                        break;
                    case EVENT_YELL:
                        Talk(YELL_RANDOM_VANNDAR);
                        events.ScheduleEvent(EVENT_YELL, urand(20, 30)*IN_MILLISECONDS);
                        break;
                    case EVENT_CHECK_COMBAT_AREA:
                        if (me->GetDistance2d(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY()) > RESET_RANGE)
                        {
                            EnterEvadeMode();
                            Talk(YELL_EVADE_VANNDAR);
                        }
                        events.ScheduleEvent(EVENT_CHECK_COMBAT_AREA, 2*IN_MILLISECONDS);
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_vanndarAI(creature);
    }
};

enum HordeFactionBossMisc
{
    // Spells
    SPELL_BOLT                  = 15234,
    SPELL_SHOCK                 = 15616,
    SPELL_EARTHBIND_TOTEM       = 15786,
    SPELL_HEALING_WAVE          = 15982,
    SPELL_SUMMONING             = 21267,

    // Quests
    QUEST_ICE_LORD              = 6801,
    QUEST_GALLON_OF_BLOOD       = 7385,

    // Waypoints
    WP_FINAL_POS                = 77,

    // Gameobjects
    GO_ALTAR_OF_SUMMONING       = 178465,

    // Events
    EVENT_BOLT                  = 1,
    EVENT_SHOCK                 = 2,
    EVENT_EARTHBIND_TOTEM       = 3,
    EVENT_HEALING_WAVE          = 4,

    // Actions
    ACTION_COUNT_ONE            = 1,
    ACTION_COUNT_FIVE           = 5,

    // Creatures
    NPC_LOKHOLAR                = 13256,
    NPC_GUARD                   = 13284,

    // Texts
    SAY_SUMMON                  = 0
};

class npc_primalist_thurloga : public CreatureScript
{
    public:
        npc_primalist_thurloga() : CreatureScript("npc_primalist_thurloga") { }

        bool OnQuestReward(Player* /*player*/, Creature* creature, Quest const* quest, uint32 /*opt*/)
        {
            switch (quest->GetQuestId())
            {
                case QUEST_ICE_LORD:
                    creature->AI()->DoAction(ACTION_COUNT_ONE);
                    break;
                case QUEST_GALLON_OF_BLOOD:
                    creature->AI()->DoAction(ACTION_COUNT_FIVE);
                    break;
            }

            return false;
        }

        struct npc_primalist_thurlogaAI : public npc_escortAI
        {
            npc_primalist_thurlogaAI(Creature* creature) : npc_escortAI(creature) { }

            void InitializeAI() override
            {
                QuestCounter = 0;
                SummonCounter = 0;
                StartEvent = false;
                Summoned = false;
            }

            void EnterCombat(Unit* who) override
            {
                events.ScheduleEvent(EVENT_BOLT,            urand(2, 3)   * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SHOCK,           urand(5, 6)   * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_EARTHBIND_TOTEM, urand(2, 4)   * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_HEALING_WAVE,    urand(15, 20) * IN_MILLISECONDS);
            }

            void DoAction(int32 action) override
            {
                if (action == ACTION_COUNT_ONE)
                    ++QuestCounter;

                if (action == ACTION_COUNT_FIVE)
                {
                    for (uint8 i = 0; i < 5; ++i)
                        ++QuestCounter;
                }
            }

            void SetData(uint32 type, uint32 data) override
            {
                if (type == 1 && data == 1)
                {
                    ++SummonCounter;
                }
            }

            void WaypointReached(uint32 waypointId) override
            {
                if (waypointId == WP_FINAL_POS)
                {
                    Talk(SAY_SUMMON);
                    me->setFaction(FACTION_FRIENDLY_TO_ALL);
                    DoCastSelf(SPELL_SUMMONING);
                    me->SummonGameObject(GO_ALTAR_OF_SUMMONING, -362.974f, -129.988f, 26.4231f, 3.9156f, 0.0f, 0.0f, 0.798636f, 0.601815f, 120000);
                }
            }

            void UpdateAI(uint32 diff) override
            {
                npc_escortAI::UpdateAI(diff);

                if (!StartEvent)
                {
                    if (QuestCounter >= 10)
                    {
                        StartEvent = true;
                        Start(true, true);
                        SetDespawnAtFar(false);
                        SetDespawnAtEnd(false);
                        me->setActive(true);

                        //Helperlist 
                        std::list<Creature*> HelperList;
                        me->GetCreatureListWithEntryInGrid(HelperList, NPC_GUARD, 200.0f);
                        if (!HelperList.empty())
                            for (std::list<Creature*>::iterator itr = HelperList.begin(); itr != HelperList.end(); itr++)
                            {
                                (*itr)->SetOwnerGUID(me->GetGUID());
                                (*itr)->GetMotionMaster()->MoveFollow(me, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE + urand(1.0f, 3.0f));
                                (*itr)->SetReactState(REACT_AGGRESSIVE);
                                (*itr)->SetFacingTo((*itr)->GetAngle(me));
                                (*itr)->SetSpeedRate(MOVE_RUN, 2.0f);
                                (*itr)->SetSpeedRate(MOVE_WALK, 2.0f);
                            }
                    }
                }

                if (!Summoned)
                {
                    if (SummonCounter >= 10)
                    {
                        Summoned = true;
                        me->SummonCreature(NPC_LOKHOLAR, -317.794495f, -171.622177f, 9.259236f, 5.576638f, TEMPSUMMON_CORPSE_DESPAWN, 0);
                    }
                }

                events.Update(diff);

                if (!UpdateVictim())
                    return;
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_BOLT:
                            DoCastVictim(SPELL_BOLT);
                            events.ScheduleEvent(EVENT_BOLT, urand(4, 5)*IN_MILLISECONDS);
                            break;
                        case EVENT_SHOCK:
                            DoCastVictim(SPELL_SHOCK);
                            events.ScheduleEvent(EVENT_SHOCK, urand(14000, 17000)*IN_MILLISECONDS);
                            break;
                        case EVENT_EARTHBIND_TOTEM:
                            DoCastSelf(EVENT_EARTHBIND_TOTEM, true);
                            break;
                        case EVENT_HEALING_WAVE:
                            DoCastSelf(SPELL_HEALING_WAVE);
                            events.ScheduleEvent(EVENT_HEALING_WAVE, urand(12000, 17000)*IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

    private:
        EventMap events;
        uint32 QuestCounter;
        uint32 SummonCounter;
        bool StartEvent;
        bool Summoned;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_primalist_thurlogaAI(creature);
    }
};

enum AllianceFactionBossMisc
{
    // Spells
    SPELL_MOONFIRE           = 22206,
    SPELL_ENTANGLING_ROOTS   = 22127,
    SPELL_REJUVENATION       = 15981,

    // Quests
    QUEST_IVUS               = 6881,
    QUEST_CRYSTAL_CLUSTER    = 7386,

    // Waypoints
    WP_FINAL                 = 79,

    // Gameobjects
    GO_CIRCLE_OF_CALLING     = 178670,

    // Events
    EVENT_MOONFIRE           = 1,
    EVENT_ENTANGLING_ROOTS   = 2,
    EVENT_REJUVENATION       = 3,

    // Creatures
    NPC_IVUS                 = 13419,
    NPC_DRUID                = 13443,

    // Misc
    MOUNT_MODEL              = 6444
};

class npc_arch_druid_renferal : public CreatureScript
{
    public:
        npc_arch_druid_renferal() : CreatureScript("npc_arch_druid_renferal") { }

        bool OnQuestReward(Player* /*player*/, Creature* creature, Quest const* quest, uint32 /*opt*/)
        {
            switch (quest->GetQuestId())
            {
                case QUEST_IVUS:
                    creature->AI()->DoAction(ACTION_COUNT_ONE);
                    break;
                case QUEST_CRYSTAL_CLUSTER:
                    creature->AI()->DoAction(ACTION_COUNT_FIVE);
                    break;
            }

            return false;
        }

        struct npc_arch_druid_renferalAI : public npc_escortAI
        {
            npc_arch_druid_renferalAI(Creature* creature) : npc_escortAI(creature) { }

            void InitializeAI() override
            {
                QuestCounter  = 0;
                SummonCounter = 0;
                StartEvent    = false;
                Summoned      = false;
            }

            void EnterCombat(Unit* who) override
            {
                events.ScheduleEvent(EVENT_MOONFIRE,         urand(2, 3)   * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_ENTANGLING_ROOTS, urand(10, 12) * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_REJUVENATION,     urand(15, 20) * IN_MILLISECONDS);
            }

            void DoAction(int32 action) override
            {
                if (action == ACTION_COUNT_ONE)
                    ++QuestCounter;

                if (action == ACTION_COUNT_FIVE)
                {
                    for (uint8 i = 0; i < 5; ++i)
                        ++QuestCounter;
                }
            }

            void SetData(uint32 type, uint32 data) override
            {
                if (type == 1 && data == 1)
                {
                    ++SummonCounter;
                
                }
            }

            void WaypointReached(uint32 waypointId) override
            {
                if (waypointId == WP_FINAL)
                {
                    Talk(SAY_SUMMON);
                    me->setFaction(FACTION_FRIENDLY_TO_ALL);
                    DoCastSelf(SPELL_SUMMONING);
                    me->SummonGameObject(GO_CIRCLE_OF_CALLING, -199.008636f, -344.225616f, 6.781697f, 2.553954f, 0.0f, 0.0f, 0.798636f, 0.601815f, 120000);
                }
            }

            void UpdateAI(uint32 diff) override
            {
                npc_escortAI::UpdateAI(diff);

                if (!StartEvent)
                {
                    if (QuestCounter >= 10)
                    {
                        StartEvent = true;
                        Start(true, true);
                        SetDespawnAtFar(false);
                        SetDespawnAtEnd(false);
                        me->Mount(MOUNT_MODEL);
                        me->SetSpeedRate(MOVE_RUN, 1.6f);
                        me->setActive(true);

                        // Helperlist 
                        std::list<Creature*> HelperList;
                        me->GetCreatureListWithEntryInGrid(HelperList, NPC_DRUID, 40.0f);
                        if (!HelperList.empty())
                            for (std::list<Creature*>::iterator itr = HelperList.begin(); itr != HelperList.end(); itr++)
                            {
                                (*itr)->SetOwnerGUID(me->GetGUID());
                                (*itr)->GetMotionMaster()->MoveFollow(me, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE + urand(1.0f, 3.0f));
                                (*itr)->SetReactState(REACT_AGGRESSIVE);
                                (*itr)->SetFacingTo((*itr)->GetAngle(me));
                                (*itr)->Mount(MOUNT_MODEL);
                                (*itr)->SetSpeedRate(MOVE_RUN, 1.6f);
                            }
                    }
                }

                if (!Summoned)
                {
                    if (SummonCounter >= 10)
                    {
                        Summoned = true;
                        me->SummonCreature(NPC_IVUS, -233.976944f, -320.777557f, 6.790611f, 2.553954f, TEMPSUMMON_CORPSE_DESPAWN, 0);
                    }
                }

                events.Update(diff);

                if (!UpdateVictim())
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_MOONFIRE:
                            DoCastVictim(SPELL_MOONFIRE);
                            events.ScheduleEvent(EVENT_MOONFIRE, urand(3, 4) * IN_MILLISECONDS);
                            break;
                        case EVENT_ENTANGLING_ROOTS:
                            DoCastVictim(SPELL_ENTANGLING_ROOTS);
                            events.ScheduleEvent(EVENT_ENTANGLING_ROOTS, urand(10, 12) * IN_MILLISECONDS);
                            break;
                        case EVENT_REJUVENATION:
                            DoCastSelf(SPELL_REJUVENATION);
                            events.ScheduleEvent(EVENT_REJUVENATION, urand(15, 20) * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

    private:
        EventMap events;
        uint32 QuestCounter;
        uint32 SummonCounter;
        bool StartEvent;
        bool Summoned;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_arch_druid_renferalAI(creature);
    }
};

/*######
## npc_stable_master
######*/

enum StableMasterMisc
{
    NPC_FROSTWOLF         = 10981,
    NPC_ALTERAC_RAM       = 10990,
    QUEST_EMPTY_STABLES_H = 7001,
    QUEST_EMPTY_STABLES_A = 7027,
};

class npc_stable_master : public CreatureScript
{
    public:
        npc_stable_master() : CreatureScript("npc_stable_master") { }

        struct npc_daranelleAI : public ScriptedAI
        {
            npc_daranelleAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override { }

            void EnterCombat(Unit* /*who*/) override { }

            void MoveInLineOfSight(Unit* who) override
            {
                if (who->GetEntry() == NPC_FROSTWOLF && me->IsWithinDistInMap(who, 5.0f))
                {
                    if (Creature* frostwolf = me->FindNearestCreature(NPC_FROSTWOLF, 10.0f))
                        frostwolf->DespawnOrUnsummon();
                    if (Player* player = me->FindNearestPlayer(10.0f))
                        player->CompleteQuest(QUEST_EMPTY_STABLES_H);
                }

                if (who->GetEntry() == NPC_ALTERAC_RAM && me->IsWithinDistInMap(who, 5.0f))
                {
                    if (Creature* ram = me->FindNearestCreature(NPC_ALTERAC_RAM, 10.0f))
                        ram->DespawnOrUnsummon();
                    if (Player* player = me->FindNearestPlayer(10.0f))
                        player->CompleteQuest(QUEST_EMPTY_STABLES_A);
                }
                
                ScriptedAI::MoveInLineOfSight(who);
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_daranelleAI(creature);
        }
};

void AddSC_alterac_valley()
{
    new npc_av_marshal_or_warmaster();
    new boss_drekthar();
    new boss_vanndar();
    new npc_primalist_thurloga();
    new npc_arch_druid_renferal();
    new npc_stable_master();
}
