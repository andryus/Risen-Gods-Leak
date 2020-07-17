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
#include "violet_hold.h"

enum Spells
{
    SPELL_BLOODLUST                             = 54516,
    SPELL_BREAK_BONDS                           = 59463,  //Only in heroic
    SPELL_WINDFURY                              = 32910,
    SPELL_CHAIN_HEAL                            = 54481,
    H_SPELL_CHAIN_HEAL                          = 59473,
    SPELL_EARTH_SHIELD                          = 54479,
    H_SPELL_EARTH_SHIELD                        = 59471,
    SPELL_EARTH_SHOCK                           = 54511,
    SPELL_LIGHTNING_BOLT                        = 53044,
    SPELL_STORMSTRIKE                           = 51876
};

enum Yells
{
    SAY_AGGRO                                   = 0,
    SAY_SLAY                                    = 1,
    SAY_DEATH                                   = 2,
    SAY_SPAWN                                   = 3,
    SAY_ADD_KILLED                              = 4,
    SAY_BOTH_ADDS_KILLED                        = 5
};

class boss_erekem : public CreatureScript
{
public:
    boss_erekem() : CreatureScript("boss_erekem") { }

    struct boss_erekemAI : public ScriptedAI
    {
        boss_erekemAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        uint32 BloodlustTimer;
        uint32 ChainHealTimer;
        uint32 EarthShockTimer;
        uint32 LightningBoltTimer;
        uint32 EarthShieldTimer;
        uint32 WindfuryTimer;
        uint32 BreakBondsTimer;
        uint32 StormspikeTimer;


        InstanceScript* instance;

        void Reset()
        {
            BloodlustTimer = 15000;
            ChainHealTimer = 0;
            EarthShockTimer = urand(2000, 8000);
            LightningBoltTimer = urand(5000, 10000);
            EarthShieldTimer = 20000;
            WindfuryTimer = 10000;
            BreakBondsTimer = 25000;
            StormspikeTimer = 1000;

            if (instance)
            {
                if (instance->GetData(DATA_WAVE_COUNT) == 6)
                    instance->SetData(DATA_1ST_BOSS_EVENT, NOT_STARTED);
                else if (instance->GetData(DATA_WAVE_COUNT) == 12)
                    instance->SetData(DATA_2ND_BOSS_EVENT, NOT_STARTED);
            }

            if (Creature* Guard1 = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_EREKEM_GUARD_1)) : ObjectGuid::Empty))
            {
                if (!Guard1->IsAlive())
                    Guard1->Respawn();
            }
            if (Creature* Guard2 = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_EREKEM_GUARD_2)) : ObjectGuid::Empty))
            {
                if (!Guard2->IsAlive())
                    Guard2->Respawn();
            }
        }

        void AttackStart(Unit* who)
        {
            if (me->IsImmuneToPC()|| me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                return;

            if (me->Attack(who, true))
            {
                AddThreat(who, 0.0f);
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);
                DoStartMovement(who);

                if (Creature* Guard1 = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_EREKEM_GUARD_1)) : ObjectGuid::Empty))
                {
                    Guard1->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    Guard1->SetImmuneToPC(false);
                    if (!Guard1->GetVictim() && Guard1->AI())
                        Guard1->AI()->AttackStart(who);
                }
                if (Creature* Guard2 = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_EREKEM_GUARD_2)) : ObjectGuid::Empty))
                {
                    Guard2->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    Guard2->SetImmuneToPC(false);
                    if (!Guard2->GetVictim() && Guard2->AI())
                        Guard2->AI()->AttackStart(who);
                }
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(SAY_AGGRO);
            DoCastSelf(SPELL_EARTH_SHIELD);

            if (instance)
            {
                if (GameObject* Door = instance->instance->GetGameObject(ObjectGuid(instance->GetGuidData(DATA_EREKEM_CELL))))
                    if (Door->GetGoState() == GO_STATE_READY)
                    {
                        EnterEvadeMode();
                        return;
                    }

                if (instance->GetData(DATA_WAVE_COUNT) == 6)
                    instance->SetData(DATA_1ST_BOSS_EVENT, IN_PROGRESS);
                else if (instance->GetData(DATA_WAVE_COUNT) == 12)
                    instance->SetData(DATA_2ND_BOSS_EVENT, IN_PROGRESS);
            }
        }

        void MoveInLineOfSight(Unit* /*who*/) {}

        void UpdateAI(uint32 diff)
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            //spam stormstrike in hc mode if spawns are dead
            if (IsHeroic() && StormspikeTimer <= diff)
            {
                if (Creature* Guard1 = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_EREKEM_GUARD_1)) : ObjectGuid::Empty))
                {
                    if (Creature* Guard2 = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_EREKEM_GUARD_2)) : ObjectGuid::Empty))
                    {
                        if (!Guard1->IsAlive() && !Guard2->IsAlive())                       
                            DoCastVictim(SPELL_STORMSTRIKE);                                              
                    }
                }
                StormspikeTimer = 1*IN_MILLISECONDS;  
            } else StormspikeTimer -= diff;

            if (IsHeroic())
            {
                if (BreakBondsTimer <= diff)
                {
                    DoCastSelf(SPELL_BREAK_BONDS);
                    BreakBondsTimer = 25000;
                } else BreakBondsTimer -= diff;
            }

            if (WindfuryTimer <= diff)
            {
                DoCastSelf(SPELL_WINDFURY);
                WindfuryTimer = 10000;
            } else WindfuryTimer -= diff;

            if (EarthShieldTimer <= diff)
            {
                DoCastSelf(SPELL_EARTH_SHIELD);
                EarthShieldTimer = 20000;
            } else EarthShieldTimer -= diff;

            if (ChainHealTimer <= diff)
            {
                if (ObjectGuid TargetGUID = GetChainHealTargetGUID())
                {
                    if (Creature* target = ObjectAccessor::GetCreature(*me, TargetGUID))
                        DoCast(target, SPELL_CHAIN_HEAL);

                    //If one of the adds is dead spawn heals faster
                    Creature* Guard1 = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_EREKEM_GUARD_1)) : ObjectGuid::Empty);
                    Creature* Guard2 = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_EREKEM_GUARD_2)) : ObjectGuid::Empty);
                    ChainHealTimer = ((Guard1 && !Guard1->IsAlive()) || (Guard2 && !Guard2->IsAlive()) ? 3000 : 8000) + rand32()%3000;
                }
            } else ChainHealTimer -= diff;

            if (BloodlustTimer <= diff)
            {
                DoCastSelf(SPELL_BLOODLUST);
                BloodlustTimer = urand(35000, 45000);
            } else BloodlustTimer -= diff;

            if (EarthShockTimer <= diff)
            {
                DoCastVictim(SPELL_EARTH_SHOCK);
                EarthShockTimer = urand(8000, 13000);
            } else EarthShockTimer -= diff;

            if (LightningBoltTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                    DoCast(target, SPELL_LIGHTNING_BOLT);
                LightningBoltTimer = urand(18000, 24000);
            } else LightningBoltTimer -= diff;

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(SAY_DEATH);

            if (instance)
            {
                if (instance->GetData(DATA_WAVE_COUNT) == 6)
                {
                    instance->SetData(DATA_1ST_BOSS_EVENT, DONE);
                    instance->SetData(DATA_WAVE_COUNT, 7);
                }
                else if (instance->GetData(DATA_WAVE_COUNT) == 12)
                {
                    instance->SetData(DATA_2ND_BOSS_EVENT, DONE);
                    instance->SetData(DATA_WAVE_COUNT, 13);
                }
            }
        }

        void KilledUnit(Unit* victim)
        {
            if (victim == me)
                return;
            Talk(SAY_SLAY);
        }

        ObjectGuid GetChainHealTargetGUID()
        {
            if (HealthBelowPct(85))
                return me->GetGUID();

            Creature* Guard1 = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_EREKEM_GUARD_1)) : ObjectGuid::Empty);
            if (Guard1 && Guard1->IsAlive() && !Guard1->HealthAbovePct(75))
                return Guard1->GetGUID();

            Creature* Guard2 = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_EREKEM_GUARD_2)) : ObjectGuid::Empty);
            if (Guard2 && Guard2->IsAlive() && !Guard2->HealthAbovePct(75))
                return Guard2->GetGUID();

            return ObjectGuid::Empty;
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_erekemAI (creature);
    }
};

enum GuardSpells
{
    SPELL_GUSHING_WOUND                   = 39215,
    SPELL_HOWLING_SCREECH                 = 54462,
    SPELL_STRIKE                          = 14516
};

class npc_erekem_guard : public CreatureScript
{
public:
    npc_erekem_guard() : CreatureScript("npc_erekem_guard") { }

    struct npc_erekem_guardAI : public ScriptedAI
    {
        npc_erekem_guardAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, true);
        }

        uint32 GushingWoundTimer;
        uint32 HowlingScreechTimer;
        uint32 StrikeTimer;

        InstanceScript* instance;

        void Reset()
        {
            StrikeTimer = urand(4000, 8000);
            HowlingScreechTimer = urand(8000, 13000);
            GushingWoundTimer = urand(1000, 3000);
        }

        void AttackStart(Unit* who)
        {
            if (me->IsImmuneToPC() || me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                return;

            if (me->Attack(who, true))
            {
                AddThreat(who, 0.0f);
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);
                DoStartMovement(who);
            }
        }

        void MoveInLineOfSight(Unit* /*who*/) {}

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();

            if (StrikeTimer <= diff)
            {
                DoCastVictim(SPELL_STRIKE);
                StrikeTimer = urand(4000, 8000);
            } else StrikeTimer -= diff;

            if (HowlingScreechTimer <= diff)
            {
                DoCastVictim(SPELL_HOWLING_SCREECH);
                HowlingScreechTimer = urand(8000, 13000);
            } else HowlingScreechTimer -= diff;

            if (GushingWoundTimer <= diff)
            {
                DoCastVictim(SPELL_GUSHING_WOUND);
                GushingWoundTimer = urand(7000, 12000);
            } else GushingWoundTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_erekem_guardAI (creature);
    }
};

void AddSC_boss_erekem()
{
    new boss_erekem();
    new npc_erekem_guard();
}
