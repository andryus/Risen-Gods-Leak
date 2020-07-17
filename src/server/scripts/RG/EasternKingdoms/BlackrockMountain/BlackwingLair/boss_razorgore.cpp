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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "blackwing_lair.h"
#include "Player.h"
#include "MoveSpline.h"

enum Say
{
    SAY_EGGS_BROKEN1        = 0,
    SAY_EGGS_BROKEN2        = 1,
    SAY_EGGS_BROKEN3        = 2,
    SAY_DEATH               = 3,
};

enum SayGrethok
{
    SAY_ENTER_COMBAT        = 0
};

enum SaySpawns
{
    SAY_TROOPS_EVADE        = 0
};

enum Spells
{
    // Orb & Misc
    SPELL_MINDCONTROL       = 19832,
    SPELL_MINDCONTROL_DEBUFF= 23014,
    SPELL_MIND_EXHAUST      = 23958,
    SPELL_ORB_EXPLOSION     = 20037,
    
    // Razorgore
    SPELL_EGG_DESTROY       = 19873,

    SPELL_CLEAVE            = 19632,
    SPELL_WARSTOMP          = 24375,
    SPELL_FIREBALLVOLLEY    = 22425,
    SPELL_CONFLAGRATION     = 23023,

    // Grethok the Controller
    SPELL_POLYMORPH         = 22274,
    SPELL_CHARM             = 14515,
    SPELL_SLOW              = 13747,
    SPELL_ARCANE_MISSLES    = 22273,
};

enum Summons
{
    NPC_ELITE_DRACHKIN      = 12422,
    NPC_ELITE_WARRIOR       = 12458,
    NPC_WARRIOR             = 12416,
    NPC_MAGE                = 12402,
    NPC_WARLOCK             = 12459,

    NPC_NEFARIANS_TROOPS    = 14459,
};

enum EVENTS
{
    // Razorgore
    EVENT_CLEAVE = 1,
    EVENT_STOMP,
    EVENT_FIREBALL,
    EVENT_CONFLAGRATION,

    // Grethok the Controller
    EVENT_POLYMORPH,
    EVENT_CHARM,
    EVENT_SLOW,
    EVENT_ARCANE_MISSLES,
};

Position const SummonPosition[8] =
{
    { -7661.207520f, -1043.268188f, 407.199554f, 6.280452f },
    { -7608.501953f, -1116.077271f, 407.199921f, 0.816443f },
    { -7531.841797f, -1063.765381f, 407.199615f, 2.874187f },
    { -7584.175781f, -989.6691289f, 407.199585f, 4.527447f },
    { -7568.547852f, -1013.112488f, 407.204926f, 3.773467f },
    { -7547.319336f, -1040.971924f, 407.205078f, 3.789175f },
    { -7624.260742f, -1095.196899f, 407.205017f, 0.544694f },
    { -7644.145020f, -1065.628052f, 407.204956f, 0.501492f },
};

Position const RoomExitPos = { -7554.37f, -1024.46f, 408.48f };

Position const RazorgoreOrcPositions[3] =
{
    { -7618.29f, -1021.42f, 413.56f, 5.28f },
    { -7624.0f, -1022.74f, 413.56f, 5.31f, },
    { -7615.21f, -1016.6f, 413.56f, 5.31f, },
};

Position const EggPositions[30] =
{
    { -7549.48f, -1069.96f, 408.49f, 5.75959f },
    { -7554.42f, -1061.5f, 408.49f, 3.99681f },
    { -7563.15f, -1088.7f, 413.381f, 5.8294f },
    { -7564.89f, -1058.87f, 408.49f, 2.28638f },
    { -7566.0f, -1045.93f, 408.49f, 3.05433f },
    { -7568.27f, -1097.68f, 413.381f, 2.79252f },
    { -7568.62f, -1086.58f, 413.381f, 0.85521f },
    { -7569.38f, -1079.73f, 413.381f, 3.59538f },
    { -7572.49f, -1095.03f, 413.381f, 3.42085f },
    { -7576.92f, -1083.69f, 413.381f, 3.38594f },
    { -7577.84f, -1035.97f, 408.49f, 5.16618f },
    { -7578.64f, -1089.95f, 413.381f, 2.21656f },
    { -7579.49f, -1051.48f, 408.157f, 0.523598f },
    { -7580.8f, -1067.29f, 408.49f, 3.29869f },
    { -7584.68f, -1075.84f, 408.49f, 3.01941f },
    { -7586.37f, -1024.43f, 408.49f, 3.35105f },
    { -7588.84f, -1053.79f, 408.157f, 4.55531f },
    { -7592.35f, -1010.84f, 408.49f, 3.73501f },
    { -7592.38f, -1035.68f, 408.157f, 1.62316f },
    { -7594.37f, -1102.9f, 408.49f, 5.37562f },
    { -7597.53f, -1094.54f, 408.49f, 2.37364f },
    { -7599.0f, -1044.77f, 408.157f, 5.25344f },
    { -7601.14f, -1077.11f, 408.218f, 5.0091f },
    { -7604.36f, -1060.24f, 408.157f, 3.50812f },
    { -7609.94f, -1035.11f, 408.49f, 4.34587f },
    { -7611.6f, -1020.32f, 413.381f, 3.08918f },
    { -7618.1f, -1069.33f, 408.49f, 4.95674f },
    { -7619.76f, -1058.94f, 408.49f, 1.81514f },
    { -7626.69f, -1011.71f, 413.381f, 0.226893f },
    { -7628.32f, -1044.57f, 408.49f, 6.10865f },
};

class boss_razorgore : public CreatureScript
{
    public:
        boss_razorgore() : CreatureScript("boss_razorgore") { }
    
        struct boss_razorgoreAI : public BossAI
        {
            boss_razorgoreAI(Creature* creature) : BossAI(creature, BOSS_RAZORGORE), initialized(false) { }
    
            void OnCharmed(bool apply) override
            {
                if (!apply)
                {
                    me->DeleteCharmInfo(); // HACK should be called by core
                    me->SetInCombatWithZone();
                }
                else
                    me->StopMoving();
            }

            void AttackStart(Unit* target) override
            {
                if (auto creature = target->ToCreature())
                {
                    if (!creature->GetCharmerOrOwnerPlayerOrPlayerItself() &&
                        creature->GetEntry() != NPC_BLACKWING_DRAGON && creature->GetEntry() != NPC_BLACKWING_TASKMASTER &&
                        creature->GetEntry() != NPC_BLACKWING_LEGIONAIRE && creature->GetEntry() != NPC_BLACKWING_WARLOCK)
                        return;
                }

                if (me->IsCharmed())
                    me->Attack(target, true);
                else
                    BossAI::AttackStart(target);
            }
    
            void Reset() override
            {
                BossAI::Reset();
                ResetEncounter();
                secondPhase = false;
                waveCount = 0;
                me->ApplySpellImmune(0, IMMUNITY_ID, 19872, true); // immune to self stun
            }

            void JustReachedHome() override
            {
                BossAI::JustReachedHome();
                initialized = false;
                ResetEncounter();
            }

            void JustSummoned(Creature* summon) override
            {
                BossAI::JustSummoned(summon);
                summon->SetInCombatWithZone();
                if (auto charmer = me->GetCharmer())
                    summon->AI()->AttackStart(charmer);
            }

            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
                events.ScheduleEvent(EVENT_CLEAVE, 15 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_STOMP, 35 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FIREBALL, 7 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_CONFLAGRATION, 12 * IN_MILLISECONDS);
                waveTimer.Reset(30 * IN_MILLISECONDS);

                if (InstanceScript* instance = me->GetInstanceScript())
                    if (Creature* grethok = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_GRETHOK_THE_CONTROLLER)))
                        grethok->AI()->AttackStart(who);
            }
    
            void DoChangePhase()
            {
                events.RescheduleEvent(EVENT_CLEAVE,        15 * IN_MILLISECONDS);
                events.RescheduleEvent(EVENT_STOMP,         35 * IN_MILLISECONDS);
                events.RescheduleEvent(EVENT_FIREBALL,       7 * IN_MILLISECONDS);
                events.RescheduleEvent(EVENT_CONFLAGRATION, 12 * IN_MILLISECONDS);

                secondPhase = true;
                me->RemoveAllAuras();
                me->LoadCreaturesAddon();
                me->SetHealth(me->GetMaxHealth());
                me->ResetPlayerDamageReq();
                me->SetInCombatWithZone();
                if (Creature* trigger = me->SummonCreature(NPC_NEFARIANS_TROOPS, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()))
                    trigger->AI()->Talk(SAY_TROOPS_EVADE);

                for (auto var : summons)
                    if (Creature* summon = ObjectAccessor::GetCreature(*me, var)) {
                        if (summon->isDead())
                            continue;
                        if (summon->GetEntry() != NPC_BLACKWING_DRAGON && summon->GetEntry() != NPC_BLACKWING_TASKMASTER &&
                            summon->GetEntry() != NPC_BLACKWING_LEGIONAIRE && summon->GetEntry() != NPC_BLACKWING_WARLOCK)
                            continue;
                        summon->SetHomePosition(RoomExitPos);
                        summon->AI()->EnterEvadeMode();
                        summon->DespawnOrUnsummon(summon->movespline->Duration());
                    }
            }

            uint32 GetDragonCount()
            {
                return std::count_if(summons.begin(), summons.end(), [this](ObjectGuid itr)
                {
                    if (auto summon = sObjectAccessor->GetCreature(*me, itr))
                        return summon->GetEntry() == NPC_BLACKWING_DRAGON && summon->IsAlive();
                    return false;
                });
            }

            uint32 GetOrcCount()
            {
                return std::count_if(summons.begin(), summons.end(), [this](ObjectGuid itr)
                {
                    if (auto summon = sObjectAccessor->GetCreature(*me, itr))
                        return (summon->GetEntry() == NPC_BLACKWING_WARLOCK || summon->GetEntry() == NPC_BLACKWING_TASKMASTER
                            || summon->GetEntry() == NPC_BLACKWING_LEGIONAIRE) && summon->IsAlive();
                    return false;
                });
            }

            void DoSpawnWave()
            {
                if (secondPhase)
                    return;

                uint32 entrys[] = { NPC_BLACKWING_DRAGON, NPC_BLACKWING_WARLOCK, NPC_BLACKWING_TASKMASTER, NPC_BLACKWING_LEGIONAIRE };
                uint32 spawnPerWave = waveCount < 2 ? 4 : 8;
                uint32 dragonSpawnCount = waveCount == 0 ? urand(0, 1) : urand(1, 4);
                dragonSpawnCount = std::min(dragonSpawnCount, 12 - GetDragonCount()); // maximum at 8 dragons at the same time
                uint32 OrcCount = GetOrcCount();
                for (uint32 spawnPos = 0; spawnPos < spawnPerWave; spawnPos++)
                {
                    bool spawnOrc = spawnPos >= dragonSpawnCount;

                    if (spawnOrc && OrcCount >= 40)
                        break;

                    if (spawnPos >= 4 && roll_chance_i(25))
                        continue;

                    me->SummonCreature(entrys[spawnOrc ? urand(1, 3) : 0], SummonPosition[spawnPos]);
                    if (spawnOrc)
                        OrcCount++;
                }
                ++waveCount;
                waveTimer.Reset(15 * IN_MILLISECONDS);
            }

            void DamageTaken(Unit* /*who*/, uint32& damage) override
            {
                if (!secondPhase && damage >= me->GetHealth()) {
                    damage = 0;
                    me->SetHealth(me->GetMaxHealth());
                    me->RemoveAllAuras();
                    EnterEvadeMode();
                    Talk(SAY_DEATH);
                    DoCastSelf(SPELL_ORB_EXPLOSION);
                }
            }
    
            void ResetEncounter()
            {
                if (initialized)
                    return;

                initialized = true;

                for (auto itr = eggList.begin(); itr != eggList.end(); ++itr)
                    if (GameObject* egg = sObjectAccessor->GetGameObject(*me, *itr))
                        egg->RemoveFromWorld();
                eggList.clear();

                if (Creature* grethok = me->SummonCreature(NPC_GRETHOK_THE_CONTROLLER, RazorgoreOrcPositions[0])) {
                    me->SummonCreature(NPC_BLACKWING_GUARDSMAN, RazorgoreOrcPositions[1]);
                    me->SummonCreature(NPC_BLACKWING_GUARDSMAN, RazorgoreOrcPositions[2]);

                    for (uint8 i = 0; i < 30; ++i)
                    {
                        if (auto egg = me->SummonGameObject(GO_RAZORGORE_EGG, EggPositions[i].GetPositionX(), EggPositions[i].GetPositionY(),
                            EggPositions[i].GetPositionZ(), EggPositions[i].GetOrientation(), 0, 0, 0, 0, 0)) {
                            if (roll_chance_i(30)) {
                                Position pos;
                                egg->GetPosition(&pos);
                                me->SummonCreature(NPC_BLACKWING_SPELLMARKER, pos);
                            }
                            eggList.insert(egg->GetGUID());
                        }
                    }
                }
            }

            void OnEggDestroyed(GameObject* egg)
            {
                if (!egg)
                    return;

                auto itr = eggList.find(egg->GetGUID());
                if (itr != eggList.end())
                {
                    eggList.erase(itr);
                    if (eggList.size() <= 0)
                        DoChangePhase();
                }

                egg->RemoveFromWorld();
            }

            uint32 GetData(uint32 index) const override
            {
                return secondPhase;
            }

            void UpdateAI(uint32 diff) override
            {
                if (me->IsInCombat() || me->IsCharmed())
                {
                    waveTimer.Update(diff);
                    if (waveTimer.Passed())
                        DoSpawnWave();
                }

                if (me->IsCharmed())
                {
                    if (me->GetVictim())
                    {
                        if (!me->IsValidAttackTarget(me->GetVictim()))
                            me->AttackStop();
                        else
                            DoMeleeAttackIfReady();
                    }
                    return;
                }
                
                events.Update(diff);
    
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
    
                if (!UpdateVictim())
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CLEAVE:
                            DoCastVictim(SPELL_CLEAVE);
                            events.ScheduleEvent(EVENT_CLEAVE, urand(7, 10) * IN_MILLISECONDS);
                            break;
                        case EVENT_STOMP:
                            DoCastVictim(SPELL_WARSTOMP);
                            events.ScheduleEvent(EVENT_STOMP, urand(15, 25) * IN_MILLISECONDS);
                            break;
                        case EVENT_FIREBALL:
                            DoCastVictim(SPELL_FIREBALLVOLLEY);
                            events.ScheduleEvent(EVENT_FIREBALL, urand(12, 15) * IN_MILLISECONDS);
                            break;
                        case EVENT_CONFLAGRATION:
                            DoCastVictim(SPELL_CONFLAGRATION);
                            if (me->GetVictim() && me->GetVictim()->HasAura(SPELL_CONFLAGRATION))
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 100, true))
                                    me->TauntApply(target);
                            events.ScheduleEvent(EVENT_CONFLAGRATION, 30 * IN_MILLISECONDS);
                            break;
                    }
                }
                
                DoMeleeAttackIfReady();
            }
    
        private:
            bool initialized;
            bool secondPhase;
            TimeTrackerSmall waveTimer;
            uint32 waveCount;
            std::unordered_set<ObjectGuid> eggList;
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_razorgoreAI>(creature);
        }
};

class npc_bwl_grethok_controller : public CreatureScript
{
public:
    npc_bwl_grethok_controller() : CreatureScript("npc_bwl_grethok_controller") { }

    struct npc_bwl_grethok_controllerAI : public ScriptedAI
    {
        npc_bwl_grethok_controllerAI(Creature* creature) : ScriptedAI(creature) { }

        void EnterCombat(Unit* who) override
        {
            Talk(SAY_ENTER_COMBAT);

            if (InstanceScript* instance = me->GetInstanceScript()) {
                if (Creature* razor = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_RAZORGORE_THE_UNTAMED)))
                    razor->AI()->AttackStart(who);
            }

            me->InterruptNonMeleeSpells(false);

            me->CallForHelp(15.0f);
            me->SetInCombatWithZone();

            events.Reset();
            events.ScheduleEvent(EVENT_POLYMORPH, 6 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_CHARM, 10 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_SLOW, 1 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_ARCANE_MISSLES, 3 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (!UpdateVictim()) {
                if (InstanceScript* instance = me->GetInstanceScript())
                    if (Creature* razor = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_RAZORGORE_THE_UNTAMED)))
                        me->CastSpell(razor, SPELL_MINDCONTROL_DEBUFF);
                return;
            }

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_POLYMORPH:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                        DoCast(target, SPELL_POLYMORPH);
                    events.ScheduleEvent(EVENT_POLYMORPH, 6 * IN_MILLISECONDS);
                    break;
                case EVENT_CHARM:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                        DoCast(target, SPELL_CHARM);
                    events.ScheduleEvent(EVENT_CHARM, 10 * IN_MILLISECONDS);
                    break;
                case EVENT_SLOW:
                    DoCastVictim(SPELL_SLOW);
                    events.ScheduleEvent(EVENT_SLOW, 10 * IN_MILLISECONDS);
                    break;
                case EVENT_ARCANE_MISSLES:
                    DoCastVictim(SPELL_ARCANE_MISSLES);
                    events.ScheduleEvent(EVENT_ARCANE_MISSLES, 5 * IN_MILLISECONDS);
                    break;
                }
            }
        }

    private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_bwl_grethok_controllerAI(creature);
    }
};

class go_orb_of_domination : public GameObjectScript
{
    public:
        go_orb_of_domination() : GameObjectScript("go_orb_of_domination") { }
    
        bool OnGossipHello(Player* player, GameObject* go) override
        {
            if (player->HasAura(SPELL_MIND_EXHAUST))
                return true;
            
            if (InstanceScript* instance = go->GetInstanceScript())
            {
                if (instance->GetBossState(BOSS_RAZORGORE) == DONE)
                    return true;

                if (Creature* razor = ObjectAccessor::GetCreature(*go, instance->GetGuidData(DATA_RAZORGORE_THE_UNTAMED)))
                    if (!razor->AI()->GetData(0))
                    {
                        player->CastSpell(player, SPELL_MIND_EXHAUST, true);
                        player->CastSpell(razor, SPELL_MINDCONTROL);
                    }
            }
            return true;
        }
};

class spell_egg_event : public SpellScriptLoader
{
    public:
        spell_egg_event() : SpellScriptLoader("spell_egg_event") { }

        class spell_egg_eventSpellScript : public SpellScript
        {
            PrepareSpellScript(spell_egg_eventSpellScript);

            void HandleOnHit()
            {
                if (Creature* creatureCaster = GetCaster()->ToCreature())
                if (auto razorAI = dynamic_cast<boss_razorgore::boss_razorgoreAI*>(creatureCaster->AI())) {
                    razorAI->OnEggDestroyed(GetHitGObj());

                    if (roll_chance_i(35))
                        razorAI->Talk(RAND(SAY_EGGS_BROKEN1, SAY_EGGS_BROKEN2, SAY_EGGS_BROKEN3));
                }
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_egg_eventSpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_egg_eventSpellScript();
        }
};

class spell_razorgore_charm : public SpellScriptLoader
{
public:
    spell_razorgore_charm() : SpellScriptLoader("spell_razorgore_charm") { }

    class spell_razorgore_charm_AuraScripts : public AuraScript
    {
        PrepareAuraScript(spell_razorgore_charm_AuraScripts);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return sSpellMgr->GetSpellInfo(SPELL_MIND_EXHAUST);
        }

        bool Load() override
        {
            caster = GetCaster();
            if (!caster)
                return false;
            return caster->GetTypeId() == TYPEID_PLAYER;
        }

        void HandleOnEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Position pos;
            caster->GetPosition(&pos);
            if (GameObject* go = caster->FindNearestGameObject(GO_ORB_OF_DOMINATION, 15.0f))
                go->GetPosition(&pos);
            if (Creature* trigger = caster->SummonCreature(NPC_ORB_TRIGGER, pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60*IN_MILLISECONDS))
                if (InstanceScript* instance = caster->GetInstanceScript())
                    if (instance->GetBossState(DATA_RAZORGORE_THE_UNTAMED) != DONE)
                        if (Creature* razor = ObjectAccessor::GetCreature(*caster, instance->GetGuidData(DATA_RAZORGORE_THE_UNTAMED)))
                            trigger->CastSpell(razor, SPELL_MINDCONTROL_DEBUFF);
        }

        void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (Creature* trigger = caster->FindNearestCreature(NPC_ORB_TRIGGER, 100.0f))
                trigger->DespawnOrUnsummon();
        }

        void Register() override
        {
            OnEffectApply += AuraEffectApplyFn(spell_razorgore_charm_AuraScripts::HandleOnEffectApply, EFFECT_0, SPELL_AURA_MOD_POSSESS, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(spell_razorgore_charm_AuraScripts::HandleOnEffectRemove, EFFECT_0, SPELL_AURA_MOD_POSSESS, AURA_EFFECT_HANDLE_REAL);
        }

        private:
            Unit* caster = nullptr;
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_razorgore_charm_AuraScripts();
    }
};


void AddSC_boss_razorgore()
{
    new boss_razorgore();
    new npc_bwl_grethok_controller();
    new go_orb_of_domination();
    new spell_egg_event();
    new spell_razorgore_charm();
}
