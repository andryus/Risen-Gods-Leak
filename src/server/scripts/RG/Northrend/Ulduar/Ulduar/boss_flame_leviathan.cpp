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

/*
 * Comment: there is missing code on triggers,
 *          brann bronzebeard needs correct gossip info.
 *          requires more work involving area triggers.
 *          if reached brann speaks through his radio..
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "CombatAI.h"
#include "PassiveAI.h"
#include "ScriptedEscortAI.h"
#include "ObjectMgr.h"
#include "SpellScript.h"
#include "ulduar.h"
#include "Vehicle.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "DBCStores.h"

// ###### Related Creatures/Object ######
enum Creatures
{
    NPC_SEAT                                    = 33114,
    NPC_MECHANOLIFT                             = 33214,
    NPC_LIQUID                                  = 33189,
    NPC_CONTAINER                               = 33218,
    NPC_THORIM_BEACON                           = 33365,
    NPC_MIMIRON_BEACON                          = 33370,
    NPC_HODIR_BEACON                            = 33212,
    NPC_FREYA_BEACON                            = 33367,
    NPC_THORIM_TARGET_BEACON                    = 33364,
    NPC_MIMIRON_TARGET_BEACON                   = 33369,
    NPC_HODIR_TARGET_BEACON                     = 33108,
    NPC_FREYA_TARGET_BEACON                     = 33366,
    NPC_WRITHING_LASHER                         = 33387,
    NPC_WARD_OF_LIFE                            = 34275,
    NPC_LOREKEEPER                              = 33686, // Hard mode starter
    NPC_BRANZ_BRONZBEARD                        = 33579, // Normal mode starter
    NPC_DELORAH                                 = 33701,
    NPC_ULDUAR_GAUNTLET_GENERATOR               = 33571, // Trigger tied to towers
    NPC_ULDUAR_GAUNTLET_GENERATOR_2             = 34159, // Trigger tied to special towers
    NPC_TOWER_CANNON                            = 33264,
    NPC_POOL_OF_TAR                             = 33090,
    NPC_AMBUSHER                                = 32188,
    NPC_FROSTBROOD                              = 31721
};

enum Towers
{
    GO_TOWER_OF_STORMS  = 194377,
    GO_TOWER_OF_FLAMES  = 194371,
    GO_TOWER_OF_FROST   = 194370,
    GO_TOWER_OF_LIFE    = 194375,
};

enum EmoteLevi
{
    EMOTE_PURSUE        = 12,
    EMOTE_OVERLOAD      = 13,
    EMOTE_REPAIR        = 14,
};

enum Yells
{
    SAY_AGGRO           = 0,
    SAY_SLAY            = 1, 
    SAY_DEATH           = 2,
    SAY_TARGET          = 3,
    SAY_HARDMODE        = 4, 
    SAY_TOWER_NONE      = 5, 
    SAY_TOWER_FROST     = 6,
    SAY_TOWER_FLAME     = 7,
    SAY_TOWER_NATURE    = 8,
    SAY_TOWER_STORM     = 9,
    SAY_PLAYER_RIDING   = 10,
    SAY_OVERLOAD        = 11,
};

// ###### Datas ######
enum Achievements
{
    ACHIEVEMENT_TAKE_OUT_THOSE_TURRETS_10 = 2909,
    ACHIEVEMENT_TAKE_OUT_THOSE_TURRETS_25 = 2910,
};

enum Actions
{
    ACTION_ACTIVATE_HARD_MODE                   = 0,
    ACTION_DESPAWN_ADDS                         = 1,
};

enum Data
{
    DATA_SHUTOUT                                = 29112912, // 2911, 2912 are achievement IDs
    DATA_UNBROKEN                               = 29052906, // 2905, 2906 are achievement IDs
    DATA_ORBIT_ACHIEVEMENTS                     = 1,
};

enum Seats
{
    SEAT_PLAYER                                 = 0,
    SEAT_TURRET                                 = 1,
    SEAT_DEVICE                                 = 2,
    SEAT_CANNON                                 = 7,
};

enum TowersState
{
    TOWER_INTACT                                = 1,
    TOWER_DESTROYED                             = 0,
};

// ###### Event Controlling ######
enum Events
{
    EVENT_PURSUE              = 1,
    EVENT_MISSILE             = 2,
    EVENT_VENT                = 3,
    EVENT_SPEED               = 4,
    EVENT_SHUTDOWN            = 5,
    EVENT_REPAIR              = 6,
    EVENT_THORIM_S_HAMMER     = 7,    // Tower of Storms
    EVENT_MIMIRON_S_INFERNO   = 8,    // Tower of Flames
    EVENT_HODIR_S_FURY        = 9,    // Tower of Frost
    EVENT_FREYA_S_WARD        = 10,   // Tower of Nature
    EVENT_FREYA_S_WARD_SUMMON = 11,
};

// ###### Spells ######
enum Spells
{
    SPELL_PURSUED                               = 62374,
    SPELL_GATHERING_SPEED                       = 62375,
    SPELL_BATTERING_RAM                         = 62376,
    SPELL_FLAME_VENTS                           = 62396,
    SPELL_MISSILE_BARRAGE                       = 62400,
    SPELL_SYSTEMS_SHUTDOWN                      = 62475,
    SPELL_OVERLOAD_CIRCUIT                      = 62399,
    SPELL_START_THE_ENGINE                      = 62472,
    SPELL_SEARING_FLAME                         = 62402,
    SPELL_BLAZE                                 = 62292,
    SPELL_TAR_PASSIVE                           = 62288,
    SPELL_SMOKE_TRAIL                           = 63575,
    SPELL_ELECTROSHOCK                          = 62522,
    SPELL_NAPALM                                = 63666,
    //TOWER Additional SPELLS
    SPELL_THORIM_S_HAMMER                       = 62911, // Tower of Storms
    SPELL_MIMIRON_S_INFERNO                     = 62909, // Tower of Flames
    SPELL_MIMIRONS_INFERNO                      = 62910, // Tower of Flames
    SPELL_HODIR_S_FURY                          = 62533, // Tower of Frost
    SPELL_FREYA_S_WARD                          = 62906, // Tower of Nature
    SPELL_FREYA_SUMMONS                         = 62947, // Tower of Nature
    //TOWER ap & health spells
    SPELL_BUFF_TOWER_OF_STORMS                  = 65076,
    SPELL_BUFF_TOWER_OF_FLAMES                  = 65075,
    SPELL_BUFF_TOWER_OF_FR0ST                   = 65077,
    SPELL_BUFF_TOWER_OF_LIFE                    = 64482,
    //Additional Spells
    SPELL_LASH                                  = 65062,
    SPELL_FREYA_S_WARD_EFFECT_1                 = 62947,
    SPELL_FREYA_S_WARD_EFFECT_2                 = 62907,
    SPELL_AUTO_REPAIR                           = 62705,
    AURA_DUMMY_BLUE                             = 63294,
    AURA_DUMMY_GREEN                            = 63295,
    AURA_DUMMY_YELLOW                           = 63292,
    SPELL_DUSTY_EXPLOSION                       = 63360,
    SPELL_DUST_CLOUD_IMPACT                     = 54740,
    AURA_STEALTH_DETECTION                      = 18950,
    SPELL_RIDE_VEHICLE                          = 46598,
    SPELL_ANTI_AIR_ROCKET_DMG                   = 62363,
    SPELL_GROUND_SLAM                           = 62625,
    SPELL_LEVIATHAN_FLAMES                      = 62297, // use to free players from hodirs freezing
    SPELL_PARACHUTE                             = 61243, // used by falling pyrit containers
    SPELL_ROPE_BEAM                             = 63605, // used by mechanolifts on passanger
};

enum DisplayIDs
{
    DISPLAYID_PYRIT                             = 28783,
    DISPLAYID_CONTAINER                         = 28782, // used by Pyrit while falling
};

// ##### Additional Data #####
Position const Center                = {354.8771f, -12.90240f, 409.803650f, 0.0f};
Position const MimironBeaconPosition = {383.182f, -29.9824f, 409.805f, 0.0f};
Position const FreyaBeaconPosition[] =
{
    {377.02f, -119.10f, 409.81f, 0.0f},
    {377.02f,   54.78f, 409.81f, 0.0f},
    {185.62f,   54.78f, 409.81f, 0.0f},
    {185.62f, -119.10f, 409.81f, 0.0f},
};

enum Health
{
    HEALTH_10_0T                                =  23009250,
    HEALTH_10_1T                                =  32212950,
    HEALTH_10_2T                                =  45098130,
    HEALTH_10_3T                                =  63137382,
    HEALTH_10_4T                                =  88392335,
    HEALTH_25_0T                                =  70000693,
    HEALTH_25_1T                                =  98000970,
    HEALTH_25_2T                                = 137201358,
    HEALTH_25_3T                                = 192081902,
    HEALTH_25_4T                                = 268914662,
};

class boss_flame_leviathan : public CreatureScript
{
    public:
        boss_flame_leviathan() : CreatureScript("boss_flame_leviathan") { }

        struct boss_flame_leviathanAI : public BossAI
        {
            boss_flame_leviathanAI(Creature* creature) : BossAI(creature, BOSS_LEVIATHAN), vehicle(creature->GetVehicleKit()) {}

            // Called once on creature loading
            void InitializeAI()
            {
                if (me->isDead())
                    return;

                Reset();

                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);

                // At first Leviathan is passive behind main door
                me->SetReactState(REACT_PASSIVE);

                // If both colossus are dead drive to center and let the door explode
                if (instance->GetData(DATA_COLOSSUS) >= 2)
                    DoAction(ACTION_DRIVE_TO_CENTER);
            }

            void Reset()
            {
                BossAI::Reset();

                ASSERT(vehicle);
                // Reset Combat stati
                ShutdownCounter = 0;
                ShutdownActive = false;
                Pursued = false;

                AchievementShutout = true;
                AchievementUnbroken = true;

                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED, true);
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                _EnterEvadeMode();

                if (me->IsAlive())
                    instance->SetBossState(0, FAIL);

                me->ClearUnitState(UNIT_STATE_STUNNED | UNIT_STATE_ROOT); // Remove effect of shutdown
                me->GetMotionMaster()->MoveTargetedHome();

                me->SetReactState(REACT_AGGRESSIVE);

                Reset();
            }

            void EnterCombat(Unit* who)
            {
                BossAI::EnterCombat(who);

                me->SetReactState(REACT_AGGRESSIVE);

                CheckTowersAndMode();
                DespawnEmptyVehicles();

                events.ScheduleEvent(EVENT_PURSUE, 30*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_MISSILE, urand(1500, 4*IN_MILLISECONDS));
                events.ScheduleEvent(EVENT_VENT, 20*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SPEED, 9500);

                vehicle->InstallAllAccessories(false);

                Talk(hardmode ? SAY_HARDMODE : SAY_AGGRO);
            }

            void DamageTaken(Unit* doneBy, uint32& damage)
            {
                if (doneBy->GetTypeId() == TYPEID_UNIT && doneBy->GetEntry() == NPC_POOL_OF_TAR && me->GetHealthPct() <= 5.0f)
                    damage = 0;
            }

            void DespawnEmptyVehicles()
            {
                std::list<Creature*> vehicleList;
                me->GetCreatureListWithEntryInGrid(vehicleList, NPC_VEHICLE_CHOPPER, 1000.0f);
                me->GetCreatureListWithEntryInGrid(vehicleList, NPC_VEHICLE_DEMOLISHER, 1000.0f);
                me->GetCreatureListWithEntryInGrid(vehicleList, NPC_VEHICLE_SIEGE, 1000.0f);

                for (std::list<Creature*>::const_iterator itr = vehicleList.begin(); itr != vehicleList.end(); ++itr)
                    if ((*itr)->GetPositionX() < 130.0f) // out of combat area
                        (*itr)->DespawnOrUnsummon();
            }

            // Checks for hardmode mode.
            void CheckTowersAndMode()
            {
                hardmode = false;
                towersIntact = 0;

                // Has hardmode been activated?
                uint32 flag = instance->GetData(DATA_LEVIATHAN_HARDMODE);

                hardmode = flag != 0;

                if (flag & HODIR_TOWER)
                {
                    me->AddAura(SPELL_BUFF_TOWER_OF_FR0ST, me);
                    events.ScheduleEvent(EVENT_HODIR_S_FURY, 105*IN_MILLISECONDS);
                    ++towersIntact;
                }

                if (flag & THORIM_TOWER)
                {
                    me->AddAura(SPELL_BUFF_TOWER_OF_STORMS, me);
                    events.ScheduleEvent(EVENT_THORIM_S_HAMMER, 35*IN_MILLISECONDS);
                    ++towersIntact;
                }

                if (flag & FREYA_TOWER)
                {
                    me->AddAura(SPELL_BUFF_TOWER_OF_LIFE, me);
                    events.ScheduleEvent(EVENT_FREYA_S_WARD, 140*IN_MILLISECONDS);
                    ++towersIntact;
                }

                if (flag & MIMIRON_TOWER)
                {
                    me->AddAura(SPELL_BUFF_TOWER_OF_FLAMES, me);
                    events.ScheduleEvent(EVENT_MIMIRON_S_INFERNO, 70*IN_MILLISECONDS);
                    ++towersIntact;
                }

                // Hack Health (Spell Mod Increase Health Percent stacks additive)
                switch (towersIntact)
                {
                    case 4:  me->SetMaxHealth(RAID_MODE(HEALTH_10_4T, HEALTH_25_4T)); break;
                    case 3:  me->SetMaxHealth(RAID_MODE(HEALTH_10_3T, HEALTH_25_3T)); break;
                    case 2:  me->SetMaxHealth(RAID_MODE(HEALTH_10_2T, HEALTH_25_2T)); break;
                    case 1:  me->SetMaxHealth(RAID_MODE(HEALTH_10_1T, HEALTH_25_1T)); break;
                    default: me->SetMaxHealth(RAID_MODE(HEALTH_10_0T, HEALTH_25_0T)); break;
                }
                me->SetHealth(me->GetMaxHealth());

                // Set Loot Mode
                me->ResetLootMode();
                switch (towersIntact)
                {
                    case 4: me->AddLootMode(LOOT_MODE_HARD_MODE_4);
                    case 3: me->AddLootMode(LOOT_MODE_HARD_MODE_3);
                    case 2: me->AddLootMode(LOOT_MODE_HARD_MODE_2);
                    case 1: me->AddLootMode(LOOT_MODE_HARD_MODE_1); break;
                    default: me->AddLootMode(32); break;
                }
            }

            void JustDied(Unit* killer)
            {
                // Despawn adds first
                EntryCheckPredicate pred(NPC_FREYA_BEACON);
                summons.DoAction(ACTION_DESPAWN_ADDS, pred);
                instance->SetBossState(BOSS_LEVIATHAN, DONE);

                BossAI::JustDied(killer);

                Talk(SAY_DEATH);
                me->RemoveAllAuras();
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_PURSUED)
                {
                    ResetThreatList();
                    AddThreat(target, 9999999.0f);
                    if (Player* player = target->GetCharmerOrOwnerPlayerOrPlayerItself())
                        me->MonsterTextEmote(EMOTE_PURSUE, player, true);
                }
            }

            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                // Prevent applying auras by players, such as Curse of Elements
                if (caster && caster->GetTypeId() == TYPEID_PLAYER)
                    me->RemoveAurasDueToSpell(spell->Id);

                switch (spell->Id)
                {
                    case SPELL_ELECTROSHOCK:
                        me->InterruptNonMeleeSpells(true);
                        break;
                    case SPELL_OVERLOAD_CIRCUIT:
                        ++ShutdownCounter;
                        if (ShutdownCounter == RAID_MODE(2, 4))
                        {
                            ShutdownCounter = 0;
                            events.ScheduleEvent(EVENT_SHUTDOWN, 4000);
                            me->RemoveAurasDueToSpell(SPELL_OVERLOAD_CIRCUIT);
                            me->InterruptNonMeleeSpells(true);
                        }
                        break;
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING) || me->HasAura(SPELL_SYSTEMS_SHUTDOWN))
                    return;

                if (!Pursued && me->GetVictim() && !me->GetVictim()->HasAura(SPELL_PURSUED))
                {
                    Pursued = true;
                    events.RescheduleEvent(EVENT_PURSUE, 1000);
                }

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_PURSUE:
                            Talk(SAY_TARGET);
                            DoCast(SPELL_PURSUED);
                            Pursued = false;
                            events.ScheduleEvent(EVENT_PURSUE, 30*IN_MILLISECONDS);
                            break;
                        case EVENT_MISSILE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                                DoCast(target, SPELL_MISSILE_BARRAGE, true);
                            events.ScheduleEvent(EVENT_MISSILE, 1500);
                            break;
                        case EVENT_SPEED:
                            DoCastAOE(SPELL_GATHERING_SPEED, true);
                            events.ScheduleEvent(EVENT_SPEED, 10*IN_MILLISECONDS);
                            break;
                        case EVENT_VENT:
                            DoCastAOE(SPELL_FLAME_VENTS);
                            events.ScheduleEvent(EVENT_VENT, 20*IN_MILLISECONDS);
                            break;
                        case EVENT_SHUTDOWN:
                            Talk(SAY_OVERLOAD);
                            Talk(EMOTE_OVERLOAD);

                            me->RemoveAurasDueToSpell(SPELL_GATHERING_SPEED);
                            me->AddAura(SPELL_SYSTEMS_SHUTDOWN, me);

                            ShutdownActive = true;
                            me->SetReactState(REACT_PASSIVE);
                            me->GetMotionMaster()->Clear();
                            me->GetMotionMaster()->MoveIdle();
                            me->AddUnitState(UNIT_STATE_STUNNED | UNIT_STATE_ROOT);

                            if (AchievementShutout)
                                AchievementShutout = false;

                            events.ScheduleEvent(EVENT_REPAIR, 20*IN_MILLISECONDS);
                            break;
                        case EVENT_REPAIR:
                            Talk(EMOTE_REPAIR);

                            ShutdownActive = false;
                            me->ClearUnitState(UNIT_STATE_STUNNED | UNIT_STATE_ROOT);
                            me->SetReactState(REACT_AGGRESSIVE);
                            me->GetMotionMaster()->MoveChase(me->GetVictim());
                            break;
                        case EVENT_THORIM_S_HAMMER: // Tower of Storms
                            for (uint8 i = 0; i < RAID_MODE(3, 7); ++i)
                                DoSummon(NPC_THORIM_BEACON, me, float(urand(20, 50)), 20000, TEMPSUMMON_TIMED_DESPAWN);
                            Talk(SAY_TOWER_STORM);
                            break;
                        case EVENT_MIMIRON_S_INFERNO: // Tower of Flames
                            Talk(SAY_TOWER_FLAME);
                            DoSummon(NPC_MIMIRON_BEACON, MimironBeaconPosition);
                            break;
                        case EVENT_HODIR_S_FURY:      // Tower of Frost
                            Talk(SAY_TOWER_FROST);
                            for (uint8 i = 0; i < RAID_MODE(2, 5); ++i)
                                DoSummon(NPC_HODIR_BEACON, me, 50, 0);
                            break;
                        case EVENT_FREYA_S_WARD:    // Tower of Nature
                            Talk(SAY_TOWER_NATURE);
                            for (uint8 i = 0; i < 4; ++i)
                                DoSummon(NPC_FREYA_BEACON, FreyaBeaconPosition[i]);
                            events.ScheduleEvent(EVENT_FREYA_S_WARD_SUMMON, 15*IN_MILLISECONDS);
                            break;
                        case EVENT_FREYA_S_WARD_SUMMON:
                            DoCast(SPELL_FREYA_S_WARD);
                            events.ScheduleEvent(EVENT_FREYA_S_WARD_SUMMON, 30*IN_MILLISECONDS);
                            break;
                    }
                }

                if (me->IsWithinMeleeRange(me->GetVictim())) // bugged spell casts on units that are boarded on leviathan
                    DoSpellAttackIfReady(SPELL_BATTERING_RAM);

                DoMeleeAttackIfReady();
            }

            void JustSummoned(Creature* summon)
            {
                if (summon->GetEntry() == NPC_SEAT)
                    return;

                summons.Summon(summon);
                switch(summon->GetEntry())
                {
                    case NPC_WRITHING_LASHER:
                    case NPC_WARD_OF_LIFE:
                    {
                        summon->setActive(true);
                        // This workaround is needed because DoInCombatWithZone adds combat state only to players and not to vehicles
                        for (Player& player : me->GetMap()->GetPlayers())
                        {
                            if (player.IsGameMaster())
                                continue;

                            Unit* target = NULL;

                            if (player.IsAlive())
                            {
                                if (Unit* base = player.GetVehicleBase())   // is on vehicle
                                {
                                    if (Unit* baseBase = base->GetVehicleBase()) // Sub seats
                                        target = baseBase;
                                    else
                                        target = base;
                                }
                                else
                                    target = &player;
                            }

                            if (target)
                            {
                                summon->SetInCombatWith(target);
                                player.SetInCombatWith(summon);
                                AddThreat(target, 0.0f, summon);
                            }
                        }
                        break;
                    }
                }
            }

            uint32 GetData(uint32 type) const
            {
                switch (type)
                {
                    case DATA_SHUTOUT:
                        return AchievementShutout ? 1 : 0;
                    case DATA_UNBROKEN:
                        return AchievementUnbroken ? 1 : 0;
                    case DATA_ORBIT_ACHIEVEMENTS:
                        return hardmode ? towersIntact : 0;
                    default:
                        break;
                }

                return 0;
            }

            void SetData(uint32 id, uint32 data)
            {
                if (id == DATA_UNBROKEN)
                    AchievementUnbroken = data ? true : false;
            }

            void DoAction(int32 action)
            {
                // Start encounter
                if (action == ACTION_DRIVE_TO_CENTER)
                {
                    if (GameObject* Gate = me->FindNearestGameObject(GO_LEVIATHAN_GATE, 50.0f))
                        Gate->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);

                    me->SetHomePosition(Center);
                    me->GetMotionMaster()->MoveTargetedHome();
                    //! For some reason this doesn't work -.-
//                    me->GetMotionMaster()->MoveCharge(Center.GetPositionX(), Center.GetPositionY(), Center.GetPositionZ()); // position center
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                }
            }

            private:
                // In fight usage
                Vehicle* vehicle;
                uint8 ShutdownCounter;
                bool ShutdownActive;
                bool Pursued;
                bool AchievementShutout;
                bool AchievementUnbroken;
                // Pre fight usage
                uint8 towersIntact;
                bool hardmode;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_flame_leviathanAI(creature);
        }
};

//! Script for the seat on which the player lands when he is thrown by a demolisher
class boss_flame_leviathan_seat : public CreatureScript
{
    public:
        boss_flame_leviathan_seat() : CreatureScript("boss_flame_leviathan_seat") { }

        struct boss_flame_leviathan_seatAI : public ScriptedAI
        {
            boss_flame_leviathan_seatAI(Creature* creature) : ScriptedAI(creature), vehicle(creature->GetVehicleKit())
            {
                ASSERT(vehicle);
                me->SetReactState(REACT_PASSIVE);
                me->SetDisplayId(me->GetCreatureTemplate()->Modelid2);
                instance = creature->GetInstanceScript();
                ASSERT(instance);
            }

            InstanceScript* instance;
            Vehicle* vehicle;

            void Reset()
            {
                if (!me->FindNearestCreature(NPC_LEVIATHAN, 100.0f))
                    me->DespawnOrUnsummon();
            }

            void PassengerBoarded(Unit* who, int8 seatId, bool apply)
            {
                Unit* leviathan = ObjectAccessor::GetUnit(*me, ObjectGuid(instance->GetGuidData(BOSS_LEVIATHAN)));
                if (!leviathan)
                    return;

                if (seatId == SEAT_PLAYER)
                {
                    if (!apply)
                        return;
                    else
                        Talk(SAY_PLAYER_RIDING);

                    if (Unit* turretU = vehicle->GetPassenger(SEAT_TURRET))
                        if (Creature* turret = turretU->ToCreature())
                        {
                            turret->setFaction(leviathan->getFaction());
                            turret->SetUInt32Value(UNIT_FIELD_FLAGS, 0); // unselectable
                            turret->AI()->AttackStart(who);
                        }

                    if (Unit* deviceU = vehicle->GetPassenger(SEAT_DEVICE))
                        if (Creature* device = deviceU->ToCreature())
                        {
                            device->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                            device->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        }

                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_flame_leviathan_seatAI(creature);
        }
};

//! Canon on the Flame Leviathan
class boss_flame_leviathan_defense_cannon : public CreatureScript
{
    public:
        boss_flame_leviathan_defense_cannon() : CreatureScript("boss_flame_leviathan_defense_cannon") { }

        struct boss_flame_leviathan_defense_cannonAI : public ScriptedAI
        {
            boss_flame_leviathan_defense_cannonAI(Creature* creature) : ScriptedAI(creature) {}

            uint32 NapalmTimer;

            void Reset ()
            {
                if (!me->FindNearestCreature(NPC_LEVIATHAN, 100.0f))
                    me->DespawnOrUnsummon();

                NapalmTimer = 5000;
                DoCastSelf(AURA_STEALTH_DETECTION);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (NapalmTimer <= diff)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        if(CanAIAttack(target))
                            DoCast(target, SPELL_NAPALM, true);

                    NapalmTimer = 5000;
                }
                else
                    NapalmTimer -= diff;
            }

            bool CanAIAttack(Unit const* who) const
            {
                if (who->GetTypeId() != TYPEID_PLAYER || !who->GetVehicle() || who->GetVehicleBase()->GetEntry() == NPC_SEAT || who->GetEntry()== NPC_SEAT)
                    return false;
                return true;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_flame_leviathan_defense_cannonAI(creature);
        }
};

//! Canon that shoots at player on Flame Leviathan Seats
class boss_flame_leviathan_defense_turret : public CreatureScript
{
    public:
        boss_flame_leviathan_defense_turret() : CreatureScript("boss_flame_leviathan_defense_turret") { }

        struct boss_flame_leviathan_defense_turretAI : public TurretAI
        {
            boss_flame_leviathan_defense_turretAI(Creature* creature) : TurretAI(creature) {}

            void Reset()
            {
                if (!me->FindNearestCreature(NPC_LEVIATHAN, 100.0f))
                    me->DespawnOrUnsummon();
            }

            void DamageTaken(Unit* who, uint32 &damage)
            {
                if (!CanAIAttack(who))
                    damage = 0;

                if (damage >= me->GetHealth())
                {
                    AchievementEntry const* achievement = sAchievementStore.LookupEntry(who->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL ? ACHIEVEMENT_TAKE_OUT_THOSE_TURRETS_10 : ACHIEVEMENT_TAKE_OUT_THOSE_TURRETS_25);
                    if (achievement)
                        if (who->ToPlayer())
                            who->ToPlayer()->CompletedAchievement(achievement);

                    if (Vehicle* seat = me->GetVehicle())
                    {
                        if (Unit* device = seat->GetPassenger(SEAT_DEVICE))
                            if (Creature* creature = device->ToCreature())
                            {
                                bool dummy = true;
                                creature->AI()->OnSpellClick(NULL, dummy);
                            }
                        if (Unit* base = seat->GetBase())
                        {
                            base->ExitVehicle();
                            if (Creature* creat = base->ToCreature())
                                creat->DespawnOrUnsummon();
                        }
                    }
                }
            }

            bool CanAIAttack(Unit const* who) const
            {
                if (who->GetTypeId() != TYPEID_PLAYER || !who->GetVehicle() || who->GetVehicleBase()->GetEntry() != NPC_SEAT)
                    return false;
                return true;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_flame_leviathan_defense_turretAI(creature);
        }
};

//! Overload device that triggers Overload Circut
class boss_flame_leviathan_overload_device : public CreatureScript
{
    public:
        boss_flame_leviathan_overload_device() : CreatureScript("boss_flame_leviathan_overload_device") { }

        struct boss_flame_leviathan_overload_deviceAI : public PassiveAI
        {
            boss_flame_leviathan_overload_deviceAI(Creature* creature) : PassiveAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            void Reset()
            {
                if (!me->FindNearestCreature(NPC_LEVIATHAN, 100.0f))
                    me->DespawnOrUnsummon();
            }

            void OnSpellClick(Unit* /*clicker*/, bool& result)
            {
                if (me->GetVehicle())
                {
                    if (Unit* levi = ObjectAccessor::GetUnit(*me, ObjectGuid(instance->GetGuidData(BOSS_LEVIATHAN))))
                        DoCast(levi, SPELL_OVERLOAD_CIRCUIT);
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    if (Unit* player = me->GetVehicle()->GetPassenger(SEAT_PLAYER))
                    {
                        me->GetVehicleBase()->CastSpell(player, SPELL_SMOKE_TRAIL, true);
                        player->ExitVehicle();
                        player->GetMotionMaster()->MoveKnockbackFrom(me->GetVehicleBase()->GetPositionX(), me->GetVehicleBase()->GetPositionY(), 30, 30);
                    }
                }
                result = true;
            }
            private:
                InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_flame_leviathan_overload_deviceAI(creature);
        }
};

class npc_flame_leviathan_safety_container : public CreatureScript
{
public:
    npc_flame_leviathan_safety_container() : CreatureScript("npc_flame_leviathan_safety_container") { }

    struct npc_flame_leviathan_safety_containerAI : PassiveAI
    {
        npc_flame_leviathan_safety_containerAI(Creature* creature) : PassiveAI(creature) { }

        void UpdateAI(uint32 /*diff*/) 
        {
            if (me->GetPositionZ() <= 420.f && !me->GetVehicle())
                me->DespawnOrUnsummon();
        }
    };

     CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_flame_leviathan_safety_containerAI(creature);
        }
};

class npc_mechanolift : public CreatureScript
{
    public:
        npc_mechanolift() : CreatureScript("npc_mechanolift") { }

        struct npc_mechanoliftAI : public PassiveAI
        {
            bool triggered;

            npc_mechanoliftAI(Creature* creature) : PassiveAI(creature)
            {
                me->AddUnitMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING);
            }

            void Reset()
            {
                me->GetMotionMaster()->MoveTargetedHome();
                triggered = false;
            }

            void JustDied(Unit* /*killer*/)
            {
                if (!triggered)
                {
                    triggered = true;
                    if (Creature* liquid = me->SummonCreature(NPC_LIQUID, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 190000))
                    {
                        liquid->AddUnitMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING);
                        liquid->GetMotionMaster()->MoveFall();
                        if (Creature* container = me->FindNearestCreature(NPC_CONTAINER, 3.f,true))
                            container->DespawnOrUnsummon();

                        me->DespawnOrUnsummon();
                    }
                }
            }

            void PassengerBoarded(Unit* unit, int8 /*seat*/, bool apply)
            {
                if (apply)
                    DoCast(unit, SPELL_ROPE_BEAM);
            }

            void EnterEvadeMode(EvadeReason /*why*/) override {}
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_mechanoliftAI(creature);
        }
};

class npc_liquid_pyrite : public CreatureScript
{
    public:
        npc_liquid_pyrite() : CreatureScript("npc_liquid_pyrite") { }

        struct npc_liquid_pyriteAI : public PassiveAI
        {
            npc_liquid_pyriteAI(Creature* creature) : PassiveAI(creature) { }

            void Reset()
            {
                DoCastSelf(SPELL_PARACHUTE, true);
                me->SetDisplayId(DISPLAYID_CONTAINER);
            }

            void MovementInform(uint32 type, uint32 /*id*/)
            {
                if (type == EFFECT_MOTION_TYPE)
                {
                    DoCastSelf(SPELL_DUSTY_EXPLOSION, true);
                    DoCastSelf(SPELL_DUST_CLOUD_IMPACT, true);
                    me->RemoveAurasDueToSpell(SPELL_PARACHUTE);
                    me->SetDisplayId(DISPLAYID_PYRIT);
                }
            }

            void DamageTaken(Unit* /*who*/, uint32& damage)
            {
                damage = 0;
            }

            void UpdateAI(uint32 /*diff*/) {}
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_liquid_pyriteAI(creature);
        }
};

class npc_pool_of_tar : public CreatureScript
{
    public:
        npc_pool_of_tar() : CreatureScript("npc_pool_of_tar") { }

        struct npc_pool_of_tarAI : public ScriptedAI
        {
            npc_pool_of_tarAI(Creature* creature) : ScriptedAI(creature)
            {
                me->AddAura(SPELL_TAR_PASSIVE, me);
            }

            void DamageTaken(Unit* /*who*/, uint32& damage)
            {
                damage = 0;
            }

            void SpellHit(Unit* /*caster*/, SpellInfo const* spell)
            {
                if ((spell->Id == 65044 || spell->Id == 65045) && !me->HasAura(SPELL_BLAZE))
                    DoCastSelf(SPELL_BLAZE, true);
            }

            void UpdateAI(uint32 /*diff*/) {}
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_pool_of_tarAI(creature);
        }
};

class npc_gauntlet_generator : public CreatureScript
{
    public:
        npc_gauntlet_generator() : CreatureScript("npc_gauntlet_generator") { }

        struct npc_gauntlet_generatorAI : public ScriptedAI
        {
            npc_gauntlet_generatorAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            void Reset()
            {
                spawnTimer = 3000;
            }

            void UpdateAI(uint32 diff)
            {
                if (spawnTimer <= diff)
                {
                    if (Player* plr = me->FindNearestPlayer(150.0f))
                    {
                        if (!plr->IsGameMaster())
                        {
                            if (me->GetEntry() == NPC_ULDUAR_GAUNTLET_GENERATOR_2)
                            {
                                GameObject* tower = me->FindNearestGameObject(194371, 30.0f);
                                if (!tower)
                                    tower = me->FindNearestGameObject(194377, 30.0f);
                                if (!tower)
                                    tower = me->FindNearestGameObject(194370, 30.0f);
                                if (!tower)
                                    tower = me->FindNearestGameObject(194375, 30.0f);
                                if (tower && tower->GetDestructibleState() != GO_DESTRUCTIBLE_DESTROYED && tower->GetGOValue()->Building.Health < tower->GetGOValue()->Building.MaxHealth - 5000)
                                    if (TempSummon* summon = me->SummonCreature(33236, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000))
                                        summon->GetMotionMaster()->MoveFollow(plr, 1.0f, summon->GetAngle(plr)); //Without this just lots of npcs spawns without doing anything
                            }
                            else
                                    if (TempSummon* summon = me->SummonCreature(33236, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000))
                                        summon->GetMotionMaster()->MoveFollow(plr, 1.0f, summon->GetAngle(plr));
                        }
                    }
                    spawnTimer = 3000;
                }
                else spawnTimer -= diff;
            }

        private:
            uint32 spawnTimer;
            InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_gauntlet_generatorAI(creature);
        }
};

class npc_ulduar_colossus : public CreatureScript
{
    public:
        npc_ulduar_colossus() : CreatureScript("npc_ulduar_colossus") { }

        struct npc_ulduar_colossusAI : public ScriptedAI
        {
            npc_ulduar_colossusAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            void Reset()
            {
                GroundSlamTimer = urand(8*IN_MILLISECONDS, 10*IN_MILLISECONDS);
            }

            void JustDied(Unit* /*Who*/)
            {
                if (me->GetHomePosition().IsInDist(&Center, 50.0f))
                    instance->SetData(DATA_COLOSSUS, 0);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (GroundSlamTimer <= diff)
                {
                    DoCastVictim(SPELL_GROUND_SLAM);
                    GroundSlamTimer = urand(20*IN_MILLISECONDS, 25*IN_MILLISECONDS);
                }
                else
                    GroundSlamTimer -= diff;

                DoMeleeAttackIfReady();
            }

        private:
            uint32 GroundSlamTimer;
            InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_ulduar_colossusAI(creature);
        }
};

class npc_thorims_hammer : public CreatureScript
{
    public:
        npc_thorims_hammer() : CreatureScript("npc_thorims_hammer") { }

        struct npc_thorims_hammerAI : public ScriptedAI
        {
            npc_thorims_hammerAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                me->SetImmuneToAll(true);
                me->SetReactState(REACT_PASSIVE);
                me->AddAura(AURA_DUMMY_BLUE, me);
            }

            void AttackStart(Unit* /*target*/) {}

            void Reset()
            {
                thorimsHammerTimer = urand(4, 7)*IN_MILLISECONDS;
                me->GetMotionMaster()->MoveRandom(100.0f);
            }

            void UpdateAI(uint32 diff)
            {
                if (!me->HasAura(AURA_DUMMY_GREEN))
                    me->AddAura(AURA_DUMMY_GREEN, me);
                if (!me->HasAura(AURA_DUMMY_YELLOW))
                    me->AddAura(AURA_DUMMY_YELLOW, me);

                UpdateVictim();

                if (thorimsHammerTimer <= diff)
                {
                    if (Creature* trigger = DoSummonFlyer(NPC_THORIM_TARGET_BEACON, me, 20, 0, 3000, TEMPSUMMON_TIMED_DESPAWN))
                    {
                        trigger->CastSpell(trigger, SPELL_THORIM_S_HAMMER);
                    }

                    thorimsHammerTimer = urand(4, 7)*IN_MILLISECONDS;
                }
                else
                    thorimsHammerTimer -= diff;
            }

            private:
                uint32 thorimsHammerTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_thorims_hammerAI(creature);
        }
};

class npc_mimirons_inferno : public CreatureScript
{
public:
    npc_mimirons_inferno() : CreatureScript("npc_mimirons_inferno") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_mimirons_infernoAI(creature);
    }

    struct npc_mimirons_infernoAI : public npc_escortAI
    {
        npc_mimirons_infernoAI(Creature* creature) : npc_escortAI(creature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_NOT_SELECTABLE);
            me->SetImmuneToAll(true);
            me->AddAura(AURA_DUMMY_YELLOW, me);
            me->SetReactState(REACT_PASSIVE);
            SetCombatMovement(false);
        }

        void WaypointReached(uint32 /*i*/) {}
        void AttackStart(Unit* /*target*/) {}

        void Reset()
        {
            infernoTimer = 2*IN_MILLISECONDS;
        }

        uint32 infernoTimer;

        void UpdateAI(uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);

            if (!HasEscortState(STATE_ESCORT_ESCORTING))
                Start(false, true, ObjectGuid::Empty, NULL, false, true);
            else
            {
                if (infernoTimer <= diff)
                {
                    if (Creature* trigger = DoSummonFlyer(NPC_MIMIRON_TARGET_BEACON, me, 20, 0, 20000, TEMPSUMMON_TIMED_DESPAWN))
                    {
                        trigger->CastSpell(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), SPELL_MIMIRON_S_INFERNO, true);
                        trigger->CastSpell(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), SPELL_MIMIRONS_INFERNO, true);
                        infernoTimer = 2*IN_MILLISECONDS;
                    }
                }
                else
                    infernoTimer -= diff;

                if (!me->HasAura(AURA_DUMMY_YELLOW))
                    me->AddAura(AURA_DUMMY_YELLOW, me);
            }
        }
    };

};

class npc_hodirs_fury : public CreatureScript
{
    public:
        npc_hodirs_fury() : CreatureScript("npc_hodirs_fury") { }

        struct npc_hodirs_furyAI : public ScriptedAI
        {
            bool isTriggered;
            uint32 timer;

            npc_hodirs_furyAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                me->SetImmuneToPC(true);
                me->AddAura(AURA_DUMMY_BLUE, me);
                me->SetReactState(REACT_PASSIVE);
            }

            void Reset()
            {
                isTriggered = false;
                timer = 2*IN_MILLISECONDS;
            }

            void AttackStart(Unit* /*target*/) {}

            void MovementInform(uint32 /*type*/, uint32 /*id*/)
            {
                // When waypoint reached, wait a few seconds and drop the frost ball
                isTriggered = true;
                timer = 2*IN_MILLISECONDS;
            }

            void UpdateAI(uint32 diff)
            {
                if (!me->HasAura(AURA_DUMMY_BLUE))
                    me->AddAura(AURA_DUMMY_BLUE, me);

                if (!isTriggered) // Finding player
                {
                    if (timer <= diff) // move to random player
                    {
                        std::list<Creature*> vehicleList;
                        me->GetCreatureListWithEntryInGrid(vehicleList, NPC_VEHICLE_CHOPPER, 200.0f);
                        me->GetCreatureListWithEntryInGrid(vehicleList, NPC_VEHICLE_SIEGE, 200.0f);
                        me->GetCreatureListWithEntryInGrid(vehicleList, NPC_VEHICLE_DEMOLISHER, 200.0f);

                        if (vehicleList.size())
                        {
                            std::list<Creature*>::iterator itr = vehicleList.begin();
                            std::advance(itr, urand(0, vehicleList.size() - 1));
                            me->GetMotionMaster()->MovePoint(0, (*itr)->GetPositionX(), (*itr)->GetPositionY(), (*itr)->GetPositionZ());
                        }
                        timer = 30*IN_MILLISECONDS;
                    }
                    else
                        timer -= diff;
                }
                else
                {
                    if (timer <= diff)
                    {
                         if (Creature* trigger = DoSummonFlyer(NPC_HODIR_TARGET_BEACON, me, 20, 0, 1000, TEMPSUMMON_TIMED_DESPAWN))
                            trigger->CastSpell(me, SPELL_HODIR_S_FURY, true);
                         isTriggered = false;
                         timer = 3*IN_MILLISECONDS;
                    }
                    else
                        timer -= diff;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_hodirs_furyAI(creature);
        }
};

class npc_freyas_ward : public CreatureScript
{
    public:
        npc_freyas_ward() : CreatureScript("npc_freyas_ward") { }

        struct npc_freyas_wardAI : public ScriptedAI
        {
            npc_freyas_wardAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                me->SetReactState(REACT_PASSIVE);
            }

            void Reset()
            {
                me->AddAura(AURA_DUMMY_GREEN, me);
            }

            void AttackStart(Unit* /*target*/) {}

            void UpdateAI(uint32 /*diff*/)
            {
                if (!me->HasAura(AURA_DUMMY_GREEN))
                    me->AddAura(AURA_DUMMY_GREEN, me);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_freyas_wardAI(creature);
        }
};

class spell_freyas_ward : public SpellScriptLoader
{
    public:
        spell_freyas_ward() : SpellScriptLoader("spell_freyas_ward") { }

        class spell_freyas_ward_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_freyas_ward_SpellScript);

            void HandleDummyHit(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                    if (const WorldLocation* location = GetExplTargetDest())
                        caster->CastSpell(location->GetPositionX(), location->GetPositionY(), location->GetPositionZ(), GetSpellInfo()->Effects[EFFECT_1].CalcValue(), true);
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_freyas_ward_SpellScript::HandleDummyHit, EFFECT_1, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_freyas_ward_SpellScript();
        }
};

//npc lore keeper
#define GOSSIP_ITEM_1  "Sekund�re Verteidigungssysteme aktivieren"
#define GOSSIP_ITEM_2  "best�tigt"


class npc_lorekeeper : public CreatureScript
{
    public:
        npc_lorekeeper() : CreatureScript("npc_lorekeeper") { }

        struct npc_lorekeeperAI : public ScriptedAI
        {
            npc_lorekeeperAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            void DoAction(int32 action)
            {
                // Start encounter
                if (action == 0)
                {
                    for (int32 i = 0; i < RAID_MODE(2, 5); ++i)
                        DoSummon(NPC_VEHICLE_SIEGE, SiegePositions[i], 3000, TEMPSUMMON_MANUAL_DESPAWN);
                    for (int32 i = 0; i < RAID_MODE(2, 5); ++i)
                        DoSummon(NPC_VEHICLE_CHOPPER, ChopperPositions[i], 3000, TEMPSUMMON_MANUAL_DESPAWN);
                    for (int32 i = 0; i < RAID_MODE(2, 5); ++i)
                        DoSummon(NPC_VEHICLE_DEMOLISHER, DemolisherPositions[i], 3000, TEMPSUMMON_MANUAL_DESPAWN);
                    return;
                }
            }
        };

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();
            InstanceScript* instance = creature->GetInstanceScript();
            if (!instance)
                return true;

            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF+1:
                    if (player)
                    {
                        player->PrepareGossipMenu(creature);
                        instance->instance->LoadGrid(364, -16); //make sure leviathan is loaded

                        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
                    }
                    break;
                case GOSSIP_ACTION_INFO_DEF+2:
                    if (player)
                        player->CLOSE_GOSSIP_MENU();

                    if (instance->instance->GetCreature(ObjectGuid(instance->GetGuidData(BOSS_LEVIATHAN))))
                    {
                        instance->SetData(DATA_LEVIATHAN_HARDMODE, 0); //enable hard mode activating the 4 additional events spawning additional vehicles
                        creature->SetVisible(false);
                        creature->AI()->DoAction(0); // spawn the vehicles
                        if (Creature* Delorah = creature->FindNearestCreature(NPC_DELORAH, 1000, true))
                        {
                            if (Creature* brand = creature->FindNearestCreature(NPC_BRANZ_BRONZBEARD, 1000, true))
                            {
                                Delorah->GetMotionMaster()->MovePoint(0, brand->GetPositionX()-4, brand->GetPositionY(), brand->GetPositionZ());
                                brand->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                                //TODO DoScriptText(xxxx, Delorah, Branz); when reached at branz
                            }
                        }
                        creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    }
                    break;
            }

            return true;
        }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            InstanceScript* instance = creature->GetInstanceScript();
            if (instance && instance->GetData(BOSS_LEVIATHAN) !=DONE && player)
            {
                player->PrepareGossipMenu(creature);

                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
                player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            }
            return true;
        }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lorekeeperAI(creature);
        }
};

////npc_brann_bronzebeard this requires more work involving area triggers. if reached this guy speaks through his radio..
#define GOSSIP_ITEM_BRANN_1  "Wir sind Bereit. Lasst uns den Angriff beginnen!"

enum BrannStartEventMisc
{
    ACTION_START         = 0,
    GO_PROTECTION_BUBBLE = 194484,

    // Events
    EVENT_BRANN_START_1 = 1,
    EVENT_BRANN_START_2 = 2,
    EVENT_BRANN_START_3 = 3,
    EVENT_BRANN_START_4 = 4,
    EVENT_BRANN_START_5 = 5,
    EVENT_BRANN_START_6 = 6,
    EVENT_BRANN_START_7 = 7,
    EVENT_BRANN_START_8 = 8,
    EVENT_BRANN_START_9 = 9,

    // Texts
    SAY_BRANN_INTRO_1    = 0,
    SAY_BRANN_INTRO_2    = 1,
    SAY_BRANN_INTRO_3    = 2,
    SAY_BRANN_INTRO_4    = 3,

    SAY_MAGE_1           = 0,
    SAY_MAGE_2           = 1,

    // Creatures
    NPC_PENTARUS         = 33624,
    NPC_MAGE_B           = 33672,
    NPC_BATTLEMAGE_B     = 33662,
    NPC_ENGINEER_B       = 33626,
    SPELL_SHIELD_B       = 48310
};

class npc_brann_bronzebeard : public CreatureScript
{
public:
    npc_brann_bronzebeard() : CreatureScript("npc_brann_bronzebeard") { }

    struct npc_brann_bronzebeardAI : public ScriptedAI
    {
        npc_brann_bronzebeardAI(Creature* creature) : ScriptedAI(creature) { }

        void DoAction(int32 action)
        {
            // Start encounter
            if (action == ACTION_START)
            {
                events.ScheduleEvent(EVENT_BRANN_START_1, 9 * IN_MILLISECONDS);

                for (int32 i = 0; i < RAID_MODE(2, 5); ++i)
                    DoSummon(NPC_VEHICLE_SIEGE, SiegePositions[i], 3000, TEMPSUMMON_MANUAL_DESPAWN);
                for (int32 i = 0; i < RAID_MODE(2, 5); ++i)
                    DoSummon(NPC_VEHICLE_CHOPPER, ChopperPositions[i], 3000, TEMPSUMMON_MANUAL_DESPAWN);
                for (int32 i = 0; i < RAID_MODE(2, 5); ++i)
                    DoSummon(NPC_VEHICLE_DEMOLISHER, DemolisherPositions[i], 3000, TEMPSUMMON_MANUAL_DESPAWN);
                return;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_BRANN_START_1:
                        if (Creature* pentarus = me->FindNearestCreature(NPC_PENTARUS, 20.0f))
                            pentarus->AI()->Talk(SAY_MAGE_1);
                        events.ScheduleEvent(EVENT_BRANN_START_2, 10 * IN_MILLISECONDS);
                        Talk(SAY_BRANN_INTRO_1);
                        break;
                    case EVENT_BRANN_START_2:
                        Talk(SAY_BRANN_INTRO_2);
                        if (Creature* pentarus = me->FindNearestCreature(NPC_PENTARUS, 20.0f))
                        {
                            pentarus->SetWalk(false);
                            pentarus->GetMotionMaster()->MovePoint(1, -678.796936f, -81.0948f, 427.1159f);
                        }
                        events.ScheduleEvent(EVENT_BRANN_START_3, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_BRANN_START_3:
                        if (Creature* gnome = me->FindNearestCreature(NPC_ENGINEER_B, 20.0f))
                            gnome->GetMotionMaster()->MovePoint(1, -800.580872f, -39.9979f, 429.8446f);
                        events.ScheduleEvent(EVENT_BRANN_START_4, 4 * IN_MILLISECONDS);
                        break;
                    case EVENT_BRANN_START_4:
                        me->SetWalk(false);
                        me->GetMotionMaster()->MovePoint(1, -679.1850f, -49.4837f, 426.7723f);
                        events.ScheduleEvent(EVENT_BRANN_START_5, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_BRANN_START_5:
                        if (Creature* pentarus = me->FindNearestCreature(NPC_PENTARUS, 200.0f))
                            pentarus->AI()->Talk(SAY_MAGE_2);
                        events.ScheduleEvent(EVENT_BRANN_START_6, 9 * IN_MILLISECONDS);
                        break;
                    case EVENT_BRANN_START_6:
                        Talk(SAY_BRANN_INTRO_3);
                        events.ScheduleEvent(EVENT_BRANN_START_7, 9 * IN_MILLISECONDS);
                        break;
                    case EVENT_BRANN_START_7:
                    {                        
                        Talk(SAY_BRANN_INTRO_4);
                        if (GameObject* Procetion_Buble = me->FindNearestGameObject(GO_PROTECTION_BUBBLE, 200.0f))
                            Procetion_Buble->Delete();

                        std::list<Creature*> HelperList;
                        me->GetCreatureListWithEntryInGrid(HelperList, NPC_BATTLEMAGE_B, 300.0f);
                        if (!HelperList.empty())
                            for (std::list<Creature*>::iterator itr = HelperList.begin(); itr != HelperList.end(); itr++)
                                (*itr)->InterruptNonMeleeSpells(true);

                        me->setActive(false);
                        break;
                    }
                    default:
                        break;
                }
            }
        }

    private:
        EventMap events;
    };

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
    {
        player->PlayerTalkClass->ClearMenus();
        InstanceScript* instance = creature->GetInstanceScript();

        switch(uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF:
                if (player)
                    player->CLOSE_GOSSIP_MENU();

                instance->instance->LoadGrid(364, -16); //make sure leviathan is loaded

                if (Creature* Lorekeeper = creature->FindNearestCreature(NPC_LOREKEEPER, 1000, true)) //lore keeper of lorgannon
                    Lorekeeper->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

                creature->AI()->DoAction(ACTION_START);
                creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);                
                creature->AI()->Talk(SAY_BRANN_INTRO_1, player);
                creature->setActive(true);
                break;
        }

        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        InstanceScript* instance = creature->GetInstanceScript();
        if (instance && instance->GetData(BOSS_LEVIATHAN) !=DONE)
        {
            player->PrepareGossipMenu(creature);

            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_BRANN_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_brann_bronzebeardAI(creature);
    }
};

//! Intended to be added to vehicles driven by players (Siege, Chopper, Demolisher)
class npc_leviathan_player_vehicle : public CreatureScript
{
    public:
        npc_leviathan_player_vehicle() : CreatureScript("npc_leviathan_player_vehicle") {}

        struct npc_leviathan_player_vehicleAI : public NullCreatureAI
        {
            npc_leviathan_player_vehicleAI(Creature* creature) : NullCreatureAI(creature)
            {
                _instance = creature->GetInstanceScript();

                if (VehicleSeatEntry* vehSeat = const_cast<VehicleSeatEntry*>(sVehicleSeatStore.LookupEntry(3013)))
                    vehSeat->m_flags &= ~VEHICLE_SEAT_FLAG_ALLOW_TURNING;
            }

            void PassengerBoarded(Unit* unit, int8 seat, bool apply)
            {
                if (!unit->ToPlayer() || seat != 0)
                    return;

                // Workaround to apply vehicle scaling on entering/leaving main seats only
                if (apply)
                    unit->CastSpell(me, 65266, true);
                else
                    me->RemoveAurasDueToSpell(65266);

                if (Creature* leviathan = ObjectAccessor::GetCreature(*me, _instance ? ObjectGuid(_instance->GetGuidData(BOSS_LEVIATHAN)) : ObjectGuid::Empty))
                {
                    if (leviathan->IsInCombat())
                    {
                        leviathan->EngageWithTarget(me);
                        if (apply)
                            me->SetHealth(uint32(me->GetHealth() / 2));
                    }
                }
            }

        private:
            InstanceScript* _instance;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_leviathan_player_vehicleAI(creature);
        }
};


class go_ulduar_tower : public GameObjectScript
{
    public:
        go_ulduar_tower() : GameObjectScript("go_ulduar_tower") { }

        void OnDestroyed(GameObject* go, Player* /*player*/)
        {
            InstanceScript* instance = go->GetInstanceScript();
            if (!instance)
                return;

            switch (go->GetEntry())
            {
                case GO_TOWER_OF_STORMS:
                    instance->ProcessEvent(go, EVENT_TOWER_OF_STORM_DESTROYED);
                    break;
                case GO_TOWER_OF_FLAMES:
                    instance->ProcessEvent(go, EVENT_TOWER_OF_FLAMES_DESTROYED);
                    break;
                case GO_TOWER_OF_FROST:
                    instance->ProcessEvent(go, EVENT_TOWER_OF_FROST_DESTROYED);
                    break;
                case GO_TOWER_OF_LIFE:
                    instance->ProcessEvent(go, EVENT_TOWER_OF_LIFE_DESTROYED);
                    break;
            }

            std::list<Creature*> Npc_List;
            go->GetCreatureListWithEntryInGrid(Npc_List,NPC_ULDUAR_GAUNTLET_GENERATOR,15.0f);
            go->GetCreatureListWithEntryInGrid(Npc_List,NPC_ULDUAR_GAUNTLET_GENERATOR_2,30.0f);
            go->GetCreatureListWithEntryInGrid(Npc_List,NPC_TOWER_CANNON,20.0f);
            for(std::list<Creature*>::iterator it = Npc_List.begin(); it != Npc_List.end(); ++it)
                (*it)->DisappearAndDie();
        }
};

//class at_RX_214_repair_o_matic_station : public AreaTriggerScript
//{
//    public:
//        at_RX_214_repair_o_matic_station() : AreaTriggerScript("at_RX_214_repair_o_matic_station") { }
//
//        bool OnTrigger(Player* player, const AreaTriggerEntry* /*at*/)
//        {
//            InstanceScript* instance = player->GetInstanceScript();
//            if (Creature* vehicle = player->GetVehicleCreatureBase())
//            {
//                if (!vehicle->HasAura(SPELL_AUTO_REPAIR))
//                {
//                    player->MonsterTextEmote(EMOTE_REPAIR, player->GetGUID(), true);
//                    player->CastSpell(vehicle, SPELL_AUTO_REPAIR, true);
//                    vehicle->SetFullHealth();
//                    if (Creature* leviathan = ObjectAccessor::GetCreature(*player, instance ? ObjectGuid(instance->GetGuidData(BOSS_LEVIATHAN)) : ObjectGuid::Empty))
//                        leviathan->AI()->SetData(DATA_UNBROKEN, 0); // set bool to false thats checked in leviathan getdata
//                }
//            }
//            return true;
//        }
//};

class go_RX_214_repair_o_matic_station : public GameObjectScript
{
public:
    go_RX_214_repair_o_matic_station() : GameObjectScript("go_RX_214_repair_o_matic_station") { }

    void OnTriggerTrap(GameObject* go, Unit* victim, uint32& spellId)
    {
        InstanceScript* instance = go->GetInstanceScript();
        if (Creature* vehicle = victim->GetVehicleCreatureBase())
        {
            if (!vehicle->HasAura(SPELL_AUTO_REPAIR))
            {
                vehicle->AI()->Talk(EMOTE_REPAIR);
                go->CastSpell(vehicle, SPELL_AUTO_REPAIR);
                vehicle->SetFullHealth();
                if (Creature* leviathan = ObjectAccessor::GetCreature(*go, instance ? ObjectGuid(instance->GetGuidData(BOSS_LEVIATHAN)) : ObjectGuid::Empty))
                    leviathan->AI()->SetData(DATA_UNBROKEN, 0); // set bool to false thats checked in leviathan getdata
            }
            else spellId = 0;
        }
    }
};

// strange workaround until traj target selection works
class spell_anti_air_rocket : public SpellScriptLoader
{
    public:
        spell_anti_air_rocket() : SpellScriptLoader("spell_anti_air_rocket") { }

        class spell_anti_air_rocket_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_anti_air_rocket_SpellScript);

            void HandleTriggerMissile(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                if (Position const* pos = GetExplTargetDest())
                {
                    if (Creature* temp = GetCaster()->SummonCreature(12999, *pos, TEMPSUMMON_TIMED_DESPAWN, 500))
                    {
                        temp->SetCanFly(true);
                        std::list<Creature*> list;
                        GetCaster()->GetCreatureListWithEntryInGrid(list, NPC_MECHANOLIFT, 100.0f);
                        GetCaster()->GetCreatureListWithEntryInGrid(list, NPC_AMBUSHER, 100.0f);
                        GetCaster()->GetCreatureListWithEntryInGrid(list, NPC_FROSTBROOD, 100.0f);

                        while (!list.empty())
                        {
                            std::list<Creature*>::iterator itr = list.begin();
                            std::advance(itr, urand(0, list.size()-1));

                            if ((*itr)->IsAlive() && (*itr)->IsInBetween(GetCaster(), temp, 20.0f))
                            {
                                GetCaster()->CastSpell((*itr)->GetPositionX(), (*itr)->GetPositionY(), (*itr)->GetPositionZ(), SPELL_ANTI_AIR_ROCKET_DMG, true);
                                // (*itr)->DealDamage((*itr),(*itr)->GetHealth());
                                return;
                            }
                            list.erase(itr);
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectLaunch += SpellEffectFn(spell_anti_air_rocket_SpellScript::HandleTriggerMissile, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_anti_air_rocket_SpellScript();
        }
};

class spell_pursued : public SpellScriptLoader
{
    public:
        spell_pursued() : SpellScriptLoader("spell_pursued") { }

        class spell_pursued_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pursued_SpellScript);

            bool Load()
            {
                _target = NULL;
                return GetCaster()->GetTypeId() == TYPEID_UNIT;
            }

            void SelectTarget(std::list<WorldObject*>& targetList)
            {
                if (targetList.empty())
                    return;

                std::list<Unit*> tempList;

                // try to find demolisher or siege engine first
                for (std::list<WorldObject*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                {
                    _target = (*itr)->ToUnit();

                    if (!_target->ToCreature())
                        continue;

                    if (_target->ToCreature()->GetEntry() == NPC_VEHICLE_SIEGE || _target->ToCreature()->GetEntry() == NPC_VEHICLE_DEMOLISHER){

                        Vehicle* vehicle = _target->GetVehicleKit();
                        // NPC must be a valid vehicle installation
                        if(!vehicle)
                            continue;

                        // vehicle must be in use by player
                        bool playerFound = false;
                        for (SeatMap::const_iterator v_itr = vehicle->Seats.begin(); v_itr != vehicle->Seats.end() && !playerFound; ++v_itr)
                            if (v_itr->second.Passenger.Guid.IsPlayer())
                                playerFound = true;

                        if(playerFound)
                            tempList.push_back(_target);
                    }
                }

                if (tempList.empty())
                {
                    // no demolisher or siege engine, find a chopper
                    for (std::list<WorldObject*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                    {
                        _target = (*itr)->ToUnit();

                        if (!_target->ToCreature())
                            continue;

                        if (_target->ToCreature()->GetEntry() == NPC_VEHICLE_CHOPPER)
                            tempList.push_back(_target);
                    }
                }

                if (!tempList.empty())
                {
                    // found one or more vehicles, select a random one
                    std::list<Unit*>::iterator itr = tempList.begin();
                    std::advance(itr, urand(0, tempList.size() - 1));
                    _target = *itr;
                    targetList.clear();
                    targetList.push_back(_target);
                }
                else
                {
                    // found no vehicles, select a random player or pet
                    std::list<WorldObject*>::iterator itr = targetList.begin();
                    std::advance(itr, urand(0, targetList.size() - 1));
                    _target = (*itr)->ToUnit();
                    targetList.clear();
                    targetList.push_back(_target);
                }
            }

            void SetTarget(std::list<WorldObject*>& targetList)
            {
                targetList.clear();

                if (_target)
                    targetList.push_back(_target);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pursued_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pursued_SpellScript::SetTarget, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
            }

            Unit* _target;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pursued_SpellScript();
        }
};

class spell_load_into_catapult : public SpellScriptLoader
{
    enum Spells
    {
        SPELL_PASSENGER_LOADED = 62340,
    };

    public:
        spell_load_into_catapult() : SpellScriptLoader("spell_load_into_catapult") { }

        class spell_load_into_catapult_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_load_into_catapult_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* owner = GetOwner()->ToUnit();
                if (!owner)
                    return;

                owner->CastSpell(owner, SPELL_PASSENGER_LOADED, true);
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* owner = GetOwner()->ToUnit();
                if (!owner)
                    return;

                owner->RemoveAurasDueToSpell(SPELL_PASSENGER_LOADED);
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_load_into_catapult_AuraScript::OnApply, EFFECT_0, SPELL_AURA_CONTROL_VEHICLE, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_load_into_catapult_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_CONTROL_VEHICLE, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_load_into_catapult_AuraScript();
        }
};

class spell_vehicle_throw_passenger : public SpellScriptLoader
{
    public:
        spell_vehicle_throw_passenger() : SpellScriptLoader("spell_vehicle_throw_passenger") {}

        class spell_vehicle_throw_passenger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_vehicle_throw_passenger_SpellScript);
            void HandleScript(SpellEffIndex effIndex)
            {
                Spell* baseSpell = GetSpell();
                SpellCastTargets targets = baseSpell->m_targets;
                int32 damage = GetEffectValue();
                if (targets.HasTraj())
                    if (Vehicle* vehicle = GetCaster()->GetVehicleKit())
                        if (Unit* passenger = vehicle->GetPassenger(damage - 1))
                        {
                            // use 99 because it is 3d search
                            std::list<WorldObject*> targetList;
                            Trinity::WorldObjectSpellAreaTargetCheck check(99, GetExplTargetDest(), GetCaster(), GetCaster(), GetSpellInfo(), TARGET_CHECK_DEFAULT, NULL);
                            GetCaster()->VisitAnyNearbyObject<WorldObject, Trinity::ContainerAction>(99, targetList, check);
                            float minDist = 99 * 99;
                            Unit* target = NULL;
                            for (std::list<WorldObject*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                            {
                                if (Unit* unit = (*itr)->ToUnit())
                                    if (unit->GetEntry() == NPC_SEAT)
                                        if (Vehicle* seat = unit->GetVehicleKit())
                                            if (!seat->GetPassenger(0))
                                                if (Unit* device = seat->GetPassenger(2))
                                                    if (!device->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
                                                    {
                                                        float dist = unit->GetExactDistSq(targets.GetDstPos());
                                                        if (dist < minDist)
                                                        {
                                                            minDist = dist;
                                                            target = unit;
                                                        }
                                                    }
                            }
                            if (target && target->IsWithinDist2d(targets.GetDstPos(), GetSpellInfo()->Effects[effIndex].CalcRadius() * 2)) // now we use *2 because the location of the seat is not correct
                                passenger->EnterVehicle(target, 0);
                            else
                            {
                                passenger->ExitVehicle();
                                passenger->GetMotionMaster()->MoveJump(*targets.GetDstPos(), targets.GetSpeedXY(), targets.GetSpeedZ());
                            }
                        }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_vehicle_throw_passenger_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_vehicle_throw_passenger_SpellScript();
        }
};

class FlameVentsTargetSelector
{
    public:
        bool operator() (WorldObject* object)
        {
            Unit* unit = object->ToUnit();
            if (unit->GetTypeId() != TYPEID_PLAYER)
            {
                if (unit->ToCreature()->GetEntry() == NPC_VEHICLE_SIEGE ||
                    unit->ToCreature()->GetEntry() == NPC_VEHICLE_CHOPPER ||
                    unit->ToCreature()->GetEntry() == NPC_VEHICLE_DEMOLISHER)
                    return false;

                if (!unit->ToCreature()->IsPet())
                    return true;
            }

            // TODO: more check?
            return unit->GetVehicle();
        }
};

class spell_flame_leviathan_flame_vents : public SpellScriptLoader
{
    public:
        spell_flame_leviathan_flame_vents() : SpellScriptLoader("spell_flame_leviathan_flame_vents") { }

        class spell_flame_leviathan_flame_vents_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_flame_leviathan_flame_vents_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if(FlameVentsTargetSelector());
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_flame_leviathan_flame_vents_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_flame_leviathan_flame_vents_SpellScript();
        }
};

class spell_shield_generator : public SpellScriptLoader
{
    public:
        spell_shield_generator() : SpellScriptLoader("spell_shield_generator") { }

        class spell_shield_generator_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_shield_generator_AuraScript);

            uint32 absorbPct;

            bool Load()
            {
                absorbPct = GetSpellInfo()->Effects[EFFECT_0].CalcValue(GetCaster());
                return true;
            }

            void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
            {
                // Set absorbtion amount to unlimited
                amount = -1;

            }

            void Absorb(AuraEffect* /*aurEff*/, DamageInfo & dmgInfo, uint32 & absorbAmount)
            {
                absorbAmount = CalculatePct(dmgInfo.GetDamage(), absorbPct);
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_shield_generator_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_shield_generator_AuraScript::Absorb, EFFECT_0);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_shield_generator_AuraScript();
        }
};

class spell_link_vehicle_passenger_energy : public SpellScriptLoader
{
    public:
        spell_link_vehicle_passenger_energy() : SpellScriptLoader("spell_link_vehicle_passenger_energy") { }

        class spell_link_vehicle_passenger_energy_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_link_vehicle_passenger_energy_SpellScript);

            void HandleOnCast()
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                uint32 spellid = GetSpellInfo()->Id;
                if (spellid != 62490 && spellid != 62345 && spellid != 62522)
                    return;

                int8 seatId = 1;
                Unit* target = NULL;

                if (Vehicle* veh = caster->GetVehicleKit())
                    target = veh->GetPassenger(seatId);

                if (target)
                    target->ModifyPower(Powers(GetSpellInfo()->PowerType), -int32(GetSpellInfo()->ManaCost));
            }

            void Register()
            {
                OnCast += SpellCastFn(spell_link_vehicle_passenger_energy_SpellScript::HandleOnCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_link_vehicle_passenger_energy_SpellScript();
        }
};

class ExcludeFlameleviathanSelector
{
    public:
        bool operator() (WorldObject* object)
        {
            Unit* unit = object->ToUnit();
            if (unit->GetEntry() == NPC_SEAT ||
                unit->GetEntry() == 33142    ||
                unit->GetEntry() == 33143)
                return true;

            if (unit->GetVehicleBase() && unit->GetVehicleBase()->GetEntry() == NPC_SEAT)
                return true;

            return false;
        }
};

class spell_exclude_flameleviathan_players : public SpellScriptLoader
{
    public:
        spell_exclude_flameleviathan_players() : SpellScriptLoader("spell_exclude_flameleviathan_players") { }

        class spell_exclude_flameleviathan_players_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_exclude_flameleviathan_players_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if(ExcludeFlameleviathanSelector());
            }

            void Register()
            {
                // TARGET_UNIT_DEST_AREA_ENEMY FlameVents BatteringRam
                // TARGET_UNIT_SRC_AREA_ENEMY MissleBarrage Pursued
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_exclude_flameleviathan_players_SpellScript::FilterTargets, EFFECT_ALL, TARGET_UNIT_DEST_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_exclude_flameleviathan_players_SpellScript::FilterTargets, EFFECT_ALL, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_exclude_flameleviathan_players_SpellScript();
        }
};

class ExcludeHPBuffTargets
{
    public:
        bool operator() (Unit* unit)
        {
            if (unit->GetEntry() == NPC_SEAT            ||
                unit->GetEntry() == 33142               || // Defense Turret
                unit->GetEntry() == NPC_MECHANOLIFT     ||
                unit->GetEntry() == NPC_LIQUID          ||
                unit->GetEntry() == 33218               || // Pyrit Safetycontainer
                unit->GetEntry() == NPC_WRITHING_LASHER ||
                unit->GetEntry() == NPC_WARD_OF_LIFE)
                return false;

            return true;
        }
};

class spell_exclude_flameleviathan_hp_buff : public SpellScriptLoader
{
    public:
        spell_exclude_flameleviathan_hp_buff() : SpellScriptLoader("spell_exclude_flameleviathan_hp_buff") { }

        class spell_exclude_flameleviathan_hp_buff_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_exclude_flameleviathan_hp_buff_AuraScript);

            bool CheckAreaTarget(Unit* target)
            {
                return ExcludeHPBuffTargets()(target);
            }

            void Register()
            {
                DoCheckAreaTarget += AuraCheckAreaTargetFn(spell_exclude_flameleviathan_hp_buff_AuraScript::CheckAreaTarget);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_exclude_flameleviathan_hp_buff_AuraScript();
        }
};

class spell_leviathan_flames : public SpellScriptLoader
{
    public:
        spell_leviathan_flames() : SpellScriptLoader("spell_leviathan_flames") { }

        class spell_leviathan_flames_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_leviathan_flames_SpellScript);

            void RemoveFreeze(SpellEffIndex /*effIndex*/)
            {
                if (GetHitUnit())
                    GetHitUnit()->RemoveAurasDueToSpell(SPELL_LEVIATHAN_FLAMES);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_leviathan_flames_SpellScript::RemoveFreeze, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_leviathan_flames_SpellScript();
        }
};

class achievement_orbital_bombardment : public AchievementCriteriaScript
{
    public:
        achievement_orbital_bombardment() : AchievementCriteriaScript("achievement_orbital_bombardment") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (!target)
                return false;

            if (Creature* Leviathan = target->ToCreature())
                if (Leviathan->AI()->GetData(DATA_ORBIT_ACHIEVEMENTS) >= 1)
                    return true;

            return false;
        }
};

class achievement_orbital_devastation : public AchievementCriteriaScript
{
    public:
        achievement_orbital_devastation() : AchievementCriteriaScript("achievement_orbital_devastation") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (!target)
                return false;

            if (Creature* Leviathan = target->ToCreature())
                if (Leviathan->AI()->GetData(DATA_ORBIT_ACHIEVEMENTS) >= 2)
                    return true;

            return false;
        }
};

class achievement_nuked_from_orbit : public AchievementCriteriaScript
{
    public:
        achievement_nuked_from_orbit() : AchievementCriteriaScript("achievement_nuked_from_orbit") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (!target)
                return false;

            if (Creature* Leviathan = target->ToCreature())
                if (Leviathan->AI()->GetData(DATA_ORBIT_ACHIEVEMENTS) >= 3)
                    return true;

            return false;
        }
};

class achievement_orbit_uary : public AchievementCriteriaScript
{
    public:
        achievement_orbit_uary() : AchievementCriteriaScript("achievement_orbit_uary") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (!target)
                return false;

            if (Creature* Leviathan = target->ToCreature())
                if (Leviathan->AI()->GetData(DATA_ORBIT_ACHIEVEMENTS) == 4)
                    return true;

            return false;
        }
};

class achievement_three_car_garage_demolisher : public AchievementCriteriaScript
{
    public:
        achievement_three_car_garage_demolisher() : AchievementCriteriaScript("achievement_three_car_garage_demolisher") { }

        bool OnCheck(Player* source, Unit* /*target*/)
        {
            if (Creature* vehicle = source->GetVehicleCreatureBase())
            {
                if (vehicle->GetEntry() == NPC_VEHICLE_DEMOLISHER || vehicle->GetEntry() == 33067)
                    return true;
            }

            return false;
        }
};

class achievement_three_car_garage_chopper : public AchievementCriteriaScript
{
    public:
        achievement_three_car_garage_chopper() : AchievementCriteriaScript("achievement_three_car_garage_chopper") { }

        bool OnCheck(Player* source, Unit* /*target*/)
        {
            if (Creature* vehicle = source->GetVehicleCreatureBase())
            {
                if (vehicle->GetEntry() == NPC_VEHICLE_CHOPPER)
                    return true;
            }

            return false;
        }
};

class achievement_three_car_garage_siege : public AchievementCriteriaScript
{
    public:
        achievement_three_car_garage_siege() : AchievementCriteriaScript("achievement_three_car_garage_siege") { }

        bool OnCheck(Player* source, Unit* /*target*/)
        {
            if (Creature* vehicle = source->GetVehicleCreatureBase())
            {
                if (vehicle->GetEntry() == NPC_VEHICLE_SIEGE || vehicle->GetEntry() == 33167)
                    return true;
            }

            return false;
        }
};

class achievement_shutout : public AchievementCriteriaScript
{
    public:
        achievement_shutout() : AchievementCriteriaScript("achievement_shutout") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (target)
                if (Creature* leviathan = target->ToCreature())
                    if (leviathan->AI()->GetData(DATA_SHUTOUT))
                        return true;

            return false;
        }
};

class achievement_unbroken : public AchievementCriteriaScript
{
    public:
        achievement_unbroken() : AchievementCriteriaScript("achievement_unbroken") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (target)
                if (Creature* leviathan = target->ToCreature())
                    if (leviathan->AI()->GetData(DATA_UNBROKEN))
                        return true;

            return false;
        }
};

void AddSC_boss_flame_leviathan()
{
    new boss_flame_leviathan();
    new boss_flame_leviathan_seat();
    new boss_flame_leviathan_defense_turret();
    new boss_flame_leviathan_defense_cannon();
    new boss_flame_leviathan_overload_device();
    new npc_flame_leviathan_safety_container();
    new npc_mechanolift();
    new npc_liquid_pyrite();
    new npc_ulduar_colossus();
    new npc_thorims_hammer();
    new npc_mimirons_inferno();
    new npc_hodirs_fury();
    new npc_freyas_ward();
    new spell_freyas_ward();
    new npc_lorekeeper();
    new npc_pool_of_tar();
    new npc_brann_bronzebeard();
    new npc_gauntlet_generator();
    //new npc_leviathan_player_vehicle();

    new go_ulduar_tower();
    //new at_RX_214_repair_o_matic_station();
    new go_RX_214_repair_o_matic_station();

    new achievement_three_car_garage_demolisher();
    new achievement_three_car_garage_chopper();
    new achievement_three_car_garage_siege();
    new achievement_shutout();
    new achievement_unbroken();
    new achievement_orbital_bombardment();
    new achievement_orbital_devastation();
    new achievement_nuked_from_orbit();
    new achievement_orbit_uary();

    new spell_anti_air_rocket();
    new spell_pursued();
    new spell_load_into_catapult();
    new spell_vehicle_throw_passenger();
    new spell_flame_leviathan_flame_vents();
    new spell_shield_generator();

    new spell_link_vehicle_passenger_energy();
    new spell_exclude_flameleviathan_players();
    new spell_exclude_flameleviathan_hp_buff();
    new spell_leviathan_flames();
}
