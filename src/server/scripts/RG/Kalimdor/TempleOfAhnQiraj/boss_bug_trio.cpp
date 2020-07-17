/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

/* ScriptData
SDName: boss_kri, boss_yauj, boss_vem : The Bug Trio
SD%Complete: 100
SDComment:
SDCategory: Temple of Ahn'Qiraj
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "temple_of_ahnqiraj.h"

enum Spells
{
    SPELL_CLEAVE       = 26350,
    SPELL_TOXIC_VOLLEY = 25812,
    SPELL_POISON_CLOUD = 38718, //Only Spell with right dmg.
    SPELL_ENRAGE       = 34624, //Changed cause 25790 is cast on gamers too. Same prob with old explosion of twin emperors.

    SPELL_CHARGE       = 26561,
    SPELL_KNOCKBACK    = 26027,

    SPELL_HEAL         = 25807,
    SPELL_FEAR         = 19408
};

//void RespawnEncounter(InstanceScript* instance, Creature* me)
//{
//    for (uint8 i = 0; i < 3; ++i)
//        if (Creature* boss = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_BUG_KRI + i)))
//            if (!boss->IsAlive())
//            {
//                boss->Respawn();
//                boss->GetMotionMaster()->MoveTargetedHome();
//            }
//}

class boss_kri : public CreatureScript
{
    public:
        boss_kri() : CreatureScript("boss_kri") { }

        struct boss_kriAI : public BossAI
        {
            boss_kriAI(Creature* creature) : BossAI(creature, BOSS_BUG_KRI)
            {
                instance = creature->GetInstanceScript();
            }
        
            void Reset() override
            {
                BossAI::Reset();

                Cleave_Timer      = urand(4, 8) * IN_MILLISECONDS;
                ToxicVolley_Timer = urand(6, 12) * IN_MILLISECONDS;

                HasSpellDeath = false;
            }


            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
                me->CallForHelp(20.0f);
            }

            void JustDied(Unit* killer) override
            {
                BossAI::JustDied(killer);

                if (instance->GetData(DATA_BUG_TRIO_DEATH) < 2)
                    // Unlootable if death
                    me->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);

                instance->SetData(DATA_BUG_TRIO_DEATH, 1);
            }


            void UpdateAI(uint32 diff) override
            {
                //Return since we have no target
                if (!UpdateVictim())
                    return;

                //Cleave_Timer
                if (Cleave_Timer <= diff)
                {
                    DoCastVictim(SPELL_CLEAVE);
                    Cleave_Timer = urand(5, 12) * IN_MILLISECONDS;
                } 
                else 
                    Cleave_Timer -= diff;

                //ToxicVolley_Timer
                if (ToxicVolley_Timer <= diff)
                {
                    DoCastVictim(SPELL_TOXIC_VOLLEY);
                    ToxicVolley_Timer = urand(10, 15) * IN_MILLISECONDS;
                } 
                else 
                    ToxicVolley_Timer -= diff;

                if (!HealthAbovePct(5) && !HasSpellDeath)
                {
                    DoCastVictim(SPELL_POISON_CLOUD);
                    HasSpellDeath = true;
                }

                DoMeleeAttackIfReady();
            }

        private:
            InstanceScript* instance;
            uint32 Cleave_Timer;
            uint32 ToxicVolley_Timer;
            bool HasSpellDeath;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_kriAI>(creature);
        }
};

class boss_vem : public CreatureScript
{
    public:
        boss_vem() : CreatureScript("boss_vem") { }

        struct boss_vemAI : public BossAI
        {
            boss_vemAI(Creature* creature) : BossAI(creature, BOSS_BUG_VEM)
            {
                instance = creature->GetInstanceScript();
            }
            
            void Reset() override
            {
                BossAI::Reset();

                Charge_Timer = urand(15, 27) * IN_MILLISECONDS;
                KnockBack_Timer = urand(8, 20) * IN_MILLISECONDS;
                Enrage_Timer = 120 * IN_MILLISECONDS;

                Enraged = false;

            }

            void JustDied(Unit* killer) override
            {
                BossAI::JustDied(killer);

                instance->SetData(DATA_VEM_DEATH, 0);
                if (instance->GetData(DATA_BUG_TRIO_DEATH) < 2)// Unlootable if death
                    me->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                instance->SetData(DATA_BUG_TRIO_DEATH, 1);
            }

            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
                me->CallForHelp(20.0f);
            }

            void UpdateAI(uint32 diff) override
            {
                //Return since we have no target
                if (!UpdateVictim())
                    return;

                //Charge_Timer
                if (Charge_Timer <= diff)
                {
                    Unit* target = NULL;
                    target = SelectTarget(SELECT_TARGET_RANDOM, 0);
                    if (target)
                    {
                        DoCast(target, SPELL_CHARGE);
                        //me->SendMonsterMove(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0, true, 1);
                        AttackStart(target);
                    }
                    Charge_Timer = urand(8, 16) * IN_MILLISECONDS;
                } 
                else 
                    Charge_Timer -= diff;

                //KnockBack_Timer
                if (KnockBack_Timer <= diff)
                {
                    DoCastVictim(SPELL_KNOCKBACK);
                    if (GetThreat(me->GetVictim()))
                        ModifyThreatByPercent(me->GetVictim(), -80);
                    KnockBack_Timer = urand(15, 25) * IN_MILLISECONDS;
                } 
                else 
                    KnockBack_Timer -= diff;

                //Enrage_Timer
                if (!Enraged && Enrage_Timer <= diff)
                {
                    DoCastSelf(SPELL_ENRAGE);
                    Enraged = true;
                } 
                else 
                    Charge_Timer -= diff;

                DoMeleeAttackIfReady();
            }

        private:
            InstanceScript* instance;
            uint32 Charge_Timer;
            uint32 KnockBack_Timer;
            uint32 Enrage_Timer;
            bool Enraged;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_vemAI>(creature);
        }
};

class boss_yauj : public CreatureScript
{
    public:
        boss_yauj() : CreatureScript("boss_yauj") { }

        struct boss_yaujAI : public BossAI
        {
            boss_yaujAI(Creature* creature) : BossAI(creature, BOSS_BUG_YAUJ)
            {
                instance = creature->GetInstanceScript();
            }
        
            void Reset() override
            {
                BossAI::Reset();

                Heal_Timer = urand(25, 40) * IN_MILLISECONDS;
                Fear_Timer = urand(12, 24) * IN_MILLISECONDS;
            }

            void JustDied(Unit* killer) override
            {
                BossAI::JustDied(killer);

                if (instance)
                {
                    if (instance->GetData(DATA_BUG_TRIO_DEATH) < 2)
                        // Unlootable if death
                        me->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                    instance->SetData(DATA_BUG_TRIO_DEATH, 1);
                }

                for (uint8 i = 0; i < 10; ++i)
                    me->SummonCreature(15621, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 90000);
            }

            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
                me->CallForHelp(20.0f);
            }

            void UpdateAI(uint32 diff) override
            {
                //Return since we have no target
                if (!UpdateVictim())
                    return;

                //Fear_Timer
                if (Fear_Timer <= diff)
                {
                    DoCastVictim(SPELL_FEAR);
                    ResetThreatList();
                    Fear_Timer = 20 * IN_MILLISECONDS;
                } 
                else 
                    Fear_Timer -= diff;

                //Casting Heal to other twins or herself.
                if (Heal_Timer <= diff)
                {
                    switch (urand(0, 2))
                    {
                        case 0:
                            if (Creature* kri = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_BUG_KRI))))
                                DoCast(kri, SPELL_HEAL);
                            break;
                        case 1:
                            if (Creature* vem = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_BUG_VEM))))
                                DoCast(vem, SPELL_HEAL);
                            break;
                        case 2:
                            DoCastSelf(SPELL_HEAL);
                            break;
                    }
                    
                    Heal_Timer = urand(15, 30) * IN_MILLISECONDS;
                } 
                else 
                    Heal_Timer -= diff;

                DoMeleeAttackIfReady();
            }

        private:
            InstanceScript* instance;
            uint32 Heal_Timer;
            uint32 Fear_Timer;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_yaujAI>(creature);
        }
};

void AddSC_bug_trio()
{
    new boss_kri();
    new boss_vem();
    new boss_yauj();
}
