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
#include "SpellAuras.h"
#include "gundrak.h"
#include "Player.h"

enum Spells
{
    // boss_slad_ran
    SPELL_POISON_NOVA                             = 55081,
    SPELL_POWERFULL_BITE                          = 48287,
    SPELL_VENOM_BOLT                              = 54970,

    // npc_slad_ran_constrictor
    SPELL_GRIP_OF_SLAD_RAN                        = 55093,
    SPELL_SNAKE_WRAP                              = 55126,
    SPELL_VENOMOUS_BITE                           = 54987
};

enum Yells
{
    SAY_AGGRO                                     = 0,
    SAY_SLAY                                      = 1,
    SAY_DEATH                                     = 2,
    SAY_SUMMON_SNAKES                             = 3,
    SAY_SUMMON_CONSTRICTORS                       = 4,
    EMOTE_NOVA                                    = 5
};

enum Creatures
{
    CREATURE_SNAKE                                = 29680,
    CREATURE_CONSTRICTORS                         = 29713
};

enum Events
{
    // boss_slad_ran
    EVENT_POISON_NOVA = 1,
    EVENT_POWERFULL_BITE,
    EVENT_VENOM_BOLT,
    EVENT_SPAWN_VIPER,
    EVENT_SPAWN_CONSTRICTOR,

    // npc_slad_ran_constrictor
    EVENT_GRIP_OF_SLAD_RAN,

    // npc_slad_ran_viperAI
    EVENT_VENOM_BITE,
};

static Position SpawnLoc[]=
{
  {1783.81f, 646.637f, 133.948f, 3.71755f },
  {1775.03f, 606.586f, 134.165f, 1.43117f },
  {1717.39f, 630.041f, 129.282f, 5.96903f },
  {1765.66f, 646.542f, 134.02f,  5.11381f },
  {1716.76f, 635.159f, 129.282f, 0.191986f}
};

#define DATA_SNAKES_WHYD_IT_HAVE_TO_BE_SNAKES 1

struct boss_slad_ranAI : public BossAI
{
    boss_slad_ranAI(Creature* creature) : BossAI(creature, DATA_SLAD_RAN_EVENT) { }

    void Reset() override
    {
        spawnViper = false;
        spawnConstrictor = false;

        wrappedPlayers.clear();
        summons.DespawnAll();

        instance->SetData(DATA_SLAD_RAN_EVENT, NOT_STARTED);
    }

    void EnterCombat(Unit* /*who*/) override
    {
        Talk(SAY_AGGRO);

        instance->SetData(DATA_SLAD_RAN_EVENT, IN_PROGRESS);

        events.ScheduleEvent(EVENT_POISON_NOVA,       Seconds(25));
        events.ScheduleEvent(EVENT_POWERFULL_BITE,    Seconds(3));
        events.ScheduleEvent(EVENT_VENOM_BOLT,        Seconds(15));
        events.ScheduleEvent(EVENT_SPAWN_VIPER,       Seconds(19));
        events.ScheduleEvent(EVENT_SPAWN_CONSTRICTOR, Seconds(20));
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_POISON_NOVA:
                DoCastVictim(SPELL_POISON_NOVA);
                Talk(EMOTE_NOVA);
                events.ScheduleEvent(eventId, Seconds(30));
                break;
            case EVENT_POWERFULL_BITE:
                DoCastVictim(SPELL_POWERFULL_BITE);
                events.ScheduleEvent(eventId, Seconds(10));
                break;
            case EVENT_VENOM_BOLT:
                DoCastVictim(SPELL_VENOM_BOLT);
                events.ScheduleEvent(eventId, Seconds(10));
                break;
            case EVENT_SPAWN_VIPER:
                if (!spawnViper)
                {
                    Talk(SAY_SUMMON_SNAKES);
                    spawnViper = true;
                }
                me->SummonCreature(CREATURE_SNAKE, SpawnLoc[urand(0, 4)], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 20 * IN_MILLISECONDS);
                events.ScheduleEvent(eventId, Seconds(20));
                break;
            case EVENT_SPAWN_CONSTRICTOR:
                if (!spawnConstrictor)
                {
                    Talk(SAY_SUMMON_CONSTRICTORS);
                    spawnConstrictor = true;
                }
                me->SummonCreature(CREATURE_CONSTRICTORS, SpawnLoc[urand(0, 4)], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 20 * IN_MILLISECONDS);
                events.ScheduleEvent(eventId, Seconds(2));
                break;
            default:
                break;
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        Talk(SAY_DEATH);
        summons.DespawnAll();

        instance->SetData(DATA_SLAD_RAN_EVENT, DONE);
    }

    void KilledUnit(Unit* /*victim*/) override
    {
        Talk(SAY_SLAY);
    }

    void JustSummoned(Creature* summoned) override
    {
        if (summoned)
        {
            summoned->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
            summons.Summon(summoned);
        }
    }

    void SetGuidData(ObjectGuid guid, uint32 type) override
    {
        if (type == DATA_SNAKES_WHYD_IT_HAVE_TO_BE_SNAKES)
            wrappedPlayers.insert(guid);
    }

    bool WasWrapped(ObjectGuid guid)
    {
        return wrappedPlayers.count(guid);
    }

    private:
        bool spawnViper;
        bool spawnConstrictor;
        GuidSet wrappedPlayers;
};

struct npc_slad_ran_constrictorAI : public ScriptedAI
{
    npc_slad_ran_constrictorAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_GRIP_OF_SLAD_RAN, Seconds(1));
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_GRIP_OF_SLAD_RAN:
                    if (Unit* victim = me->GetVictim())
                    {
                        DoCastVictim(SPELL_GRIP_OF_SLAD_RAN);
                        _events.ScheduleEvent(eventId, Seconds(urand(3, 6)));

                        Aura* grip = victim->GetAura(SPELL_GRIP_OF_SLAD_RAN, me->GetGUID());
                        if (grip && grip->GetStackAmount() == 5)
                        {
                            victim->RemoveAurasDueToSpell(SPELL_GRIP_OF_SLAD_RAN, me->GetGUID());
                            victim->CastSpell(victim, SPELL_SNAKE_WRAP, true);

                            if (TempSummon* _me = me->ToTempSummon())
                                if (Creature* sladran = _me->GetSummoner()->ToCreature())
                                    sladran->AI()->SetGuidData(victim->GetGUID(), DATA_SNAKES_WHYD_IT_HAVE_TO_BE_SNAKES);

                            me->DespawnOrUnsummon();
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }
private:
    EventMap _events;
};

struct npc_slad_ran_viperAI : public ScriptedAI
{
    npc_slad_ran_viperAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_VENOM_BITE, Seconds(2));
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_VENOM_BITE:
                    DoCastVictim(SPELL_VENOMOUS_BITE);
                    _events.ScheduleEvent(EVENT_VENOM_BITE, Seconds(10));
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }
    private:
        EventMap _events;
};

class achievement_snakes_whyd_it_have_to_be_snakes : public AchievementCriteriaScript
{
    public:
        achievement_snakes_whyd_it_have_to_be_snakes() : AchievementCriteriaScript("achievement_snakes_whyd_it_have_to_be_snakes") { }

        bool OnCheck(Player* player, Unit* target) override
        {
            if (!target)
                return false;

            if (auto sladRanAI = CAST_AI(boss_slad_ranAI, target->GetAI()))
                return !sladRanAI->WasWrapped(player->GetGUID());
            return false;
        }
};

void AddSC_boss_slad_ran()
{
    new CreatureScriptLoaderEx<boss_slad_ranAI>("boss_slad_ran");
    new CreatureScriptLoaderEx<npc_slad_ran_constrictorAI>("npc_slad_ran_constrictor");
    new CreatureScriptLoaderEx<npc_slad_ran_viperAI>("npc_slad_ran_viper");
    new achievement_snakes_whyd_it_have_to_be_snakes();
}
