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
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "ulduar.h"
#include "Player.h"

// ###### Related Creatures/Object ######
enum NPC
{
    NPC_DARK_RUNE_GUARDIAN                       = 33388,
    NPC_DARK_RUNE_SENTINEL                       = 33846,
    NPC_DARK_RUNE_WATCHER                        = 33453,
    MOLE_MACHINE_TRIGGER                         = 33245,
    NPC_COMMANDER                                = 33210,
    NPC_ENGINEER                                 = 33287,
    NPC_DEFENDER                                 = 33816,
    NPC_RAZORSCALE_CONTROLLER                    = 33233,
};

enum GameObjects
{
    GO_RAZOR_HARPOON_1          = 194542,
    GO_RAZOR_HARPOON_2          = 194541,
    GO_RAZOR_HARPOON_3          = 194543,
    GO_RAZOR_HARPOON_4          = 194519,
    GO_RAZOR_BROKEN_HARPOON     = 194565,
};

// ###### Texts ######
enum Says
{
    // expedition_commander
    SAY_GREET                                    = 0,
    SAY_GROUND_PHASE                             = 1,
    SAY_AGGRO_COMMANDER                          = 2,
    // Engineer
    SAY_AGGRO_ENGINEER_1                         = 0,
    SAY_AGGRO_ENGINEER_2                         = 1,
    SAY_TURRETS                                  = 2,
    // razorscale_controller
    EMOTE_HARPOON                                = 0,
    // razorscale
    EMOTE_BREATH                                 = 1,
    EMOTE_PERMA                                  = 0,
};

enum GossipMenuDatas
{
    GOSSIP_MENU_START_AVAILABLE                 = 13853,
    GOSSIP_MENU_START_UNAVAILABLE               = 13910,
};

#define GOSSIP_ITEM_START                            "Activate Harpoons!"

// ###### Datas ######
enum Actions
{
    // Commander related
    ACTION_START_INTRO                           = 1,   // Starts intro event
    ACTION_GROUND_PHASE                          = 2,   // Notification in ground phase
    ACTION_COMMANDER_RESET                       = 3,   // Used to reset commander
    // HarpoonController related
    ACTION_HARPOON_RESET                         = 4,   // Called on Reset to reset controller
    ACTION_HARPOON_BUILD_START                   = 5,  // Starts building of harpoons
    ACTION_HARPOON_BUILD_END                     = 6,  // Stops building of harpoons, Permaground phase
    ACTION_HARPOON_FLAME                         = 7,  // Enlight the harpoons, place broken and schedule build
    // Harpoon related
    ACTION_HARPOON_REMOVE_BUILDED                = 8,   // Removed builded harpoon
    ACTION_HARPOON_PLACE_BROKEN                  = 9,   // Places broken harpoon
    ACTION_HARPOON_BUILD                         = 10,   // Schedules building event
    ACTION_HARPOON_STOP_BUILD                    = 11,  // Stops building
    // Mole machines
    ACTION_MOLE_SUMMON_SMALL                     = 12,
    ACTION_MOLE_SUMMON_BIG                       = 13,
};

enum Datas
{
    DATA_QUICK_SHAVE                             = 29192921,
};

enum Phases
{
    PHASE_FLIGHT                                 = 1,
    PHASE_GROUND                                 = 2,
    PHASE_PERMAGROUND                            = 3,
};

enum CommanderIntroSteps
{
    PHASE_INTRO_START                            = 1,
    PHASE_INTRO_SUMMON_ENGINEERS                 = 2,
    PHASE_INTRO_SUMMON_DEFENDORS                 = 3,
    PHASE_INTRO_EMOTE                            = 4,
    PHASE_INTRO_START_FIGHT                      = 5,
    MAX_PHASE_INTRO                              = 6,
};

enum Waypoints
{
    WAYPOINT_FLIGHT                              = 1,
    WAYPOINT_LAND                                = 2,
};

// ###### Event Controlling ######
enum Events
{
    // Change phase events
    EVENT_FLIGHT                                 = 1,
    EVENT_LAND                                   = 2,
    EVENT_PERMAGROUND                            = 3,
    // normal events
    EVENT_SUMMON                                 = 4,
    EVENT_FIREBALL                               = 5,
    EVENT_DEVOURING                              = 6,
    EVENT_BREATH                                 = 7,
    EVENT_WINGBUFFET                             = 8,
    EVENT_FLAMEBUFFET                            = 9,
    EVENT_FUSE                                   = 10,
    EVENT_ENRAGE                                 = 11,
    EVENT_CHECK_HOME_DIST                        = 12,

    // Razorscale Controller
    EVENT_BUILD_HARPOON                          = 12,

    // Mole Machine
    EVENT_MOLE_SUMMON_GO                         = 13,
    EVENT_MOLE_SUMMON_NPC_SMALL                  = 14,
    EVENT_MOLE_SUMMON_NPC_BIG                    = 15,
    EVENT_MOLE_DISAPPEAR                         = 16,
};

// ###### Spells ######
enum Spells
{
    // Razorscale
    SPELL_FLAMEBUFFET                            = 64016,
    SPELL_FIREBALL                               = 62796,
    SPELL_WINGBUFFET                             = 62666,
    SPELL_FLAMEBREATH                            = 63317,
    SPELL_FLAMEBREATH_25                         = 64021,   // explicit usage needed for achievement
    SPELL_FUSEARMOR                              = 64771,
    SPELL_ENRAGE                                 = 47008,
    // Devouring Flame Spells
    SPELL_FLAME_GROUND                           = 64709,
    SPELL_DEVOURING_FLAME                        = 63308,
    // HarpoonSpells
    SPELL_HARPOON_TRIGGER                        = 62505,
    SPELL_HARPOON_SHOT_1                         = 63658,
    SPELL_HARPOON_SHOT_2                         = 63657,
    SPELL_HARPOON_SHOT_3                         = 63659,
    SPELL_HARPOON_SHOT_4                         = 63524,
    SPELL_FLAMED                                 = 62696,
    // MoleMachine Spells
    SPELL_SUMMON_MOLE_MACHINE                    = 62899,
    SPELL_SUMMON_IRON_DWARVES                    = 63116,   // Script effect on caster
    SPELL_SUMMON_IRON_DWARVES_2                  = 63114,   // Script effect on caster
    SPELL_SUMMON_IRON_DWARVES_3                  = 63115,   // Script effect on caster
    SPELL_SUMMON_IRON_DWARVE_GUARDIAN            = 62926,
    SPELL_SUMMON_IRON_DWARVE_WATCHER             = 63135,
    SPELL_SUMMON_IRON_VRYCUL                     = 63798,
};

enum DarkRuneSpells
{
    // Dark Rune Watcher
    SPELL_CHAIN_LIGHTNING                        = 64758,
    SPELL_LIGHTNING_BOLT                         = 63809,
    // Dark Rune Guardian
    SPELL_STORMSTRIKE                            = 64757,
    // Dark Rune Sentinel
    SPELL_BATTLE_SHOUT                           = 46763,
    SPELL_BATTLE_SHOUT_25                        = 64062,
    SPELL_HEROIC_STRIKE                          = 45026,
    SPELL_WHIRLWIND                              = 63807,
};

// ##### Additional Data #####
#define GROUND_Z                                 391.517f

const Position PosEngRepair[4] =
{
    { 590.442f, -130.550f, GROUND_Z, 4.789f },
    { 574.850f, -133.687f, GROUND_Z, 4.252f },
    { 606.567f, -143.369f, GROUND_Z, 4.434f },
    { 560.609f, -142.967f, GROUND_Z, 5.074f },
};

const Position PosDefSpawn[4] =
{
    { 600.75f, -104.850f, GROUND_Z, 0 },
    { 596.38f, -110.262f, GROUND_Z, 0 },
    { 566.47f, -103.633f, GROUND_Z, 0 },
    { 570.41f, -108.791f, GROUND_Z, 0 },
};

const Position PosDefCombat[4] =
{
    { 614.975f, -155.138f, GROUND_Z, 4.154f },
    { 609.814f, -204.968f, GROUND_Z, 5.385f },
    { 563.531f, -201.557f, GROUND_Z, 4.108f },
    { 560.231f, -153.677f, GROUND_Z, 5.403f },
};

const Position PosHarpoon[4] =
{
    { 571.901f, -136.554f, GROUND_Z, 0 },
    { 589.450f, -134.888f, GROUND_Z, 0 },
    { 559.119f, -140.505f, GROUND_Z, 0 },
    { 606.229f, -136.721f, GROUND_Z, 0 },
};
const int32 Sectors[3][2][2] =
{
    { // Sector one, North
        {  640,  615 }, // x
        { -190, -225 }  // y
    },
    { // Sector two, Middle
        {  610,  565 }, // x
        { -200, -230 }  // y
    },
    { // Sector three, Sorth
        {  570,  540 }, // x
        { -190, -225 }  // y
    },
};

enum TriggerGUIDs
{
    TRIGGER_HARPOON_1   = 48305,
    TRIGGER_HARPOON_2   = 48307,
    TRIGGER_HARPOON_3   = 48304,
    TRIGGER_HARPOON_4   = 48306,
};

static std::map<uint64, uint64> TriggerToHarpoon;
static std::map<uint64, uint32> TriggerToBuildTime;
void LoadTriggerToHarpoon()
{
    TriggerToHarpoon[TRIGGER_HARPOON_1] = GO_RAZOR_HARPOON_1;
    TriggerToHarpoon[TRIGGER_HARPOON_2] = GO_RAZOR_HARPOON_2;
    TriggerToHarpoon[TRIGGER_HARPOON_3] = GO_RAZOR_HARPOON_3;
    TriggerToHarpoon[TRIGGER_HARPOON_4] = GO_RAZOR_HARPOON_4;

    TriggerToBuildTime[TRIGGER_HARPOON_1] = 40*IN_MILLISECONDS;
    TriggerToBuildTime[TRIGGER_HARPOON_2] = 80*IN_MILLISECONDS;
    TriggerToBuildTime[TRIGGER_HARPOON_3] = 20*IN_MILLISECONDS;
    TriggerToBuildTime[TRIGGER_HARPOON_4] = 60*IN_MILLISECONDS;
};

const Position RazorFlight = { 588.050f, -251.191f, 470.536f, 1.498f };
const Position RazorGround = { 586.966f, -175.534f, GROUND_Z, 4.682f };
const Position PosEngSpawn = { 591.951f, -95.9680f, GROUND_Z, 0.000f };

typedef std::list<Creature*>::const_iterator CLIter;
//! Accessor to Razorscale controllers to maintain Harpoons
void ControllerAccessor(Unit* source, uint32 action)
{
    std::list<Creature*> triggerList;
    source->GetCreatureListWithEntryInGrid(triggerList, NPC_RAZORSCALE_CONTROLLER, 500.0f);

    switch (action)
    {
        case ACTION_HARPOON_RESET:
            for (CLIter itr = triggerList.begin(); itr != triggerList.end(); ++itr)
                (*itr)->AI()->Reset();
            break;
        case ACTION_HARPOON_BUILD_START:
            for (CLIter itr = triggerList.begin(); itr != triggerList.end(); ++itr)
                (*itr)->AI()->DoAction(ACTION_HARPOON_BUILD);
            break;
        case ACTION_HARPOON_FLAME:
            for (CLIter itr = triggerList.begin(); itr != triggerList.end(); ++itr)
                (*itr)->CastSpell(*itr, SPELL_FLAMED, true);
            break;
        case ACTION_HARPOON_BUILD_END:
            for (CLIter itr = triggerList.begin(); itr != triggerList.end(); ++itr)
                (*itr)->AI()->DoAction(ACTION_HARPOON_STOP_BUILD);
            break;
    }
}

//! Spawned for each harpoon, maintains building etc.
class npc_razorscale_controller : public CreatureScript
{
    public:
        npc_razorscale_controller() : CreatureScript("npc_razorscale_controller") { }

        struct npc_razorscale_controllerAI : public ScriptedAI
        {
            npc_razorscale_controllerAI(Creature* creature) : ScriptedAI(creature), events(), instance(me->GetInstanceScript())
            {
                ASSERT(instance);
                me->SetDisplayId(me->GetCreatureTemplate()->Modelid2);

                repairedHarpoonGUID.Clear();
                brokenHarpoonGUID.Clear();
            }

            void Reset()
            {
                building = false;
                harpoonTime = time(0);
                events.Reset();

                DoAction(ACTION_HARPOON_REMOVE_BUILDED);
                DoAction(ACTION_HARPOON_PLACE_BROKEN);
            }

            void SpellHit(Unit* /*caster*/, SpellInfo const* spell)
            {
                switch (spell->Id)
                {
                    case SPELL_FLAMED:
                        DoAction(ACTION_HARPOON_REMOVE_BUILDED);
                        DoAction(ACTION_HARPOON_PLACE_BROKEN);
                        DoAction(ACTION_HARPOON_BUILD);
                        break;
                    case SPELL_HARPOON_SHOT_1:
                    case SPELL_HARPOON_SHOT_2:
                    case SPELL_HARPOON_SHOT_3:
                    case SPELL_HARPOON_SHOT_4:
                        if (harpoonTime < time(0))
                        {
                            DoCast(SPELL_HARPOON_TRIGGER);
                            harpoonTime = time(0) + 1;
                        }
                        break;
                }
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    case ACTION_HARPOON_BUILD:
                        events.ScheduleEvent(EVENT_BUILD_HARPOON, TriggerToBuildTime[me->GetSpawnId()]);
                        building = true;
                        break;
                    case ACTION_HARPOON_PLACE_BROKEN:
                        if (GameObject* harpoon = SummonGameObject(GO_RAZOR_BROKEN_HARPOON))
                            brokenHarpoonGUID = harpoon->GetGUID();
                        break;
                    case ACTION_HARPOON_REMOVE_BUILDED:
                        if (GameObject* Harpoon = ObjectAccessor::GetGameObject(*me, repairedHarpoonGUID))
                            Harpoon->Delete();
                        repairedHarpoonGUID.Clear();
                        break;
                    case ACTION_HARPOON_STOP_BUILD:
                        building = false;
                        break;
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!building)
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_BUILD_HARPOON:
                            Talk(EMOTE_HARPOON);
                            if (GameObject* Harpoon = SummonGameObject(TriggerToHarpoon[me->GetSpawnId()]))
                            {
                                repairedHarpoonGUID = Harpoon->GetGUID();
                                if (GameObject* BrokenHarpoon = ObjectAccessor::GetGameObject(*me, brokenHarpoonGUID))
                                {
                                    BrokenHarpoon->Delete();
                                    brokenHarpoonGUID.Clear();
                                }
                            }
                            building = false;
                            break;
                    }
                }
            }

            protected:
                inline GameObject* SummonGameObject(uint32 entry)
                {
                    return me->SummonGameObject(entry, me->GetPositionX(), me->GetPositionY(), GROUND_Z, me->GetAngle(&RazorFlight), 0, 0, 0, 0, uint32(me->GetRespawnTime()));
                }

            private:
                EventMap events;
                InstanceScript* instance;
                ObjectGuid brokenHarpoonGUID;
                ObjectGuid repairedHarpoonGUID;
                time_t harpoonTime;
                bool building;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_razorscale_controllerAI(creature);
        }
};

class boss_razorscale : public CreatureScript
{
    public:
        boss_razorscale() : CreatureScript("boss_razorscale") { }

        struct boss_razorscaleAI : public BossAI
        {
            boss_razorscaleAI(Creature* creature) : BossAI(creature, BOSS_RAZORSCALE)
            {
                ASSERT(instance);
                // Do not let Razorscale be affected by Battle Shout buff
                me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_BATTLE_SHOUT   , true);
                me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_BATTLE_SHOUT_25, true);
            }

            void Reset()
            {
                BossAI::Reset();

                permaground = false;

                me->SetReactState(REACT_PASSIVE);
                me->SetSpeedRate(MOVE_FLIGHT, 3.0f);
                me->SetCanFly(false);
                me->SetDisableGravity(true);

                if (Creature* commander = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_EXP_COMMANDER))))
                    commander->AI()->DoAction(ACTION_COMMANDER_RESET);

                ControllerAccessor(me, ACTION_HARPOON_RESET);
            }

            void EnterCombat(Unit* who)
            {
                BossAI::EnterCombat(who);

                ControllerAccessor(me, ACTION_HARPOON_BUILD_START);

                me->SetCanFly(true);
                me->SetDisableGravity(false);

                AchievementFlyCount = 0;
                HarpoonCounter      = 0;

                events.ScheduleEvent(EVENT_FLIGHT, 0);
                events.ScheduleEvent(EVENT_ENRAGE, 15 * MINUTE * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_CHECK_HOME_DIST, IN_MILLISECONDS);
            }

            void JustDied(Unit* killer)
            {
                BossAI::JustDied(killer);
                if (Creature* controller = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_RAZORSCALE_CONTROL)) : ObjectGuid::Empty))
                    controller->AI()->Reset();
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (HealthBelowPct(50) && !events.IsInPhase(PHASE_PERMAGROUND))
                    events.ScheduleEvent(EVENT_PERMAGROUND, 0);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        // ### Phase switch events ###
                        case EVENT_FLIGHT:  //! Switch to flight phase
                            events.SetPhase(PHASE_FLIGHT);

                            me->SetCanFly(true);
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            me->SetReactState(REACT_PASSIVE);
                            me->SetControlled(false, UNIT_STATE_STUNNED);

                            me->RemoveAllAuras();
                            me->AttackStop();

                            me->GetMotionMaster()->MoveTakeoff(WAYPOINT_FLIGHT, RazorFlight);

                            ++AchievementFlyCount;

                            events.RescheduleEvent(EVENT_FIREBALL,   7*IN_MILLISECONDS, 0, PHASE_FLIGHT);
                            events.RescheduleEvent(EVENT_DEVOURING, 10*IN_MILLISECONDS, 0, PHASE_FLIGHT);
                            events.RescheduleEvent(EVENT_SUMMON,     5*IN_MILLISECONDS, 0, PHASE_FLIGHT);
                            break;
                        case EVENT_LAND:    //! Switch to land phase
                            events.SetPhase(PHASE_GROUND);

                            me->SetCanFly(false);
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            me->SetReactState(REACT_PASSIVE);
                            me->SetControlled(true, UNIT_STATE_STUNNED);

                            if (Creature* commander = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_EXP_COMMANDER))))
                                commander->AI()->DoAction(ACTION_GROUND_PHASE);

                            events.CancelEvent(EVENT_FIREBALL);
                            events.CancelEvent(EVENT_DEVOURING);

                            events.ScheduleEvent(EVENT_BREATH, 40*IN_MILLISECONDS, 0, PHASE_GROUND);
                            events.ScheduleEvent(EVENT_WINGBUFFET, 43*IN_MILLISECONDS, 0, PHASE_GROUND);
                            events.ScheduleEvent(EVENT_FLIGHT, 45*IN_MILLISECONDS);
                            break;
                        case EVENT_PERMAGROUND:
                            if (permaground)
                                return;
                            permaground = true;
                            events.SetPhase(PHASE_PERMAGROUND);
                            Talk(EMOTE_PERMA);

                            me->SetCanFly(false);
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            me->SetReactState(REACT_AGGRESSIVE);
                            me->SetControlled(false, UNIT_STATE_STUNNED);
                            me->RemoveAurasDueToSpell(SPELL_HARPOON_TRIGGER);

                            events.CancelEvent(EVENT_BREATH);
                            events.CancelEvent(EVENT_WINGBUFFET);
                            events.CancelEvent(EVENT_FLIGHT);

                            events.ScheduleEvent(EVENT_WINGBUFFET,    0                 , 0, PHASE_PERMAGROUND);
                            events.ScheduleEvent(EVENT_FLAMEBUFFET,     15*IN_MILLISECONDS, 0, PHASE_PERMAGROUND);
                            events.ScheduleEvent(EVENT_DEVOURING, 15*IN_MILLISECONDS, 0, PHASE_PERMAGROUND);
                            events.ScheduleEvent(EVENT_BREATH,    20*IN_MILLISECONDS, 0, PHASE_PERMAGROUND);
                            events.ScheduleEvent(EVENT_FIREBALL,   3*IN_MILLISECONDS, 0, PHASE_PERMAGROUND);
                            events.ScheduleEvent(EVENT_DEVOURING,  6*IN_MILLISECONDS, 0, PHASE_PERMAGROUND);
                            events.ScheduleEvent(EVENT_FUSE,       5*IN_MILLISECONDS, 0, PHASE_PERMAGROUND);
                            break;
                        // ### Ground, Permaground spezific ###
                        case EVENT_BREATH:
                            if (events.IsInPhase(PHASE_GROUND))
                            {
                                me->SetControlled(false, UNIT_STATE_STUNNED);
                                // me->SetReactState(REACT_AGGRESSIVE);
                                me->RemoveAurasDueToSpell(SPELL_HARPOON_TRIGGER);
                            }

                            Talk(EMOTE_BREATH);
                            DoCast(SPELL_FLAMEBREATH);

                            if (events.IsInPhase(PHASE_PERMAGROUND))
                                events.ScheduleEvent(EVENT_BREATH, 20*IN_MILLISECONDS, 0, PHASE_PERMAGROUND);
                            break;
                        // ### Ground spezific ###
                        case EVENT_WINGBUFFET:
                            DoCastAOE(SPELL_WINGBUFFET);

                            if (events.IsInPhase(PHASE_GROUND))
                                ControllerAccessor(me, ACTION_HARPOON_FLAME);
                            else if (events.IsInPhase(PHASE_PERMAGROUND))
                                ControllerAccessor(me, ACTION_HARPOON_BUILD_END);
                            break;
                        // ### Permaground spezific ###
                        case EVENT_FLAMEBUFFET:
                            DoCastAOE(SPELL_FLAMEBUFFET);
                            events.ScheduleEvent(EVENT_FLAMEBUFFET, 10*IN_MILLISECONDS, 0, PHASE_PERMAGROUND);
                            break;
                        case EVENT_FUSE:
                            DoCastVictim(SPELL_FUSEARMOR);
                            events.ScheduleEvent(EVENT_FUSE, 10*IN_MILLISECONDS, 0, PHASE_PERMAGROUND);
                            break;
                        // ### Flight spezific ###
                        case EVENT_SUMMON:
                            SummonMoleMachines();
                            events.ScheduleEvent(EVENT_SUMMON, 45*IN_MILLISECONDS, 0, PHASE_FLIGHT);
                            break;
                        // ### Flight, Permaground spezific ###
                        case EVENT_FIREBALL:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true))
                                DoCast(target, SPELL_FIREBALL);
                            events.ScheduleEvent(EVENT_FIREBALL, 3*IN_MILLISECONDS);
                            break;
                        case EVENT_DEVOURING:
                            if (events.IsInPhase(PHASE_FLIGHT))
                            {
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true))
                                    DoCast(target, SPELL_DEVOURING_FLAME);
                            }
                            else
                                DoCastVictim(SPELL_DEVOURING_FLAME);
                            events.ScheduleEvent(EVENT_DEVOURING, 10*IN_MILLISECONDS);
                            break;
                        // ### Generic ###
                        case EVENT_ENRAGE:
                            DoCast(SPELL_ENRAGE);
                            break;
                        case EVENT_CHECK_HOME_DIST:
                            if (me->GetDistance(me->GetHomePosition()) > 200.0f)
                                EnterEvadeMode();
                            events.ScheduleEvent(EVENT_CHECK_HOME_DIST, IN_MILLISECONDS);
                            break;
                    }
                }

                if (events.IsInPhase(PHASE_PERMAGROUND))
                    DoMeleeAttackIfReady();

            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage)
            {
                if (events.IsInPhase(PHASE_FLIGHT))
                    damage = 0;
            }

            void SpellHit(Unit* /*caster*/, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_HARPOON_TRIGGER)
                    if (events.IsInPhase(PHASE_FLIGHT))
                        if (++HarpoonCounter >= RAID_MODE(2, 4))
                        {
                            HarpoonCounter = 0;
                            me->GetMotionMaster()->MoveLand(WAYPOINT_LAND, RazorGround);
                        }
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (type == EFFECT_MOTION_TYPE && id == WAYPOINT_LAND)
                    events.ScheduleEvent(EVENT_LAND, 0);
            }

            uint32 GetData(uint32 type) const
            {
                if (type == DATA_QUICK_SHAVE)
                    return AchievementFlyCount <= 2 ? 1 : 0;

                return 0;
            }

            //! Function of this function is the handling of the spawns of the mole machines
            //! Prevents spawning to many and to hard waves
            //! Normally 2/3 Machines are spawned
            //! 30%/40% chance to spawn an an additional one (big spawn)
            //! Vryculs only spawn in the middle and only once
            //! Big spawn always has a vrycul
            //! There is only one side spawned doubled
            void SummonMoleMachines()
            {
                uint8 amount = 0;

                bool big = urand(0, 100) < RAID_MODE<uint32>(30, 40);
                if (big)
                    amount = RAID_MODE(3, 4);
                else
                    amount = RAID_MODE(2, 3);

                uint8 lastSector = 255;
                bool vrycul = false;
                for (uint8 n = 0; n < amount; ++n)
                {
                    uint8 sector = lastSector;
                    while (sector == lastSector || (sector == 1 && vrycul))
                        sector = (big && !vrycul && urand(0, 100) < 80) ? 1 : urand(0, 2);
                    lastSector = sector;

                    float x = float(irand(Sectors[sector][0][1], Sectors[sector][0][0]));
                    float y = float(irand(Sectors[sector][1][1], Sectors[sector][1][0]));
                    float z = GROUND_Z;
                    if (Creature* trigger = me->SummonCreature(MOLE_MACHINE_TRIGGER, x, y, z, 0))
                    {
                        if (big && sector == 1)
                        {
                            trigger->GetAI()->DoAction(ACTION_MOLE_SUMMON_BIG);
                            vrycul = true;
                        }
                        else
                            trigger->GetAI()->DoAction(ACTION_MOLE_SUMMON_SMALL);
                    }

                }
            }

            private:
                uint8 AchievementFlyCount;
                uint8 HarpoonCounter;
                bool permaground;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_razorscaleAI(creature);
        }
};

class npc_expedition_commander : public CreatureScript
{
    public:
        npc_expedition_commander() : CreatureScript("npc_expedition_commander") { }

        struct npc_expedition_commanderAI : public ScriptedAI
        {
            npc_expedition_commanderAI(Creature* creature) : ScriptedAI(creature), summons(me), instance(me->GetInstanceScript())
            {
                ASSERT(instance);
            }

            void Reset()
            {
                summons.DespawnAll();
                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

                AttackStartTimer = 0;
                Phase = 0;
                Greet = false;
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (!Greet && me->IsWithinDistInMap(who, 10.0f) && who->GetTypeId() == TYPEID_PLAYER)
                {
                    Talk(SAY_GREET);
                    Greet = true;
                }
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    case ACTION_START_INTRO:
                        Phase = PHASE_INTRO_START;
                        break;
                    case ACTION_GROUND_PHASE:
                        Talk(SAY_GROUND_PHASE);
                        break;
                    case ACTION_COMMANDER_RESET:
                        EnterEvadeMode();
                        break;
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (Phase == 0 || Phase >= MAX_PHASE_INTRO)
                    return;

                if (AttackStartTimer <= diff)
                {
                    switch (Phase)
                    {
                        case PHASE_INTRO_START: // Start boss fight
                            instance->SetBossState(BOSS_RAZORSCALE, IN_PROGRESS);
                            summons.DespawnAll();
                            AttackStartTimer = 1000;
                            break;
                        case PHASE_INTRO_SUMMON_ENGINEERS: // Summon Engineers
                            for (uint8 n = 0; n < RAID_MODE(2, 4); n++)
                                if ((Engineer[n] = me->SummonCreature(NPC_ENGINEER, PosEngSpawn, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000)))
                                {
                                    Engineer[n]->SetHomePosition(PosEngRepair[n]);
                                    Engineer[n]->GetMotionMaster()->MoveTargetedHome();
                                }
                            if (Engineer[0])
                                Engineer[0]->AI()->Talk(SAY_AGGRO_ENGINEER_2);
                            AttackStartTimer = 14000;
                            break;
                        case PHASE_INTRO_SUMMON_DEFENDORS: // Summon Defendors
                            for (uint8 n = 0; n < 4; n++)
                                if ((Defender[n] = me->SummonCreature(NPC_DEFENDER, PosDefSpawn[n], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000)))
                                {
                                    Defender[n]->SetHomePosition(PosDefCombat[n]);
                                    Defender[n]->GetMotionMaster()->MoveTargetedHome();
                                }
                            break;
                        case PHASE_INTRO_EMOTE: // NPCs Emotes
                            for (uint8 n = 0; n < RAID_MODE(2, 4); n++)
                                Engineer[n]->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_USE_STANDING);
                            for (uint8 n = 0; n < 4; ++n)
                                Defender[n]->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_READY2H);
                            Talk(SAY_AGGRO_COMMANDER);
                            AttackStartTimer = 16000;
                            break;
                        case PHASE_INTRO_START_FIGHT:
                            if (Creature* Razorscale = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(BOSS_RAZORSCALE)) : ObjectGuid::Empty))
                            {
                                Razorscale->SetInCombatWithZone();
                                me->SetInCombatWith(Razorscale);
                            }
                            if (Engineer[0])
                                Engineer[0]->AI()->Talk(SAY_AGGRO_ENGINEER_1);
                            break;
                    }
                    ++Phase;
                }
                else
                    AttackStartTimer -= diff;
            }

            void JustSummoned(Creature* summoned)
            {
                summons.Summon(summoned);
            }

            private:
                InstanceScript* instance;
                SummonList summons;

                bool Greet;
                uint32 AttackStartTimer;
                uint8  Phase;
                Creature* Engineer[4];
                Creature* Defender[4];
        };

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();
            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF:
                    creature->AI()->DoAction(ACTION_START_INTRO);
                    break;
            }
            player->CLOSE_GOSSIP_MENU();
            return true;
        }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            InstanceScript* instance = creature->GetInstanceScript();
            if (instance && instance->GetBossState(BOSS_RAZORSCALE) == NOT_STARTED)
            {
                player->PrepareGossipMenu(creature);

                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_START, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                player->SEND_GOSSIP_MENU(GOSSIP_MENU_START_AVAILABLE, creature->GetGUID());
            }
            else
                player->SEND_GOSSIP_MENU(GOSSIP_MENU_START_UNAVAILABLE, creature->GetGUID());

            return true;
        }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_expedition_commanderAI(creature);
        }
};

class npc_mole_machine_trigger : public CreatureScript
{
    public:
        npc_mole_machine_trigger() : CreatureScript("npc_mole_machine_trigger") { }

        struct npc_mole_machine_triggerAI : public ScriptedAI
        {
            npc_mole_machine_triggerAI(Creature* creature) : ScriptedAI(creature), events(), instance(me->GetInstanceScript())
            {
                ASSERT(instance);
                SetCombatMovement(false);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_PACIFIED);
                me->SetVisible(false);
            }

            void DoAction(int32 param)
            {
                if (param == ACTION_MOLE_SUMMON_SMALL)
                    events.ScheduleEvent(EVENT_MOLE_SUMMON_NPC_SMALL, 6*IN_MILLISECONDS);
                else if (param == ACTION_MOLE_SUMMON_BIG)
                    events.ScheduleEvent(EVENT_MOLE_SUMMON_NPC_BIG, 6*IN_MILLISECONDS);

                events.ScheduleEvent(EVENT_MOLE_SUMMON_GO,  2*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_MOLE_DISAPPEAR, 10*IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_MOLE_SUMMON_GO:
                            DoCast(SPELL_SUMMON_MOLE_MACHINE);
                            break;
                        case EVENT_MOLE_DISAPPEAR:
                            if (GameObject* molemachine = me->FindNearestGameObject(GO_MOLE_MACHINE, 1.0f))
                                molemachine->Delete();
                            break;
                        case EVENT_MOLE_SUMMON_NPC_SMALL:
                            DoCast(SPELL_SUMMON_IRON_DWARVE_GUARDIAN);
                            DoCast(SPELL_SUMMON_IRON_DWARVE_WATCHER);
                            if (Is25ManRaid())
                            {
                                if (urand(0, 1))
                                    DoCast(SPELL_SUMMON_IRON_DWARVE_GUARDIAN);
                                else
                                    DoCast(SPELL_SUMMON_IRON_DWARVE_WATCHER);
                            }
                            break;
                        case EVENT_MOLE_SUMMON_NPC_BIG:
                            DoCast(SPELL_SUMMON_IRON_VRYCUL);
                            break;
                    }
                }
            }

            void JustSummoned(Creature* summoned)
            {
                if (Creature* razorscale = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_RAZORSCALE))))
                    razorscale->AI()->JustSummoned(summoned);
            }

            private:
                EventMap events;
                InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_mole_machine_triggerAI(creature);
        }
};

class npc_devouring_flame : public CreatureScript
{
    public:
        npc_devouring_flame() : CreatureScript("npc_devouring_flame") { }

        struct npc_devouring_flameAI : public ScriptedAI
        {
            npc_devouring_flameAI(Creature* creature) : ScriptedAI(creature)
            {
                SetCombatMovement(false);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_PACIFIED);
                me->SetDisplayId(me->GetCreatureTemplate()->Modelid2);
            }

            void Reset()
            {
                DoCast(SPELL_FLAME_GROUND);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_devouring_flameAI(creature);
        }
};

class npc_darkrune_watcher : public CreatureScript
{
    public:
        npc_darkrune_watcher() : CreatureScript("npc_darkrune_watcher") { }

        struct npc_darkrune_watcherAI : public ScriptedAI
        {
            npc_darkrune_watcherAI(Creature* creature) : ScriptedAI(creature){}

            uint32 ChainTimer;
            uint32 LightTimer;

            void Reset()
            {
                ChainTimer = urand(10000, 15000);
                LightTimer = urand(1000, 3000);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (ChainTimer <= diff)
                {
                    DoCastVictim(SPELL_CHAIN_LIGHTNING);
                    ChainTimer = urand(10000, 15000);
                }
                else
                    ChainTimer -= diff;

                if (LightTimer <= diff)
                {
                    DoCastVictim(SPELL_LIGHTNING_BOLT);
                    LightTimer = urand(5000, 7000);
                }
                else
                    LightTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_darkrune_watcherAI(creature);
        }
};

class npc_darkrune_guardian : public CreatureScript
{
    public:
        npc_darkrune_guardian() : CreatureScript("npc_darkrune_guardian") { }

        struct npc_darkrune_guardianAI : public ScriptedAI
        {
            npc_darkrune_guardianAI(Creature* creature) : ScriptedAI(creature){}

            uint32 StormTimer;

            void Reset()
            {
                StormTimer = urand(3000, 6000);
                killedByBreath = false;
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (StormTimer <= diff)
                {
                    DoCastVictim(SPELL_STORMSTRIKE);
                    StormTimer = urand(4000, 8000);
                }
                else
                    StormTimer -= diff;

                DoMeleeAttackIfReady();
            }

        private:
            bool killedByBreath;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_darkrune_guardianAI(creature);
        }
};

class npc_darkrune_sentinel : public CreatureScript
{
    public:
        npc_darkrune_sentinel() : CreatureScript("npc_darkrune_sentinel") { }

        struct npc_darkrune_sentinelAI : public ScriptedAI
        {
            npc_darkrune_sentinelAI(Creature* creature) : ScriptedAI(creature){}

            uint32 HeroicTimer;
            uint32 WhirlTimer;
            uint32 ShoutTimer;

            void Reset()
            {
                HeroicTimer = urand(4000, 8000);
                WhirlTimer = urand(20000, 25000);
                ShoutTimer = urand(15000, 30000);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (HeroicTimer <= diff)
                {
                    DoCastVictim(SPELL_HEROIC_STRIKE);
                    HeroicTimer = urand(4000, 6000);
                }
                else
                    HeroicTimer -= diff;

                if (WhirlTimer <= diff)
                {
                    DoCastVictim(SPELL_WHIRLWIND);
                    WhirlTimer = urand(20000, 25000);
                }
                else
                    WhirlTimer -= diff;

                if (ShoutTimer <= diff)
                {
                    DoCastSelf(SPELL_BATTLE_SHOUT);
                    ShoutTimer = urand(30000, 40000);
                }
                else
                    ShoutTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_darkrune_sentinelAI(creature);
        }
};

class go_razorscale_harpoon : public GameObjectScript
{
    public:
        go_razorscale_harpoon() : GameObjectScript("go_razorscale_harpoon") {}

        bool OnGossipHello(Player* /*player*/, GameObject* go)
        {
            go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
            return false;
        }
};

//! Script is needed, because flames are summoned as guardian, which follows his summoner
//! Prevents movement of flames
class spell_razorscale_devouring_flame : public SpellScriptLoader
{
    public:
        spell_razorscale_devouring_flame() : SpellScriptLoader("spell_razorscale_devouring_flame") { }

        class spell_razorscale_devouring_flame_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_razorscale_devouring_flame_SpellScript);

            void HandleSummon(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);

                Unit* caster = GetCaster();
                uint32 entry = uint32(GetSpellInfo()->Effects[effIndex].MiscValue);
                WorldLocation* summonLocation = GetHitDest();
                if (!caster || !summonLocation)
                    return;

                caster->SummonCreature(entry, summonLocation->GetPositionX(), summonLocation->GetPositionY(), GROUND_Z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 20000);
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_razorscale_devouring_flame_SpellScript::HandleSummon, EFFECT_0, SPELL_EFFECT_SUMMON);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_razorscale_devouring_flame_SpellScript();
        }
};

class achievement_iron_dwarf_medium_rare : public AchievementCriteriaScript
{
    public:
        achievement_iron_dwarf_medium_rare() : AchievementCriteriaScript("achievement_iron_dwarf_medium_rare") {}

        bool OnCheck(Player* /*player*/, Unit* target)
        {
            return target && target->HasAura(target->GetMap()->GetSpawnMode() == RAID_DIFFICULTY_25MAN_NORMAL ? SPELL_FLAMEBREATH_25 : SPELL_FLAMEBREATH);
        }
};

class achievement_quick_shave : public AchievementCriteriaScript
{
    public:
        achievement_quick_shave() : AchievementCriteriaScript("achievement_quick_shave") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
           if (target)
                if (Creature* razorscale = target->ToCreature())
                    if (razorscale->AI()->GetData(DATA_QUICK_SHAVE))
                        return true;

            return false;
        }
};

void AddSC_boss_razorscale()
{
    LoadTriggerToHarpoon();
    new npc_razorscale_controller();
    new go_razorscale_harpoon();
    new boss_razorscale();
    new npc_expedition_commander();
    new npc_mole_machine_trigger();
    new npc_devouring_flame();
    new npc_darkrune_watcher();
    new npc_darkrune_guardian();
    new npc_darkrune_sentinel();
    new spell_razorscale_devouring_flame();
    new achievement_iron_dwarf_medium_rare();
    new achievement_quick_shave();
}
