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
SDName: Boss_Sartura
SD%Complete: 95
SDComment:
SDCategory: Temple of Ahn'Qiraj
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "temple_of_ahnqiraj.h"

enum Sartura
{
    SAY_AGGRO           = 0,
    SAY_SLAY            = 1,
    SAY_DEATH           = 2,

    SPELL_WHIRLWIND     = 26083,
    SPELL_ENRAGE        = 28747,            //Not sure if right ID.
    SPELL_ENRAGEHARD    = 28798,

//Guard Spell
    SPELL_WHIRLWINDADD  = 26038,
    SPELL_KNOCKBACK     = 26027
};

class boss_sartura : public CreatureScript
{
    public:
        boss_sartura() : CreatureScript("boss_sartura") { }
    
        struct boss_sarturaAI : public ScriptedAI
        {
            boss_sarturaAI(Creature* creature) : ScriptedAI(creature) { }
        
            void Reset() override
            {                
                WhirlWind_Timer = 30 * IN_MILLISECONDS;
                WhirlWindRandom_Timer = urand(3, 7) * IN_MILLISECONDS;
                WhirlWindEnd_Timer = 15 * IN_MILLISECONDS;
                AggroReset_Timer = urand(45, 55) * IN_MILLISECONDS;
                AggroResetEnd_Timer = 5 * IN_MILLISECONDS;
                EnrageHard_Timer = 600 * IN_MILLISECONDS;

                WhirlWind = false;
                AggroReset = false;
                Enraged = false;
                EnragedHard = false;
            }

            void EnterCombat(Unit* /*who*/) override
            {
                Talk(SAY_AGGRO);
            }

            void JustDied(Unit* /*killer*/) override
            {
                Talk(SAY_DEATH);
            }

            void KilledUnit(Unit* /*victim*/) override
            {
                Talk(SAY_SLAY);
            }

            void UpdateAI(uint32 diff) override
            {
                //Return since we have no target
                if (!UpdateVictim())
                    return;

                if (WhirlWind)
                {
                    if (WhirlWindRandom_Timer <= diff)
                    {
                        //Attack random Gamers
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 100.0f, true))
                        {
                            AddThreat(target, 1.0f);
                            me->TauntApply(target);
                            AttackStart(target);
                        }
                        WhirlWindRandom_Timer = urand(3, 7) * IN_MILLISECONDS;
                    } 
                    else 
                        WhirlWindRandom_Timer -= diff;

                    if (WhirlWindEnd_Timer <= diff)
                    {
                        WhirlWind = false;
                        WhirlWind_Timer = urand(25, 40) * IN_MILLISECONDS;
                    } 
                    else 
                        WhirlWindEnd_Timer -= diff;
                }

                if (!WhirlWind)
                {
                    if (WhirlWind_Timer <= diff)
                    {
                        DoCastSelf(SPELL_WHIRLWIND);
                        WhirlWind = true;
                        WhirlWindEnd_Timer = 15 * IN_MILLISECONDS;
                    } 
                    else 
                        WhirlWind_Timer -= diff;

                    if (AggroReset_Timer <= diff)
                    {
                        //Attack random Gamers
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 100.0f, true))
                        {
                            AddThreat(target, 1.0f);
                            me->TauntApply(target);
                            AttackStart(target);
                        }
                        AggroReset = true;
                        AggroReset_Timer = urand(2, 5) * IN_MILLISECONDS;
                    } 
                    else 
                        AggroReset_Timer -= diff;

                    if (AggroReset)
                    {
                        if (AggroResetEnd_Timer <= diff)
                        {
                            AggroReset = false;
                            AggroResetEnd_Timer = 5 * IN_MILLISECONDS;
                            AggroReset_Timer = urand(35, 45) * IN_MILLISECONDS;
                        } 
                        else 
                            AggroResetEnd_Timer -= diff;
                    }

                    //If she is 20% enrage
                    if (!Enraged)
                    {
                        if (!HealthAbovePct(20) && !me->IsNonMeleeSpellCast(false))
                        {
                            DoCastSelf(SPELL_ENRAGE);
                            Enraged = true;
                        }
                    }

                    //After 10 minutes hard enrage
                    if (!EnragedHard)
                    {
                        if (EnrageHard_Timer <= diff)
                        {
                            DoCastSelf(SPELL_ENRAGEHARD);
                            EnragedHard = true;
                        } 
                        else 
                            EnrageHard_Timer -= diff;
                    }

                    DoMeleeAttackIfReady();
                }
            }

        private:
            uint32 WhirlWind_Timer;
            uint32 WhirlWindRandom_Timer;
            uint32 WhirlWindEnd_Timer;
            uint32 AggroReset_Timer;
            uint32 AggroResetEnd_Timer;
            uint32 EnrageHard_Timer;

            bool Enraged;
            bool EnragedHard;
            bool WhirlWind;
            bool AggroReset;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_sarturaAI(creature);
    }
};

class npc_sartura_royal_guard : public CreatureScript
{
    public:
        npc_sartura_royal_guard() : CreatureScript("npc_sartura_royal_guard") { }
    
        struct npc_sartura_royal_guardAI : public ScriptedAI
        {
            npc_sartura_royal_guardAI(Creature* creature) : ScriptedAI(creature) { }
        
            void Reset() override
            {
                WhirlWind_Timer       =            30 * IN_MILLISECONDS;
                WhirlWindRandom_Timer =   urand(3, 7) * IN_MILLISECONDS;
                WhirlWindEnd_Timer    =            15 * IN_MILLISECONDS;
                AggroReset_Timer      = urand(45, 55) * IN_MILLISECONDS;
                AggroResetEnd_Timer   =             5 * IN_MILLISECONDS;
                KnockBack_Timer       =            10 * IN_MILLISECONDS;

                WhirlWind = false;
                AggroReset = false;
            }

            void EnterCombat(Unit* /*who*/) override { }

            void UpdateAI(uint32 diff) override
            {
                //Return since we have no target
                if (!UpdateVictim())
                    return;

                if (!WhirlWind && WhirlWind_Timer <= diff)
                {
                    DoCastSelf(SPELL_WHIRLWINDADD);
                    WhirlWind = true;
                    WhirlWind_Timer = urand(25, 40) * IN_MILLISECONDS;
                    WhirlWindEnd_Timer = 15 * IN_MILLISECONDS;
                } 
                else 
                    WhirlWind_Timer -= diff;

                if (WhirlWind)
                {
                    if (WhirlWindRandom_Timer <= diff)
                    {
                        //Attack random Gamers
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 100.0f, true))
                        {
                            AddThreat(target, 1.0f);
                            me->TauntApply(target);
                            AttackStart(target);
                        }
                        WhirlWindRandom_Timer = urand(3, 7) * IN_MILLISECONDS;
                    } 
                    else 
                        WhirlWindRandom_Timer -= diff;

                    if (WhirlWindEnd_Timer <= diff)
                    {
                        WhirlWind = false;
                    } 
                    else 
                        WhirlWindEnd_Timer -= diff;
                }

                if (!WhirlWind)
                {
                    if (AggroReset_Timer <= diff)
                    {
                        //Attack random Gamers
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 100.0f, true))
                        {
                            AddThreat(target, 1.0f);
                            me->TauntApply(target);
                            AttackStart(target);
                        }
                        AggroReset = true;
                        AggroReset_Timer = urand(2, 5) * IN_MILLISECONDS;
                    } 
                    else 
                        AggroReset_Timer -= diff;

                    if (KnockBack_Timer <= diff)
                    {
                        DoCastSelf(SPELL_WHIRLWINDADD);
                        KnockBack_Timer = urand(10, 20) * IN_MILLISECONDS;
                    } 
                    else 
                        KnockBack_Timer -= diff;
                }

                if (AggroReset)
                {
                    if (AggroResetEnd_Timer <= diff)
                    {
                        AggroReset = false;
                        AggroResetEnd_Timer = 5 * IN_MILLISECONDS;
                        AggroReset_Timer = urand(30, 40) * IN_MILLISECONDS;
                    } 
                    else 
                        AggroResetEnd_Timer -= diff;
                }

                DoMeleeAttackIfReady();
            }

        private:
            uint32 WhirlWind_Timer;
            uint32 WhirlWindRandom_Timer;
            uint32 WhirlWindEnd_Timer;
            uint32 AggroReset_Timer;
            uint32 AggroResetEnd_Timer;
            uint32 KnockBack_Timer;

            bool WhirlWind;
            bool AggroReset;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_sartura_royal_guardAI(creature);
    }
};

class AreaTrigger_at_aq_sartura : public AreaTriggerScript
{
    public:
        AreaTrigger_at_aq_sartura() : AreaTriggerScript("at_aq_sartura") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) override
    {
        if (player->IsGameMaster() || !player->IsAlive())
            return false;

        if (Creature* sartura = player->FindNearestCreature(NPC_SARTURA, 400.0f))
        {
            sartura->AI()->AttackStart(player);
            sartura->SetInCombatWithZone();
        }           

        return false;
    }
};

void AddSC_boss_sartura()
{
    new boss_sartura();
    new npc_sartura_royal_guard();
    new AreaTrigger_at_aq_sartura();
}
