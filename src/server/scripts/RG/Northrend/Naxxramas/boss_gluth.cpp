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
#include "naxxramas.h"
#include "SpellInfo.h"

enum Spells
{
    SPELL_MORTAL_WOUND          = 25646,
    SPELL_ENRAGE                = 28371,
    SPELL_ENRAGE_H              = 54427,
    SPELL_DECIMATE              = 28374,
    SPELL_DECIMATE_VISUAL       = 28375,
    SPELL_BERSERK               = 26662,
    SPELL_INFECTED_WOUND        = 29306,
    SPELL_INFECTED_WOUND_AURA   = 29307,
};

enum Creatures
{
    MOB_ZOMBIE                  = 16360,
};

enum Events
{
    EVENT_WOUND         = 1,
    EVENT_ENRAGE        = 2,
    EVENT_DECIMATE      = 3,
    EVENT_BERSERK       = 4,
    EVENT_SUMMON        = 5,
    EVENT_CHECK_ZOMBIE  = 6,
};

const Position PosSummon[3] =
{
    {3267.9f, -3172.1f, 297.42f, 0.94f},
    {3253.2f, -3132.3f, 297.42f, 0},
    {3308.3f, -3185.8f, 297.42f, 1.58f},
};

#define EMOTE_NEARBY    "Gluth verschlingt einen nahen Zombie!"
#define EMOTE_ENRAGE    "Gluth wird wuetend!"
#define EMOTE_DECIMATE  "Gluth dezimiert alles Fleisch in der Naehe!"

class boss_gluth : public CreatureScript
{
    public:
        boss_gluth() : CreatureScript("boss_gluth") { }

        struct boss_gluthAI : public BossAI
        {
            boss_gluthAI(Creature* creature) : BossAI(creature, BOSS_GLUTH)
            {
                me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_INFECTED_WOUND, true);
            }

            void EnterCombat(Unit* who)
            {
                BossAI::EnterCombat(who);
                events.ScheduleEvent(EVENT_WOUND, 10000);
                events.ScheduleEvent(EVENT_ENRAGE, 15000);
                events.ScheduleEvent(EVENT_DECIMATE, 90000);
                events.ScheduleEvent(EVENT_BERSERK, 8*60000);
                events.ScheduleEvent(EVENT_SUMMON, 15000);
                events.ScheduleEvent(EVENT_CHECK_ZOMBIE, 500);
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (who->GetEntry() == MOB_ZOMBIE && me->IsWithinDistInMap(who, 7))
                {
                    SetGazeOn(who);
                    me->MonsterTextEmote(EMOTE_NEARBY, 0, true);
                }
                else
                    BossAI::MoveInLineOfSight(who);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictimWithGaze())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_WOUND:
                            DoCastVictim(SPELL_MORTAL_WOUND, true);
                            events.ScheduleEvent(EVENT_WOUND, 10000);
                            break;
                        case EVENT_ENRAGE:
                            DoCastSelf(SPELL_ENRAGE);
                            me->MonsterTextEmote(EMOTE_ENRAGE, 0, true);
                            events.ScheduleEvent(EVENT_ENRAGE, 15000);
                            break;
                        case EVENT_DECIMATE:
                            {
                                me->MonsterTextEmote(EMOTE_DECIMATE, 0, true);
                                DoCastAOE(SPELL_DECIMATE);
                                std::list<Creature*> zombieList;
                                me->GetCreatureListWithEntryInGrid(zombieList, MOB_ZOMBIE, 220.0f);
                                for (std::list<Creature*>::const_iterator itr = zombieList.begin(); itr != zombieList.end(); ++itr)
                                    if ((*itr) && (*itr)->IsAlive())
                                        (*itr)->SetHealth((*itr)->GetMaxHealth()*0.05f);
                                events.ScheduleEvent(EVENT_DECIMATE, 90000);
                            }
                            break;
                        case EVENT_BERSERK:
                            DoCast(me, SPELL_BERSERK);
                            events.ScheduleEvent(EVENT_BERSERK, 5*60000);
                            break;
                        case EVENT_SUMMON:
                            for (int32 i = 0; i < RAID_MODE(1, 2); ++i)
                                if (Creature* zombie = DoSummon(MOB_ZOMBIE, PosSummon[rand32() % RAID_MODE(1, 3)]))
                                {
                                    zombie->AI()->AttackStart(me);
                                    zombie->GetThreatManager().AddThreat(me, 1.0f);
                                }
                            events.ScheduleEvent(EVENT_SUMMON, 10000);
                            break;
                        case EVENT_CHECK_ZOMBIE:
                            if (Creature* zombie = me->FindNearestCreature(MOB_ZOMBIE, 7.0f, true))
                            {
                                if (me->IsWithinMeleeRange(zombie))
                                {
                                    me->Kill(zombie);
                                    me->ModifyHealth(int32(me->CountPctFromMaxHealth(5)));
                                }
                            }
                            events.ScheduleEvent(EVENT_CHECK_ZOMBIE, 500);
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_gluthAI (creature);
    }
};

class npc_zombie_chow : public CreatureScript
{
    public:
        npc_zombie_chow() : CreatureScript("npc_zombie_chow") { }

        struct npc_zombie_chowAI : public ScriptedAI
        {
            npc_zombie_chowAI(Creature *creature) : ScriptedAI(creature)
            {
                DoCast(me, SPELL_INFECTED_WOUND_AURA);
            }

            // Attack Gluth when hit by decimate
            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_DECIMATE)
                {
                    me->GetMotionMaster()->MoveFollow(caster, 0, 0);
                    me->SetReactState(REACT_PASSIVE);
                    me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                    me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                }
            }
        };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_zombie_chowAI(pCreature);
    }
};

void AddSC_boss_gluth()
{
    new boss_gluth();
    new npc_zombie_chow();
}
