/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2012-2015 Rising-Gods <http://www.rising-gods.de/>
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
#include "ScriptedEscortAI.h"
#include "Vehicle.h"
#include "trial_of_the_champion.h"
#include "SpellScript.h"
#include "Player.h"

enum Spells
{
    // intro
    SPELL_RISE_JAEREN       = 67715,
    SPELL_RISE_ARELAS       = 67705,
    SPELL_DEATH_RESPITE_VIS = 66798,

    // phase 1
    SPELL_PLAGUE_STRIKE     = 67724,
    SPELL_ICY_TOUCH         = 67718,
    SPELL_DEATH_RESPITE     = 67745,
    SPELL_DEATH_RESPITE_EFF = 67731,
    SPELL_OBLITERATE        = 67725,

    // phase 2 - During this phase, the Black Knight will use the same abilities as in phase 1, except for Death's Respite
    SPELL_ARMY_DEAD         = 67761,
    SPELL_DESECRATION       = 68766,
    SPELL_GHOUL_EXPLODE     = 67751,

    // phase 3
    SPELL_DEATH_BITE        = 67808,
    SPELL_MARKED_DEATH      = 67882,

    // misc
    SPELL_BLACK_KNIGHT_RES  = 67693,
    SPELL_KILL_CREDIT       = 68663,

    // ghul
    SPELL_LEAP              = 67749    
};

enum Models
{
    MODEL_SKELETON = 29846,
    MODEL_GHOST    = 21300
};

enum Says
{
    // black knight
    SAY_INTRO_1             = 0,
    SAY_INTRO_2             = 1,
    SAY_INTRO_3             = 2,
    SAY_AGGRO               = 3,
    SAY_SKELETON            = 4,
    SAY_GHOST               = 5,
    SAY_SLAY                = 6,
    SAY_DEATH               = 7,

    // ghul
    SAY_ANNOUNCER_GHUL      = 0
};

enum Phases
{
    PHASE_UNDEAD    = 1,
    PHASE_SKELETON  = 2,
    PHASE_GHOST     = 3,
    PHASE_NEXT_PHASE,
};

enum Events
{
    EVENT_PLAGUE_STRIKE     = 1,
    EVENT_ICY_TOUCH,
    EVENT_DEATH_RESPITE,
    EVENT_OBLITERATE,
    EVENT_DESECRATION,
    EVENT_DEATH_ARMY,
    EVENT_GHOUL_EXPLODE,
    EVENT_DEATH_BITE,
    EVENT_MARKED_DEATH,

    EVENT_NEXT_PHASE,
};

enum Actions
{
    ACTION_I_HAVE_HAD_WORSE_FAIL    = 1
};

static Position Location[] =
{
    { 734.773f, 591.878f, 411.725f, 0 }
};

class boss_black_knight : public CreatureScript
{
public:
    boss_black_knight() : CreatureScript("boss_black_knight") { }

    struct boss_black_knightAI : public BossAI
    {
        boss_black_knightAI(Creature* creature) : BossAI(creature, BOSS_BLACK_KNIGHT)
        {
            if (instance)
                instance->SetData(DATA_BLACK_KNIGHT_SPAWNED, 0);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetImmuneToAll(true);
            me->SetReactState(REACT_PASSIVE);
        }

        void Reset()
        {
            BossAI::Reset();

            me->SetDisplayId(me->GetNativeDisplayId());
            me->ClearUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED);
            SetEquipmentSlots(false, ITEM_BLACK_KNIGHT, EQUIP_UNEQUIP, EQUIP_UNEQUIP);
            Phase = PHASE_UNDEAD;
            IHaveHadWorse = true;
            defeated = false;
        }

        void EnterCombat(Unit* who)
        {
            BossAI::EnterCombat(who);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetImmuneToAll(false);
            SetPhase(PHASE_UNDEAD);
            Talk(SAY_AGGRO);
            summons.DoZoneInCombat();
        }

        void JustDied(Unit* killer)
        {
            BossAI::JustDied(killer);

            DoCastSelf(SPELL_KILL_CREDIT);
            Talk(SAY_DEATH);
        }

        void JustReachedHome()
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetImmuneToAll(false);
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;

            Talk(SAY_SLAY);
        }

        void DoAction(int32 action)
        {
            if (action == ACTION_I_HAVE_HAD_WORSE_FAIL)
                IHaveHadWorse = false;
        }

        uint32 GetData(uint32 /*id*/) const
        {
            return (uint32)IHaveHadWorse;
        }

        void DamageTaken(Unit* /*who*/, uint32& Damage)
        {
            // sometimes DamageTaken is called several times
            if (defeated)
                return;

            if (Damage > me->GetHealth() && Phase < PHASE_GHOST)
            {
                defeated = true;
                Damage = 0;
                me->SetHealth(0);
                me->AddUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED);
                me->SetImmuneToAll(true);
                me->SetReactState(REACT_PASSIVE);
                me->InterruptNonMeleeSpells(false);
                summons.DespawnAll();
                SetPhase(PHASE_NEXT_PHASE);
            }
        }

        void SetPhase(uint8 _phase)
        {
            events.Reset();
            switch (_phase)
            {
                case PHASE_UNDEAD:
                    events.RescheduleEvent(EVENT_PLAGUE_STRIKE, urand(12, 15)*IN_MILLISECONDS);
                    events.RescheduleEvent(EVENT_ICY_TOUCH, urand(5, 7)*IN_MILLISECONDS);
                    events.RescheduleEvent(EVENT_OBLITERATE, urand (17, 20)*IN_MILLISECONDS);
                    events.RescheduleEvent(EVENT_DEATH_RESPITE, urand(10, 16)*IN_MILLISECONDS);
                    events.RescheduleEvent(EVENT_GHOUL_EXPLODE, 15*IN_MILLISECONDS);
                    break;
                case PHASE_SKELETON:
                    events.RescheduleEvent(EVENT_DEATH_ARMY, 0);
                    events.RescheduleEvent(EVENT_PLAGUE_STRIKE, urand(12, 15)*IN_MILLISECONDS);
                    events.RescheduleEvent(EVENT_ICY_TOUCH, urand(5, 7)*IN_MILLISECONDS);
                    events.RescheduleEvent(EVENT_OBLITERATE, urand (17, 20)*IN_MILLISECONDS);
                    events.RescheduleEvent(EVENT_DESECRATION, urand(12, 16)*IN_MILLISECONDS);
                    events.RescheduleEvent(EVENT_GHOUL_EXPLODE, 8*IN_MILLISECONDS);
                    me->SetDisplayId(MODEL_SKELETON);
                    Talk(SAY_SKELETON);
                    break;
                case PHASE_GHOST:
                    events.RescheduleEvent(EVENT_DEATH_BITE, urand(2, 4)*IN_MILLISECONDS);
                    events.RescheduleEvent(EVENT_MARKED_DEATH, urand(5, 7)*IN_MILLISECONDS);
                    me->SetDisplayId(MODEL_GHOST);
                    SetEquipmentSlots(false, EQUIP_UNEQUIP, EQUIP_UNEQUIP, EQUIP_UNEQUIP);
                    Talk(SAY_GHOST);
                    break;
                case PHASE_NEXT_PHASE:
                    events.RescheduleEvent(EVENT_NEXT_PHASE, 4*IN_MILLISECONDS);
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_NEXT_PHASE:
                        Phase++;
                        SetPhase(Phase);
                        me->SetFullHealth();
                        DoCastSelf(SPELL_BLACK_KNIGHT_RES, true);
                        me->ClearUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED);
                        me->SetImmuneToAll(false);
                        me->SetReactState(REACT_AGGRESSIVE);
                        defeated = false;
                        break;

                    case EVENT_PLAGUE_STRIKE:
                        DoCastVictim(SPELL_PLAGUE_STRIKE);
                        events.RescheduleEvent(EVENT_PLAGUE_STRIKE, urand(12, 15)*IN_MILLISECONDS);
                        break;
                    case EVENT_ICY_TOUCH:
                        DoCastVictim(SPELL_ICY_TOUCH);
                        events.RescheduleEvent(EVENT_ICY_TOUCH, urand(5, 7)*IN_MILLISECONDS);
                        break;
                    case EVENT_OBLITERATE:
                        DoCastVictim(SPELL_OBLITERATE);
                        events.RescheduleEvent(EVENT_OBLITERATE, urand (17, 20)*IN_MILLISECONDS);
                        break;
                    case EVENT_DEATH_RESPITE:
                        DoCastVictim(SPELL_DEATH_RESPITE);
                        events.RescheduleEvent(EVENT_DEATH_RESPITE, urand(10, 16)*IN_MILLISECONDS);
                        break;
                    case EVENT_DEATH_ARMY:
                        DoCastSelf(SPELL_ARMY_DEAD);
                        break;
                    case EVENT_DESECRATION:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                            DoCast(target, SPELL_DESECRATION);
                        events.RescheduleEvent(EVENT_DESECRATION, urand(12, 16)*IN_MILLISECONDS);
                        break;
                    case EVENT_GHOUL_EXPLODE:
                        DoCastSelf(SPELL_GHOUL_EXPLODE);
                        if (Phase == PHASE_SKELETON && me->FindNearestCreature(35590, 100.0f, true))
                            events.RescheduleEvent(EVENT_GHOUL_EXPLODE, 8*IN_MILLISECONDS);
                        break;
                    case EVENT_DEATH_BITE:
                        DoCastAOE(SPELL_DEATH_BITE);
                        events.RescheduleEvent(EVENT_DEATH_BITE, urand(2, 4)*IN_MILLISECONDS);
                        break;
                    case EVENT_MARKED_DEATH:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                            DoCast(target, SPELL_MARKED_DEATH);
                        events.RescheduleEvent(EVENT_MARKED_DEATH, urand(5, 7)*IN_MILLISECONDS);
                        break;

                }
            }
            DoMeleeAttackIfReady();
        }

    private:
        bool IHaveHadWorse;
        uint8 Phase;
        bool defeated;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_black_knightAI (creature);
    }
};

class npc_risen_ghoul : public CreatureScript
{
public:
    npc_risen_ghoul() : CreatureScript("npc_risen_ghoul") { }

    struct npc_risen_ghoulAI : public ScriptedAI
    {
        npc_risen_ghoulAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            // there is no difficulty entry for heroic
            if (IsHeroic())
                me->SetMaxHealth(50400);
            LeapTimer = 3500;
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (LeapTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                    DoCast(target, (SPELL_LEAP));

                Talk(SAY_ANNOUNCER_GHUL);
                LeapTimer = 3500;
            } else LeapTimer -= diff;

            DoMeleeAttackIfReady();
        }

    private:
        uint32 LeapTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_risen_ghoulAI(creature);
    }
};

class npc_black_knight_skeletal_gryphon : public CreatureScript
{
public:
    npc_black_knight_skeletal_gryphon() : CreatureScript("npc_black_knight_skeletal_gryphon") { }

    struct npc_black_knight_skeletal_gryphonAI : public npc_escortAI
    {
        npc_black_knight_skeletal_gryphonAI(Creature* creature) : npc_escortAI(creature)
        {
            Start(false, true, ObjectGuid::Empty, NULL);
            instance = creature->GetInstanceScript();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetImmuneToAll(true);
            me->SetReactState(REACT_PASSIVE);
        }

        void JumpToNextStep(uint32 _timer)
        {
            PhaseTimer = _timer;
            ++Step;
        }

        void Reset()
        {
            PhaseTimer = 1000;
            Step = 0;
        }

        void WaypointReached(uint32 id)
        {
            switch (id)
            {
                case 1:
                    me->SetSpeedRate(MOVE_FLIGHT, 2.0f);
                    break;
                case 17:
                    me->SetCanFly(false);
                    me->SetSpeedRate(MOVE_RUN, 2.0f);
                    SetEscortPaused(true);
                    if (Creature* knight = ObjectAccessor::GetCreature(*me, ObjectGuid(instance ? ObjectGuid(instance->GetGuidData(NPC_BLACK_KNIGHT)) : ObjectGuid::Empty)))
                    {
                        knight->ExitVehicle();
                        knight->GetMotionMaster()->MovePoint(0, BlackKnight[1]);
                        knight->SetHomePosition(BlackKnight[1]);
                    }
                    JumpToNextStep(2500);
                    break;
                case 19:
                    me->SetCanFly(true);
                    me->SetSpeedRate(MOVE_FLIGHT, 2.5f);
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);

            if (PhaseTimer <= diff)
            {
                if (Creature* knight = ObjectAccessor::GetCreature(*me, ObjectGuid(instance ? ObjectGuid(instance->GetGuidData(NPC_BLACK_KNIGHT)) : ObjectGuid::Empty)))
                    if (Creature* announcer = ObjectAccessor::GetCreature(*me, ObjectGuid(instance ? ObjectGuid(instance->GetGuidData(DATA_ANNOUNCER)) : ObjectGuid::Empty)))
                        switch (Step)
                        {
                            case 1:
                                knight->AI()->Talk(SAY_INTRO_1);
                                knight->HandleEmoteCommand(EMOTE_ONESHOT_SPELL_CAST);
                                knight->SetUInt64Value(UNIT_FIELD_TARGET, announcer->GetGUID().GetRawValue());
                                announcer->AddAura(SPELL_DEATH_RESPITE_VIS, announcer);
                                JumpToNextStep(6500);
                                break;
                            case 2:
                                knight->AI()->Talk(SAY_INTRO_2);
                                knight->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                                announcer->GetMotionMaster()->MoveJump(Location[0], 40, 20);
                                announcer->SetStandState(UNIT_STAND_STATE_DEAD);
                                announcer->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                JumpToNextStep(9000);
                                break;
                            case 3:
                                knight->AI()->Talk(SAY_INTRO_3);
                                knight->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                                announcer->GetEntry() == NPC_JAEREN ? announcer->CastSpell(announcer, SPELL_RISE_JAEREN) : announcer->CastSpell(announcer, SPELL_RISE_ARELAS);
                                announcer->DespawnOrUnsummon();
                                knight->SetUInt64Value(UNIT_FIELD_TARGET, 0);
                                JumpToNextStep(3000);
                                break;
                            case 4:
                                knight->SetInCombatWithZone();
                                knight->SetReactState(REACT_AGGRESSIVE);
                                knight->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                knight->SetImmuneToAll(false);
                                SetEscortPaused(false);
                                break;
                            default:
                                break;
                        }
            }
            else
                PhaseTimer -= diff;
        }

    private:
        InstanceScript* instance;
        uint8 Step;
        uint32 PhaseTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_black_knight_skeletal_gryphonAI(creature);
    }
};

// 67729, 67886
class spell_toc5_ghuol_explode : public SpellScriptLoader
{
public:
    spell_toc5_ghuol_explode() : SpellScriptLoader("spell_toc5_ghuol_explode") { }

    class spell_toc5_ghuol_explode_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_toc5_ghuol_explode_SpellScript)

        void HandleOnHit()
        {
            Player* target = GetHitPlayer();
            if (!target)
                return;

            if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                if (Creature* knight = ObjectAccessor::GetCreature(*target, ObjectGuid(instance ? ObjectGuid(instance->GetGuidData(NPC_BLACK_KNIGHT)) : ObjectGuid::Empty)))
                    knight->AI()->DoAction(ACTION_I_HAVE_HAD_WORSE_FAIL);
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_toc5_ghuol_explode_SpellScript::HandleOnHit);
        }
    };

    SpellScript *GetSpellScript() const
    {
        return new spell_toc5_ghuol_explode_SpellScript();
    }
};

class spell_toc5_deaths_respite : public SpellScriptLoader
{
    public:
        spell_toc5_deaths_respite() : SpellScriptLoader("spell_toc5_deaths_respite") { }

        class spell_toc5_deaths_respite_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_toc5_deaths_respite_AuraScript);

            bool Load()
            {
                PlayerGUID.Clear();
                map = NULL;
                return true;
            }

            void HandleAfterEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* target = GetUnitOwner())
                {
                    PlayerGUID = target->GetGUID();
                    map = target->GetMap();
                }
            }

            void HandleAfterEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (!PlayerGUID || !map)
                    return;

                if (Player* target = ObjectAccessor::GetPlayer(*map, PlayerGUID))
                    target->CastSpell(target, SPELL_DEATH_RESPITE_EFF, true);
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_toc5_deaths_respite_AuraScript::HandleAfterEffectApply, EFFECT_0, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
                AfterEffectRemove += AuraEffectRemoveFn(spell_toc5_deaths_respite_AuraScript::HandleAfterEffectRemove, EFFECT_0, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL);
            }

        private:
            Map* map;
            ObjectGuid PlayerGUID;
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_toc5_deaths_respite_AuraScript();
        }
};

void AddSC_boss_black_knight()
{
    new boss_black_knight();
    new npc_risen_ghoul();
    new npc_black_knight_skeletal_gryphon();
    new spell_toc5_ghuol_explode();
    new spell_toc5_deaths_respite();
}
