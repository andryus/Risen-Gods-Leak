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
#include "utgarde_keep.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

uint32 entry_search[3] =
{
    186609,
    186610,
    186611
};

enum Misc
{
    CRAUTERIZE       = 60211,
    BURNING_BRAND_NH = 43757,
    BURNING_BRAND_HC = 59601
};

class npc_dragonflayer_forge_master : public CreatureScript
{
public:
    npc_dragonflayer_forge_master() : CreatureScript("npc_dragonflayer_forge_master") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_dragonflayer_forge_masterAI(creature);
    }

    struct npc_dragonflayer_forge_masterAI : public ScriptedAI
    {
        npc_dragonflayer_forge_masterAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            fm_Type = 0;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
        }

        InstanceScript* instance;
        uint8  fm_Type;
        uint32 CauterizeTimer;
        uint32 BurningBrandTimer;

        void Reset()
        {
            CauterizeTimer     = 5000;
            BurningBrandTimer  = 4000;
            uint16 whilecount = 0; 

            while (fm_Type == 0)
            {
                fm_Type = GetForgeMasterType();
                //if we get after first try no result, we send an error and remove flag to prevent a non attackable mob with aggro to players
                if (whilecount++ >= 1)
                {
                    TC_LOG_ERROR("misc", "NPC Script: npc_dragonflayer_forge_master can't find fm_Typ. NON_ATTACKABLE flag will be removed to prevent more bugs");
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                    break;
                }
            }

            CheckForge();
        }

        void CheckForge()
        {
            if (instance)
            {
                switch (fm_Type)
                {
                    case 1:
                        instance->SetGuidData(DATA_FORGE_MASTER_1, me->GetGUID());
                        instance->SetData(DATA_FORGE_1, me->IsAlive() ? NOT_STARTED : DONE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                        break;
                    case 2:
                        instance->SetGuidData(DATA_FORGE_MASTER_2, me->GetGUID());
                        instance->SetData(DATA_FORGE_2, me->IsAlive() ? NOT_STARTED : DONE);
                        break;
                    case 3:
                        instance->SetGuidData(DATA_FORGE_MASTER_3, me->GetGUID());
                        instance->SetData(DATA_FORGE_3, me->IsAlive() ? NOT_STARTED : DONE);
                        break;
                }
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            if (fm_Type == 0)
                fm_Type = GetForgeMasterType();

            if (instance)
            {
                switch (fm_Type)
                {
                    case 1:
                        instance->SetData(DATA_FORGE_1, DONE);
                        break;
                    case 2:
                        instance->SetData(DATA_FORGE_2, DONE);
                        break;
                    case 3:
                        instance->SetData(DATA_FORGE_3, DONE);
                        break;
                }
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            me->CallForHelp(20.0f);

            if (fm_Type == 0)
                fm_Type = GetForgeMasterType();

            if (instance)
            {
                switch (fm_Type)
                {
                    case 1:
                        instance->SetData(DATA_FORGE_1, IN_PROGRESS);
                        break;
                    case 2:
                        instance->SetData(DATA_FORGE_2, IN_PROGRESS);
                        break;
                    case 3:
                        instance->SetData(DATA_FORGE_3, IN_PROGRESS);
                        break;
                }
            }
            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
        }

        uint8 GetForgeMasterType()
        {
            float diff = 30.0f;
            uint8 near_f = 0;

            for (uint8 i = 0; i < 3; ++i)
            {
                if (GameObject* go = me->FindNearestGameObject(entry_search[i], 30))
                {
                    if (me->IsWithinDist(go, diff, false))
                    {
                        near_f = i + 1;
                        diff = me->GetDistance2d(go);
                    }
                }
            }
            return near_f > 0 && near_f < 4 ? near_f : 0;
        }

        void UpdateAI(uint32 diff)
        {
            if (fm_Type == 0)
                fm_Type = GetForgeMasterType();

            if (!UpdateVictim())
                return;

            if (CauterizeTimer <= diff)
            {
                DoCastSelf(CRAUTERIZE);
                CauterizeTimer = 6000;
            }
            else CauterizeTimer -= diff;

            if (BurningBrandTimer <= diff)
            {
                DoCastVictim(BURNING_BRAND_NH, true);
                BurningBrandTimer = 6000;
            }
            else BurningBrandTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

enum TickingTimeBomb
{
    SPELL_TICKING_TIME_BOMB_EXPLODE = 59687
};
class spell_ticking_time_bomb : public SpellScriptLoader
{
    public:
        spell_ticking_time_bomb() : SpellScriptLoader("spell_ticking_time_bomb") { }

        class spell_ticking_time_bomb_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_ticking_time_bomb_AuraScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                return (bool) sSpellMgr->GetSpellInfo(SPELL_TICKING_TIME_BOMB_EXPLODE);
            }

            void HandleOnEffectRemove(AuraEffect const* /* aurEff */, AuraEffectHandleModes /* mode */)
            {
                if (GetCaster() == GetTarget())
                {
                    GetTarget()->CastSpell(GetTarget(), SPELL_TICKING_TIME_BOMB_EXPLODE, true);
                }
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_ticking_time_bomb_AuraScript::HandleOnEffectRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_ticking_time_bomb_AuraScript();
        }
};

enum Fixate
{
    SPELL_FIXATE_TRIGGER = 40415
};
class spell_fixate : public SpellScriptLoader
{
    public:
        spell_fixate() : SpellScriptLoader("spell_fixate") { }

        class spell_fixate_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_fixate_SpellScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                return (bool) sSpellMgr->GetSpellInfo(SPELL_FIXATE_TRIGGER);
            }

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                // The unit has to cast the taunt on hisself, but we need the original caster for SPELL_AURA_MOD_TAUNT
                GetCaster()->CastSpell(GetCaster(), SPELL_FIXATE_TRIGGER, true, 0, 0, GetHitUnit()->GetGUID());
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_fixate_SpellScript::HandleScriptEffect, EFFECT_2, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_fixate_SpellScript();
        }
};

enum EnslavedProtoDrake
{
    TYPE_PROTODRAKE_AT      = 28,
    DATA_PROTODRAKE_MOVE    = 6,

    PATH_PROTODRAKE         = 125946,

    EVENT_REND              = 1,
    EVENT_FLAME_BREATH      = 2,
    EVENT_KNOCKAWAY         = 3,

    SPELL_REND              = 43931,
    SPELL_FLAME_BREATH      = 50653,
    SPELL_KNOCK_AWAY        = 49722,

    POINT_LAST              = 5,
};

Position const ProtoDrakePoint[2] =
{
    { 371.452f, 189.118f, 36.656f },
    { 371.452f, 189.118f, 30.776f }
};

const Position protodrakeCheckPos = {206.24f, -190.28f, 200.11f, 0.0f};

class npc_enslaved_proto_drake : public CreatureScript
{
public:
    npc_enslaved_proto_drake() : CreatureScript("npc_enslaved_proto_drake") { }

    struct npc_enslaved_proto_drakeAI : public ScriptedAI
    {
        npc_enslaved_proto_drakeAI(Creature* creature) : ScriptedAI(creature)
        {
            _setData = false;
        }

        void Reset()
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_REND, urand(2000, 3000));
            _events.ScheduleEvent(EVENT_FLAME_BREATH, urand(5500, 7000));
            _events.ScheduleEvent(EVENT_KNOCKAWAY, urand(3500, 6000));
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type == WAYPOINT_MOTION_TYPE && id == POINT_LAST)
            {
                me->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
            }

            if (type == POINT_MOTION_TYPE && id == 3)
            {
                me->SetHover(false, true);
                me->GetMotionMaster()->MoveLand(4, ProtoDrakePoint[1]);
                me->SetHomePosition(ProtoDrakePoint[1]);
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type == TYPE_PROTODRAKE_AT && data == DATA_PROTODRAKE_MOVE && !_setData && me->GetDistance(protodrakeCheckPos) < 5.0f)
            {
                _setData = true;
                me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                me->GetMotionMaster()->MovePath(PATH_PROTODRAKE, false);
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventid = _events.ExecuteEvent())
            {
                switch (eventid)
                {
                    case EVENT_REND:
                        DoCast(SPELL_REND);
                        _events.ScheduleEvent(EVENT_REND, urand(15000, 20000));
                        break;
                    case EVENT_FLAME_BREATH:
                        DoCast(SPELL_FLAME_BREATH);
                        _events.ScheduleEvent(EVENT_FLAME_BREATH, urand(11000, 12000));
                        break;
                    case EVENT_KNOCKAWAY:
                        DoCast(SPELL_KNOCK_AWAY);
                        _events.ScheduleEvent(EVENT_KNOCKAWAY, urand(7000, 8500));
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        bool _setData;
        EventMap _events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_enslaved_proto_drakeAI(creature);
    }
};

#define NPC_PROTO_DRAKE 24083

class speedcore_trigger_npc_utgarde_keep_proto_drake : public CreatureScript
{
public:
    speedcore_trigger_npc_utgarde_keep_proto_drake() : CreatureScript("speedcore_trigger_npc_utgarde_keep_proto_drake") { }

    struct speedcore_trigger_npc_utgarde_keep_proto_drakeAI : public ScriptedAI
    {
        speedcore_trigger_npc_utgarde_keep_proto_drakeAI(Creature* creature) : ScriptedAI(creature)
        {
            _alreadySpawned = false;
            me->SetVisible(false);
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (!_alreadySpawned && who && me->GetExactDist(who) < 15.0f)
            {
                if (Creature* drake = me->SummonCreature(NPC_PROTO_DRAKE, 339.766f, 239.482f, 81.355934f, 5.136961f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000))
                {   
                    drake->SetHover(true, true);
                    drake->SetCanFly(true);
                    drake->GetMotionMaster()->MovePoint(3, ProtoDrakePoint[0]);
                }

                _alreadySpawned = true;
            }
        }

    private:
        bool _alreadySpawned;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new speedcore_trigger_npc_utgarde_keep_proto_drakeAI(creature);
    }
};

void AddSC_utgarde_keep()
{
    new npc_dragonflayer_forge_master();
    new npc_enslaved_proto_drake();
    new spell_ticking_time_bomb();
    new spell_fixate();
    new speedcore_trigger_npc_utgarde_keep_proto_drake();
}
