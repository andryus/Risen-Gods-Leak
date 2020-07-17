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

enum Emotes
{
    EMOTE_FRENZY                 = 0
};

enum Spells
{
    GO_GAMBIT                    = 176110,
    NPC_STUDENT                  = 10475,
    SPELL_TRANSFORM              = 18115,
    YELL_WONDER                  = 1,
    SPELL_FLAMESTRIKE            = 18399,
    SPELL_BLAST_WAVE             = 16046,
    SPELL_FIRE_SHIELD            = 19626,
    SPELL_FRENZY                 = 8269,  // 28371
    NPC_MARDUK                   = 10433

};

enum Events
{
    EVENT_FIRE_SHIELD = 1,
    EVENT_BLAST_WAVE,
    EVENT_FRENZY
};

class boss_vectus : public CreatureScript
{
public:
    boss_vectus() : CreatureScript("boss_vectus") { }

    struct boss_vectusAI : public ScriptedAI
    {
        boss_vectusAI(Creature* creature) : ScriptedAI(creature) { }
        
        void Reset() override
        {
            events.Reset();
            m_uiFind_Gambit = 2 * IN_MILLISECONDS;
            FoundGambit = false;
        }

        void EnterCombat(Unit* /*who*/) override
        {
            me->CallForHelp(70.0f);
            if (Creature* marduk = me->FindNearestCreature(NPC_MARDUK, 75.0f, true))
                marduk->AI()->SetData(1, 1);
            events.ScheduleEvent(EVENT_FIRE_SHIELD, 2000);
            events.ScheduleEvent(EVENT_BLAST_WAVE, 14000);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage) override
        {
            if (me->HealthBelowPctDamaged(25, damage))
            {
                DoCastSelf(SPELL_FRENZY);
                Talk(EMOTE_FRENZY);
                events.ScheduleEvent(EVENT_FRENZY, 24000);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            //Out of combat timers
            if (!me->GetVictim())
            {
                //FindGambit
                if (m_uiFind_Gambit <= diff && !FoundGambit)
                {
                    if (GameObject* gambit = me->FindNearestGameObject(GO_GAMBIT, 200.0f))
                    { 
                        Talk(YELL_WONDER);
                        me->setFaction(FACTION_HOSTILE);
                        std::list<Creature*> StudentList;
                        me->GetCreatureListWithEntryInGrid(StudentList, NPC_STUDENT, 150.0f);
                        if (!StudentList.empty())
                            for (std::list<Creature*>::iterator itr = StudentList.begin(); itr != StudentList.end(); itr++)
                                (*itr)->AI()->DoCast(SPELL_TRANSFORM);
                        FoundGambit = true;
                     }
                     m_uiFind_Gambit = 2*IN_MILLISECONDS;
                }
                else m_uiFind_Gambit -= diff;
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
                    case EVENT_FIRE_SHIELD:
                        DoCastSelf(SPELL_FIRE_SHIELD);
                        events.ScheduleEvent(EVENT_FIRE_SHIELD, 90000);
                        break;
                    case EVENT_BLAST_WAVE:
                        DoCastSelf(SPELL_BLAST_WAVE);
                        events.ScheduleEvent(EVENT_BLAST_WAVE, 12000);
                    case EVENT_FRENZY:
                        DoCastSelf(SPELL_FRENZY);
                        Talk(EMOTE_FRENZY);
                        events.ScheduleEvent(EVENT_FRENZY, 24000);
                        break;
                    default:
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }

            DoMeleeAttackIfReady();
        }
        
        private:
            EventMap events;
            uint32 m_uiFind_Gambit;
            bool FoundGambit;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_vectusAI(creature);
    }
};

void AddSC_boss_vectus()
{
    new boss_vectus();
}
