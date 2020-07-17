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
#include "PlayerAI.h"
#include "Player.h"
#include "CreatureTextMgr.h"
#include "DBCStores.h"

enum Yells
{
    //Gorkun
    SAY_GORKUN_INTRO_2              = 0,
    SAY_GORKUN_OUTRO_1              = 1,
    SAY_GORKUN_OUTRO_2              = 2,

    //Tyrannus
    SAY_AMBUSH_1                    = 3,
    SAY_AMBUSH_2                    = 4,
    SAY_GAUNTLET_START              = 5,
    SAY_TYRANNUS_INTRO_1            = 6,
    SAY_TYRANNUS_INTRO_3            = 7,
    SAY_AGGRO                       = 8,
    SAY_SLAY                        = 9,
    SAY_DEATH                       = 10,
    SAY_MARK_RIMEFANG_1             = 11,
    SAY_MARK_RIMEFANG_2             = 12,
    SAY_DARK_MIGHT_1                = 13,
    SAY_DARK_MIGHT_2                = 14,

    //Jaina
    SAY_JAYNA_OUTRO_3               = 3,
    SAY_JAYNA_OUTRO_4               = 4,
    SAY_JAYNA_OUTRO_5               = 5,

    //Sylvanas
    SAY_SYLVANAS_OUTRO_3            = 3,
    SAY_SYLVANAS_OUTRO_4            = 4,

    // General
    GUARD_ATTACK                    = 0,
    GUARD_TYRANNUS_WIN_1            = 1,
    GUARD_TYRANNUS_WIN_2            = 2
};

enum Spells
{
    SPELL_OVERLORD_BRAND            = 69172,
    SPELL_OVERLORD_BRAND_HEAL       = 69190,
    SPELL_OVERLORD_BRAND_DAMAGE     = 69189,
    SPELL_FORCEFUL_SMASH            = 69155,
    SPELL_UNHOLY_POWER              = 69167,
    SPELL_MARK_OF_RIMEFANG          = 69275,
    SPELL_HOARFROST                 = 69246,

    SPELL_ICY_BLAST                 = 69232,
    SPELL_ICY_BLAST_AURA            = 69238,

    SPELL_EJECT_ALL_PASSENGERS      = 50630,
    SPELL_FULL_HEAL                 = 43979,

    SPELL_ICICLE                    = 69424,
    SPELL_CALL_OF_SYLVANAS          = 70639,
    SPELL_FROST_BOMB                = 70521,
    SPELL_ARCANE_FORM               = 70573,
    SPELL_ICEBLOCK                  = 46604,
};

enum Events
{
    EVENT_OVERLORD_BRAND    = 1,
    EVENT_FORCEFUL_SMASH    = 2,
    EVENT_UNHOLY_POWER      = 3,
    EVENT_MARK_OF_RIMEFANG  = 4,

    // Rimefang
    EVENT_MOVE_NEXT         = 5,
    EVENT_HOARFROST         = 6,
    EVENT_ICY_BLAST         = 7,

    EVENT_INTRO_1           = 8,
    EVENT_INTRO_2           = 9,
    EVENT_INTRO_3           = 10,
    EVENT_COMBAT_START      = 11,
    EVENT_INTRO_0           = 12,
    EVENT_MOVE_GUARD        = 13,
    EVENT_MOVE_ELITIST      = 14,
    EVENT_SPAWN_TUNNEL_ADDS = 15,

    // Silvanas/Jaina Events
    EVENT_GENERAL_TALK_1    = 16,
    EVENT_GUARDS_EMOTE      = 17,
    EVENT_GENERAL_TALK_2    = 18,
    EVENT_LADY_TALK_1       = 19,
    EVENT_LADY_PORT         = 20,
    EVENT_SINDRAGOSA_MOVE   = 21,
    EVENT_SINDRAGOSA_KILL   = 22,
    EVENT_LADY_WP           = 23,
    EVENT_LADY_TALK_2       = 24,
    EVENT_SPAWN_GUARDS      = 25,
    EVENT_OVERLORD_BRAND_REMOVE = 26
};

enum Phases
{
    PHASE_NONE      = 0,
    PHASE_INTRO     = 1,
    PHASE_COMBAT    = 2,
    PHASE_OUTRO     = 3
};

enum Actions
{
    ACTION_START_INTRO      = 1,
    ACTION_START_RIMEFANG   = 2,
    ACTION_START_OUTRO      = 3,
    ACTION_END_COMBAT       = 4,
    ACTION_CAVE_AMBUSH      = 5,
    ACTION_CAVE_END         = 6
};

enum Misc
{
    MAX_HIT_DAMAGE          = 100,
    ATTACK_FACTION          = 250
};

#define GUID_HOARFROST 1
#define ACHIEVEMENT_DO_NOT_LOOK_UP 4525

static const Position rimefangPos[10] =
{
    {1017.299f, 168.9740f, 642.9259f, 0.000000f},
    {1047.868f, 126.4931f, 665.0453f, 0.000000f},
    {1069.828f, 138.3837f, 665.0453f, 0.000000f},
    {1063.042f, 164.5174f, 665.0453f, 0.000000f},
    {1031.158f, 195.1441f, 665.0453f, 0.000000f},
    {1019.087f, 197.8038f, 665.0453f, 0.000000f},
    {967.6233f, 168.9670f, 665.0453f, 0.000000f},
    {969.1198f, 140.4722f, 665.0453f, 0.000000f},
    {986.7153f, 141.6424f, 665.0453f, 0.000000f},
    {1012.601f, 142.4965f, 665.0453f, 0.000000f},
}; 

Position spawnPoint           = { 1066.827026f, 92.398453f,  631.269470f, 2.019531f };
Position SpawnPointSindragosa = { 842.8611f,    194.5556f,   531.6536f,   6.108f    };
Position GeneralWatchPoint    = { 986.115845f, 168.704178f, 628.156250f, 6.229769f };
Position RimefangDespawnPoint = { 1248.29f,     145.924f,    733.914f,    0.793231f };
Position SindragosaWatchPoint = { 900.106f,     181.677f,    659.374f,    6.163478f };

struct startingPosition
{
    uint32 entry[2];
    Position movePosition;
} startingPositions[] =
{
    // guards
    { { NPC_FREED_SLAVE_2_ALLIANCE, NPC_FREED_SLAVE_3_HORDE }, { 1059.130859f, 129.492050f, 628.156250f, 2.027885f } }, // Melee
    { { NPC_FREED_SLAVE_2_ALLIANCE, NPC_FREED_SLAVE_3_HORDE }, { 1057.530151f, 128.049194f, 628.156250f, 1.882581f } }, // Melee
    { { NPC_FREED_SLAVE_2_ALLIANCE, NPC_FREED_SLAVE_3_HORDE }, { 1043.914795f, 121.489182f, 628.156250f, 2.047514f } }, // Melee
    { { NPC_FREED_SLAVE_2_ALLIANCE, NPC_FREED_SLAVE_3_HORDE }, { 1047.155151f, 123.162659f, 628.156250f, 2.047514f } }, // Melee
    { { NPC_FREED_SLAVE_2_ALLIANCE, NPC_FREED_SLAVE_3_HORDE }, { 1049.674072f, 124.463539f, 628.156250f, 2.047514f } }, // Melee
    { { NPC_FREED_SLAVE_2_ALLIANCE, NPC_FREED_SLAVE_3_HORDE }, { 1051.997070f, 125.823349f, 628.156250f, 1.953267f } }, // Melee
    { { NPC_FREED_SLAVE_2_ALLIANCE, NPC_FREED_SLAVE_3_HORDE }, { 1038.808472f, 116.224686f, 628.156982f, 1.953267f } }, // Melee
    { { NPC_FREED_SLAVE_2_ALLIANCE, NPC_FREED_SLAVE_3_HORDE }, { 1041.133423f, 117.159966f, 628.156982f, 1.953267f } }, // Melee
    { { NPC_FREED_SLAVE_3_ALLIANCE, NPC_FREED_SLAVE_1_HORDE }, { 1040.846069f, 115.154816f, 628.156982f, 1.953267f } }, // Mage
    { { NPC_FREED_SLAVE_3_ALLIANCE, NPC_FREED_SLAVE_1_HORDE }, { 1042.025635f, 110.008507f, 628.440796f, 1.921851f } }, // Mage
    { { NPC_FREED_SLAVE_1_ALLIANCE, NPC_FREED_SLAVE_2_HORDE }, { 1044.931885f, 110.885567f, 628.313904f, 2.051441f } }, // Shami Chainheal
    { { NPC_FREED_SLAVE_1_ALLIANCE, NPC_FREED_SLAVE_2_HORDE }, { 1050.821533f, 116.580193f, 628.156250f, 1.925777f } }, // Shami Chainheal
    { { NPC_FREED_SLAVE_1_ALLIANCE, NPC_FREED_SLAVE_2_HORDE }, { 1054.818726f, 118.061897f, 628.156250f, 1.925777f } }, // Shami Chainheal
    { { NPC_FREED_SLAVE_1_ALLIANCE, NPC_FREED_SLAVE_2_HORDE }, { 1059.453979f, 119.616463f, 628.156250f, 1.925777f } }, // Shami Chainheal
    { { NPC_FREED_SLAVE_3_ALLIANCE, NPC_FREED_SLAVE_1_HORDE }, { 1061.744751f, 120.465576f, 628.156250f, 1.925777f } }, // Mage
    { { NPC_FREED_SLAVE_3_ALLIANCE, NPC_FREED_SLAVE_1_HORDE }, { 1060.008301f, 127.096161f, 628.156250f, 1.921850f } }, // Mage
    { { NPC_FREED_SLAVE_3_ALLIANCE, NPC_FREED_SLAVE_1_HORDE }, { 1056.486084f, 113.563972f, 628.231018f, 1.925777f } }, // Mage
    { { NPC_FREED_SLAVE_3_ALLIANCE, NPC_FREED_SLAVE_1_HORDE }, { 1052.784180f, 112.191757f, 628.156616f, 1.925777f } }, // Mage


    // leads
    { { NPC_MARTIN_VICTUS_2, NPC_GORKUN_IRONSKULL_1 }, { 1046.355713f, 125.106544f, 628.156189f, 2.047514f } },
    { { 0, 0 }, { 0.0f, 0.0f, 0.0f, 0.0f } }
};

struct EndingPosition
{
    uint32 entry[2];
    Position movePosition;
}  EndingPositions[] =
{
    // guards
    { { NPC_FREED_SLAVE_2_ALLIANCE, NPC_FREED_SLAVE_3_HORDE }, { 1042.089844f, 118.854813f, 628.156311f, 1.874737f } }, // Melee
    { { NPC_FREED_SLAVE_2_ALLIANCE, NPC_FREED_SLAVE_3_HORDE }, { 1041.002808f, 126.117165f, 628.156311f, 2.137846f } }, // Melee
    { { NPC_FREED_SLAVE_2_ALLIANCE, NPC_FREED_SLAVE_3_HORDE }, { 1024.322754f, 126.521034f, 628.156311f, 2.463786f } }, // Melee
    { { NPC_FREED_SLAVE_2_ALLIANCE, NPC_FREED_SLAVE_3_HORDE }, { 1022.499390f, 189.018997f, 628.156311f, 3.669370f } }, // Melee
    { { NPC_FREED_SLAVE_2_ALLIANCE, NPC_FREED_SLAVE_3_HORDE }, { 1035.839111f, 120.174759f, 628.156311f, 2.173186f } }, // Melee
    { { NPC_FREED_SLAVE_2_ALLIANCE, NPC_FREED_SLAVE_3_HORDE }, { 1056.420898f, 132.196472f, 628.156311f, 2.004328f } }, // Melee
    { { NPC_FREED_SLAVE_2_ALLIANCE, NPC_FREED_SLAVE_3_HORDE }, { 1057.168457f, 128.240829f, 628.156311f, 2.004328f } }, // Melee
    { { NPC_FREED_SLAVE_2_ALLIANCE, NPC_FREED_SLAVE_3_HORDE }, { 1061.112793f, 151.921921f, 628.156311f, 3.154937f } }, // Melee
    { { NPC_FREED_SLAVE_3_ALLIANCE, NPC_FREED_SLAVE_1_HORDE }, { 1020.393738f, 126.399719f, 628.156311f, 2.459859f } }, // Mage
    { { NPC_FREED_SLAVE_3_ALLIANCE, NPC_FREED_SLAVE_1_HORDE }, { 1032.632446f, 133.378723f, 628.156311f, 2.569815f } }, // Mage
    { { NPC_FREED_SLAVE_1_ALLIANCE, NPC_FREED_SLAVE_2_HORDE }, { 1042.381592f, 152.575714f, 628.156311f, 3.154937f } }, // Shami Chainheal
    { { NPC_FREED_SLAVE_1_ALLIANCE, NPC_FREED_SLAVE_2_HORDE }, { 1052.329956f, 167.582108f, 628.156311f, 2.954661f } }, // Shami Chainheal
    { { NPC_FREED_SLAVE_1_ALLIANCE, NPC_FREED_SLAVE_2_HORDE }, { 1050.470947f, 163.390915f, 628.156311f, 2.719041f } }, // Shami Chainheal
    { { NPC_FREED_SLAVE_1_ALLIANCE, NPC_FREED_SLAVE_2_HORDE }, { 1014.536987f, 145.644577f, 628.156250f, 2.467710f } }, // Shami Chainheal
    { { NPC_FREED_SLAVE_3_ALLIANCE, NPC_FREED_SLAVE_1_HORDE }, { 1028.568237f, 164.450928f, 628.156311f, 3.037127f } }, // Mage
    { { NPC_FREED_SLAVE_3_ALLIANCE, NPC_FREED_SLAVE_1_HORDE }, { 1029.287109f, 168.275467f, 628.156311f, 3.037127f } }, // Mage
    { { NPC_FREED_SLAVE_3_ALLIANCE, NPC_FREED_SLAVE_1_HORDE }, { 1043.139282f, 190.213654f, 628.156311f, 3.229550f } }, // Mage
    { { NPC_FREED_SLAVE_3_ALLIANCE, NPC_FREED_SLAVE_1_HORDE }, { 1056.040039f, 188.726288f, 628.156311f, 4.077777f } }, // Mage
};

static Position const miscPos = { 1018.376f, 167.2495f, 628.2811f, 0.000000f };   //tyrannus combat start position

class boss_tyrannus : public CreatureScript
{
    public:
        boss_tyrannus() : CreatureScript("boss_tyrannus") { }

        struct boss_tyrannusAI : public BossAI
        {
            boss_tyrannusAI(Creature* creature) : BossAI(creature, DATA_TYRANNUS)
            {
            }

            void InitializeAI()
            {
                if (instance->GetBossState(DATA_TYRANNUS) != DONE)
                    Reset();
                else
                    me->DespawnOrUnsummon();
            }

            void Reset()
            {
                events.Reset();
                TalkAmbush = false;
                StartFight = false;
                CastOverlordBrand = false;
                events.SetPhase(PHASE_NONE);
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_DECREASE_SPEED, false);
                instance->SetBossState(DATA_TYRANNUS, NOT_STARTED);

                std::list<Creature*> AddList;
                me->GetCreatureListWithEntryInGrid(AddList, NPC_SKELETON,               300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_FALLEN_WARRIOR,         100.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_SORCERER,               300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_2_ALLIANCE, 300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_3_ALLIANCE, 300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_1_ALLIANCE, 300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_3_HORDE,    300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_1_HORDE,    300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_2_HORDE,    300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_MARTIN_VICTUS_2,        300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_GORKUN_IRONSKULL_1,     300.0f);
                if (!AddList.empty())
                    for (std::list<Creature*>::iterator itr = AddList.begin(); itr != AddList.end(); itr++)
                        (*itr)->DespawnOrUnsummon();
            }

            Creature* GetRimefang()
            {
                return ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_RIMEFANG)));
            }

            void EnterCombat(Unit* /*who*/)
            {
                Talk(SAY_AGGRO);
            }

            void AttackStart(Unit* victim)
            {
                if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                    return;

                if (victim && me->Attack(victim, true) && !events.IsInPhase(PHASE_INTRO))
                    me->GetMotionMaster()->MoveChase(victim);
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                instance->SetBossState(DATA_TYRANNUS, FAIL);
                if (Creature* rimefang = GetRimefang())
                    rimefang->AI()->EnterEvadeMode();

                me->DespawnOrUnsummon();
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);
                instance->SetBossState(DATA_TYRANNUS, DONE);

                // Prevent corpse despawning
                if (TempSummon* summ = me->ToTempSummon())
                    summ->SetTempSummonType(TEMPSUMMON_DEAD_DESPAWN);

                // Stop Combat for Rimefang and Despawn him
                if (Creature* rimefang = GetRimefang())
                {
                    rimefang->AI()->DoAction(ACTION_END_COMBAT);
                    rimefang->SetWalk(false);
                    rimefang->DespawnOrUnsummon(25 * IN_MILLISECONDS);
                    rimefang->GetMotionMaster()->MovePoint(0, RimefangDespawnPoint, true);
                }                    

                // Despawn all Combat Horde and Alliance Adds
                std::list<Creature*> AddList;
                me->GetCreatureListWithEntryInGrid(AddList, NPC_SKELETON,            300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_FALLEN_WARRIOR,      300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_SORCERER,            300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_2_ALLIANCE, 300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_3_ALLIANCE, 300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_1_ALLIANCE, 300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_3_HORDE, 300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_1_HORDE, 300.0f);
                me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_2_HORDE, 300.0f);
                if (!AddList.empty())
                    for (std::list<Creature*>::iterator itr = AddList.begin(); itr != AddList.end(); itr++)
                        (*itr)->DespawnOrUnsummon();

                // Move General to WatchPosition
                std::list<Creature*> LeadList;
                me->GetCreatureListWithEntryInGrid(LeadList, NPC_MARTIN_VICTUS_2,    300.0f);
                me->GetCreatureListWithEntryInGrid(LeadList, NPC_GORKUN_IRONSKULL_1, 300.0f);
                if (!LeadList.empty())
                    for (std::list<Creature*>::iterator itr = LeadList.begin(); itr != LeadList.end(); itr++)
                    {
                        (*itr)->GetMotionMaster()->MovePoint(0, GeneralWatchPoint, true);
                        (*itr)->SetWalk(false);
                    }                        

                // Spawn Sindragosa
                me->SummonCreature(NPC_SINDRAGOSA, SpawnPointSindragosa, TEMPSUMMON_DEAD_DESPAWN, 300000);
            }

            void DoAction(int32 actionId)
            {
                switch (actionId)
                {
                    case ACTION_START_INTRO: 
                        if (!StartFight)
                        {
                            StartFight = true;
                            events.SetPhase(PHASE_INTRO);
                            events.ScheduleEvent(EVENT_INTRO_0, 1 * IN_MILLISECONDS, 0, PHASE_INTRO);
                            events.ScheduleEvent(EVENT_MOVE_GUARD, 10 * IN_MILLISECONDS, 0, PHASE_INTRO);
                            events.ScheduleEvent(EVENT_INTRO_1, 12 * IN_MILLISECONDS, 0, PHASE_INTRO);
                            events.ScheduleEvent(EVENT_MOVE_ELITIST, 15 * IN_MILLISECONDS, 0, PHASE_INTRO);
                            events.ScheduleEvent(EVENT_INTRO_2, 22 * IN_MILLISECONDS, 0, PHASE_INTRO);
                            events.ScheduleEvent(EVENT_INTRO_3, 34 * IN_MILLISECONDS, 0, PHASE_INTRO);
                            events.ScheduleEvent(EVENT_COMBAT_START, 36 * IN_MILLISECONDS, 0, PHASE_INTRO);
                            instance->SetBossState(DATA_TYRANNUS, IN_PROGRESS);
                        }
                        break;
                    case ACTION_CAVE_AMBUSH:
                        if (!TalkAmbush)
                        {
                            sCreatureTextMgr->SendChat(me, SAY_GAUNTLET_START, NULL, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_MAP); // yell is too small
                            TalkAmbush = true;
                        }
                        break;
                    case ACTION_CAVE_END:
                        if (instance->GetData(DATA_DO_NOT_LOOK_UP) == 0)
                        {
                            if (AchievementEntry const *AchievDoNotLookUp = sAchievementStore.LookupEntry(ACHIEVEMENT_DO_NOT_LOOK_UP))
                            {
                                Map* map = me->GetMap();
                                if (map && map->IsDungeon() && map->IsHeroic())
                                {
                                    for (Player& player : map->GetPlayers())
                                        player.CompletedAchievement(AchievDoNotLookUp);
                                }
                            }
                        }
                        break;
                    default:
                        break;
                }
            }

            bool CanAIAttack(Unit const* victim) const
            {
                return victim->GetTypeId() == TYPEID_PLAYER;
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() && !events.IsInPhase(PHASE_INTRO))
                    return;

                int32 entryIndex;
                if (instance->GetData(DATA_TEAM_IN_INSTANCE) == ALLIANCE)
                    entryIndex = 0;
                else
                    entryIndex = 1;

                std::list<Creature*> GuardList;
                std::list<Creature*> ElitistList;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_INTRO_0:
                            Talk(SAY_TYRANNUS_INTRO_1);
                            DoAction(ACTION_CAVE_END);
                            for (int8 i = 0; startingPositions[i].entry[entryIndex] != 0; ++i)
                            {
                                if (Creature* summon = me->SummonCreature(startingPositions[i].entry[entryIndex], spawnPoint, TEMPSUMMON_DEAD_DESPAWN))
                                {
                                    summon->GetMotionMaster()->MovePoint(0, startingPositions[i].movePosition);
                                    summon->HandleEmoteCommand(EMOTE_STATE_READY1H);
                                }
                            }
                            break;
                        case EVENT_MOVE_GUARD:
                            me->GetCreatureListWithEntryInGrid(GuardList, NPC_FREED_SLAVE_2_ALLIANCE, 200.0f);
                            me->GetCreatureListWithEntryInGrid(GuardList, NPC_FREED_SLAVE_3_ALLIANCE, 200.0f);
                            me->GetCreatureListWithEntryInGrid(GuardList, NPC_FREED_SLAVE_1_ALLIANCE, 200.0f);
                            me->GetCreatureListWithEntryInGrid(GuardList, NPC_FREED_SLAVE_3_HORDE, 200.0f);
                            me->GetCreatureListWithEntryInGrid(GuardList, NPC_FREED_SLAVE_1_HORDE, 200.0f);
                            me->GetCreatureListWithEntryInGrid(GuardList, NPC_FREED_SLAVE_2_HORDE, 200.0f);
                            if (!GuardList.empty())
                                for (std::list<Creature*>::iterator itr = GuardList.begin(); itr != GuardList.end(); itr++)
                                {
                                    (*itr)->SetOrientation(5.167220f);
                                    (*itr)->setFaction(ATTACK_FACTION); 
                                    (*itr)->SetHomePosition((*itr)->GetPositionX(), (*itr)->GetPositionY(), (*itr)->GetPositionZ(), (*itr)->GetOrientation());
                                }
                            break;
                        case EVENT_INTRO_1:
                            me->GetCreatureListWithEntryInGrid(ElitistList, NPC_MARTIN_VICTUS_2,    200.0f);
                            me->GetCreatureListWithEntryInGrid(ElitistList, NPC_GORKUN_IRONSKULL_1, 200.0f);
                            if (!ElitistList.empty())
                                for (std::list<Creature*>::iterator itr = ElitistList.begin(); itr != ElitistList.end(); itr++)
                                {
                                    (*itr)->setFaction(ATTACK_FACTION);
                                    (*itr)->AI()->Talk(GUARD_ATTACK);
                                }
                            break;
                        case EVENT_MOVE_ELITIST:
                            me->GetCreatureListWithEntryInGrid(ElitistList, NPC_MARTIN_VICTUS_2,    200.0f);
                            me->GetCreatureListWithEntryInGrid(ElitistList, NPC_GORKUN_IRONSKULL_1, 200.0f);
                            if (!ElitistList.empty())
                                for (std::list<Creature*>::iterator itr = ElitistList.begin(); itr != ElitistList.end(); itr++)
                                    (*itr)->GetMotionMaster()->MovePoint(0, 1057.049194f, 112.967766f, 628.301331f);
                            break;
                        case EVENT_INTRO_2:
                            Talk(SAY_TYRANNUS_INTRO_3);
                            break;
                        case EVENT_INTRO_3:
                            me->ExitVehicle();
                            me->GetMotionMaster()->MovePoint(0, miscPos);
                            break;
                        case EVENT_COMBAT_START:
                            if (Creature* rimefang = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_RIMEFANG))))
                                rimefang->AI()->DoAction(ACTION_START_RIMEFANG);    //set rimefang also infight
                            events.SetPhase(PHASE_COMBAT);
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                            me->SetReactState(REACT_AGGRESSIVE);
                            DoCastSelf(SPELL_FULL_HEAL);
                            me->SetInCombatWithZone();
                            DoZoneInCombat();
                            AttackStart(me->FindNearestPlayer(200.0f));
                            events.ScheduleEvent(EVENT_OVERLORD_BRAND, urand(5, 7)*IN_MILLISECONDS);
                            events.ScheduleEvent(EVENT_FORCEFUL_SMASH, urand(14, 16)*IN_MILLISECONDS);
                            events.ScheduleEvent(EVENT_MARK_OF_RIMEFANG, urand(25, 27)*IN_MILLISECONDS);
                            events.ScheduleEvent(EVENT_SPAWN_TUNNEL_ADDS, 1 * IN_MILLISECONDS);
                            break;
                        case EVENT_SPAWN_TUNNEL_ADDS:
                            if (Creature* add = me->SummonCreature(NPC_FALLEN_WARRIOR, 1059.377563f, 95.529877f, 630.705200f, 2.038017f, TEMPSUMMON_CORPSE_DESPAWN, 0))
                                add->AI()->SetData(DATA_SPAWN_TUNNEL_ADDS, DATA_SPAWN_TUNNEL_ADDS);
                            if (Creature* add = me->SummonCreature(NPC_SKELETON, 1063.507935f, 92.039215f, 631.164795f, 1.950838f, TEMPSUMMON_CORPSE_DESPAWN, 0))
                                add->AI()->SetData(DATA_SPAWN_TUNNEL_ADDS, DATA_SPAWN_TUNNEL_ADDS);
                            if (Creature* add = me->SummonCreature(NPC_SKELETON, 1068.021484f, 94.767143f, 631.087097f, 2.026847f, TEMPSUMMON_CORPSE_DESPAWN, 0))
                                add->AI()->SetData(DATA_SPAWN_TUNNEL_ADDS, DATA_SPAWN_TUNNEL_ADDS);
                            if (Creature* add = me->SummonCreature(NPC_SKELETON, 1063.507935f, 92.039215f, 631.164795f, 1.950838f, TEMPSUMMON_CORPSE_DESPAWN, 0))
                                add->AI()->SetData(DATA_SPAWN_TUNNEL_ADDS, DATA_SPAWN_TUNNEL_ADDS);
                            events.ScheduleEvent(EVENT_SPAWN_TUNNEL_ADDS, 20 * IN_MILLISECONDS);
                        case EVENT_OVERLORD_BRAND:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                            {
                                if (!CastOverlordBrand)
                                {
                                    CastOverlordBrand = true;
                                    DoCast(target, SPELL_OVERLORD_BRAND);
                                    events.ScheduleEvent(EVENT_OVERLORD_BRAND_REMOVE, 10 * IN_MILLISECONDS);
                                }
                            }
                            events.ScheduleEvent(EVENT_OVERLORD_BRAND, urand(11, 12)*IN_MILLISECONDS);
                            break;
                        case EVENT_FORCEFUL_SMASH:
                            DoCastVictim(SPELL_FORCEFUL_SMASH);
                            events.ScheduleEvent(EVENT_UNHOLY_POWER, 1*IN_MILLISECONDS);
                            break;
                        case EVENT_UNHOLY_POWER:
                            Talk(SAY_DARK_MIGHT_1);
                            Talk(SAY_DARK_MIGHT_2);
                            DoCastSelf(SPELL_UNHOLY_POWER);
                            events.ScheduleEvent(EVENT_FORCEFUL_SMASH, urand(40, 48)*IN_MILLISECONDS);
                            break;
                        case EVENT_MARK_OF_RIMEFANG:
                            Talk(SAY_MARK_RIMEFANG_1);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                            {
                                Talk(SAY_MARK_RIMEFANG_2, target);
                                DoCast(target, SPELL_MARK_OF_RIMEFANG);
                            }
                            events.ScheduleEvent(EVENT_MARK_OF_RIMEFANG, urand(24, 26)*IN_MILLISECONDS);
                            break;
                        case EVENT_OVERLORD_BRAND_REMOVE:
                            CastOverlordBrand = false;
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady(true);
            }
            private:
                bool TalkAmbush;
                bool StartFight;
                bool CastOverlordBrand;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetPitOfSaronAI<boss_tyrannusAI>(creature);
        }        
};

class boss_rimefang : public CreatureScript
{
    public:
        boss_rimefang() : CreatureScript("boss_rimefang") { }

        struct boss_rimefangAI : public ScriptedAI
        {
            boss_rimefangAI(Creature* creature) : ScriptedAI(creature), _vehicle(creature->GetVehicleKit())
            {
                ASSERT(_vehicle);
            }

            void Reset()
            {
                _events.Reset();
                _events.SetPhase(PHASE_NONE);
                _currentWaypoint = 0;
                _hoarfrostTargetGUID.Clear();
                me->SetCanFly(true);
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }

            void JustReachedHome()
            {
                _vehicle->InstallAllAccessories(false);
            }

            void DoAction(int32 actionId)
            {
                switch (actionId)
                { 
                    case ACTION_START_RIMEFANG:
                        _events.SetPhase(PHASE_COMBAT);
                        me->SetInCombatWithZone();
                        DoZoneInCombat();
                        _events.ScheduleEvent(EVENT_MOVE_NEXT, 500, 0, PHASE_COMBAT);
                        _events.ScheduleEvent(EVENT_ICY_BLAST, 15 * IN_MILLISECONDS, 0, PHASE_COMBAT);
                        break;
                    case ACTION_END_COMBAT:
                        _EnterEvadeMode();
                        break;
                    default:
                        break;
                }                
            }

            void SetGuidData(ObjectGuid guid, uint32 type) override
            {
                if (type == GUID_HOARFROST)
                {
                    _hoarfrostTargetGUID = guid;
                    _events.ScheduleEvent(EVENT_HOARFROST, 1*IN_MILLISECONDS);
                }
            }

            bool CanAIAttack(Unit const* victim) const
            {
                return victim->GetTypeId() == TYPEID_PLAYER;
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() && !_events.IsInPhase(PHASE_COMBAT))
                    return;

                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_MOVE_NEXT:
                            if (_currentWaypoint >= 10 || _currentWaypoint == 0)
                                _currentWaypoint = 1;
                            me->GetMotionMaster()->MovePoint(0, rimefangPos[_currentWaypoint]);
                            ++_currentWaypoint;
                            _events.ScheduleEvent(EVENT_MOVE_NEXT, 2*IN_MILLISECONDS, 0, PHASE_COMBAT);
                            break;
                        case EVENT_ICY_BLAST:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                DoCast(target, SPELL_ICY_BLAST);
                            _events.ScheduleEvent(EVENT_ICY_BLAST, 15*IN_MILLISECONDS, 0, PHASE_COMBAT);
                            break;
                        case EVENT_HOARFROST:
                            if (Unit* target = ObjectAccessor::GetUnit(*me, _hoarfrostTargetGUID))
                            {
                                DoCast(target, SPELL_HOARFROST);
                                _hoarfrostTargetGUID.Clear();
                            }
                            break;
                        default:
                            break;
                    }
                }
            }

        private:
            Vehicle* _vehicle;
            ObjectGuid _hoarfrostTargetGUID;
            EventMap _events;
            uint8 _currentWaypoint;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_rimefangAI(creature);
        }
};

class npc_jaina_or_sylvanas_pos_outro : public CreatureScript
{
public:
    npc_jaina_or_sylvanas_pos_outro() : CreatureScript("npc_jaina_or_sylvanas_pos_outro") { }

    struct npc_jaina_or_sylvanas_pos_outroAI : public ScriptedAI
    {
        npc_jaina_or_sylvanas_pos_outroAI(Creature* creature) : ScriptedAI(creature) 
        {
            instance = creature->GetInstanceScript();
        }

        void InitializeAI()
        {
            events.Reset();
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            events.ScheduleEvent(EVENT_GENERAL_TALK_1, 11 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_SPAWN_GUARDS, 1 * IN_MILLISECONDS);

            // Visual Effect for Bunnies
            std::list<Creature*> BunnyList;
            me->GetCreatureListWithEntryInGrid(BunnyList, NPC_LARGE_BUNNY, 22.0f);
            if (!BunnyList.empty())
                for (std::list<Creature*>::iterator itr = BunnyList.begin(); itr != BunnyList.end(); itr++)
                    (*itr)->CastSpell((*itr), SPELL_ARCANE_FORM, true);
        }       

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            int32 entryIndex;
            if (instance->GetData(DATA_TEAM_IN_INSTANCE) == ALLIANCE)
                entryIndex = 0;
            else
                entryIndex = 1;

            switch (events.ExecuteEvent())
            {
                case EVENT_SPAWN_GUARDS:
                    for (int8 i = 0; EndingPositions[i].entry[entryIndex] != 0; ++i)
                    {
                        if (Creature* summon = me->SummonCreature(EndingPositions[i].entry[entryIndex], spawnPoint, TEMPSUMMON_DEAD_DESPAWN))
                        {
                            summon->GetMotionMaster()->MovePoint(0, EndingPositions[i].movePosition);
                        }
                    }
                    break;
                case EVENT_GENERAL_TALK_1:
                {
                    std::list<Creature*> LeadList;
                    me->GetCreatureListWithEntryInGrid(LeadList, NPC_MARTIN_VICTUS_2, 500.0f);
                    me->GetCreatureListWithEntryInGrid(LeadList, NPC_GORKUN_IRONSKULL_1, 500.0f);
                    if (!LeadList.empty())
                        for (std::list<Creature*>::iterator itr = LeadList.begin(); itr != LeadList.end(); itr++)
                            (*itr)->AI()->Talk(GUARD_TYRANNUS_WIN_1);
                    events.ScheduleEvent(EVENT_GUARDS_EMOTE, 2 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_GENERAL_TALK_2, 17 * IN_MILLISECONDS);
                    break;
                }
                case EVENT_GUARDS_EMOTE:
                {
                    std::list<Creature*> AddList;
                    me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_2_ALLIANCE, 300.0f);
                    me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_3_ALLIANCE, 300.0f);
                    me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_1_ALLIANCE, 300.0f);
                    me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_3_HORDE, 300.0f);
                    me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_1_HORDE, 300.0f);
                    me->GetCreatureListWithEntryInGrid(AddList, NPC_FREED_SLAVE_2_HORDE, 300.0f);
                    if (!AddList.empty())
                        for (std::list<Creature*>::iterator itr = AddList.begin(); itr != AddList.end(); itr++)
                            (*itr)->HandleEmoteCommand(EMOTE_ONESHOT_CHEER);                       
                    events.ScheduleEvent(EVENT_GUARDS_EMOTE, 2 * IN_MILLISECONDS);
                    break;
                }
                case EVENT_GENERAL_TALK_2:
                {
                    std::list<Creature*> LeadList;
                    me->GetCreatureListWithEntryInGrid(LeadList, NPC_MARTIN_VICTUS_2,    500.0f);
                    me->GetCreatureListWithEntryInGrid(LeadList, NPC_GORKUN_IRONSKULL_1, 500.0f);
                    if (!LeadList.empty())
                        for (std::list<Creature*>::iterator itr = LeadList.begin(); itr != LeadList.end(); itr++)
                            (*itr)->AI()->Talk(GUARD_TYRANNUS_WIN_2);
                    events.ScheduleEvent(EVENT_LADY_TALK_1, 14 * IN_MILLISECONDS);
                    break;
                }
                case EVENT_LADY_TALK_1:
                    Talk(SAY_SYLVANAS_OUTRO_3);
                    events.ScheduleEvent(EVENT_LADY_PORT, 1 * IN_MILLISECONDS);
                    break;
                case EVENT_LADY_PORT:
                    if (Map* map = me->GetMap())
                    {
                        for (Player& player : map->GetPlayers())
                            me->CastSpell(&player, SPELL_CALL_OF_SYLVANAS);
                    }
                    events.ScheduleEvent(EVENT_SINDRAGOSA_MOVE, 0.1 * IN_MILLISECONDS);
                    break;
                case EVENT_SINDRAGOSA_MOVE:
                    if (Creature* sindragosa = me->FindNearestCreature(NPC_SINDRAGOSA, 1000.0f))
                    {
                        sindragosa->setActive(true);
                        sindragosa->SetCanFly(true);
                        sindragosa->SetSpeedRate(MOVE_WALK, 5.0f);
                        sindragosa->SetSpeedRate(MOVE_RUN, 5.0f);
                        sindragosa->GetMotionMaster()->MovePoint(0, SindragosaWatchPoint, true);
                    }
                    events.ScheduleEvent(EVENT_SINDRAGOSA_KILL, 4 * IN_MILLISECONDS);
                    break;
                case EVENT_SINDRAGOSA_KILL:
                {
                    if (Creature* sindragosa = me->FindNearestCreature(NPC_SINDRAGOSA, 1000.0f))
                    {
                        sindragosa->CastSpell(sindragosa, SPELL_FROST_BOMB, true);
                        sindragosa->DespawnOrUnsummon(3 * IN_MILLISECONDS);
                    }
                    events.ScheduleEvent(EVENT_LADY_WP, 7 * IN_MILLISECONDS);

                    //Mage Block attack of Sindragosa
                    std::list<Creature*> GuardList;
                    me->GetCreatureListWithEntryInGrid(GuardList, NPC_FREED_SLAVE_3_ALLIANCE, 200.0f);
                    me->GetCreatureListWithEntryInGrid(GuardList, NPC_FREED_SLAVE_1_HORDE, 200.0f);
                    if (!GuardList.empty())
                        for (std::list<Creature*>::iterator itr = GuardList.begin(); itr != GuardList.end(); itr++)
                            (*itr)->CastSpell((*itr), SPELL_ICEBLOCK, true);

                    GuardList.clear();
                    me->GetCreatureListWithEntryInGrid(GuardList, NPC_FREED_SLAVE_2_ALLIANCE, 200.0f);
                    me->GetCreatureListWithEntryInGrid(GuardList, NPC_FREED_SLAVE_1_ALLIANCE, 200.0f);
                    me->GetCreatureListWithEntryInGrid(GuardList, NPC_FREED_SLAVE_3_HORDE, 200.0f);
                    me->GetCreatureListWithEntryInGrid(GuardList, NPC_FREED_SLAVE_2_HORDE, 200.0f);
                    if (!GuardList.empty())
                        for (std::list<Creature*>::iterator itr = GuardList.begin(); itr != GuardList.end(); itr++)
                            (*itr)->Kill((*itr));
                    break;
                }
                case EVENT_LADY_WP: 
                {
                    me->GetMotionMaster()->MovePath(NPC_SYLVANAS_PART2 * 10, false);
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    // Remove Visual Effect for Bunnies
                    std::list<Creature*> BunnyList;
                    me->GetCreatureListWithEntryInGrid(BunnyList, NPC_LARGE_BUNNY, 22.0f);
                    if (!BunnyList.empty())
                        for (std::list<Creature*>::iterator itr = BunnyList.begin(); itr != BunnyList.end(); itr++)
                            (*itr)->RemoveAura(SPELL_ARCANE_FORM);

                    std::list<Creature*> GuardList;
                    me->GetCreatureListWithEntryInGrid(GuardList, NPC_FREED_SLAVE_3_ALLIANCE, 200.0f);
                    me->GetCreatureListWithEntryInGrid(GuardList, NPC_FREED_SLAVE_1_HORDE, 200.0f);
                    if (!GuardList.empty())
                        for (std::list<Creature*>::iterator itr = GuardList.begin(); itr != GuardList.end(); itr++)
                            (*itr)->HandleEmoteCommand(EMOTE_ONESHOT_CRY);

                    events.ScheduleEvent(EVENT_LADY_TALK_2, 5 * IN_MILLISECONDS);
                    break;
                }  
                case EVENT_LADY_TALK_2:
                    Talk(SAY_SYLVANAS_OUTRO_4);
                    break;
                default:
                    break;
            }
        }
    private:
        EventMap events;
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_jaina_or_sylvanas_pos_outroAI(creature);
    }
};

class player_overlord_brandAI : public PlayerAI
{
    public:
        player_overlord_brandAI(Player& player, ObjectGuid casterGUID) : PlayerAI(player), _tyrannusGUID(casterGUID) { }

        void DamageDealt(Unit* /*victim*/, uint32& damage, DamageEffectType /*damageType*/)
        {
            if (Creature* tyrannus = ObjectAccessor::GetCreature(me, _tyrannusGUID))
                if (Unit* victim = tyrannus->GetVictim())
                    me.CastCustomSpell(SPELL_OVERLORD_BRAND_DAMAGE, SPELLVALUE_BASE_POINT0, damage, victim, true, NULL, NULL, tyrannus->GetGUID());
        }

        void HealDone(Unit* /*target*/, uint32& addHealth)
        {
            if (Creature* tyrannus = ObjectAccessor::GetCreature(me, _tyrannusGUID))
                me.CastCustomSpell(SPELL_OVERLORD_BRAND_HEAL, SPELLVALUE_BASE_POINT0, int32(addHealth * 5.5f), tyrannus, true, NULL, NULL, tyrannus->GetGUID());
        }

        void UpdateAI(uint32 /*diff*/) override { }

    private:
        ObjectGuid _tyrannusGUID;
};

class spell_tyrannus_overlord_brand : public SpellScriptLoader
{
    public:
        spell_tyrannus_overlord_brand() : SpellScriptLoader("spell_tyrannus_overlord_brand") { }

        class spell_tyrannus_overlord_brand_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_tyrannus_overlord_brand_AuraScript);

            bool Load()
            {
                return GetCaster() && GetCaster()->GetEntry() == NPC_TYRANNUS;
            }

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
                    return;

                oldAI = GetTarget()->GetAI();
                oldAIState = GetTarget()->IsAIEnabled;
                GetTarget()->SetAI(new player_overlord_brandAI(*GetTarget()->ToPlayer(), GetCasterGUID()));
                GetTarget()->IsAIEnabled = true;
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
                    return;

                GetTarget()->IsAIEnabled = oldAIState;
                UnitAI* thisAI = GetTarget()->GetAI();
                GetTarget()->SetAI(oldAI);
                delete thisAI;
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_tyrannus_overlord_brand_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_tyrannus_overlord_brand_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }

            UnitAI* oldAI;
            bool oldAIState;
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_tyrannus_overlord_brand_AuraScript();
        }
};

class spell_tyrannus_mark_of_rimefang : public SpellScriptLoader
{
    public:
        spell_tyrannus_mark_of_rimefang() : SpellScriptLoader("spell_tyrannus_mark_of_rimefang") { }

        class spell_tyrannus_mark_of_rimefang_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_tyrannus_mark_of_rimefang_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* caster = GetCaster();
                if (!caster || caster->GetTypeId() != TYPEID_UNIT)
                    return;

                if (InstanceScript* instance = caster->GetInstanceScript())
                    if (Creature* rimefang = ObjectAccessor::GetCreature(*caster, ObjectGuid(instance->GetGuidData(DATA_RIMEFANG))))
                        rimefang->AI()->SetGuidData(GetTarget()->GetGUID(), GUID_HOARFROST);
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_tyrannus_mark_of_rimefang_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_tyrannus_mark_of_rimefang_AuraScript();
        }
};

// 69232 - Icy Blast
class spell_tyrannus_rimefang_icy_blast : public SpellScriptLoader
{
    public:
        spell_tyrannus_rimefang_icy_blast() : SpellScriptLoader("spell_tyrannus_rimefang_icy_blast") { }

        class spell_tyrannus_rimefang_icy_blast_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_tyrannus_rimefang_icy_blast_SpellScript);

            bool Validate(SpellInfo const* /*spellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_ICY_BLAST_AURA))
                    return false;
                return true;
            }

            void HandleTriggerMissile(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                if (Position const* pos = GetHitDest())
                    if (TempSummon* summon = GetCaster()->SummonCreature(NPC_ICY_BLAST, *pos, TEMPSUMMON_TIMED_DESPAWN, 60000))
                        summon->CastSpell(summon, SPELL_ICY_BLAST_AURA, true);
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_tyrannus_rimefang_icy_blast_SpellScript::HandleTriggerMissile, EFFECT_1, SPELL_EFFECT_TRIGGER_MISSILE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_tyrannus_rimefang_icy_blast_SpellScript();
        }
};

// 69425 | 70827 Icicle Damage
class spell_icycle_damage : public SpellScriptLoader
{
public:
    spell_icycle_damage() : SpellScriptLoader("spell_icycle_damage") { }

    class spell_icycle_damage_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_icycle_damage_SpellScript);

        void HandleScript(SpellEffIndex /*eff*/)
        {
            Unit* caster = GetCaster();
            if (!caster || !GetHitPlayer())
                return;

            if (GetHitDamage() >= MAX_HIT_DAMAGE)
                if (InstanceScript* instance = caster->GetInstanceScript())
                    instance->SetData(DATA_DO_NOT_LOOK_UP, 1);
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_icycle_damage_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_icycle_damage_SpellScript();
    }
};

class at_tyrannus_event_starter : public AreaTriggerScript
{
    public:
        at_tyrannus_event_starter() : AreaTriggerScript("at_tyrannus_event_starter") { }

        bool OnTrigger(Player* player, const AreaTriggerEntry* /*at*/)
        {
            InstanceScript* instance = player->GetInstanceScript();
            if (player->IsGameMaster() || !instance)
                return false;

            if (instance->GetBossState(DATA_TYRANNUS) != IN_PROGRESS && instance->GetBossState(DATA_TYRANNUS) != DONE)
                if (Creature* tyrannus = ObjectAccessor::GetCreature(*player, ObjectGuid(instance->GetGuidData(DATA_TYRANNUS))))
                {
                    tyrannus->AI()->DoAction(ACTION_START_INTRO);
                    return true;
                }

            return false;
        }
};

class at_tyrannus_cave_starter : public AreaTriggerScript
{
public:
    at_tyrannus_cave_starter() : AreaTriggerScript("at_tyrannus_cave_starter") { }

    bool OnTrigger(Player* player, const AreaTriggerEntry* /*at*/)
    {
        InstanceScript* instance = player->GetInstanceScript();
        if (player->IsGameMaster() || !instance)
            return false;

        if (instance->GetBossState(DATA_TYRANNUS) == NOT_STARTED)
            if (Creature* tyrannus = ObjectAccessor::GetCreature(*player, ObjectGuid(instance->GetGuidData(DATA_TYRANNUS))))
            {
                tyrannus->AI()->DoAction(ACTION_CAVE_AMBUSH);
            return true;
            }
        return false;
    }
};

class npc_tyrannus_icicle : public CreatureScript
{
   public:
       npc_tyrannus_icicle() : CreatureScript("npc_tyrannus_icicle") { }

       CreatureAI* GetAI(Creature* creature) const
       {
           return new npc_tyrannus_icicleAI(creature);
       }

       struct npc_tyrannus_icicleAI : public ScriptedAI
       {
           npc_tyrannus_icicleAI(Creature *creature) : ScriptedAI(creature)
           {
               Instance = creature->GetInstanceScript();
               me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
               me->SetReactState(REACT_PASSIVE);
               me->SetVisible(false);
               me->setActive(true);
           }

           void InitializeAI() override
           {
               if (!Instance || static_cast<InstanceMap*>(me->GetMap())->GetScriptId() != sObjectMgr->GetScriptId(PoSScriptName))
                   me->IsAIEnabled = false;
               Reset();
           }

           void Reset() override
           {
               IcicleTimer = urand(5, 100)*IN_MILLISECONDS;
               me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_PACIFIED);
               me->SetReactState(REACT_PASSIVE);
           }

           void UpdateAI(uint32(diff)) override
           {
               if (Instance->GetBossState(DATA_TYRANNUS) != DONE)
               {
                   if (IcicleTimer <= diff)
                   {
                       DoCast(me, SPELL_ICICLE);
                       IcicleTimer = urand(30, 140)*IN_MILLISECONDS;
                   }
                   else 
                       IcicleTimer -= diff;
               }
           }
       private:
           InstanceScript* Instance;
           uint32 IcicleTimer;
    };

};

void AddSC_boss_tyrannus()
{
    new boss_tyrannus();
    new boss_rimefang();
    new npc_jaina_or_sylvanas_pos_outro();
    new spell_tyrannus_overlord_brand();
    new spell_tyrannus_mark_of_rimefang();
    new spell_tyrannus_rimefang_icy_blast();
    new at_tyrannus_event_starter();
    new npc_tyrannus_icicle();
    new spell_icycle_damage();
    new at_tyrannus_cave_starter();
}
