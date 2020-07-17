/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2010 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

// Known bugs:
//    - They should be floating but they aren't respecting the floor =(
//    - Hardcoded bullets spawner

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Pet.h"
#include "trial_of_the_crusader.h"

enum Yells
{
    SAY_AGGRO               = 0,
    SAY_NIGHT               = 1,
    SAY_LIGHT               = 2,
    EMOTE_VORTEX            = 3,
    EMOTE_TWINK_PACT        = 4,
    SAY_TWINK_PACT          = 5,
    SAY_KILL_PLAYER         = 6,
    SAY_BERSERK             = 7,
    SAY_DEATH               = 8
};

enum Equipment
{
    EQUIP_MAIN_1         = 9423,
    EQUIP_MAIN_2         = 37377
};

enum Summons
{
    NPC_BULLET_CONTROLLER        = 34743,

    NPC_BULLET_DARK              = 34628,
    NPC_BULLET_LIGHT             = 34630,
    NPC_BULLET_STALKER_DARK      = 34704,
    NPC_BULLET_STALKER_LIGHT     = 34720
};

enum Events
{
    EVENT_SUMMON_FEW_BALLS = 1,
    EVENT_SUMMON_MANY_BALLS,

    EVENT_BERSERK,
    EVENT_SPECIAL,
    EVENT_CHECK_HP_AFTER_PACT,
    EVENT_SPIKE,
    EVENT_TOUCH,
    EVENT_REMOVE_TAUNT_IMMUNITY,
    EVENT_REMOVE_ATTACKSPEEDBUFF,

    EVENT_CHECK_BULLETS_IN_RANGE,
};

enum BossSpells
{
    SPELL_LIGHT_TWIN_SPIKE      = 66075,
    SPELL_LIGHT_SURGE           = 65766,
    SPELL_LIGHT_SHIELD          = 65858,
    SPELL_LIGHT_TWIN_PACT       = 65876,
    SPELL_LIGHT_VORTEX          = 66046,
    SPELL_LIGHT_VORTEX_DAMAGE   = 66048,
    SPELL_LIGHT_TOUCH           = 65950,
    SPELL_LIGHT_ESSENCE         = 65686,
    SPELL_EMPOWERED_LIGHT       = 65748,
    SPELL_TWIN_EMPATHY_LIGHT    = 66133,
    SPELL_UNLEASHED_LIGHT       = 65795,
    SPELL_KILL_DARK_VALKYR      = 68400,

    SPELL_DARK_TWIN_SPIKE       = 66069,
    SPELL_DARK_SURGE            = 65768,
    SPELL_DARK_SHIELD           = 65874,
    SPELL_DARK_TWIN_PACT        = 65875,
    SPELL_DARK_VORTEX           = 66058,
    SPELL_DARK_VORTEX_DAMAGE    = 66059,
    SPELL_DARK_TOUCH            = 66001,
    SPELL_DARK_ESSENCE          = 65684,
    SPELL_EMPOWERED_DARK        = 65724,
    SPELL_TWIN_EMPATHY_DARK     = 66132,
    SPELL_UNLEASHED_DARK        = 65808,
    SPELL_KILL_LIGHT_VALKYR     = 68401,

    SPELL_CONTROLLER_PERIODIC   = 66149,
    SPELL_POWER_TWINS           = 65879,
    SPELL_BERSERK               = 64238,
    SPELL_POWERING_UP           = 67590,
    SPELL_SURGE_OF_SPEED        = 65828,

    SPELL_ATTACKSPEED1          = 67318,
    SPELL_ATTACKSPEED2          = 65949,
    SPELL_ATTACKSPEED3          = 67319,
    SPELL_ATTACKSPEED4          = 67320,
};

#define SPELL_DARK_ESSENCE_HELPER RAID_MODE<uint32>(65684, 67176, 67177, 67178)
#define SPELL_LIGHT_ESSENCE_HELPER RAID_MODE<uint32>(65686, 67222, 67223, 67224)

#define SPELL_POWERING_UP_HELPER RAID_MODE<uint32>(67590, 67602, 67603, 67604)

#define SPELL_UNLEASHED_DARK_HELPER RAID_MODE<uint32>(65808, 67172, 67173, 67174)
#define SPELL_UNLEASHED_LIGHT_HELPER RAID_MODE<uint32>(65795, 67238, 67239, 67240)

struct SpecialAbilities
{
    uint32 spellId1;
    uint32 spellId2;
    uint32 emote1;
    uint32 emote2;
    bool isPact;
    uint32 valkyrie;
};

static const SpecialAbilities LightVortex   = { SPELL_LIGHT_VORTEX, 0,                     EMOTE_VORTEX,     0,              false, NPC_LIGHTBANE };
static const SpecialAbilities DarkVortex    = { SPELL_DARK_VORTEX,  0,                     EMOTE_VORTEX,     0,              false, NPC_DARKBANE  };
static const SpecialAbilities LightTwinPact = { SPELL_LIGHT_SHIELD, SPELL_LIGHT_TWIN_PACT, EMOTE_TWINK_PACT, SAY_TWINK_PACT, true,  NPC_LIGHTBANE };
static const SpecialAbilities DarkTwinPact  = { SPELL_DARK_SHIELD,  SPELL_DARK_TWIN_PACT,  EMOTE_TWINK_PACT, SAY_TWINK_PACT, true,  NPC_DARKBANE  };

#define ESSENCE_REMOVE 0
#define ESSENCE_APPLY 1

class OrbsDespawner : public BasicEvent
{
    public:
        explicit OrbsDespawner(Creature* creature) : _creature(creature)
        {
        }

        bool Execute(uint64 /*currTime*/, uint32 /*diff*/)
        {
            _creature->VisitAnyNearbyObject<Creature, Trinity::FunctionAction>(5000.0f, *this);
            return true;
        }

        void operator()(Creature* creature) const
        {
            switch (creature->GetEntry())
            {
                case NPC_BULLET_DARK:
                case NPC_BULLET_LIGHT:
                    creature->DespawnOrUnsummon();
                    return;
                default:
                    return;
            }
        }

    private:
        Creature* _creature;
};

struct boss_twin_baseAI : public BossAI
{
    boss_twin_baseAI(Creature* creature) : BossAI(creature, BOSS_VALKIRIES) { }

    void Reset()
    {
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        me->SetReactState(REACT_PASSIVE);
        me->ModifyAuraState(AuraState, true);
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
        /* Uncomment this once that they are floating above the ground
        me->SetLevitate(true);
        me->SetFlying(true); */
        IsBerserk = false;

        events.ScheduleEvent(EVENT_SPECIAL, 45 * IN_MILLISECONDS);
        ShuffleSpecialAbilities();
        Stage = 0;
        events.ScheduleEvent(EVENT_SPIKE, 17 * IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_TOUCH, 20 * IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_BERSERK, RAID_MODE(9, 6, 9, 6)*MINUTE*IN_MILLISECONDS);

        summons.DespawnAll();
    }

    void JustReachedHome()
    {
        if (instance)
            instance->SetBossState(BOSS_VALKIRIES, FAIL);

        summons.DespawnAll();
        me->DespawnOrUnsummon();
    }

    void MovementInform(uint32 uiType, uint32 uiId)
    {
        if (uiType != POINT_MOTION_TYPE)
            return;

        switch (uiId)
        {
            case 1:
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                me->SetImmuneToPC(false);
                me->SetReactState(REACT_AGGRESSIVE);
                break;
            default:
                break;
        }
    }

    void KilledUnit(Unit* who)
    {
        if (who->GetTypeId() == TYPEID_PLAYER)
            Talk(SAY_KILL_PLAYER);
    }

    void JustSummoned(Creature* summoned)
    {
        summons.Summon(summoned);
    }

    void SummonedCreatureDespawn(Creature* summoned)
    {
        switch (summoned->GetEntry())
        {
            case NPC_LIGHT_ESSENCE:
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LIGHT_ESSENCE_HELPER);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_POWERING_UP_HELPER);
                break;
            case NPC_DARK_ESSENCE:
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_DARK_ESSENCE_HELPER);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_POWERING_UP_HELPER);
                break;
            case NPC_BULLET_CONTROLLER:
                me->m_Events.AddEvent(new OrbsDespawner(me), me->m_Events.CalculateTime(100));
                break;
            default:
                break;
        }
        summons.Despawn(summoned);
    }

    void JustDied(Unit* /*killer*/)
    {
        Talk(SAY_DEATH);
        if (instance)
        {
            if (Creature* pSister = GetSister())
            {
                if (!pSister->IsAlive())
                {
                    me->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                    pSister->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                    BossAI::JustDied(nullptr);
                }
                else
                {
                    me->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                    instance->SetBossState(BOSS_VALKIRIES, SPECIAL);
                }
            }
        }
        DoCast(InstaKillSpellId);
        summons.DespawnAll();
    }

    // Called when sister pointer needed
    Creature* GetSister()
    {
        return ObjectAccessor::GetCreature((*me), ObjectGuid(instance->GetGuidData(SisterNpcId)));
    }

    void EnterCombat(Unit* /*who*/)
    {
        me->SetInCombatWithZone();
        me->setActive(true);
        if (instance)
        {
            if (Creature* pSister = GetSister())
            {
                me->AddAura(MyEmphatySpellId, pSister);
                pSister->SetInCombatWithZone();
            }
            instance->SetBossState(BOSS_VALKIRIES, IN_PROGRESS);
        }

        Talk(SAY_AGGRO);
        DoCastSelf(SurgeSpellId);
    }

    void EnableDualWield(bool mode = true)
    {
        SetEquipmentSlots(false, Weapon, mode ? Weapon : int32(EQUIP_UNEQUIP), EQUIP_UNEQUIP);
        me->SetCanDualWield(mode);
        me->UpdateDamagePhysical(mode ? OFF_ATTACK : BASE_ATTACK);
    }

    void ShuffleSpecialAbilities()
    {
        if (me->GetEntry() == NPC_LIGHTBANE)
            return;

        SpecialAbility[0] = RAND(LightVortex, DarkVortex, LightTwinPact, DarkTwinPact);

        do
        {
            SpecialAbility[1] = RAND(LightVortex, DarkVortex, LightTwinPact, DarkTwinPact);
        } while (SpecialAbility[1].spellId1 == SpecialAbility[0].spellId1);

        do
        {
            SpecialAbility[2] = RAND(LightVortex, DarkVortex, LightTwinPact, DarkTwinPact);
        } while (SpecialAbility[2].spellId1 == SpecialAbility[0].spellId1 || SpecialAbility[2].spellId1 == SpecialAbility[1].spellId1);

        do
        {
            SpecialAbility[3] = RAND(LightVortex, DarkVortex, LightTwinPact, DarkTwinPact);
        } while (SpecialAbility[3].spellId1 == SpecialAbility[0].spellId1 || SpecialAbility[3].spellId1 == SpecialAbility[1].spellId1 || SpecialAbility[3].spellId1 == SpecialAbility[2].spellId1);

        if (Creature* sister = GetSister())
            if (boss_twin_baseAI* sisterAI = dynamic_cast<boss_twin_baseAI*>(sister->AI()))
                for (uint8 i=0; i<4; ++i)
                    sisterAI->SpecialAbility[i] = SpecialAbility[i];
    }

    void UpdateAI(uint32 diff)
    {
        if (!instance || !UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_BERSERK:
                    if (!IsBerserk)
                    {
                        DoCastSelf(SPELL_BERSERK, true);
                        Talk(SAY_BERSERK);
                        IsBerserk = true;
                    }
                    break;
                case EVENT_SPECIAL:
                    events.RescheduleEvent(EVENT_SPECIAL, 45 * IN_MILLISECONDS);
                    if (SpecialAbility[Stage].valkyrie == me->GetEntry())
                    {
                        // Twin Pact
                        if (SpecialAbility[Stage].isPact)
                        {
                            DoCastSelf(SpecialAbility[Stage].spellId1);
                            DoCastSelf(SpecialAbility[Stage].spellId2);
                            if (Creature* sister = GetSister())
                                sister->CastSpell(sister, SPELL_POWER_TWINS);
                            Talk(SpecialAbility[Stage].emote2);

                            events.ScheduleEvent(EVENT_CHECK_HP_AFTER_PACT, 16050);
                            events.ScheduleEvent(EVENT_REMOVE_TAUNT_IMMUNITY, 16050);
                            events.ScheduleEvent(EVENT_REMOVE_ATTACKSPEEDBUFF, 15 * SEC);
                        }
                        // Vortex
                        else
                        {
                            DoCastAOE(SpecialAbility[Stage].spellId1);
                            events.ScheduleEvent(EVENT_REMOVE_TAUNT_IMMUNITY, 10 * IN_MILLISECONDS);
                        }

                        Talk(SpecialAbility[Stage].emote1);

                        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                    }
                    if (++Stage == 4)
                    {
                        ShuffleSpecialAbilities();
                        Stage = 0;
                    }
                    break;
                case EVENT_CHECK_HP_AFTER_PACT:
                    tmpHP = 0;
                    if (Creature* sister = GetSister())
                    {
                        sister->GetHealth() > me->GetHealth() ? tmpHP = sister->GetHealth() : tmpHP = me->GetHealth();

                        // count damage we've done on the other sister to the damage requirement
                        // SetHealth does not account for that
                        sister->LowerPlayerDamageReq(sister->GetHealth() - tmpHP);
                        me->LowerPlayerDamageReq(me->GetHealth() - tmpHP);

                        sister->SetHealth(tmpHP);
                        me->SetHealth(tmpHP);
                    }
                    break;
                case EVENT_REMOVE_TAUNT_IMMUNITY:
                    me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
                    break;
                case EVENT_REMOVE_ATTACKSPEEDBUFF:
                    // All Difficulties of The Attackspeed Buff
                    me->RemoveAurasDueToSpell(SPELL_ATTACKSPEED1);
                    me->RemoveAurasDueToSpell(SPELL_ATTACKSPEED2);
                    me->RemoveAurasDueToSpell(SPELL_ATTACKSPEED3);
                    me->RemoveAurasDueToSpell(SPELL_ATTACKSPEED4);
                    if (Creature* sister = GetSister())
                    {
                        sister->RemoveAurasDueToSpell(SPELL_ATTACKSPEED1);
                        sister->RemoveAurasDueToSpell(SPELL_ATTACKSPEED2);
                        sister->RemoveAurasDueToSpell(SPELL_ATTACKSPEED3);
                        sister->RemoveAurasDueToSpell(SPELL_ATTACKSPEED4);
                    }
                    break;
                case EVENT_SPIKE:
                    DoCastVictim(SpikeSpellId);
                    events.RescheduleEvent(EVENT_SPIKE, 20 * IN_MILLISECONDS);
                    break;
                case EVENT_TOUCH:
                    if (IsHeroic())
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true, OtherEssenceSpellId))
                            DoCast(target, TouchSpellId);
                    events.RescheduleEvent(EVENT_TOUCH, urand(40 * IN_MILLISECONDS, 50 * IN_MILLISECONDS));
                    break;
                default:
                    return;
            }
        }
        DoMeleeAttackIfReady(true);
    }

    protected:
        AuraStateType AuraState;

        uint8  Stage;
        SpecialAbilities SpecialAbility[4];

        bool   IsBerserk;

        uint32 tmpHP;
        uint32 Weapon;

        uint32 SisterNpcId;
        uint32 MyEmphatySpellId;
        uint32 OtherEssenceSpellId;
        uint32 SurgeSpellId;
        uint32 SpikeSpellId;
        uint32 TouchSpellId;
        uint32 InstaKillSpellId;
};

class boss_fjola : public CreatureScript
{
    public:
        boss_fjola() : CreatureScript("boss_fjola") { }

        struct boss_fjolaAI : public boss_twin_baseAI
        {
            boss_fjolaAI(Creature* creature) : boss_twin_baseAI(creature) { }

            void Reset()
            {
                SetEquipmentSlots(false, EQUIP_MAIN_1, EQUIP_UNEQUIP, EQUIP_NO_CHANGE);
                Weapon = EQUIP_MAIN_1;
                AuraState = AURA_STATE_UNKNOWN22;
                SisterNpcId = NPC_DARKBANE;
                MyEmphatySpellId = SPELL_TWIN_EMPATHY_DARK;
                OtherEssenceSpellId = SPELL_DARK_ESSENCE_HELPER;
                SurgeSpellId = SPELL_LIGHT_SURGE;
                TouchSpellId = SPELL_LIGHT_TOUCH;
                SpikeSpellId = SPELL_LIGHT_TWIN_SPIKE;
                InstaKillSpellId = SPELL_KILL_DARK_VALKYR;

                if (instance)
                    instance->DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT,  EVENT_START_TWINS_FIGHT);
                boss_twin_baseAI::Reset();
            }

            void EnterCombat(Unit* who)
            {
                if (instance)
                    instance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT,  EVENT_START_TWINS_FIGHT);

                me->SummonCreature(NPC_BULLET_CONTROLLER, ToCCommonLoc[1].GetPositionX(), ToCCommonLoc[1].GetPositionY(), ToCCommonLoc[1].GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN);
                boss_twin_baseAI::EnterCombat(who);
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                instance->DoUseDoorOrButton(ObjectGuid(instance->GetGuidData(GO_MAIN_GATE_DOOR)));
                boss_twin_baseAI::EnterEvadeMode();
            }

            void JustReachedHome()
            {
                if (instance)
                    instance->DoUseDoorOrButton(ObjectGuid(instance->GetGuidData(GO_MAIN_GATE_DOOR)));

                boss_twin_baseAI::JustReachedHome();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_fjolaAI(creature);
        }
};

class boss_eydis : public CreatureScript
{
    public:
        boss_eydis() : CreatureScript("boss_eydis") { }

        struct boss_eydisAI : public boss_twin_baseAI
        {
            boss_eydisAI(Creature* creature) : boss_twin_baseAI(creature) { }

            void Reset()
            {
                SetEquipmentSlots(false, EQUIP_MAIN_2, EQUIP_UNEQUIP, EQUIP_NO_CHANGE);
                Weapon = EQUIP_MAIN_2;
                AuraState = AURA_STATE_UNKNOWN19;
                SisterNpcId = NPC_LIGHTBANE;
                MyEmphatySpellId = SPELL_TWIN_EMPATHY_LIGHT;
                OtherEssenceSpellId = SPELL_LIGHT_ESSENCE_HELPER;
                SurgeSpellId = SPELL_DARK_SURGE;
                TouchSpellId = SPELL_DARK_TOUCH;
                SpikeSpellId = SPELL_DARK_TWIN_SPIKE;
                InstaKillSpellId = SPELL_KILL_LIGHT_VALKYR;
                boss_twin_baseAI::Reset();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_eydisAI(creature);
        }
};

class npc_essence_of_twin : public CreatureScript
{
    public:
        npc_essence_of_twin() : CreatureScript("npc_essence_of_twin") { }

        struct npc_essence_of_twinAI : public ScriptedAI
        {
            npc_essence_of_twinAI(Creature* creature) : ScriptedAI(creature) { }

            uint32 GetData(uint32 data) const
            {
                uint32 spellReturned = 0;
                switch (me->GetEntry())
                {
                    case NPC_LIGHT_ESSENCE:
                        spellReturned = (data == ESSENCE_REMOVE) ? SPELL_DARK_ESSENCE_HELPER : SPELL_LIGHT_ESSENCE_HELPER;
                        break;
                    case NPC_DARK_ESSENCE:
                        spellReturned = (data == ESSENCE_REMOVE) ? SPELL_LIGHT_ESSENCE_HELPER : SPELL_DARK_ESSENCE_HELPER;
                        break;
                    default:
                        break;
                }

                return spellReturned;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_essence_of_twinAI(creature);
        };

        bool OnGossipHello(Player* player, Creature* creature)
        {
            player->RemoveAurasDueToSpell(creature->GetAI()->GetData(ESSENCE_REMOVE));
            player->CastSpell(player, creature->GetAI()->GetData(ESSENCE_APPLY), true);
            player->CLOSE_GOSSIP_MENU();
            return true;
        }
};

struct npc_unleashed_ballAI : public ScriptedAI
{
    npc_unleashed_ballAI(Creature* creature) : ScriptedAI(creature)
    {
    }

    void MoveToNextPoint()
    {
        float x0 = ToCCommonLoc[1].GetPositionX(), y0 = ToCCommonLoc[1].GetPositionY(), r = 47.0f;
        float y = y0;
        float x = frand(x0 - r, x0 + r);
        float sq = pow(r, 2) - pow(x - x0, 2);
        float rt = sqrtf(fabs(sq));
        if (urand(0, 1))
            y = y0 + rt;
        else
            y = y0 - rt;
        me->GetMotionMaster()->MovePoint(0, x, y, PLATFORM_GROUND_Z);
    }

    void Reset()
    {
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        me->SetReactState(REACT_PASSIVE);
        me->SetDisableGravity(true);
        me->SetCanFly(true);
        SetCombatMovement(false);
        MoveToNextPoint();
        WPReached = false;
    }

    void MovementInform(uint32 uiType, uint32 uiId)
    {
        if (uiType != POINT_MOTION_TYPE)
            return;

        if (uiId == 0)
            WPReached = true;
    }

    protected:
        bool WPReached;
};

class npc_unleashed_dark : public CreatureScript
{
    public:
        npc_unleashed_dark() : CreatureScript("npc_unleashed_dark") { }

        struct npc_unleashed_darkAI : public npc_unleashed_ballAI
        {
            npc_unleashed_darkAI(Creature* creature) : npc_unleashed_ballAI(creature) {}

            void UpdateAI(uint32 diff)
            {
                if (WPReached)
                {
                    MoveToNextPoint();
                    WPReached = false;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_unleashed_darkAI(creature);
        }
};

class npc_unleashed_light : public CreatureScript
{
    public:
        npc_unleashed_light() : CreatureScript("npc_unleashed_light") { }

        struct npc_unleashed_lightAI : public npc_unleashed_ballAI
        {
            npc_unleashed_lightAI(Creature* creature) : npc_unleashed_ballAI(creature) {}

            void UpdateAI(uint32 diff)
            {
                if (WPReached)
                {
                    MoveToNextPoint();
                    WPReached = false;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_unleashed_lightAI(creature);
        }
};

class npc_bullet_controller : public CreatureScript
{
    public:
        npc_bullet_controller() : CreatureScript("npc_bullet_controller") { }

        struct npc_bullet_controllerAI : public ScriptedAI
        {
            npc_bullet_controllerAI(Creature* creature) : ScriptedAI(creature)
            {
                SetCombatMovement(false);
                Reset();
            }

            void Reset()
            {
                if (IsHeroic())
                    events.ScheduleEvent(EVENT_SUMMON_FEW_BALLS, 1);
                
                events.ScheduleEvent(EVENT_SUMMON_MANY_BALLS, RAID_MODE(20000, 25000, 20000, 25000));

                events.ScheduleEvent(EVENT_CHECK_BULLETS_IN_RANGE, 0.5*IN_MILLISECONDS); // Event to Check Bullets in Range of Players - repeated every 250ms
            }

            void SpawnBalls(uint8 white, uint8 black)
            {
                std::list<Creature*> darkStalkerList;
                std::list<Creature*> lightStalkerList;
                me->GetCreatureListWithEntryInGrid(darkStalkerList, NPC_BULLET_STALKER_DARK, 1000.0f);
                me->GetCreatureListWithEntryInGrid(lightStalkerList, NPC_BULLET_STALKER_LIGHT, 1000.0f);

                /* create a list with all possible spawn positions which are represented by stalker npcs 
                pick one position at random, spawn a ball at this position and delete this stalker
                from the list to prevent multiple spawns of the same color at the same position */
                for (uint8 i = 0; i < white; i++)
                {
                    if (!lightStalkerList.empty())
                    {
                        std::list<Creature*>::const_iterator itr = lightStalkerList.begin();
                        advance(itr, urand(0, lightStalkerList.size() - 1));
                        Creature* lightstalker = *itr;
                        lightstalker->SummonCreature(NPC_BULLET_LIGHT, lightstalker->GetPositionX(), lightstalker->GetPositionY(), lightstalker->GetPositionZ());
                        lightStalkerList.remove(*itr);
                    }
                }
                for (uint8 i = 0; i < black; i++)
                {
                    if (!darkStalkerList.empty())
                    {
                        std::list<Creature*>::const_iterator itr = darkStalkerList.begin();
                        advance(itr, urand(0, darkStalkerList.size() - 1));
                        Creature* darkstalker = *itr;
                        darkstalker->SummonCreature(NPC_BULLET_DARK, darkstalker->GetPositionX(), darkstalker->GetPositionY(), darkstalker->GetPositionZ());
                        darkStalkerList.remove(*itr);
                    }
                }
            }

            void UpdateAI(uint32 diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SUMMON_FEW_BALLS: // every few seconds two balls are spawned in heroic mode
                            SpawnBalls(1, 1);
                            events.RescheduleEvent(EVENT_SUMMON_FEW_BALLS, RAID_MODE(0, 0, 5000, 2500));
                            break;
                        case EVENT_SUMMON_MANY_BALLS: // in all difficulties every 30 seconds a bunch of balls is spawned
                            if (Is25ManRaid())
                                SpawnBalls(urand(14, 18), urand(14, 18));
                            else
                                SpawnBalls(urand(5, 7), urand(5, 7));

                            events.RescheduleEvent(EVENT_SUMMON_MANY_BALLS, 30000);
                            break;
                        case EVENT_CHECK_BULLETS_IN_RANGE: // Check every 0,25 sec for bullet in range of player
                            for (Player& player : me->GetMap()->GetPlayers())
                            {
                                if (player.IsAlive())
                                {
                                    if (Creature* BulletDark = player.FindNearestCreature(NPC_BULLET_DARK, 1.5f, true))
                                    {
                                        BulletDark->CastSpell((Unit*)NULL, SPELL_UNLEASHED_DARK_HELPER);
                                        BulletDark->GetMotionMaster()->MoveIdle();
                                        BulletDark->DespawnOrUnsummon(1 * IN_MILLISECONDS);

                                    }
                                    if (Creature* BulletLight = player.FindNearestCreature(NPC_BULLET_LIGHT, 1.5f, true))
                                    {
                                        BulletLight->CastSpell((Unit*)NULL, SPELL_UNLEASHED_LIGHT_HELPER);
                                        BulletLight->GetMotionMaster()->MoveIdle();
                                        BulletLight->DespawnOrUnsummon(1 * IN_MILLISECONDS);
                                    }
                                }
                            }
                            events.RescheduleEvent(EVENT_CHECK_BULLETS_IN_RANGE, 0.25*IN_MILLISECONDS);
                            break;
                    }
                }
            }

            private:
                EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_bullet_controllerAI(creature);
        }
};

class spell_powering_up : public SpellScriptLoader
{
    public:
        spell_powering_up() : SpellScriptLoader("spell_powering_up") { }

        class spell_powering_up_SpellScript : public SpellScript
        {
            public:
                PrepareSpellScript(spell_powering_up_SpellScript)

            uint32 spellId;
            uint32 poweringUp;

            bool Load()
            {
                spellId = sSpellMgr->GetSpellIdForDifficulty(SPELL_SURGE_OF_SPEED, GetCaster());
                if (!sSpellMgr->GetSpellInfo(spellId))
                    return false;

                poweringUp = sSpellMgr->GetSpellIdForDifficulty(SPELL_POWERING_UP, GetCaster());
                if (!sSpellMgr->GetSpellInfo(poweringUp))
                    return false;

                return true;
            }

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                if (Unit* target = GetHitUnit())
                {
                    if (Aura* pAura = target->GetAura(poweringUp))
                    {
                        if (pAura->GetStackAmount() >= 100)
                        {
                            if (target->GetDummyAuraEffect(SPELLFAMILY_GENERIC, 2206, EFFECT_1))
                                target->CastSpell(target, SPELL_EMPOWERED_DARK, true);

                            if (target->GetDummyAuraEffect(SPELLFAMILY_GENERIC, 2845, EFFECT_1))
                                target->CastSpell(target, SPELL_EMPOWERED_LIGHT, true);

                            target->RemoveAurasDueToSpell(poweringUp);
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_powering_up_SpellScript::HandleScriptEffect, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_powering_up_SpellScript();
        }
};

class spell_valkyr_essences : public SpellScriptLoader
{
    public:
        spell_valkyr_essences() : SpellScriptLoader("spell_valkyr_essences") { }

        class spell_valkyr_essences_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_valkyr_essences_AuraScript);

            uint32 spellId;

            bool Load()
            {
                spellId = sSpellMgr->GetSpellIdForDifficulty(SPELL_SURGE_OF_SPEED, GetCaster());
                if (!sSpellMgr->GetSpellInfo(spellId))
                    return false;
                return true;
            }

            void Absorb(AuraEffect* /*aurEff*/, DamageInfo & dmgInfo, uint32 & /*absorbAmount*/)
            {
                if (Unit* owner = GetUnitOwner())
                {
                    if (dmgInfo.GetSpellInfo())
                    {
                        if (uint32 poweringUp = sSpellMgr->GetSpellIdForDifficulty(SPELL_POWERING_UP, owner))
                        {
                            if (urand(0, 99) < 5)
                                GetTarget()->CastSpell(GetTarget(), spellId, true);

                            // Twin Vortex part
                            uint32 lightVortex = sSpellMgr->GetSpellIdForDifficulty(SPELL_LIGHT_VORTEX_DAMAGE, owner);
                            uint32 darkVortex = sSpellMgr->GetSpellIdForDifficulty(SPELL_DARK_VORTEX_DAMAGE, owner);
                            static int32 stacksCount = 6 - 1; // needs to be recasted on every change, this already applies 1 stack

                            if (lightVortex && darkVortex && stacksCount)
                            {
                                if (dmgInfo.GetSpellInfo()->Id == darkVortex || dmgInfo.GetSpellInfo()->Id == lightVortex)
                                {
                                    Aura* pAura = owner->GetAura(poweringUp);
                                    if (pAura)
                                    {
                                        pAura->ModStackAmount(stacksCount);
                                        owner->CastSpell(owner, poweringUp, true);
                                    }
                                    else
                                    {
                                        owner->CastSpell(owner, poweringUp, true);
                                        if (Aura* pTemp = owner->GetAura(poweringUp))
                                            pTemp->ModStackAmount(stacksCount);
                                    }
                                }
                            }

                            // Picking floating balls
                            uint32 unleashedDark = sSpellMgr->GetSpellIdForDifficulty(SPELL_UNLEASHED_DARK, owner);
                            uint32 unleashedLight = sSpellMgr->GetSpellIdForDifficulty(SPELL_UNLEASHED_LIGHT, owner);

                            if (unleashedDark && unleashedLight)
                            {
                                if (dmgInfo.GetSpellInfo()->Id == unleashedDark || dmgInfo.GetSpellInfo()->Id == unleashedLight)
                                {
                                    // need to do the things in this order, else players might have 100 charges of Powering Up without anything happening
                                    Aura* pAura = owner->GetAura(poweringUp);
                                    if (pAura)
                                    {
                                        // 2 lines together add the correct amount of buff stacks
                                        pAura->ModStackAmount(stacksCount);
                                        owner->CastSpell(owner, poweringUp, true);
                                    }
                                    else
                                    {
                                        owner->CastSpell(owner, poweringUp, true);
                                        if (Aura* pTemp = owner->GetAura(poweringUp))
                                            pTemp->ModStackAmount(stacksCount);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_valkyr_essences_AuraScript::Absorb, EFFECT_0);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_valkyr_essences_AuraScript();
        }
};

class spell_power_of_the_twins : public SpellScriptLoader
{
    public:
        spell_power_of_the_twins() : SpellScriptLoader("spell_power_of_the_twins") { }

        class spell_power_of_the_twins_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_power_of_the_twins_AuraScript);

            bool Load()
            {
                return GetCaster()->GetTypeId() == TYPEID_UNIT;
            }

            void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                {
                    if (Creature* Valk = ObjectAccessor::GetCreature(*GetCaster(), ObjectGuid(instance->GetGuidData(GetCaster()->GetEntry()))))
                        CAST_AI(boss_twin_baseAI, Valk->AI())->EnableDualWield(true);
                }
            }

            void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                {
                    if (Creature* Valk = ObjectAccessor::GetCreature(*GetCaster(), ObjectGuid(instance->GetGuidData(GetCaster()->GetEntry()))))
                        CAST_AI(boss_twin_baseAI, Valk->AI())->EnableDualWield(false);
                }
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_power_of_the_twins_AuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_power_of_the_twins_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);

            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_power_of_the_twins_AuraScript();
        }
};

void AddSC_boss_twin_valkyr()
{
    new boss_fjola();
    new boss_eydis();
    new npc_unleashed_light();
    new npc_unleashed_dark();
    new npc_essence_of_twin();
    new npc_bullet_controller();

    new spell_powering_up();
    new spell_valkyr_essences();
    new spell_power_of_the_twins();
}
