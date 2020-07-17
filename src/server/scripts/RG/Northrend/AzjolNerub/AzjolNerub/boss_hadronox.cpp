
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

// TODO: - nonelite spells
//       - spawn 2 more crusher groups

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "azjol_nerub.h"

enum Spells
{
    SPELL_ACID_CLOUD          = 53400,
    H_SPELL_ACID_CLOUD        = 59419,
    SPELL_LEECH_POISON        = 53030,
    SPELL_LEECH_POISON_PCT    = 53800,
    H_SPELL_LEECH_POISON      = 59417,
    SPELL_PIERCE_ARMOR        = 53418,
    SPELL_WEB_GRAB            = 57731,
    H_SPELL_WEB_GRAB          = 59421,
    SPELL_WEB_FRONT_DOORS     = 53177, //visual
    SPELL_WEB_SIDE_DOORS      = 53185, //visual
    // anubar crusher
    SPELL_FRENZY              = 53801,
    SPELL_SMASH               = 53318,
    H_SPELL_SMASH             = 59346,
    //anubar nerubian
    SPELL_CRUSHING_WEBS       = 53322,
    SPELL_CRUSHING_WEBS_H     = 59347,
    SPELL_INFECTED_WOUNDS     = 53330,
    SPELL_INFECTED_WOUNDS_H   = 59348,
    SPELL_ANIMATED_BONES      = 53334
};

enum Creatures
{
    NPC_ANUBAR_CHAMPION       = 29117,
    NPC_ANUBAR_CRYPT_FIEND    = 29118,
    NPC_ADD_CHAMPION          = 29062,
    NPC_ADD_CRYPT_FIEND       = 29063,
    NPC_ADD_NECROMANCER       = 29064,
    NPC_WORLD_TRIGGER         = 23472
};

enum Events
{
    EVENT_CLOUD = 1,
    EVENT_LEECH,
    EVENT_PIECRE,
    EVENT_GRAB,
    EVENT_SPAWN_1,
    EVENT_SPAWN_2,
    EVENT_SPAWN_3
};

enum Actions
{
    ACTION_EVENT_START      = 1,
};

Position const HadronoxWaypoints[26] =
{
    {563.284f, 513.756f, 697.139f, 5.97266f}, // 1
    {578.625f, 511.529f, 698.386f, 0.021696f},// 2
    {596.352f, 511.913f, 695.054f, 0.021696f},// 3
    {610.962f, 516.671f, 695.35f, 0.728555f}, // 4
    {618.175f, 526.986f, 698.047f, 1.19979f}, // 5
    {619.764f, 541.707f, 707.337f, 2.06452f}, // 6
    {611.146f, 559.05f, 717.123f, 2.02132f},  // 7
    {601.868f, 575.858f, 723.168f, 2.16662f}, // 8
    {590.636f, 593.436f, 735.498f, 1.94514f}, // 9 - close first door
    {574.33f, 585.284f, 728.232f, 3.9047f},   // 10
    {556.293f, 567.65f, 729.233f, 3.72013f},  // 12
    {543.171f, 566.961f, 731.389f, 3.00935f}, // 13
    {533.621f, 579.485f, 733.449f, 1.68124f}, // 14 - close last doors
    {527.92f, 555.761f, 732.456f, 4.69717f},  // 15

};

Position const SpawnPos[3] =
{
    {582.961304f, 606.298645f, 739.3f, 0.0f},
    {575.927f, 611.744f, 771.424f, 0.0f},
    {483.05f, 612.804f, 771.481f, 0.0f},
};

class boss_hadronox : public CreatureScript
{
public:
    boss_hadronox() : CreatureScript("boss_hadronox") { }

    struct boss_hadronoxAI : public BossAI
    {
        boss_hadronoxAI(Creature* creature) : BossAI(creature, BOSS_HADRONOX)
        {
            home = creature->GetHomePosition();
            HadronoxEventMovement = false;
            AchivementHadronoxDenied = false;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetImmuneToAll(true);
            me->SetReactState(REACT_PASSIVE);
        }

        ObjectGuid Trigger2GUID;
        ObjectGuid Trigger3GUID;

        void Reset() override
        {
            Trigger2GUID = ObjectGuid((uint64)127378);
            Trigger3GUID = ObjectGuid((uint64)127377);
        }

        void DoAction(int32 action) override
        {
            if (action == ACTION_EVENT_START && !HadronoxEventMovement)
            {
                HadronoxEventMovement = true;
                AchivementHadronoxDenied = true;

                events.ScheduleEvent(EVENT_CLOUD, urand(5, 10) *IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_LEECH, urand(7, 12) *IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SPAWN_1, 1*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SPAWN_2, 1*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SPAWN_3, 1*IN_MILLISECONDS);
                me->GetMotionMaster()->MovePath(me->GetSpawnId() * 10, false);

                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetImmuneToAll(false);
                me->SetReactState(REACT_PASSIVE);
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != WAYPOINT_MOTION_TYPE)
                return;

            if (id == 8)
            {
                if (Creature* WorldTrigger = me->FindNearestCreature(NPC_WORLD_TRIGGER, 25.0f))
                {
                    me->AddAura(SPELL_WEB_FRONT_DOORS, WorldTrigger);
                    me->AddAura(SPELL_WEB_SIDE_DOORS, WorldTrigger);
                }
                events.CancelEvent(EVENT_SPAWN_1);
            }
            else if (id == 13)
            {
                if (Creature* WorldTrigger = ObjectAccessor::GetCreature(*me, Trigger2GUID))
                {
                    me->AddAura(SPELL_WEB_FRONT_DOORS, WorldTrigger);
                    me->AddAura(SPELL_WEB_SIDE_DOORS, WorldTrigger);
                }
                if (Creature* WorldTrigger = ObjectAccessor::GetCreature(*me, Trigger3GUID))
                {
                    me->AddAura(SPELL_WEB_FRONT_DOORS, WorldTrigger);
                    me->AddAura(SPELL_WEB_SIDE_DOORS, WorldTrigger);
                }
                events.CancelEvent(EVENT_SPAWN_2);
                events.CancelEvent(EVENT_SPAWN_3);
                AchivementHadronoxDenied = false;
            }

            if (id == 14)
                me->SetReactState(REACT_AGGRESSIVE);
        }

        void DamageTaken(Unit* who, uint32& /*Damage*/) override
        {
            if (who->GetTypeId() == TYPEID_PLAYER && HadronoxEventMovement)
            {
                BossAI::EnterCombat(who);

                HadronoxEventMovement = false;
                me->SetReactState(REACT_AGGRESSIVE);
                AddThreat(who, 100.0f);
                me->Attack(who, true);
                events.ScheduleEvent(EVENT_PIECRE, urand(10, 15) *IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_GRAB, urand(15, 20) *IN_MILLISECONDS);
            }
        }

        uint32 GetData(uint32 /*type*/) const
        {
            if (DATA_DENIED)
                return AchivementHadronoxDenied ? 1 : 0;

            return 0;
        }

        void UpdateAI(uint32 diff) override
        {
            if (!HadronoxEventMovement)
            {
                if (!UpdateVictim())
                    return;
            }
            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CLOUD:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 60, false))
                            DoCast(target, DUNGEON_MODE(SPELL_ACID_CLOUD, H_SPELL_ACID_CLOUD));
                        events.ScheduleEvent(EVENT_CLOUD, urand(25, 35) *IN_MILLISECONDS);
                        break;
                    case EVENT_LEECH:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 20, false))
                            DoCast(target, DUNGEON_MODE(SPELL_LEECH_POISON, H_SPELL_LEECH_POISON));
                        events.ScheduleEvent(EVENT_LEECH, urand(10, 20) *IN_MILLISECONDS);
                        break;
                    case EVENT_PIECRE:
                        DoCastVictim(SPELL_PIERCE_ARMOR);
                        events.ScheduleEvent(EVENT_PIECRE, urand(15, 20) *IN_MILLISECONDS);
                        break;
                    case EVENT_GRAB:
                        DoCastSelf(DUNGEON_MODE(SPELL_WEB_GRAB, H_SPELL_WEB_GRAB));
                        DoCastSelf(DUNGEON_MODE(SPELL_ACID_CLOUD, H_SPELL_ACID_CLOUD));
                        events.ScheduleEvent(EVENT_GRAB, urand(17, 23) *IN_MILLISECONDS);
                        break;
                    case EVENT_SPAWN_1:
                        if (Creature* nerubian1 = me->SummonCreature(RAND(NPC_ADD_CHAMPION, NPC_ADD_CRYPT_FIEND, NPC_ADD_NECROMANCER), SpawnPos[0]))
                            nerubian1->AI()->SetData(1, 0);
                        events.ScheduleEvent(EVENT_SPAWN_1, urand(10000, 15000));
                        break;
                    case EVENT_SPAWN_2:
                        if (Creature* nerubian2 = me->SummonCreature(RAND(NPC_ADD_CHAMPION, NPC_ADD_CRYPT_FIEND, NPC_ADD_NECROMANCER), SpawnPos[1]))
                            nerubian2->AI()->SetData(2, 0);
                        events.ScheduleEvent(EVENT_SPAWN_2, urand(10000, 15000));
                        break;
                    case EVENT_SPAWN_3:
                        if (Creature* nerubian3 = me->SummonCreature(RAND(NPC_ADD_CHAMPION, NPC_ADD_CRYPT_FIEND, NPC_ADD_NECROMANCER), SpawnPos[2]))
                            nerubian3->AI()->SetData(3, 0);
                        events.ScheduleEvent(EVENT_SPAWN_3, urand(10000, 15000));
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    private:
        Position home;
        bool HadronoxEventMovement;
        bool AchivementHadronoxDenied;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_hadronoxAI(creature);
    }
};

class npc_anubar_crusher : public CreatureScript
{
public:
    npc_anubar_crusher() : CreatureScript("npc_anubar_crusher") { }

    struct npc_anubar_crusherAI : public ScriptedAI
    {
        npc_anubar_crusherAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset() override
        {
            smashTimer = urand(7, 10) *IN_MILLISECONDS;

            if (Creature* champion = me->FindNearestCreature(NPC_ANUBAR_CHAMPION, 200.0f, false))
                champion->Respawn();
            if (Creature* cryptFiend = me->FindNearestCreature(NPC_ANUBAR_CRYPT_FIEND, 200.0f, false))
                cryptFiend->Respawn();
        }

        void JustDied(Unit* /*killer*/) override
        {
            if (Creature* hadronox = ObjectAccessor::GetCreature(*me, ObjectGuid(instance ? instance->GetGuidData(DATA_HADRONOX) : ObjectGuid::Empty)))
                hadronox->AI()->DoAction(ACTION_EVENT_START);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (smashTimer <= diff)
            {
                DoCastVictim(DUNGEON_MODE(SPELL_SMASH, H_SPELL_SMASH));
                smashTimer = urand(10, 15) *IN_MILLISECONDS;
            }
            else
                smashTimer -= diff;

            if (HealthBelowPct(30) && !me->HasAura(SPELL_FRENZY))
            {
                me->InterruptNonMeleeSpells(true);
                DoCastSelf(SPELL_FRENZY, true);
            }

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        uint32 smashTimer;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_anubar_crusherAI(creature);
    }
};

class npc_hadronox_nerubian : public CreatureScript
{
public:
    npc_hadronox_nerubian() : CreatureScript("npc_hadronox_nerubian") { }

    struct npc_hadronox_nerubianAI : public npc_escortAI
    {
        npc_hadronox_nerubianAI(Creature* creature) : npc_escortAI(creature)
        {
            instance = creature->GetInstanceScript();
            WaypointPath = 0;
        }

        void Reset() override
        {
            crushingWebsTimer = urand(4000, 8000);
            infectedWoundTimer = urand(8000, 12000);
            CombatTimer = 2000;
        }

        void SetData(uint32 type, uint32 /*Data*/) override
        {
            switch (type)
            {
                case 1:
                    AddWaypoint(0, 582.961304f, 606.298645f, 739.3f);
                    AddWaypoint(1, 590.240662f, 597.044556f, 739.2f);
                    AddWaypoint(2, 600.471436f, 585.080200f, 726.2f);
                    AddWaypoint(3, 611.900940f, 562.884094f, 718.9f);
                    AddWaypoint(4, 615.533936f, 529.052002f, 703.3f);
                    AddWaypoint(5, 606.844604f, 515.054199f, 696.2f);
                    WaypointPath = 1;
                    break;
                case 2:
                    AddWaypoint(0, 575.927f, 611.744f, 771.424f);
                    AddWaypoint(1, 569.373f, 606.833f, 764.575f);
                    AddWaypoint(2, 559.436f, 598.693f, 751.406f);
                    AddWaypoint(3, 546.261f, 587.177f, 733.661f);
                    AddWaypoint(4, 535.264f, 576.75f, 732.952f);
                    AddWaypoint(5, 540.986f, 567.946f, 731.816f);
                    AddWaypoint(6, 551.609f, 556.838f, 730.631f);
                    AddWaypoint(7, 566.687f, 566.713f, 727.047f);
                    AddWaypoint(8, 578.086f, 575.827f, 726.81f);
                    AddWaypoint(9, 587.852f, 573.983f, 725.422f);
                    AddWaypoint(10, 600.94f, 565.22f, 721.072f);
                    AddWaypoint(11, 610.655f, 552.827f, 713.895f);
                    AddWaypoint(12, 619.238f, 539.782f, 706.51f);
                    AddWaypoint(13, 617.304f, 523.208f, 695.573f);
                    AddWaypoint(14, 607.33f, 514.405f, 695.915f);
                    WaypointPath = 2;
                    break;
                case 3:
                    AddWaypoint(0, 483.05f, 612.804f, 771.481f);
                    AddWaypoint(1, 488.406f, 607.468f, 767.194f);
                    AddWaypoint(2, 495.653f, 601.625f, 757.751f);
                    AddWaypoint(3, 506.101f, 593.227f, 744.16f);
                    AddWaypoint(4, 516.984f, 584.42f, 735.67f);
                    AddWaypoint(5, 524.408f, 577.811f, 734.255f);
                    AddWaypoint(6, 538.342f, 565.406f, 732.069f);
                    AddWaypoint(7, 548.83f, 561.896f, 730.625f);
                    AddWaypoint(8, 562.411f, 565.433f, 727.914f);
                    AddWaypoint(9, 568.807f, 577.252f, 727.871f);
                    AddWaypoint(10, 576.23f, 580.63f, 727.511f);
                    AddWaypoint(11, 590.901f, 576.855f, 725.493f);
                    AddWaypoint(12, 603.32f, 568.109f, 721.131f);
                    AddWaypoint(13, 608.709f, 555.798f, 715.537f);
                    AddWaypoint(14, 620.222f, 545.051f, 708.971f);
                    AddWaypoint(15, 622.207f, 532.963f, 702.121f);
                    AddWaypoint(16, 613.537f, 520.764f, 695.447f);
                    AddWaypoint(17, 605.593f, 513.019f, 695.244f);
                    WaypointPath = 3;
                    break;
            }
            if (type <= 3)
            {
                //if is in combat, stop combat, to can start escort
                if (me->IsInCombat())
                    me->CombatStop(true);
                Start(true, true, ObjectGuid::Empty, NULL);
                SetDespawnAtEnd(false);
            }
        }

        void WaypointReached(uint32 /*WaypointId*/) override { }

        void MoveInLineOfSight(Unit* who) override
        {
            if (who->GetEntry() == NPC_HADRONOX && who->GetExactDist2d(me->GetPositionX(), me->GetPositionY()) <= 10.0f)
            {
                AddThreat(who, 1.0f);
                me->Attack(who, true);
                me->GetMotionMaster()->MoveChase(who);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            npc_escortAI::UpdateAI(diff);

            if (CombatTimer <= diff)
            {
                if (Creature* hadronox = me->FindNearestCreature(NPC_HADRONOX, 10.0f))
                {
                    AddThreat(hadronox, 1.0f);
                    me->Attack(hadronox, true);
                    me->GetMotionMaster()->MoveChase(hadronox);
                }
                CombatTimer = 2000;
            }
            else
                CombatTimer -= diff;

            if (!UpdateVictim())
                return;

            if (crushingWebsTimer <= diff)
            {
                DoCastVictim(DUNGEON_MODE(SPELL_CRUSHING_WEBS, SPELL_CRUSHING_WEBS_H));
                crushingWebsTimer = urand(9, 14)*IN_MILLISECONDS;
            }
            else
                crushingWebsTimer -= diff;

            if (infectedWoundTimer <= diff)
            {
                DoCastVictim(DUNGEON_MODE(SPELL_INFECTED_WOUNDS, SPELL_INFECTED_WOUNDS_H));
                infectedWoundTimer = urand(5, 7)*IN_MILLISECONDS;
            }
            else
                infectedWoundTimer -= diff;

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        uint32 WaypointPath;

        uint32 crushingWebsTimer;
        uint32 infectedWoundTimer;
        uint32 CombatTimer;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_hadronox_nerubianAI(creature);
    }
};

class spell_hadronox_leech_poison : public SpellScriptLoader
{
public:
    spell_hadronox_leech_poison() : SpellScriptLoader("spell_hadronox_leech_poison") { }

    class spell_hadronox_leech_poison_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_hadronox_leech_poison_AuraScript);

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* caster = GetCaster();
            if (caster && caster->IsAlive() && GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_DEATH)
                caster->CastSpell(caster, SPELL_LEECH_POISON_PCT, true);
        }

        void Register()
        {
            AfterEffectRemove += AuraEffectRemoveFn(spell_hadronox_leech_poison_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_LEECH, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_hadronox_leech_poison_AuraScript();
    }
};

class achievement_hadronox_denied : public AchievementCriteriaScript
{
public:
    achievement_hadronox_denied() : AchievementCriteriaScript("achievement_hadronox_denied") { }

    bool OnCheck(Player* /*player*/, Unit* target)
    {
        if (!target)
            return false;

        if (Creature* hadronox = target->ToCreature())
            if (hadronox->AI()->GetData(DATA_DENIED))
                return true;

        return false;
    }
};

class npc_animated_bones : public CreatureScript
{
public:
    npc_animated_bones() : CreatureScript("npc_animated_bones") { }
    
    struct npc_animated_bonesAI : public ScriptedAI
    {
        npc_animated_bonesAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->SetMaxHealth(DUNGEON_MODE(1988, 3258));
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_animated_bonesAI(creature);
    }
};

void AddSC_boss_hadronox()
{
    new boss_hadronox();
    new npc_anubar_crusher();
    new npc_hadronox_nerubian();
    new spell_hadronox_leech_poison();
    new achievement_hadronox_denied();
    new npc_animated_bones();
}
