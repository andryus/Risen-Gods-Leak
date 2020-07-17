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
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "pit_of_saron.h"
#include "Vehicle.h"
#include "CreatureTextMgr.h"
#include "SmartAI.h"

enum Spells
{
    SPELL_TACTICAL_BLINK        = 69584, // Ymirjar Flamebearer
    SPELL_FROST_BREATH          = 69527, // Iceborn Proto-Drake
    SPELL_STRANGULATE           = 69413  // Start Event
};

enum Misc
{
    NPC_WRATHBONE_SKELKETON     = 36877
};

class npc_iceborn_protodrake : public CreatureScript
{
    public:
        npc_iceborn_protodrake() : CreatureScript("npc_iceborn_protodrake") { }

        struct npc_iceborn_protodrakeAI: public ScriptedAI
        {
            npc_iceborn_protodrakeAI(Creature* creature) : ScriptedAI(creature), _vehicle(creature->GetVehicleKit())
            {
                ASSERT(_vehicle);
            }

            void Reset()
            {
                _frostBreathCooldown = 5 * IN_MILLISECONDS;
            }

            void EnterCombat(Unit* /*who*/)
            {
                _vehicle->RemoveAllPassengers();
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (_frostBreathCooldown < diff)
                {
                    DoCastVictim(SPELL_FROST_BREATH);
                    _frostBreathCooldown = 10 * IN_MILLISECONDS;
                }
                else
                    _frostBreathCooldown -= diff;

                DoMeleeAttackIfReady();
            }

        private:
            Vehicle* _vehicle;
            uint32 _frostBreathCooldown;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_iceborn_protodrakeAI(creature);
        }
};

class spell_trash_npc_glacial_strike : public SpellScriptLoader
{
    public:
        spell_trash_npc_glacial_strike() : SpellScriptLoader("spell_trash_npc_glacial_strike") { }

        class spell_trash_npc_glacial_strike_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_trash_npc_glacial_strike_AuraScript);

            void PeriodicTick(AuraEffect const* /*aurEff*/)
            {
                if (GetTarget()->IsFullHealth())
                {
                    GetTarget()->RemoveAura(GetId(), ObjectGuid::Empty, 0, AURA_REMOVE_BY_ENEMY_SPELL);
                    PreventDefaultAction();
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_trash_npc_glacial_strike_AuraScript::PeriodicTick, EFFECT_2, SPELL_AURA_PERIODIC_DAMAGE_PERCENT);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_trash_npc_glacial_strike_AuraScript();
        }
};

enum EyeOfTheLichkingMisc
{
    EVENT_TALK = 1,
    SAY_TEXT   = 0
};

// Convert this to SAI if Zone wide texts for SAI are merged from TC
class npc_pos_eye_of_the_lichking : public CreatureScript
{
    public:
        npc_pos_eye_of_the_lichking() : CreatureScript("npc_pos_eye_of_the_lichking") { }

        struct npc_pos_eye_of_the_lichkingAI : public ScriptedAI
        {
            npc_pos_eye_of_the_lichkingAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                _events.Reset();
                _events.RescheduleEvent(EVENT_TALK, urand(20, 50) * IN_MILLISECONDS);
                me->setActive(true);
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_TALK:
                            sCreatureTextMgr->SendChat(me, SAY_TEXT, NULL, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_MAP);
                            _events.RescheduleEvent(EVENT_TALK, urand(240, 300) * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }
            }
        private:
            EventMap _events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_pos_eye_of_the_lichkingAI(creature);
    }
};

class npc_pos_start_event : public CreatureScript
{
public:
    npc_pos_start_event() : CreatureScript("npc_pos_start_event") { }

    struct npc_pos_start_eventAI : public SmartAI
    {
        npc_pos_start_eventAI(Creature* creature) : SmartAI(creature) { }

        void SpellHit(Unit* /*caster*/, SpellInfo const* spell)
        {
            if (spell->Id == SPELL_STRANGULATE)
            {
                me->SetCanFly(true);
                me->SetHover(true);
                me->SetDisableGravity(true);
                me->GetMotionMaster()->MovePoint(0, me->GetPositionX() + urand(2.0f, 10.0f), me->GetPositionY() + urand(2.0f, 10.0f), me->GetPositionZ() + 10.0f);
            }
        }

        bool CanAIAttack(Unit const* target) const override
        {
            return target->GetEntry() == NPC_FALLEN_WARRIOR || target->GetEntry() == NPC_SKELETON;
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_pos_start_eventAI(creature);
    }
};

class npc_pos_cave_attacker : public CreatureScript
{
    public:
        npc_pos_cave_attacker() : CreatureScript("npc_pos_cave_attacker") { }

        struct npc_pos_cave_attackerAI : public SmartAI
        {
            npc_pos_cave_attackerAI(Creature* creature) : SmartAI(creature) 
            {
                instance = creature->GetInstanceScript();
                EnableAttack = false;
            }

            void SetData(uint32 type, uint32 data)
            {
                if (type == DATA_SPAWN_TUNNEL_ADDS && data == DATA_SPAWN_TUNNEL_ADDS)
                    if (!EnableAttack)
                        EnableAttack = true;

                SmartAI::SetData(type, data);
            }

            bool CanAIAttack(Unit const* target) const override
            {
                // Spirits must not select targets in frostmourne room or Creatures
                if (instance->GetBossState(DATA_TYRANNUS) == IN_PROGRESS && target->GetTypeId() == TYPEID_PLAYER && EnableAttack)
                    return false;
                return true;
            }
        private:
            InstanceScript *instance;
            bool EnableAttack;
        };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_pos_cave_attackerAI(creature);
    }
};

class npc_pos_corrupted_champion : public CreatureScript
{
public:
    npc_pos_corrupted_champion() : CreatureScript("npc_pos_corrupted_champion") { }

    struct npc_pos_corrupted_championAI : public SmartAI
    {
        npc_pos_corrupted_championAI(Creature* creature) : SmartAI(creature) 
        {
            me->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), 528.71f, me->GetOrientation());
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_pos_corrupted_championAI(creature);
    }
};

class npc_pos_geist_ambusher : public CreatureScript
{
public:
    npc_pos_geist_ambusher() : CreatureScript("npc_pos_geist_ambusher") { }

    struct npc_pos_geist_ambusherAI : public SmartAI
    {
        npc_pos_geist_ambusherAI(Creature* creature) : SmartAI(creature) 
        { 
            instance = creature->GetInstanceScript();
        }

        void JustDied(Unit* /*killer*/)
        {
            if (instance)
            {
                instance->SetGuidData(DATA_SLAVE_DIED, me->GetGUID());
                if (instance->GetData(DATA_ALIVE_SLAVE) == 0)
                {
                    // Despawn all the Slaves
                    std::list<Creature*> SlaveList;
                    me->GetCreatureListWithEntryInGrid(SlaveList, NPC_RESCUED_SLAVE_ALLIANCE, 50.0f);
                    me->GetCreatureListWithEntryInGrid(SlaveList, NPC_RESCUED_SLAVE_HORDE, 50.0f);
                    if (!SlaveList.empty())
                        for (std::list<Creature*>::iterator itr = SlaveList.begin(); itr != SlaveList.end(); itr++)
                            (*itr)->DespawnOrUnsummon(2 * IN_MILLISECONDS);
                }            
            }
        }

        void UpdateAI(uint32 diff) override
        {
            const uint32 SPELL_DEVOUR_HUMANOID = 69503;

            SmartAI::UpdateAI(diff);

            if (me->GetVictim() && me->GetVictim()->GetTypeId() == TYPEID_UNIT && me->GetVictim()->getFaction() == 1770 
                && me->IsWithinMeleeRange(me->GetVictim()) && !me->IsNonMeleeSpellCast(true))
                me->CastSpell(me->GetVictim(), SPELL_DEVOUR_HUMANOID);

            if (!me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_1) && !me->IsInCombat() && me->GetMotionMaster()->GetCurrentMovementGeneratorType() != RANDOM_MOTION_TYPE)
                me->GetMotionMaster()->MoveRandom(5.f);
        }

        private:
            InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_pos_geist_ambusherAI(creature);
    }   
};

class at_pos_freed_slave : public AreaTriggerScript
{
public:
    at_pos_freed_slave() : AreaTriggerScript("at_pos_freed_slave") { }

    bool OnTrigger(Player* player, const AreaTriggerEntry* /*at*/)
    {
        InstanceScript* instance = player->GetInstanceScript();
        if (player->IsGameMaster() || !instance)
            return false;

        if (instance->GetBossState(DATA_GARFROST) == DONE)
        {            
            std::list<Creature*> SlaveList;
            player->GetCreatureListWithEntryInGrid(SlaveList, NPC_RESCUED_SLAVE_ALLIANCE, 50.0f);
            player->GetCreatureListWithEntryInGrid(SlaveList, NPC_RESCUED_SLAVE_HORDE,    50.0f);
            if (!SlaveList.empty())
                for (std::list<Creature*>::iterator itr = SlaveList.begin(); itr != SlaveList.end(); itr++)
                    (*itr)->AI()->SetData(1, 1);
            return true;
        }

        return false;
    }
};


class at_pos_wrathbone_skeleton_spawn : public AreaTriggerScript
{
public:
    at_pos_wrathbone_skeleton_spawn() : AreaTriggerScript("at_pos_wrathbone_skeleton_spawn") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/)
    {
        InstanceScript* instance = player->GetInstanceScript();
        if (!player->IsAlive())
            return false;

        if (instance->GetBossState(DATA_TYRANNUS) != DONE)
        { 
            if (!player->FindNearestCreature(NPC_WRATHBONE_SKELKETON, 10.0f))
            {
                if (Creature* Skeleton = player->SummonCreature(NPC_WRATHBONE_SKELKETON, player->GetPositionX() + 4, player->GetPositionY() + 4, player->GetPositionZ(), 0, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 220 * IN_MILLISECONDS))
                    Skeleton->AI()->AttackStart(player);
                if (Creature* Skeleton = player->SummonCreature(NPC_WRATHBONE_SKELKETON, player->GetPositionX() - 4, player->GetPositionY() + 4, player->GetPositionZ(), 0, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 220 * IN_MILLISECONDS))
                    Skeleton->AI()->AttackStart(player);
            }
        }                  
        return true;
    }
};

void AddSC_pit_of_saron()
{
    new npc_iceborn_protodrake();
    new spell_trash_npc_glacial_strike();
    new npc_pos_eye_of_the_lichking();
    new npc_pos_start_event();
    new npc_pos_corrupted_champion();
    new npc_pos_geist_ambusher();
    new at_pos_freed_slave();
    new npc_pos_cave_attacker();
    new at_pos_wrathbone_skeleton_spawn();
}
