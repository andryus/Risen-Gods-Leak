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

#include "ulduar.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "Vehicle.h"
#include "Player.h"

// ###### Related Creatures/Object ######
enum NPCs
{
    NPC_ROCKET                                  = 34050,
    NPC_BURST_TARGET                            = 34211,
    NPC_JUNK_BOT                                = 33855,
    NPC_JUNK_BOT_TRIGGER                        = 1010586,
    NPC_ASSAULT_BOT                             = 34057,
    NPC_ASSAULT_BOT_TRIGGER                     = 1010585,
    NPC_BOOM_BOT                                = 33836,
    NPC_EMERGENCY_BOT                           = 34147,
    NPC_EMERGENCY_BOT_TRIGGER                   = 1010584,
    NPC_FLAMES_INITIAL                          = 34363,
    NPC_FLAME_SPREAD                            = 34121,
    NPC_FROST_BOMB                              = 34149,
    NPC_FROST_BOMB_DMG                          = 37186,
    NPC_MKII_TURRET                             = 34071,
    NPC_PROXIMITY_MINE                          = 34362,
    NPC_COMPUTER                                = 34143,
};

enum GameObjects
{
    GO_CACHE_OF_INNOVATION_10                      = 194789,
    GO_CACHE_OF_INNOVATION_HARDMODE_10             = 194957,
    GO_CACHE_OF_INNOVATION_25                      = 194956,
    GO_CACHE_OF_INNOVATION_HARDMODE_25             = 194958,
};

// ###### Texts ######
enum Yells
{
    // not used
    SAY_AGGRO                                   = 0,
    SAY_HARDMODE_ON                             = 1,
    SAY_MKII_ACTIVATE                           = 2,
    SAY_MKII_SLAY                               = 3,
    SAY_MKII_DEATH                              = 4,
    SAY_VX001_ACTIVATE                          = 5,
    SAY_VX001_SLAY                              = 6,
    SAY_VX001_DEATH                             = 7,
    SAY_AERIAL_ACTIVATE                         = 8,
    SAY_AERIAL_SLAY                             = 9,
    SAY_AERIAL_DEATH                            = 10,
    SAY_V07TRON_ACTIVATE                        = 11,
    SAY_V07TRON_SLAY                            = 12,
    SAY_V07TRON_DEATH                           = 13,
    SAY_BERSERK                                 = 14,
    SAY_YS_HELP                                 = 15,

    SAY_ENRAGE_INIT                             = 0,
    SAY_ENRAGE_10                               = 1,
    SAY_ENRAGE_9                                = 2,
    SAY_ENRAGE_8                                = 3,
    SAY_ENRAGE_7                                = 4,
    SAY_ENRAGE_6                                = 5,
    SAY_ENRAGE_5                                = 6,
    SAY_ENRAGE_4                                = 7,
    SAY_ENRAGE_3                                = 8,
    SAY_ENRAGE_2                                = 9,
    SAY_ENRAGE_1                                = 10,
    SAY_ENRAGE_0                                = 11,
    SAY_ENRAGE_KILL                             = 12,
};

#define EMOTE_LEVIATHAN                         "Leviathan MK II begins to cast Plasma Blast!"

// ###### Datas ######
enum Phases
{
    PHASE_NULL,
    PHASE_INTRO,
    PHASE_COMBAT,
    PHASE_MODULE_SOLO,
    PHASE_MODULE_ASSEMBLED,
    PHASE_VX001_ACTIVATION,
    PHASE_AERIAL_ACTIVATION,
    PHASE_V0L7R0N_ACTIVATION,
};

enum Actions
{
    ACTION_ACTIVATE_VX001 = 1,
    ACTION_ACTIVATE_AERIAL,
    ACTION_DISABLE_AERIAL,
    ACTION_ACTIVATE_V0L7R0N,
    ACTION_ENTER_ENRAGE,
    ACTION_ACTIVATE_HARD_MODE,
    ACTION_FAIL_SET_UP_US_THE_BOMB,
    ACTION_DELAY,
    ACTION_MODULE_REPAIRED,
    ACTION_MODULE_DIED,

    ACTION_START_SOLO,
    ACTION_START_ASSEMBLED,
    ACTION_DESPAWN,

    ACTION_STUN_DURING_LASER_BARRAGE,
    ACTION_REMOVE_STUN_AFTER_LASER_BARRAGE,
};

enum Datas
{
    DATA_GET_HARD_MODE = 1,
};

enum Achievements
{
    ACHIEVEMENT_SET_UP_US_THE_BOMB_10          = 2989,
    ACHIEVEMENT_SET_UP_US_THE_BOMB_25          = 3237,
};

// ###### Event Controlling ######
enum Events
{
    // MIMIRON
    EVENT_SPAWN_FLAMES = 1,
    EVENT_ENRAGE,
    EVENT_ENRAGE_EXPLOSION,
    EVENT_ENRAGE_COUNTDOWN,
    EVENT_TALKING_SEQUENCE,

    // Leviathan MK II
    EVENT_PROXIMITY_MINE,
    EVENT_NAPALM_SHELL,
    EVENT_PLASMA_BLAST,
    EVENT_PLASMA_BLAST_END,
    EVENT_SHOCK_BLAST,
    EVENT_FLAME_SUPPRESSANT,
    EVENT_EXTINGUISH_FLAMES,
    EVENT_CHECK_DIST,

    // VX-001
    EVENT_RAPID_BURST,
    EVENT_PRE_LASER_BARRAGE,
    EVENT_LASER_BARRAGE,
    EVENT_LASER_BARRAGE_END,
    EVENT_ROCKET_STRIKE,
    EVENT_HEAT_WAVE,
    EVENT_HAND_PULSE,
    EVENT_FROST_BOMB,
    EVENT_FLAME_SUPPRESSANT_VX001,

    // Aerial Command Unit
    EVENT_PLASMA_BALL,
    EVENT_REACTIVATE_AERIAL,
    EVENT_SUMMON_BOTS,
    EVENT_SUMMON_BOOM_BOT,

    // HARDMODE
    EVENT_FROST_BOMB_GROW,
    EVENT_FROST_BOMB_EXPLODE,
    EVENT_FLAME_SPREAD,
    EVENT_FLAME_MOVEMENT,
};

// ###### Spells ######
enum Spells
{
    // Leviathan MK II
    SPELL_MINES_SPAWN                           = 65347,
    SPELL_PROXIMITY_MINES                       = 63027,
    SPELL_PLASMA_BLAST                          = 62997,
    SPELL_SHOCK_BLAST                           = 63631,
    SPELL_EXPLOSION                             = 66351,
    SPELL_NAPALM_SHELL                          = 63666,
    SPELL_NAPALM_SHELL_25                       = 65026,
    SPELL_NAPALM_SHELL_TRIGGER                  = 64539,

    // VX-001
    SPELL_RAPID_BURST                           = 63382,
    SPELL_RAPID_BURST_LEFT_10                   = 63387,
    SPELL_RAPID_BURST_RIGHT_10                  = 64019,
    SPELL_RAPID_BURST_LEFT_25                   = 64531,
    SPELL_RAPID_BURST_RIGHT_25                  = 64532,
    SPELL_ROCKET_STRIKE_TRIGGER                 = 63681,
    SPELL_ROCKET_STRIKE                         = 63036,
    SPELL_ROCKET_STRIKE_AURA                    = 64064,
    SPELL_ROCKET_STRIKE_DMG                     = 63041,
    SPELL_SPINNING_UP                           = 63414,
    SPELL_LASER_BARRAGE                         = 63274,
    SPELL_HEAT_WAVE                             = 63677,
    SPELL_HAND_PULSE                            = 64348,

    // Aerial Command Unit
    SPELL_PLASMA_BALL                           = 63689,
    SPELL_MAGNETIC_CORE                         = 64436,
    SPELL_MAGNETIC_CORE_VISUAL                  = 64438,
    SPELL_BOOM_BOT_SUMMON                       = 63811,
    SPELL_BOOM_BOT                              = 63801,
    SPELL_MAGNETIC_FIELD                        = 64668,
    SPELL_HOVER                                 = 57764, // Set Hover position
    SPELL_BERSERK                               = 47008,

    // V0-L7R-0N
    SPELL_SELF_REPAIR                           = 64383,
    SPELL_HEAL_HALF                             = 64188,

    // Hard Mode
    SPELL_EMERGENCY_MODE                        = 64582,
    SPELL_FLAME_SUPPRESSANT                     = 64570,
    SPELL_CLEAR_FIRES                           = 65224,
    SPELL_FLAME_SUPPRESSANT_VX001               = 65192,
    SPELL_SUMMON_FLAMES_TRIGGER                 = 64567,
    SPELL_SUMMON_FLAMES_INITIAL                 = 64563,
    SPELL_SUMMON_FLAMES_SPREAD                  = 64564,
    SPELL_FLAME                                 = 64561,
    SPELL_FROST_BOMB_SELECT                     = 64623,
    SPELL_FROST_BOMB_SUMMON                     = 64627,
    SPELL_FROST_BOMB                            = 64624,
    SPELL_FROST_BOMB_EXPLOSION_10               = 64626,
    SPELL_FROST_BOMB_EXPLOSION_25               = 65333,
    SPELL_WATER_SPRAY                           = 64619,
    SPELL_SIREN                                 = 64616,

    // NPCs
    SPELL_BOMB_BOT                              = 63801, // should be 63767
    SPELL_NOT_SO_FRIENDLY_FIRE                  = 65040,

    // Bot summon Spells
    SPELL_SUMMON_SCARPBOT_BOT                   = 63819,
    SPELL_SUMMON_SCARPBOT_BOT_VISUAL            = 64398,
    SPELL_SUMMON_ASSAULT_BOT                    = 64427,
    SPELL_SUMMON_ASSAULT_BOT_VISUAL             = 64426,
    SPELL_SUMMON_FIRE_BOT                       = 64426,
    SPELL_SUMMON_FIRE_BOT_VISUAL                = 64621,
};

// ##### Additional Data #####
Position const SummonPos[9] =
{
    {2703.93f, 2569.32f, 364.397f, 0},
    {2715.33f, 2569.23f, 364.397f, 0},
    {2726.85f, 2569.28f, 364.397f, 0},
    {2765.24f, 2534.38f, 364.397f, 0},
    {2759.54f, 2544.30f, 364.397f, 0},
    {2753.82f, 2554.22f, 364.397f, 0},
    {2764.95f, 2604.11f, 364.397f, 0},
    {2759.19f, 2594.26f, 364.397f, 0},
    {2753.56f, 2584.30f, 364.397f, 0},
};

Position const PositionVX001Summon = {2744.65f, 2569.46f, 364.397f, 3.14159f};
Position const PositionAerialSummon = {2744.65f, 2569.46f, 380.0f, 3.14159f};
Position const PositionRoomCenter = {2744.65f, 2569.46f, 364.397f};

enum Misc
{
    FLAME_MAX_COUNT    = 95,
};

void DespawnCreatures(Creature* searcher, uint32 entry, float distance)
{
    std::list<Creature*> creatures;
    searcher->GetCreatureListWithEntryInGrid(creatures, entry, distance);

    for (std::list<Creature*>::const_iterator iter = creatures.begin(); iter != creatures.end(); ++iter)
        (*iter)->DespawnOrUnsummon();
}

void DespawnMinions(Creature* me)
{
    DespawnCreatures(me, NPC_FLAMES_INITIAL, 100.0f);
    DespawnCreatures(me, NPC_FLAME_SPREAD, 100.0f);
    DespawnCreatures(me, NPC_PROXIMITY_MINE, 100.0f);
    DespawnCreatures(me, NPC_ROCKET, 100.0f);
    DespawnCreatures(me, NPC_JUNK_BOT, 100.0f);
    DespawnCreatures(me, NPC_JUNK_BOT_TRIGGER, 100.0f);
    DespawnCreatures(me, NPC_ASSAULT_BOT, 100.0f);
    DespawnCreatures(me, NPC_ASSAULT_BOT_TRIGGER, 100.0f);
    DespawnCreatures(me, NPC_EMERGENCY_BOT, 100.0f);
    DespawnCreatures(me, NPC_EMERGENCY_BOT_TRIGGER, 100.0f);
    DespawnCreatures(me, NPC_BOOM_BOT, 100.0f);
}

class boss_mimiron : public CreatureScript
{
public:
    boss_mimiron() : CreatureScript("boss_mimiron") { }

    struct boss_mimironAI : public BossAI
    {
        boss_mimironAI(Creature* creature) : BossAI(creature, BOSS_MIMIRON)
        {
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_ROCKET_STRIKE_DMG, true);
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_FROST_BOMB_EXPLOSION_10, true);
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_FROST_BOMB_EXPLOSION_25, true);
            me->SetReactState(REACT_PASSIVE);
            ASSERT(instance);
        }

        void Reset()
        {
            if (me->getFaction() == 35)
                return;

            BossAI::Reset();

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_USE_STANDING);

            me->ExitVehicle();
            me->GetMotionMaster()->MoveTargetedHome();

            instance->SetData(DATA_MIMIRON_ELEVATOR, GO_STATE_ACTIVE);

            StartNewSequence(PHASE_NULL, 0);
            HardMode = false;
            Enraged = false;
            SetUsUpTheBomb = true;
            FlameCount = 0;
            DeadModuleCounter = 0;

            DespawnMinions(me);

            for (uint8 data = DATA_LEVIATHAN_MK_II; data <= DATA_AERIAL_UNIT; ++data)
                if (Creature* creature = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(data))))
                    if (creature->IsAlive())
                    {
                        creature->ExitVehicle();
                        creature->AI()->EnterEvadeMode();
                    }

            if (GameObject* go = me->FindNearestGameObject(GO_BIG_RED_BUTTON, 200.0f))
            {
                go->SetGoState(GO_STATE_READY);
                go->SetLootState(GO_JUST_DEACTIVATED);
                go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
            }
        }

        void EndEncounter()
        {
            if (!Enraged && HardMode)
                if (Creature* computer = ObjectAccessor::GetCreature(*me, ComputerGUID))
                    computer->AI()->Talk(SAY_ENRAGE_KILL);

            Talk(SAY_V07TRON_DEATH);

            me->setFaction(35);
            me->ExitVehicle();

            for (uint8 data = DATA_LEVIATHAN_MK_II; data <= DATA_AERIAL_UNIT; ++data)
                if (Creature* creature = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(data))))
                {
                    creature->ExitVehicle();
                    creature->DisappearAndDie();
                    creature->AI()->DoAction(ACTION_DESPAWN);
                }

            BossAI::JustDied(nullptr);

            if (HardMode)
                me->SummonGameObject(RAID_MODE(GO_CACHE_OF_INNOVATION_HARDMODE_10, GO_CACHE_OF_INNOVATION_HARDMODE_25), 2744.65f, 2569.46f, 364.314f, 3.14159f, 0, 0, 0.7f, 0.7f, 604800);
            else
                me->SummonGameObject(RAID_MODE(GO_CACHE_OF_INNOVATION_10, GO_CACHE_OF_INNOVATION_25), 2744.65f, 2569.46f, 364.314f, 3.14159f, 0, 0, 0.7f, 0.7f, 604800);

            if (Unit* mk11 = me->FindNearestCreature(NPC_LEVIATHAN_MKII, 100.0f, false))
            {
                for (Player& player : me->GetMap()->GetPlayers())
                    player.KilledMonsterCredit(NPC_LEVIATHAN_MKII, mk11->GetGUID());
            }

            if (SetUsUpTheBomb)
                instance->DoCompleteAchievement(RAID_MODE(ACHIEVEMENT_SET_UP_US_THE_BOMB_10, ACHIEVEMENT_SET_UP_US_THE_BOMB_25));

            EnterEvadeMode();
            me->DespawnOrUnsummon(5*IN_MILLISECONDS);
        }

        void EnterCombat(Unit * who)
        {
            BossAI::EnterCombat(who);

            if (instance)
                instance->SetData(DATA_MIMIRON_TELEPORTER, 1);

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
            phase = PHASE_INTRO;

            ExplosionCount = 0;
            ComputerGUID.Clear();
            MinutesRemaining = 10;

            if (HardMode)
            {
                if (Creature* computer = me->FindNearestCreature(NPC_COMPUTER, 100.0f))
                    ComputerGUID = computer->GetGUID();

                events.RescheduleEvent(EVENT_ENRAGE,           10*MINUTE*IN_MILLISECONDS);
                events.RescheduleEvent(EVENT_ENRAGE_COUNTDOWN,  1*MINUTE*IN_MILLISECONDS);
                events.RescheduleEvent(EVENT_SPAWN_FLAMES,      5*       IN_MILLISECONDS);
            }
            else
                events.RescheduleEvent(EVENT_ENRAGE, 15*MINUTE*IN_MILLISECONDS);

            JumpToNextStep(100);

            if (GameObject* go = me->FindNearestGameObject(GO_BIG_RED_BUTTON, 200.0f))
                go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventID = events.ExecuteEvent())
            {
                switch (eventID)
                {
                    case EVENT_ENRAGE:
                        if (HardMode)
                        {
                            if (Creature* computer = ObjectAccessor::GetCreature(*me, ComputerGUID))
                                computer->AI()->Talk(SAY_ENRAGE_0);
                        }
                        else
                            Talk(SAY_BERSERK);

                        for (uint8 data = DATA_LEVIATHAN_MK_II; data <= DATA_AERIAL_UNIT; ++data)
                            if (Creature* creature = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(data))))
                                creature->AI()->DoAction(ACTION_ENTER_ENRAGE);

                        Enraged = true;

                        if (HardMode)
                            events.ScheduleEvent(EVENT_ENRAGE_EXPLOSION, 1*IN_MILLISECONDS);
                        break;
                    case EVENT_SPAWN_FLAMES:
                        if (FlameCount <= FLAME_MAX_COUNT)
                        {
                            DoCast(SPELL_SUMMON_FLAMES_TRIGGER);
                            events.RescheduleEvent(EVENT_SPAWN_FLAMES, urand(27, 30)*IN_MILLISECONDS);
                        }
                        else
                            events.RescheduleEvent(EVENT_SPAWN_FLAMES, 5*IN_MILLISECONDS);
                        break;
                    case EVENT_ENRAGE_EXPLOSION:
                        if (++ExplosionCount <= RAID_MODE(10, 25)) // do soom pew pew
                        {
                            std::list<Player*> players;
                            me->GetPlayerListInGrid(players, 100.f);
                            if (players.empty())
                                break;
                            Player* player = Trinity::Containers::SelectRandomContainerElement(players);

                            if (player && !player->IsGameMaster())
                            {
                                me->CastSpell(player, SPELL_SUMMON_FLAMES_INITIAL, true);
                                if (Creature* flame = player->FindNearestCreature(NPC_FLAMES_INITIAL, 15.0f))
                                    flame->CastSpell((Unit*)NULL, SPELL_EXPLOSION, true);
                            }
                        }
                        else // enough pew pew, kill all those who survived
                        {
                            for (Player& player : me->GetMap()->GetPlayers())
                                if (player.IsAlive() && !player.IsGameMaster())
                                    me->Kill(&player);
                        }
                        events.RescheduleEvent(EVENT_ENRAGE_EXPLOSION, RAID_MODE(1000, 400));
                        break;
                    case EVENT_ENRAGE_COUNTDOWN:
                        if (--MinutesRemaining > 1)
                            events.RescheduleEvent(EVENT_ENRAGE_COUNTDOWN, 1*MINUTE*IN_MILLISECONDS);

                        if (Creature* computer = ObjectAccessor::GetCreature(*me, ComputerGUID))
                        {
                            switch (MinutesRemaining)
                            {
                                case 9: 
                                    computer->AI()->Talk(SAY_ENRAGE_9); 
                                    break;
                                case 8: 
                                    computer->AI()->Talk(SAY_ENRAGE_8);
                                    break;
                                case 7: 
                                    computer->AI()->Talk(SAY_ENRAGE_7);
                                    break;
                                case 6: 
                                    computer->AI()->Talk(SAY_ENRAGE_6);
                                    break;
                                case 5: 
                                    computer->AI()->Talk(SAY_ENRAGE_5);
                                    break;
                                case 4: 
                                    computer->AI()->Talk(SAY_ENRAGE_4);
                                    break;
                                case 3: 
                                    computer->AI()->Talk(SAY_ENRAGE_3);
                                    break;
                                case 2: 
                                    computer->AI()->Talk(SAY_ENRAGE_2);
                                    break;
                                case 1: 
                                    computer->AI()->Talk(SAY_ENRAGE_1);
                                    break;
                            }
                        }
                        break;
                    case EVENT_TALKING_SEQUENCE:
                        HandleTalkingSequence();
                        break;
                }
            }
        }

        void StartNewSequence(Phases ph, uint32 timer)
        {
            phase = ph;
            TalkingStep = 0;
            events.CancelEvent(EVENT_TALKING_SEQUENCE);
            if (timer)
                JumpToNextStep(timer);
        }

        void JumpToNextStep(uint32 timer)
        {
            events.RescheduleEvent(EVENT_TALKING_SEQUENCE, timer);
            ++TalkingStep;
        }

        void HandleTalkingSequence()
        {
            if (phase == PHASE_INTRO)
            {
                switch (TalkingStep)
                {
                    case 1:
                        if (HardMode)
                        {
                            if (Creature* computer = ObjectAccessor::GetCreature(*me, ComputerGUID))
                                computer->AI()->Talk(SAY_ENRAGE_INIT);
                        }
                        else
                            Talk(SAY_AGGRO);
                        JumpToNextStep(1500);
                        break;
                    case 2:
                        if (HardMode)
                            if (Creature* computer = ObjectAccessor::GetCreature(*me, ComputerGUID))
                                computer->AI()->Talk(SAY_ENRAGE_10);
                        JumpToNextStep(1500);
                        break;
                    case 3:
                        if (HardMode)
                            Talk(SAY_HARDMODE_ON);
                        JumpToNextStep(1*IN_MILLISECONDS);
                        break;
                    case 4:
                        if (Creature* Leviathan = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_LEVIATHAN_MK_II))))
                            me->EnterVehicle(Leviathan, 4);
                        JumpToNextStep(1*IN_MILLISECONDS);
                        break;
                    case 5:
                        me->ChangeSeat(2);
                        JumpToNextStep(1*IN_MILLISECONDS);
                        break;
                    case 6:
                        me->ChangeSeat(5);
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_STAND);
                        JumpToNextStep(1000);
                        break;
                    case 7:
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_TALK);
                        JumpToNextStep(2*IN_MILLISECONDS);
                        break;
                    case 8:
                        me->ChangeSeat(6);
                        JumpToNextStep(1*IN_MILLISECONDS);
                        break;
                    case 9:
                        if (Creature* Leviathan = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_LEVIATHAN_MK_II))))
                        {
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_STAND);
                            Leviathan->AI()->DoAction(ACTION_START_SOLO);
                            phase = PHASE_COMBAT;
                        }
                        break;
                }
            }
            else if (phase == PHASE_VX001_ACTIVATION)
            {
                switch (TalkingStep)
                {
                    case 1:
                        Talk(SAY_MKII_DEATH);
                        JumpToNextStep(10*IN_MILLISECONDS);
                        break;
                    case 2:
                        me->ChangeSeat(1);
                        JumpToNextStep(2*IN_MILLISECONDS);
                        break;
                    case 3:
                        instance->SetData(DATA_MIMIRON_ELEVATOR, GO_STATE_READY);
                        JumpToNextStep(5*IN_MILLISECONDS);
                        break;
                    case 4:
                        if (Creature* VX_001 = me->SummonCreature(NPC_VX_001, PositionVX001Summon, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10*IN_MILLISECONDS))
                        {
                            instance->SetData(DATA_MIMIRON_ELEVATOR, GO_STATE_ACTIVE_ALTERNATIVE);
                            VX_001->SetVisible(true);
                            for (uint8 i = 5; i < 7; ++i)
                                if (Creature* rocket = VX_001->SummonCreature(NPC_ROCKET, *VX_001))
                                {
                                    rocket->EnterVehicle(VX_001, i);
                                    rocket->SetReactState(REACT_PASSIVE);
                                }
                        }
                        JumpToNextStep(8*IN_MILLISECONDS);
                        break;
                    case 5:
                        me->ExitVehicle();
                        if (Creature* VX_001 = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_VX_001))))
                            me->EnterVehicle(VX_001, 0);
                        JumpToNextStep(3500);
                        break;
                    case 6:
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_TALK);
                        Talk(SAY_VX001_ACTIVATE);
                        JumpToNextStep(10*IN_MILLISECONDS);
                        break;
                    case 7:
                        me->ChangeSeat(1);
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_SIT);
                        JumpToNextStep(2*IN_MILLISECONDS);
                        break;
                    case 8:
                        if (Creature* VX_001 = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_VX_001))))
                            VX_001->HandleEmoteCommand(EMOTE_ONESHOT_EMERGE);
                        JumpToNextStep(3500);
                        break;
                    case 9:
                        if (Creature* VX_001 = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_VX_001))))
                        {
                            VX_001->AddAura(SPELL_HOVER, VX_001); // Hover
                            VX_001->AI()->DoAction(ACTION_START_SOLO);
                            phase = PHASE_COMBAT;
                        }
                        break;
                }
            }
            else if (phase == PHASE_AERIAL_ACTIVATION)
            {
                switch (TalkingStep)
                {
                    case 1:
                        me->ChangeSeat(4);
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_STAND);
                        JumpToNextStep(2500);
                        break;
                    case 2:
                        Talk(SAY_VX001_DEATH);
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_TALK);
                        JumpToNextStep(5*IN_MILLISECONDS);
                        break;
                    case 3:
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_STAND);
                        if (Creature* AerialUnit = me->SummonCreature(NPC_AERIAL_COMMAND_UNIT, PositionAerialSummon, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10*IN_MILLISECONDS))
                            AerialUnit->SetVisible(true);
                        JumpToNextStep(5*IN_MILLISECONDS);
                        break;
                    case 4:
                        me->ExitVehicle();
                        if (Creature* AerialUnit = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_AERIAL_UNIT))))
                            me->EnterVehicle(AerialUnit, 0);
                        JumpToNextStep(2*IN_MILLISECONDS);
                        break;
                    case 5:
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_TALK);
                        Talk(SAY_AERIAL_ACTIVATE);
                        JumpToNextStep(8*IN_MILLISECONDS);
                        break;
                    case 6:
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_STAND);
                        if (Creature* AerialUnit = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_AERIAL_UNIT))))
                        {
                            AerialUnit->AI()->DoAction(ACTION_START_SOLO);
                            phase = PHASE_COMBAT;
                        }
                        break;
                }
            }
            else if (phase == PHASE_V0L7R0N_ACTIVATION)
            {
                switch (TalkingStep)
                {
                    case 1:
                        Talk(SAY_AERIAL_DEATH);
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_STAND);

                        if (Creature* Leviathan = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_LEVIATHAN_MK_II))))
                            Leviathan->GetMotionMaster()->MovePoint(0, PositionRoomCenter);

                        me->ExitVehicle();

                        JumpToNextStep(5*IN_MILLISECONDS);
                        break;
                    case 2:
                        if (Creature* VX_001 = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_VX_001))))
                        {
                            VX_001->SetStandState(UNIT_STAND_STATE_STAND);
                            VX_001->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_CUSTOM_SPELL_01);

                            if (Creature* Leviathan = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_LEVIATHAN_MK_II))))
                                VX_001->EnterVehicle(Leviathan, 7);
                        }
                        JumpToNextStep(2*IN_MILLISECONDS);
                        break;
                    case 3:
                        Talk(SAY_V07TRON_ACTIVATE);
                        if (Creature* VX_001 = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_VX_001))))
                            if (Creature* AerialUnit = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_AERIAL_UNIT))))
                            {
                                AerialUnit->SetCanFly(false);
                                AerialUnit->EnterVehicle(VX_001, 3);
                            }
                        JumpToNextStep(10*IN_MILLISECONDS);
                        break;
                    case 4:
                        if (Creature* Leviathan = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_LEVIATHAN_MK_II))))
                            Leviathan->AI()->DoAction(ACTION_START_ASSEMBLED);
                        if (Creature* VX_001 = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_VX_001)))){
                            VX_001->AI()->DoAction(ACTION_START_ASSEMBLED);
                            me->EnterVehicle(VX_001, 1);
                        }
                        if (Creature* AerialUnit = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_AERIAL_UNIT))))
                            AerialUnit->AI()->DoAction(ACTION_START_ASSEMBLED);
                        phase = PHASE_COMBAT;
                        break;
                }
            }
        }

        uint32 GetData(uint32 type) const
        {
            switch (type)
            {
                case DATA_GET_HARD_MODE:
                    return HardMode ? 1 : 0;
                case DATA_FLAME_COUNT:
                    return FlameCount;
            }
            return 0;
        }

        void DoAction(int32 action)
        {
            switch (action)
            {
                case ACTION_ACTIVATE_VX001:
                    StartNewSequence(PHASE_VX001_ACTIVATION, 100);
                    break;
                case ACTION_ACTIVATE_AERIAL:
                    StartNewSequence(PHASE_AERIAL_ACTIVATION, 5*IN_MILLISECONDS);
                    break;
                case ACTION_ACTIVATE_V0L7R0N:
                    me->SetVisible(true);
                    StartNewSequence(PHASE_V0L7R0N_ACTIVATION, 1*IN_MILLISECONDS);
                    break;
                case ACTION_ACTIVATE_HARD_MODE:
                    HardMode = true;
                    DoZoneInCombat(me, 80.0f);
                    break;
                case ACTION_FAIL_SET_UP_US_THE_BOMB:
                    SetUsUpTheBomb = false;
                    break;
                case ACTION_MODULE_DIED:
                    if (++DeadModuleCounter >= 3)
                        EndEncounter();
                    break;
                case ACTION_MODULE_REPAIRED:
                    if (DeadModuleCounter > 0)
                        --DeadModuleCounter;
                    break;
            }
        }

        void JustSummoned(Creature* creature)
        {
            BossAI::JustSummoned(creature);

            if (creature->GetEntry() != NPC_FLAME_SPREAD)
                return;

            ++FlameCount;
        }

        void SummonedCreatureDies(Creature* creature, Unit* /*killer*/)
        {
            if (creature->GetEntry() != NPC_FLAME_SPREAD && creature->GetEntry() != NPC_FLAMES_INITIAL)
                return;

            --FlameCount;
        }

        void SummonedCreatureDespawn(Creature* summon)
        {
            BossAI::SummonedCreatureDespawn(summon);

            if (summon->GetEntry() != NPC_FLAME_SPREAD && summon->GetEntry() != NPC_FLAMES_INITIAL)
                return;

            if (FlameCount > 0)
                --FlameCount;
        }

        private:
            Phases phase;
            uint32 TalkingStep;
            uint8 DeadModuleCounter;
            bool SetUsUpTheBomb;

            // Enrage
            bool Enraged;
            uint8 ExplosionCount;

            // Hardmode
            bool HardMode;
            ObjectGuid ComputerGUID;
            uint8 MinutesRemaining;
            uint16 FlameCount;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_mimironAI(creature);
    }
};

struct ModuleAI : public BossAI
{
    ModuleAI(Creature* creature) : BossAI(creature, BOSS_MIMIRON)
    {
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
        me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_ROCKET_STRIKE_DMG, true);
        me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_FROST_BOMB_EXPLOSION_10, true);
        me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_FROST_BOMB_EXPLOSION_25, true);
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
    }

    void EnterEvadeMode(EvadeReason /*why*/)
    {
        if (Creature* mimiron = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_MIMIRON))))
            mimiron->AI()->EnterEvadeMode();
    }

    virtual void Reset()
    {
        events.Reset();
        events.SetPhase(PHASE_NULL);
        summons.DespawnAll();

        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
        me->SetStandState(UNIT_STAND_STATE_STAND);
        me->SetReactState(REACT_PASSIVE);
        me->RemoveAllAurasExceptType(SPELL_AURA_CONTROL_VEHICLE);

        isStunnedDuringLaserBarrage = false;
        HardMode = false;
    }

    void KilledUnit(Unit* /*who*/)
    {
        if (!urand(0, 5))
            if (Creature* mimiron = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_MIMIRON))))
            {
                if (events.IsInPhase(PHASE_MODULE_SOLO))
                    mimiron->AI()->Talk(GetSoloKillSay());
                else
                    mimiron->AI()->Talk(SAY_V07TRON_SLAY);
            }
    }

    void DamageTaken(Unit* /*who*/, uint32 &damage)
    {
        if (!events.IsInPhase(PHASE_MODULE_SOLO) && ! events.IsInPhase(PHASE_MODULE_ASSEMBLED))
        {
            damage = 0;
            return;
        }

        if (damage >= me->GetHealth())
        {
            damage = 0;

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);

            me->InterruptNonMeleeSpells(true);
            me->AttackStop();
            me->SetReactState(REACT_PASSIVE);
            me->RemoveAllAurasExceptType(SPELL_AURA_CONTROL_VEHICLE);

            if (events.IsInPhase(PHASE_MODULE_SOLO))
            {
                HandleJustDiedSolo();
            }
            else
            {
                me->SetStandState(UNIT_STAND_STATE_DEAD);

                DoCast(SPELL_SELF_REPAIR);

                if (Creature* mimiron = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_MIMIRON))))
                    mimiron->AI()->DoAction(ACTION_MODULE_DIED);

                HandleJustDiedAssembled();
            }

            events.SetPhase(PHASE_NULL);
            events.Reset();
        }
    }

    void SpellHit(Unit* /*caster*/, SpellInfo const* spell)
    {
        if (spell->Id == SPELL_SELF_REPAIR)
        {
            if (Creature* mimiron = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_MIMIRON))))
                mimiron->AI()->DoAction(ACTION_MODULE_REPAIRED);

            me->SetReactState(REACT_AGGRESSIVE);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);

            events.SetPhase(PHASE_MODULE_ASSEMBLED);

            if (HardMode)
                DoCastSelf(SPELL_EMERGENCY_MODE, true);

            HandleStartAssembled();
        }
    }

    void EnterCombat(Unit* /*who*/)
    {
        if (Creature* mimiron = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_MIMIRON))))
            HardMode = mimiron->AI()->GetData(DATA_GET_HARD_MODE);

        if (HardMode)
            DoCastSelf(SPELL_EMERGENCY_MODE, true);
    }

    virtual void DoAction(int32 action)
    {
        switch (action)
        {
            case ACTION_START_SOLO:
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                me->SetImmuneToPC(false);
                me->SetReactState(REACT_AGGRESSIVE);
                events.SetPhase(PHASE_MODULE_SOLO);
                me->SetInCombatWithZone();
                isStunnedDuringLaserBarrage = false;
                HandleStartSolo();

                break;
            case ACTION_START_ASSEMBLED:
                if (HardMode)
                    DoCastSelf(SPELL_EMERGENCY_MODE, true);

                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                me->SetReactState(REACT_AGGRESSIVE);
                me->SetInCombatWithZone();
                me->Attack(me->GetVictim(), false);

                isStunnedDuringLaserBarrage = false;

                DoCastSelf(SPELL_HEAL_HALF, true);

                me->SetStandState(UNIT_STAND_STATE_STAND);

                events.Reset();
                events.SetPhase(PHASE_MODULE_ASSEMBLED);
                HandleStartAssembled();
                break;
            case ACTION_STUN_DURING_LASER_BARRAGE:
                isStunnedDuringLaserBarrage = true;
                me->SetReactState(REACT_PASSIVE);
                me->AttackStop();
                break;
            case ACTION_REMOVE_STUN_AFTER_LASER_BARRAGE:
                isStunnedDuringLaserBarrage = false;
                me->SetReactState(REACT_AGGRESSIVE);
                me->AI()->AttackStart(me->GetVictim());
                break;
            case ACTION_ENTER_ENRAGE:
                DoCastSelf(SPELL_BERSERK, true);
                break;
            case ACTION_DESPAWN:
                summons.DespawnAll();
                break;
        }
    }

    virtual uint32 GetSoloKillSay() const = 0;
    virtual void HandleJustDiedSolo() {};
    virtual void HandleJustDiedAssembled() {};
    virtual void HandleStartAssembled() {};
    virtual void HandleStartSolo() {};

protected:
    bool HardMode;
    bool isStunnedDuringLaserBarrage;
};

class boss_leviathan_mk : public CreatureScript
{
public:
    boss_leviathan_mk() : CreatureScript("boss_leviathan_mk") { }

    struct boss_leviathan_mkAI : public ModuleAI
    {
        boss_leviathan_mkAI(Creature* creature) : ModuleAI(creature), vehicle(creature->GetVehicleKit()) {
        }

        void PassengerBoarded(Unit* passenger, int8 /*seatId*/, bool apply)
        {
            if (apply && passenger->GetEntry() == NPC_MKII_TURRET)
                passenger->ToCreature()->SetReactState(REACT_PASSIVE);
        }

        uint32 GetSoloKillSay() const
        {
            return SAY_MKII_SLAY;
        }

        void HandleJustDiedSolo()
        {
            if (Creature* Mimiron = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_MIMIRON))))
                Mimiron->AI()->DoAction(ACTION_ACTIVATE_VX001);

            if (Unit* turret = vehicle->GetPassenger(3))
            {
                turret->ExitVehicle();
                me->Kill(turret, false);
            }

            me->SetSpeedRate(MOVE_RUN, 1.5f);
            me->GetMotionMaster()->MovePoint(0, 2790.11f, 2595.83f, 364.32f);
        }

        void HandleStartSolo()
        {
            events.ScheduleEvent(EVENT_PROXIMITY_MINE,  1*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_PLASMA_BLAST,   10*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_NAPALM_SHELL,   urand(8, 10)*IN_MILLISECONDS);
            if (HardMode)
                events.ScheduleEvent(EVENT_FLAME_SUPPRESSANT, 1*MINUTE*IN_MILLISECONDS);
            canMeleeAttack = true;
        }

        void HandleStartAssembled()
        {
            me->SetSpeedRate(MOVE_RUN, 1.0f);

            events.RescheduleEvent(EVENT_PROXIMITY_MINE, 20*IN_MILLISECONDS);
            events.RescheduleEvent(EVENT_SHOCK_BLAST, 37*IN_MILLISECONDS);
            canMeleeAttack = true;
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (isStunnedDuringLaserBarrage)
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING) || me->IsNonMeleeSpellCast(true))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_PROXIMITY_MINE:
                        DoCast(SPELL_PROXIMITY_MINES);
                        if (events.IsInPhase(PHASE_MODULE_SOLO))
                            events.RescheduleEvent(EVENT_PROXIMITY_MINE, 35*IN_MILLISECONDS);
                        else
                            events.RescheduleEvent(EVENT_PROXIMITY_MINE, 32500);
                        return;
                    case EVENT_PLASMA_BLAST:
                        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                        me->MonsterTextEmote(EMOTE_LEVIATHAN, 0, true);
                        DoCastVictim(SPELL_PLASMA_BLAST);
                        canMeleeAttack = false;
                        events.RescheduleEvent(EVENT_PLASMA_BLAST, urand(30*IN_MILLISECONDS, 35*IN_MILLISECONDS), 0, PHASE_MODULE_SOLO);
                        events.RescheduleEvent(EVENT_SHOCK_BLAST, 9*IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_PLASMA_BLAST_END, 9 * IN_MILLISECONDS);
                        return;
                    case EVENT_SHOCK_BLAST:
                        canMeleeAttack = true;
                        DoCastAOE(SPELL_SHOCK_BLAST);
                        // in PHASE_MODULE_SOLO, Shock Blasts is Rescheduled by EVENT_PLASMA_BLAST
                        if (events.IsInPhase(PHASE_MODULE_ASSEMBLED))
                            events.RescheduleEvent(EVENT_SHOCK_BLAST, 23*IN_MILLISECONDS);
                        return;
                    case EVENT_FLAME_SUPPRESSANT:
                        DoCastAOE(SPELL_FLAME_SUPPRESSANT, true);
                        return;
                    case EVENT_NAPALM_SHELL:
                        DoCast(SPELL_NAPALM_SHELL_TRIGGER);
                        events.ScheduleEvent(EVENT_NAPALM_SHELL, urand(8, 10)*IN_MILLISECONDS, 0, PHASE_MODULE_SOLO);
                        return;
                    case EVENT_PLASMA_BLAST_END:
                        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
                        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
                        return;
                }
            }

            if (canMeleeAttack)
                DoMeleeAttackIfReady();
        }

        private:
            Vehicle* vehicle;
            bool canMeleeAttack;

    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_leviathan_mkAI(creature);
    }
};

class boss_vx_001 : public CreatureScript
{
public:
    boss_vx_001() : CreatureScript("boss_vx_001") { }

    struct boss_vx_001AI : public ModuleAI
    {
        boss_vx_001AI(Creature* creature) : ModuleAI(creature), vehicle(me->GetVehicleKit()) {
        }

        void Reset()
        {
            ModuleAI::Reset();

            me->SetVisible(false);
            me->SetReactState(REACT_PASSIVE);

            Spinning = false;
            SpinTimer = 250;
        }

        uint32 GetSoloKillSay() const
        {
            return SAY_VX001_SLAY;
        }

        void HandleStartSolo()
        {
            if (HardMode)
            {
                events.ScheduleEvent(EVENT_FROST_BOMB,              15*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FLAME_SUPPRESSANT_VX001,  1*IN_MILLISECONDS);
            }

            events.ScheduleEvent(EVENT_RAPID_BURST,        2500, 0, PHASE_MODULE_SOLO);
            events.ScheduleEvent(EVENT_PRE_LASER_BARRAGE, urand(35, 40)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_ROCKET_STRIKE,     20*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_HEAT_WAVE,         urand(8, 10)*IN_MILLISECONDS);

            me->SetReactState(REACT_PASSIVE);
            me->AttackStop();
        }

        void HandleStartAssembled()
        {
            if (HardMode) {
                events.ScheduleEvent(EVENT_FROST_BOMB,              15*IN_MILLISECONDS);
            }

            events.RescheduleEvent(EVENT_PRE_LASER_BARRAGE, 30*IN_MILLISECONDS);
            events.RescheduleEvent(EVENT_ROCKET_STRIKE,     20*IN_MILLISECONDS);
            events.RescheduleEvent(EVENT_HAND_PULSE,        10*IN_MILLISECONDS);

            me->SetReactState(REACT_AGGRESSIVE);
        }

        void HandleJustDiedSolo()
        {
            if (Creature* mimiron = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_MIMIRON))))
                mimiron->AI()->DoAction(ACTION_ACTIVATE_AERIAL);

            Spinning = false;
            me->GetMotionMaster()->Initialize();
        }
        void HandleJustDiedAssembled()
        {
            Spinning = false;
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (Spinning)
            {
                if (SpinTimer <= diff)
                {
                    float orient = me->GetOrientation() - M_PI / 60;
                    float x, y, z;
                    z = me->GetPositionZ() + 5.0f;
                    me->GetNearPoint2D(x, y, 40.0f, orient);
                    if (Creature* temp = me->SummonCreature(NPC_BURST_TARGET, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 250))
                    {
                        if (Creature* leviathan = me->GetVehicleCreatureBase())
                            leviathan->SetFacingToObject(temp);
                        else
                            me->SetFacingToObject(temp);
                    }

                    SpinTimer = 250;
                }
                else
                    SpinTimer -= diff;
            }

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING) || me->IsNonMeleeSpellCast(true))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_RAPID_BURST:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                            DoCast(target, SPELL_RAPID_BURST);
                        events.ScheduleEvent(EVENT_RAPID_BURST, 5*IN_MILLISECONDS, 0, PHASE_MODULE_SOLO);
                        return;
                    case EVENT_PRE_LASER_BARRAGE:
                    {
                        me->SetReactState(REACT_PASSIVE);
                        me->AttackStop();
                        me->RemoveAurasDueToSpell(SPELL_RAPID_BURST);

                        float orient = float(2*M_PI * frand(0.0f, 1.0f));
                        float x, y, z;
                        z = me->GetPositionZ() + 5.0f;
                        me->GetNearPoint2D(x, y, 40.0f, orient);

                        if (Creature* temp = me->SummonCreature(NPC_BURST_TARGET, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 250))
                        {
                            if (Creature* leviathan = me->GetVehicleCreatureBase())
                                leviathan->SetFacingToObject(temp);
                            else
                                me->SetFacingToObject(temp);
                        }

                        // stun leviathan during laser barrage
                        if (events.IsInPhase(PHASE_MODULE_ASSEMBLED)) {
                            if (Creature* leviathan = me->GetVehicleCreatureBase()) {
                                leviathan->AI()->DoAction(ACTION_STUN_DURING_LASER_BARRAGE);
                            }
                        }

                        Spinning = true;
                        DoCast(SPELL_SPINNING_UP);

                        events.DelayEvents(14500);
                        events.ScheduleEvent(EVENT_PRE_LASER_BARRAGE, 37*IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_LASER_BARRAGE_END, 14500);
                        return;
                    }
                    case EVENT_LASER_BARRAGE_END:
                        if (events.IsInPhase(PHASE_MODULE_ASSEMBLED)) {
                            me->SetReactState(REACT_AGGRESSIVE);
                            // remove stun on leviathan
                            if (Creature* leviathan = me->GetVehicleCreatureBase()) {
                                leviathan->AI()->DoAction(ACTION_REMOVE_STUN_AFTER_LASER_BARRAGE);
                            }
                        }
                        Spinning = false;
                        return;
                    case EVENT_ROCKET_STRIKE:
                        me->CastCustomSpell(SPELL_ROCKET_STRIKE_TRIGGER, SPELLVALUE_MAX_TARGETS,
                            (events.IsInPhase(PHASE_MODULE_ASSEMBLED)) ? 2: 1);
                        events.ScheduleEvent(EVENT_ROCKET_STRIKE, urand(20*IN_MILLISECONDS, 25*IN_MILLISECONDS));
                        return;
                    case EVENT_HEAT_WAVE:
                        DoCastAOE(SPELL_HEAT_WAVE);
                        events.ScheduleEvent(EVENT_HEAT_WAVE, 10*IN_MILLISECONDS, 0, PHASE_MODULE_SOLO);
                        return;
                    case EVENT_HAND_PULSE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                            DoCast(target, SPELL_HAND_PULSE);
                        events.ScheduleEvent(EVENT_HAND_PULSE, urand(3*IN_MILLISECONDS, 4*IN_MILLISECONDS));
                        return;
                    case EVENT_FROST_BOMB:
                        DoCast(SPELL_FROST_BOMB_SELECT);
                        events.ScheduleEvent(EVENT_FROST_BOMB, 45*IN_MILLISECONDS);
                        return;
                    case EVENT_FLAME_SUPPRESSANT_VX001:
                        DoCastAOE(SPELL_FLAME_SUPPRESSANT_VX001);
                        events.ScheduleEvent(EVENT_FLAME_SUPPRESSANT_VX001, 10*IN_MILLISECONDS, 0, PHASE_MODULE_SOLO);
                        return;
                }
            }
        }

        private:
            bool Spinning;
            uint32 SpinTimer;
            Vehicle* vehicle;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_vx_001AI(creature);
    }
};

class boss_aerial_unit : public CreatureScript
{
public:
    boss_aerial_unit() : CreatureScript("boss_aerial_unit") { }

    struct boss_aerial_unitAI : public ModuleAI
    {
        boss_aerial_unitAI(Creature* creature) : ModuleAI(creature)
        {
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
        }

        uint8 SpawnedAdds;

        void Reset()
        {
            ModuleAI::Reset();

            me->SetVisible(false);

            SpawnedAdds = 1;
        }

        uint32 GetSoloKillSay() const
        {
            return SAY_AERIAL_SLAY;
        }

        void HandleJustDiedSolo()
        {
            if (Creature* mimiron = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_MIMIRON))))
                mimiron->AI()->DoAction(ACTION_ACTIVATE_V0L7R0N);

            me->GetMotionMaster()->Clear(true);
        }
        void HandleJustDiedAssembled() {}
        void HandleStartAssembled()
        {
            events.RescheduleEvent(EVENT_PLASMA_BALL, 2*IN_MILLISECONDS);
        }

        void HandleStartSolo()
        {
            events.ScheduleEvent(EVENT_PLASMA_BALL,      1*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_SUMMON_BOTS,     10*IN_MILLISECONDS, 0, PHASE_MODULE_SOLO);
            events.ScheduleEvent(EVENT_SUMMON_BOOM_BOT, 12*IN_MILLISECONDS, 0, PHASE_MODULE_SOLO);
        }

        void DoAction(int32 action)
        {
            switch (action)
            {
                case ACTION_DISABLE_AERIAL:
                    if (events.IsInPhase(PHASE_MODULE_ASSEMBLED))
                        return;
                    me->CastStop();
                    me->SetReactState(REACT_PASSIVE);
                    me->GetMotionMaster()->Clear(true);
                    DoCastSelf(SPELL_MAGNETIC_CORE);
                    DoCastSelf(SPELL_MAGNETIC_CORE_VISUAL);
                    if (Creature* magneticCore = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_MAGNETIC_CORE))))
                        if (magneticCore->IsAlive())
                            me->NearTeleportTo(magneticCore->GetPositionX(), magneticCore->GetPositionY(), 368.965f, 0, false);
                    events.RescheduleEvent(EVENT_PLASMA_BALL, 22*IN_MILLISECONDS, 0, PHASE_MODULE_SOLO);
                    events.RescheduleEvent(EVENT_SUMMON_BOTS, 24*IN_MILLISECONDS, 0, PHASE_MODULE_SOLO);
                    events.RescheduleEvent(EVENT_SUMMON_BOOM_BOT, 25*IN_MILLISECONDS, 0, PHASE_MODULE_SOLO);
                    events.ScheduleEvent(EVENT_REACTIVATE_AERIAL, 20*IN_MILLISECONDS, 0, PHASE_MODULE_SOLO);
                    break;
                default:
                    ModuleAI::DoAction(action);
                    break;
            }
        }

        //! These "workarounds" are needed because ChaseMovementGenerators, forces the Aerial Unit to land when victim is out of range.
        bool MoveIfNeeded()
        {
            if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
                return true;

            float dist = me->GetDistance(me->GetVictim());

            if (dist < 30.0f)
                return false;

            float x, y, z;
            me->GetNearPoint2D(x, y, dist - 38.0f, me->GetAngle(me->GetVictim()));
            z = me->GetPositionZ();

            me->GetMotionMaster()->MovePoint(0, x, y, z);
            return true;
        }

        void AttackStart(Unit* victim)
        {
            me->Attack(victim, false);
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
                    case EVENT_PLASMA_BALL:
                        if (events.IsInPhase(PHASE_MODULE_SOLO) && me->GetVictim())
                        {
                            if (!MoveIfNeeded())
                                DoCastVictim(SPELL_PLASMA_BALL);
                        }
                        else if (events.IsInPhase(PHASE_MODULE_ASSEMBLED) && me->GetVictim())
                        {
                            if (me->IsWithinDist3d(me->GetVictim(), 30.0f))
                                DoCastVictim(SPELL_PLASMA_BALL);
                            else if (Unit* target = SelectTarget(SELECT_TARGET_MINDISTANCE, 0))
                                DoCast(target, SPELL_PLASMA_BALL);
                        }
                        events.ScheduleEvent(EVENT_PLASMA_BALL, 2*IN_MILLISECONDS);
                        break;
                    case EVENT_REACTIVATE_AERIAL:
                        me->RemoveAurasDueToSpell(SPELL_MAGNETIC_CORE_VISUAL);
                        me->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), 380.04f, M_PI_F, false);
                        me->SetReactState(REACT_AGGRESSIVE);
                        break;
                    case EVENT_SUMMON_BOTS:
                        SpawnAdds();
                        events.ScheduleEvent(EVENT_SUMMON_BOTS, 10*IN_MILLISECONDS, 0, PHASE_MODULE_SOLO);
                        break;
                    case EVENT_SUMMON_BOOM_BOT:
                        DoCast(SPELL_BOOM_BOT_SUMMON);
                        events.DelayEvents(2*IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_SUMMON_BOOM_BOT, 12*IN_MILLISECONDS, 0, PHASE_MODULE_SOLO);
                        break;
                }
            }
        }

        void SpawnAdds()
        {
            ++SpawnedAdds;
            me->SummonCreature(NPC_JUNK_BOT_TRIGGER, SummonPos[rand32()%9], TEMPSUMMON_TIMED_DESPAWN, 10*IN_MILLISECONDS);
            if (SpawnedAdds >= 3)
            {
                me->SummonCreature(NPC_ASSAULT_BOT_TRIGGER, SummonPos[rand32()%9], TEMPSUMMON_TIMED_DESPAWN, 10*IN_MILLISECONDS);
                if (HardMode)
                    for (uint8 i = 0; i < 2; ++i)
                        me->SummonCreature(NPC_EMERGENCY_BOT_TRIGGER, SummonPos[rand32()%9], TEMPSUMMON_TIMED_DESPAWN, 10*IN_MILLISECONDS);

                SpawnedAdds = 0;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_aerial_unitAI(creature);
    }
};

class npc_proximity_mine : public CreatureScript
{
public:
    npc_proximity_mine() : CreatureScript("npc_proximity_mine") { }

    struct npc_proximity_mineAI : public ScriptedAI
    {
        npc_proximity_mineAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            instance = creature->GetInstanceScript();
            BoomTimer = 35*IN_MILLISECONDS;
            DelayTimer = urand(1500, 2000);
            Boom = false;
            Delay = true;
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (!Delay && !Boom && me->IsWithinDistInMap(who, 0.5f) && who->ToPlayer() && !who->ToPlayer()->IsGameMaster())
            {
                DoCastAOE(SPELL_EXPLOSION);
                me->DespawnOrUnsummon(1*IN_MILLISECONDS);
                Boom = true;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (Delay)
            {
                if (DelayTimer <= diff)
                    Delay = false;
                else
                    DelayTimer -= diff;
            }

            if (BoomTimer <= diff)
            {
                if (!Boom)
                {
                    DoCastAOE(SPELL_EXPLOSION);
                    me->DespawnOrUnsummon(1*IN_MILLISECONDS);
                    Boom = true;
                }
            }
            else
                BoomTimer -= diff;
        }

         void SpellHitTarget(Unit* target, SpellInfo const* spell)
         {
             if (target->GetTypeId() != TYPEID_PLAYER)
                 return;

             if (spell->Id == SPELL_EXPLOSION)
                 if (Unit* mimiron = ObjectAccessor::GetUnit(*me, instance ? ObjectGuid(instance->GetGuidData(BOSS_MIMIRON)) : ObjectGuid::Empty))
                     mimiron->GetAI()->DoAction(ACTION_FAIL_SET_UP_US_THE_BOMB);
         }

        private:
            InstanceScript* instance;
            uint32 BoomTimer;
            uint32 DelayTimer;
            bool Boom;
            bool Delay;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_proximity_mineAI(creature);
    }
};

class npc_rocket_strike : public CreatureScript
{
public:
    npc_rocket_strike() : CreatureScript("npc_rocket_strike") { }

    struct npc_rocket_strikeAI : public ScriptedAI
    {
        npc_rocket_strikeAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            instance = me->GetInstanceScript();
            me->DespawnOrUnsummon(6*IN_MILLISECONDS);
            DoCastSelf(SPELL_ROCKET_STRIKE_AURA, true);
        }

        void SpellHitTarget(Unit* target, SpellInfo const* spell)
        {
            if (target->GetTypeId() != TYPEID_PLAYER)
                return;

            if (spell->Id == SPELL_ROCKET_STRIKE_DMG)
                if (Unit* mimiron = ObjectAccessor::GetUnit(*me, instance ? ObjectGuid(instance->GetGuidData(BOSS_MIMIRON)) : ObjectGuid::Empty))
                    mimiron->GetAI()->DoAction(ACTION_FAIL_SET_UP_US_THE_BOMB);
        }

        void UpdateAI(uint32 /*diff*/) {}
        void AttackStart(Unit* /*target*/) {}

        private:
            InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_rocket_strikeAI(creature);
    }
};

class npc_magnetic_core : public CreatureScript
{
    public:
        npc_magnetic_core() : CreatureScript("npc_magnetic_core") { }

        struct npc_magnetic_coreAI : public ScriptedAI
        {
            npc_magnetic_coreAI(Creature* creature) : ScriptedAI(creature)
            {
                SetCombatMovement(false);
                me->DespawnOrUnsummon(21*IN_MILLISECONDS);
                if (Creature* AerialUnit = me->FindNearestCreature(NPC_AERIAL_COMMAND_UNIT, 20.0f, true))
                    AerialUnit->AI()->DoAction(ACTION_DISABLE_AERIAL);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_magnetic_coreAI(creature);
        }
};

class npc_assault_bot : public CreatureScript
{
    public:
        npc_assault_bot() : CreatureScript("npc_assault_bot") { }

        struct npc_assault_botAI : public ScriptedAI
        {
            npc_assault_botAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = me->GetInstanceScript();
            }

            void Reset()
            {
                FieldTimer = urand(4*IN_MILLISECONDS, 6*IN_MILLISECONDS);
                AttackTimer = 2*IN_MILLISECONDS;
                me->SetReactState(REACT_PASSIVE);
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                me->SetInCombatWithZone();

                if (Creature* mimiron = me->FindNearestCreature(NPC_MIMIRON, 60.f, true))
                    if (mimiron->AI()->GetData(DATA_GET_HARD_MODE) == 1)
                        DoCastSelf(SPELL_EMERGENCY_MODE, true);
            }

            void UpdateAI(uint32 diff)
            {
                if (me->HasReactState(REACT_PASSIVE))
                {
                    if (AttackTimer <= diff)
                    {
                        me->SetReactState(REACT_AGGRESSIVE);

                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                            me->AI()->AttackStart(target);
                    }
                    else
                    {
                        AttackTimer -= diff;
                        return;
                    }
                }

                if (!UpdateVictim())
                    return;

                if (FieldTimer <= diff)
                {
                    DoCastVictim(SPELL_MAGNETIC_FIELD);
                    FieldTimer = urand(15*IN_MILLISECONDS, 20*IN_MILLISECONDS);
                }
                else
                    FieldTimer -= diff;

                DoMeleeAttackIfReady();
            }

            void BeforeSpellHit(Unit* /*caster*/, SpellInfo const* spell, int32& /*damage*/) override
            {
                if (!me->isDead())
                    if (spell->Id == SPELL_ROCKET_STRIKE_DMG)
                        DoCastAOE(SPELL_NOT_SO_FRIENDLY_FIRE, true);
            }

        private:
            InstanceScript* instance;
            uint32 FieldTimer;
            uint32 AttackTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_assault_botAI(creature);
        }
};

class npc_emergency_bot : public CreatureScript
{
    public:
        npc_emergency_bot() : CreatureScript("npc_emergency_bot") { }

        struct npc_emergency_botAI : public ScriptedAI
        {
            npc_emergency_botAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetReactState(REACT_PASSIVE);

                instance = me->GetInstanceScript();
            }

            void Reset()
            {
                FollowFlame = true;
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                me->SetInCombatWithZone();

                DoCastSelf(SPELL_EMERGENCY_MODE, true);
                if (Is25ManRaid())
                    DoCastSelf(SPELL_SIREN, true);
            }

            void MovementInform(uint32 /*type*/, uint32 id)
            {
                if (id == 1)
                    if (Creature* target = ObjectAccessor::GetCreature(*me, targetedFlame))
                        me->SetFacingToObject(target);
            }

            void UpdateAI(uint32 diff)
            {
                if (FollowFlame)
                {
                    if (Creature* followUnit = me->FindNearestCreature(NPC_FLAME_SPREAD, 35.0f, true))
                    {
                        float x, y;
                        followUnit->GetNearPoint2D(x, y, 5.0f, 0);
                        me->GetMotionMaster()->MovePoint(1, x, y, followUnit->GetPositionZ());
                        targetedFlame = followUnit->GetGUID();
                    }
                    else
                        me->GetMotionMaster()->MoveRandom(50.0f);
                    FollowFlame = false;
                    SprayTimer = 8*IN_MILLISECONDS;
                }
                else
                    if (SprayTimer <= diff)
                    {
                        DoCast(SPELL_WATER_SPRAY);

                        FollowFlame = true;
                    }
                    else
                        SprayTimer -= diff;
            }

        private:
            InstanceScript* instance;
            uint32 SprayTimer;
            bool FollowFlame;
            ObjectGuid targetedFlame;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_emergency_botAI(creature);
        }
};

class npc_mimiron_bomb_bot : public CreatureScript
{
    public:
        npc_mimiron_bomb_bot() : CreatureScript("npc_mimiron_bomb_bot") { }

        struct npc_mimiron_bomb_botAI : public ScriptedAI
        {
            npc_mimiron_bomb_botAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = me->GetInstanceScript();
                me->SetReactState(REACT_PASSIVE);

                me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), 364.314f, me->GetOrientation());
                me->GetMotionMaster()->MoveFall();
            }

            void Reset()
            {
                Despawn = false;
                AttackTimer = 3*IN_MILLISECONDS;
            }

            void JustDied(Unit* /*killer*/)
            {
                DoCastSelf(SPELL_BOMB_BOT, true);
            }

            void UpdateAI(uint32 diff)
            {
                if (me->HasReactState(REACT_PASSIVE))
                {
                    if (AttackTimer <= diff)
                    {
                        me->SetReactState(REACT_AGGRESSIVE);
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        {
                            AddThreat(target, 999999.0f);
                            AttackStart(target);
                        }
                    }
                    else
                    {
                        AttackTimer -= diff;
                        return;
                    }
                }

                if (!UpdateVictim())
                    return;

                if (!Despawn && me->IsWithinMeleeRange(me->GetVictim()))
                {
                    Despawn = true;
                    DoCastSelf(SPELL_BOMB_BOT, true);
                    me->DespawnOrUnsummon(500);
                }
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell)
            {
                if (target->GetTypeId() != TYPEID_PLAYER)
                    return;

                if (spell->Id == SPELL_BOMB_BOT)
                    if (Unit* mimiron = ObjectAccessor::GetUnit(*me, instance ? ObjectGuid(instance->GetGuidData(BOSS_MIMIRON)) : ObjectGuid::Empty))
                        mimiron->GetAI()->DoAction(ACTION_FAIL_SET_UP_US_THE_BOMB);
            }

        private:
            bool Despawn;
            InstanceScript* instance;
            uint32 AttackTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_mimiron_bomb_botAI(creature);
        }
};

class npc_mimiron_flame_trigger : public CreatureScript
{
    public:
        npc_mimiron_flame_trigger() : CreatureScript("npc_mimiron_flame_trigger") { }

        struct npc_mimiron_flame_triggerAI : public ScriptedAI
        {
            npc_mimiron_flame_triggerAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetInCombatWithZone();
                instance = me->GetInstanceScript();
                me->SetReactState(REACT_PASSIVE);
            }

            void Reset()
            {
                z = me->GetPositionZ();
                MaxDistance = 6.3f;
                events.ScheduleEvent(EVENT_FLAME_SPREAD, 2*IN_MILLISECONDS);
            }

            void MovementInform(uint32 /*type*/, uint32 id)
            {
                if (id != 1)
                    return;

                DoCast(SPELL_SUMMON_FLAMES_SPREAD);
            }

            void UpdateAI(uint32 diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_FLAME_SPREAD:
                            if (Creature* mimiron = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(BOSS_MIMIRON)) : ObjectGuid::Empty))
                            {
                                if (mimiron->AI()->GetData(DATA_FLAME_COUNT) < FLAME_MAX_COUNT)
                                {
                                    if (Player* target = me->FindNearestPlayer(100.0f))
                                    {
                                        if (target->GetExactDist2d(me->GetPositionX(), me->GetPositionY()) < MaxDistance)
                                        {
                                            me->GetMotionMaster()->MovePoint(1, target->GetPositionX(), target->GetPositionY(), z);
                                        }
                                        else
                                        {
                                            me->SetFacingToObject(target);
                                            events.ScheduleEvent(EVENT_FLAME_MOVEMENT, 250);
                                        }
                                    }
                                }
                                else
                                    me->DespawnOrUnsummon();
                            }
                            events.ScheduleEvent(EVENT_FLAME_SPREAD, 5*IN_MILLISECONDS);
                            break;
                        case EVENT_FLAME_MOVEMENT:
                            float x = me->GetPositionX();
                            float y = me->GetPositionY();
                            float o = me->GetOrientation();

                            x += MaxDistance * cos(o);
                            y += MaxDistance * sin(o);

                            me->GetMotionMaster()->MovePoint(1, x, y, z);
                            break;
                    }
                }
            }

            void JustSummoned(Creature* creature)
            {
                if (Creature* mimiron = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(BOSS_MIMIRON)) : ObjectGuid::Empty))
                    mimiron->AI()->JustSummoned(creature);
            }

            void SummonedCreatureDespawn(Creature* summon)
            {
                if (Creature* mimiron = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(BOSS_MIMIRON)) : ObjectGuid::Empty))
                    mimiron->AI()->SummonedCreatureDespawn(summon);
            }

        private:
            InstanceScript* instance;
            EventMap events;

            float z;
            float MaxDistance;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_mimiron_flame_triggerAI(creature);
        }
};

class npc_mimiron_flame_spread : public CreatureScript
{
    public:
        npc_mimiron_flame_spread() : CreatureScript("npc_mimiron_flame_spread") { }

        struct npc_mimiron_flame_spreadAI : public ScriptedAI
        {
            npc_mimiron_flame_spreadAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetReactState(REACT_PASSIVE);
            }

            void Reset()
            {
                DoCastSelf(SPELL_FLAME, true);
            }

            void EnterEvadeMode(EvadeReason /*why*/) override {}
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_mimiron_flame_spreadAI(creature);
        }
};

class npc_frost_bomb : public CreatureScript
{
    public:
        npc_frost_bomb() : CreatureScript("npc_frost_bomb") { }

        struct npc_frost_bombAI : public ScriptedAI
        {
            npc_frost_bombAI(Creature* creature) : ScriptedAI(creature)
            {
                SetCombatMovement(false);
                me->SetReactState(REACT_PASSIVE);
                DoCastSelf(SPELL_FROST_BOMB, true);
                me->SetObjectScale(0.3f);

                GrowCount = 20;
                GrowTimer = 500;
            }

            void UpdateAI(uint32 diff)
            {
                if (GrowTimer <= diff)
                {
                    me->SetObjectScale(me->GetFloatValue(OBJECT_FIELD_SCALE_X) + 0.10f);
                    --GrowCount;

                    GrowTimer = 500;
                }
                else
                    GrowTimer -= diff;

                if (!GrowCount)
                {
                    if (TempSummon* bomb = me->SummonCreature(NPC_FROST_BOMB_DMG, *me, TEMPSUMMON_TIMED_DESPAWN, 500))
                        bomb->CastSpell(me, RAID_MODE(SPELL_FROST_BOMB_EXPLOSION_10, SPELL_FROST_BOMB_EXPLOSION_25), true);
                    me->DespawnOrUnsummon(100);
                }
            }

        private:
            uint32 GrowTimer;
            uint8 GrowCount;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_frost_bombAI(creature);
        }
};

class npc_junk_bot : public CreatureScript
{
public:
    npc_junk_bot() : CreatureScript("npc_junk_bot") { }

    struct npc_junk_botAI : public ScriptedAI
    {
        npc_junk_botAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
            AttackTimer = 2*IN_MILLISECONDS;
        }

        void IsSummonedBy(Unit* /*summoner*/)
        {
            me->SetInCombatWithZone();

            if (Creature* mimiron = me->FindNearestCreature(NPC_MIMIRON, 60.0f, true))
                if (mimiron->AI()->GetData(DATA_GET_HARD_MODE))
                    DoCastSelf(SPELL_EMERGENCY_MODE, true);
        }

        void UpdateAI(uint32 diff)
        {
            if (me->HasReactState(REACT_PASSIVE))
            {
                if (AttackTimer <= diff)
                {
                    me->SetReactState(REACT_AGGRESSIVE);
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                        AttackStart(target);
                }
                else
                {
                    AttackTimer -= diff;
                    return;
                }
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

        private:
            uint32 AttackTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_junk_botAI (creature);
    }
};

class go_not_push_button : public GameObjectScript
{
public:
    go_not_push_button() : GameObjectScript("go_not_push_button") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        InstanceScript* instance = go->GetInstanceScript();

        if (!instance->CheckRequiredBosses(BOSS_MIMIRON))
            return true;

        if (Creature* mimiron = ObjectAccessor::GetCreature(*player, ObjectGuid(instance->GetGuidData(BOSS_MIMIRON))))
        {
            mimiron->AI()->DoAction(ACTION_ACTIVATE_HARD_MODE);
            go->UseDoorOrButton();
            return false;
        }

        return true;
    }
};

struct GMCheckSelector
{
    bool operator()(Player* player)
    {
        return player->IsGameMaster();
    }
};

class spell_rapid_burst : public SpellScriptLoader
{
    public:
        spell_rapid_burst() : SpellScriptLoader("spell_rapid_burst") { }

        class spell_rapid_burst_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_rapid_burst_AuraScript);

            bool Load()
            {
                right = true;
                return true;
            }

            void HandleTurn(AuraEffect const* /*aurEff*/)
            {
                if (Unit* caster = GetCaster())
                    caster->SetFacingToObject(GetOwner());
            }

            void HandleDummyTick(AuraEffect const* /*aurEff*/)
            {
                Unit* owner = GetUnitOwner();
                uint32 spell = 0;

                switch (owner->GetMap()->GetDifficulty())
                {
                    case RAID_DIFFICULTY_10MAN_NORMAL:
                        if (right)
                            spell = SPELL_RAPID_BURST_RIGHT_10;
                        else
                            spell = SPELL_RAPID_BURST_LEFT_10;
                        break;
                    case RAID_DIFFICULTY_25MAN_NORMAL:
                        if (right)
                            spell = SPELL_RAPID_BURST_RIGHT_25;
                        else
                            spell = SPELL_RAPID_BURST_LEFT_25;
                        break;
                    default:
                        break;
                }

                right = !right;
                owner->CastSpell((Unit*) NULL, spell, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_rapid_burst_AuraScript::HandleTurn, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_rapid_burst_AuraScript::HandleDummyTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }

            private:
                bool right;
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_rapid_burst_AuraScript();
        }
};

class spell_proximity_mines : public SpellScriptLoader
{
    public:
        spell_proximity_mines() : SpellScriptLoader("spell_proximity_mines") { }

        class spell_proximity_mines_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_proximity_mines_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                for (uint8 i = urand(8, 10); i > 0; --i)
                    GetCaster()->CastSpell((Unit*) NULL, SPELL_MINES_SPAWN, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_proximity_mines_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_proximity_mines_SpellScript();
        }
};

class DistanceTargetSelector
{
    public:
        DistanceTargetSelector(Unit* caster, float min = 0, float max = 0) : _caster(caster), _min(min), _max(max) {}

        bool operator()(WorldObject* object)
        {
            float dist = object->GetDistance(_caster);

            if (_min && dist < _min)
                return true;

            if (_max && dist > _max)
                return true;

            return false;
        }

    private:
        Unit* _caster;
        float _min;
        float _max;
};

class spell_frost_bomb_summon : public SpellScriptLoader
{
    public:
        spell_frost_bomb_summon() : SpellScriptLoader("spell_frost_bomb_summon") { }

        class spell_frost_bomb_summon_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_frost_bomb_summon_SpellScript);

            void CheckTargets(std::list<WorldObject*>& objects)
            {
                Unit* caster = GetCaster();
                std::list<WorldObject*> copy = objects;

                objects.remove_if(DistanceTargetSelector(caster, 17.5f, 75.0f));

                if (objects.empty())
                    objects = copy;

                Trinity::Containers::RandomResize(objects, 1);
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                GetCaster()->CastSpell(GetHitUnit(), SPELL_FROST_BOMB_SUMMON, true);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_frost_bomb_summon_SpellScript::CheckTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
                OnEffectHitTarget += SpellEffectFn(spell_frost_bomb_summon_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_frost_bomb_summon_SpellScript();
        }
};

class spell_napalm_shell_trigger : public SpellScriptLoader
{
    public:
        spell_napalm_shell_trigger() : SpellScriptLoader("spell_napalm_shell_trigger") { }

        class spell_napalm_shell_trigger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_napalm_shell_trigger_SpellScript);

            void CheckTargets(std::list<WorldObject*>& objects)
            {
                std::list<WorldObject*> original = objects;
                Unit* caster = GetCaster();

                objects.remove_if(DistanceTargetSelector(caster, 15.0f, 75.0f));

                if (objects.empty())
                    objects = original;

                Trinity::Containers::RandomResize(objects, 1);
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                GetCaster()->CastSpell(GetHitUnit(),
                    GetCaster()->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL ? SPELL_NAPALM_SHELL : SPELL_NAPALM_SHELL_25,
                    true);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_napalm_shell_trigger_SpellScript::CheckTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnEffectHitTarget += SpellEffectFn(spell_napalm_shell_trigger_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_napalm_shell_trigger_SpellScript();
        }
};

class spell_rocket_strike_trigger : public SpellScriptLoader
{
    public:
        spell_rocket_strike_trigger() : SpellScriptLoader("spell_rocket_strike_trigger") { }

        class spell_rocket_strike_trigger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rocket_strike_trigger_SpellScript);

            bool Load()
            {
                slot = 5;
                return true;
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();

                if (Vehicle* vehicle = GetCaster()->GetVehicleKit())
                    if (Unit* rocket = vehicle->GetPassenger(slot++))
                        if (rocket->GetDistance2d(caster) < 10.0f)  // Prevent not-casting if rockets are "away"
                            caster = rocket;

                caster->CastSpell(GetHitUnit(), SPELL_ROCKET_STRIKE, false);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_rocket_strike_trigger_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }

            private:
                uint8 slot;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rocket_strike_trigger_SpellScript();
        }
};

class spell_extinguish_flames : public SpellScriptLoader
{
    public:
        spell_extinguish_flames() : SpellScriptLoader("spell_extinguish_flames") { }

        class spell_extinguish_flames_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_extinguish_flames_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                GetHitCreature()->DespawnOrUnsummon();
            }

            void Register()
            {
                switch (m_scriptSpellId)
                {
                    case SPELL_CLEAR_FIRES: // Effect 0: TARGET_UNIT_SRC_AREA_ENTRY
                    case SPELL_WATER_SPRAY: // Effect 0: TARGET_UNIT_CONE_ENTRY
                    case SPELL_FLAME_SUPPRESSANT_VX001: // Effect 0: TARGET_UNIT_SRC_AREA_ENTRY
                        OnEffectHitTarget += SpellEffectFn(spell_extinguish_flames_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
                        break;
                    case SPELL_FROST_BOMB_EXPLOSION_10: // Effect 2: TARGET_UNIT_SRC_AREA_ENTRY
                    case SPELL_FROST_BOMB_EXPLOSION_25:
                        OnEffectHitTarget += SpellEffectFn(spell_extinguish_flames_SpellScript::HandleDummy, EFFECT_2, SPELL_EFFECT_DUMMY);
                        break;
                }
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_extinguish_flames_SpellScript();
        }
};

class spell_summon_flames_trigger : public SpellScriptLoader
{
    public:
        spell_summon_flames_trigger() : SpellScriptLoader("spell_summon_flames_trigger") { }

        class spell_summon_flames_trigger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_summon_flames_trigger_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                GetCaster()->CastSpell(GetHitUnit(), SPELL_SUMMON_FLAMES_INITIAL, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_summon_flames_trigger_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_summon_flames_trigger_SpellScript();
        }
};

class achievement_firefighter : public AchievementCriteriaScript
{
    public:
        achievement_firefighter() : AchievementCriteriaScript("achievement_firefighter") {}

        bool OnCheck(Player* player, Unit* /*target*/)
        {
            if (!player)
                return false;

            if (InstanceScript* instance = player->GetInstanceScript())
                if (Creature* mimiron = ObjectAccessor::GetCreature(*player, ObjectGuid(instance->GetGuidData(BOSS_MIMIRON))))
                    if (mimiron->AI()->GetData(DATA_GET_HARD_MODE))
                        return true;

            return false;
        }
};

void AddSC_boss_mimiron()
{
    new boss_mimiron();
    new boss_leviathan_mk();
    new boss_vx_001();
    new boss_aerial_unit();

    new npc_proximity_mine();
    new npc_rocket_strike();
    new npc_magnetic_core();
    new npc_assault_bot();
    new npc_emergency_bot();
    new npc_mimiron_bomb_bot();
    new npc_mimiron_flame_trigger();
    new npc_mimiron_flame_spread();
    new npc_frost_bomb();
    new npc_junk_bot();

    new go_not_push_button();

    new spell_rapid_burst();
    new spell_proximity_mines();
    new spell_frost_bomb_summon();
    new spell_napalm_shell_trigger();
    new spell_rocket_strike_trigger();
    new spell_extinguish_flames();
    new spell_summon_flames_trigger();

    new achievement_firefighter();
}
