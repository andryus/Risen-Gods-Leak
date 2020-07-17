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
SDName: Boss_Huhuran
SD%Complete: 100
SDComment:
SDCategory: Temple of Ahn'Qiraj
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

enum Huhuran
{
    EMOTE_FRENZY_KILL           = 0,
    EMOTE_BERSERK               = 1,

    SPELL_FRENZY                = 26051,
    SPELL_BERSERK               = 26068,
    SPELL_POISONBOLT            = 26052,
    SPELL_NOXIOUSPOISON         = 26053,
    SPELL_WYVERNSTING           = 26180,
    SPELL_ACIDSPIT              = 26050
};

class boss_huhuran : public CreatureScript
{
    public:
        boss_huhuran() : CreatureScript("boss_huhuran") { }
    
        struct boss_huhuranAI : public ScriptedAI
        {
            boss_huhuranAI(Creature* creature) : ScriptedAI(creature) { }
        
            void Reset() override
            {
                Frenzy_Timer = urand(25, 35) * IN_MILLISECONDS;
                Wyvern_Timer = urand(18, 28) * IN_MILLISECONDS;
                Spit_Timer = 8 * IN_MILLISECONDS;
                PoisonBolt_Timer = 4 * IN_MILLISECONDS;
                NoxiousPoison_Timer = urand(10, 20) * IN_MILLISECONDS;
                FrenzyBack_Timer = 15 * IN_MILLISECONDS;

                Frenzy = false;
                Berserk = false;
            }

            void EnterCombat(Unit* /*who*/) override { }

            void UpdateAI(uint32 diff) override
            {
                //Return since we have no target
                if (!UpdateVictim())
                    return;

                //Frenzy_Timer
                if (!Frenzy && Frenzy_Timer <= diff)
                {
                    DoCastSelf(SPELL_FRENZY);
                    Talk(EMOTE_FRENZY_KILL);
                    Frenzy = true;
                    PoisonBolt_Timer = 3 * IN_MILLISECONDS;
                    Frenzy_Timer = urand(25, 35) * IN_MILLISECONDS;
                } 
                else 
                    Frenzy_Timer -= diff;

                // Wyvern Timer
                if (Wyvern_Timer <= diff)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        DoCast(target, SPELL_WYVERNSTING);
                    Wyvern_Timer = urand(15, 32) * IN_MILLISECONDS;
                } 
                else 
                    Wyvern_Timer -= diff;

                //Spit Timer
                if (Spit_Timer <= diff)
                {
                    DoCastVictim(SPELL_ACIDSPIT);
                    Spit_Timer = urand(5, 10) * IN_MILLISECONDS;
                } 
                else 
                    Spit_Timer -= diff;

                //NoxiousPoison_Timer
                if (NoxiousPoison_Timer <= diff)
                {
                    DoCastVictim(SPELL_NOXIOUSPOISON);
                    NoxiousPoison_Timer = urand(12, 24) * IN_MILLISECONDS;
                } 
                else 
                    NoxiousPoison_Timer -= diff;

                //PoisonBolt only if frenzy or berserk
                if (Frenzy || Berserk)
                {
                    if (PoisonBolt_Timer <= diff)
                    {
                        DoCastVictim(SPELL_POISONBOLT);
                        PoisonBolt_Timer = 3 * IN_MILLISECONDS;
                    } 
                    else 
                        PoisonBolt_Timer -= diff;
                }

                //FrenzyBack_Timer
                if (Frenzy && FrenzyBack_Timer <= diff)
                {
                    me->InterruptNonMeleeSpells(false);
                    Frenzy = false;
                    FrenzyBack_Timer = 15 * IN_MILLISECONDS;
                } 
                else 
                    FrenzyBack_Timer -= diff;

                if (!Berserk && HealthBelowPct(31))
                {
                    me->InterruptNonMeleeSpells(false);
                    Talk(EMOTE_BERSERK);
                    DoCastSelf(SPELL_BERSERK);
                    Berserk = true;
                }

                DoMeleeAttackIfReady();
            }

        private:
            uint32 Frenzy_Timer;
            uint32 Wyvern_Timer;
            uint32 Spit_Timer;
            uint32 PoisonBolt_Timer;
            uint32 NoxiousPoison_Timer;
            uint32 FrenzyBack_Timer;

            bool Frenzy;
            bool Berserk;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_huhuranAI(creature);
    }
};

void AddSC_boss_huhuran()
{
    new boss_huhuran();
}
