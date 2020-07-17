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

/* Script Data Start
SDName: Boss malygos
Script Data End */

/* ToDo:
 - Find sniffed spawn position for chest ?
 - Implement a better way to disappear the gameobjects (phase ?)
 - Implement scaling for player's drakes (should be done with aura 66668, it is casted - but it is not working as it should)
 - Remove hack that re-adds targets to the aggro list after they enter to a vehicle when it works as expected
 - Remove DB-spawned Vortexes, TempSummon them when needed
 - Nexus Lords should circle down before attacking
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "PassiveAI.h"
#include "eye_of_eternity.h"
#include "ScriptedEscortAI.h"
#include "Vehicle.h"
#include "Vehicle.h"
#include "CombatAI.h"
#include "CreatureTextMgr.h"
#include "DBCStores.h"


enum MalygosSay
{
    // INTRO
    SAY_INTRO1                    =    21,
    SAY_INTRO2                    =    22,
    SAY_INTRO3                    =    23,
    //SAY_INTRO4                  =    -1999697,
    //SAY_INTRO5                  =    -1999696,

    // PHASE 1
    // SAY_AGGRO_P_ONE            =    -1999695,
    SAY_P2_SURGE_OF_POWER         =    24,
    //SAY_KILLED_PLAYER_P_ONE1    =    -1999693,
    //SAY_KILLED_PLAYER_P_ONE2    =    -1999692,
    //SAY_KILLED_PLAYER_P_ONE3    =    -1999691,
    //SAY_END_P_ONE               =    -1999690,

    //PHASE 2
    //SAY_AGGRO_P_TWO             =    -1999689,
    //SAY_ANTI_MAGIC_SHELL        =    -1999688,
    SAY_VORTEX                    =    25,
    //SAY_KILLED_PLAYER_P_TWO1    =    -1999686,
    //SAY_KILLED_PLAYER_P_TWO2    =    -1999685,
    //SAY_KILLED_PLAYER_P_TWO3    =    -1999684,
    //SAY_BUFF_SPARK              =    -1999679,
    //SAY_END_P_TWO               =    -1999683,

    // PHASE 3
    //SAY_INTRO_P_THREE           =    -1999682,
    //SAY_AGGRO_P_THREE           =    -1999681,
    //SAY_SURGE_POWER             =    -1999680,    // P3 Sourge of Power
    //SAY_KILLED_PLAYER_P_THREE1  =    -1999678,
    //SAY_KILLED_PLAYER_P_THREE2  =    -1999677,
    //SAY_KILLED_PLAYER_P_THREE3  =    -1999676,
    //SAY_SPELL_CASTING_P_THREE1  =    -1999675, // used for static field / if someone comes too near
    //SAY_SPELL_CASTING_P_THREE2  =    -1999674,
    //SAY_SPELL_CASTING_P_THREE3  =    -1999673,
    //SAY_DEATH                   =    -1999672,

    // EMOTES & WARNINGS
    SAY_SURGE_WARNING_P_THREE     =    26,
    SAY_EMOTE_BREATH_P2           =    27,
    //SAY_EMOTE_POWERSPARK        =    -1999669,

    // ALEXSTRASZA PROXY
    SAY_DRAGONS_SPAWN             =    16,

    // OUTRO
    //SAY_ALEXSTRASZA_OUTRO1      =    -1999667,
    //SAY_ALEXSTRASZA_OUTRO2      =    -1999666,
    //SAY_ALEXSTRASZA_OUTRO3      =    -1999665,
    //SAY_ALEXSTRASZA_OUTRO4      =    -1999664
};


enum Texts
{
    // Malygos
    SAY_AGGRO_P_ONE                     = 0,
    SAY_KILLED_PLAYER_P_ONE             = 1,
    SAY_END_P_ONE                       = 2,
    SAY_AGGRO_P_TWO                     = 3,
    SAY_ANTI_MAGIC_SHELL                = 4, // not sure when execute it
    SAY_MAGIC_BLAST                     = 5,  // not sure when execute it
    SAY_KILLED_PLAYER_P_TWO             = 6,
    SAY_END_P_TWO                       = 7,
    SAY_INTRO_P_THREE                   = 8,
    SAY_AGGRO_P_THREE                   = 9,
    SAY_SURGE_POWER                     = 10,  // not sure when execute it
    SAY_BUFF_SPARK                      = 11,
    SAY_KILLED_PLAYER_P_THREE           = 12,
    SAY_SPELL_CASTING_P_THREE           = 13,
    SAY_DEATH,

    // Alexstrasza
    SAY_ONE                             = 0,
    SAY_TWO                             = 1,
    SAY_THREE                           = 2,
    SAY_FOUR                            = 3,

    // Power Sparks
    EMOTE_POWER_SPARK_SUMMONED            = 0
};

enum Phases
{
    PHASE_INTRO = 1,
    PHASE_ONE,
    PHASE_TWO,
    PHASE_PRE_THREE,
    PHASE_THREE,
};

enum Events
{
    // Phase 1
    EVENT_ARCANE_BREATH = 1,
    EVENT_ARCANE_STORM,
    EVENT_VORTEX,
    EVENT_POWER_SPARKS,

    // Phase 2
    EVENT_SURGE_OF_POWER,
    EVENT_SUMMON_ARCANE,
    EVENT_PHASE_2_ADDS,
    EVENT_PHASE_2_ROTATION,

    // Phase Pre-3
    EVENT_PLATTFORM_CHANNEL,
    EVENT_PLATTFORM_BOOM,
    EVENT_DESTROY_PLATTFORM,
    EVENT_SUMMON_RED_DRAKES,
    // Phase 3
    EVENT_SURGE_OF_POWER_PHASE_3,
    EVENT_STATIC_FIELD,
    EVENT_ARCANE_PULSE,

    // Yells
    EVENT_YELL_0,
    EVENT_YELL_1,
    EVENT_YELL_2,
    EVENT_YELL_3,
    EVENT_YELL_4,
};

enum Spells
{
    // ========= MALYGOS =========
    SPELL_BERSERK               = 47008,

    //## Intro ##
    SPELL_PORTAL_BEAM           = 56046,
    //## Phase 1 ##
    SPELL_ARCANE_BREATH         = 56272,
    SPELL_ARCANE_STORM          = 61693,
    //## Phase 2 ##
    SPELL_SURGE_OF_POWER        = 56505, //casted on the platform
    //## Phase 3 ##
    SPELL_STATIC_FIELD          = 57430,
    SPELL_ARCANE_PULSE          = 57432, //damage for near player
    SPELL_SURGE_OF_POWER_P3     = 57407,

    // ========= VORTEX HANDLING =========
    SPELL_VORTEX_1              = 56237, // Vortex extra effect 1
    SPELL_VORTEX_2              = 55873, // Vortex visual, casted by trigger
    SPELL_VORTEX_3              = 56105, // this spell must handle all the script - casted by the boss and to himself
    //SPELL_VORTEX_4            = 55853, // damage | used to enter to the vehicle - defined in eye_of_eternity.h
    //SPELL_VORTEX_5            = 56263, // damage | used to enter to the vehicle - defined in eye_of_eternity.h
    SPELL_VORTEX_6              = 73040, // teleport - (casted to all raid) | caster 30090 | target player

    // ========= POWER SPARK HANDLING =========
    //## Power Spark
    SPELL_POWER_SPARK_DEATH     = 55852, //buff for player
    SPELL_POWER_SPARK_MALYGOS   = 56152, //buff for malygos
    //## Power Spark Portal
    SPELL_SUMMON_POWER_SPARK    = 56142,
    //SPELL_PORTAL_OPENED       = 61236, //defined in eye_of_eternity.h
    SPELL_PORTAL_VISUAL_CLOSED  = 55949,

    // ========= ARCANE BOMB HANDLING =========
    SPELL_SUMMON_ARCANE_BOMB    = 56429,
    SPELL_ARCANE_BOMB_VISUAL    = 56430,
    SPELL_ARCANE_BOMB           = 56431,
    SPELL_ARCANE_OVERLOAD       = 56432,

    // ========= TRASH =========
    //## Scion of Eternity ##
    SPELL_ARCANE_BARRAGE        = 56397,
    SPELL_ARCANE_SHOCK          = 57058,
    //## Nexus Lord ##
    SPELL_HASTE                 = 57060,

    // ========= ALEXSTRASZA OUTRO =========
    SPELL_GIFT_CHANNEL          = 61028,
    SPELL_GIFT_VISUAL           = 61023,

    SPELL_SUMMON_POWER_SPARK_OPEN_VISUAL = 56140, // not used

  //SPELL_SUMMON_RED_DRAGON     = 56070, //summons red dragon

    SPELL_PLATTFORM_CHANNEL     = 58842, //unknown when used
    SPELL_PLATTFORM_BOOM        = 59084,
};

enum Movements
{
    MOVE_PHASE_0_PORTALS = 1,
    MOVE_PHASE_1_INIT,
    MOVE_PHASE_1_VORTEX_PREP,
    MOVE_PHASE_1_VORTEX,
    MOVE_PHASE_1_VORTEX_END,
    MOVE_PHASE_2_ROTATION,
    MOVE_PHASE_2_DEEP_BREATH,
    MOVE_PHASE_3_CENTER,
};

enum Achievements
{
    ACHIEV_TIMED_START_EVENT = 20387,
};

enum Actions
{
    ACTION_INIT_PHASE_ONE,
};

enum MalygosData
{
    DATA_SUMMON_DEATHS, // phase 2
    DATA_PHASE,
};

#define GROUND_Z 266.170288f

#define MAX_HOVER_DISK_WAYPOINTS 18
#define MALYGOS_MAX_WAYPOINTS 16
#define MAX_SUMMONS_PHASE_TWO RAID_MODE(6,12)
#define MAX_MALYGOS_POS 2
#define HOVERDISC_SEAT 0

const Position HoverDiskWaypoints[MAX_HOVER_DISK_WAYPOINTS] =
{
    {782.9821f, 1296.652f, 282.1114f, 0.0f},
    {779.5459f, 1287.228f, 282.1393f, 0.0f},
    {773.0028f, 1279.52f,  282.4164f, 0.0f},
    {764.3626f, 1274.476f, 282.4731f, 0.0f},
    {754.3961f, 1272.639f, 282.4171f, 0.0f},
    {744.4422f, 1274.412f, 282.222f,  0.0f},
    {735.575f,  1279.742f, 281.9674f, 0.0f},
    {729.2788f, 1287.187f, 281.9943f, 0.0f},
    {726.1191f, 1296.688f, 282.2997f, 0.0f},
    {725.9396f, 1306.531f, 282.2448f, 0.0f},
    {729.3045f, 1316.122f, 281.9108f, 0.0f},
    {735.8322f, 1323.633f, 282.1887f, 0.0f},
    {744.4616f, 1328.999f, 281.9948f, 0.0f},
    {754.4739f, 1330.666f, 282.049f,  0.0f},
    {764.074f,  1329.053f, 281.9949f, 0.0f},
    {772.8409f, 1323.951f, 282.077f,  0.0f},
    {779.5085f, 1316.412f, 281.9145f, 0.0f},
    {782.8365f, 1306.778f, 282.3035f, 0.0f},
};

const Position MalygosCirclingWPs[MALYGOS_MAX_WAYPOINTS] =
{
    {812.7299f, 1391.672f, 283.2763f, 0.0f},
    {848.2912f, 1358.61f, 283.2763f, 0.0f}, // portal waypoint
    {853.9227f, 1307.911f, 283.2763f, 0.0f},
    {847.1437f, 1265.538f, 283.2763f, 0.0f},
    {839.9229f, 1245.245f, 283.2763f, 0.0f},
    {827.3463f, 1221.818f, 283.2763f, 0.0f}, // portal waypoint (start)
    {803.2727f, 1203.851f, 283.2763f, 0.0f},
    {772.9372f, 1197.981f, 283.2763f, 0.0f},
    {732.1138f, 1200.647f, 283.2763f, 0.0f},
    {693.8761f, 1217.995f, 283.2763f, 0.0f}, // portal waypoint
    {664.5038f, 1256.539f, 283.2763f, 0.0f},
    {650.1497f, 1303.485f, 283.2763f, 0.0f},
    {662.9109f, 1350.291f, 283.2763f, 0.0f},
    {677.6391f, 1377.607f, 283.2763f, 0.0f}, // portal waypoint
    {704.8198f, 1401.162f, 283.2763f, 0.0f},
    {755.2642f, 1417.1f, 283.2763f, 0.0f},
};

const Position MalygosPositions[MAX_MALYGOS_POS] =
{
    {754.544f, 1301.71f, 320.0f,  0.0f},
    {754.39f,  1301.27f, 292.91f, 0.0f},

};

class boss_malygos : public CreatureScript
{
public:
    boss_malygos() : CreatureScript("boss_malygos") { }

    struct boss_malygosAI : public BossAI
    {
        boss_malygosAI(Creature* creature) : BossAI(creature, DATA_MALYGOS_EVENT)
        {
            // If we enter in combat when MovePoint generator is active, it overrwrites our homeposition
            homePosition = creature->GetHomePosition();
        }

        void Reset()
        {
            // workaround for scions calling SetData on wipe for phase pre-3 switch
            phase = PHASE_INTRO;

            BossAI::Reset();

            berserkerTimer = 600000;
            currentPos = 0;

            introTalkTimer = 0;

            delayedMovementTimer = 8000;
            delayedMovement = false;

            currentPos = 0;
            preFightWP = 5;
            summonDeaths = 0;

            SetPhase(PHASE_INTRO, true);


            me->AddUnitMovementFlag(MOVEMENTFLAG_HOVER);
            me->SetDisableGravity(true);
            me->SetCanFly(true);
            me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
            SetCombatMovement(true);

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_1);

            me->SetSpeedRate(MOVE_FLIGHT, 2.5f);
            me->GetMotionMaster()->MoveIdle();

            if (instance)
                instance->DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);

            canMove = true;
            canAttack = true;
            spamProtect = false;
            spamProtectTimer = 0;

            delayedMovementVortex = false;
            delayedMovementTimerVortex = 500;
            delayedMovementToStartVortex = true;

            phaseTwoSwitch = false;
            phaseTwoDelayed = false;

            instance->SetData(DATA_LIGHTING, LIGHT_NATIVE << 16);
        }

        uint32 GetData(uint32 data) const
        {
            if (data == DATA_PHASE)
                return phase;

            return 0;
        }

        void SetData(uint32 data, uint32 /*value*/)
        {
            if (data == DATA_SUMMON_DEATHS && phase == PHASE_TWO)
            {
                summonDeaths++;

                if (summonDeaths >= MAX_SUMMONS_PHASE_TWO)
                {
                    SetPhase(PHASE_PRE_THREE, true);
                    instance->SetData(DATA_LIGHTING, LIGHT_WARP << 16);
                }
            }
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            me->SetHomePosition(homePosition);
            me->AddUnitMovementFlag(MOVEMENTFLAG_HOVER);

            HandleRedDrakes(false);

            BossAI::EnterEvadeMode();

            if (instance)
                instance->SetBossState(DATA_MALYGOS_EVENT, FAIL);
        }

        void SetPhase(uint8 _phase, bool setEvents = false)
        {
            events.Reset();

            events.SetPhase(_phase);
            phase = _phase;

            if (setEvents)
                SetPhaseEvents(_phase);
        }

        void StartPhaseThree()
        {
            if (!instance)
                return;

            SetPhase(PHASE_THREE, true);

            instance->SetData(DATA_LIGHTING, LIGHT_NATIVE << 16);

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_1);

            me->GetMotionMaster()->MoveIdle();
            me->GetMotionMaster()->MovePoint(MOVE_PHASE_3_CENTER, MalygosPositions[0].GetPositionX(), MalygosPositions[0].GetPositionY(), MalygosPositions[0].GetPositionZ());
            me->Attack(GetTargetPhaseThree(), false);
        }

        void SetPhaseEvents(uint8 _phase)
        {
            switch (_phase)
            {
                case PHASE_INTRO:
                    portalMover = true;
                    portalMoverTimer = 0;
                case PHASE_ONE:
                    events.RescheduleEvent(EVENT_ARCANE_BREATH, urand(15000, 20000), 0, _phase);
                    events.RescheduleEvent(EVENT_ARCANE_STORM, urand(5000, 10000), 0, _phase);
                    events.RescheduleEvent(EVENT_VORTEX, urand(33000, 38000), 0, _phase);
                    events.RescheduleEvent(EVENT_POWER_SPARKS, urand(23000, 27000), 0, _phase);
                    break;
                case PHASE_TWO:
                    events.RescheduleEvent(EVENT_YELL_0, 0, 0, _phase);
                    events.RescheduleEvent(EVENT_YELL_1, 24000, 0, _phase);
                    events.RescheduleEvent(EVENT_SURGE_OF_POWER, urand(60000, 70000), 0, _phase);
                    events.RescheduleEvent(EVENT_SUMMON_ARCANE, urand(7000, 9000), 0, _phase);
                    events.RescheduleEvent(EVENT_PHASE_2_ADDS, urand(8000, 10000), 0, _phase);
                    break;
                case PHASE_PRE_THREE:
                    me->GetMotionMaster()->MoveIdle();
                    events.RescheduleEvent(EVENT_YELL_2, 0, 0, _phase);
                    events.RescheduleEvent(EVENT_PLATTFORM_CHANNEL, 4 * IN_MILLISECONDS, 0, _phase);
                    events.RescheduleEvent(EVENT_PLATTFORM_BOOM, 9 * IN_MILLISECONDS, 0, _phase);
                    events.RescheduleEvent(EVENT_DESTROY_PLATTFORM, 11 * IN_MILLISECONDS, 0, _phase);
                    events.RescheduleEvent(EVENT_SUMMON_RED_DRAKES, 11 * IN_MILLISECONDS, 0, _phase);
                    break;
                case PHASE_THREE:
                    events.RescheduleEvent(EVENT_YELL_4, 16000, 0, _phase);
                    events.RescheduleEvent(EVENT_SURGE_OF_POWER_PHASE_3, urand(7000, 16000), 0, _phase);
                    events.RescheduleEvent(EVENT_STATIC_FIELD, urand(20000, 20000), 0, _phase);
                    events.RescheduleEvent(EVENT_ARCANE_PULSE, 10000, 0, _phase);
                    break;
                default:
                    break;
            }
        }

        void EnterCombat(Unit* who)
        {
            BossAI::EnterCombat(who);

            if (instance)
                instance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);

            me->RemoveUnitMovementFlag(MOVEMENTFLAG_HOVER);
            me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
            me->SetDisableGravity(false);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        void KilledUnit(Unit* who)
        {
            if (who->GetTypeId() == TYPEID_PLAYER || who->IsVehicle())
            switch (phase)
            {
                case PHASE_ONE:
                    Talk(SAY_KILLED_PLAYER_P_ONE);
                    break;
                case PHASE_TWO:
                    Talk(SAY_KILLED_PLAYER_P_TWO);
                    break;
                case PHASE_THREE:
                    Talk(SAY_KILLED_PLAYER_P_THREE);
                    break;
            }
        }

        void SpellHit(Unit* caster, const SpellInfo* spell)
        {
            if (spell->Id == SPELL_POWER_SPARK_MALYGOS)
            {
                if (Creature* creature = caster->ToCreature())
                    creature->DespawnOrUnsummon();

                Talk(SAY_BUFF_SPARK);
            }
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (!me->IsInCombat())
                return;

            if (who->GetEntry() == NPC_POWER_SPARK && who->GetExactDist2d(me->GetPositionX(), me->GetPositionY()) <= 2.5f)
            {
                // not sure about the distance | I think it is better check this here than in the UpdateAI function...
                if (!who->HasAura(SPELL_POWER_SPARK_DEATH))
                    who->CastSpell(me, SPELL_POWER_SPARK_MALYGOS, true);
            }
        }

        void PrepareForVortex()
        {
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
            Talk(SAY_VORTEX);
            if (RAID_MODE(1, 0))
            {
                events.RescheduleEvent(EVENT_ARCANE_BREATH, urand(20000, 25000), 0, PHASE_ONE);
                events.RescheduleEvent(EVENT_ARCANE_STORM, urand(17000, 20000), 0, PHASE_ONE);
            }
            else
            {
                events.RescheduleEvent(EVENT_ARCANE_BREATH, urand(19000, 22000), 0, PHASE_ONE);
                events.RescheduleEvent(EVENT_ARCANE_STORM, urand(14000, 16000), 0, PHASE_ONE);
            }
            me->GetMotionMaster()->MovementExpired();
            me->GetMotionMaster()->MovePoint(MOVE_PHASE_1_VORTEX_PREP, MalygosPositions[1].GetPositionX(), MalygosPositions[1].GetPositionY(), me->GetPositionZ());
            // continues in MovementInform function.
        }

        void ExecuteVortex()
        {
            DoCastSelf(SPELL_VORTEX_1, true);
            // the vortex execution continues in the dummy effect of this spell (see its script)
            if (Creature* trigger =  ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_SURGE_TRIGGER))))
                trigger->CastSpell(me, SPELL_VORTEX_2, true);
            DoCastSelf(SPELL_VORTEX_3, true);
        }

        void MovementInform(uint32 /*type*/, uint32 id)
        {
            switch (id)
            {
                case MOVE_PHASE_0_PORTALS:
                    if (Creature* portal = me->FindNearestCreature(NPC_PORTAL_TRIGGER, 50.0f))
                    {
                        me->SetFacingToObject(portal);
                        //me->SendMovementFlagUpdate();
                        me->CastSpell(portal, SPELL_PORTAL_BEAM, false);
                    }
                    portalMover = true;
                    portalMoverTimer = urand(5, 10)*IN_MILLISECONDS;
                    break;
                case MOVE_PHASE_1_VORTEX_PREP:
                    delayedMovementVortex = true;
                    break;
                case MOVE_PHASE_1_VORTEX:
                    me->SetSpeedRate(MOVE_FLIGHT, 1.75f);
                    me->SetDisableGravity(true);
                    me->GetMotionMaster()->MoveIdle();
                    me->HandleEmoteCommand(EMOTE_STATE_CUSTOM_SPELL_01);
                    ExecuteVortex();
                    delayedMovementVortex = true;
                    break;
                case MOVE_PHASE_1_VORTEX_END:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_FLY_SIT_GROUND_DOWN);
                    me->GetMotionMaster()->MoveChase(me->GetVictim());
                    me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
                    me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
                    break;
                case MOVE_PHASE_1_INIT:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_FLY_SIT_GROUND_DOWN);
                    me->RemoveUnitMovementFlag(MOVEMENTFLAG_HOVER);
                    me->SetDisableGravity(false);
                    me->SetInCombatWithZone();
                    break;
                case MOVE_PHASE_2_ROTATION:
                    events.RescheduleEvent(EVENT_PHASE_2_ROTATION, 0); // on next UpdateAI
                    break;
                case MOVE_PHASE_2_DEEP_BREATH:
                    Talk(SAY_EMOTE_BREATH_P2);
                    me->GetMotionMaster()->MoveIdle();
                    delayedMovement = true;
                    if (Creature* trigger = me->FindNearestCreature(30334, 100.0f))
                        DoCast(trigger, SPELL_SURGE_OF_POWER, true);
                    break;
                case MOVE_PHASE_3_CENTER:
                    canMove = false;
                    me->StopMoving();
                    SetCombatMovement(false);
                    // malygos will move into center of platform and then he does not chase dragons, he just turns to his current target.
                    me->GetMotionMaster()->MoveIdle();
                    break;
                default:
                    break;
            }
        }

        void StartPhaseTwo()
        {
            SetPhase(PHASE_TWO, true);
            instance->SetData(DATA_LIGHTING, 2000 + (LIGHT_RUNES << 16));
            me->HandleEmoteCommand(EMOTE_ONESHOT_FLY_SIT_GROUND_UP);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_1);
            me->RemoveAllAuras();
            me->AddUnitMovementFlag(MOVEMENTFLAG_HOVER);
            me->SetDisableGravity(true);
            me->GetMotionMaster()->MoveIdle();
            me->GetMotionMaster()->MoveTakeoff(MOVE_PHASE_2_ROTATION, MalygosCirclingWPs[0]);
        }

        void DoAction(int32 action)
        {
            if (action != ACTION_INIT_PHASE_ONE)
                return;

            SetPhase(PHASE_ONE, true);
            Talk(SAY_AGGRO_P_ONE);
            Position center = {754.255f, 1301.72f ,266.17f, 0.0f};
            float radius = 20.0f;
            float vecX = me->GetPositionX() - center.GetPositionX();
            float vecY = me->GetPositionY() - center.GetPositionY();
            float magV = sqrt(vecX*vecX + vecY*vecY);
            float targetX = center.GetPositionX() + vecX / magV * radius;
            float targetY = center.GetPositionY() + vecY / magV * radius;

            Position posLand;
            posLand.Relocate(targetX, targetY, GROUND_Z);
            me->GetMotionMaster()->MoveLand(MOVE_PHASE_1_INIT, posLand);
        }

        void SpellHitTarget(Unit* /*target*/, SpellInfo const* spell)
        {
            if (spell->Id == SPELL_ARCANE_PULSE && !spamProtect)
            {
                Talk(SAY_SPELL_CASTING_P_THREE);
                spamProtectTimer = 5000;
                spamProtect = true;
            }
        }

        void JustSummoned(Creature* summon)
        {
            summons.Summon(summon);

            if (summon->GetEntry() == NPC_ARCANE_OVERLOAD)
                DoCast(summon, SPELL_ARCANE_BOMB_VISUAL);
        }

        void UpdateAI(uint32 diff)
        {
            if (spamProtectTimer <= diff)
                spamProtect = false;
            else spamProtectTimer -= diff;

            if (phase == PHASE_INTRO)
            {
                if (introTalkTimer <= diff)
                {
                    Talk(RAND(SAY_INTRO1, SAY_INTRO2, SAY_INTRO3));
                    introTalkTimer = urand(30000,90000);
                }
                else
                    introTalkTimer -= diff;
            }

            if (phase == PHASE_INTRO && portalMover)
            {
                if (portalMoverTimer <= diff)
                {
                    //me->SendMovementFlagUpdate();
                    me->SetDisableGravity(true);
                    me->InterruptSpell(CURRENT_CHANNELED_SPELL);
                    me->GetMotionMaster()->MovePoint(MOVE_PHASE_0_PORTALS, MalygosCirclingWPs[preFightWP].GetPositionX(),
                    MalygosCirclingWPs[preFightWP].GetPositionY(), MalygosCirclingWPs[preFightWP].GetPositionZ()+15.0f);
                    preFightWP = preFightWP <= 4 ? 13 : preFightWP - 4;
                    portalMover = false;
                }
                else
                    portalMoverTimer -= diff;
            }

            if (!UpdateVictim())
                return;

            events.Update(diff);


            if (phase == PHASE_THREE && !me->HasAura(SPELL_BERSERK))
            {
                if (!canMove)
                {
                    // it can change if the player falls from the vehicle.
                    if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
                    {
                        me->GetMotionMaster()->MovementExpired();
                        me->GetMotionMaster()->MoveIdle();
                    }
                }
                else if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() != POINT_MOTION_TYPE)
                {
                    me->GetMotionMaster()->MovementExpired();
                    me->GetMotionMaster()->MovePoint(MOVE_PHASE_3_CENTER, MalygosPositions[0].GetPositionX(), MalygosPositions[0].GetPositionY(), MalygosPositions[0].GetPositionZ());
                }
            }

            if (berserkerTimer <= diff)
            {
                DoCast(SPELL_BERSERK);
                canMove = true;
                me->SetSpeedRate(MOVE_FLIGHT, 2.5f);
                berserkerTimer = 10000;
            }
            else
                berserkerTimer -= diff;

            // we need a better way for pathing
            if (delayedMovement)
            {
                if (delayedMovementTimer <= diff)
                {
                    me->GetMotionMaster()->MovePoint(MOVE_PHASE_2_ROTATION, MalygosCirclingWPs[currentPos]);
                    delayedMovementTimer = 15000;
                    delayedMovement = false;
                }
                delayedMovementTimer -= diff;
            }

            if (delayedMovementVortex)
            {
                if (delayedMovementTimerVortex <= diff)
                {
                    me->SetSpeedRate(MOVE_FLIGHT, 3.5f);
                    if (delayedMovementToStartVortex)
                    {
                        me->GetMotionMaster()->MoveTakeoff(MOVE_PHASE_1_VORTEX, MalygosPositions[1]);
                        delayedMovementTimerVortex = 11000;
                        delayedMovementToStartVortex = false;
                    }
                    else
                    {
                        Position posLand;
                        posLand.Relocate(MalygosPositions[1].GetPositionX(), MalygosPositions[1].GetPositionY(), GROUND_Z);
                        me->GetMotionMaster()->MoveLand(MOVE_PHASE_1_VORTEX_END, posLand);
                        delayedMovementToStartVortex = true;
                        delayedMovementTimerVortex = 500;
                        if (phaseTwoSwitch && phaseTwoDelayed)
                            StartPhaseTwo();
                        phaseTwoDelayed = false;
                    }
                    delayedMovementVortex = false;
                }
                else
                    delayedMovementTimerVortex -= diff;
            }

            // We can't cast if we are casting already. The Trigger Cast Spell Vortex
            Unit* trigger = ObjectAccessor::GetUnit(*me, ObjectGuid(instance->GetGuidData(DATA_VORTEX_TRIGGER)));
            if (trigger && trigger->HasUnitState(UNIT_STATE_CASTING))
            {
                canAttack = false;
                return;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING) || me->HasAura(SPELL_VORTEX_2))
                return;

            // at 50 % health malygos switch to phase 2
            if (me->GetHealthPct() <= 50.0f && phase == PHASE_ONE)
            {
                if (phaseTwoDelayed)
                    phaseTwoSwitch = true;
                else
                    StartPhaseTwo();
            }

            if (!canAttack)
            {
                if (me->GetPositionZ() <= 1.f+GROUND_Z)
                {
                    me->Attack(me->GetVictim(), true);
                    canAttack = true;
                    me->SetSpeedRate(MOVE_FLIGHT, 3.5f);
                }
            }

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_YELL_0:
                        Talk(SAY_END_P_ONE);
                        break;
                    case EVENT_YELL_1:
                        Talk(SAY_AGGRO_P_TWO);
                        break;
                    case EVENT_YELL_2:
                        Talk(SAY_END_P_TWO);
                        break;
                    case EVENT_YELL_4:
                        Talk(SAY_AGGRO_P_THREE);
                        break;
                    case EVENT_ARCANE_BREATH:
                        DoCastVictim(SPELL_ARCANE_BREATH);
                        events.RescheduleEvent(EVENT_ARCANE_BREATH, urand(35000, 60000), 0, PHASE_ONE);
                        break;
                    case EVENT_ARCANE_STORM:
                        if (Unit* player = SelectTarget(SELECT_TARGET_RANDOM))
                            me->CastSpell(player, SPELL_ARCANE_STORM, true);
                        events.RescheduleEvent(EVENT_ARCANE_STORM, urand(5000, 10000), 0, PHASE_ONE);
                        break;
                    case EVENT_VORTEX:
                        PrepareForVortex();
                        phaseTwoDelayed = true;
                        events.RescheduleEvent(EVENT_VORTEX, urand(60000, 66000), 0, PHASE_ONE);
                        break;
                    case EVENT_POWER_SPARKS:
                        instance->SetData(DATA_POWER_SPARKS_HANDLING, 0);
                        events.RescheduleEvent(EVENT_POWER_SPARKS, urand(29000, 31000), 0, PHASE_ONE);
                        break;
                    case EVENT_PHASE_2_ADDS:
                        for (uint8 i = 0; i < RAID_MODE(4,8); i++)
                        {
                            if (Creature* disc = me->SummonCreature(NPC_HOVER_DISK_CASTER, HoverDiskWaypoints[i*RAID_MODE(4,2)], TEMPSUMMON_MANUAL_DESPAWN))
                                disc->AI()->DoAction(i*RAID_MODE(4,2));
                        }
                        for (uint8 i = 0; i < RAID_MODE(2,4); i++)
                        {
                            if (Creature* disc = me->SummonCreature(NPC_HOVER_DISK_MELEE, HoverDiskWaypoints[0], TEMPSUMMON_MANUAL_DESPAWN))
                            {
                                disc->SetInCombatWithZone();
                                if (Unit* playertarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 50000.0f, true))
                                    disc->GetMotionMaster()->MovePoint(0, playertarget->GetPositionX(), playertarget->GetPositionY(), playertarget->GetPositionZ());
                            }
                        }
                        break;
                    case EVENT_PHASE_2_ROTATION:
                        currentPos = currentPos == MALYGOS_MAX_WAYPOINTS - 1 ? 0 : currentPos+1;
                        me->GetMotionMaster()->MovementExpired();
                        me->GetMotionMaster()->MovePoint(MOVE_PHASE_2_ROTATION, MalygosCirclingWPs[currentPos]);
                        break;
                    case EVENT_SURGE_OF_POWER:
                        Talk(SAY_P2_SURGE_OF_POWER);
                        me->GetMotionMaster()->MovePoint(MOVE_PHASE_2_DEEP_BREATH, MalygosPositions[0]);
                        events.RescheduleEvent(EVENT_SUMMON_ARCANE, 15000, 0, PHASE_TWO);
                        events.RescheduleEvent(EVENT_SURGE_OF_POWER, urand(60000, 70000), 0, PHASE_TWO);
                        break;
                    case EVENT_SUMMON_ARCANE:
                        DoCast(SPELL_SUMMON_ARCANE_BOMB);
                        Talk(SAY_ANTI_MAGIC_SHELL);
                        events.RescheduleEvent(EVENT_SUMMON_ARCANE, urand(12000, 15000), 0, PHASE_TWO);
                        break;
                    case EVENT_PLATTFORM_CHANNEL:
                        me->GetMotionMaster()->MoveIdle();
                        if (Creature* trigger =  ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_SURGE_TRIGGER))))
                        {
                            trigger->CastSpell(trigger, SPELL_PLATTFORM_CHANNEL, false);
                            me->SetFacingToObject(trigger);
                        }
                        break;
                   case EVENT_PLATTFORM_BOOM:
                        if (Creature* trigger = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_SURGE_TRIGGER))))
                           trigger->CastSpell(trigger, SPELL_PLATTFORM_BOOM, false);
                        break;
                   case EVENT_DESTROY_PLATTFORM:
                        if (Creature* proxy = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_PROXY))))
                            proxy->AI()->Talk(SAY_DRAGONS_SPAWN);

                        // this despawns Hover Disks
                        summons.DespawnAll();
                        // players that used Hover Disk are not in the aggro list
                        me->SetInCombatWithZone();

                        if (GameObject* go = GameObject::GetGameObject(*me, ObjectGuid(instance->GetGuidData(DATA_PLATFORM))))
                            go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);

                        me->GetMotionMaster()->MovePoint(MOVE_PHASE_3_CENTER, MalygosPositions[1]);
                        break;
                    case EVENT_SUMMON_RED_DRAKES:
                        Talk(SAY_INTRO_P_THREE);
                        HandleRedDrakes(true);
                        StartPhaseThree();
                        break;
                    case EVENT_SURGE_OF_POWER_PHASE_3:
                        Talk(SAY_SURGE_POWER);
                        if (Is25ManRaid())
                            DoCastAOE(SPELL_SURGE_OF_POWER_P3);
                        else if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.f, true))
                        {
                            Talk(SAY_SURGE_WARNING_P_THREE);
                            if (Unit * veh = target->GetVehicleBase())
                                DoCast(veh, SPELL_SURGE_OF_POWER_P3);
                        }
                        events.RescheduleEvent(EVENT_SURGE_OF_POWER_PHASE_3, urand(7000, 16000), 0, PHASE_THREE);
                        break;
                    case EVENT_STATIC_FIELD:
                        Talk(SAY_SPELL_CASTING_P_THREE);
                        DoCast(GetTargetPhaseThree(), SPELL_STATIC_FIELD);
                        events.RescheduleEvent(EVENT_STATIC_FIELD, urand(20000, 30000), 0, PHASE_THREE);
                        break;
                    case EVENT_ARCANE_PULSE:
                        DoCast(SPELL_ARCANE_PULSE);
                        events.RescheduleEvent(EVENT_ARCANE_PULSE, 1000, 0, PHASE_THREE);
                        break;
                    default:
                        break;
                }
            }

            if (canAttack && phase != PHASE_THREE)
                DoMeleeAttackIfReady();
        }

        void HandleRedDrakes(bool apply)
        {
            for (Player& player : instance->instance->GetPlayers())
            {
                if (player.IsGameMaster() || !player.IsAlive())
                    continue;

                if (apply)
                    player.CastSpell(&player, SPELL_SUMMON_RED_DRAGON, true);
                else
                    player.ExitVehicle(); // not happy with this
            }
        }

        Unit* GetTargetPhaseThree()
        {
            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
            {
                // we are a drake
                if (target->GetVehicleKit())
                    return target;

                // we are a player using a drake (or at least you should)
                if (target->GetTypeId() == TYPEID_PLAYER)
                {
                    if (Unit* base = target->GetVehicleBase())
                        return base;
                }
            }
            // is a player falling from a vehicle?
            return NULL;
        }

        void JustDied(Unit* killer)
        {
            BossAI::JustDied(killer);
            Talk(SAY_DEATH);

            for (Player& player : instance->instance->GetPlayers())
            {
                if (player.GetQuestStatus(RAID_MODE(QUEST_JUDGEMENT_10, QUEST_JUDGEMENT_25)) == QUEST_STATUS_INCOMPLETE)
                {
                    me->SummonGameObject(RAID_MODE(GO_HEART_OF_MAGIC_10, GO_HEART_OF_MAGIC_25), HearthOfMagicPos.GetPositionX(), HearthOfMagicPos.GetPositionY(), HearthOfMagicPos.GetPositionZ(), HearthOfMagicPos.GetOrientation(), 0, 0, 0, 0, 7*DAY);
                    break;
                }
            }

            instance->SetData(DATA_LIGHTING, 2000 + (LIGHT_CLOUDS << 16));

            me->DespawnOrUnsummon();
        }

    private:
        uint8 phase;
        uint32 berserkerTimer;
        uint8 currentPos;               // used for phase 2 rotation...
        bool delayedMovement;           // used in phase 2
        uint32 delayedMovementTimer;    // used in phase 2
        uint32 introTalkTimer;
        uint32 portalMoverTimer;
        uint8 preFightWP;
        uint8 summonDeaths;
        Position homePosition;          // it can get bugged because core thinks we are pathing
        bool portalMover;
        bool canMove;
        bool canAttack;
        bool delayedMovementVortex;
        uint32 delayedMovementTimerVortex;
        bool delayedMovementToStartVortex;
        bool spamProtect;
        uint32 spamProtectTimer;
        bool phaseTwoSwitch;
        bool phaseTwoDelayed;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_malygosAI(creature);
    }
};

class npc_portal_eoe: public CreatureScript
{
public:
    npc_portal_eoe() : CreatureScript("npc_portal_eoe") { }

    struct npc_portal_eoeAI : public ScriptedAI
    {
        npc_portal_eoeAI(Creature* creature) : ScriptedAI(creature), instance(creature->GetInstanceScript()) { }

        void Reset()
        {
            summonTimer = 0;
            portalCloseTimer = 0;
            summonedSpark = false;
        }

        void SpellHit(Unit* /*caster*/, const SpellInfo* spell)
        {
            if (spell->Id == SPELL_PORTAL_OPENED)
            {
                portalCloseTimer = 12000;
                summonTimer = 1000;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (instance)
                if (Creature* malygos = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_MALYGOS))))
                    if (malygos->AI()->GetData(DATA_PHASE) != PHASE_ONE)
                    {
                        if (me->HasAura(SPELL_PORTAL_OPENED))
                        {
                            me->RemoveAura(SPELL_PORTAL_OPENED);
                            DoCastSelf(SPELL_PORTAL_VISUAL_CLOSED, true);
                        }
                        return;
                    }

            if (!me->HasAura(SPELL_PORTAL_OPENED))
                return;

            if (!summonedSpark)
            {
                if (summonTimer <= diff)
                {
                    DoCast(SPELL_SUMMON_POWER_SPARK);

                    summonedSpark = true;
                }
                else
                    summonTimer -= diff;
            }
            else
            {
                if (portalCloseTimer <= diff)
                {
                    DoCastSelf(SPELL_PORTAL_VISUAL_CLOSED, true);
                    me->RemoveAura(SPELL_PORTAL_OPENED);
                    summonedSpark = false;
                }
                else
                    portalCloseTimer -= diff;
            }
        }

        void JustSummoned(Creature* summon)
        {
            summon->SetInCombatWithZone();
        }

    private:
        bool summonedSpark;
        uint32 summonTimer;
        uint32 portalCloseTimer;
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_portal_eoeAI(creature);
    }
};

class npc_power_spark: public CreatureScript
{
public:
    npc_power_spark() : CreatureScript("npc_power_spark") { }

    struct npc_power_sparkAI : public PassiveAI
    {
        npc_power_sparkAI(Creature* creature) : PassiveAI(creature), instance(creature->GetInstanceScript())
        {
            me->SetDisableGravity(true);
            me->SetSpeedRate(MOVE_FLIGHT, 0.5f);
            me->SetSpeedRate(MOVE_RUN, 0.5f);
            // Talk range was not enough for this encounter
            Talk(EMOTE_POWER_SPARK_SUMMONED);
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            me->DespawnOrUnsummon();
        }

        void UpdateAI(uint32 /*diff*/)
        {
            if (Creature* malygos = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_MALYGOS))))
            {
                if (malygos->AI()->GetData(DATA_PHASE) != PHASE_ONE)
                {
                    me->DespawnOrUnsummon();
                    return;
                }
                else if (malygos)
                {
                    me->GetMotionMaster()->MovePoint(0, malygos->GetPositionX(), malygos->GetPositionY(), malygos->GetPositionZ());
                }
            }
        }

        void JustDied(Unit* killer)
        {
            if (me->GetPositionZ() > GROUND_Z)
                me->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), GROUND_Z, 0);

            if (killer && !killer->IsCharmedOwnedByPlayerOrPlayer())
                DoCast(killer, SPELL_POWER_SPARK_MALYGOS, true);
            else
                DoCastAOE(SPELL_POWER_SPARK_DEATH, true);
        }

    private:
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_power_sparkAI(creature);
    }
};

class npc_hover_disk : public CreatureScript
{
public:
    npc_hover_disk() : CreatureScript("npc_hover_disk") { }

    struct npc_hover_diskAI : public npc_escortAI
    {
        npc_hover_diskAI(Creature* creature) : npc_escortAI(creature), instance(creature->GetInstanceScript())
        {
            // when hit by an arcane bomb disk is unmoveable
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_ARCANE_BOMB, true);

            if (me->GetEntry() == NPC_HOVER_DISK_CASTER)
                me->SetReactState(REACT_PASSIVE);
        }

        void Reset()
        {
            reverseFlyer = false;
        }

        // Prevents disabling AI
        void OnCharmed(bool /*apply*/) {}

        void PassengerBoarded(Unit* unit, int8 /*seat*/, bool apply)
        {
            if (apply)
            {
                if (unit->GetTypeId() == TYPEID_UNIT)
                {
                    me->setFaction(FACTION_HOSTILE);
                    if (Creature* maly = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_MALYGOS)) : ObjectGuid::Empty))
                        maly->AI()->JustSummoned(unit->ToCreature());
                }
            }
            else
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                me->GetMotionMaster()->MoveIdle();
                if (me->GetPositionZ() > GROUND_Z)
                    me->GetMotionMaster()->MoveFall();

                me->SetCanFly(true);
                me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), GROUND_Z, me->GetOrientation());
                me->setFaction(FACTION_FRIENDLY_TO_ALL);
                SetEscortPaused(true);
                me->SetReactState(REACT_PASSIVE);
            }
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            // we dont evade
        }

        void DoAction(int32 action)
        {
            if (me->GetEntry() != NPC_HOVER_DISK_CASTER)
                return;

            if (action > 14 || action < 0)
                return;

            // 0 2 4 6 8 10 12 14
            if ( action%4 == 0 )
                reverseFlyer = true;

            const uint32 done = action;
            uint32 i = action;
            uint32 count = 0;
            do
            {
                AddWaypoint(count, HoverDiskWaypoints[i].GetPositionX(), HoverDiskWaypoints[i].GetPositionY(), HoverDiskWaypoints[i].GetPositionZ());
                count++;

                if (!reverseFlyer)
                    i = i == MAX_HOVER_DISK_WAYPOINTS-1 ? 0 : 1+i;
                else
                    i = i == 0 ? MAX_HOVER_DISK_WAYPOINTS-1 : i-1;
            }
            while (i != done);

            Start(true, false, ObjectGuid::Empty, 0, false, true);
        }

        void UpdateEscortAI(const uint32 /*diff*/)
        {
            if (!(me->GetUnitMovementFlags() & MOVEMENTFLAG_CAN_FLY))
                me->SetCanFly(true);
        }

        void WaypointReached(uint32 /*i*/)
        {
        }

    private:
        InstanceScript* instance;
        bool reverseFlyer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_hover_diskAI(creature);
    }
};

class npc_nexus_lord : public CreatureScript
{
public:
    npc_nexus_lord() : CreatureScript("npc_nexus_lord") { }

    struct npc_nexus_lordAI : public ScriptedAI
    {
        npc_nexus_lordAI(Creature* creature) : ScriptedAI(creature), instance(creature->GetInstanceScript()) { }

        void Reset()
        {
            shockTimer = urand(9000, 12000);
            hasteTimer = urand(10000, 15000);
        }

        void JustDied(Unit* /*killer*/)
        {
            if (instance)
                if (Creature* malygos = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_MALYGOS))))
                    malygos->AI()->SetData(DATA_SUMMON_DEATHS, 1);

            me->DespawnOrUnsummon();
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (shockTimer <= diff)
            {
                DoCastVictim(SPELL_ARCANE_SHOCK);
                shockTimer = urand(9000, 12000);
            }
            else
                shockTimer -= diff;

            if (hasteTimer <= diff)
            {
                DoCastSelf(SPELL_HASTE);
                hasteTimer = urand(17000, 27000);
            }
            else
                hasteTimer -= diff;

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        uint32 shockTimer;
        uint32 hasteTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_nexus_lordAI (creature);
    }
};

class npc_scion_of_eternity : public CreatureScript
{
public:
    npc_scion_of_eternity() : CreatureScript("npc_scion_of_eternity") { }

    struct npc_scion_of_eternityAI : public ScriptedAI
    {
        npc_scion_of_eternityAI(Creature* creature) : ScriptedAI(creature), instance(creature->GetInstanceScript()) { }

        void Reset()
        {
            arcaneTimer = urand(4000, 6000);
            me->SetHealth(RAID_MODE<uint32>(201600, 302400));
            me->SetPower(POWER_MANA, RAID_MODE<uint32>(220350, 440700));
            killerGUID.Clear();
        }

        void DamageTaken(Unit* attacker, uint32& damage)
        {
            if (attacker && me->GetHealth() < damage)
                killerGUID = attacker->GetGUID();
        }

        void JustDied(Unit* killer)
        {
            me->DespawnOrUnsummon();

            if (killer)
                killerGUID = killer->GetGUID();

            if (instance)
                if (Creature* malygos = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_MALYGOS))))
                    malygos->AI()->SetData(DATA_SUMMON_DEATHS, 1);
        }

        ObjectGuid GetGuidData(uint32 /*id*/ = 0) const override
        {
            return killerGUID;
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (arcaneTimer <= diff)

            {
                if (Unit* player = SelectTarget(SELECT_TARGET_RANDOM))
                    me->CastSpell(player, SPELL_ARCANE_BARRAGE, true);
                arcaneTimer = urand(4000, 6000);
            }
            else
                arcaneTimer -= diff;
        }

    private:
        InstanceScript* instance;
        uint32 arcaneTimer;
        ObjectGuid killerGUID;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_scion_of_eternityAI (creature);
    }
};

class npc_arcane_overload : public CreatureScript
{
public:
    npc_arcane_overload() : CreatureScript("npc_arcane_overload") { }

    struct npc_arcane_overloadAI : public PassiveAI
    {
        npc_arcane_overloadAI(Creature* creature) : PassiveAI(creature) { }

        uint32 DespawnTimer;

        void Reset()
        {
            DespawnTimer = 120000;
        }

        void SpellHit(Unit* /*caster*/, const SpellInfo* spell)
        {
            if (spell->Id == uint32(SPELL_ARCANE_BOMB_VISUAL))
            {
                DoCastAOE(SPELL_ARCANE_BOMB);
                DoCastSelf(SPELL_ARCANE_OVERLOAD, false);
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (DespawnTimer < diff)
            {
                me->DespawnOrUnsummon();
            }else DespawnTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_arcane_overloadAI (creature);
    }
};

class npc_wyrmrest_skytalon : public CreatureScript
{
public:
    npc_wyrmrest_skytalon() : CreatureScript("npc_wyrmrest_skytalon") { }

    struct npc_wyrmrest_skytalonAI : public VehicleAI
    {
        npc_wyrmrest_skytalonAI(Creature* creature) : VehicleAI(creature), instance(creature->GetInstanceScript())
        {
            timer = 1500;
            entered = false;
        }

        void PassengerBoarded(Unit* unit, int8 /*seat*/, bool apply)
        {
            if (!apply)
            {
                if (unit->GetTypeId() == TYPEID_PLAYER)
                    me->DespawnOrUnsummon(5000);
                if (InstanceScript* instance = me->GetInstanceScript())
                    if (instance->GetBossState(DATA_MALYGOS_EVENT) == IN_PROGRESS)
                        unit->Kill(unit, true);
            }
        }

        void MakePlayerEnter()
        {
            if (!instance)
                return;

            if (Unit* summoner = me->ToTempSummon()->GetSummoner())
            {
                if (Creature* malygos = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_MALYGOS))))
                {
                    summoner->CastSpell(me, SPELL_RIDE_RED_DRAGON, true);
                    me->setFaction(summoner->getFaction());
                    malygos->GetThreatManager().resetAllAggro();
                    malygos->AI()->AttackStart(me);

                    float victim_threat = malygos->GetThreatManager().getThreat(summoner);

                    malygos->GetThreatManager().addThreat(me, victim_threat ? victim_threat : 1.0f);
                    //AddThreat(me, victim_threat ? victim_threat : 1.0f, malygos);
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!entered)
            {
                if (timer <= diff)
                {
                    MakePlayerEnter();
                    entered = true;
                }
                else
                    timer -= diff;
            }
        }

    private:
        InstanceScript* instance;
        uint32 timer;
        bool entered;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_wyrmrest_skytalonAI (creature);
    }
};

enum AlexstraszaEvents
{
    EVENT_A_YELL_1 = 1,
    EVENT_A_YELL_2,
    EVENT_A_YELL_3,
    EVENT_A_YELL_4,
    EVENT_A_CREATE_GIFT,
    EVENT_A_MOVE_IN
};

class npc_alexstrasza_eoe : public CreatureScript
{
public:
    npc_alexstrasza_eoe() : CreatureScript("npc_alexstrasza_eoe") { }

    struct npc_alexstrasza_eoeAI : public ScriptedAI
    {
        npc_alexstrasza_eoeAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            events.Reset();
            me->SetSpeedRate(MOVE_FLIGHT, 2.0f);
            events.RescheduleEvent(EVENT_A_MOVE_IN, 0);
            me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_A_MOVE_IN:
                        me->GetMotionMaster()->MovePoint(0, 788.254761f, 1326.236938f, GROUND_Z+20);
                        events.RescheduleEvent(EVENT_A_CREATE_GIFT, 8*IN_MILLISECONDS);
                        break;
                    case EVENT_A_CREATE_GIFT:
                        if (Creature* gift = me->FindNearestCreature(NPC_ALEXSTRASZA_S_GIFT, 200.0f))
                        {
                            me->SetFacingToObject(gift);
                            DoCast(gift, SPELL_GIFT_VISUAL, true);
                            DoCast(gift, SPELL_GIFT_CHANNEL);
                        }
                        events.RescheduleEvent(EVENT_A_YELL_1, 2*IN_MILLISECONDS);
                        break;
                    case EVENT_A_YELL_1: {

                        Player* lootrecipient = NULL;

                        if (Creature* giftNPC = me->FindNearestCreature(NPC_ALEXSTRASZA_S_GIFT, 200.0f)) {
                            if (GameObject* giftGO = me->SummonGameObject(RAID_MODE(GO_ALEXSTRASZA_S_GIFT_10, GO_ALEXSTRASZA_S_GIFT_25), giftNPC->GetPositionX(), giftNPC->GetPositionY(), giftNPC->GetPositionZ(), giftNPC->GetOrientation(), 0, 0, 0, 0, 7*DAY)) {

                                giftGO->SetLootState(GO_READY);

                                // Try to get raid leader GUID from instance script
                                if (InstanceScript* instance = me->GetInstanceScript()) {
                                    if (ObjectGuid raidLeaderGUID = ObjectGuid(instance->GetGuidData(DATA_RAID_LEADER_GUID)))
                                        lootrecipient = ObjectAccessor::GetPlayer(*me, raidLeaderGUID);
                                }

                                // if it failed, simply take nearest player
                                if (!lootrecipient)
                                    lootrecipient = me->FindNearestPlayer(300.0f);

                                // set loot recipient
                                if (lootrecipient)
                                    giftGO->SetLootRecipient(lootrecipient);

                            }
                        }

                        Talk(SAY_ONE);
                        events.RescheduleEvent(EVENT_A_YELL_2, 7000);
                        break;
                    }
                    case EVENT_A_YELL_2:
                        Talk(SAY_TWO);
                        events.RescheduleEvent(EVENT_A_YELL_3, 5000);
                        break;
                    case EVENT_A_YELL_3:
                        Talk(SAY_THREE);
                        events.RescheduleEvent(EVENT_A_YELL_4, 23000);
                        break;
                    case EVENT_A_YELL_4:
                        Talk(SAY_FOUR);
                        break;
                }
            }
        }
    private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_alexstrasza_eoeAI (creature);
    }
};

class npc_trigger_invincible : public CreatureScript
{
public:
    npc_trigger_invincible() : CreatureScript("npc_trigger_invincible") { }

    struct npc_trigger_invincibleAI : public PassiveAI
    {
        npc_trigger_invincibleAI(Creature* creature) : PassiveAI(creature)
        {
            me->SetDisplayId(11686); // invisible
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage)
        {
            damage = 0;
        }

        void UpdateAI(uint32 /*diff*/)
        {
            if (me->IsInCombat() && me->getAttackers().empty() && !me->HasAura(SPELL_SURGE_OF_POWER))
                EnterEvadeMode();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_trigger_invincibleAI (creature);
    }
};

class achievement_denyin_the_scion : public AchievementCriteriaScript
{
public:
    achievement_denyin_the_scion() : AchievementCriteriaScript("achievement_denyin_the_scion") { }

    bool OnCheck(Player* source, Unit* target)
    {
        // only reward the killer
        if (!target || !target->ToCreature() || !target->ToCreature()->AI()->GetGuidData(0) || target->ToCreature()->AI()->GetGuidData(0) != source->GetGUID())
            return false;

        // when on a hover disk
        if (Unit* disk = source->GetVehicleBase())
            if (disk->GetEntry() == NPC_HOVER_DISK_CASTER || disk->GetEntry() == NPC_HOVER_DISK_MELEE)
                return true;

        return false;
    }
};

class go_focusing_iris : public GameObjectScript
{
public:
    go_focusing_iris() : GameObjectScript("go_focusing_iris") { }

    enum Spells
    {
        SPELL_IRIS_OPENED = 61012,
    };

    bool OnGossipHello(Player* /*player*/, GameObject* go)
    {
        if (Unit * trigger = go->FindNearestCreature(22517, 10.0f))
            trigger->CastSpell(trigger, SPELL_IRIS_OPENED, true);
        return false;
    }
};

class spell_malygos_vortex_dummy : public SpellScriptLoader
{
public:
    spell_malygos_vortex_dummy() : SpellScriptLoader("spell_malygos_vortex_dummy") { }

    class spell_malygos_vortex_dummy_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_malygos_vortex_dummy_SpellScript)

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
                // each player will enter to the trigger vehicle (entry 30090) already spawned (each one can hold up to 5 players, it has 5 seats)
                // the players enter to the vehicles casting SPELL_VORTEX_4 OR SPELL_VORTEX_5
                if (InstanceScript* instance = caster->GetInstanceScript())
                    instance->SetData(DATA_VORTEX_HANDLING, 0);

            // the rest of the vortex execution continues when SPELL_VORTEX_2 is removed.
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_malygos_vortex_dummy_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_malygos_vortex_dummy_SpellScript();
    }
};

class spell_malygos_vortex_visual : public SpellScriptLoader
{
public:
    spell_malygos_vortex_visual() : SpellScriptLoader("spell_malygos_vortex_visual") { }

    class spell_malygos_vortex_visual_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_malygos_vortex_visual_AuraScript);

        void HandleEffectPeriodicUpdate(AuraEffect* /*aurEff*/)
        {
            if (Unit* target = GetUnitOwner())
                target->InterruptNonMeleeSpells(false);
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            InstanceScript* instance = caster->GetInstanceScript();
            if (!instance)
                return;

            Unit* trigger = ObjectAccessor::GetUnit(*caster, ObjectGuid(instance->GetGuidData(DATA_VORTEX_TRIGGER)));
            if (!trigger)
                return;

            const ThreatContainer::StorageType &m_threatlist = caster->GetThreatManager().getThreatList();
            for (std::list<HostileReference*>::const_iterator itr = m_threatlist.begin(); itr!= m_threatlist.end(); ++itr)
            {
                if (Unit* target = (*itr)->getTarget())
                {
                    Player* targetPlayer = target->ToPlayer();
                    if (!targetPlayer || targetPlayer->IsGameMaster())
                        continue;

                    // teleport spell - i am not sure but might be it must be casted by each vehicle when its passenger leaves it
                    trigger->CastSpell(targetPlayer, SPELL_VORTEX_6, true);
                }
            }

            if (Creature* malygos = caster->ToCreature())
            {
                // This is a hack, we have to re add players to the threat list because when they enter to the vehicles they are removed.
                // Anyway even with this issue, the boss does not enter in evade mode - this prevents iterate an empty list in the next vortex execution.
                malygos->SetInCombatWithZone();

                malygos->RemoveUnitMovementFlag(MOVEMENTFLAG_HOVER);
                malygos->SetDisableGravity(false);
                malygos->AttackStop();
                malygos->GetMotionMaster()->MovePoint(0, 754.255f, 1301.72f, 266.17f);
                malygos->RemoveAura(SPELL_VORTEX_1);
            }
        }

        void Register()
        {
            OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_malygos_vortex_visual_AuraScript::HandleEffectPeriodicUpdate, EFFECT_0, SPELL_AURA_DUMMY);
            AfterEffectRemove += AuraEffectRemoveFn(spell_malygos_vortex_visual_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_malygos_vortex_visual_AuraScript();
    }
};

class ArcaneOverloadRangeCheck
{
    public:
        explicit ArcaneOverloadRangeCheck(Unit * caster, float range) : _range(range)
        {
            caster->GetPosition(&_pos);
        }

        bool operator()(WorldObject* object)
        {
            // from WorldObject::_IsWithinDist without GetObjectSize()
            float dx = _pos.GetPositionX() - object->GetPositionX();
            float dy = _pos.GetPositionY() - object->GetPositionY();
            float dz = _pos.GetPositionZ() - object->GetPositionZ();
            float distsq = dx*dx + dy*dy + dz*dz;

            return distsq > _range * _range;
        }

    private:
        float _range;
        Position _pos;
};

// 56438 - Arcane Overload (Triggered #1)
class spell_malygos_arcane_overload : public SpellScriptLoader
{
public:
    spell_malygos_arcane_overload() : SpellScriptLoader("spell_malygos_arcane_overload") { }

    enum Spells
    {
        SPELL_ARCANE_OVERLOAD_SCALE_AURA = 56435,
    };

    class spell_malygos_arcane_overload_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_malygos_arcane_overload_SpellScript)

        void TargetSelect(std::list<WorldObject*>& targetList)
        {
            Unit * caster = GetCaster();
            if (!caster)
                return;

            Aura * scaleAura = caster->GetAura(SPELL_ARCANE_OVERLOAD_SCALE_AURA);
            if (!scaleAura)
                return;

            // radius 12.0 -> reduction at 1 tick per sec for duration of 45 secs
            float range = GetSpellInfo()->Effects[0].CalcRadius() * (1.f - scaleAura->GetStackAmount()/45.f);

            // the magic happens here, remove targets out of $range
            targetList.remove_if(ArcaneOverloadRangeCheck(caster, range>0.f ? range : 0.f));
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_malygos_arcane_overload_SpellScript::TargetSelect, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_malygos_arcane_overload_SpellScript();
    }
};

class SurgeOfPowerCheck
{
    public:
        explicit SurgeOfPowerCheck() { }

        bool operator()(WorldObject* object)
        {
            Unit* unit = object->ToUnit();
            return unit->IsVehicle() || (unit->ToPlayer() && unit->GetVehicle());
        }
};

// 56548 - Surge of Power
class spell_malygos_surge_of_power : public SpellScriptLoader
{
public:
    spell_malygos_surge_of_power() : SpellScriptLoader("spell_malygos_surge_of_power") { }

    class spell_malygos_surge_of_power_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_malygos_surge_of_power_SpellScript)

        void TargetSelect(std::list<WorldObject*>& targetList)
        {
            // ignore vehicles themself and players on a vehicle
            targetList.remove_if(SurgeOfPowerCheck());
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_malygos_surge_of_power_SpellScript::TargetSelect, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_malygos_surge_of_power_SpellScript();
    }
};

class ArcaneBarrageCheck
{
    public:
        explicit ArcaneBarrageCheck() { }

        bool operator()(WorldObject* object)
        {
            Unit* unit = object->ToUnit();
            return unit->IsVehicle() || (unit->ToPlayer() && unit->GetVehicle()) || unit->HasAura(63934);
        }
};

// 56397 - Arcane Barrage
class spell_malygos_arcane_barrage : public SpellScriptLoader
{
public:
    spell_malygos_arcane_barrage() : SpellScriptLoader("spell_malygos_arcane_barrage") { }

    enum Spells
    {
        SPELL_ARCANE_BARRAGE_TRIGGER = 63934,
    };

    class spell_malygos_arcane_barrage_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_malygos_arcane_barrage_SpellScript)

        bool Validate(SpellInfo const* /*spell*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_ARCANE_BARRAGE_TRIGGER))
                return false;
            return true;
        }

        void TargetSelect(std::list<WorldObject*>& targetList)
        {
            // ignore vehicles themself, ignore players on a vehicle and units with protection aura
            targetList.remove_if(ArcaneBarrageCheck());

            if (targetList.empty())
                return;

            // get one random target
            std::list<WorldObject*>::iterator itr = targetList.begin();
            std::advance(itr, urand(0, targetList.size() - 1));
            Unit * target = (*itr)->ToUnit();
            targetList.clear();
            targetList.push_back(target);
        }

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            Unit * caster = GetCaster();
            Unit * target = GetHitUnit();

            if (!caster || !target)
                return;

            target->CastSpell(target, SPELL_ARCANE_BARRAGE_TRIGGER, true, NULL, NULL, caster->GetGUID());
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_malygos_arcane_barrage_SpellScript::TargetSelect, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            OnEffectHitTarget += SpellEffectFn(spell_malygos_arcane_barrage_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);;
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_malygos_arcane_barrage_SpellScript();
    }
};

// 60936 - Surge of Power (Phase 3)
class spell_malygos_surge_of_power_phase3 : public SpellScriptLoader
{
public:
    spell_malygos_surge_of_power_phase3() : SpellScriptLoader("spell_malygos_surge_of_power_phase3") { }

    class spell_malygos_surge_of_power_phase3_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_malygos_surge_of_power_phase3_SpellScript)


        bool Load()
        {
            firstCall = false;
            return true;
        }

        SpellCastResult Warning()
        {
            /*
             This is so wrong: OnCheckCast is currently the only hook being called, before
             the spell is actually casted, as in castbar casted.
            */

            Unit* caster = GetCaster();
            if (!caster || !caster->ToCreature())
                return SPELL_CAST_OK;

            if (firstCall)
                return SPELL_CAST_OK;
            else
                firstCall = !firstCall;

            targets.clear();

            std::list<Creature*> creatureList;
            caster->GetCreatureListWithEntryInGrid(creatureList, NPC_WYRMREST_SKYTALON, 5000.f);

            if (creatureList.empty())
                return SPELL_CAST_OK;

            uint32 max_count = caster->GetMap()->GetSpawnMode() & 1 /* == 25man */ ? 3 : 1;
            for (uint32 x=0; x<max_count; x++)
            {
                std::list<Creature*>::iterator itr = creatureList.begin();
                std::advance(itr, urand(0, creatureList.size() - 1));
                targets.push_back((*itr)->GetGUID());

                if ((*itr)->GetVehicleKit())
                    if (Unit * passenger = (*itr)->GetVehicleKit()->GetPassenger(0))
                        if (Creature* malygos = caster->ToCreature())
                            malygos->AI()->Talk(SAY_SURGE_WARNING_P_THREE);
            }

            return SPELL_CAST_OK;
        }

        void TargetSelect(std::list<WorldObject*>& targetList)
        {
            targetList.clear();

            Unit * caster = GetCaster();
            if (!caster)
                return;

            if (!targets.empty())
            {
                for (std::list<ObjectGuid>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
                {
                    if (Unit* target = ObjectAccessor::GetUnit(*caster, *itr))
                        targetList.push_back(target);
                }
            }
        }

        void Register()
        {
            OnCheckCast += SpellCheckCastFn(spell_malygos_surge_of_power_phase3_SpellScript::Warning);
            if (m_scriptSpellId == 60936)
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_malygos_surge_of_power_phase3_SpellScript::TargetSelect, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        }

    private:
        std::list<ObjectGuid> targets;
        bool firstCall;
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_malygos_surge_of_power_phase3_SpellScript();
    }
};

void AddSC_boss_malygos()
{
    new boss_malygos();
    new npc_portal_eoe();
    new npc_power_spark();
    new npc_hover_disk();
    new npc_arcane_overload();
    new npc_wyrmrest_skytalon();
    new npc_alexstrasza_eoe();
    new npc_scion_of_eternity();
    new npc_nexus_lord();
    new npc_trigger_invincible();
    new achievement_denyin_the_scion();
    new go_focusing_iris();
    new spell_malygos_vortex_dummy();
    new spell_malygos_vortex_visual();
    new spell_malygos_arcane_overload();
    new spell_malygos_surge_of_power();
    new spell_malygos_arcane_barrage();
    new spell_malygos_surge_of_power_phase3();
}
