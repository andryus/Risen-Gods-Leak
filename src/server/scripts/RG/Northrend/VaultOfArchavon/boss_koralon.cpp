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
#include "vault_of_archavon.h"

enum Events
{
    //! Koralon
    EVENT_BURNING_BREATH    = 1,
    EVENT_BURNING_FURY      = 2,
    EVENT_FLAME_CINDER      = 3,
    EVENT_METEOR_FISTS      = 4,

    //! Flame Warder
    EVENT_FW_LAVA_BIRST     = 5,
    EVENT_FW_METEOR_FISTS   = 6,

    EVENT_LOS_CHECK         = 7,
};

enum Spells
{
    //! Koralon spells
    SPELL_BURNING_BREATH            = 66665,
    SPELL_BURNING_FURY              = 66721,
    SPELL_FLAME_CINDER              = 66684,
    SPELL_METEOR_FISTS              = 66725,
    SPELL_METEOR_FISTS_TRIGGER      = 67333,

    //! Flame Warder spells
    SPELL_FW_LAVA_BIRST             = 66813,
    SPELL_FW_METEOR_FISTS_A         = 66808,
    SPELL_FW_METEOR_FISTS_B         = 67331
};

class boss_koralon : public CreatureScript
{
    public:
        boss_koralon() : CreatureScript("boss_koralon") { }

        struct boss_koralonAI : public BossAI
        {
            boss_koralonAI(Creature* creature) : BossAI(creature, BOSS_KORALON) { }

            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);

                firstFlameCinder = false;

                events.ScheduleEvent(EVENT_BURNING_FURY,   20 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_BURNING_BREATH, 15 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_METEOR_FISTS,   75 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FLAME_CINDER,   10 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_LOS_CHECK,       1 * IN_MILLISECONDS);
            }

            void Reset() override
            {
                BossAI::Reset();

                timerLOSCheck = 0;
                rotateTimer   = 0;
                me->GetPosition(&lastPos);
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                _DespawnAtEvade();
            }

            void UpdateAI(uint32 diff) override
            {
                if (rotateTimer)
                {
                    rotateTimer += diff;
                    if (rotateTimer >= 3000)
                    {
                        if (!me->HasUnitMovementFlag(MOVEMENTFLAG_LEFT))
                        {
                            me->SetUnitMovementFlags(MOVEMENTFLAG_LEFT);
                            me->SendMovementFlagUpdate();
                            rotateTimer = 1;
                            return;
                        }
                        else
                        {
                            me->RemoveUnitMovementFlag(MOVEMENTFLAG_LEFT);
                            me->SendMovementFlagUpdate();
                            rotateTimer = 0;
                            return;
                        }
                    }
                }
				
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING) || me->IsNonMeleeSpellCast(true))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_BURNING_FURY:
                            DoCastSelf(SPELL_BURNING_FURY);
                            events.ScheduleEvent(EVENT_BURNING_FURY, 20 * IN_MILLISECONDS);
                            break;
                        case EVENT_BURNING_BREATH:
                            rotateTimer = 1500;
                            DoCastSelf(SPELL_BURNING_BREATH);
                            events.ScheduleEvent(EVENT_BURNING_BREATH, 45 * IN_MILLISECONDS);
                            return;
                        case EVENT_METEOR_FISTS:
                            DoCastSelf(SPELL_METEOR_FISTS);
                            events.ScheduleEvent(EVENT_METEOR_FISTS, 45 * IN_MILLISECONDS);
                            break;
                        case EVENT_FLAME_CINDER:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, false))
                                DoCast(target, SPELL_FLAME_CINDER);
                            if (firstFlameCinder)
                            {
                                firstFlameCinder = false;
                                events.ScheduleEvent(EVENT_FLAME_CINDER, 2 * IN_MILLISECONDS);
                            }
                            else
                            {
                                firstFlameCinder = true;
                                events.ScheduleEvent(EVENT_FLAME_CINDER, 8 * IN_MILLISECONDS);
                            }
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

            void MoveInLineOfSight(Unit *who) override
            {
                if (me->GetVictim())
                    return;
                
                if (Player* player = who->ToPlayer())
                {
                    if (player->GetDistance(me) <= 15.0f)
                        AttackStart(player);
                }
            }

        private:
            bool firstFlameCinder;
            uint32 timerLOSCheck;
            uint32 rotateTimer;
            Position lastPos;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_koralonAI(creature);
        }
};

class spell_meteor_fists : public SpellScriptLoader
{
    public:
        spell_meteor_fists() : SpellScriptLoader("spell_meteor_fists") { }

        class spell_meteor_fists_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_meteor_fists_AuraScript);

            bool Validate(SpellInfo const* /*spellInfo*/)
            {
                return sSpellMgr->GetSpellInfo(SPELL_METEOR_FISTS_TRIGGER);
            }

            void HandleProc(ProcEventInfo& /*eventInfo*/)
            {
                if (Unit* owner = GetUnitOwner())
                {
                    if (!owner->ToCreature())
                        return;

                    owner->CastSpell((Unit*)NULL, SPELL_METEOR_FISTS_TRIGGER, true);
                }
            }

            void Register()
            {
                OnProc += AuraProcFn(spell_meteor_fists_AuraScript::HandleProc);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_meteor_fists_AuraScript();
        }
};

class spell_meteor_fists_damage : public SpellScriptLoader
{
    public:
        spell_meteor_fists_damage() : SpellScriptLoader("spell_meteor_fists_damage") { }

        class spell_meteor_fists_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_meteor_fists_damage_SpellScript);

            bool Load()
            {
                targetCount = 1;
                return true;
            }

            void FilterTargets(std::list<WorldObject*>& targetList)
            {
                for (std::list<WorldObject*>::const_iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                    ++targetCount;
            }

            void HandleDummyHitTarget(SpellEffIndex /*effIndex*/)
            {
                SetHitDamage(GetHitDamage() / targetCount);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_meteor_fists_damage_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_TARGET_ENEMY);
                OnEffectHitTarget += SpellEffectFn(spell_meteor_fists_damage_SpellScript::HandleDummyHitTarget, EFFECT_0, SPELL_EFFECT_WEAPON_PERCENT_DAMAGE);
            }

        private:
            int8 targetCount;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_meteor_fists_damage_SpellScript();
        }
};

class npc_flame_warder : public CreatureScript
{
    public:
        npc_flame_warder() : CreatureScript("npc_flame_warder") { }

        struct npc_flame_warderAI : public ScriptedAI
        {
            npc_flame_warderAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                events.Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                DoZoneInCombat();

                events.ScheduleEvent(EVENT_FW_LAVA_BIRST,     5*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FW_METEOR_FISTS, 10*IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_FW_LAVA_BIRST:
                            DoCastVictim(SPELL_FW_LAVA_BIRST);
                            events.ScheduleEvent(EVENT_FW_LAVA_BIRST, 15*IN_MILLISECONDS);
                            break;
                        case EVENT_FW_METEOR_FISTS:
                            DoCastSelf(SPELL_METEOR_FISTS);
                            events.ScheduleEvent(EVENT_FW_METEOR_FISTS, 30*IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_flame_warderAI(creature);
        }
};

void AddSC_boss_koralon()
{
    new boss_koralon();
    new spell_meteor_fists();
    new spell_meteor_fists_damage();

    new npc_flame_warder();
}
