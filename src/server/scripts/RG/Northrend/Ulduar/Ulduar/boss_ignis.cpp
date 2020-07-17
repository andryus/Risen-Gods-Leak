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
#include "Vehicle.h"
#include "Player.h"
#include "GameTime.h"
#include "DBCStores.h"

// ###### Related Creatures/Object ######
enum Creatures
{
    NPC_IRON_CONSTRUCT    = 33121,
};

// ###### Texts ######
enum Yells
{
    SAY_AGGRO             = 0,
    SAY_SLAY              = 4,
    SAY_DEATH             = 6,
    SAY_SUMMON            = 1,
    SAY_SLAG_POT          = 2,
    SAY_SCORCH            = 3,
    SAY_BERSERK           = 5,
    SAY_JETS              = 7
};

// ###### Datas ######
enum Actions
{
    ACTION_REMOVE_BUFF  = 1,
};

enum AchievementData
{
    DATA_SHATTERED                  = 29252926, // ToDo rename -.-
    ACHIEVEMENT_IGNIS_START_EVENT   = 20951,
    ACHIEVEMENT_HOT_POCKET_10       = 2927,
    ACHIEVEMENT_HOT_POCKET_25       = 2928,
};

// ###### Event Controlling ######
enum Events
{
    EVENT_JET               = 1,
    EVENT_SCORCH            = 2,
    EVENT_SLAG_POT          = 3,
    EVENT_GRAB_POT          = 4,
    EVENT_CHANGE_POT        = 5,
    EVENT_END_POT           = 6,
    EVENT_CONSTRUCT         = 7,
    EVENT_BERSERK           = 8,
    EVENT_CHECK_HOME_DIST   = 9
};

// ###### Spells ######
enum Spells
{
    SPELL_FLAME_JETS            = 62680,
    SPELL_SCORCH                = 62546,
    SPELL_SCORCH_SUMMON         = 62551,
    SPELL_SLAG_POT              = 62717,
    SPELL_SLAG_POT_DAMAGE       = 65722,
    SPELL_SLAG_IMBUED           = 62836,
    SPELL_ACTIVATE_CONSTRUCT    = 62488,
    SPELL_STRENGHT              = 64473,
    SPELL_GRAB                  = 62707,    // Effect 2: Script Effect, possibly enter vehicle
    SPELL_BERSERK               = 47008,

    // Iron Construct
    SPELL_HEAT                  = 65667,
    SPELL_MOLTEN                = 62373,
    SPELL_BRITTLE_10            = 62382,   // spelldifficulty.dbc bugged?
    SPELL_BRITTLE_25            = 67114,
    SPELL_SHATTER               = 62383,   // Effect 2: Send Event? is there a effect?
    SPELL_GROUND                = 62548,
};

class boss_ignis : public CreatureScript
{
    public:
        boss_ignis() : CreatureScript("boss_ignis") { }

        struct boss_ignis_AI : public BossAI
        {
            boss_ignis_AI(Creature* creature) : BossAI(creature, BOSS_IGNIS), _vehicle(me->GetVehicleKit())
            {
                ASSERT(_vehicle);
            }

            void Reset()
            {
                BossAI::Reset();
                //! Kick player out of Slag Pot
                if (_vehicle)
                    _vehicle->RemoveAllPassengers();

                instance->DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEVEMENT_IGNIS_START_EVENT);

                //! Respawn construct adds on reset
                std::list<Creature*> constructList;
                me->GetCreatureListWithEntryInGrid(constructList, NPC_IRON_CONSTRUCT, 220.0f);
                for (std::list<Creature*>::const_iterator itr = constructList.begin(); itr != constructList.end(); ++itr)
                    if ((*itr) && !(*itr)->IsAlive())
                        (*itr)->Respawn();
            }

            void EnterCombat(Unit* who)
            {
                BossAI::EnterCombat(who);
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_JET,        30*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SCORCH,     25*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SLAG_POT,   35*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_CONSTRUCT,  15*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_END_POT,    40*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_BERSERK,     8*MINUTE*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_CHECK_HOME_DIST, IN_MILLISECONDS);

                _slagPotGUID.Clear();
                _shattered          = false;
                _firstConstructKill = 0;

                instance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEVEMENT_IGNIS_START_EVENT);
            }

            void JustDied(Unit* killer)
            {
                BossAI::JustDied(killer);
                Talk(SAY_DEATH);

                //! Despawns active construct adds at the end
                std::list<Creature*> constructList;
                me->GetCreatureListWithEntryInGrid(constructList, NPC_IRON_CONSTRUCT, 220.0f);
                for (std::list<Creature*>::const_iterator itr = constructList.begin(); itr != constructList.end(); ++itr)
                    if ((*itr) && !(*itr)->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                        (*itr)->DespawnOrUnsummon();
            }

            uint32 GetData(uint32 type) const
            {
                if (type == DATA_SHATTERED)
                    return _shattered ? 1 : 0;

                return 0;
            }

            void KilledUnit(Unit* /*victim*/)
            {
                if (!urand(0, 4))
                    Talk(SAY_SLAY);
            }

            void DoAction(int32 action)
            {
                if (action != ACTION_REMOVE_BUFF)
                    return;

                me->RemoveAuraFromStack(SPELL_STRENGHT);

                // Shattered Achievement
                // Does this really return seconds?
                time_t secondKill = game::time::GetGameTime();
                if ((secondKill - _firstConstructKill) < 5)
                    _shattered = true;
                _firstConstructKill = secondKill;
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
                        case EVENT_JET:
                            Talk(SAY_JETS);
                            DoCastSelf(SPELL_FLAME_JETS);
                            events.ScheduleEvent(EVENT_JET, urand(35*IN_MILLISECONDS, 40*IN_MILLISECONDS));
                            break;
                        case EVENT_SLAG_POT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0U, NonTankTargetSelector(me, true)))
                            {
                                Talk(SAY_SLAG_POT);
                                _slagPotGUID = target->GetGUID();
                                DoCast(target, SPELL_GRAB);
                                events.DelayEvents(3000);
                                events.ScheduleEvent(EVENT_GRAB_POT, 1000);
                            }
                            events.ScheduleEvent(EVENT_SLAG_POT, RAID_MODE(30*IN_MILLISECONDS, 15*IN_MILLISECONDS));
                            break;
                        case EVENT_GRAB_POT:
                            if (Unit* slagPotTarget = ObjectAccessor::GetUnit(*me, _slagPotGUID))
                            {
                                DoCast(slagPotTarget, SPELL_SLAG_POT);
                                slagPotTarget->EnterVehicle(me, 1); // Needed because unknown vehicle grab spell maybe 62711
                                events.ScheduleEvent(EVENT_END_POT, 10*IN_MILLISECONDS);
                            }
                            else
                                _slagPotGUID.Clear();
                            break;
                        case EVENT_END_POT:
                            if (Unit* slagPotTarget = ObjectAccessor::GetUnit(*me, _slagPotGUID))
                                slagPotTarget->ExitVehicle();
                            _slagPotGUID.Clear();
                            break;
                        case EVENT_SCORCH:
                            Talk(SAY_SCORCH);
                            DoCastSelf(SPELL_SCORCH_SUMMON, true);
                            DoCast(SPELL_SCORCH);
                            events.ScheduleEvent(EVENT_SCORCH, 25*IN_MILLISECONDS);
                            break;
                        case EVENT_CONSTRUCT:
                            Talk(SAY_SUMMON);
                            DoCast(SPELL_ACTIVATE_CONSTRUCT);
                            events.ScheduleEvent(EVENT_CONSTRUCT, RAID_MODE(40*IN_MILLISECONDS, 30*IN_MILLISECONDS));
                            break;
                        case EVENT_BERSERK:
                            DoCastSelf(SPELL_BERSERK, true);
                            Talk(SAY_BERSERK);
                            break;
                        case EVENT_CHECK_HOME_DIST:
                            if (me->GetDistance(me->GetHomePosition()) > 200.0f)
                                EnterEvadeMode();
                            events.ScheduleEvent(EVENT_CHECK_HOME_DIST, IN_MILLISECONDS);
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            ObjectGuid _slagPotGUID;
            Vehicle* _vehicle;
            time_t _firstConstructKill; //! ToDo
            bool _shattered;

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_ignis_AI(creature);
        }
};

class npc_iron_construct : public CreatureScript
{
    public:
        npc_iron_construct() : CreatureScript("npc_iron_construct") { }

        struct npc_iron_constructAI : public ScriptedAI
        {
            npc_iron_constructAI(Creature* creature) : ScriptedAI(creature), _instance(creature->GetInstanceScript())
            {
                ASSERT(_instance);
            }

            void Reset()
            {
                me->setFaction(31); // Friendly
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage)
            {
                if (me->HasAura(RAID_MODE(SPELL_BRITTLE_10, SPELL_BRITTLE_25)) && damage >= RAID_MODE<uint32>(3000, 5000))
                {
                    me->RemoveAura(RAID_MODE(SPELL_BRITTLE_10, SPELL_BRITTLE_25));
                    DoCast(SPELL_SHATTER);
                    if (Creature* ignis = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(BOSS_IGNIS))))
                        ignis->AI()->DoAction(ACTION_REMOVE_BUFF);

                    me->DespawnOrUnsummon(1000);
                }
            }

            void UpdateAI(uint32 /*uiDiff*/)
            {
                if (!UpdateVictim())
                    return;

                if (Aura* aur = me->GetAura(SPELL_HEAT))
                {
                    if (aur->GetStackAmount() >= 10)
                    {
                        me->RemoveAura(SPELL_HEAT);
                        DoCast(SPELL_MOLTEN);
                        me->GetThreatManager().resetAllAggro();
                    }
                }

                // Water pools
                if (me->IsInWater() && me->HasAura(SPELL_MOLTEN))
                {
                    DoCast(RAID_MODE(SPELL_BRITTLE_10, SPELL_BRITTLE_25));
                    me->RemoveAura(SPELL_MOLTEN);
                }

                DoMeleeAttackIfReady();
            }

        private:
            InstanceScript* _instance;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_iron_constructAI(creature);
        }
};

class npc_scorch_ground : public CreatureScript
{
    public:
        npc_scorch_ground() : CreatureScript("npc_scorch_ground") { }

        struct npc_scorch_groundAI : public ScriptedAI
        {
            npc_scorch_groundAI(Creature* creature) : ScriptedAI(creature)
            {
                creature->SetDisplayId(16925); //model 2 in db cannot overwrite wdb fields
                if (me->IsInWater())
                    me->DespawnOrUnsummon();
            }

            void Reset()
            {
                DoCastSelf(SPELL_GROUND);
                me->DespawnOrUnsummon(40*IN_MILLISECONDS);
            }

            void UpdateAI(uint32 /*diff*/) {}
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_scorch_groundAI(creature);
        }
};

class spell_ignis_slag_pot : public SpellScriptLoader
{
    public:
        spell_ignis_slag_pot() : SpellScriptLoader("spell_ignis_slag_pot") { }

        class spell_ignis_slag_pot_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_ignis_slag_pot_AuraScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellStore.LookupEntry(SPELL_SLAG_POT_DAMAGE))
                    return false;
                if (!sSpellStore.LookupEntry(SPELL_SLAG_IMBUED))
                    return false;
                return true;
            }

            void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
            {
                if (Unit* target = GetTarget())
                    GetCaster()->CastSpell(target, SPELL_SLAG_POT_DAMAGE, true);
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTarget()->IsAlive() && GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
                {
                    GetTarget()->CastSpell(GetTarget(), SPELL_SLAG_IMBUED, true);
                    if(Player* player = GetTarget()->ToPlayer())
                        player->CompletedAchievement(sAchievementStore.LookupEntry(GetCaster()->GetMap()->GetSpawnMode() & 1 /* == 25man */ ? ACHIEVEMENT_HOT_POCKET_25 : ACHIEVEMENT_HOT_POCKET_10));
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_ignis_slag_pot_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
                AfterEffectRemove += AuraEffectRemoveFn(spell_ignis_slag_pot_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_ignis_slag_pot_AuraScript();
        }
};

//! There is possibly no need for this "hack" anymore
class spell_ignis_heat : public SpellScriptLoader
{
    public:
        spell_ignis_heat() : SpellScriptLoader("spell_ignis_heat") { }

        class spell_ignis_heat_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_ignis_heat_AuraScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellStore.LookupEntry(SPELL_HEAT))
                    return false;
                return true;
            }

            void HandleAfterEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();

                target->SetSpeedRate(MOVE_WALK, 0.5f + GetStackAmount()*0.05f);
                target->SetSpeedRate(MOVE_RUN, 0.5f + GetStackAmount()*0.05f);
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_ignis_heat_AuraScript::HandleAfterEffectApply, EFFECT_0, SPELL_AURA_MOD_INCREASE_SPEED, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_ignis_heat_AuraScript();
        }
};

class RemoveActiveConstructs
{
    public:
        bool operator() (WorldObject* object)
        {
            if (!object->ToUnit()->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                return true;
            return false;
        }
};

class spell_ignis_activate_construct : public SpellScriptLoader
{
    public:
        spell_ignis_activate_construct() : SpellScriptLoader("spell_ignis_activate_construct") { }

        class spell_ignis_activate_construct_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ignis_activate_construct_SpellScript);

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                Creature* target = GetHitCreature();
                if (!target)
                    return;

                target->setFaction(14); // Hostile
                target->SetReactState(REACT_AGGRESSIVE); // Set to passive in script
                target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE); // Flag set in DB
                target->SetInCombatWithZone();

                GetCaster()->CastSpell(GetCaster(), SPELL_STRENGHT, true);
            }

            void FilterTargets(std::list<WorldObject*>& targetList)
            {
                targetList.remove_if(RemoveActiveConstructs());
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_ignis_activate_construct_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ignis_activate_construct_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_ignis_activate_construct_SpellScript();
        }
};

class achievement_ignis_shattered : public AchievementCriteriaScript
{
    public:
        achievement_ignis_shattered() : AchievementCriteriaScript("achievement_ignis_shattered") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (target && target->IsAIEnabled)
                return target->GetAI()->GetData(DATA_SHATTERED);

            return false;
        }
};

void AddSC_boss_ignis()
{
    new boss_ignis();
    new npc_iron_construct();
    new npc_scorch_ground();
    new achievement_ignis_shattered();
//    new spell_ignis_heat();
    new spell_ignis_slag_pot();
    new spell_ignis_activate_construct();
}
