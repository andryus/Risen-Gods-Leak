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
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "ulduar.h"
#include "Player.h"

// ###### Texts ######
enum Yells
{
    // Steelbreaker
    SAY_STEELBREAKER_AGGRO                      = 0,
    SAY_STEELBREAKER_SLAY                       = 1,
    SAY_STEELBREAKER_POWER                      = 2,
    SAY_STEELBREAKER_DEATH_1                    = 3,
    SAY_STEELBREAKER_DEATH_2                    = 4,
    SAY_STEELBREAKER_BERSERK                    = 5,
    // Molgeim
    SAY_MOLGEIM_AGGRO                           = 0,
    SAY_MOLGEIM_SLAY                            = 1,
    SAY_MOLGEIM_RUNE_DEATH                      = 2,
    SAY_MOLGEIM_SUMMON                          = 3,
    SAY_MOLGEIM_DEATH_1                         = 4,
    SAY_MOLGEIM_DEATH_2                         = 5,
    SAY_MOLGEIM_BERSERK                         = 6,
    // Brundir
    SAY_BRUNDIR_AGGRO                           = 0,
    SAY_BRUNDIR_SLAY                            = 1,
    SAY_BRUNDIR_SPECIAL                         = 2,
    SAY_BRUNDIR_FLIGHT                          = 3,
    SAY_BRUNDIR_DEATH_1                         = 4,
    SAY_BRUNDIR_DEATH_2                         = 5,
    SAY_BRUNDIR_BERSERK                         = 6,
};

// ###### Datas ######
enum MovePoints
{
    POINT_FLY       = 1,
    POINT_LAND      = 2,
    POINT_CHASE     = 3,
};

enum Data
{
    DATA_I_CHOOSE_YOU                       = 1,
    DATA_CANT_DO_THAT_WHILE_STUNNED         = 2,
};

enum Actions
{
    ACTION_MATE_DIED                        = 1,
};

enum Phases
{
    PHASE_THREE_ALIVE       = 1,
    PHASE_TWO_ALIVE         = 2,
    PHASE_ONE_ALIVE         = 3,
};

// ###### Event Controlling ######
enum Events
{
    EVENT_ENRAGE                    = 1,
    // Steelbreaker
    EVENT_FUSION_PUNCH              = 2,
    EVENT_STATIC_DISRUPTION         = 3,
    EVENT_OVERWHELMING_POWER        = 4,
    // Molgeim
    EVENT_RUNE_OF_POWER             = 6,
    EVENT_SHIELD_OF_RUNES           = 7,
    EVENT_RUNE_OF_DEATH             = 8,
    EVENT_RUNE_OF_SUMMONING         = 9,
    EVENT_LIGHTNING_BLAST           = 10,
    // Brundir
    EVENT_CHAIN_LIGHTNING           = 12,
    EVENT_OVERLOAD                  = 13,
    EVENT_LIGHTNING_WHIRL           = 14,
    EVENT_LIGHTNING_TENDRILS_START  = 15,
    EVENT_LIGHTNING_TENDRILS_END    = 16,
    EVENT_THREAT_WIPE               = 17,
    EVENT_STORMSHIELD               = 18,
};

enum EventGroups
{
    GROUP_EVENT_BRUNDIR_GROUND      = 1,
    GROUP_EVENT_BRUNDIR_FLY         = 2,
};

// ###### Spells ######
enum Spells
{
    // Any boss
    SPELL_SUPERCHARGE                   = 61920,
    SPELL_BERSERK                       = 47008, // Hard enrage, don't know the correct ID.
    SPELL_CREDIT_MARKER                 = 65195, // spell_dbc
    SPELL_IRON_BOOT_FLASK               = 58501,

    // Steelbreaker
    SPELL_HIGH_VOLTAGE                  = 61890,
    SPELL_FUSION_PUNCH                  = 61903,
    SPELL_STATIC_DISRUPTION             = 61911,
    SPELL_OVERWHELMING_POWER            = 64637,
    SPELL_ELECTRICAL_CHARGE             = 61901,
    SPELL_ELECTRICAL_CHARGE_2           = 61902,

    // Runemaster Molgeim
    SPELL_SHIELD_OF_RUNES               = 62274,
    SPELL_SHIELD_OF_RUNES_BUFF          = 62277,
    SPELL_SUMMON_RUNE_OF_POWER          = 63513,
    SPELL_SUMMON_RUNE_OF_POWER_SELECTOR = 61973,
    SPELL_RUNE_OF_POWER                 = 61974,
    SPELL_RUNE_OF_DEATH                 = 62269,
    SPELL_RUNE_OF_SUMMONING             = 62273, // This is the spell that summons the rune
    SPELL_RUNE_OF_SUMMONING_VIS         = 62019, // Visual
    SPELL_RUNE_OF_SUMMONING_SUMMON      = 62020, // Spell that summons
    SPELL_LIGHTNING_BLAST               = 62054,

    // Stormcaller Brundir
    SPELL_CHAIN_LIGHTNING               = 61879,
    SPELL_CHAIN_LIGHTNING_25            = 63479,
    SPELL_OVERLOAD                      = 61869,
    SPELL_LIGHTNING_WHIRL               = 61915,
    SPELL_LIGHTNING_WHIRL_25            = 63483,
    SPELL_LIGHTNING_TENDRILS            = 61887,
    SPELL_LIGHTNING_TENDRILS_25         = 63486,
    SPELL_STORMSHIELD                   = 64187
};

// ##### Additional Data #####
#define BRUNDIR_FLOOR_HEIGHT            427.28f
#define BRUNDIR_TENDRILS_HEIGHT         435.0f

bool IsEncounterDoneWithInform(InstanceScript* instance, Creature* me)
{
    if (instance->GetBossState(BOSS_ASSEMBLY) != IN_PROGRESS)
        return false;

    bool allDead = true;

    for (uint8 i = 0; i < 3; ++i)
    {
        if (Creature* boss = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_STEELBREAKER + i))))
            if (boss->IsAlive() && boss != me)
            {
                boss->GetAI()->DoAction(ACTION_MATE_DIED);
                allDead = false;
            }
    }

    return allDead;
}

class boss_steelbreaker : public CreatureScript
{
    public:
        boss_steelbreaker() : CreatureScript("boss_steelbreaker") { }

        struct boss_steelbreakerAI : public BossAI
        {
            boss_steelbreakerAI(Creature* creature) : BossAI(creature, BOSS_ASSEMBLY)
            {
                ASSERT(instance);
                clean = false;
            }

            uint8 phase;

            void Reset()
            {
                BossAI::Reset();
                if (Creature* molgeim = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_MOLGEIM))))
                    molgeim->AI()->EnterEvadeMode();
                if (Creature* brundir = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_BRUNDIR))))
                    brundir->AI()->EnterEvadeMode();
            }

            void EnterCombat(Unit* who)
            {
                BossAI::EnterCombat(who);

                clean = false;
                
                if (Creature* molgeim = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_MOLGEIM))))
                    molgeim->AI()->AttackStart(who);
                if (Creature* brundir = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_BRUNDIR))))
                    brundir->AI()->AttackStart(who);

                Talk(SAY_STEELBREAKER_AGGRO);

                me->RemoveLootMode(LOOT_MODE_DEFAULT);

                me->RemoveAurasDueToSpell(SPELL_SUPERCHARGE);

                DoCastSelf(SPELL_HIGH_VOLTAGE);

                phase = PHASE_THREE_ALIVE;

                events.ScheduleEvent(EVENT_ENRAGE,      900*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FUSION_PUNCH, 15*IN_MILLISECONDS);
            }


            uint32 GetData(uint32 type) const
            {
                if (type == DATA_I_CHOOSE_YOU)
                    return (phase == PHASE_ONE_ALIVE);

                return 0;
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    case ACTION_MATE_DIED:
                        {
                            switch (++phase)
                            {
                            case PHASE_TWO_ALIVE:
                                {
                                    events.ScheduleEvent(EVENT_STATIC_DISRUPTION, 30*IN_MILLISECONDS);
                                    break;
                                }
                            case PHASE_ONE_ALIVE:
                                {
                                    me->ResetLootMode();
                                    events.ScheduleEvent(EVENT_OVERWHELMING_POWER, urand(2500,5000));
                                    break;
                                }
                            }

                            me->SetFullHealth();
                            me->AddAura(SPELL_SUPERCHARGE, me);
                            break;
                        }

                    //! Handled over InstanceScript because Melddown kills or similar doesn't count as kill from Steelbreaker
                    case ACTION_ASSEMBLY_PLAYER_DIED:
                        if (phase == PHASE_ONE_ALIVE)
                            DoCastSelf(SPELL_ELECTRICAL_CHARGE, true);
                        break;
                }
            }

            void JustDied(Unit* killer)
            {
                Talk(RAND(SAY_STEELBREAKER_DEATH_1, SAY_STEELBREAKER_DEATH_2));

                if (IsEncounterDoneWithInform(instance, me))
                {
                    DoCastSelf(SPELL_CREDIT_MARKER, true);
                    BossAI::JustDied(killer);
                }

                if (instance->GetData(DATA_ASSEMBLY_DEAD) < 2)
                    me->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE); // Unlootable if dead
                else
                    me->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);

                instance->SetData(DATA_ASSEMBLY_DEAD, 1);
            }

            void KilledUnit(Unit* /*who*/)
            {
                Talk(SAY_STEELBREAKER_SLAY);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                {
                    if (!clean) 
                        if (me->HasAura(SPELL_ELECTRICAL_CHARGE_2))
                        {
                            me->RemoveAura(SPELL_ELECTRICAL_CHARGE_2);
                            clean = true;
                        }
                    return;
                }

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ENRAGE:
                            Talk(SAY_STEELBREAKER_BERSERK);
                            DoCastSelf(SPELL_BERSERK, true);
                            break;
                        case EVENT_FUSION_PUNCH:
                            DoCastVictim(SPELL_FUSION_PUNCH);
                            events.ScheduleEvent(EVENT_FUSION_PUNCH, urand(13*IN_MILLISECONDS, 22*IN_MILLISECONDS));
                            return;
                        case EVENT_STATIC_DISRUPTION:
                        {
                            Unit* target = GetDisruptionTarget();
                            if (!target)
                                target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true);
                            if (target)
                                DoCast(target, SPELL_STATIC_DISRUPTION);
                            events.ScheduleEvent(EVENT_STATIC_DISRUPTION, 20*IN_MILLISECONDS);
                            return;
                        }
                        case EVENT_OVERWHELMING_POWER:
                            Talk(SAY_STEELBREAKER_POWER);

                            DoCastVictim(SPELL_OVERWHELMING_POWER);

                            events.ScheduleEvent(EVENT_OVERWHELMING_POWER, RAID_MODE(63*IN_MILLISECONDS, 38*IN_MILLISECONDS));
                            return;
                    }
                }

                DoMeleeAttackIfReady();
            }

            // try to prefer ranged targets
            Unit* GetDisruptionTarget()
            {
                Map* map = me->GetMap();

                std::list<Player*> playerList;
                for (Player& player : map->GetPlayers())
                {
                    if (!me->IsValidAttackTarget(&player))
                        continue;

                    float distance = player.GetDistance(*me);
                    if (distance < 15.0f || distance > 100.0f)
                        continue;

                    playerList.push_back(&player);
                }

                if (playerList.empty())
                    return NULL;

                std::list<Player*>::const_iterator itr = playerList.begin();
                std::advance(itr, urand(0, playerList.size() - 1));
                return *itr;
            }

        private:
            bool clean;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_steelbreakerAI(creature);
        }
};

class boss_runemaster_molgeim : public CreatureScript
{
    public:
        boss_runemaster_molgeim() : CreatureScript("boss_runemaster_molgeim") { }

        struct boss_runemaster_molgeimAI : public BossAI
        {
            boss_runemaster_molgeimAI(Creature* creature) : BossAI(creature, BOSS_ASSEMBLY)
            {
                ASSERT(instance);
            }

            uint8 phase;

            void Reset()
            {
                BossAI::Reset();

                me->RemoveAurasDueToSpell(SPELL_SUPERCHARGE);

                if (Creature* brundir = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_BRUNDIR))))
                    brundir->AI()->EnterEvadeMode();
                if (Creature* steelbreaker = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_STEELBREAKER))))
                    steelbreaker->AI()->EnterEvadeMode();
            }

            void EnterCombat(Unit* who)
            {
                BossAI::EnterCombat(who);

                if (Creature* steelbreaker = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_STEELBREAKER))))
                    steelbreaker->AI()->AttackStart(who);
                if (Creature* brundir = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_BRUNDIR))))
                    brundir->AI()->AttackStart(who);

                Talk(SAY_MOLGEIM_AGGRO);

                me->RemoveLootMode(LOOT_MODE_DEFAULT);

                phase = PHASE_THREE_ALIVE;

                events.ScheduleEvent(EVENT_ENRAGE,          900*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SHIELD_OF_RUNES,  27*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_RUNE_OF_POWER,    60*IN_MILLISECONDS);
            }

            uint32 GetData(uint32 type) const
            {
                if (type == DATA_I_CHOOSE_YOU)
                    return (phase == PHASE_ONE_ALIVE);

                return 0;
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                case ACTION_MATE_DIED:
                    {
                        switch (++phase)
                        {
                        case PHASE_TWO_ALIVE:
                            {
                                events.ScheduleEvent(EVENT_RUNE_OF_DEATH, 20*IN_MILLISECONDS);
                                break;
                            }
                        case PHASE_ONE_ALIVE:
                            {
                                me->ResetLootMode();
                                events.ScheduleEvent(EVENT_RUNE_OF_SUMMONING, urand(20*IN_MILLISECONDS, 30*IN_MILLISECONDS));
                                break;
                            }
                        }

                        me->SetFullHealth();
                        me->AddAura(SPELL_SUPERCHARGE, me);
                        break;
                    }
                }
            }

            void JustDied(Unit* killer)
            {
                Talk(RAND(SAY_MOLGEIM_DEATH_1, SAY_MOLGEIM_DEATH_2));

                if (IsEncounterDoneWithInform(instance, me))
                {
                    DoCastSelf(SPELL_CREDIT_MARKER, true);
                    BossAI::JustDied(killer);
                }

                if (instance->GetData(DATA_ASSEMBLY_DEAD) < 2)
                    me->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE); // Unlootable if dead
                else
                    me->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);

                instance->SetData(DATA_ASSEMBLY_DEAD, 1);
            }

            void KilledUnit(Unit* /*victim*/)
            {
                Talk(SAY_MOLGEIM_SLAY);
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
                        case EVENT_ENRAGE:
                            Talk(SAY_MOLGEIM_BERSERK);
                            DoCastSelf(SPELL_BERSERK, true);
                            break;
                        case EVENT_RUNE_OF_POWER:
                            {
                                std::list<Creature*> targets;
                                targets.push_back(me);

                                if (Creature* steelbreaker = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_STEELBREAKER))))
                                    if (steelbreaker->IsAlive())
                                        targets.push_back(steelbreaker);

                                if (Creature* brundir = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_BRUNDIR))))
                                    if (brundir->IsAlive())
                                        targets.push_back(brundir);

                                if (Creature* target = Trinity::Containers::SelectRandomContainerElement(targets))
                                    DoCast(target, SPELL_SUMMON_RUNE_OF_POWER_SELECTOR);

                                events.ScheduleEvent(EVENT_RUNE_OF_POWER, urand(35*IN_MILLISECONDS, 45*IN_MILLISECONDS));
                                return;
                            }
                        case EVENT_SHIELD_OF_RUNES:
                            DoCastSelf(SPELL_SHIELD_OF_RUNES);
                            events.ScheduleEvent(EVENT_SHIELD_OF_RUNES, urand(27*IN_MILLISECONDS, 34*IN_MILLISECONDS));
                            return;
                        case EVENT_RUNE_OF_DEATH:
                            Talk(SAY_MOLGEIM_RUNE_DEATH);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                DoCast(target, SPELL_RUNE_OF_DEATH);
                            events.ScheduleEvent(EVENT_RUNE_OF_DEATH, urand(25*IN_MILLISECONDS, 35*IN_MILLISECONDS));
                            break;
                        case EVENT_RUNE_OF_SUMMONING:
                            Talk(SAY_MOLGEIM_SUMMON);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                DoCast(target, SPELL_RUNE_OF_SUMMONING);
                            events.ScheduleEvent(EVENT_RUNE_OF_SUMMONING, urand(35*IN_MILLISECONDS, 45*IN_MILLISECONDS));
                            return;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_runemaster_molgeimAI(creature);
        }
};

class boss_stormcaller_brundir : public CreatureScript
{
    public:
        boss_stormcaller_brundir() : CreatureScript("boss_stormcaller_brundir") { }

        struct boss_stormcaller_brundirAI : public BossAI
        {
            boss_stormcaller_brundirAI(Creature* c) : BossAI(c, BOSS_ASSEMBLY)
            {
                ASSERT(instance);
            }

            uint8 phase;

            void Reset()
            {
                BossAI::Reset();
                if (Creature* molgeim = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_MOLGEIM))))
                    molgeim->AI()->EnterEvadeMode();
                if (Creature* steelbreaker = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_STEELBREAKER))))
                    steelbreaker->AI()->EnterEvadeMode();

                me->SetDisableGravity(false);
                me->SetSpeedRate(MOVE_RUN, me->GetCreatureTemplate()->speed_run);

                me->RemoveAurasDueToSpell(SPELL_SUPERCHARGE);

                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, false);  // Should be interruptable unless overridden by spell (Overload)
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN,      false);  // Reset immumity, Brundir should be stunnable by default
            }

            void EnterCombat(Unit* who)
            {
                BossAI::EnterCombat(who);

                if (Creature* steelbreaker = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_STEELBREAKER))))
                    steelbreaker->AI()->AttackStart(who);
                if (Creature* molgeim = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_MOLGEIM))))
                    molgeim->AI()->AttackStart(who);

                Talk(SAY_BRUNDIR_AGGRO);

                iDidntCastChainLightningOrLightningWhirl = true;

                me->RemoveLootMode(LOOT_MODE_DEFAULT);

                phase = PHASE_THREE_ALIVE;

                events.ScheduleEvent(EVENT_ENRAGE,          900*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, urand( 9*IN_MILLISECONDS, 17*IN_MILLISECONDS), GROUP_EVENT_BRUNDIR_GROUND);
                events.ScheduleEvent(EVENT_OVERLOAD,        urand(60*IN_MILLISECONDS, 80*IN_MILLISECONDS), GROUP_EVENT_BRUNDIR_GROUND);
            }

            uint32 GetData(uint32 type) const
            {
                if (type == DATA_I_CHOOSE_YOU)
                    return (phase == PHASE_ONE_ALIVE);
                else if (type == DATA_CANT_DO_THAT_WHILE_STUNNED)
                    return iDidntCastChainLightningOrLightningWhirl;

                return 0;
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                case ACTION_MATE_DIED:
                    {
                        switch (++phase)
                        {
                        case PHASE_TWO_ALIVE:
                            {
                                events.ScheduleEvent(EVENT_LIGHTNING_WHIRL, urand(12500, 20000), GROUP_EVENT_BRUNDIR_GROUND);
                                break;
                            }
                        case PHASE_ONE_ALIVE:
                            {
                                me->ResetLootMode();

                                DoCastSelf(SPELL_STORMSHIELD, true);
                                events.ScheduleEvent(EVENT_LIGHTNING_TENDRILS_START, urand(40000, 80000), GROUP_EVENT_BRUNDIR_GROUND);
                                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
                                break;
                            }
                        }

                        me->SetFullHealth();
                        me->AddAura(SPELL_SUPERCHARGE, me);
                        break;
                    }
                }
            }

            void JustDied(Unit* killer)
            {
                Talk(RAND(SAY_BRUNDIR_DEATH_1, SAY_BRUNDIR_DEATH_2));

                if (IsEncounterDoneWithInform(instance, me))
                {
                    DoCastSelf(SPELL_CREDIT_MARKER, true);
                    BossAI::JustDied(killer);
                }

                if (instance->GetData(DATA_ASSEMBLY_DEAD) < 2)
                    me->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE); // Unlootable if dead
                else
                    me->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);

                instance->SetData(DATA_ASSEMBLY_DEAD, 1);
            }

            void KilledUnit(Unit* /*victim*/)
            {
                Talk(SAY_BRUNDIR_SLAY);
            }

            void SpellHitTarget(Unit* /*target*/, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_CHAIN_LIGHTNING    ||
                    spell->Id == SPELL_CHAIN_LIGHTNING_25 ||
                    spell->Id == SPELL_LIGHTNING_WHIRL    ||
                    spell->Id == SPELL_LIGHTNING_WHIRL_25)
                {
                    iDidntCastChainLightningOrLightningWhirl = false;
                }
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                switch (id)
                {
                    case POINT_FLY:
                    {
                        me->SetSpeedRate(MOVE_RUN, 0.95f);
                        break;
                    }
                    case POINT_LAND:
                    {
                        me->SetSpeedRate(MOVE_RUN, me->GetCreatureTemplate()->speed_run);

                        me->ApplySpellImmune(0, IMMUNITY_STATE,  SPELL_AURA_MOD_TAUNT,   false);
                        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
                        me->SetReactState(REACT_AGGRESSIVE);

                        me->SetDisableGravity(false);
                        me->GetMotionMaster()->MoveChase(me->GetVictim());
                        break;
                    }
                    case POINT_CHASE:
                    {
                        if (Unit* target = me->GetVictim())
                            me->GetMotionMaster()->MovePoint(POINT_CHASE, target->GetPositionX(), target->GetPositionY(), BRUNDIR_TENDRILS_HEIGHT);
                        break;
                    }
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING) || me->IsNonMeleeSpellCast(true))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ENRAGE:
                            Talk(SAY_BRUNDIR_BERSERK);
                            DoCastSelf(SPELL_BERSERK, true);
                            break;
                        case EVENT_CHAIN_LIGHTNING:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM,0, 0.0f, true))
                                DoCast(SPELL_CHAIN_LIGHTNING);
                            events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, urand(3*IN_MILLISECONDS, 5*IN_MILLISECONDS), GROUP_EVENT_BRUNDIR_GROUND);
                            return;
                        case EVENT_OVERLOAD:
                            DoCast(SPELL_OVERLOAD);
                            events.ScheduleEvent(EVENT_OVERLOAD, urand(60*IN_MILLISECONDS, 80*IN_MILLISECONDS), GROUP_EVENT_BRUNDIR_GROUND);
                            return;
                        case EVENT_LIGHTNING_WHIRL:
                            DoCast(SPELL_LIGHTNING_WHIRL);
                            events.ScheduleEvent(EVENT_LIGHTNING_WHIRL, urand(15*IN_MILLISECONDS, 25*IN_MILLISECONDS), GROUP_EVENT_BRUNDIR_GROUND);
                            return;
                        case EVENT_THREAT_WIPE:
                            ResetThreatList();
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                            {
                                AddThreat(target, 99999.9f);
                                me->GetMotionMaster()->MovePoint(POINT_CHASE, target->GetPositionX(), target->GetPositionY(), BRUNDIR_TENDRILS_HEIGHT);
                            }
                            events.ScheduleEvent(EVENT_THREAT_WIPE, 5*IN_MILLISECONDS, GROUP_EVENT_BRUNDIR_FLY);
                            break;
                        case EVENT_LIGHTNING_TENDRILS_START:
                            me->ApplySpellImmune(0, IMMUNITY_STATE,  SPELL_AURA_MOD_TAUNT,   true);
                            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                            me->SetReactState(REACT_PASSIVE);

                            DoCast(SPELL_LIGHTNING_TENDRILS);
                            me->SetDisableGravity(true);
                            me->SetCanFly(true);
                            me->GetMotionMaster()->MovePoint(POINT_FLY, me->GetPositionX(), me->GetPositionY(), BRUNDIR_TENDRILS_HEIGHT);

                            events.ScheduleEvent(EVENT_LIGHTNING_TENDRILS_END, 35*IN_MILLISECONDS, GROUP_EVENT_BRUNDIR_FLY);
                            events.ScheduleEvent(EVENT_THREAT_WIPE, 5*IN_MILLISECONDS, GROUP_EVENT_BRUNDIR_FLY);
                            events.DelayEvents(35*IN_MILLISECONDS, GROUP_EVENT_BRUNDIR_GROUND);
                            break;
                        case EVENT_LIGHTNING_TENDRILS_END:
                            events.CancelEventGroup(GROUP_EVENT_BRUNDIR_FLY);

                            me->GetMotionMaster()->MovePoint(POINT_LAND, me->GetPositionX(), me->GetPositionY(), BRUNDIR_FLOOR_HEIGHT);
                            me->RemoveAurasDueToSpell(RAID_MODE(SPELL_LIGHTNING_TENDRILS, SPELL_LIGHTNING_TENDRILS_25));

                            events.ScheduleEvent(EVENT_LIGHTNING_TENDRILS_START, urand(40*IN_MILLISECONDS, 80*IN_MILLISECONDS), GROUP_EVENT_BRUNDIR_GROUND);
                            break;
                    }
                }
            }

        private:
            bool iDidntCastChainLightningOrLightningWhirl;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_stormcaller_brundirAI(creature);
        }
};

class npc_rune_of_power : public CreatureScript
{
    public:
        npc_rune_of_power() : CreatureScript("npc_rune_of_power") { }

        struct npc_rune_of_powerAI : public ScriptedAI
        {
            npc_rune_of_powerAI(Creature* creature) : ScriptedAI(creature)
            {
                DoCast(SPELL_RUNE_OF_POWER);
                me->DespawnOrUnsummon(60*IN_MILLISECONDS);
            }


            void UpdateAI(uint32 /*diff*/) {}
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_rune_of_powerAI(creature);
        }
};

class npc_lightning_elemental : public CreatureScript
{
    public:
        npc_lightning_elemental() : CreatureScript("npc_lightning_elemental") { }

        struct npc_lightning_elementalAI : public ScriptedAI
        {
            npc_lightning_elementalAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetReactState(REACT_PASSIVE);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
            }

            void Reset()
            {
                Casted = false;
                me->SetInCombatWithZone();
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                    AttackStart(target);
            }

            void UpdateAI(uint32 /*diff*/)
            {
                if (!UpdateVictim())
                    return;

                if (me->IsWithinMeleeRange(me->GetVictim()) && !Casted)
                {
                    DoCastSelf(SPELL_LIGHTNING_BLAST, true);
                    me->DespawnOrUnsummon(500);
                    Casted = true;
                }
            }

        private:
            bool Casted;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lightning_elementalAI(creature);
        }
};

class npc_rune_of_summoning : public CreatureScript
{
    public:
        npc_rune_of_summoning() : CreatureScript("npc_rune_of_summoning") { }

        struct npc_rune_of_summoningAI : public ScriptedAI
        {
            npc_rune_of_summoningAI(Creature* creature) : ScriptedAI(creature), Instance(me->GetInstanceScript())
            {
                ASSERT(Instance);
            }

            void Reset()
            {
                SummonCount = 0;
                SummonTimer = 5*IN_MILLISECONDS;
                me->AddAura(SPELL_RUNE_OF_SUMMONING_VIS, me);
            }

            void JustSummoned(Creature* summon)
            {
                if (Creature* Molgeim = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(DATA_MOLGEIM))))
                    Molgeim->AI()->JustSummoned(summon);

                if (++SummonCount >= 10)
                    me->DespawnOrUnsummon();
            }

            void UpdateAI(uint32 diff)
            {
                if (SummonTimer <= diff)
                {
                    DoCast(SPELL_RUNE_OF_SUMMONING_SUMMON);
                    SummonTimer = 2000;
                }
                else
                    SummonTimer -= diff;
            }

        private:
            InstanceScript* Instance;
            uint32 SummonCount;
            uint32 SummonTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_rune_of_summoningAI(creature);
        }
};

class spell_shield_of_runes : public SpellScriptLoader
{
    public:
        spell_shield_of_runes() : SpellScriptLoader("spell_shield_of_runes") { }

        class spell_shield_of_runes_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_shield_of_runes_AuraScript);

            void HandleAfterEffectAbsorb(AuraEffect* aurEff, DamageInfo& /*dmgInfo*/, uint32& absorbAmount)
            {
                if (int32(absorbAmount) >= aurEff->GetAmount())
                    if (Unit* owner = GetUnitOwner())
                        owner->CastSpell(owner, SPELL_SHIELD_OF_RUNES_BUFF, true);
            }

            void Register()
            {
                 AfterEffectAbsorb += AuraEffectAbsorbFn(spell_shield_of_runes_AuraScript::HandleAfterEffectAbsorb, EFFECT_0);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_shield_of_runes_AuraScript();
        }
};

class achievement_i_choose_you : public AchievementCriteriaScript
{
    public:
        achievement_i_choose_you() : AchievementCriteriaScript("achievement_i_choose_you") {}

        bool OnCheck(Player* /*player*/, Unit* target)
        {
            if (!target)
                return false;

            if (Creature* boss = target->ToCreature())
                if (boss->AI()->GetData(DATA_I_CHOOSE_YOU))
                    return true;

            return false;
        }
};

class achievement_but_i_am_on_your_side : public AchievementCriteriaScript
{
    public:
        achievement_but_i_am_on_your_side() : AchievementCriteriaScript("achievement_but_i_am_on_your_side") {}

        bool OnCheck(Player* player, Unit* target)
        {
            if (!target || !player)
                return false;

            if (Creature* boss = target->ToCreature())
                if (boss->AI()->GetData(DATA_I_CHOOSE_YOU) && player->HasAura(SPELL_IRON_BOOT_FLASK))
                    return true;

            return false;
        }
};

class achievement_cant_do_that_while_stunned : public AchievementCriteriaScript
{
    public:
        achievement_cant_do_that_while_stunned() : AchievementCriteriaScript("achievement_cant_do_that_while_stunned") {}

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (!target)
                return false;

            InstanceScript* instance = target->GetInstanceScript();
            if (!instance)
                return false;

            if (Unit* brundir = ObjectAccessor::GetCreature(*target, ObjectGuid(instance->GetGuidData(DATA_BRUNDIR))))
                if (brundir->GetAI()->GetData(DATA_CANT_DO_THAT_WHILE_STUNNED))
                {
                    for (uint8 i = 0; i < 3; ++i)
                        if (Creature* boss = ObjectAccessor::GetCreature(*target, ObjectGuid(instance->GetGuidData(DATA_STEELBREAKER + i))))
                        {
                            if (boss != target->ToCreature())
                                if (boss->IsAlive())
                                    return false;
                        }
                        else
                            return false;

                    return true;
                }

            return false;
        }
};

void AddSC_boss_assembly_of_iron()
{
    new boss_steelbreaker();
    new boss_runemaster_molgeim();
    new boss_stormcaller_brundir();

    new npc_lightning_elemental();
    new npc_rune_of_summoning();
    new npc_rune_of_power();

    new spell_shield_of_runes();

    new achievement_i_choose_you();
    new achievement_but_i_am_on_your_side();
    new achievement_cant_do_that_while_stunned();
}
