/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2012 Rising-Gods <https://www.rising-gods.de/>
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
SDName: Boss_Ouro
SD%Complete: 85
SDComment: No model for submerging. Currently just invisible.
SDCategory: Temple of Ahn'Qiraj
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "temple_of_ahnqiraj.h"
#include "Player.h"

enum Spells
{
    SPELL_SWEEP             = 26103, //player knockback every 15sec
    SPELL_SANDBLAST         = 26102, //player stunn and damage every 20sec
    SPELL_BOLDER            = 26616,
    SPELL_GROUND_RUPTURE    = 26100, //damage whe ouro rises
    SPELL_BIRTH             = 26262, //The Birth Animation
    SPELL_BERSERK           = 26615,
    SPELL_QUAKE             = 26093,
};

enum Summons
{
    NPC_SCRAB           = 15718,
    NPC_TRIGGER         = 15717,
    NPC_DIRT_MAUND      = 15712
};

enum Events
{
    EVENT_SWEEP         = 1,
    EVENT_SANDBLAST,
    EVENT_SUMMON_SCRABS,

    EVENT_BIRTH,
    EVENT_SUBMERGE,
    EVENT_SUBMERGE_END,
    EVENT_MEELE_CHECK,
    EVENT_BOLDER,
    EVENT_QUAKE,

    EVENT_BERSERK
};

enum Phases
{
    PHASE_MEELE         = 1,
    PHASE_UNDERGROUND   = 2,
    PHASE_ENRAGE        = 3
};

class boss_ouro : public CreatureScript
{
    public:
        boss_ouro() : CreatureScript("boss_ouro") { }

        struct boss_ouroAI : public BossAI
        {
            boss_ouroAI(Creature* creature) : BossAI(creature, BOSS_OURO)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);
            }

            void Reset() override
            {
                BossAI::Reset();

                me->SetVisible(false);
                me->SetReactState(REACT_PASSIVE);
                PhaseUnderground = false;
            }

            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);

                me->SetVisible(true);
                me->SetReactState(REACT_AGGRESSIVE);

                events.RescheduleEvent(EVENT_BIRTH, 0.5 * IN_MILLISECONDS);
                events.RescheduleEvent(EVENT_SWEEP, 15 * IN_MILLISECONDS);
                events.RescheduleEvent(EVENT_SANDBLAST, 20 * IN_MILLISECONDS);
                events.RescheduleEvent(EVENT_SUBMERGE, 90 * IN_MILLISECONDS);
                events.RescheduleEvent(EVENT_MEELE_CHECK, 10 * IN_MILLISECONDS, 0, PHASE_MEELE);
                events.RescheduleEvent(EVENT_SUMMON_SCRABS, 45 * IN_MILLISECONDS);

                events.SetPhase(PHASE_MEELE);
            }

            void JustSummoned(Creature* summon) override
            {
                if (summon->GetEntry() == NPC_SCRAB) // despawn scab after 45sec
                    summon->DespawnOrUnsummon(45 * IN_MILLISECONDS);
            }

            bool CheckCombatState()
            {
                if (!me->IsInCombat())
                {
                    for (Player& player : me->GetMap()->GetPlayers())
                    {
                        if (player.IsAlive() && !player.IsGameMaster() && player.IsWithinDist2d(me, 115.0f))
                        {
                            EnterCombat(&player);
                            return false; // At this moment we don't want any updates
                        }
                    }
                }
                else
                {
                    for (Player& player : me->GetMap()->GetPlayers())
                        if (player.IsAlive() && !player.IsGameMaster())
                            return true; // Return when at least one player is alive

                    EnterEvadeMode();
                    return false;
                }
                return false;
            }

            void UpdateAI(uint32 diff) override
            {
                //! UpdateVictim replacement
                if (!CheckCombatState())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SWEEP:
                            DoCastVictim(SPELL_SWEEP);
                            events.RescheduleEvent(EVENT_SWEEP, 15 * IN_MILLISECONDS);
                            break;
                        case EVENT_SANDBLAST:
                            DoCastVictim(SPELL_SANDBLAST);
                            events.RescheduleEvent(EVENT_SANDBLAST, 20 * IN_MILLISECONDS);
                            break;
                        case EVENT_SUMMON_SCRABS:
                            for (uint8 i=0; i<20; i++)
                            {
                                if (Creature* scrab = me->SummonCreature(NPC_SCRAB, me->GetPositionX() + (irand(-25, 25)), me->GetPositionY() + (irand(-25, 25)), me->GetPositionZ(), me->GetOrientation() + (frand(-M_PI_F, M_PI_F)), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000))
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                        scrab->AI()->AttackStart(target);
                            }
                            events.RescheduleEvent(EVENT_SUMMON_SCRABS, 45 * IN_MILLISECONDS);
                            break;
                        case EVENT_BIRTH:
                            DoCastSelf(SPELL_BIRTH);
                            break;
                        case EVENT_SUBMERGE:
                            me->SetVisible(false);
                            me->RemoveAllAuras();
                            PhaseUnderground = true;
                            events.RescheduleEvent(EVENT_SUBMERGE_END, 30 * IN_MILLISECONDS);
                            events.RescheduleEvent(EVENT_QUAKE, 5 * IN_MILLISECONDS);
                            events.CancelEvent(EVENT_SWEEP);
                            events.CancelEvent(EVENT_SANDBLAST);
                            break;
                        case EVENT_SUBMERGE_END:
                            me->SetVisible(true);
                            PhaseUnderground = false;
                            events.RescheduleEvent(EVENT_SWEEP, 15 * IN_MILLISECONDS);
                            events.RescheduleEvent(EVENT_SANDBLAST, 20 * IN_MILLISECONDS);
                            events.RescheduleEvent(EVENT_SUBMERGE, 90 * IN_MILLISECONDS);
                            events.RescheduleEvent(EVENT_MEELE_CHECK, 10 * IN_MILLISECONDS);
                            events.CancelEvent(EVENT_QUAKE);
                            if (Unit* target = SelectTarget(SELECT_TARGET_MAXTHREAT, 0))
                                me->NearTeleportTo(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), me->GetOrientation());
                            DoCastAOE(SPELL_GROUND_RUPTURE);
                            DoCastSelf(SPELL_BIRTH);
                            break;
                        case EVENT_MEELE_CHECK:
                            if (!PhaseUnderground)
                                if (!me->IsWithinMeleeRange(me->GetVictim()))
                                    if (me->HealthAbovePct(20))
                                        events.RescheduleEvent(EVENT_SUBMERGE, 0);
                            events.RescheduleEvent(EVENT_MEELE_CHECK, 10 * IN_MILLISECONDS);
                            break;                       
                        case EVENT_QUAKE:
                            for (uint8 i = 0; i<3; i++)
                            {
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                    if (Creature* trigger = me->SummonCreature(NPC_DIRT_MAUND, me->GetPositionX() + (irand(-25, 25)), target->GetPositionY() + (irand(-25, 25)), target->GetPositionZ(), target->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 15000))
                                    {
                                        trigger->GetMotionMaster()->MoveFollow(target, 0, 0);
                                        trigger->setFaction(16); // 16 - monster
                                    }

                            }
                            events.RescheduleEvent(EVENT_QUAKE, 15000);
                            break;                       
                        case EVENT_BOLDER:
                            if (!me->IsWithinMeleeRange(me->GetVictim()))
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                    DoCast(target, SPELL_BOLDER);
                            events.RescheduleEvent(EVENT_BOLDER, 3 * IN_MILLISECONDS);
                            break;
                    }
                }

                if (me->GetHealthPct() <= 20 && !me->HasAura(SPELL_BERSERK))
                {
                    DoCastSelf(SPELL_BERSERK);
                    events.RescheduleEvent(EVENT_BOLDER, 2.5 * IN_MILLISECONDS);
                }

                if (!PhaseUnderground)
                    DoMeleeAttackIfReady();
            }

        private:
            bool PhaseUnderground;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_ouroAI (creature);
    }
};

class npc_quake_target : public CreatureScript
{
    public:
        npc_quake_target() : CreatureScript("npc_quake_target") { }

        struct npc_quake_targetAI : public ScriptedAI
        {
            npc_quake_targetAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = me->GetInstanceScript();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetImmuneToAll(true);
            }

            void Reset() override
            {
                QuakeTimer = 1000;
            }

            void UpdateAI(uint32 diff) override
            {
                if (QuakeTimer <= diff)
                {
                    DoCastAOE(SPELL_QUAKE, true);
                    QuakeTimer = 1 * IN_MILLISECONDS;

                    if (instance)
                        if (instance->GetBossState(BOSS_OURO) != IN_PROGRESS)
                            me->DespawnOrUnsummon();
                }
                else QuakeTimer -= diff;
            }

        private:
            InstanceScript* instance;
            uint32 QuakeTimer;

        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_quake_targetAI(creature);
        }
};

void AddSC_boss_ouro()
{
    new boss_ouro();
    new npc_quake_target();
}
