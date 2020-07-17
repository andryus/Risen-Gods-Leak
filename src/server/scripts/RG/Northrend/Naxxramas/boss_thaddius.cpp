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
#include "Player.h"
#include "naxxramas.h"
#include "GridNotifiers.h"
#include "SpellAuras.h"

//Stalagg
enum StalaggYells
{
    SAY_STAL_AGGRO          = 0,
    SAY_STAL_SLAY           = 1,
    SAY_STAL_DEATH          = 2
};

enum StalagSpells
{
    SPELL_POWERSURGE                    = 28134,
    H_SPELL_POWERSURGE                  = 54529,
    SPELL_MAGNETIC_PULL                 = 28338,
    SPELL_STALAGG_TESLA                 = 28097
};

//Feugen
enum FeugenYells
{
    SAY_FEUG_AGGRO          = 0,
    SAY_FEUG_SLAY           = 1,
    SAY_FEUG_DEATH          = 2
};

enum FeugenSpells
{
    SPELL_STATICFIELD                   = 28135,
    H_SPELL_STATICFIELD                 = 54528,
    SPELL_FEUGEN_TESLA                  = 28109
};

// Thaddius DoAction
enum ThaddiusActions
{
    ACTION_FEUGEN_RESET,
    ACTION_FEUGEN_DIED,
    ACTION_STALAGG_RESET,
    ACTION_STALAGG_DIED
};

enum Summons
{
    NPC_TESLA_COIL                      = 16218,
    GO_TESLA                            = 181477
};

#define EMOTE_POLARITY_SHIFT "Die Polaritaet hat sich verschoben!"
#define EMOTE_TESLA_OVERLOAD "Teslaspule Ueberlaed!"

//Tesla Spells (deals damage to the group if Stalagg or Feugen are too far away)
enum TeslaSpells
{
    SPELL_SHOCK                         = 28099,
    SPELL_STALAGG_TESLA_PASSIVE         = 28097,
    SPELL_FEUGEN_TESLA_PASSIVE          = 28109,
    SPELL_THADDIUS_CHANNEL              = 45537
};

//Thaddius
enum ThaddiusYells
{
    SAY_GREET               = 0,
    SAY_AGGRO               = 1,
    SAY_SLAY                = 2,
    SAY_ELECT               = 3,
    SAY_DEATH               = 4,
    SAY_SCREAM              = 5
};

enum ThaddiusSpells
{
    SPELL_BALL_LIGHTNING                = 28299,
    SPELL_CHAIN_LIGHTNING               = 28167,
    H_SPELL_CHAIN_LIGHTNING             = 54531,
    SPELL_BERSERK                       = 27680,

    SPELL_POLARITY_SHIFT                = 28089,
    SPELL_POLATITY_SHIFT_25             = 39096,

    SPELL_POSITIV_DAMAGE                = 28062,
    SPELL_NEGATIV_DAMAGE                = 28085,
    SPELL_POSITIV_DAMAGE_25             = 39090,
    SPELL_NEGATIV_DAMAGE_25             = 39093,

    SPELL_POSITIV_STACK                 = 29659,
    SPELL_NEGATIV_STACK                 = 29660,
    SPELL_POSITIV_STACK_25              = 39089, // Not used, +100% DMG
    SPELL_NEGATIV_STACK_25              = 39092, // Not used, +100% DMG

    SPELL_POSITIV_AURA                  = 28059,
    SPELL_NEGATIV_AURA                  = 28084,
    SPELL_POSITIV_AURA_25               = 39088,
    SPELL_NEGATIV_AURA_25               = 39091,

    SPELL_THADDIUS_SPAWN                = 28160
};

enum Events
{
    EVENT_SHIFT = 1,
    EVENT_CHAIN,
    EVENT_BERSERK,
    EVENT_INTRO_2 = 5
};

enum Achievement
{
    ACHIEVEMENT_SCHOCKING_10            = 2178,
    ACHIEVEMENT_SCHOCKING_25            = 2179,
};

enum PhasesThaddius
{
    PHASE_INTRO     = 0
};

class boss_thaddius : public CreatureScript
{
public:
    boss_thaddius() : CreatureScript("boss_thaddius") { }

    struct boss_thaddiusAI : public BossAI
    {
        boss_thaddiusAI(Creature* creature) : BossAI(creature, BOSS_THADDIUS),
            _introDone(false)
        {
            // init is a bit tricky because thaddius shall track the life of both adds, but not if there was a wipe
            // and, in particular, if there was a crash after both adds were killed (should not respawn)

            // Moreover, the adds may not yet be spawn. So just track down the status if mob is spawn
            // and each mob will send its status at reset (meaning that it is alive)
            checkFeugenAlive = false;
            if (Creature* Feugen = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_FEUGEN))))
                checkFeugenAlive = Feugen->IsAlive();

            checkStalaggAlive = false;
            if (Creature* Stalagg = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_STALAGG))))
                checkStalaggAlive = Stalagg->IsAlive();

            if (!checkFeugenAlive && !checkStalaggAlive)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                me->SetImmuneToPC(false);
                me->SetReactState(REACT_AGGRESSIVE);
            }
            else
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                me->SetImmuneToPC(true);
                me->SetReactState(REACT_PASSIVE);
            }
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);

            if (GameObject* tesla = me->SummonGameObject(GO_TESLA, 3487.76f, -2911.199f, 319.406f, 3.909f, 0.0f, 0.0f, 0.0f, 0.0f, WEEK*IN_MILLISECONDS))
                TeslaGuid[0] = tesla->GetGUID();
            if (GameObject* tesla = me->SummonGameObject(GO_TESLA, 3527.81f, -2952.379f, 319.325f, 3.909f, 0.0f, 0.0f, 0.0f, 0.0f, WEEK*IN_MILLISECONDS))
                TeslaGuid[1] = tesla->GetGUID();
        }

        bool checkStalaggAlive;
        bool checkFeugenAlive;
        bool Fight;
        uint32 AddsTimer;
        uint32 FightStartTimer;
        ObjectGuid TeslaGuid[2];

        void MoveInLineOfSight(Unit* who)
        {
           if (!_introDone && me->IsWithinDistInMap(who, 100.0f))
                {
                    _introDone = true;
                    Talk(SAY_GREET);
                    events.SetPhase(PHASE_INTRO);
                    events.ScheduleEvent(EVENT_INTRO_2, 10*IN_MILLISECONDS, 0, PHASE_INTRO);
                }
            }
        
        void Reset()
        {
            BossAI::Reset();
            Fight = false;
            FightStartTimer = 5000;

            if (instance)
            {
                if (Creature* Stalagg = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_STALAGG))))
                {
                    Stalagg->Respawn();
                    Position homePos = Stalagg->GetHomePosition();
                    Stalagg->NearTeleportTo(homePos.m_positionX, homePos.m_positionY, homePos.m_positionZ, homePos.m_orientation);
                }
                if (Creature* Feugen = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_FEUGEN))))
                {
                    Feugen->Respawn();
                    Position homePos = Feugen->GetHomePosition();
                    Feugen->NearTeleportTo(homePos.m_positionX, homePos.m_positionY, homePos.m_positionZ, homePos.m_orientation);
                }

                instance->SetData(DATA_SHOCKING, 0);
            }
        }

        void SpellHitTarget(Unit* who, SpellInfo const* spell)
        {
            if ((who && who->GetTypeId() == TYPEID_PLAYER)
                && (spell->Id == SPELL_POSITIV_DAMAGE || spell->Id == SPELL_NEGATIV_DAMAGE))
                instance->SetData(DATA_SHOCKING, 1);
        }

        void KilledUnit(Unit* victim)
        {
            if (!(rand32()%5))
                Talk(SAY_SLAY);
        }

        void JustDied(Unit* killer)
        {
            BossAI::JustDied(killer);
            Talk(SAY_DEATH);
            if(me->GetInstanceScript())
                me->GetInstanceScript()->SetBossState(BOSS_THADDIUS, DONE);

            if (me->GetInstanceScript()->GetData(DATA_SHOCKING) == 0)
            {
                if (RAID_MODE(0, 1))
                    me->GetInstanceScript()->DoCompleteAchievement(ACHIEVEMENT_SCHOCKING_25);
                else
                    me->GetInstanceScript()->DoCompleteAchievement(ACHIEVEMENT_SCHOCKING_10);
            }
        }

        void DoAction(int32 action)
        {
            switch(action)
            {
            case ACTION_FEUGEN_RESET:
                checkFeugenAlive = true;
                break;
            case ACTION_FEUGEN_DIED:
                checkFeugenAlive = false;
                break;
            case ACTION_STALAGG_RESET:
                checkStalaggAlive = true;
                break;
            case ACTION_STALAGG_DIED:
                checkStalaggAlive = false;
                break;
            }

            if (!checkFeugenAlive && !checkStalaggAlive)
            {
                if (Creature* tesla = me->SummonCreature(NPC_TESLA_COIL, 3487.76f, -2911.199f, 319.406f, 3.909f, TEMPSUMMON_TIMED_DESPAWN, 5000))
                    tesla->CastSpell(me, SPELL_THADDIUS_CHANNEL, true);
                if (Creature* tesla = me->SummonCreature(NPC_TESLA_COIL, 3527.81f, -2952.379f, 319.325f, 3.909f, TEMPSUMMON_TIMED_DESPAWN, 5000))
                    tesla->CastSpell(me, SPELL_THADDIUS_CHANNEL, true);
                FightStartTimer = 5000;
                Fight = true;
            }
            else
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                me->SetImmuneToPC(true);
                me->SetReactState(REACT_PASSIVE);
            }
        }

        void EnterCombat(Unit* who)
        {
            BossAI::EnterCombat(who);
            Talk(SAY_AGGRO);
            events.ScheduleEvent(EVENT_SHIFT, 15000);
            events.ScheduleEvent(EVENT_CHAIN, urand(10000, 20000));
            events.ScheduleEvent(EVENT_BERSERK, 360000);
        }

        void UpdateAI(uint32 diff)
        {
            if ((!UpdateVictim() && !events.IsInPhase(PHASE_INTRO)))
            return;

            if (Fight)
            {
                if (FightStartTimer <= diff)
                {
                    me->RemoveAura(SPELL_THADDIUS_SPAWN);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                    me->SetImmuneToPC(false);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->MonsterTextEmote(EMOTE_TESLA_OVERLOAD, 0, true);
                    me->SetInCombatWithZone();
                    Fight = false;
                }
                else FightStartTimer -= diff;
            }

            if (checkFeugenAlive && checkStalaggAlive)
                AddsTimer = 0;

            if (checkStalaggAlive != checkFeugenAlive)
            {
                AddsTimer += diff;
                if (AddsTimer > 5000)
                {
                    if (!checkStalaggAlive)
                    {
                        if (instance)
                            if (Creature* Stalagg = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_STALAGG))))
                            {
                                Stalagg->Respawn();
                                Position homePos = Stalagg->GetHomePosition();
                                Stalagg->NearTeleportTo(homePos.m_positionX, homePos.m_positionY, homePos.m_positionZ, homePos.m_orientation);
                            }
                    }
                    else
                    {
                        if (instance)
                            if (Creature* Feugen = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_FEUGEN))))
                            {
                                Feugen->Respawn();
                                Position homePos = Feugen->GetHomePosition();
                                Feugen->NearTeleportTo(homePos.m_positionX, homePos.m_positionY, homePos.m_positionZ, homePos.m_orientation);
                            }
                    }
                }
            }

            if (!UpdateVictim())
                return;

            events.Update(diff);


            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                case EVENT_INTRO_2:
                    Talk(SAY_SCREAM);
                    events.ScheduleEvent(EVENT_INTRO_2, 10*IN_MILLISECONDS, 0, PHASE_INTRO);
                    break;
                case EVENT_SHIFT:
                    DoCastAOE(SPELL_POLARITY_SHIFT);
                    events.ScheduleEvent(EVENT_SHIFT, 30000);
                    return;
                case EVENT_CHAIN:
                    DoCastVictim(RAID_MODE(SPELL_CHAIN_LIGHTNING, H_SPELL_CHAIN_LIGHTNING));
                    Talk(SAY_ELECT);
                    events.ScheduleEvent(EVENT_CHAIN, urand(10000, 20000));
                    return;
                case EVENT_BERSERK:
                    DoCastSelf(SPELL_BERSERK);
                    return;
                }
            }

            if (events.GetTimer() > 15000 && !me->IsWithinMeleeRange(me->GetVictim()))
                DoCastVictim(SPELL_BALL_LIGHTNING);
            else
                DoMeleeAttackIfReady();
        }
      private: 
          bool _introDone;

   };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_thaddiusAI (creature);
    }
};

class npc_stalagg : public CreatureScript
{
public:
    npc_stalagg() : CreatureScript("npc_stalagg") { }

    struct npc_stalaggAI : public ScriptedAI
    {
        npc_stalaggAI(Creature* creature) : ScriptedAI(creature)
        {
            Instance = creature->GetInstanceScript();
            if (Creature* tesla = me->SummonCreature(NPC_TESLA_COIL, 3487.76f, -2911.199f, 319.406f, 3.909f, TEMPSUMMON_MANUAL_DESPAWN))
                TeslaGuid = tesla->GetGUID();
        }
        
        void Reset()
        {
            if (Instance)
                if (Creature* Thaddius = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(DATA_THADDIUS))))
                    if (Thaddius->AI())
                        Thaddius->AI()->DoAction(ACTION_STALAGG_RESET);

            PowerSurgeTimer = urand(20000, 25000);
            MagneticPullTimer = 20000;
            IdleTimer = 3000;
            ShockTimer = 1000;

            Switch = false;
            homePosition = me->GetHomePosition();
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(SAY_STAL_AGGRO);
            DoCast(SPELL_STALAGG_TESLA);
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(SAY_STAL_DEATH);
            if (Instance)
                if (Creature* Thaddius = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(DATA_THADDIUS))))
                    if (Thaddius->AI())
                        Thaddius->AI()->DoAction(ACTION_STALAGG_DIED);
        }

        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim())
                return;

            if (MagneticPullTimer <= uiDiff)
            {
                if (Creature* Feugen = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(DATA_FEUGEN))))
                {
                    Unit* StalaggVictim = me->GetVictim();
                    Unit* FeugenVictim = Feugen->GetVictim();

                    if (FeugenVictim && StalaggVictim)
                    {
                        // magnetic pull is not working. So just jump.
                        // reset aggro to be sure that feugen will not follow the jump
                        // switch aggro, tank gets generated thread from the other tank
                        float TempThreat = Feugen->GetThreatManager().getThreat(FeugenVictim);
                        Feugen->GetThreatManager().ModifyThreatByPercent(FeugenVictim, -100);
                        FeugenVictim->GetMotionMaster()->MoveJump(*me, 40, 20);
                        AddThreat(StalaggVictim, TempThreat, Feugen);
                        Feugen->SetReactState(REACT_PASSIVE);

                        TempThreat = me->GetThreatManager().getThreat(StalaggVictim);
                        me->GetThreatManager().ModifyThreatByPercent(StalaggVictim, -100);
                        StalaggVictim->GetMotionMaster()->MoveJump(*Feugen, 40, 20);
                        AddThreat(FeugenVictim, TempThreat);
                        me->SetReactState(REACT_PASSIVE);
                        IdleTimer = 3000;
                        Switch = true;
                    }
                }

                MagneticPullTimer = 20000;
            }
            else MagneticPullTimer -= uiDiff;

            if (PowerSurgeTimer <= uiDiff)
            {
                DoCastSelf(RAID_MODE(SPELL_POWERSURGE, H_SPELL_POWERSURGE));
                PowerSurgeTimer = urand(15000, 20000);
            } else PowerSurgeTimer -= uiDiff;

            if (Switch)
            {
                if (IdleTimer <= uiDiff)
                {
                    if (Creature* Feugen = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(DATA_FEUGEN))))
                        Feugen->SetReactState(REACT_AGGRESSIVE);
                    me->SetReactState(REACT_AGGRESSIVE);
                    Switch = false;
                }
                else IdleTimer -= uiDiff;
            }

            if (me->GetDistance(homePosition) > 15)
            {
                if (ShockTimer <= uiDiff)
                {
                    if (Creature *Tesla = ObjectAccessor::GetCreature(*me, TeslaGuid))
                        if (Unit *Target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50000, true))
                            Tesla->CastSpell(Target, SPELL_SHOCK, true);
                    ShockTimer = 1000;
                }else ShockTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
        private:
            InstanceScript* Instance;
            uint32 PowerSurgeTimer;
            uint32 MagneticPullTimer;
            uint32 ShockTimer;
            uint32 IdleTimer;
            ObjectGuid TeslaGuid;
            bool Switch;
            Position homePosition;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_stalaggAI(creature);
    }
};

class npc_feugen : public CreatureScript
{
public:
    npc_feugen() : CreatureScript("npc_feugen") { }

    struct npc_feugenAI : public ScriptedAI
    {
        npc_feugenAI(Creature* creature) : ScriptedAI(creature)
        {
            Instance = creature->GetInstanceScript();
            if (Creature* tesla = me->SummonCreature(NPC_TESLA_COIL, 3527.810f, -2952.379f, 319.325f, 3.909f, TEMPSUMMON_MANUAL_DESPAWN))
                TeslaGuid = tesla->GetGUID();
        }
       
        void Reset()
        {
            if (Instance)
                if (Creature* Thaddius = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(DATA_THADDIUS))))
                    if (Thaddius->AI())
                        Thaddius->AI()->DoAction(ACTION_FEUGEN_RESET);

            staticFieldTimer = 5000;
            ShockTimer = 1000;
            homePosition = me->GetHomePosition();
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(SAY_FEUG_AGGRO);
            DoCast(SPELL_FEUGEN_TESLA);
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(SAY_FEUG_DEATH);
            if (Instance)
                if (Creature* Thaddius = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(DATA_THADDIUS))))
                    if (Thaddius->AI())
                        Thaddius->AI()->DoAction(ACTION_FEUGEN_DIED);
        }

        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim())
                return;

            if (staticFieldTimer <= uiDiff)
            {
                DoCastSelf(RAID_MODE(SPELL_STATICFIELD, H_SPELL_STATICFIELD));
                staticFieldTimer = 5000;
            } else staticFieldTimer -= uiDiff;

            if (me->GetDistance(homePosition) > 15)
            {
                if (ShockTimer <= uiDiff)
                {
                    if (Creature *Tesla = ObjectAccessor::GetCreature(*me, TeslaGuid))
                        if (Unit *Target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50000, true))
                            Tesla->CastSpell(Target, SPELL_SHOCK, false);
                    ShockTimer = 1000;
                }else ShockTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    private:
        InstanceScript* Instance;
        uint32 staticFieldTimer;
        uint32 ShockTimer;
        ObjectGuid TeslaGuid;
        Position homePosition;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_feugenAI(creature);
    }
};

class spell_polarity_shift : public SpellScriptLoader
{
public:
    spell_polarity_shift() : SpellScriptLoader("spell_polarity_shift") { }

    class spell_polarity_shift_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_polarity_shift_SpellScript);

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* target = GetHitUnit();
            GetCaster()->MonsterTextEmote(EMOTE_POLARITY_SHIFT, target, true);

            if (GetSpellInfo()->Id == SPELL_POLARITY_SHIFT)
                target->CastSpell(target, roll_chance_i(50) ? SPELL_POSITIV_AURA : SPELL_NEGATIV_AURA, true);
            else
                target->CastSpell(target, roll_chance_i(50) ? SPELL_POSITIV_AURA_25 : SPELL_NEGATIV_AURA_25, true);
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_polarity_shift_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_polarity_shift_SpellScript();
    }
};

class spell_polarity_shift_damage : public SpellScriptLoader
{
public:
    spell_polarity_shift_damage() : SpellScriptLoader("spell_polarity_shift_damage") { }

    class spell_polarity_shift_damage_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_polarity_shift_damage_SpellScript);

        bool Load()
        {
            switch (GetSpellInfo()->Id)
            {
                case SPELL_POSITIV_DAMAGE:
                    StackSpell = SPELL_POSITIV_STACK;
                    AuraSpell = SPELL_POSITIV_AURA;
                    break;
                case SPELL_POSITIV_DAMAGE_25:
                    StackSpell = SPELL_POSITIV_STACK;
                    AuraSpell = SPELL_POSITIV_AURA_25;
                    break;
                case SPELL_NEGATIV_DAMAGE:
                    StackSpell = SPELL_NEGATIV_STACK;
                    AuraSpell = SPELL_NEGATIV_AURA;
                    break;
                case SPELL_NEGATIV_DAMAGE_25:
                    StackSpell = SPELL_NEGATIV_STACK;
                    AuraSpell = SPELL_NEGATIV_AURA_25;
                    break;
            }
            StackCount = 0;
            return true;
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            uint32 before = targets.size();
            targets.remove_if(Trinity::UnitAuraCheck(true, AuraSpell));
            StackCount = before - targets.size();
        }

        void ChangeStack()
        {
            Unit* caster = GetCaster();
            if (StackCount != 0)
            {
                if (!caster->HasAura(StackSpell))
                    caster->CastSpell(caster, StackSpell, true);

                if (Aura* aura = caster->GetAura(StackSpell))
                    aura->SetStackAmount(StackCount);
            }
            else
                caster->RemoveAurasDueToSpell(StackSpell);
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_polarity_shift_damage_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ALLY);
            AfterCast += SpellCastFn(spell_polarity_shift_damage_SpellScript::ChangeStack);
        }

    private:
        uint32 AuraSpell;
        uint32 StackSpell;
        uint32 StackCount;
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_polarity_shift_damage_SpellScript();
    }
};

void AddSC_boss_thaddius()
{
    new boss_thaddius();
    new npc_stalagg();
    new npc_feugen();
    new spell_polarity_shift();
    new spell_polarity_shift_damage();
}
