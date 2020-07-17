/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2010-2018 Rising Gods <http://www.rising-gods.de/>
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
#include "gundrak.h"

enum Texts
{
    EMOTE_SPAWN                 = 0
};

enum Spells
{
    SPELL_ECK_BERSERK           = 55816, // Eck goes berserk, increasing his attack speed by 150% and all damage he deals by 500%.
    SPELL_ECK_BITE              = 55813, // Eck bites down hard, inflicting 150% of his normal damage to an enemy.
    SPELL_ECK_SPIT              = 55814, // Eck spits toxic bile at enemies in a cone in front of him, inflicting 2970 Nature damage and draining 220 mana every 1 sec for 3 sec.
    SPELL_ECK_SPRING_1          = 55815, // Eck leaps at a distant target.  --> Drops aggro and charges a random player. Tank can simply taunt him back.
    SPELL_ECK_SPRING_2          = 55837, // Eck leaps at a distant target.
    
    // Dweller
    SPELL_REGURGITATE           = 55643,
    SPELL_SPRING                = 55652
};

enum Events
{
    EVENT_BITE                  = 1,
    EVENT_SPIT,
    EVENT_SPRING,
    EVENT_BERSERK,

    // Dweller
    EVENT_REGURGITATE,
    EVENT_SPRING_2
};

const Position EckSpawnPoint = { 1643.877930f, 936.278015f, 107.204948f, 0.668432f };

struct boss_eckAI : public BossAI
{
    boss_eckAI(Creature* creature) : BossAI(creature, DATA_ECK_THE_FEROCIOUS_EVENT) { Talk(EMOTE_SPAWN); }

    void Reset() override
    {
        BossAI::Reset();
        _berserk = false;
        instance->SetData(DATA_ECK_THE_FEROCIOUS_EVENT, NOT_STARTED);
    }

    void EnterCombat(Unit* who) override
    {
        BossAI::EnterCombat(who);
        events.ScheduleEvent(EVENT_BITE, Seconds(5));
        events.ScheduleEvent(EVENT_SPIT, Seconds(10));
        events.ScheduleEvent(EVENT_SPRING, Seconds(8));
        events.ScheduleEvent(EVENT_BERSERK, Seconds(urand(60, 90)));
        instance->SetData(DATA_ECK_THE_FEROCIOUS_EVENT, IN_PROGRESS);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage) override
    {
        if (!_berserk && me->HealthBelowPctDamaged(20, damage))
        {
            events.RescheduleEvent(EVENT_BERSERK, Seconds(1));
            _berserk = true;
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        instance->SetData(DATA_ECK_THE_FEROCIOUS_EVENT, DONE);
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_BITE:
                DoCastVictim(SPELL_ECK_BITE);
                events.ScheduleEvent(eventId, Seconds(urand(8, 12)));
                break;
            case EVENT_SPIT:
                DoCastVictim(SPELL_ECK_SPIT);
                events.ScheduleEvent(eventId, Seconds(urand(6, 14)));
                break;
            case EVENT_SPRING:
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 35.0f, true))
                    DoCast(target, RAND(SPELL_ECK_SPRING_1, SPELL_ECK_SPRING_2));
                events.ScheduleEvent(eventId, Seconds(urand(5, 10)));
                break;
            case EVENT_BERSERK:
                DoCast(me, SPELL_ECK_BERSERK);
                _berserk = true;
                break;
            default:
                break;
        }
    }
private:
    bool _berserk;
};

struct npc_ruins_dwellerAI : public ScriptedAI
{
    npc_ruins_dwellerAI(Creature* creature) : ScriptedAI(creature) 
    { 
        instance = creature->GetInstanceScript(); 
    }
            
    void EnterCombat(Unit* /*who*/) override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_REGURGITATE, Seconds(urand(2, 5)));
        _events.ScheduleEvent(EVENT_SPRING_2, Seconds(urand(10, 12)));
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
                case EVENT_REGURGITATE:
                    DoCastVictim(SPELL_REGURGITATE);
                    _events.ScheduleEvent(eventId, Seconds(urand(14, 19)));
                    break;
                case EVENT_SPRING_2:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                        DoCast(target, SPELL_SPRING);
                    if (me->CanHaveThreatList() && !me->GetThreatManager().isThreatListEmpty())
                        ResetThreatList();
                    _events.ScheduleEvent(eventId, Seconds(urand(9, 15)));
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (instance)
        {
            instance->SetGuidData(DATA_RUIN_DWELLER_DIED, me->GetGUID());
            if (instance->GetData(DATA_ALIVE_RUIN_DWELLERS) == 0)
                if (instance->instance->IsHeroic())
                    me->SummonCreature(NPC_ECK, EckSpawnPoint, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5 * MINUTE * IN_MILLISECONDS);
        }
    }
private:
    InstanceScript* instance;
    EventMap _events;
};

void AddSC_boss_eck()
{
    new CreatureScriptLoaderEx<boss_eckAI>("boss_eck");
    new CreatureScriptLoaderEx<npc_ruins_dwellerAI>("npc_ruins_dweller");
}
