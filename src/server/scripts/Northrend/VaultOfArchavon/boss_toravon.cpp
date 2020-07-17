/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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
#include "vault_of_archavon.h"
#include "Pet.h"

enum Spells
{
    // Toravon
    SPELL_FREEZING_GROUND   = 72090,    
    SPELL_FROZEN_ORB        = 72091,
    SPELL_WHITEOUT          = 72034,    // Every 38 sec. cast. (after SPELL_FROZEN_ORB)
    SPELL_FROZEN_MALLET     = 71993,

    // Frost Warder
    SPELL_FROST_BLAST       = 72123,    // don't know cd... using 20 secs.
    SPELL_FROZEN_MALLET_2   = 72122,

    // Frozen Orb
    SPELL_FROZEN_ORB_DMG    = 72081,    // priodic dmg aura
    SPELL_FROZEN_ORB_AURA   = 72067,    // make visible

    // Frozen Orb Stalker
    SPELL_FROZEN_ORB_SUMMON = 72094     // summon orb
};

enum Events
{
    //! Toravon
    EVENT_FREEZING_GROUND   = 1,
    EVENT_FROZEN_ORB        = 2,
    EVENT_WHITEOUT          = 3,
    EVENT_LOS_CHECK         = 5,

    //! Frost Warder
    EVENT_FROST_BLAST       = 4
};

class boss_toravon : public CreatureScript
{
    public:
        boss_toravon() : CreatureScript("boss_toravon") { }

        struct boss_toravonAI : public BossAI
        {
            boss_toravonAI(Creature* creature) : BossAI(creature, BOSS_TORAVON) { }
           
            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();

                DoCast(me, SPELL_FROZEN_MALLET);

                events.ScheduleEvent(EVENT_FROZEN_ORB, 13*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_WHITEOUT, 25*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FREEZING_GROUND, 15*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_LOS_CHECK, 1*IN_MILLISECONDS);
            }

            void Reset()
            {
                _Reset();

                timerLOSCheck = 0;
                me->GetPosition(&lastPos);
                instance->DoRemoveAurasOnPlayerAndPets(SPELL_WHITEOUT);
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();

                instance->DoRemoveAurasOnPlayerAndPets(SPELL_WHITEOUT);
            }

            void UpdateAI(uint32 diff)
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
                        case EVENT_FROZEN_ORB:
                            me->CastCustomSpell(SPELL_FROZEN_ORB, SPELLVALUE_MAX_TARGETS, 1, me);
                            events.ScheduleEvent(EVENT_FROZEN_ORB, 32*IN_MILLISECONDS);
                            break;
                        case EVENT_WHITEOUT:
                            DoCast(me, SPELL_WHITEOUT);
                            events.ScheduleEvent(EVENT_WHITEOUT, 38*IN_MILLISECONDS);
                            break;
                        case EVENT_FREEZING_GROUND:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                                DoCast(target, SPELL_FREEZING_GROUND);
                            events.ScheduleEvent(EVENT_FREEZING_GROUND, 20*IN_MILLISECONDS);
                            break;
                        case EVENT_LOS_CHECK:
                            if (Unit* victim = me->GetVictim())
                            {
                                if (!(me->IsWithinLOS(victim->GetPositionX(), victim->GetPositionY(), victim->GetPositionZ())))
                                {
                                    if (me->GetExactDist(&lastPos) == 0.0f)
                                    {
                                        timerLOSCheck++;
                                        if (timerLOSCheck >= 5)
                                        {
                                            me->NearTeleportTo(victim->GetPositionX(), victim->GetPositionY(), victim->GetPositionZ(), me->GetOrientation());
                                            timerLOSCheck = 0;
                                        }
                                    }
                                    else
                                    {
                                        me->GetPosition(&lastPos);
                                        timerLOSCheck = 0;
                                    }
                                }
                            }
                            events.RescheduleEvent(EVENT_LOS_CHECK, 1*IN_MILLISECONDS);
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
            uint32 timerLOSCheck;
            Position lastPos;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_toravonAI(creature);
        }
};

class mob_frost_warder : public CreatureScript
{
    public:
        mob_frost_warder() : CreatureScript("mob_frost_warder") { }

        struct mob_frost_warderAI : public ScriptedAI
        {
            mob_frost_warderAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                events.Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                DoZoneInCombat();

                DoCast(me, SPELL_FROZEN_MALLET_2);

                events.ScheduleEvent(EVENT_FROST_BLAST, 5*IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (events.ExecuteEvent() == EVENT_FROST_BLAST)
                {
                    DoCastVictim(SPELL_FROST_BLAST);
                    events.ScheduleEvent(EVENT_FROST_BLAST, 20*IN_MILLISECONDS);
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_frost_warderAI(creature);
        }
};

class mob_frozen_orb : public CreatureScript
{
public:
    mob_frozen_orb() : CreatureScript("mob_frozen_orb") { }

    struct mob_frozen_orbAI : public ScriptedAI
    {
        mob_frozen_orbAI(Creature* creature) : ScriptedAI(creature)
        { 
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
        }

        void Reset()
        {
            done = false;
            killTimer = 60*IN_MILLISECONDS; // if after this time there is no victim -> destroy!
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoZoneInCombat();
        }

        void UpdateAI(uint32 diff)
        {
            if (!done)
            {
                DoCast(me, SPELL_FROZEN_ORB_AURA, true);
                DoCast(me, SPELL_FROZEN_ORB_DMG, true);
                done = true;
            }

            if (killTimer <= diff)
            {
                if (!UpdateVictim())
                    me->DespawnOrUnsummon();
                killTimer = 10*IN_MILLISECONDS;
            }
            else
                killTimer -= diff;
        }

    private:
        uint32 killTimer;
        bool done;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_frozen_orbAI(creature);
    }
};

class mob_frozen_orb_stalker : public CreatureScript
{
    public:
        mob_frozen_orb_stalker() : CreatureScript("mob_frozen_orb_stalker") { }

        struct mob_frozen_orb_stalkerAI : public ScriptedAI
        {
            mob_frozen_orb_stalkerAI(Creature* creature) : ScriptedAI(creature)
            {
                creature->SetVisible(true);
                creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_REMOVE_CLIENT_CONTROL);
                creature->SetReactState(REACT_PASSIVE);

                instance = creature->GetInstanceScript();
                spawned = false;

                SetCombatMovement(false);
            }

            void UpdateAI(uint32 /*diff*/)
            {
                if (spawned)
                    return;

                spawned = true;
                Unit* toravon = ObjectAccessor::GetCreature(*me, instance ? instance->GetData64(NPC_TORAVON) : 0);
                if (!toravon)
                    return;

                uint8 num_orbs = RAID_MODE(1, 3);
                for (uint8 i = 0; i < num_orbs; ++i)
                {
                    Position pos;
                    me->GetNearPoint(toravon, pos.m_positionX, pos.m_positionY, pos.m_positionZ, 0.0f, 0.0f, 0.0f);
                    me->SetPosition(pos);
                    DoCast(me, SPELL_FROZEN_ORB_SUMMON);
                }
            }

        private:
            InstanceScript* instance;
            bool spawned;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_frozen_orb_stalkerAI(creature);
        }
};

void AddSC_boss_toravon()
{
    new boss_toravon();
    new mob_frost_warder();
    new mob_frozen_orb();
    new mob_frozen_orb_stalker();
}
