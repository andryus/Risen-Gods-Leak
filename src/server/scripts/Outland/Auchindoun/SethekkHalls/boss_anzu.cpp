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

/*
Name: Boss_Anzu
%Complete: 80%
Comment:
Category: Auchindoun, Sethekk Halls
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "sethekk_halls.h"
#include "PassiveAI.h"

enum AnzuTexts
{
    SAY_ANZU_INTRO1               = 0,
    SAY_ANZU_INTRO2               = 1,
    SAY_SUMMON                    = 2,
    SAY_SPELL_BOMB                = 3
};

enum AnzuSpells
{
    SPELL_PARALYZING_SCREECH      = 40184,
    SPELL_SPELL_BOMB              = 40303,
    SPELL_CYCLONE                 = 40321,
    SPELL_BANISH_SELF             = 42354,
    SPELL_SHADOWFORM              = 40973,
    SPELL_PROTECTION_OF_THE_HAWK  = 40237,
    SPELL_SPITE_OF_THE_EAGLE      = 40240,
    SPELL_SPEED_OF_THE_FALCON     = 40241
};

enum AnzuEvents
{
    EVENT_SPELL_SCREECH           = 1,
    EVENT_SPELL_BOMB              = 2,
    EVENT_SPELL_CYCLONE           = 3
};

enum AnzuCreatures
{
    NPC_HAWK_SPIRIT               = 23134,
    NPC_EAGLE_SPIRIT              = 23136,
    NPC_FALCON_SPIRIT             = 23135
};

uint32 AnzuSpirits[] = { NPC_HAWK_SPIRIT, NPC_EAGLE_SPIRIT, NPC_FALCON_SPIRIT };

float AnzuSpiritLoc[][3] = 
{
    { -113.0f, 293.0f, 27.0f },
    { -77.0f,  315.0f, 27.0f },
    { -62.0f,  288.0f, 27.0f }
};

Position const PosSummonBrood[7] =
{
    { -118.1717f, 284.5299f, 121.2287f, 2.775074f },
    { -98.15528f, 293.4469f, 109.2385f, 0.174533f },
    { -99.70160f, 270.1699f, 98.27389f, 6.178465f },
    { -69.25543f, 303.0768f, 97.84479f, 5.532694f },
    { -87.59662f, 263.5181f, 92.70478f, 1.658063f },
    { -73.54323f, 276.6267f, 94.25807f, 2.802979f },
    { -81.70527f, 280.8776f, 44.58830f, 0.526849f }
};

class boss_anzu : public CreatureScript
{
    public:
        boss_anzu() : CreatureScript("boss_anzu") { }

        struct boss_anzuAI : public BossAI
        {
            boss_anzuAI(Creature* creature) : BossAI(creature, DATA_ANZU), summons(me)
            {
                Talk_Timer = 1;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->setFaction(FACTION_FRIENDLY_TO_ALL);
                me->AddAura(SPELL_SHADOWFORM, me);
            }
                   
            void Reset() override
            {
                BossAI::Reset();

                BroodCount = 0;
                BanishedTimes = 2;
                Banish_Timer = 0;
                Banished = false;
            }

            void JustSummoned(Creature* summon) override
            {
                if (summon->GetEntry() == NPC_BROOD_OF_ANZU)
                    BroodCount++;
                summons.Summon(summon);
            }
    
            void SummonedCreatureDies(Creature* summon, Unit* /*who*/) override
            {
                if (summon->GetEntry() == NPC_BROOD_OF_ANZU)
                    BroodCount--;
                summons.Despawn(summon);
                summons.RemoveNotExisting();
            }
    
            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
                events.Reset();
                events.ScheduleEvent(EVENT_SPELL_SCREECH, 14 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SPELL_BOMB,     5 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SPELL_CYCLONE,  8 * IN_MILLISECONDS);
                SummonSpirits();
            }

            void SummonSpirits()
            {
                for (uint8 i = 0; i < 3; i++)
                    me->SummonCreature(AnzuSpirits[i], AnzuSpiritLoc[i][0], AnzuSpiritLoc[i][1], AnzuSpiritLoc[i][2], 0, TEMPSUMMON_MANUAL_DESPAWN, 0);
            }

            void SummonBroods()
            {
                Talk(SAY_SUMMON);
                DoCastSelf(SPELL_BANISH_SELF, true);
                for (uint8 i = 0; i < 5; i++)
                    if (Creature* brood = me->SummonCreature(NPC_BROOD_OF_ANZU, PosSummonBrood[i], TEMPSUMMON_CORPSE_DESPAWN, 1 * IN_MILLISECONDS))
                    {
                        brood->SetReactState(REACT_AGGRESSIVE);
                        brood->SetInCombatWithZone();
                    }                   
            }
    
            void UpdateAI(uint32 diff) override
            {
                if (Talk_Timer)
                {
                    Talk_Timer += diff;
                    if (Talk_Timer >= 1 * IN_MILLISECONDS && Talk_Timer < 10 * IN_MILLISECONDS)
                    {
                        Talk(SAY_ANZU_INTRO1);
                        Talk_Timer = 10 * IN_MILLISECONDS;
                    }
                    else if (Talk_Timer >= 16 * IN_MILLISECONDS)
                    {
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        me->setFaction(FACTION_HOSTILE);
                        me->SetInCombatWithZone();
                        me->RemoveAurasDueToSpell(SPELL_SHADOWFORM);
                        Talk(SAY_ANZU_INTRO2);
                        Talk_Timer = 0;
                    }
                }

                if (Banished)
                {
                    if (BroodCount == 0 || Banish_Timer < diff)
                    {
                        Banished = false;
                        me->RemoveAurasDueToSpell(SPELL_BANISH_SELF);
                    }
                    else
                        Banish_Timer -= diff;
                }
    
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
    
                switch (events.ExecuteEvent())
                {
                    case EVENT_SPELL_SCREECH:
                        DoCastSelf(SPELL_PARALYZING_SCREECH, false);
                        events.Repeat(23 * IN_MILLISECONDS);
                        events.DelayEvents(3 * IN_MILLISECONDS);
                        break;
                    case EVENT_SPELL_BOMB:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                        {
                            if (target->getPowerType() == POWER_MANA)
                            {
                                me->CastSpell(target, SPELL_SPELL_BOMB, false);
                                Talk(SAY_SPELL_BOMB, target);
                            }
                        }
                        events.Repeat(urand(16 * IN_MILLISECONDS, uint32(24.5 * IN_MILLISECONDS)));
                        events.DelayEvents(3 * IN_MILLISECONDS);
                        break;
                    case EVENT_SPELL_CYCLONE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 45.0f, true))
                            me->CastSpell(target, SPELL_CYCLONE, false);
                        events.Repeat(urand(22, 27) * IN_MILLISECONDS);
                        events.DelayEvents(3 * IN_MILLISECONDS);
                        break;
                }

                if (HealthBelowPct(33 * BanishedTimes))
                {
                    BanishedTimes--;
                    Banished = true;
                    Banish_Timer = 45 * IN_MILLISECONDS;
                    events.Repeat(10 * IN_MILLISECONDS);
                    SummonBroods();
                }

                if (!Banished)
                    DoMeleeAttackIfReady();
            }

        private:
            SummonList summons;
            uint32 Talk_Timer;
            uint32 Banish_Timer;
            uint8 BroodCount;
            uint8 BanishedTimes;
            bool Banished;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetSethekkHallsAI<boss_anzuAI>(creature);
        }
};

class npc_anzu_spirit : public CreatureScript
{
    public:
        npc_anzu_spirit() : CreatureScript("npc_anzu_spirit") { }
        
        struct npc_anzu_spiritAI : public PassiveAI
        {
            npc_anzu_spiritAI(Creature* creature) : PassiveAI(creature) { }

            void Reset()
            {
                HotCheckTimer = 3 * IN_MILLISECONDS;
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_16);
            }
        
            void UpdateAI(uint32 diff) override
            {
                if (HotCheckTimer <= diff)
                {
                    if (me->HasAuraType(SPELL_AURA_PERIODIC_HEAL))
                    {
                        switch (me->GetEntry())
                        {
                            case NPC_HAWK_SPIRIT:
                                DoCastSelf(SPELL_PROTECTION_OF_THE_HAWK);
                                break;
                            case NPC_EAGLE_SPIRIT:
                                DoCastSelf(SPELL_SPITE_OF_THE_EAGLE);
                                break;
                            case NPC_FALCON_SPIRIT:
                                DoCastSelf(SPELL_SPEED_OF_THE_FALCON);
                                break;
                            default:
                                break;
                        }    
                    }

                    HotCheckTimer = 3 * IN_MILLISECONDS;
                }
                else
                    HotCheckTimer -= diff;
            }   

        private:
            uint32 HotCheckTimer;
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetSethekkHallsAI<npc_anzu_spiritAI>(creature);
        }
};

void AddSC_boss_anzu()
{
    new boss_anzu();
    new npc_anzu_spirit();
}
