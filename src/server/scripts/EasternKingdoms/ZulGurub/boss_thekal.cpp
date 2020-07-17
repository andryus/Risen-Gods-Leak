/*
 * Copyright (C) 2008-2017 TrinityCore <http://www.trinitycore.org/>
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

/* ScriptData
SDName: Boss_Thekal
SD%Complete: 95
SDComment: Almost finished.
SDCategory: Zul'Gurub
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "zulgurub.h"
#include "SpellInfo.h"

enum Says
{
    SAY_AGGRO                 = 0,
    SAY_DEATH                 = 1
};

enum Spells
{
    SPELL_MORTAL_CLEAVE       = 22859,
    SPELL_SILENCE             = 23207,
    SPELL_FRENZY              = 23128,
    SPELL_FORCE_PUNCH         = 24189,
    SPELL_CHARGE              = 24408,
    SPELL_ENRAGE              = 23537,
    SPELL_SUMMON_TIGERS       = 24183,
    SPELL_TIGER_FORM          = 24169,
    SPELL_RESURRECT           = 24173,

    // Zealot Lor'Khan Spells
    SPELL_SHIELD              = 25020,
    SPELL_BLOODLUST           = 24185,
    SPELL_GREATER_HEAL        = 24208,
    SPELL_DISARM              = 22691,

    // Zealot Zath Spells
    SPELL_SWEEPING_STRIKES    = 18765,
    SPELL_SINISTER_STRIKE     = 15667,
    SPELL_GOUGE               = 24698,
    SPELL_KICK                = 15614,
    SPELL_BLIND               = 21060
};

enum Actions
{
    ACTION_PREVENT_REVIVE     = 1
};

enum Phases
{
    PHASE_NORMAL              = 1,
    PHASE_FAKE_DEATH          = 2,
    PHASE_WAITING             = 3,
    PHASE_TIGER               = 4
};

// abstract base class for faking death
struct boss_thekalBaseAI : public ScriptedAI
{
    boss_thekalBaseAI(Creature* creature) : ScriptedAI(creature)
    {
        Phase = PHASE_NORMAL;
    }

    uint8 Phase;

    virtual void OnFakeingDeath() {}
    virtual void OnRevive() {}

    void DamageTaken(Unit* /*killer*/, uint32& damage) override
    {
        if (damage < me->GetHealth())
            return;

        // Prevent glitch if in fake death
        if (Phase == PHASE_FAKE_DEATH || Phase == PHASE_WAITING)
        {
            damage = 0;
            return;
        }

        // Only init fake in normal phase
        if (Phase != PHASE_NORMAL)
            return;

        damage = 0;

        me->InterruptNonMeleeSpells(true);
        me->SetHealth(0);
        me->StopMoving();
        me->ClearComboPointHolders();
        me->RemoveAllAurasOnDeath();
        me->ModifyAuraState(AURA_STATE_HEALTHLESS_20_PERCENT, false);
        me->ModifyAuraState(AURA_STATE_HEALTHLESS_35_PERCENT, false);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        me->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE_PERCENT);
        me->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
        me->RemoveAurasByType(SPELL_AURA_PERIODIC_LEECH);
        me->ClearAllReactives();
        me->GetMotionMaster()->Clear();
        me->GetMotionMaster()->MoveIdle();
        me->SetStandState(UNIT_STAND_STATE_DEAD);

        Phase = PHASE_FAKE_DEATH;

        OnFakeingDeath();
    }

    void Revive(bool OnlyFlags = false)
    {
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        me->SetStandState(UNIT_STAND_STATE_STAND);

        if (OnlyFlags)
            return;

        me->SetHealth(me->GetMaxHealth());
        Phase = PHASE_NORMAL;

        ResetThreatList();
        Reset();

        // Assume Attack
        if (me->GetVictim())
            me->GetMotionMaster()->MoveChase(me->GetVictim());

        OnRevive();
    }

    void PreventRevive()
    {
        if (me->IsNonMeleeSpellCast(true))
            me->InterruptNonMeleeSpells(true);

        Phase = PHASE_WAITING;
    }
};

class boss_thekal : public CreatureScript
{
    public:
        boss_thekal() : CreatureScript("boss_thekal") { }

        struct boss_thekalAI : public boss_thekalBaseAI
        {
            boss_thekalAI(Creature* creature) : boss_thekalBaseAI(creature)
            {
                instance = creature->GetInstanceScript();
            }
                            
            void Reset() override
            {
                MortalCleaveTimer     = 4 * IN_MILLISECONDS;
                SilenceTimer          = 9 * IN_MILLISECONDS;
                FrenzyTimer           = 30 * IN_MILLISECONDS;
                ForcePunchTimer       = 4 * IN_MILLISECONDS;
                ChargeTimer           = 12 * IN_MILLISECONDS;
                EnrageTimer           = 32 * IN_MILLISECONDS;
                SummonTigersTimer     = 25 * IN_MILLISECONDS;
                ResurrectTimer        = 10 * IN_MILLISECONDS;
                Phase                 = PHASE_NORMAL;
        
                Enraged = false;
        
                // remove fake death
                Revive(true);
            }
        
            void EnterCombat(Unit* /*who*/) override
            {
                Talk(SAY_AGGRO);
            }
        
            void JustDied(Unit* /*killer*/) override
            {
                Talk(SAY_DEATH);
        
                instance->SetBossState(DATA_THEKAL, DONE);
        
                // remove the two adds
                Unit* Lorkhan = ObjectAccessor::GetUnit(*me, ObjectGuid(instance->GetGuidData(DATA_LORKHAN)));
                Unit* pZath = ObjectAccessor::GetUnit(*me, ObjectGuid(instance->GetGuidData(DATA_ZATH)));
        
                if (!Lorkhan || !pZath)
                    return;
                Lorkhan->ToCreature()->DisappearAndDie();
                pZath->ToCreature()->DisappearAndDie();
            }
        
            void JustReachedHome() override
            {
                instance->SetBossState(DATA_THEKAL, FAIL);
            }

            void SpellHitTarget(Unit* /*target*/, const SpellInfo* spellInfo) override
            {
                if (spellInfo->Id == SPELL_RESURRECT)
                    Revive();
            }
        
            // Only call in context where m_pInstance is valid
            bool CanPreventAddsResurrect()
            {
                // If any add is alive, return false
                if (instance->GetBossState(DATA_ZATH) != SPECIAL || instance->GetBossState(DATA_LORKHAN) != SPECIAL)
                    return false;
        
                // Else Prevent them Resurrecting
                if (Creature* pLorkhan = me->FindNearestCreature(NPC_ZEALOT_LORKHAN, 500.0f))
                    pLorkhan->AI()->DoAction(ACTION_PREVENT_REVIVE);
        
                if (Creature* pzath = me->FindNearestCreature(NPC_ZEALOT_ZATH, 500.0f))
                    pzath->AI()->DoAction(ACTION_PREVENT_REVIVE);
        
                return true;
            }
        
            void OnFakeingDeath()
            {
                ResurrectTimer = 10 * IN_MILLISECONDS;
        
                if (instance)
                {
                    instance->SetBossState(DATA_THEKAL, SPECIAL);
        
                    // If both Adds are already dead, don't wait 10 seconds
                    if (CanPreventAddsResurrect())
                        ResurrectTimer = 1 * IN_MILLISECONDS;
                }
            }
        
            void OnRevive()
            {
                if (!instance)
                    return;
        
                // Both Adds are 'dead' enter tiger phase
                if (CanPreventAddsResurrect())
                {
                    DoCastSelf(SPELL_TIGER_FORM, true);
                    Phase = PHASE_TIGER;
                }
            }
        
            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;
        
                switch (Phase)
                {
                    case PHASE_FAKE_DEATH:
                        if (ResurrectTimer < diff)
                        {
                            // resurrect him in any case
                            DoCastSelf(SPELL_RESURRECT);                        
                    
                            Phase = PHASE_WAITING;
                            if (instance)
                            {
                                CanPreventAddsResurrect();
                                instance->SetBossState(DATA_THEKAL, IN_PROGRESS);
                            }
                        }
                        else
                            ResurrectTimer -= diff;
                    
                        // No break needed here
                    case PHASE_WAITING:
                        return;
                    
                    case PHASE_NORMAL:
                        if (MortalCleaveTimer < diff)
                        {
                            DoCastVictim(SPELL_MORTAL_CLEAVE);
                            MortalCleaveTimer = urand(15, 20) * IN_MILLISECONDS;
                        }
                        else
                            MortalCleaveTimer -= diff;
                    
                        if (SilenceTimer < diff)
                        {
                            DoCastVictim(SPELL_SILENCE, true);
                            SilenceTimer = urand(20, 25) * IN_MILLISECONDS;
                        }
                        else
                            SilenceTimer -= diff;
                    
                        break;
                    case PHASE_TIGER:
                        if (ChargeTimer < diff)
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            {
                                DoCast(target, SPELL_CHARGE);
                                ResetThreatList();
                                AttackStart(target);
                            }
                            ChargeTimer = urand(15, 22) * IN_MILLISECONDS;
                        }
                        else
                            ChargeTimer -= diff;
                    
                        if (FrenzyTimer < diff)
                        {
                            DoCastSelf(SPELL_FRENZY, true);
                            FrenzyTimer = 30 * IN_MILLISECONDS;
                        }
                        else
                            FrenzyTimer -= diff;
                    
                        if (ForcePunchTimer < diff)
                        {
                            DoCastSelf(SPELL_FORCE_PUNCH);
                            ForcePunchTimer = urand(16, 21) * IN_MILLISECONDS;
                        }
                        else
                            ForcePunchTimer -= diff;
                    
                        if (SummonTigersTimer < diff)
                        {
                            DoCastSelf(SPELL_SUMMON_TIGERS);
                            SummonTigersTimer = 50 * IN_MILLISECONDS;
                        }
                        else
                            SummonTigersTimer -= diff;
                    
                        if (!Enraged && !HealthAbovePct(11))
                        {
                            DoCastSelf(SPELL_ENRAGE);
                            Enraged = true;
                        }
                    
                        break;
                }
        
                DoMeleeAttackIfReady();
            }

            private:
                InstanceScript* instance;

                uint32 MortalCleaveTimer;
                uint32 SilenceTimer;
                uint32 FrenzyTimer;
                uint32 ForcePunchTimer;
                uint32 ChargeTimer;
                uint32 EnrageTimer;
                uint32 SummonTigersTimer;
                uint32 ResurrectTimer;

                bool Enraged;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_thekalAI>(creature);
        }
};

/*######
## npc_zealot_lorkhan
######*/

class npc_zealot_lorkhan : public CreatureScript
{
    public:
        npc_zealot_lorkhan() : CreatureScript("npc_zealot_lorkhan") { }

        struct npc_zealot_lorkhanAI : public boss_thekalBaseAI
        {
            npc_zealot_lorkhanAI(Creature* creature) : boss_thekalBaseAI(creature)
            {
                instance = creature->GetInstanceScript();
            }
                            
            void Reset() override
            {
                ShieldTimer      = 1 * IN_MILLISECONDS;
                BloodLustTimer   = 16 * IN_MILLISECONDS;
                GreaterHealTimer = 32 * IN_MILLISECONDS;
                DisarmTimer      = 6 * IN_MILLISECONDS;
                Phase            = PHASE_NORMAL;
        
                instance->SetBossState(DATA_LORKHAN, NOT_STARTED);
        
                Revive(true);
            }
        
            void EnterCombat(Unit* /*who*/) override
            {
                instance->SetBossState(DATA_LORKHAN, IN_PROGRESS);
            }
        
            void OnFakeingDeath()
            {
                ResurrectTimer = 10 * IN_MILLISECONDS;
                instance->SetBossState(DATA_LORKHAN, SPECIAL);
            }

            void DoAction(int32 actionId) override
            {
                if (actionId == ACTION_PREVENT_REVIVE)
                {
                    if (me->IsNonMeleeSpellCast(true))
                        me->InterruptNonMeleeSpells(true);

                    Phase = PHASE_WAITING;
                }
            }

            void SpellHitTarget(Unit* /*target*/, const SpellInfo* spellInfo) override
            {
                if (spellInfo->Id == SPELL_RESURRECT)
                    Revive();
            }
        
            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;
        
                switch (Phase)
                {
                    case PHASE_FAKE_DEATH:
                        if (ResurrectTimer < diff)
                        {
                            if (instance->GetBossState(DATA_THEKAL) != SPECIAL || instance->GetBossState(DATA_ZATH) != SPECIAL)
                            {
                                DoCastSelf(SPELL_RESURRECT);
                                instance->SetBossState(DATA_LORKHAN, IN_PROGRESS);
                            }
                    
                            Phase = PHASE_WAITING;
                        }
                        else
                            ResurrectTimer -= diff;
                    
                        // no break needed here
                    case PHASE_WAITING:
                        return;
                    
                    case PHASE_NORMAL:
                        // Shield_Timer
                        if (ShieldTimer < diff)
                        {
                            DoCastSelf(SPELL_SHIELD);
                            ShieldTimer = 61 * IN_MILLISECONDS;
                        }
                        else
                            ShieldTimer -= diff;
                    
                        // BloodLust_Timer
                        if (BloodLustTimer < diff)
                        {
                            // ToDo: research if this should be cast on Thekal or Zath
                            DoCastSelf(SPELL_BLOODLUST);
                            BloodLustTimer = urand(20, 28) * IN_MILLISECONDS;
                        }
                        else
                            BloodLustTimer -= diff;
                    
                        // Casting Greaterheal to Thekal or Zath if they are in meele range.
                        // TODO - why this range check?
                        if (GreaterHealTimer < diff)
                        {
                            if (instance)
                            {
                                Unit* pThekal = ObjectAccessor::GetUnit(*me, ObjectGuid(instance->GetGuidData(DATA_THEKAL)));
                                Unit* pZath = ObjectAccessor::GetUnit(*me, ObjectGuid(instance->GetGuidData(DATA_ZATH)));
                    
                                if (!pThekal || !pZath)
                                    return;
                    
                                switch (urand(0, 1))
                                {
                                    case 0:
                                        if (me->IsWithinMeleeRange(pThekal))
                                            DoCast(pThekal, SPELL_GREATER_HEAL);
                                        break;
                                    case 1:
                                        if (me->IsWithinMeleeRange(pZath))
                                            DoCast(pZath, SPELL_GREATER_HEAL);
                                        break;
                                }
                            }
                    
                            GreaterHealTimer = urand(15, 20) * IN_MILLISECONDS;
                        }
                        else
                            GreaterHealTimer -= diff;
                      
                        // Disarm_Timer
                        if (DisarmTimer < diff)
                        {
                            DoCastVictim(SPELL_DISARM);
                            DisarmTimer = urand(15, 25) * IN_MILLISECONDS;
                        }
                        else
                            DisarmTimer -= diff;
                    
                        break;
                }
        
                DoMeleeAttackIfReady();
            }

            private:
                InstanceScript* instance;

                uint32 ShieldTimer;
                uint32 BloodLustTimer;
                uint32 GreaterHealTimer;
                uint32 DisarmTimer;
                uint32 ResurrectTimer;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<npc_zealot_lorkhanAI>(creature);
        }
};

/*######
## npc_zealot_zath
######*/

class npc_zealot_zath : public CreatureScript
{
    public:
        npc_zealot_zath() : CreatureScript("npc_zealot_zath") { }

        struct npc_zealot_zathAI : public boss_thekalBaseAI
        {
            npc_zealot_zathAI(Creature* creature) : boss_thekalBaseAI(creature)
            {
                instance = creature->GetInstanceScript();
            }
                            
            void Reset() override
            {
                SweepingStrikesTimer = 13 * IN_MILLISECONDS;
                SinisterStrikeTimer  = 8 * IN_MILLISECONDS;
                GougeTimer           = 25 * IN_MILLISECONDS;
                KickTimer            = 18 * IN_MILLISECONDS;
                BlindTimer           = 5 * IN_MILLISECONDS;
                Phase                = PHASE_NORMAL;
        
                instance->SetBossState(DATA_ZATH, NOT_STARTED);
        
                Revive(true);
            }
        
            void EnterCombat(Unit* /*who*/) override
            {
                instance->SetBossState(DATA_ZATH, IN_PROGRESS);
            }
        
            void OnFakeingDeath()
            {
                ResurrectTimer = 10 * IN_MILLISECONDS;
        
                instance->SetBossState(DATA_ZATH, SPECIAL);
            }

            void DoAction(int32 actionId) override
            {
                if (actionId == ACTION_PREVENT_REVIVE)
                {
                    if (me->IsNonMeleeSpellCast(true))
                        me->InterruptNonMeleeSpells(true);

                    Phase = PHASE_WAITING;
                }
            }

            void SpellHitTarget(Unit* /*target*/, const SpellInfo* spellInfo) override
            {
                if (spellInfo->Id == SPELL_RESURRECT)
                    Revive();
            }
        
            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;
        
                switch (Phase)
                {
                    case PHASE_FAKE_DEATH:
                        if (ResurrectTimer < diff)
                        {
                            if (instance->GetBossState(DATA_THEKAL) != SPECIAL || instance->GetBossState(DATA_LORKHAN) != SPECIAL)
                            {
                                DoCastSelf(SPELL_RESURRECT);
                                instance->SetBossState(DATA_ZATH, IN_PROGRESS);
                            }
                    
                            Phase = PHASE_WAITING;
                        }
                        else
                            ResurrectTimer -= diff;
                    
                        // no break needed here
                    case PHASE_WAITING:
                        return;
                    case PHASE_NORMAL:
                        // SweepingStrikes_Timer
                        if (SweepingStrikesTimer < diff)
                        {
                            DoCastVictim(SPELL_SWEEPING_STRIKES);
                            SweepingStrikesTimer = urand(22, 26) * IN_MILLISECONDS;
                        }
                        else
                            SweepingStrikesTimer -= diff;
                    
                        // SinisterStrike_Timer
                        if (SinisterStrikeTimer < diff)
                        {
                            DoCastVictim(SPELL_SINISTER_STRIKE);
                            SinisterStrikeTimer = urand(8, 16) * IN_MILLISECONDS;
                        }
                        else
                            SinisterStrikeTimer -= diff;
                    
                        // Gouge_Timer
                        if (GougeTimer < diff)
                        {
                            DoCastVictim(SPELL_GOUGE);
                    
                            if (GetThreat(me->GetVictim()))
                                ModifyThreatByPercent(me->GetVictim(), -100);
                    
                            GougeTimer = urand(17, 27) * IN_MILLISECONDS;
                        }
                        else
                            GougeTimer -= diff;
                    
                        // Kick_Timer
                        if (KickTimer < diff)
                        {
                            DoCastVictim(SPELL_KICK);
                            KickTimer = urand(15, 25) * IN_MILLISECONDS;
                        }
                        else
                            KickTimer -= diff;
                    
                        // Blind_Timer
                        if (BlindTimer < diff)
                        {
                            DoCastVictim(SPELL_BLIND);
                            BlindTimer = urand(10, 20) * IN_MILLISECONDS;
                        }
                        else
                            BlindTimer -= diff;
                    
                        break;
                }
        
                DoMeleeAttackIfReady();
            }

            private:
                uint32 SweepingStrikesTimer;
                uint32 SinisterStrikeTimer;
                uint32 GougeTimer;
                uint32 KickTimer;
                uint32 BlindTimer;
                uint32 ResurrectTimer;

                InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<npc_zealot_zathAI>(creature);
        }
};

void AddSC_boss_thekal()
{
    new boss_thekal();
    new npc_zealot_lorkhan();
    new npc_zealot_zath();
}
