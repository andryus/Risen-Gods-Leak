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

/*
 * Comment: Missing AI for Twisted Visages
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ahnkahet.h"
#include "Player.h"
#include "SpellInfo.h"

enum Spells
{
    SPELL_INSANITY                                = 57496, //Dummy
    INSANITY_VISUAL                               = 57561,
    SPELL_INSANITY_TARGET                         = 57508,
    SPELL_MIND_FLAY                               = 57941,
    SPELL_SHADOW_BOLT_VOLLEY                      = 57942,
    SPELL_SHIVER                                  = 57949,
    SPELL_CLONE_PLAYER                            = 57507, //casted on player during insanity
    SPELL_INSANITY_PHASING_1                      = 57508,
    SPELL_INSANITY_PHASING_2                      = 57509,
    SPELL_INSANITY_PHASING_3                      = 57510,
    SPELL_INSANITY_PHASING_4                      = 57511,
    SPELL_INSANITY_PHASING_5                      = 57512
};

enum ClassSpellsNormal
{
    SPELL_WL_CORRUPTION                         = 57645,
    SPELL_WL_SHADOW_BOLT                        = 57644, 

    SPELL_SH_EARTH_SHOCK                        = 57783, 
    SPELL_SH_LIGHTNING_BOLT                     = 57781,

    SPELL_MG_FIREBALL                           = 57628, 
    SPELL_MG_FROSTNOVA                          = 57629,

    SPELL_RG_HEMORRHAGE                         = 45897,
    SPELL_RG_EVISCERATE                         = 46189, 

    SPELL_PL_HOLY_SHOCK                         = 32771, 
    SPELL_PL_HAMMER_OF_JUSTICE                  = 37369, 

    SPELL_PR_HOLY_SMITE                         = 57778, 
    SPELL_PR_RENEW                              = 57777, 

    SPELL_HU_SHOOT                              = 16496, 
    SPELL_HU_MULTI_SHOT                         = 66081, 
    SPELL_HU_WING_CLIP                          = 40652,

    SPELL_WR_WHIRLWIND                          = 17207, 
    SPELL_WR_SURGE                              = 74399, 

    SPELL_DR_MOONFIRE                           = 57647, 
    SPELL_DR_WRATH                              = 57648, 

    SPELL_DK_PLAGU_STRIKE                       = 57599, 
    SPELL_DK_BLOOD_STRIKE                       = 57601, 
};

enum ClassSpellsHeroic
{
    SPELL_WL_CORRUPTION_H                       = 60016,
    SPELL_WL_SHADOW_BOLT_H                      = 60015,

    SPELL_SH_EARTH_SHOCK_H                      = 24685,
    SPELL_SH_LIGHTNING_BOLT_H                   = 69567,

    SPELL_MG_FIREBALL_H                         = 47074,
    SPELL_MG_FROSTNOVA_H                        = 57629,

    SPELL_RG_HEMORRHAGE_H                       = 45897,
    SPELL_RG_EVISCERATE_H                       = 46189,

    SPELL_PL_HOLY_SHOCK_H                       = 38921,
    SPELL_PL_HAMMER_OF_JUSTICE_H                = 37369,

    SPELL_PR_HOLY_SMITE_H                       = 60005,
    SPELL_PR_RENEW_H                            = 47079,

    SPELL_HU_SHOOT_H                            = 16496,
    SPELL_HU_MULTI_SHOT_H                       = 48098,
    SPELL_HU_WING_CLIP_H                        = 40652,

    SPELL_WR_WHIRLWIND_H                        = 17207,
    SPELL_WR_SURGE_H                            = 74399,

    SPELL_DR_MOONFIRE_H                         = 47072,
    SPELL_DR_WRATH_H                            = 31784,

    SPELL_DK_PLAGU_STRIKE_H                     = 58843,
    SPELL_DK_BLOOD_STRIKE_H                     = 61696
};

// NON - Heroic

uint32 ClassSpellList[MAX_CLASSES][2] =
{
    {0, 0},
    {SPELL_WR_WHIRLWIND,    SPELL_WR_SURGE             },    // CLASS_WARRIOR
    {SPELL_PL_HOLY_SHOCK,   SPELL_PL_HAMMER_OF_JUSTICE },    // CLASS_PALADIN
    {SPELL_HU_SHOOT,        SPELL_HU_MULTI_SHOT        },    // CLASS_HUNTER
    {SPELL_RG_HEMORRHAGE,   SPELL_RG_EVISCERATE        },    // CLASS_ROGUE
    {SPELL_PR_HOLY_SMITE,   SPELL_PR_RENEW             },    // CLASS_PRIEST
    {SPELL_DK_PLAGU_STRIKE, SPELL_DK_BLOOD_STRIKE      },    // CLASS_DEATH_KNIGHT
    {SPELL_SH_EARTH_SHOCK,  SPELL_SH_LIGHTNING_BOLT    },    // CLASS_SHAMAN
    { SPELL_MG_FIREBALL,    SPELL_MG_FROSTNOVA         },    // CLASS_MAGE
    { SPELL_WL_CORRUPTION,  SPELL_WL_SHADOW_BOLT       },    // CLASS_WARLOCK
    {0,                     0                          },    // CLASS_UNK
    {SPELL_DR_MOONFIRE,     SPELL_DR_WRATH             }     // CLASS_DRUID
};

uint32 ClassSpellListHeroic[MAX_CLASSES][2] =
{
    { 0,                       0                            },
    { SPELL_WR_WHIRLWIND_H,    SPELL_WR_SURGE_H             },    // CLASS_WARRIOR
    { SPELL_PL_HOLY_SHOCK_H,   SPELL_PL_HAMMER_OF_JUSTICE_H },    // CLASS_PALADIN
    { SPELL_HU_SHOOT_H,        SPELL_HU_MULTI_SHOT_H        },    // CLASS_HUNTER
    { SPELL_RG_HEMORRHAGE_H,   SPELL_RG_EVISCERATE_H        },    // CLASS_ROGUE
    { SPELL_PR_HOLY_SMITE_H,   SPELL_PR_RENEW_H             },    // CLASS_PRIEST
    { SPELL_DK_PLAGU_STRIKE_H, SPELL_DK_BLOOD_STRIKE_H      },    // CLASS_DEATH_KNIGHT
    { SPELL_SH_EARTH_SHOCK_H,  SPELL_SH_LIGHTNING_BOLT_H    },    // CLASS_SHAMAN
    { SPELL_MG_FIREBALL_H,     SPELL_MG_FROSTNOVA_H         },    // CLASS_MAGE
    { SPELL_WL_CORRUPTION_H,   SPELL_WL_SHADOW_BOLT_H       },    // CLASS_WARLOCK
    { 0,                       0                            },    // CLASS_UNK
    { SPELL_DR_MOONFIRE,       SPELL_DR_WRATH               }     // CLASS_DRUID
};

enum Creatures
{
    MOB_TWISTED_VISAGE                            = 30625
};

//not in db
enum Yells
{
    SAY_AGGRO                                     = 0,
    SAY_SLAY                                      = 1,
    SAY_DEATH                                     = 2
};

enum Events
{
    EVENT_MINDFLAY                                = 1,
    EVENT_SDADOWBOLT                              = 2,
    EVENT_SHIVER                                  = 3
};

enum Achievements
{
    ACHIEV_QUICK_DEMISE_START_EVENT               = 20382,
};

class boss_volazj : public CreatureScript
{
public:
    boss_volazj() : CreatureScript("boss_volazj") { }

    struct boss_volazjAI : public BossAI
    {
        boss_volazjAI(Creature* creature) : BossAI(creature, DATA_HERALD_VOLAZJ), Summons(me), instance(creature->GetInstanceScript()) { }
       
        // returns the percentage of health after taking the given damage.
        uint32 GetHealthPct(uint32 damage)
        {
            if (damage > me->GetHealth())
                return 0;
            return 100*(me->GetHealth()-damage)/me->GetMaxHealth();
        }

        void DamageTaken(Unit* /*pAttacker*/, uint32 &damage)
        {
            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                damage = 0;

            if ((GetHealthPct(0) >= 66 && GetHealthPct(damage) < 66)||
                (GetHealthPct(0) >= 33 && GetHealthPct(damage) < 33))
            {
                me->InterruptNonMeleeSpells(false);
                DoCastSelf(SPELL_INSANITY, false);
            }
        }

        void SpellHitTarget(Unit* target, const SpellInfo* spell)
        {
            if (spell->Id == SPELL_INSANITY)
            {
                // Not good target or too many players
                if (target->GetTypeId() != TYPEID_PLAYER || insanityHandled > 4)
                    return;
                // First target - start channel visual and set self as unnattackable
                if (!insanityHandled)
                {
                    // Channel visual
                    DoCastSelf(INSANITY_VISUAL, true);
                    // Unattackable
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    me->SetControlled(true, UNIT_STATE_STUNNED);
                }
                // phase mask
                target->CastSpell(target, SPELL_INSANITY_TARGET+insanityHandled, true);
                // summon twisted party members for this target
                for (Player& player : me->GetMap()->GetPlayers())
                {
                    if (!player.IsAlive())
                        continue;
                    // Summon clone
                    if (Creature* summon = me->SummonCreature(MOB_TWISTED_VISAGE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_CORPSE_DESPAWN, 0))
                    {
                        // clone
                        player.CastSpell(summon, SPELL_CLONE_PLAYER, true);
                        // set phase
                        summon->SetPhaseMask((1<<(4+insanityHandled)), true);
                        summon->AI()->SetGuidData(player.GetGUID());
                        summon->AI()->AttackStart(target);
                    }
                }
                ++insanityHandled;
            }
        }

        void ResetPlayersPhaseMask()
        {
            for (Player& player : me->GetMap()->GetPlayers())
                player.RemoveAurasDueToSpell(GetSpellForPhaseMask(player.GetPhaseMask()));
        }

        void Reset()
        {
            BossAI::Reset();

            if (instance)
                instance->DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_QUICK_DEMISE_START_EVENT);

            // Visible for all players in insanity
            me->SetPhaseMask((1|16|32|64|128|256), true);
            // Used for Insanity handling
            insanityHandled = 0;

            ResetPlayersPhaseMask();

            // Cleanup
            Summons.DespawnAll();
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetControlled(false, UNIT_STATE_STUNNED);

            events.Reset();
        }

        void EnterCombat(Unit* who)
        {
            BossAI::EnterCombat(who);
            Talk(SAY_AGGRO);
            events.ScheduleEvent(EVENT_MINDFLAY,   8 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_SDADOWBOLT, 5 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_SHIVER,    15 * IN_MILLISECONDS);
            if (instance)
                instance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_QUICK_DEMISE_START_EVENT);
        }

        void JustSummoned(Creature* summon)
        {
            Summons.Summon(summon);
        }

        uint32 GetSpellForPhaseMask(uint32 phase)
        {
            uint32 spell = 0;
            switch (phase)
            {
                case 16:
                    spell = SPELL_INSANITY_PHASING_1;
                    break;
                case 32:
                    spell = SPELL_INSANITY_PHASING_2;
                    break;
                case 64:
                    spell = SPELL_INSANITY_PHASING_3;
                    break;
                case 128:
                    spell = SPELL_INSANITY_PHASING_4;
                    break;
                case 256:
                    spell = SPELL_INSANITY_PHASING_5;
                    break;
            }
            return spell;
        }

        void SummonedCreatureDespawn(Creature* summon)
        {
            uint32 phase= summon->GetPhaseMask();
            uint32 nextPhase = 0;
            Summons.Despawn(summon);

            // Check if all summons in this phase killed
            for (SummonList::const_iterator iter = Summons.begin(); iter != Summons.end(); ++iter)
            {
                if (Creature* visage = ObjectAccessor::GetCreature(*me, *iter))
                {
                    // Not all are dead
                    if (phase == visage->GetPhaseMask())
                        return;
                    else
                        nextPhase = visage->GetPhaseMask();
                }
            }

            // Roll Insanity
            uint32 spell = GetSpellForPhaseMask(phase);
            uint32 spell2 = GetSpellForPhaseMask(nextPhase);
            Map* map = me->GetMap();
            if (!map)
                return;

            for (Player& player : map->GetPlayers())
            {
                if (player.HasAura(spell))
                {
                    player.RemoveAurasDueToSpell(spell);
                    if (spell2) // if there is still some different mask cast spell for it
                        player.CastSpell(&player, spell2, true);
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            if (insanityHandled)
            {
                if (!Summons.empty())
                    return;

                insanityHandled = 0;
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetControlled(false, UNIT_STATE_STUNNED);
                me->RemoveAurasDueToSpell(INSANITY_VISUAL);
            }

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_MINDFLAY:
                        DoCastVictim(SPELL_MIND_FLAY);
                        events.ScheduleEvent(EVENT_MINDFLAY, 20 * IN_MILLISECONDS);
                        break;
                    case EVENT_SDADOWBOLT:
                        DoCastVictim(SPELL_SHADOW_BOLT_VOLLEY);
                        events.ScheduleEvent(EVENT_SDADOWBOLT, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_SHIVER:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            DoCast(target, SPELL_SHIVER);
                        events.ScheduleEvent(EVENT_SHIVER, 15 * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* killer)
        {
            Talk(SAY_DEATH);
            Summons.DespawnAll();
            ResetPlayersPhaseMask();
            BossAI::JustDied(killer);
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Talk(SAY_SLAY);
        }

        private:
            InstanceScript* instance;
            EventMap events;            
            uint32 insanityHandled;
            SummonList Summons;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_volazjAI(creature);
    }
};

class npc_twisted_visage : public CreatureScript
{
    public:
        npc_twisted_visage() : CreatureScript("npc_twisted_visage") { }

    struct npc_twisted_visageAI : public ScriptedAI
    {
        npc_twisted_visageAI(Creature* c) : ScriptedAI(c)
        {
            playerGUID.Clear();
        }

        void Reset()
        {
            const CreatureTemplate* cinfo = me->GetCreatureTemplate();
            me->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, 0.1 * cinfo->mindmg);
            me->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, 0.1 * cinfo->maxdmg);
            me->UpdateDamagePhysical(BASE_ATTACK);
        }

        void SetGuidData(ObjectGuid guid, uint32 /*type*/) override
        {
            playerGUID = guid;
            spellTimerA = urand(2000,15000);
            spellTimerB = urand(5000,20000);
        }

        void UpdateAI(uint32 diff)
        {
           // if (!UpdateVictim() || !playerGUID)
           //     return;

            if (spellTimerA < diff)
            {
                HandleSpell(0);
                spellTimerA = urand(5000,15000);
            }
            else
                spellTimerA -= diff;

            if (spellTimerB < diff)
            {
                HandleSpell(1);
                spellTimerB = urand(7500,20000);
            }
            else
                spellTimerB -=diff;

            DoMeleeAttackIfReady();
        }

        void HandleSpell(uint8 i)
        {
            if (!IsHeroic())
            {
                if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
                    if (uint32 spellID = ClassSpellList[player->getClass()][i])
                        DoCastVictim(spellID, true);
            }
            else
            {
                if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
                    if (uint32 spellID = ClassSpellListHeroic[player->getClass()][i])
                        DoCastVictim(spellID, true);
            }            
        }

        private:
            ObjectGuid playerGUID;
            uint32 spellTimerA;
            uint32 spellTimerB;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_twisted_visageAI (creature);
    }
};

void AddSC_boss_volazj()
{
    new boss_volazj();
    new npc_twisted_visage();
}
