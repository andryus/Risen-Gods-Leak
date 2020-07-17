/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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
#include "PassiveAI.h"
#include "BattlegroundIC.h"
#include "Player.h"
#include "Vehicle.h"
#include "SpellScript.h"
#include "SpellInfo.h"

class npc_four_car_garage : public CreatureScript
{
public:
    npc_four_car_garage() : CreatureScript("npc_four_car_garage") { }

    struct npc_four_car_garageAI : public NullCreatureAI
    {
        npc_four_car_garageAI(Creature* creature) : NullCreatureAI(creature)
        {
            me->SetRespawnTime(RESPAWN_3_MINUTES);
            me->SetRespawnDelay(RESPAWN_3_MINUTES);
            me->SetCorpseDelay(CORSPE_DESPAWN_5_SECONDS);
            me->setRegeneratingHealth(false);
        }

        void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply) override
        {
            if (apply)
            {
                uint32 spellId = 0;

                switch (me->GetEntry())
                {
                    case NPC_DEMOLISHER:
                        spellId = SPELL_DRIVING_CREDIT_DEMOLISHER;
                        break;
                    case NPC_GLAIVE_THROWER_A:
                    case NPC_GLAIVE_THROWER_H:
                        spellId = SPELL_DRIVING_CREDIT_GLAIVE;
                        /*break;
                    case NPC_SIEGE_ENGINE_H_RG:
                    case NPC_SIEGE_ENGINE_A_RG:*/
                        spellId = SPELL_DRIVING_CREDIT_SIEGE;
                        break;
                    case NPC_CATAPULT:
                        spellId = SPELL_DRIVING_CREDIT_CATAPULT;
                        break;
                    default:
                        return;
                }

                me->CastSpell(who, spellId, true);
                me->CastSpell(who, SPELL_DRIVING_CREDIT_GLAIVE, true);
            }
        }

        void JustDied(Unit* victim)
        {
            me->SetRespawnTime(RESPAWN_3_MINUTES);
            me->SetRespawnDelay(RESPAWN_3_MINUTES);
            me->SetCorpseDelay(CORSPE_DESPAWN_5_SECONDS);
            float x, y, z, o;
            me->GetHomePosition(x, y, z, o);
            me->NearTeleportTo(x, y, z, o);
            victim->CastSpell(victim, SPELL_DESTROYED_VEHICLE_ACHIEVEMENT, true);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_four_car_garageAI(creature);
    }
};

enum BossSpells
{
    SPELL_BRUTAL_STRIKE         = 58460,
    SPELL_DAGGER_THROW          = 67280,
    SPELL_CRUSHING_LEAP         = 68506,
    SPELL_RAGE                  = 66776,
};

enum BossEvents
{
    EVENT_BRUTAL_STRIKE     = 1,
    EVENT_DAGGER_THROW      = 2,
    EVENT_CRUSHING_LEAP     = 3,
    EVENT_CKECK_RANGE       = 4,
};

class npc_ic_horde_alliance_boss : public CreatureScript
{
public:
    npc_ic_horde_alliance_boss() : CreatureScript("npc_ic_horde_alliance_boss") { }

    struct npc_ic_horde_alliance_bossAI : public ScriptedAI
    {
        npc_ic_horde_alliance_bossAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            uint32 NPC_GUARD;
            me->GetEntry() == NPC_HIGH_COMMANDER_HALFORD_WYRMBANE ? NPC_GUARD = NPC_SEVEN_TH_LEGION_INFANTRY : NPC_GUARD = NPC_KOR_KRON_GUARD;

            std::list<Creature*> addList;
            me->GetCreatureListWithEntryInGrid(addList, NPC_GUARD, 100.0f);
            for (std::list<Creature*>::const_iterator itr = addList.begin(); itr != addList.end(); ++itr)
                (*itr)->Respawn();
        };

        void EnterCombat(Unit* /*who*/)
        {
            events.ScheduleEvent(EVENT_BRUTAL_STRIKE, 5*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_DAGGER_THROW,  7*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_CRUSHING_LEAP, 15*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_CKECK_RANGE,   1*IN_MILLISECONDS);
        }

        void SpellHit(Unit* caster, SpellInfo const* spell)
        {
            if (caster->IsVehicle())
                me->Kill(caster);
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
                    case EVENT_BRUTAL_STRIKE:
                        DoCastVictim(SPELL_BRUTAL_STRIKE);
                        events.ScheduleEvent(EVENT_BRUTAL_STRIKE, 5*IN_MILLISECONDS);
                        break;
                    case EVENT_DAGGER_THROW:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                            DoCast(target, SPELL_DAGGER_THROW);
                        events.ScheduleEvent(EVENT_DAGGER_THROW, 7*IN_MILLISECONDS);
                        break;
                    case EVENT_CRUSHING_LEAP:
                        DoCastVictim(SPELL_CRUSHING_LEAP);
                        events.ScheduleEvent(EVENT_CRUSHING_LEAP, 25*IN_MILLISECONDS);
                        break;
                    case EVENT_CKECK_RANGE:
                        if (me->GetDistance(me->GetHomePosition()) > 25.0f)
                            DoCastSelf(SPELL_RAGE);
                        else
                            me->RemoveAurasDueToSpell(SPELL_RAGE);
                        events.ScheduleEvent(EVENT_CKECK_RANGE, 1*IN_MILLISECONDS);
                        break;;
                }
            }
            DoMeleeAttackIfReady();
        }

    private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ic_horde_alliance_bossAI(creature);
    }
};

enum gunshipSpells
{
    SPELL_GUNSHIP_TELEPORT_HORDE        = 66637,
    SPELL_GUNSHIP_TELEPORT_ALLIANCE     = 66630
};

class npc_ic_gunship_cannon : public CreatureScript
{
public:
    npc_ic_gunship_cannon() : CreatureScript("npc_ic_gunship_cannon") {}

    struct npc_ic_gunship_cannonAI : public NullCreatureAI
    {
        npc_ic_gunship_cannonAI(Creature* creature) : NullCreatureAI(creature) { }

        void Reset()
        {
            // set unit state root to prevent moving, for unknown reason flag not move doesn't work
            me->SetControlled(true, UNIT_STATE_ROOT);
        }

        void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply)
        {
            if (!apply)
            {
                uint32 spellId;
                who->ToPlayer()->GetTeam() == TEAM_ALLIANCE ? spellId = SPELL_GUNSHIP_TELEPORT_ALLIANCE : spellId = SPELL_GUNSHIP_TELEPORT_HORDE;
                who->CastSpell(who, spellId, true);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ic_gunship_cannonAI(creature);
    }
};

enum Launch
{
    SPELL_LAUNCH_NO_FALLING_DAMAGE = 66251
};

class spell_gen_launch : public SpellScriptLoader
{
public:
    spell_gen_launch() : SpellScriptLoader("spell_gen_launch") {}

    class spell_gen_launch_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_gen_launch_SpellScript);

        void HandleOnCast()
        {
            if (Unit* vehicle = GetCaster())
            {
                if (Unit* player = vehicle->GetVehicleKit()->GetPassenger(0))
                {
                    player->ExitVehicle();
                    player->AddAura(SPELL_LAUNCH_NO_FALLING_DAMAGE, player); // prevents falling damage
                    playerGUID = player->GetGUID();
                }
            }
        }

        void HandleAfterCast()
        {
            if (Player* player = ObjectAccessor::GetPlayer(*GetCaster(), playerGUID))
            {
                Position pos;
                GetExplTargetDest()->GetPosition(&pos);
                float angle = player->GetRelativeAngle(pos.GetPositionX(), pos.GetPositionY());
                float dist = player->GetDistance(pos);
                float speedXY = 60.0f;
                float speedZ = 20.0f;

                bool checkCollision = true;
                if ((pos.m_positionX < Positi[A_MAX].x && pos.m_positionX > Positi[A_MIN].x && pos.m_positionY < Positi[A_MAX].y && pos.m_positionY > Positi[A_MIN].y) ||
                    (pos.m_positionX < Positi[H_MAX].x && pos.m_positionX > Positi[H_MIN].x && pos.m_positionY < Positi[H_MAX].y && pos.m_positionY > Positi[H_MIN].y))
                {
                    checkCollision = false;
                    speedXY = 30.0f;
                    speedZ = 60.0f;
                    player->CastSpell(player, SPELL_ACHIEVEMENT_BACK_DOOR_JOB, true);
                }
                if (checkCollision && !player->GetMap()->isInLineOfSight(pos.m_positionX, pos.m_positionY, pos.m_positionZ, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetPhaseMask()))
                    pos = player->GetFirstCollisionPosition(dist, angle);

                player->GetMotionMaster()->MoveJump(pos, speedXY, speedZ);
            }
        }

        void Register()
        {
            OnCast += SpellCastFn(spell_gen_launch_SpellScript::HandleOnCast);
            AfterCast += SpellCastFn(spell_gen_launch_SpellScript::HandleAfterCast);
        }

    private:
        ObjectGuid playerGUID;
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_gen_launch_SpellScript();
    }
};

class spell_gen_gunship_portal : public SpellScriptLoader
{
public:
    spell_gen_gunship_portal() : SpellScriptLoader("spell_gen_gunship_portal") { }

    class spell_gen_gunship_portal_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_gen_gunship_portal_SpellScript);

        bool Load()
        {
            return GetCaster()->GetTypeId() == TYPEID_PLAYER;
        }

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            Player* caster = GetCaster()->ToPlayer();
            if (Battleground* bg = caster->GetBattleground())
                if (bg->GetTypeID(true) == BATTLEGROUND_IC)
                    bg->DoAction(ACTION_TELEPORT_PLAYER_TO_TRANSPORT, caster->GetGUID());
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_gen_gunship_portal_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_gen_gunship_portal_SpellScript();
    }
};

enum parachuteIC
{
    SPELL_PARACHUTE_IC = 66657,
};

class spell_gen_parachute_ic : public SpellScriptLoader
{
public:
    spell_gen_parachute_ic() : SpellScriptLoader("spell_gen_parachute_ic") { }

    class spell_gen_parachute_ic_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_gen_parachute_ic_AuraScript)

        void HandleTriggerSpell(AuraEffect const* /*aurEff*/)
        {
            if (Player* target = GetTarget()->ToPlayer())
                if (target->m_movementInfo.fallTime > 2000)
                    target->CastSpell(target, SPELL_PARACHUTE_IC, true);
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_gen_parachute_ic_AuraScript::HandleTriggerSpell, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_gen_parachute_ic_AuraScript();
    }
};

class spell_ic_teleport_trigger : public SpellScriptLoader
{
public:
    spell_ic_teleport_trigger() : SpellScriptLoader("spell_ic_teleport_trigger") { }

    class spell_ic_teleport_trigger_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ic_teleport_trigger_SpellScript);

        void HandleOnCast()
        {
            if (Player* player = GetCaster()->ToPlayer())
                if (Unit* trigger = player->FindNearestCreature(NPC_WORLD_TRIGGER_LARGE_AOI_NOT_IMMUNE_PC_NPC_IC, 35.0f))
                    player->TeleportTo(trigger->GetMapId(), trigger->GetPositionX(), trigger->GetPositionY(), trigger->GetPositionZ(), trigger->GetOrientation());
        }

        void Register()
        {
            OnCast += SpellCastFn(spell_ic_teleport_trigger_SpellScript::HandleOnCast);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ic_teleport_trigger_SpellScript();
    }
};

enum AbombAchievments
{
    SPELL_HUGE_SEAFORIUM_BLAST  = 66672,
    SPELL_TINY_SEAFORIUM_BLAST  = 66676,

    SPELL_HUGE_BOMB_CREDIT      = 68367,
    SPELL_TINY_BOMB_CREDIT      = 68366
};

class spell_ic_seaforium_blast : public SpellScriptLoader
{
public:
    spell_ic_seaforium_blast() : SpellScriptLoader("spell_ic_seaforium_blast") { }

    class spell_ic_seaforium_blast_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ic_seaforium_blast_SpellScript);

        void HandleOnHit()
        {
            uint32 spellId;
            m_scriptSpellId == SPELL_HUGE_SEAFORIUM_BLAST ? spellId = SPELL_HUGE_BOMB_CREDIT : spellId = SPELL_TINY_BOMB_CREDIT;

            if (GameObject* go = GetHitGObj())
                if (go->GetGOValue()->Building.Health)
                    if (Unit* caster = GetOriginalCaster())
                        caster->CastSpell(caster, spellId, true);
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_ic_seaforium_blast_SpellScript::HandleOnHit);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ic_seaforium_blast_SpellScript();
    }
};

enum SeaforiumBombSpells
{
    SPELL_SEAFORIUM_BLAST        = 66676,
    //SPELL_HUGE_SEAFORIUM_BLAST = 66672,
    SPELL_A_BOMB_INABLE_CREDIT   = 68366,
    SPELL_A_BOMB_INATION_CREDIT  = 68367
};

class spell_ioc_seaforium_blast_credit : public SpellScriptLoader
{
    public:
        spell_ioc_seaforium_blast_credit() : SpellScriptLoader("spell_ioc_seaforium_blast_credit") { }
    
        class spell_ioc_seaforium_blast_credit_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ioc_seaforium_blast_credit_SpellScript);
    
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_A_BOMB_INABLE_CREDIT) || !sSpellMgr->GetSpellInfo(SPELL_A_BOMB_INATION_CREDIT))
                    return false;
                return true;
            }
    
            void HandleAchievementCredit(SpellEffIndex /*effIndex*/)
            {
                uint32 _creditSpell = 0;
                Unit* caster = GetOriginalCaster();
                if (!caster)
                    return;
    
                if (GetSpellInfo()->Id == SPELL_SEAFORIUM_BLAST)
                    _creditSpell = SPELL_A_BOMB_INABLE_CREDIT;
                else if (GetSpellInfo()->Id == SPELL_HUGE_SEAFORIUM_BLAST)
                    _creditSpell = SPELL_A_BOMB_INATION_CREDIT;
    
                if (GetHitGObj() && GetHitGObj()->IsDestructibleBuilding())
                    caster->CastSpell(caster, _creditSpell, true);
            }
    
            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_ioc_seaforium_blast_credit_SpellScript::HandleAchievementCredit, EFFECT_1, SPELL_EFFECT_GAMEOBJECT_DAMAGE);
            }
        };
    
        SpellScript* GetSpellScript() const
        {
            return new spell_ioc_seaforium_blast_credit_SpellScript();
        }
};

void AddSC_isle_of_conquest()
{
    new npc_four_car_garage();
    new npc_ic_horde_alliance_boss();
    new npc_ic_gunship_cannon();

    new spell_gen_launch();
    new spell_gen_parachute_ic();
    new spell_gen_gunship_portal();
    new spell_ic_teleport_trigger();
    new spell_ic_seaforium_blast();
    new spell_ioc_seaforium_blast_credit();
}
