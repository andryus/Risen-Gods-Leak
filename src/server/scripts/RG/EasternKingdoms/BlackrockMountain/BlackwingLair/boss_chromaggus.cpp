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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "PlayerAI.h"
#include "blackwing_lair.h"

enum Emotes
{
    EMOTE_FRENZY                                           = 0,
    EMOTE_SHIMMER                                          = 1,
};

enum Spells
{
    SPELL_FIRE_VULNERABILITY    = 22277,
    SPELL_FROST_VULNERABILITY   = 22278,
    SPELL_SHADOW_VULNERABILITY  = 22279,
    SPELL_NATURE_VULNERABILITY  = 22280,
    SPELL_ARCANE_VULNERABILITY  = 22281,
    // Breaths
    SPELL_INCINERATE            = 23308, // Incinerate 23308, 23309
    SPELL_TIMELAPSE             = 23310, // Time lapse 23310, 23311(old threat mod that was removed in 2.01)
    SPELL_CORROSIVEACID         = 23313, // Corrosive Acid 23313, 23314
    SPELL_IGNITEFLESH           = 23315, // Ignite Flesh 23315, 23316
    SPELL_FROSTBURN             = 23187, // Frost burn 23187, 23189
    //SPELL_BROODAF             = 23173, // not in client dbc. ToDo: Needs SpellScript
    SPELL_BROODAF_BLUE          = 23153,
    SPELL_BROODAF_BLACK         = 23154,
    SPELL_BROODAF_RED           = 23155,
    SPELL_BROODAF_BRONZE        = 23170,
    SPELL_BROODAF_GREEN         = 23169,
    SPELL_CHROMATIC_MUT         = 23174, // Spell cast on player if they get all 5 debuffs
    SPELL_FRENZY                = 23128,
    SPELL_ENRAGE                = 23537
};

enum Events
{
    EVENT_SHIMMER       = 1,
    EVENT_BREATH_1      = 2,
    EVENT_BREATH_2      = 3,
    EVENT_AFFLICTION    = 4,
    EVENT_FRENZY        = 5
};

class ChromaggusCharmedPlayerAI : public SimpleCharmedPlayerAI
{
public:
    ChromaggusCharmedPlayerAI(Player& player) : SimpleCharmedPlayerAI(player) { }

    void UpdateAI(uint32 diff) override
    {
        SimpleCharmedPlayerAI::UpdateAINoSpells(diff);
    }
};

class boss_chromaggus : public CreatureScript
{
    public:
        boss_chromaggus() : CreatureScript("boss_chromaggus") { }
    
        struct boss_chromaggusAI : public BossAI
        {
            boss_chromaggusAI(Creature* creature) : BossAI(creature, BOSS_CHROMAGGUS)
            {
                // Select the 2 breaths that we are going to use until despawned
                Breath1Spell = RAND(SPELL_INCINERATE, SPELL_TIMELAPSE, SPELL_CORROSIVEACID, SPELL_IGNITEFLESH, SPELL_FROSTBURN);
                do
                    Breath2Spell = RAND(SPELL_INCINERATE, SPELL_TIMELAPSE, SPELL_CORROSIVEACID, SPELL_IGNITEFLESH, SPELL_FROSTBURN);
                while (Breath1Spell == Breath2Spell);
            }
    
            void Reset() override
            {
                BossAI::Reset();
                Enraged = false;
                shimmerSpellId = 0;
                for (Player& player : me->GetMap()->GetPlayers())
                    if (player.HasAura(SPELL_CHROMATIC_MUT))
                        me->Kill(&player);
            }
    
            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
    
                events.ScheduleEvent(EVENT_SHIMMER, IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_BREATH_1, 30 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_BREATH_2, 60 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_AFFLICTION, 10 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FRENZY, urand(12, 15) * IN_MILLISECONDS);
            }

            void JustDied(Unit* killer) override
            {
                BossAI::JustDied(killer);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BROODAF_BLUE);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BROODAF_BLACK);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BROODAF_RED);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BROODAF_BRONZE);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BROODAF_GREEN);
            }
    
            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;
    
                events.Update(diff);
    
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
    
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SHIMMER:
                            if (shimmerSpellId)
                                me->RemoveAura(shimmerSpellId);
                            shimmerSpellId = RAND(SPELL_FIRE_VULNERABILITY, SPELL_FROST_VULNERABILITY, SPELL_SHADOW_VULNERABILITY, SPELL_NATURE_VULNERABILITY, SPELL_ARCANE_VULNERABILITY);
                            DoCastSelf(shimmerSpellId);
                            Talk(EMOTE_SHIMMER);
                            events.ScheduleEvent(EVENT_SHIMMER, 45 * IN_MILLISECONDS);
                            break;
                        case EVENT_BREATH_1:
                            DoCastVictim(Breath1Spell);
                            events.ScheduleEvent(EVENT_BREATH_1, 60 * IN_MILLISECONDS);
                            break;
                        case EVENT_BREATH_2:
                            DoCastVictim(Breath2Spell);
                            events.ScheduleEvent(EVENT_BREATH_2, 60 * IN_MILLISECONDS);
                            break;
                        case EVENT_AFFLICTION:
                            {
                                bool hasChromaticMutation = true;
                                for (Player& player : me->GetMap()->GetPlayers())
                                {
                                    DoCast(&player, RAND(SPELL_BROODAF_BLUE, SPELL_BROODAF_BLACK, SPELL_BROODAF_RED, SPELL_BROODAF_BRONZE, SPELL_BROODAF_GREEN), true);
                                    if (player.HasAura(SPELL_BROODAF_BLUE) && player.HasAura(SPELL_BROODAF_BLACK) &&
                                        player.HasAura(SPELL_BROODAF_RED) && player.HasAura(SPELL_BROODAF_BRONZE) &&
                                        player.HasAura(SPELL_BROODAF_GREEN))
                                    {
                                        //DoCast(player, SPELL_CHROMATIC_MUT);
                                        me->AddAura(SPELL_CHROMATIC_MUT, &player);
                                        player.RemoveAurasDueToSpell(SPELL_BROODAF_BLUE);
                                        player.RemoveAurasDueToSpell(SPELL_BROODAF_BLACK);
                                        player.RemoveAurasDueToSpell(SPELL_BROODAF_RED);
                                        player.RemoveAurasDueToSpell(SPELL_BROODAF_BRONZE);
                                        player.RemoveAurasDueToSpell(SPELL_BROODAF_GREEN);
                                    }

                                    if (!player.HasAura(SPELL_CHROMATIC_MUT))
                                        hasChromaticMutation = false;
                                }
                                if (hasChromaticMutation)
                                    for (Player& player : me->GetMap()->GetPlayers())
                                        me->Kill(&player);
                            }
                            events.ScheduleEvent(EVENT_AFFLICTION, 10 * IN_MILLISECONDS);
                            break;
                        case EVENT_FRENZY:
                            DoCastSelf(SPELL_FRENZY);
                            events.ScheduleEvent(EVENT_FRENZY, urand(12, 15) * IN_MILLISECONDS);
                            break;
                    }
                }

                if (!Enraged && HealthBelowPct(20))
                {
                    DoCastSelf(SPELL_ENRAGE);
                    Enraged = true;
                }
    
                DoMeleeAttackIfReady();
            }

            PlayerAI* GetAIForCharmedPlayer(Player* player) override
            {
                return new ChromaggusCharmedPlayerAI(*player);
            }
    
        private:
            uint32 Breath1Spell;
            uint32 Breath2Spell;
            uint32 shimmerSpellId;
            bool Enraged;
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_chromaggusAI (creature);
        }
};

void AddSC_boss_chromaggus()
{
    new boss_chromaggus();
}
