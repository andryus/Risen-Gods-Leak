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
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "SpellScript.h"
#include "obsidian_sanctum.h"
#include "PassiveAI.h"
#include "Pet.h"
#include "DBCStores.h"

enum Yells
{
    //Sartharion Yell
    SAY_SARTHARION_AGGRO                        = 0,
    SAY_SARTHARION_BERSERK                      = 1,
    SAY_SARTHARION_BREATH                       = 2,
    SAY_SARTHARION_CALL_SHADRON                 = 3,
    SAY_SARTHARION_CALL_TENEBRON                = 4,
    SAY_SARTHARION_CALL_VESPERON                = 5,
    SAY_SARTHARION_DEATH                        = 6,
    SAY_SARTHARION_SPECIAL                      = 7,
    SAY_SARTHARION_SLAY                         = 8,
    WHISPER_LAVA_CHURN                          = 9,

    WHISPER_HATCH_EGGS                          = 6,
    WHISPER_OPEN_PORTAL                         = 6, // whisper, shared by two dragons

    WHISPER_SHADRON_DICIPLE                     = 7,
    WHISPER_VESPERON_DICIPLE                    = 7,
};

enum TeneText
{
    SAY_TENEBRON_AGGRO                      = 0,
    SAY_TENEBRON_SLAY                       = 1,
    SAY_TENEBRON_DEATH                      = 2,
    SAY_TENEBRON_BREATH                     = 3,
    SAY_TENEBRON_RESPOND                    = 4,
    SAY_TENEBRON_SPECIAL                    = 5
};

enum ShadText
{
    SAY_SHADRON_AGGRO                       = 0,
    SAY_SHADRON_SLAY                        = 1,
    SAY_SHADRON_DEATH                       = 2,
    SAY_SHADRON_BREATH                      = 3,
    SAY_SHADRON_RESPOND                     = 4,
    SAY_SHADRON_SPECIAL                     = 5
};

enum VespText
{
    SAY_VESPERON_AGGRO                      = 0,
    SAY_VESPERON_SLAY                       = 1,
    SAY_VESPERON_DEATH                      = 2,
    SAY_VESPERON_BREATH                     = 3,
    SAY_VESPERON_RESPOND                    = 4,
    SAY_VESPERON_SPECIAL                    = 5,
};

enum Spells
{
    // Sartharion
    SPELL_BERSERK                               = 61632,
    SPELL_CLEAVE                                = 56909,
    SPELL_FLAME_BREATH                          = 56908,
    SPELL_FLAME_BREATH_H                        = 58956,
    SPELL_TAIL_LASH                             = 56910,
    SPELL_TAIL_LASH_H                           = 58957,
    SPELL_WILL_OF_SARTHARION                    = 61254,
    SPELL_PYROBUFFET_RANGE                      = 58907,    // casted on player out of combat area


    // Twilight portal
    SPELL_TWILIGHT_SHIFT_ENTER                  = 57620,    // Enter Twilight Phase
    SPELL_TWILIGHT_SHIFT                        = 57874,    // Twilight Shift Aura, on remove trigger SPELL_TWILIGHT_RESIDUE
    SPELL_TWILIGHT_SHIFT_REMOVAL                = 61187,    // leave phase
    SPELL_TWILIGHT_SHIFT_REMOVAL_ALL            = 61190,    // leave phase, AOE spell on all players

    SPELL_TWILIGHT_RESIDUE                      = 61885,    // makes immune to shadow damage, applied when leave twilight phase


    // Flame tsunami
    SPELL_FLAME_TSUNAMI                         = 57494,    // the visual dummy
    SPELL_FLAME_TSUNAMI_LEAP                    = 60241,    // SPELL_EFFECT_138 some leap effect, causing caster to move in direction

    SPELL_FLAME_TSUNAMI_DMG_AURA                = 57492,    // periodic damage, npc has this aura
    SPELL_FLAME_TSUNAMI_BUFF                    = 60430,


    //Fire cyclone
    SPELL_CYCLONE_AURA                          = 57560,    // triggers 57562 Burning Winds, applied in creature_template_addon with sql
    SPELL_CYCLONE_AURA2_1                       = 57598,
    SPELL_CYCLONE_AURA2_2                       = 58964,
    SPELL_LAVA_STRIKE                           = 57571,


    //Whelps
    SPELL_FADE_ARMOR                            = 60708
};

enum DragonSpells
{
    // Vesperon, Shadron, Tenebron
    SPELL_SHADOW_BREATH_H                       = 59126,
    SPELL_SHADOW_BREATH                         = 57570,
    SPELL_SHADOW_FISSURE                        = 57579,
    SPELL_SHADOW_FISSURE_H                      = 59127,
    SPELL_VOID_BLAST                            = 57581,
    SPELL_VOID_BLAST_H                          = 59128,

    //Vesperon
    SPELL_POWER_OF_VESPERON                     = 61251,    // Vesperon's presence decreases the maximum health of all enemies by 25%.
    SPELL_TWILIGHT_TORMENT_VESP                 = 57935,    // Shadow damage only, used in single fight
    SPELL_TWILIGHT_TORMENT_VESP_ACO             = 58835,    // Fire and Shadow damage, used in sartharion fight

    //Shadron
    SPELL_POWER_OF_SHADRON                      = 58105,    // Shadron's presence increases Fire damage taken by all enemies by 100%.
    SPELL_GIFT_OF_TWILIGTH_SHA                  = 57835,    // TARGET_SCRIPT shadron
    SPELL_GIFT_OF_TWILIGTH_SAR                  = 58766,    // TARGET_SCRIPT sartharion

    //Tenebron
    SPELL_POWER_OF_TENEBRON                     = 61248,    // Tenebron's presence increases Shadow damage taken by all enemies by 100%.
    SPELL_SUMMON_TWILIGHT_WHELP                 = 58035,    // doesn't work, will spawn NPC_TWILIGHT_WHELP
    SPELL_SUMMON_SARTHARION_TWILIGHT_WHELP      = 58826,    // doesn't work, will spawn NPC_SHARTHARION_TWILIGHT_WHELP
    SPELL_HATCH_EGGS_H                          = 59189,
    SPELL_HATCH_EGGS                            = 58542,
    SPELL_HATCH_EGGS_EFFECT_H                   = 59190,
    SPELL_HATCH_EGGS_EFFECT                     = 58685
};

enum Creatures
{
    NPC_FLAME_TSUNAMI                           = 30616,    // for the flame waves

    NPC_ACOLYTE_OF_SHADRON                      = 31218,

    NPC_TWILIGHT_EGG                            = 30882,
    NPC_TWILIGHT_WHELP                          = 30890,

    NPC_SARTHARION_TWILIGHT_EGG                 = 31204,
    NPC_SHARTHARION_TWILIGHT_WHELP              = 31214
};

enum Achievements
{
    ACHIEV_TWILIGHT_ASSIST                      = 2049,
    H_ACHIEV_TWILIGHT_ASSIST                    = 2052,
    ACHIEV_TWILIGHT_DUO                         = 2050,
    H_ACHIEV_TWILIGHT_DUO                       = 2053,
    ACHIEV_TWILIGHT_ZONE                        = 2051,
    H_ACHIEV_TWILIGHT_ZONE                      = 2054,
    ACHIEV_GONNA_GO_WHEN_THE_VOLCANO_BLOWS_10   = 2047,
    ACHIEV_GONNA_GO_WHEN_THE_VOLCANO_BLOWS_25   = 2048,
    ACHIEV_LESS_IS_MORE_10                      = 624,
    ACHIEV_LESS_IS_MORE_25                      = 1877
};

enum Waypoints
{
    WAYPOINT_TSUNAMI        = 0,

    WAYPOINT_DRAGON_INIT    = 1,
    WAYPOINT_DRAGON_MOVE    = 2,
    WAYPOINT_DRAGON_LAND    = 3,


    MAX_WAYPOINT            = 6,
};

enum Actions
{
    ACTION_DRAGON_PORTAL_CLOSED         = 1,
    ACTION_TENEBRON_EGG_KILLED          = 2,
};

enum FlameData
{
    DATA_SIDE               = 0,
    DATA_NUMBER
};

enum FlameSide
{
    SIDE_LEFT               = 0,
    SIDE_RIGHT              = 1
};

//each dragons special points. First where fly to before connect to connon, second where land point is.
Position const Tenebron[] =
{
    {3212.854f, 575.597f, 109.856f, 0.0f},                      //init
    {3246.425f, 565.367f, 61.249f, 0.0f}                        //end
};

Position const Shadron[] =
{
    {3293.238f, 472.223f, 106.968f, 0.0f},
    {3271.669f, 526.907f, 61.931f, 0.0f}
};

Position const Vesperon[] =
{
    {3193.310f, 472.861f, 102.697f, 0.0f},
    {3227.268f, 533.238f, 59.995f, 0.0f}
};

Position const DragonCommon[MAX_WAYPOINT] =
{
    {3214.012f, 468.932f, 98.652f, 0.0f},
    {3244.950f, 468.427f, 98.652f, 0.0f},
    {3283.520f, 496.869f, 98.652f, 0.0f},
    {3287.316f, 555.875f, 98.652f, 0.0f},
    {3250.479f, 585.827f, 98.652f, 0.0f},
    {3209.969f, 566.523f, 98.652f, 0.0f}
};

Position const TwilightEggs[] =
{
    {3219.28f, 669.121f, 88.5549f, 0.0f},
    {3221.55f, 682.852f, 90.5361f, 0.0f},
    {3239.77f, 685.94f, 90.3168f, 0.0f},
    {3250.33f, 669.749f, 88.7637f, 0.0f},
    {3246.6f, 642.365f, 84.8752f, 0.0f},
    {3233.68f, 653.117f, 85.7051f, 0.0f}
};

Position const TwilightEggsSarth[] =
{
    {3252.73f, 515.762f , 58.5501f, 0.0f},
    {3256.56f, 521.119f , 58.6061f, 0.0f},
    {3255.63f, 527.513f , 58.7568f, 0.0f},
    {3264.90f, 525.865f , 58.6436f, 0.0f},
    {3264.26f, 516.364f , 58.8011f, 0.0f},
    {3257.54f, 502.285f , 58.2077f, 0.0f}
};

Position const FlameLeftSpawn[] =
{
    {3210.00f, 573.211f, 57.0833f, 0.0f},
    {3210.00f, 532.211f, 57.0833f, 0.0f},
    {3210.00f, 491.211f, 57.0833f, 0.0f}
};

Position const FlameLeftDirection[] =
{
    {3281.28f, 573.211f, 57.0833f, 0.0f},
    {3281.28f, 532.211f, 57.0833f, 0.0f},
    {3281.28f, 491.211f, 57.0833f, 0.0f}
};

Position const FlameRightSpawn[] =
{
    {3281.28f, 511.711f, 57.0833f, static_cast<float>(M_PI)},
    {3281.28f, 552.711f, 57.0833f, static_cast<float>(M_PI)}
};

Position const FlameRightDirection[] =
{
    {3210.00f, 511.711f, 57.0833f, 0},
    {3210.00f, 552.711f, 57.0833f, 0}
};

Position const TwilightPortal[] =
{
    {3219.67f, 656.795f, 87.2898f, 5.92596f}, // tenebron
    {3362.01f, 553.726f, 95.7068f, 4.56818f}, // shadron
    {3137.26f, 501.08f, 87.9118f, 0.846795f}, // vesperon
    {3238.37f, 518.595f, 58.9057f, 0.739184f} // sartharion
};

Position const AcolyteofShadron = {3363.92f, 534.703f, 97.2683f, 0.0f};
Position const AcolyteofShadron2 = {3246.57f, 551.263f, 58.6164f, 0.0f};
Position const AcolyteofVesperon = {3145.68f, 520.71f, 89.7f, 0.0f};
Position const AcolyteofVesperon2 = {3246.57f, 551.263f, 58.6164f, 0.0f};

static uint32 DragonAura[] =        {SPELL_POWER_OF_TENEBRON,   SPELL_POWER_OF_SHADRON,   SPELL_POWER_OF_VESPERON};
static const Position DragonWP[] =  {Tenebron[0],               Shadron[0],               Vesperon[0]};

static uint32 GenDebuff[] = {SPELL_TWILIGHT_SHIFT, SPELL_TWILIGHT_SHIFT_ENTER, SPELL_TWILIGHT_TORMENT_VESP, SPELL_TWILIGHT_TORMENT_VESP_ACO};

enum Events
{
    EVENT_ENRAGE = 1,
    EVENT_TENEBRON,
    EVENT_SHADRON,
    EVENT_VESPRON,
    EVENT_APPLY_IMMUNITY,
    EVENT_FLAME_TSUNAMI,
    EVENT_FLAME_BREATH,
    EVENT_TAIL_SWEEP,
    EVENT_CLEAVE,
    EVENT_LAVA_STRIKE,
};

class boss_sartharion : public CreatureScript
{
public:
    boss_sartharion() : CreatureScript("boss_sartharion") { }

    struct boss_sartharionAI : public BossAI
    {
        boss_sartharionAI(Creature* creature) : BossAI(creature, BOSS_SARTHARION)
        {
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);
        }

        void Reset()
        {
            BossAI::Reset();

            players.clear();
            IsBerserk = false;
            IsSoftEnraged = false;
            me->RemoveAllAuras();

            if (GameObject* portal = me->FindNearestGameObject(GO_TWILIGHT_PORTAL, 50.0f))
                portal->Delete();
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            _EnterEvadeMode();

            me->SetReactState(REACT_PASSIVE);
            me->AttackStop();
            me->CombatStop();

            me->GetMotionMaster()->MoveTargetedHome();
            if (instance)
            {
                for (uint8 i=0; i<4; ++i)
                    instance->DoRemoveAurasDueToSpellOnPlayers(GenDebuff[i]);
                for (uint8 i=0; i<3; ++i)
                    instance->DoRemoveAurasDueToSpellOnPlayers(DragonAura[i]);
            }

            CyclonesEvading();

            Reset();
        }

        uint32 GetData(uint32 type) const
        {
            if (type == DATA_DRAGONS_STILL_ALIVE)
                return dragonsStillAlive;

            return 0;
        }

        void JustReachedHome()
        {
            me->SetReactState(REACT_AGGRESSIVE);
            Reset();
        }

        void EnterCombat(Unit* who)
        {
            BossAI::EnterCombat(who);
            Talk(SAY_SARTHARION_AGGRO);

            events.ScheduleEvent(EVENT_ENRAGE, 15*MINUTE*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_TENEBRON, 20*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_SHADRON, 65*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_VESPRON, 110*IN_MILLISECONDS);

            events.ScheduleEvent(EVENT_FLAME_TSUNAMI, 30*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_FLAME_BREATH, 20*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_TAIL_SWEEP, 20*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_CLEAVE, 7*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_LAVA_STRIKE, urand(5, 20)*IN_MILLISECONDS);

            if (instance && (instance->GetData(DATA_TENEBRON_PREFIGHT) == false))
                events.ScheduleEvent(EVENT_APPLY_IMMUNITY, 90*IN_MILLISECONDS);

            DoCastSelf(SPELL_PYROBUFFET_RANGE, true);

            dragonsStillAlive = 3;

            if (instance)
            {
                for (uint8 i = 0; i<3; ++i)
                {
                    if (Creature* tempDragon = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_TENEBRON + i)) : ObjectGuid::Empty))
                        if (tempDragon->IsAlive() && !me->HasAura(SPELL_WILL_OF_SARTHARION))
                            DoCastSelf(SPELL_WILL_OF_SARTHARION);
                }

                if (instance->GetData(DATA_TENEBRON_PREFIGHT)) // already killed
                    dragonsStillAlive--;

                if (instance->GetData(DATA_SHADRON_PREFIGHT)) // already killed
                    dragonsStillAlive--;

                if (instance->GetData(DATA_VESPERON_PREFIGHT)) // already killed
                    dragonsStillAlive--;
            }

            me->ResetLootMode();
            switch (dragonsStillAlive) {
                case 3:
                    me->AddLootMode(LOOT_MODE_HARD_MODE_3); // 3 dragons alive -> mount
                case 2:
                    me->AddLootMode(LOOT_MODE_HARD_MODE_2); // 2 dragons alive -> hardmode loot
                case 1:
                    me->AddLootMode(LOOT_MODE_HARD_MODE_1); // 1 dragon alive -> semihardmode loot
                default: // all 3 dragons killed -> normalmode loot
                    me->AddLootMode(LOOT_MODE_DEFAULT); // No extra loot mode
                    break;

            }

            players.clear();
            if (instance)
            {
                for (Player& player : instance->instance->GetPlayers())
                    players.insert(player.GetGUID());
                FetchDragons();
            }
        }

        void JustDied(Unit* killer)
        {
            BossAI::JustDied(killer);

            Talk(SAY_SARTHARION_DEATH);

            if (instance)
                for (std::set<ObjectGuid>::iterator itr = players.begin(); itr != players.end(); ++itr)
                    if (Player* player = ObjectAccessor::GetPlayer(*me, *itr))
                        instance->DoCompleteAchievement(RAID_MODE(ACHIEV_GONNA_GO_WHEN_THE_VOLCANO_BLOWS_10, ACHIEV_GONNA_GO_WHEN_THE_VOLCANO_BLOWS_25));

            CyclonesEvading();
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Talk(SAY_SARTHARION_SLAY);
        }

        void FetchDragons()
        {
            for (uint8 i=0; i<3; ++i)
            {
                if (Creature* tempDragon = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_TENEBRON + i)) : ObjectGuid::Empty))
                    if (tempDragon->IsAlive() && !tempDragon->GetVictim())
                    {
                        tempDragon->AddAura(DragonAura[i], tempDragon);
                        tempDragon->GetMotionMaster()->MovePoint(WAYPOINT_DRAGON_INIT, DragonWP[i]);
                        tempDragon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        tempDragon->SetReactState(REACT_PASSIVE);
                    }
            }
        }

        void CallDragon(uint32 DataId)
        {
            if (Creature* tempDragon = ObjectAccessor::GetCreature(*me, ObjectGuid(instance ? ObjectGuid(instance->GetGuidData(DataId)) : ObjectGuid::Empty)))
            {
                if (tempDragon->IsAlive())
                {
                    switch (tempDragon->GetEntry())
                    {
                        case NPC_TENEBRON:
                            Talk(SAY_SARTHARION_CALL_TENEBRON);
                            tempDragon->GetMotionMaster()->MovePoint(WAYPOINT_DRAGON_LAND, Tenebron[1]);
                            break;
                        case NPC_SHADRON:
                            Talk(SAY_SARTHARION_CALL_SHADRON);
                            tempDragon->GetMotionMaster()->MovePoint(WAYPOINT_DRAGON_LAND, Shadron[1]);
                            break;
                        case NPC_VESPERON:
                            Talk(SAY_SARTHARION_CALL_VESPERON);
                            tempDragon->GetMotionMaster()->MovePoint(WAYPOINT_DRAGON_LAND, Vesperon[1]);
                            break;
                    }
                }
            }
        }

        void SummonFlameTsunami()
        {
            switch (urand(0, 1))
            {
                case 0:
                    for (uint8 i=0; i<3; ++i)
                        if (Creature* tempLeft = me->SummonCreature(NPC_FLAME_TSUNAMI, FlameLeftSpawn[i] ,TEMPSUMMON_TIMED_DESPAWN, 14000))
                        {
                            tempLeft->AI()->SetData(DATA_SIDE, SIDE_LEFT);
                            tempLeft->AI()->SetData(DATA_NUMBER, i);
                        }
                    break;
                case 1:
                    for (uint8 i=0; i<2; ++i)
                        if (Creature* tempRight = me->SummonCreature(NPC_FLAME_TSUNAMI, FlameRightSpawn[i] ,TEMPSUMMON_TIMED_DESPAWN, 14000))
                        {
                            tempRight->AI()->SetData(DATA_SIDE, SIDE_RIGHT);
                            tempRight->AI()->SetData(DATA_NUMBER, i);
                        }
                    break;
            }

            if (Map* map = me->GetMap())
                if (map->IsDungeon())
                {
                    Map::PlayerList const &PlayerList = map->GetPlayers();

                    for (Player& player : map->GetPlayers())
                        if (player.IsAlive())
                            Talk(WHISPER_LAVA_CHURN, &player);
                }
        }

        void CyclonesEvading()
        {
            //evade cyclones to prevent infightbug
            std::list<Creature*> FireCyclonesList;
            me->GetCreatureListWithEntryInGrid(FireCyclonesList, NPC_FIRE_CYCLONE, 200.0f);
            if (!FireCyclonesList.empty())
                for (std::list<Creature*>::iterator itr = FireCyclonesList.begin(); itr != FireCyclonesList.end(); ++itr)
                {
                    if ((*itr)->IsInWorld() && (*itr)->ToCreature())
                        (*itr)->AI()->EnterEvadeMode();
                }
        }

        void CastLavaStrike(bool addAura) //TODO
        {
            std::list<Creature*> FireCyclonesList;
            Trinity::AllCreaturesOfEntryInRange checker(me, NPC_FIRE_CYCLONE, 200.0f);
            me->VisitAnyNearbyObject<Creature, Trinity::ContainerAction>(200.0f, FireCyclonesList, checker);

            if (FireCyclonesList.empty())
                return;

            std::list<Creature*>::iterator itr = FireCyclonesList.begin();

            advance(itr, rand32()%FireCyclonesList.size());
            if (addAura)
            {
                (*itr)->CastSpell(*itr, SPELL_CYCLONE_AURA2_1, true);
                if (Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 200, true))
                    (*itr)->CastSpell(pTarget, SPELL_LAVA_STRIKE, true);
                Talk(SAY_SARTHARION_SPECIAL);
            }
            else
            {
                if (Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 200, true))
                    (*itr)->CastSpell(pTarget, SPELL_LAVA_STRIKE, true);
            }
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/)
        {
            if (me->GetHealthPct() <= 35 && !IsBerserk)
            {
                for (uint8 i=0; i<3; ++i)
                {
                    if (Creature* tempDragon = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_TENEBRON + i)) : ObjectGuid::Empty))
                        if (tempDragon->IsAlive())
                            tempDragon->AddAura(SPELL_BERSERK, tempDragon);
                }
                Talk(SAY_SARTHARION_BERSERK);
                IsBerserk = true;
            }
            else if (me->GetHealthPct() <= 10 && !IsSoftEnraged)
            {
                IsSoftEnraged = true;
                CastLavaStrike(true);
                events.CancelEvent(EVENT_LAVA_STRIKE);
            }
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
                    case EVENT_FLAME_TSUNAMI:
                        SummonFlameTsunami();
                        events.ScheduleEvent(EVENT_FLAME_TSUNAMI, 30*IN_MILLISECONDS);
                        break;
                    case EVENT_FLAME_BREATH:
                        Talk(SAY_SARTHARION_BREATH);
                        DoCastVictim(RAID_MODE(SPELL_FLAME_BREATH, SPELL_FLAME_BREATH_H));
                        events.ScheduleEvent(EVENT_FLAME_BREATH, urand(25, 35)*IN_MILLISECONDS);
                        break;
                    case EVENT_TAIL_SWEEP:
                        DoCastVictim(RAID_MODE(SPELL_TAIL_LASH, SPELL_TAIL_LASH_H));
                        events.ScheduleEvent(EVENT_TAIL_SWEEP, urand(15, 20)*IN_MILLISECONDS);
                        break;
                    case EVENT_CLEAVE:
                        DoCastVictim(SPELL_CLEAVE);
                        events.ScheduleEvent(EVENT_CLEAVE, urand(7, 10)*IN_MILLISECONDS);
                        break;
                    case EVENT_LAVA_STRIKE:
                        CastLavaStrike(false);
                            events.RescheduleEvent(EVENT_LAVA_STRIKE, urand(5, 20)*IN_MILLISECONDS);
                        break;
                    case EVENT_APPLY_IMMUNITY:
                        DoCastSelf(SPELL_GIFT_OF_TWILIGTH_SAR);
                        break;
                    case EVENT_ENRAGE:
                        DoCastSelf(SPELL_BERSERK);
                        Talk(SAY_SARTHARION_BERSERK);
                        break;
                    case EVENT_TENEBRON:
                        CallDragon(DATA_TENEBRON);
                        break;
                    case EVENT_SHADRON:
                        CallDragon(DATA_SHADRON);
                        break;
                    case EVENT_VESPRON:
                        CallDragon(DATA_VESPERON);
                        break;
                }
            }
            // Don't attack current target if he's not visible for us.
            if (me->GetVictim() && me->GetVictim()->HasAura(SPELL_TWILIGHT_SHIFT))
                me->GetThreatManager().ModifyThreatByPercent(me->GetVictim(), -100);

            DoMeleeAttackIfReady();
        }

        void PlayerHitByVolcano(ObjectGuid guid)
        {
            players.erase(guid); // this is called by the spell, when hits a player
        }

    private:
        uint32 dragonsStillAlive;
        bool IsBerserk;
        bool IsSoftEnraged;
        std::set<ObjectGuid> players;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_sartharionAI(creature);
    }
};

struct sartharion_dragonAI : public ScriptedAI
{
    sartharion_dragonAI(Creature* creature) : ScriptedAI(creature), summons(me)
    {
        instance = creature->GetInstanceScript();
    }

    InstanceScript* instance;
    EventMap events;

    void Reset()
    {
        summons.DespawnAll();
        me->RemoveAllAuras();

        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

        WaypointId = 0;

        if (GameObject* portal = me->FindNearestGameObject(GO_TWILIGHT_PORTAL, 50.0f))
            portal->Delete();
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
         _EnterEvadeMode();

        if (instance)
        {
            DoCastAOE(SPELL_TWILIGHT_SHIFT_REMOVAL_ALL, true);
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_TWILIGHT_TORMENT_VESP);
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_TWILIGHT_TORMENT_VESP_ACO);
        }

        Reset();

        me->GetMotionMaster()->MoveTargetedHome();
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
    }

    void JustSummoned(Creature* summon)
    {
        summons.Summon(summon);
    }

    void MovementInform(uint32 type, uint32 id)
    {
        if (type != POINT_MOTION_TYPE)
            return;

        if (instance && instance->GetBossState(BOSS_SARTHARION) != IN_PROGRESS)
        {
            EnterEvadeMode(EVADE_REASON_OTHER);
            return;
        }

        switch (id)
        {
            case WAYPOINT_DRAGON_INIT:
                switch (me->GetEntry())
                {
                    case NPC_SHADRON:  WaypointId = 2; break;
                    case NPC_TENEBRON: WaypointId = 4; break;
                    case NPC_VESPERON: WaypointId = 0; break;
                }
                me->GetMotionMaster()->MovePoint(WAYPOINT_DRAGON_MOVE, DragonCommon[WaypointId]);
                return;
            case WAYPOINT_DRAGON_MOVE:
                if (++WaypointId > 5)
                    WaypointId = 0;
                me->GetMotionMaster()->MovePoint(WAYPOINT_DRAGON_MOVE, DragonCommon[WaypointId]);
                return;
            case WAYPOINT_DRAGON_LAND:
                me->GetMotionMaster()->Clear();
                me->SetInCombatWithZone();
                me->SetReactState(REACT_AGGRESSIVE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true))
                {
                    AddThreat(target, 1.0f);
                    me->Attack(target, true);
                    me->GetMotionMaster()->MoveChase(target);
                }
                return;
            default:
                return;
        }
    }

    //used when open portal and spawn mobs in phase
    void DoRaidWhisper(int32 iTextId)
    {
        Map* map = me->GetMap();
        if (map && map->IsDungeon())
        {
            for (Player& player : map->GetPlayers())
                Talk(iTextId, &player);
        }
    }

    void OpenPortal()
    {
        if (!instance)
            return;

        DoCastAOE(SPELL_TWILIGHT_SHIFT_REMOVAL_ALL, true);


        if (GameObject* portal = me->FindNearestGameObject(GO_TWILIGHT_PORTAL, 50.0f))
            if (portal)
                return;
        // not selectable wehen spawned by dragon
        Player* player = me->FindNearestPlayer(150.0f);
        if (!player)
            return;

        if (instance->GetBossState(BOSS_SARTHARION) == IN_PROGRESS)
            player->SummonGameObject(GO_TWILIGHT_PORTAL, TwilightPortal[3].m_positionX, TwilightPortal[3].m_positionY, TwilightPortal[3].m_positionZ, TwilightPortal[3].m_orientation, 0, 0, 0, 0, HOUR);
        else
            switch (me->GetEntry())
            {
                case NPC_TENEBRON:
                    player->SummonGameObject(GO_TWILIGHT_PORTAL, TwilightPortal[0].m_positionX, TwilightPortal[0].m_positionY, TwilightPortal[0].m_positionZ, TwilightPortal[0].m_orientation, 0, 0, 0, 0, HOUR);
                    break;
                case NPC_SHADRON:
                    player->SummonGameObject(GO_TWILIGHT_PORTAL, TwilightPortal[1].m_positionX, TwilightPortal[1].m_positionY, TwilightPortal[1].m_positionZ, TwilightPortal[1].m_orientation, 0, 0, 0, 0, HOUR);
                    break;
                case NPC_VESPERON:
                    player->SummonGameObject(GO_TWILIGHT_PORTAL, TwilightPortal[2].m_positionX, TwilightPortal[2].m_positionY, TwilightPortal[2].m_positionZ, TwilightPortal[2].m_orientation, 0, 0, 0, 0, HOUR);
                    break;
            }

    }

    void JustDied(Unit* /*killer*/)
    {
        if (!instance)
            return;

        switch (me->GetEntry())
        {
            case NPC_TENEBRON:
                Talk(SAY_TENEBRON_DEATH);
                instance->SetData(DATA_TENEBRON_EVENT, DONE);
                summons.DespawnAll();
                break;
            case NPC_SHADRON:
                Talk(SAY_SHADRON_DEATH);
                instance->SetData(DATA_SHADRON_EVENT, DONE);
                break;
            case NPC_VESPERON:
                Talk(SAY_VESPERON_DEATH);
                instance->SetData(DATA_VESPERON_EVENT, DONE);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_TWILIGHT_TORMENT_VESP);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_TWILIGHT_TORMENT_VESP_ACO);
                break;
        }
    }

    void UpdateAI(uint32 /*diff*/) {}

    private:
        SummonList summons;
        int8 WaypointId;
};

enum DragonEvents
{
    EVENT_SHADDOW_BREATH = 1,
    EVENT_SHADDOW_FISSURE,
    EVENT_OPEN_PORTAL,
};

class npc_tenebron : public CreatureScript
{
public:
    npc_tenebron() : CreatureScript("npc_tenebron") { }

    struct npc_tenebronAI : public sartharion_dragonAI
    {
        npc_tenebronAI(Creature* creature) : sartharion_dragonAI(creature) { }

        bool HasPortalOpen;

        void Reset()
        {
            sartharion_dragonAI::Reset();
            me->RemoveAllAuras();
            events.Reset();
            HasPortalOpen = false;
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(SAY_TENEBRON_AGGRO);
            me->SetInCombatWithZone();

            events.ScheduleEvent(EVENT_SHADDOW_BREATH, 20000);
            events.ScheduleEvent(EVENT_SHADDOW_FISSURE, 5000);
            events.ScheduleEvent(EVENT_OPEN_PORTAL, 30000);
        }

        void DoAction(int32 action)
        {
            switch (action)
            {
                case ACTION_DRAGON_PORTAL_CLOSED:
                    HasPortalOpen = false;
                    break;
                case ACTION_TENEBRON_EGG_KILLED:
                    if (++destroyedEggsCount == 6)
                        DoCastAOE(SPELL_TWILIGHT_SHIFT_REMOVAL_ALL, true);
                    break;
            }
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Talk(SAY_TENEBRON_SLAY);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
            {
                sartharion_dragonAI::UpdateAI(diff);
                return;
            }

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SHADDOW_BREATH:
                        Talk(SAY_TENEBRON_BREATH);
                        DoCastVictim(RAID_MODE(SPELL_SHADOW_BREATH, SPELL_SHADOW_BREATH_H));
                        events.ScheduleEvent(EVENT_SHADDOW_BREATH, urand(20000,25000));
                        break;
                    case EVENT_SHADDOW_FISSURE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200, true))
                            DoCast(target, RAID_MODE(SPELL_SHADOW_FISSURE, SPELL_SHADOW_FISSURE));
                        events.ScheduleEvent(EVENT_SHADDOW_FISSURE, urand(15000,20000));
                        break;
                    case EVENT_OPEN_PORTAL:
                        if (HasPortalOpen)
                            events.ScheduleEvent(EVENT_OPEN_PORTAL, 10000);
                        else
                        {
                            OpenPortal();
                            DoRaidWhisper(WHISPER_HATCH_EGGS);

                            if (instance->GetBossState(BOSS_SARTHARION) == IN_PROGRESS)
                                for (uint32 i = 0; i < 6; ++i)
                                    me->SummonCreature(NPC_SARTHARION_TWILIGHT_EGG, TwilightEggsSarth[i], TEMPSUMMON_CORPSE_DESPAWN, 20000);
                            else
                                for (uint32 i = 0; i < 6; ++i)
                                    me->SummonCreature(NPC_TWILIGHT_EGG, TwilightEggs[i], TEMPSUMMON_CORPSE_DESPAWN, 20000);

                            destroyedEggsCount = 0;
                            events.ScheduleEvent(EVENT_OPEN_PORTAL, 30000);
                        }
                        break;
                }
            }
            // Don't attack current target if he's not visible for us.
            if (me->GetVictim() && me->GetVictim()->HasAura(SPELL_TWILIGHT_SHIFT))
                me->GetThreatManager().ModifyThreatByPercent(me->GetVictim(), -100);

            DoMeleeAttackIfReady();
        }

    private:
        uint8 destroyedEggsCount;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_tenebronAI(creature);
    }
};

class npc_shadron : public CreatureScript
{
public:
    npc_shadron() : CreatureScript("npc_shadron") { }

    struct npc_shadronAI : public sartharion_dragonAI
    {
        npc_shadronAI(Creature* creature) : sartharion_dragonAI(creature) { }

        bool HasPortalOpen;

        void Reset()
        {
            sartharion_dragonAI::Reset();
            me->RemoveAllAuras();
            events.Reset();
            HasPortalOpen = false;
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(SAY_SHADRON_AGGRO);
            me->SetInCombatWithZone();

            events.ScheduleEvent(EVENT_SHADDOW_BREATH, 20000);
            events.ScheduleEvent(EVENT_SHADDOW_FISSURE, 5000);
            events.ScheduleEvent(EVENT_OPEN_PORTAL, urand(5000,20000));
        }

        void DoAction(int32 action)
        {
            if (action == ACTION_DRAGON_PORTAL_CLOSED)
                HasPortalOpen = false;
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Talk(SAY_SHADRON_SLAY);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
            {
                sartharion_dragonAI::UpdateAI(diff);
                return;
            }

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SHADDOW_BREATH:
                        Talk(SAY_SHADRON_BREATH);
                        DoCastVictim(RAID_MODE(SPELL_SHADOW_BREATH, SPELL_SHADOW_BREATH_H));
                        events.ScheduleEvent(EVENT_SHADDOW_BREATH, urand(20000,25000));
                        break;
                    case EVENT_SHADDOW_FISSURE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200, true))
                            DoCast(target, RAID_MODE(SPELL_SHADOW_FISSURE, SPELL_SHADOW_FISSURE));
                        events.ScheduleEvent(EVENT_SHADDOW_FISSURE, urand(15000,20000));
                        break;
                    case EVENT_OPEN_PORTAL:
                        if (!HasPortalOpen)
                            {
                                if (me->HasAura(SPELL_GIFT_OF_TWILIGTH_SHA))
                                    return;

                                OpenPortal();
                                DoRaidWhisper(WHISPER_OPEN_PORTAL);
                                HasPortalOpen = true;
                                if (instance)
                                {
                                    if (instance->GetBossState(BOSS_SARTHARION) == IN_PROGRESS)
                                        me->SummonCreature(NPC_ACOLYTE_OF_SHADRON, AcolyteofShadron2, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 20000);
                                    else
                                    {
                                        me->SummonCreature(NPC_ACOLYTE_OF_SHADRON, AcolyteofShadron, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 20000);
                                        me->AddAura(SPELL_GIFT_OF_TWILIGTH_SHA, me);
                                    }
                                }
                                events.ScheduleEvent(EVENT_OPEN_PORTAL, urand(60000,65000));
                            }
                        else
                            events.ScheduleEvent(EVENT_OPEN_PORTAL, 10000);
                        break;
                }
            }

            // Don't attack current target if he's not visible for us.
            if (me->GetVictim() && me->GetVictim()->HasAura(SPELL_TWILIGHT_SHIFT))
                me->GetThreatManager().ModifyThreatByPercent(me->GetVictim(), -100);

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_shadronAI(creature);
    }
};

class npc_vesperon : public CreatureScript
{
public:
    npc_vesperon() : CreatureScript("npc_vesperon") { }

    struct npc_vesperonAI : public sartharion_dragonAI
    {
        npc_vesperonAI(Creature* creature) : sartharion_dragonAI(creature) { }

        bool HasPortalOpen;

        void Reset()
        {
            sartharion_dragonAI::Reset();
            me->RemoveAllAuras();
            events.Reset();
            HasPortalOpen = false;
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(SAY_VESPERON_AGGRO);
            me->SetInCombatWithZone();

            events.ScheduleEvent(EVENT_SHADDOW_BREATH, 20000);
            events.ScheduleEvent(EVENT_SHADDOW_FISSURE, 5000);
            events.ScheduleEvent(EVENT_OPEN_PORTAL, urand(5000,20000));
        }

        void DoAction(int32 action)
        {
            if (action == ACTION_DRAGON_PORTAL_CLOSED)
                HasPortalOpen = false;
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Talk(SAY_VESPERON_SLAY);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
            {
                sartharion_dragonAI::UpdateAI(diff);
                return;
            }

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SHADDOW_BREATH:
                        Talk(SAY_VESPERON_BREATH);
                        DoCastVictim(RAID_MODE(SPELL_SHADOW_BREATH, SPELL_SHADOW_BREATH_H));
                        events.ScheduleEvent(EVENT_SHADDOW_BREATH, urand(20000,25000));
                        break;
                    case EVENT_SHADDOW_FISSURE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200, true))
                            DoCast(target, RAID_MODE(SPELL_SHADOW_FISSURE, SPELL_SHADOW_FISSURE));
                        events.ScheduleEvent(EVENT_SHADDOW_FISSURE, urand(15000,20000));
                        break;
                    case EVENT_OPEN_PORTAL:
                        if (!HasPortalOpen)
                        {
                            OpenPortal();
                            HasPortalOpen = true;
                            DoRaidWhisper(WHISPER_OPEN_PORTAL);

                            if (instance->GetBossState(BOSS_SARTHARION) == IN_PROGRESS)
                            {
                                me->SummonCreature(NPC_ACOLYTE_OF_VESPERON, AcolyteofVesperon2, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 20000);
                                DoCast(SPELL_TWILIGHT_TORMENT_VESP_ACO);
                            }
                            else
                            {
                                me->SummonCreature(NPC_ACOLYTE_OF_VESPERON, AcolyteofVesperon, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 20000);
                                DoCast(SPELL_TWILIGHT_TORMENT_VESP);
                            }
                            events.ScheduleEvent(EVENT_OPEN_PORTAL, urand(60000,70000));
                        }
                        else
                            events.ScheduleEvent(EVENT_OPEN_PORTAL, 10000);
                }
            }
            // Don't attack current target if he's not visible for us.
            if (me->GetVictim() && me->GetVictim()->HasAura(SPELL_TWILIGHT_SHIFT))
                me->GetThreatManager().ModifyThreatByPercent(me->GetVictim(), -100);

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_vesperonAI(creature);
    }
};

class npc_acolyte_of_shadron : public CreatureScript
{
public:
    npc_acolyte_of_shadron() : CreatureScript("npc_acolyte_of_shadron") { }

    struct npc_acolyte_of_shadronAI : public ScriptedAI
    {
        npc_acolyte_of_shadronAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            me->SetPhaseMask(PHASEMASK_NORMAL, false);
            me->AddAura(SPELL_TWILIGHT_SHIFT_ENTER, me);
        }

        void JustDied(Unit* /*killer*/)
        {
            if (instance)
            {
                if (Creature* shadron = ObjectAccessor::GetCreature(*me, ObjectGuid(instance ? ObjectGuid(instance->GetGuidData(DATA_SHADRON)) : ObjectGuid::Empty)))
                    shadron->AI()->DoAction(ACTION_DRAGON_PORTAL_CLOSED);

                DoCastAOE(SPELL_TWILIGHT_SHIFT_REMOVAL_ALL, true);

                if (instance->GetBossState(BOSS_SARTHARION) == IN_PROGRESS)
                {
                    //not solo fight, so main boss has deduff
                    if (Creature* sartharion = ObjectAccessor::GetCreature(*me, ObjectGuid(instance ? ObjectGuid(instance->GetGuidData(DATA_SARTHARION)) : ObjectGuid::Empty)))
                        sartharion->RemoveAurasDueToSpell(SPELL_GIFT_OF_TWILIGTH_SAR);
                }
                else
                    //event not in progress, then solo fight and must remove debuff mini-boss
                    if (Creature* shadron = ObjectAccessor::GetCreature(*me, ObjectGuid(instance ? ObjectGuid(instance->GetGuidData(DATA_SHADRON)) : ObjectGuid::Empty)))
                        shadron->RemoveAurasDueToSpell(SPELL_GIFT_OF_TWILIGTH_SHA);
            }
        }

    private:
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_acolyte_of_shadronAI(creature);
    }
};

class npc_acolyte_of_vesperon : public CreatureScript
{
public:
    npc_acolyte_of_vesperon() : CreatureScript("npc_acolyte_of_vesperon") { }

    struct npc_acolyte_of_vesperonAI : public ScriptedAI
    {
        npc_acolyte_of_vesperonAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            me->SetPhaseMask(PHASEMASK_NORMAL, false);
            me->AddAura(SPELL_TWILIGHT_SHIFT_ENTER, me);
        }

        void JustDied(Unit* /*killer*/)
        {
            if (instance)
            {
                if (Creature* vesperon = ObjectAccessor::GetCreature(*me, ObjectGuid(instance ? ObjectGuid(instance->GetGuidData(DATA_VESPERON)) : ObjectGuid::Empty)))
                    vesperon->AI()->DoAction(ACTION_DRAGON_PORTAL_CLOSED);

                DoCastAOE(SPELL_TWILIGHT_SHIFT_REMOVAL_ALL, true);

                // remove twilight torment on Vesperon
                if (instance->GetBossState(BOSS_SARTHARION) == IN_PROGRESS)
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_TWILIGHT_TORMENT_VESP_ACO);
                else
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_TWILIGHT_TORMENT_VESP);
            }
        }

    private:
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_acolyte_of_vesperonAI(creature);
    }
};

class npc_twilight_eggs : public CreatureScript
{
public:
    npc_twilight_eggs() : CreatureScript("npc_twilight_eggs") { }

    struct npc_twilight_eggsAI : public PassiveAI
    {
        npc_twilight_eggsAI(Creature* creature) : PassiveAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            me->AddAura(SPELL_TWILIGHT_SHIFT_ENTER, me);
            HatchEggTimer = 20000;
        }

         void JustDied(Unit* /*killer*/)
         {
             if (Creature* tenebron = ObjectAccessor::GetCreature(*me, ObjectGuid(instance ? ObjectGuid(instance->GetGuidData(DATA_TENEBRON)) : ObjectGuid::Empty)))
                 tenebron->AI()->DoAction(ACTION_TENEBRON_EGG_KILLED);
         }

        void SpawnWhelps()
        {
            if (instance)
            {
                if (instance->GetBossState(BOSS_SARTHARION) == IN_PROGRESS)
                {
                    if (Creature* whelp = me->SummonCreature(NPC_SHARTHARION_TWILIGHT_WHELP, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30*IN_MILLISECONDS))
                        whelp->SetInCombatWithZone();
                }
                else
                    if (Creature* whelp = me->SummonCreature(NPC_TWILIGHT_WHELP, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30*IN_MILLISECONDS))
                        whelp->SetInCombatWithZone();
            }
            me->DisappearAndDie();
        }

        void UpdateAI(uint32 diff)
        {
            if (HatchEggTimer <= diff)
            {
                if (Creature* tenebron = instance->instance->GetCreature(ObjectGuid(instance->GetGuidData(DATA_TENEBRON))))
                    tenebron->AI()->DoAction(ACTION_DRAGON_PORTAL_CLOSED);

                SpawnWhelps();
            }
            else
                HatchEggTimer -= diff;
        }

    private:
        uint32 HatchEggTimer;
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_twilight_eggsAI(creature);
    }
};

class npc_twilight_whelp : public CreatureScript
{
public:
    npc_twilight_whelp() : CreatureScript("npc_twilight_whelp") { }

    struct npc_twilight_whelpAI : public ScriptedAI
    {
        npc_twilight_whelpAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 FadeArmorTimer;

        void Reset()
        {
            me->RemoveAllAuras();
            me->SetPhaseMask(PHASEMASK_NORMAL, true);
            me->SetInCombatWithZone();
            FadeArmorTimer = urand(1000, 3000);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (FadeArmorTimer <= diff)
            {
                DoCastVictim(SPELL_FADE_ARMOR);
                FadeArmorTimer = urand(3500, 7500);
            }
            else
                FadeArmorTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_twilight_whelpAI(creature);
    }
};

class npc_flame_tsunami : public CreatureScript
{
public:
    npc_flame_tsunami() : CreatureScript("npc_flame_tsunami") { }

    struct npc_flame_tsunamiAI : public PassiveAI
    {
        npc_flame_tsunamiAI(Creature* creature) : PassiveAI(creature)
        {
            me->AddAura(SPELL_FLAME_TSUNAMI, me);
            me->AddAura(SPELL_FLAME_TSUNAMI_DMG_AURA, me);
        }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->SetDisplayId(11686);

            MoveTimer = 5000;
            ShrinkTimer = 12000;
            
            side = SIDE_LEFT;
            number = 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
            case DATA_SIDE:
                side = data;
                break;
            case DATA_NUMBER:
                number = data;
                break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (MoveTimer <= diff)
            {
                if (side == SIDE_LEFT)
                    me->GetMotionMaster()->MovePoint(WAYPOINT_TSUNAMI, FlameLeftDirection[number]);
                else
                    me->GetMotionMaster()->MovePoint(WAYPOINT_TSUNAMI, FlameRightDirection[number]);
                MoveTimer = 15000;
            }
            else
                MoveTimer -= diff;
            
            if (ShrinkTimer <= diff)
            {
                me->SetObjectScale(me->GetObjectSize()*0.7);
                ShrinkTimer = 100;
            }
            else
                ShrinkTimer -= diff;

        }
    private:
        uint32 ShrinkTimer;
        uint32 MoveTimer;
        uint32 side;
        uint32 number;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_flame_tsunamiAI(creature);
    }
};

class npc_twilight_fissure : public CreatureScript
{
public:
    npc_twilight_fissure() : CreatureScript("npc_twilight_fissure") { }

    struct npc_twilight_fissureAI : public ScriptedAI
    {
        npc_twilight_fissureAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
        }

        uint32 VoidBlastTimer;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            VoidBlastTimer = 5000;
        }

        void UpdateAI(uint32 diff)
        {
            if (VoidBlastTimer <= diff)
            {
                DoCast(RAID_MODE(SPELL_VOID_BLAST, SPELL_VOID_BLAST_H));
                me->DespawnOrUnsummon();

            } else VoidBlastTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_twilight_fissureAI(creature);
    }
};

class spell_twilight_shift_remove : public SpellScriptLoader
{
    public:
        spell_twilight_shift_remove() : SpellScriptLoader("spell_twilight_shift_remove") { }

        class spell_twilight_shift_removeAuraScript : public AuraScript
        {
            PrepareAuraScript(spell_twilight_shift_removeAuraScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_TWILIGHT_SHIFT_ENTER) || !sSpellMgr->GetSpellInfo(SPELL_TWILIGHT_SHIFT))
                    return false;
                return true;
            }

            void HandleAfterEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* target = GetTarget())
                {
                    target->RemoveAurasDueToSpell(SPELL_TWILIGHT_SHIFT_ENTER);
                    target->RemoveAurasDueToSpell(SPELL_TWILIGHT_SHIFT);
                }
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_twilight_shift_removeAuraScript::HandleAfterEffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_twilight_shift_removeAuraScript();
        }
};

class spell_gen_lava_strike_57591 : public SpellScriptLoader
{
public:
    spell_gen_lava_strike_57591() : SpellScriptLoader("spell_gen_lava_strike_57591") { }

    class spell_gen_lava_strike_57591_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_gen_lava_strike_57591_SpellScript)

        enum Spells
        {
            SPELL_TRIGGERED = 57572,
        };

        bool Validate(SpellInfo const * /*spellEntry*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_TRIGGERED))
                return false;
            return true;
        }

        void HandleOnHit()
        {
            Player* target = GetHitPlayer() ;
            if (!target)
                return;

            GetCaster()->CastSpell(target, SPELL_TRIGGERED, true);
            InstanceScript* instance = GetCaster()->GetInstanceScript();
            if (instance)
                if (Creature* sartharion = ObjectAccessor::GetCreature(*GetCaster(), ObjectGuid(instance->GetGuidData(DATA_SARTHARION))))
                    if (boss_sartharion::boss_sartharionAI* sartharionAI = CAST_AI(boss_sartharion::boss_sartharionAI, sartharion->AI()))
                        sartharionAI->PlayerHitByVolcano(target->GetGUID());
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_gen_lava_strike_57591_SpellScript::HandleOnHit);
        }
    };

    SpellScript *GetSpellScript() const
    {
        return new spell_gen_lava_strike_57591_SpellScript();
    }
};

class spell_gen_lava_strike_57572 : public SpellScriptLoader
{
public:
    spell_gen_lava_strike_57572() : SpellScriptLoader("spell_gen_lava_strike_57572") { }

    class spell_gen_lava_strike_57572_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_gen_lava_strike_57572_SpellScript)

        bool Load()
        {
            if (roll_chance_i(80))
                return true;
            return false;
        }

        void DoNothing(SpellEffIndex effIndex)
        {
             PreventHitDefaultEffect(effIndex);
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_gen_lava_strike_57572_SpellScript::DoNothing, EFFECT_0, SPELL_EFFECT_SUMMON);
        }
    };

    SpellScript *GetSpellScript() const
    {
        return new spell_gen_lava_strike_57572_SpellScript();
    }
};

class spell_gen_lava_strike_57578 : public SpellScriptLoader
{
public:
    spell_gen_lava_strike_57578() : SpellScriptLoader("spell_gen_lava_strike_57578") { }

    class spell_gen_lava_strike_57578_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_gen_lava_strike_57578_SpellScript)
        enum Spells
        {
            SPELL_TRIGGERED = 57571,
        };

        Unit* target;

        bool Load()
        {
            // initialize variable
            target = NULL;
            return true;
        }

        bool Validate(SpellInfo const * /*spellEntry*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_TRIGGERED))
                return false;
            return true;
        }

        void FilterTargets(std::list<WorldObject*>& unitList)
        {
            if (unitList.empty())
                return;

            std::list<WorldObject*>::iterator itr = unitList.begin();
            std::advance(itr, rand32() % unitList.size());
            target = (*itr)->ToUnit();
            unitList.clear();
            unitList.push_back(target);
        }

        void HandleDummy(SpellEffIndex effIndex)
        {
            Unit* caster = GetCaster();
            target = GetHitUnit();

            if (!caster || !target)
                return;

            PreventHitDefaultEffect(effIndex);
            caster->CastSpell(target, SPELL_TRIGGERED, true);
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_gen_lava_strike_57578_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            OnEffectHitTarget += SpellEffectFn(spell_gen_lava_strike_57578_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript *GetSpellScript() const
    {
        return new spell_gen_lava_strike_57578_SpellScript();
    }
};

class spell_gen_flame_tsunami : public SpellScriptLoader
{
public:
    spell_gen_flame_tsunami() : SpellScriptLoader("spell_gen_flame_tsunami") { }

    class spell_gen_flame_tsunami_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_gen_flame_tsunami_SpellScript)

        bool Load()
        {
            if (Unit* caster = GetCaster())
            {
                //! HACK
                if (SpellValue* values = const_cast<SpellValue*>(GetSpellValue()))
                {
                    if (caster->GetMap())
                    {
                        if (caster->GetMap()->GetSpawnMode() == RAID_DIFFICULTY_25MAN_NORMAL)
                        {
                            values->EffectBasePoints[EFFECT_0] = 3000;
                            values->EffectBasePoints[EFFECT_1] = 3000;
                        }
                        else
                        {
                            values->EffectBasePoints[EFFECT_0] = 1500;
                            values->EffectBasePoints[EFFECT_1] = 1500;
                        }
                    }
                }
            }
            return true;
        }

        void Register(){}
    };

    SpellScript *GetSpellScript() const
    {
        return new spell_gen_flame_tsunami_SpellScript();
    }
};

class InCombatArea
{
    public:
        bool operator() (WorldObject* object)
        {
            float fX = object->GetPositionX();
            float fY = object->GetPositionY();
            return (fX > 3218.86f && fX < 3275.69f && fY < 572.40f && fY > 484.68f);
        }
};

class spell_pyrobuffet_targeting : public SpellScriptLoader
{
    public:
        spell_pyrobuffet_targeting() : SpellScriptLoader("spell_pyrobuffet_targeting") { }

        class spell_pyrobuffet_targeting_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pyrobuffet_targeting_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if (InCombatArea());
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pyrobuffet_targeting_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pyrobuffet_targeting_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pyrobuffet_targeting_SpellScript::FilterTargets, EFFECT_2, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pyrobuffet_targeting_SpellScript();
        }
};

class go_twilight_portal : public GameObjectScript
{
public:
    go_twilight_portal() : GameObjectScript("go_twilight_portal") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/)
    {
        player->CastSpell(player, SPELL_TWILIGHT_SHIFT_ENTER, true);
        player->CastSpell(player, SPELL_TWILIGHT_SHIFT, true);

        player->RemoveAurasDueToSpell(SPELL_TWILIGHT_TORMENT_VESP);
        player->RemoveAurasDueToSpell(SPELL_TWILIGHT_TORMENT_VESP_ACO);
        return true;
    }
};

class go_normal_portal : public GameObjectScript
{
public:
    go_normal_portal() : GameObjectScript("go_normal_portal") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/)
    {
        player->RemoveAurasDueToSpell(SPELL_TWILIGHT_SHIFT_ENTER);
        player->RemoveAurasDueToSpell(SPELL_TWILIGHT_SHIFT);
        player->CastSpell(player, SPELL_TWILIGHT_RESIDUE, true);
        return true;
    }
};

class achievement_twilight_assist : public AchievementCriteriaScript
{
    public:
        achievement_twilight_assist() : AchievementCriteriaScript("achievement_twilight_assist") { }

        bool OnCheck(Player* /*player*/, Unit* target)
        {
            if (!target)
                return false;

            if (Creature* Sartharion = target->ToCreature())
                if (Sartharion->AI()->GetData(DATA_DRAGONS_STILL_ALIVE) >= 1)
                    return true;

            return false;
        }
};

class achievement_twilight_duo : public AchievementCriteriaScript
{
    public:
        achievement_twilight_duo() : AchievementCriteriaScript("achievement_twilight_duo") { }

        bool OnCheck(Player* /*player*/, Unit* target)
        {
            if (!target)
                return false;

            if (Creature* Sartharion = target->ToCreature())
                if (Sartharion->AI()->GetData(DATA_DRAGONS_STILL_ALIVE) >= 2)
                    return true;

            return false;
        }
};

class achievement_twilight_zone : public AchievementCriteriaScript
{
    public:
        achievement_twilight_zone() : AchievementCriteriaScript("achievement_twilight_zone") { }

        bool OnCheck(Player* /*player*/, Unit* target)
        {
            if (!target)
                return false;

            if (Creature* Sartharion = target->ToCreature())
                if (Sartharion->AI()->GetData(DATA_DRAGONS_STILL_ALIVE) >= 3)
                    return true;

            return false;
        }
};

void AddSC_boss_sartharion()
{
    new boss_sartharion();

    new npc_vesperon();
    new npc_shadron();
    new npc_tenebron();

    new npc_acolyte_of_shadron();
    new npc_acolyte_of_vesperon();

    new npc_twilight_eggs();
    new npc_flame_tsunami();
    new npc_twilight_fissure();
    new npc_twilight_whelp();

    new spell_twilight_shift_remove();
    new spell_gen_lava_strike_57591();
    new spell_gen_lava_strike_57572();
    new spell_gen_lava_strike_57578();
    new spell_gen_flame_tsunami();
    new spell_pyrobuffet_targeting();

    new go_twilight_portal();
    new go_normal_portal();

    new achievement_twilight_assist();
    new achievement_twilight_duo();
    new achievement_twilight_zone();
}
