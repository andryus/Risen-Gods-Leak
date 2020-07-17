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
SDName: Boss_Ragnaros
SD%Complete: 95
SDComment: some spells doesnt work correctly
SDCategory: Molten Core
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "molten_core.h"

enum Texts
{
    SAY_SUMMON_MAJ              = 0,
    SAY_ARRIVAL1_RAG            = 1,
    SAY_ARRIVAL2_MAJ            = 2,
    SAY_ARRIVAL3_RAG            = 3,
    SAY_ARRIVAL5_RAG            = 4,
    SAY_REINFORCEMENTS1         = 5,
    SAY_REINFORCEMENTS2         = 6,
    SAY_HAND                    = 7,
    SAY_WRATH                   = 8,
    SAY_KILL                    = 9,
    SAY_MAGMABURST              = 10
};

enum Spells
{
    SPELL_HAND_OF_RAGNAROS      = 19780,
    SPELL_WRATH_OF_RAGNAROS     = 20566,
    SPELL_LAVA_BURST            = 21158,
    SPELL_MAGMA_BLAST           = 20565,                   // Ranged attack
    SPELL_SONS_OF_FLAME_DUMMY   = 21108,                   // Server side effect
    SPELL_RAGSUBMERGE           = 21107,                   // Stealth aura
    SPELL_RAGEMERGE             = 20568,
    SPELL_MELT_WEAPON           = 21388,
    SPELL_ELEMENTAL_FIRE        = 20564,
    SPELL_ERRUPTION             = 17731,
    SPELL_ROOT_SELF             = 23973
};

enum Events
{
    EVENT_ERUPTION              = 1,
    EVENT_WRATH_OF_RAGNAROS     = 2,
    EVENT_HAND_OF_RAGNAROS      = 3,
    EVENT_LAVA_BURST            = 4,
    EVENT_ELEMENTAL_FIRE        = 5,
    EVENT_MAGMA_BLAST           = 6,
    EVENT_SUBMERGE              = 7,

    EVENT_INTRO_1               = 8,
    EVENT_INTRO_2               = 9,
    EVENT_INTRO_3               = 10,
    EVENT_INTRO_4               = 11,
    EVENT_INTRO_5               = 12
};

enum RagnarosCreatures
{
    NPC_SON_OF_FLAME            = 12143
};

class boss_ragnaros : public CreatureScript
{
    public:
        boss_ragnaros() : CreatureScript("boss_ragnaros") { }

        struct boss_ragnarosAI : public BossAI
        {
            boss_ragnarosAI(Creature* creature) : BossAI(creature, BOSS_RAGNAROS)
            {
                _introState = 0;
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }

            void Reset() override
            {
                BossAI::Reset();
                _emergeTimer = 90 * IN_MILLISECONDS;
                _hasYelledMagmaBurst = false;
                _hasSubmergedOnce = false;
                _isBanished = false;
                me->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
            }

            void EnterCombat(Unit* victim) override
            {
                BossAI::EnterCombat(victim);
                events.ScheduleEvent(EVENT_ERUPTION,          15 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_WRATH_OF_RAGNAROS, 30 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_HAND_OF_RAGNAROS,  25 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_LAVA_BURST,        10 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_ELEMENTAL_FIRE,     3 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_MAGMA_BLAST,        2 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SUBMERGE,         180 * IN_MILLISECONDS);
            }

            void KilledUnit(Unit* /*victim*/) override
            {
                if (urand(0, 99) < 25)
                    Talk(SAY_KILL);
            }

            void UpdateAI(uint32 diff) override
            {
                if (_introState != 2)
                {
                    if (!_introState)
                    {
                        me->HandleEmoteCommand(EMOTE_ONESHOT_EMERGE);
                        events.ScheduleEvent(EVENT_INTRO_1,  4 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_INTRO_2, 23 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_INTRO_3, 42 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_INTRO_4, 43 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_INTRO_5, 53 * IN_MILLISECONDS);
                        _introState = 1;
                    }

                    events.Update(diff);

                    while (uint32 eventId = events.ExecuteEvent())
                    {
                        switch (eventId)
                        {
                            case EVENT_INTRO_1:
                                Talk(SAY_ARRIVAL1_RAG);
                                break;
                            case EVENT_INTRO_2:
                                Talk(SAY_ARRIVAL3_RAG);
                                break;
                            case EVENT_INTRO_3:
                                me->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK1H);
                                break;
                            case EVENT_INTRO_4:
                                Talk(SAY_ARRIVAL5_RAG);
                                if (Creature* executus = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_MAJORDOMO_EXECUTUS))))
                                    me->Kill(executus);
                                break;
                            case EVENT_INTRO_5:
                                me->SetReactState(REACT_AGGRESSIVE);
                                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                _introState = 2;
                                break;
                            default:
                                break;
                        }
                    }
                }
                else
                {
                    if (_isBanished && ((_emergeTimer <= diff) || (instance->GetData(DATA_RAGNAROS_ADDS)) > 8))
                    {
                        // Become unbanished again
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->setFaction(FACTION_HOSTILE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_EMERGE);
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            AttackStart(target);
                        instance->SetData(DATA_RAGNAROS_ADDS, 0);

                        //DoCastSelf(SPELL_RAGEMERGE); //"phase spells" didnt worked correctly so Ive commented them and wrote solution witch doesnt need core support
                        _isBanished = false;
                    }
                    else if (_isBanished)
                    {
                        _emergeTimer -= diff;
                        //Do nothing while banished
                        return;
                    }

                    //Return since we have no target
                    if (!UpdateVictim())
                        return;

                    events.Update(diff);

                    while (uint32 eventId = events.ExecuteEvent())
                    {
                        switch (eventId)
                        {
                            case EVENT_ERUPTION:
                                DoCastVictim(SPELL_ERRUPTION);
                                events.ScheduleEvent(EVENT_ERUPTION, urand(20, 45) * IN_MILLISECONDS);
                                break;
                            case EVENT_WRATH_OF_RAGNAROS:
                                DoCastVictim(SPELL_WRATH_OF_RAGNAROS);
                                if (urand(0, 1))
                                    Talk(SAY_WRATH);
                                events.ScheduleEvent(EVENT_WRATH_OF_RAGNAROS, 25 * IN_MILLISECONDS);
                                break;
                            case EVENT_HAND_OF_RAGNAROS:
                                DoCastSelf(SPELL_HAND_OF_RAGNAROS);
                                if (urand(0, 1))
                                    Talk(SAY_HAND);
                                events.ScheduleEvent(EVENT_HAND_OF_RAGNAROS, 20 * IN_MILLISECONDS);
                                break;
                            case EVENT_LAVA_BURST:
                                DoCastVictim(SPELL_LAVA_BURST);
                                events.ScheduleEvent(EVENT_LAVA_BURST, 10 * IN_MILLISECONDS);
                                break;
                            case EVENT_ELEMENTAL_FIRE:
                                DoCastVictim(SPELL_ELEMENTAL_FIRE);
                                events.ScheduleEvent(EVENT_ELEMENTAL_FIRE, urand(10, 14) * IN_MILLISECONDS);
                                break;
                            case EVENT_MAGMA_BLAST:
                                if (!me->IsWithinMeleeRange(me->GetVictim()))
                                {
                                    DoCastVictim(SPELL_MAGMA_BLAST);
                                    if (!_hasYelledMagmaBurst)
                                    {
                                        //Say our dialog
                                        Talk(SAY_MAGMABURST);
                                        _hasYelledMagmaBurst = true;
                                    }
                                }
                                events.ScheduleEvent(EVENT_MAGMA_BLAST, 2.5 * IN_MILLISECONDS);
                                break;
                            case EVENT_SUBMERGE:
                            {
                                if (instance && !_isBanished)
                                {
                                    //Creature spawning and ragnaros becomming unattackable
                                    //is not very well supported in the core //no it really isnt
                                    //so added normaly spawning and banish workaround and attack again after 90 secs.
                                    me->RemoveAllAuras();
                                    me->AttackStop();
                                    ResetThreatList();
                                    me->SetReactState(REACT_PASSIVE);
                                    me->InterruptNonMeleeSpells(false);
                                    //Root self
                                    //DoCastSelf(SPELL_ROOT_SELF);
                                    me->setFaction(FACTION_FRIENDLY_TO_ALL);
                                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                    me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_SUBMERGED);
                                    me->HandleEmoteCommand(EMOTE_ONESHOT_SUBMERGE);
                                    instance->SetData(DATA_RAGNAROS_ADDS, 0);

                                    if (!_hasSubmergedOnce)
                                    {
                                        Talk(SAY_REINFORCEMENTS1);

                                        // summon 8 elementals
                                        for (uint8 i = 0; i < 8; ++i)
                                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                                if (Creature* summoned = me->SummonCreature(NPC_SON_OF_FLAME, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 900* IN_MILLISECONDS))
                                                    summoned->AI()->AttackStart(target);

                                        _hasSubmergedOnce = true;
                                        _isBanished = true;
                                        //DoCastSelf(SPELL_RAGSUBMERGE);
                                        _emergeTimer = 90 * IN_MILLISECONDS;

                                    }
                                    else
                                    {
                                        Talk(SAY_REINFORCEMENTS2);

                                        for (uint8 i = 0; i < 8; ++i)
                                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                                if (Creature* summoned = me->SummonCreature(NPC_SON_OF_FLAME, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 900* IN_MILLISECONDS))
                                                    summoned->AI()->AttackStart(target);

                                        _isBanished = true;
                                        //DoCastSelf(SPELL_RAGSUBMERGE);
                                        _emergeTimer = 90 * IN_MILLISECONDS;
                                    }
                                }
                                events.ScheduleEvent(EVENT_SUBMERGE, 180 * IN_MILLISECONDS);
                                break;
                            }
                            default:
                                break;
                        }
                    }

                    DoMeleeAttackIfReady();
                }
            }

        private:
            uint32 _emergeTimer;
            uint8 _introState;
            bool _hasYelledMagmaBurst;
            bool _hasSubmergedOnce;
            bool _isBanished;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_ragnarosAI>(creature);
        }
};

class npc_son_of_flame : public CreatureScript
{
    public:
        npc_son_of_flame() : CreatureScript("npc_SonOfFlame") { }

        struct npc_son_of_flameAI : public ScriptedAI 
        {
            npc_son_of_flameAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = me->GetInstanceScript();
            }

            void JustDied(Unit* /*killer*/) override
            {
                instance->SetData(DATA_RAGNAROS_ADDS, 1);
            }

            void UpdateAI(uint32 /*diff*/) override
            {
                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }

        private:
            InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<npc_son_of_flameAI>(creature);
        }
};

void AddSC_boss_ragnaros()
{
    new boss_ragnaros();
    new npc_son_of_flame();
}