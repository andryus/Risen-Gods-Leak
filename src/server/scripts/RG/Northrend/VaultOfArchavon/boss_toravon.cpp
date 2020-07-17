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
#include "vault_of_archavon.h"
#include "Pet.h"

enum Spells
{
    // Toravon
    SPELL_FREEZING_GROUND   = 72090,
    SPELL_FROZEN_ORB        = 72091,
    SPELL_WHITEOUT          = 72034,    // Every 38 sec. cast. (after SPELL_FROZEN_ORB)
    SPELL_WHITEOUT_H        = 72096,
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
    EVENT_ORB_SUMMON        = 6,

    //! Frost Warder
    EVENT_FROST_BLAST       = 4
};

enum Misc
{
    NPC_FROZEN_ORB          = 38456,
    NPC_ORB_STALKER         = 38461
};

class boss_toravon : public CreatureScript
{
    public:
        boss_toravon() : CreatureScript("boss_toravon") { }

        struct boss_toravonAI : public BossAI
        {
            boss_toravonAI(Creature* creature) : BossAI(creature, BOSS_TORAVON) { }

            void EnterCombat(Unit* who)
            {
                BossAI::EnterCombat(who);

                DoCastSelf(SPELL_FROZEN_MALLET);

                events.ScheduleEvent(EVENT_FROZEN_ORB,      13*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_WHITEOUT,        25*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FREEZING_GROUND, 15*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_LOS_CHECK,        1*IN_MILLISECONDS);
            }

            void Reset()
            {
                BossAI::Reset();

                timerLOSCheck = 0;
                me->GetPosition(&lastPos);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_WHITEOUT);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_WHITEOUT_H);
                std::list<Creature*> AddList;
                me->GetCreatureListWithEntryInGrid(AddList, NPC_FROZEN_ORB, 300.0f);
                if (!AddList.empty())
                    for (std::list<Creature*>::iterator itr = AddList.begin(); itr != AddList.end(); itr++)
                        (*itr)->DespawnOrUnsummon();
            }

            void JustDied(Unit* killer)
            {
                BossAI::JustDied(killer);

                std::list<Creature*> AddList;
                me->GetCreatureListWithEntryInGrid(AddList, NPC_FROZEN_ORB, 300.0f);
                if (!AddList.empty())
                    for (std::list<Creature*>::iterator itr = AddList.begin(); itr != AddList.end(); itr++)
                        (*itr)->DespawnOrUnsummon();

                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_WHITEOUT);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_WHITEOUT_H);
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                _DespawnAtEvade();
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
                            DoCastSelf(SPELL_FROZEN_ORB);
                            events.ScheduleEvent(EVENT_ORB_SUMMON, 2 * IN_MILLISECONDS);
                            events.ScheduleEvent(EVENT_FROZEN_ORB, 32*IN_MILLISECONDS);
                            break;
                        case EVENT_ORB_SUMMON:
                            for (uint8 i = 0; i < uint32(me->GetMap()->Is25ManRaid() ? 3 : 1); ++i)
                            {
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 150.0f, true))
                                    target->SummonCreature(NPC_ORB_STALKER, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 600000);
                            }
                            break;
                        case EVENT_WHITEOUT:
                            DoCastSelf(SPELL_WHITEOUT);
                            events.ScheduleEvent(EVENT_WHITEOUT, 38*IN_MILLISECONDS);
                            break;
                        case EVENT_FREEZING_GROUND:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 150, true))
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

class npc_frost_warder : public CreatureScript
{
    public:
        npc_frost_warder() : CreatureScript("npc_frost_warder") { }

        struct npc_frost_warderAI : public ScriptedAI
        {
            npc_frost_warderAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                events.Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                DoZoneInCombat();

                DoCastSelf(SPELL_FROZEN_MALLET_2);

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
            return new npc_frost_warderAI(creature);
        }
};

class npc_frozen_orb : public CreatureScript
{
public:
    npc_frozen_orb() : CreatureScript("npc_frozen_orb") { }

    struct npc_frozen_orbAI : public ScriptedAI
    {
        npc_frozen_orbAI(Creature* creature) : ScriptedAI(creature) { }

        void InitializeAI() override
        {
            ScriptedAI::InitializeAI();
            Creature* toravon = ObjectAccessor::GetCreature(*me, ObjectGuid(me->GetInstanceScript()->GetGuidData(NPC_TORAVON)));
            if (!toravon)
                return;

            toravon->AI()->JustSummoned(me);

            DoCastSelf(SPELL_FROZEN_ORB_AURA, true);
            DoCastSelf(SPELL_FROZEN_ORB_DMG, true);
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_frozen_orbAI(creature);
    }
};

class npc_frozen_orb_stalker : public CreatureScript
{
    public:
        npc_frozen_orb_stalker() : CreatureScript("npc_frozen_orb_stalker") { }

        struct npc_frozen_orb_stalkerAI : public ScriptedAI
        {
            npc_frozen_orb_stalkerAI(Creature* creature) : ScriptedAI(creature)
            {
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
                Unit* toravon = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(NPC_TORAVON)) : ObjectGuid::Empty);
                if (!toravon)
                    return;

                uint8 num_orbs = RAID_MODE(1, 3);
                for (uint8 i = 0; i < num_orbs; ++i)
                {
                    Position pos;
                    me->GetNearPoint(toravon, pos.m_positionX, pos.m_positionY, pos.m_positionZ, 0.0f, 0.0f, 0.0f);
                    me->SetPosition(pos);
                    DoCastSelf(SPELL_FROZEN_ORB_SUMMON);
                }
            }

        private:
            InstanceScript* instance;
            bool spawned;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_frozen_orb_stalkerAI(creature);
        }
};

void AddSC_boss_toravon()
{
    new boss_toravon();
    new npc_frost_warder();
    new npc_frozen_orb();
    new npc_frozen_orb_stalker();
}
