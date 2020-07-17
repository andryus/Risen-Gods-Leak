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
#include "SpellScript.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "ulduar.h"
#include "Player.h"

// ###### Related Creatures/Object ######
enum FreyaNpcs
{
    NPC_SUN_BEAM = 33170,
    NPC_DETONATING_LASHER = 32918,
    NPC_ANCIENT_CONSERVATOR = 33203,
    NPC_ANCIENT_WATER_SPIRIT = 33202,
    NPC_STORM_LASHER = 32919,
    NPC_SNAPLASHER = 32916,
    NPC_NATURE_BOMB = 34129,
    NPC_EONARS_GIFT = 33228,
    NPC_HEALTHY_SPORE = 33215,
    NPC_UNSTABLE_SUN_BEAM = 33050,
    NPC_IRON_ROOTS = 33088,
    NPC_STRENGTHENED_IRON_ROOTS = 33168,
};

enum GameObjects
{
    GO_NATURE_BOMB = 194902,

    GO_CHEST_10_0_ELDER = 194324,
    GO_CHEST_10_1_ELDER = 194325,
    GO_CHEST_10_2_ELDER = 194326,
    GO_CHEST_10_3_ELDER = 194327,
    GO_CHEST_25_0_ELDER = 194328,
    GO_CHEST_25_1_ELDER = 194329,
    GO_CHEST_25_2_ELDER = 194330,
    GO_CHEST_25_3_ELDER = 194331,
};

// ###### Texts ######
enum FreyaYells
{
    // Freya
    SAY_AGGRO = 0,
    SAY_AGGRO_WITH_ELDER = 1,
    SAY_SLAY_1 = 2,
    SAY_SLAY_2 = 3,
    SAY_DEATH = 4,
    SAY_BERSERK = 5,
    SAY_SUMMON_CONSERVATOR = 6,
    SAY_SUMMON_TRIO = 7,
    SAY_SUMMON_LASHERS = 8,
    SAY_YS_HELP = 9,
    EMOTE_SPAWN_LIFEBINDER_GIFT = 10,

    // Elder Brightleaf
    SAY_BRIGHTLEAF_AGGRO = 0,
    SAY_BRIGHTLEAF_SLAY_1 = 1,
    SAY_BRIGHTLEAF_SLAY_2 = 2,
    SAY_BRIGHTLEAF_DEATH = 3,

    // Elder Ironbranch
    SAY_IRONBRANCH_AGGRO = 3,
    SAY_IRONBRANCH_SLAY_1 = 2,
    SAY_IRONBRANCH_SLAY_2 = 0,
    SAY_IRONBRANCH_DEATH = 1,

    // Elder Stonebark
    SAY_STONEBARK_AGGRO = 2,
    SAY_STONEBARK_SLAY_1 = 3,
    SAY_STONEBARK_SLAY_2 = 0,
    SAY_STONEBARK_DEATH = 1,
};

// ###### Datas ######
enum FreyaActions
{
    ACTION_FREYA_START_FIGHT = 1,
};

enum Datas
{
    DATA_GETTING_BACK_TO_NATURE = 1,
    DATA_KNOCK_ON_WOOD = 2,
    GUID_TRIO_MEMBER_DIED = 3,
    DATA_LUMBER_JACK = 1,
    DATA_LUMBER_JACK_COUNT = 1
};

enum Timers
{
    WAVE_DURATION_MAX = 60000,
    WAVE_TIMEDIFF_MAX = 10000,
};

enum Misc
{
    MAX_WAVE_COUNT = 6,
    MAX_NATURE_BOMBS_10 = 5,
    MAX_NATURE_BOMBS_25 = 12,
};

enum Phases
{
    PHASE_WAVE = 1,
    PHASE_FREYA = 2,
};

// ###### Event Controlling ######
enum FreyaEvents
{
    // Freya
    EVENT_WAVE = 1,
    EVENT_EONAR_GIFT,
    EVENT_NATURE_BOMB,
    EVENT_UNSTABLE_ENERGY,
    EVENT_STRENGTHENED_IRON_ROOTS,
    EVENT_GROUND_TREMOR,
    EVENT_SUNBEAM,
    EVENT_ENRAGE,
    EVENT_CHECK_ATTUNED_TO_NATURE,
    EVENT_CHECK_HOME_DIST,

    // Elder Stonebark
    EVENT_TREMOR,
    EVENT_BARK,
    EVENT_FISTS,

    // Elder Ironbranch
    EVENT_IMPALE,
    EVENT_IRON_ROOTS,
    EVENT_THORN_SWARM,

    // Elder Brightleaf
    EVENT_SOLAR_FLARE,
    EVENT_UNSTABLE_SUN_BEAM,
    EVENT_FLUX,

    // Freya Achiev
    EVENT_ACHIEV_CANADIAN_DEFOREST,
    EVENT_RESET_LUMBER_JACK_COUNTER,

    EVENT_TRIO_RESPAWN = 100,
};

// ###### Spells ######
enum FreyaSpells
{
    // Freya
    SPELL_ATTUNED_TO_NATURE = 62519,
    SPELL_TOUCH_OF_EONAR = 62528,
    SPELL_SUNBEAM = 62623,
    SPELL_ENRAGE = 47008,
    SPELL_FREYA_GROUND_TREMOR = 62437,
    SPELL_ROOTS_FREYA = 62862,
    SPELL_IRON_ROOTS_DOT = 62861,
    SPELL_IRON_ROOTS_DOT_25 = 62438,
    SPELL_STONEBARK_ESSENCE = 62483,
    SPELL_IRONBRANCH_ESSENCE = 62484,
    SPELL_BRIGHTLEAF_ESSENCE = 62485,
    SPELL_DRAINED_OF_POWER = 62467,
    SPELL_SUMMON_EONAR_GIFT = 62572,

    // Stonebark
    SPELL_FISTS_OF_STONE = 62344,
    SPELL_GROUND_TREMOR = 62325,
    SPELL_PETRIFIED_BARK = 62337,
    SPELL_PETRIFIED_BARK_25 = 62933,
    SPELL_PETRIFIED_BARK_DMG = 62379,

    // Ironbranch
    SPELL_IMPALE = 62310,
    SPELL_ROOTS_IRONBRANCH_DOT = 62283,
    SPELL_ROOTS_IRONBRANCH_DOT_25 = 62930,
    SPELL_ROOTS_IRONBRANCH_SUMMON = 62275,
    SPELL_THORN_SWARM = 62285,

    // Brightleaf
    SPELL_FLUX_AURA = 62239,
    SPELL_FLUX = 62262,
    SPELL_FLUX_PLUS = 62251,
    SPELL_FLUX_MINUS = 62252,
    SPELL_SOLAR_FLARE = 62240,
    SPELL_UNSTABLE_SUN_BEAM_SUMMON = 62207, // Trigger 62221

    // Stack Removing of Attuned to Nature
    SPELL_REMOVE_25STACK = 62521,
    SPELL_REMOVE_10STACK = 62525,
    SPELL_REMOVE_2STACK = 62524,

    // Achievement spells
    SPELL_DEFORESTATION_CREDIT = 65015,
    SPELL_KNOCK_ON_WOOD_CREDIT = 65074,

    // Wave summoning spells
    SPELL_SUMMON_LASHERS = 62688,
    SPELL_SUMMON_LASHER_SINGLE = 62687,
    SPELL_SUMMON_TRIO = 62686,
    SPELL_SUMMON_ANCIENT_CONSERVATOR = 62685,

    // Detonating Lasher
    SPELL_DETONATE = 62598,
    SPELL_FLAME_LASH = 62608,

    // Ancient Water Spirit
    SPELL_TIDAL_WAVE = 62653,
    SPELL_TIDAL_WAVE_EFFECT = 62654,
    SPELL_TIDAL_WAVE_EFFECT_25 = 62936,

    // Storm Lasher
    SPELL_LIGHTNING_LASH = 62648,
    SPELL_STORMBOLT = 62649,
    SPELL_STORM_LASHER_VISUAL = 62639,

    // Snaplasher
    SPELL_HARDENED_BARK = 62664,
    SPELL_HARDENED_BARK_25 = 64191,
    SPELL_BARK_AURA = 62663,

    // Ancient Conservator
    SPELL_CONSERVATOR_GRIP = 62532,
    SPELL_NATURE_FURY = 62589,
    SPELL_SUMMON_SPORE_PERIODIC = 62566,
    SPELL_SPORE_SUMMON_NW = 62582,
    SPELL_SPORE_SUMMON_NE = 62591,
    SPELL_SPORE_SUMMON_SE = 62592,
    SPELL_SPORE_SUMMON_SW = 62593,

    // Healthly Spore
    SPELL_HEALTHY_SPORE_VISUAL = 62538,
    SPELL_GROW = 62559,
    SPELL_POTENT_PHEROMONES = 62541,

    // Eonar's Gift
    SPELL_LIFEBINDERS_GIFT = 62584,
    SPELL_PHEROMONES = 62619,
    SPELL_EONAR_VISUAL = 62579,

    // Nature Bomb
    SPELL_NATURE_BOMB = 64587,
    SPELL_OBJECT_BOMB = 64600,
    SPELL_SUMMON_NATURE_BOMB = 64606,
    SPELL_SUMMON_NATURE_BOMBS = 64604,

    // Unstable Sun Beam (Brightleaf)
    SPELL_UNSTABLE_SUN_BEAM = 62211,
    SPELL_UNSTABLE_ENERGY = 62217,
    SPELL_PHOTOSYNTHESIS = 62209,
    SPELL_UNSTABLE_SUN_BEAM_TRIGGERED = 62243,

    // Sun Beam (Freya)
    SPELL_FREYA_UNSTABLE_ENERGY = 62451,
    SPELL_FREYA_UNSTABLE_ENERGY_VISUAL = 62216,
    SPELL_FREYA_UNSTABLE_SUNBEAM = 62450,
};

enum FreyaAchievements
{
    ACHIEVEMENT_GET_BACK_TO_NATURE_10 = 2982,
    ACHIEVEMENT_GET_BACK_TO_NATURE_25 = 2983
};

// ##### Additional Data #####
const Position ChestSpawnPosition = {2368.5f, -37.254501f, 424.106995f, 1.55988f};

uint32 GetChestForDifficulty(Difficulty difficulty, uint8 elders)
{
    switch (elders)
    {
    case 3:
        return difficulty == RAID_DIFFICULTY_10MAN_NORMAL ? GO_CHEST_10_3_ELDER : GO_CHEST_25_3_ELDER;
    case 2:
        return difficulty == RAID_DIFFICULTY_10MAN_NORMAL ? GO_CHEST_10_2_ELDER : GO_CHEST_25_2_ELDER;
    case 1:
        return difficulty == RAID_DIFFICULTY_10MAN_NORMAL ? GO_CHEST_10_1_ELDER : GO_CHEST_25_1_ELDER;
    case 0:
    default:
        return difficulty == RAID_DIFFICULTY_10MAN_NORMAL ? GO_CHEST_10_0_ELDER : GO_CHEST_25_0_ELDER;
    }
}

enum WaveTypes
{
    WAVE_TRIO = 1,
    WAVE_LASHERS = 2,
    WAVE_CONSERVATOR = 3,
};

struct DeadTrioMember
{
    uint64 type;
    int64 time;
};

struct TrioWave
{
    ObjectGuid one;
    ObjectGuid two;
    ObjectGuid three;
    uint8  alive;

    TrioWave() : one(), two(), three(), alive(3) {}
};

class boss_freya : public CreatureScript
{
public:
    boss_freya() : CreatureScript("boss_freya") { }

    struct boss_freyaAI : public BossAI
    {
        boss_freyaAI(Creature* creature) : BossAI(creature, BOSS_FREYA)
        {
            ASSERT(instance);
        }

        ~boss_freyaAI()
        {
            for (std::list<TrioWave*>::iterator itr = TrioWaveData.begin(); itr != TrioWaveData.end(); ++itr)
                if (*itr)
                    delete (*itr);
        }

        void Reset()
        {
            BossAI::Reset();

            DespawnRoots();

            for (uint8 i = 0; i < 3; ++i)
                if (Creature* elder = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_ELDER_BRIGHTLEAF + i))))
                    if (elder->IsAlive())
                    {
                        elder->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        elder->SetReactState(REACT_AGGRESSIVE);
                    }
        }

        void EnterCombat(Unit* who)
        {
            BossAI::EnterCombat(who);

            Phase = PHASE_WAVE;

            encounterTime = 0;
            waveTime = 0;
            ElderCount = 0;
            CurrentWave = 0;
            WaveCount = 0;
            activeWaves = 0;
            AttunedToNatureStack = 150;

            LasherDeadCount = 0;

            deforestationCompleted = false;

            Waves.clear();

            for (std::list<TrioWave*>::iterator itr = TrioWaveData.begin(); itr != TrioWaveData.end(); ++itr)
                if (*itr)
                    delete (*itr);
            TrioWaveData.clear();
            TrioRespawnData.clear();
            DeadTrioMemberData.clear();


            for (uint8 i = 0; i < 3; ++i)
            {
                Creature* elder = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_ELDER_BRIGHTLEAF + i)));
                if (elder && elder->IsAlive())
                {
                    elder->AI()->DoAction(ACTION_FREYA_START_FIGHT);
                    ++ElderCount;

                    switch (elder->GetEntry())
                    {
                    case NPC_ELDER_BRIGHTLEAF:
                        elder->CastSpell(me, SPELL_BRIGHTLEAF_ESSENCE, true);
                        events.ScheduleEvent(EVENT_UNSTABLE_ENERGY, urand(10, 20)*IN_MILLISECONDS);
                        break;
                    case NPC_ELDER_STONEBARK:
                        elder->CastSpell(me, SPELL_STONEBARK_ESSENCE, true);
                        events.ScheduleEvent(EVENT_GROUND_TREMOR, urand(10, 20)*IN_MILLISECONDS);
                        break;
                    case NPC_ELDER_IRONBRANCH:
                        elder->CastSpell(me, SPELL_IRONBRANCH_ESSENCE, true);
                        events.ScheduleEvent(EVENT_STRENGTHENED_IRON_ROOTS, urand(10, 20)*IN_MILLISECONDS);
                        break;
                    }
                    elder->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    elder->SetReactState(REACT_PASSIVE);
                }
            }

            if (ElderCount == 0)
                Talk(SAY_AGGRO);
            else
                Talk(SAY_AGGRO_WITH_ELDER);

            me->CastCustomSpell(SPELL_ATTUNED_TO_NATURE, SPELLVALUE_AURA_STACK, 150, me, true);
            DoCastSelf(SPELL_TOUCH_OF_EONAR, true);

            events.ScheduleEvent(EVENT_WAVE, 10 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_EONAR_GIFT, 25 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_ENRAGE, 600 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_SUNBEAM, urand(5, 15)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_CHECK_ATTUNED_TO_NATURE, 15 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_CHECK_HOME_DIST, IN_MILLISECONDS);
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Talk(RAND(SAY_SLAY_1, SAY_SLAY_2));
        }

        void JustDied(Unit* killer)
        {
            BossAI::JustDied(killer);

            Talk(SAY_DEATH);

            //Achievement Cast
            DoCastSelf(SPELL_KNOCK_ON_WOOD_CREDIT, true);

            me->setFaction(35);
            me->DespawnOrUnsummon(7500);

            for (uint8 i = 0; i < 3; ++i)
            {
                Creature* elder = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_ELDER_BRIGHTLEAF + i)));
                if (elder && elder->IsAlive())
                {
                    elder->setFaction(35);
                    elder->AI()->EnterEvadeMode();
                }
            }

            me->SummonGameObject(GetChestForDifficulty(me->GetMap()->GetDifficulty(), ElderCount),
                ChestSpawnPosition.GetPositionX(), ChestSpawnPosition.GetPositionY(), ChestSpawnPosition.GetPositionZ(), ChestSpawnPosition.GetOrientation(),
                0, 0, 1, 1, 7 * DAY);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            encounterTime += diff;

            events.Update(diff);

            // For achievement check
            AttunedToNatureStack = me->GetAuraCount(SPELL_ATTUNED_TO_NATURE);
            if (!AttunedToNatureStack && Phase != PHASE_FREYA)
            {
                Phase = PHASE_FREYA;
                events.CancelEvent(EVENT_WAVE);
                events.ScheduleEvent(EVENT_NATURE_BOMB, urand(10, 20)*IN_MILLISECONDS);
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    // ARBITRARY PHASE - NORMAL MODE
                case EVENT_ENRAGE:
                    Talk(SAY_BERSERK);
                    DoCastSelf(SPELL_ENRAGE, true);
                    break;
                case EVENT_SUNBEAM:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        DoCast(target, SPELL_SUNBEAM);
                    events.ScheduleEvent(EVENT_SUNBEAM, urand(10, 15)*IN_MILLISECONDS);
                    return;
                case EVENT_EONAR_GIFT:
                    DoCast(SPELL_SUMMON_EONAR_GIFT);
                    Talk(EMOTE_SPAWN_LIFEBINDER_GIFT);
                    events.RescheduleEvent(EVENT_EONAR_GIFT, urand(40, 50)*IN_MILLISECONDS);
                    break;
                case EVENT_CHECK_HOME_DIST:
                    if (me->GetDistance(me->GetHomePosition()) > 200.0f)
                        EnterEvadeMode(EVADE_REASON_BOUNDARY);
                    events.ScheduleEvent(EVENT_CHECK_HOME_DIST, IN_MILLISECONDS);
                    break;
                    // ARBITRARY PHASE - HARD MODE
                case EVENT_UNSTABLE_ENERGY:
                    DoCast(SPELL_FREYA_UNSTABLE_SUNBEAM);
                    events.ScheduleEvent(EVENT_UNSTABLE_ENERGY, urand(15, 20)*IN_MILLISECONDS);
                    break;
                case EVENT_STRENGTHENED_IRON_ROOTS:
                    DoCast(SPELL_ROOTS_FREYA);
                    events.RescheduleEvent(EVENT_STRENGTHENED_IRON_ROOTS, urand(45000, 60000));
                    break;
                case EVENT_GROUND_TREMOR:
                    DoCastAOE(SPELL_FREYA_GROUND_TREMOR);
                    events.ScheduleEvent(EVENT_GROUND_TREMOR, urand(25, 28)*IN_MILLISECONDS);
                    return;
                    // FIRST PHASE
                case EVENT_WAVE:
                    waveTime = 0;
                    if (WaveCount < MAX_WAVE_COUNT && Phase == PHASE_WAVE)
                    {
                        GenerateNewWave();
                        WaveCount++;
                        activeWaves++;
                        switch (CurrentWave)
                        {
                        case WAVE_LASHERS:
                            Talk(SAY_SUMMON_LASHERS);
                            DoCast(SPELL_SUMMON_LASHERS);
                            break;
                        case WAVE_TRIO:
                            Talk(SAY_SUMMON_TRIO);
                            TrioWaveData.push_back(new TrioWave());
                            DoCast(SPELL_SUMMON_TRIO);
                            break;
                        case WAVE_CONSERVATOR:
                            Talk(SAY_SUMMON_CONSERVATOR);
                            DoCast(SPELL_SUMMON_ANCIENT_CONSERVATOR);
                            break;
                        }
                        events.ScheduleEvent(EVENT_WAVE, WAVE_DURATION_MAX);
                    }
                    break;
                case EVENT_ACHIEV_CANADIAN_DEFOREST:
                    DoCastSelf(SPELL_DEFORESTATION_CREDIT, true);
                    break;
                    // SECOND PHASE
                case EVENT_NATURE_BOMB:
                    me->CastCustomSpell(SPELL_SUMMON_NATURE_BOMBS, SPELLVALUE_MAX_TARGETS, RAID_MODE(MAX_NATURE_BOMBS_10, MAX_NATURE_BOMBS_25));
                    events.ScheduleEvent(EVENT_NATURE_BOMB, urand(10000, 12000));
                    break;
                }

                if (eventId >= EVENT_TRIO_RESPAWN)
                {
                    uint8 index = eventId - EVENT_TRIO_RESPAWN;
                    TrioWave* wave = TrioRespawnData[index];

                    if (wave->alive == 0)
                        continue;

                    wave->alive = 3;

                    if (Creature* creature = ObjectAccessor::GetCreature(*me, wave->one))
                    {
                        if (creature->IsAlive())
                            creature->SetFullHealth();
                        else
                        {
                            creature->Respawn();
                            DoZoneInCombat(creature);
                        }
                    }

                    if (Creature* creature = ObjectAccessor::GetCreature(*me, wave->two))
                    {
                        if (creature->IsAlive())
                            creature->SetFullHealth();
                        else
                        {
                            creature->Respawn();
                            DoZoneInCombat(creature);
                        }
                    }

                    if (Creature* creature = ObjectAccessor::GetCreature(*me, wave->three))
                    {
                        if (creature->IsAlive())
                            creature->SetFullHealth();
                        else
                        {
                            creature->Respawn();
                            DoZoneInCombat(creature);
                        }
                    }
                }
            }

            if (Phase == PHASE_WAVE)
                waveTime += diff;

            DoMeleeAttackIfReady();
        }

        void GenerateNewWave()
        {
            if (!CurrentWave)
            {
                bool spawned[3] = {false, false, false};
                for (uint8 i = 0; i < MAX_WAVE_COUNT; ++i)
                {
                    if (spawned[0] && spawned[1] && spawned[2])
                        for (uint8 j = 0; j < 3; ++j)
                            spawned[j] = false;

                    uint8 id = 0;
                    do
                    {
                        id = urand(0, 2);
                    } while (spawned[id]);
                    spawned[id] = true;

                    Waves.push_back(id + 1);
                }
            }

            CurrentWave = *Waves.begin();
            Waves.erase(Waves.begin());
        }

        void JustSummoned(Creature* summon)

        {
            switch (summon->GetEntry())
            {
            case NPC_SNAPLASHER:
            case NPC_STORM_LASHER:
            case NPC_ANCIENT_WATER_SPIRIT:
            {
                TrioWave* data = TrioWaveData.back();
                if (!data->one)
                    data->one = summon->GetGUID();
                else if (!data->two)
                    data->two = summon->GetGUID();
                else
                    data->three = summon->GetGUID();
                break;
            }
            case NPC_DETONATING_LASHER:
                summon->SetStandState(UNIT_STAND_STATE_SUBMERGED);
                summon->SetReactState(REACT_PASSIVE);
                break;
            }

            BossAI::JustSummoned(summon);
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/)
        {
            switch (summon->GetEntry())
            {
            case NPC_ANCIENT_CONSERVATOR:
                summon->CastSpell(me, SPELL_REMOVE_25STACK, true);
                summon->DespawnOrUnsummon(5 * IN_MILLISECONDS);

                activeWaves--;
                TimeCheck();
                break;
            case NPC_DETONATING_LASHER:
                summon->CastSpell(me, SPELL_REMOVE_2STACK, true);
                summon->DespawnOrUnsummon(5 * IN_MILLISECONDS);


                if (++LasherDeadCount >= 10)
                {
                    activeWaves--;
                    TimeCheck();
                    LasherDeadCount = 0;
                }
                break;
            case NPC_ANCIENT_WATER_SPIRIT:
                if (!deforestationCompleted)
                    DeforestationTrioMemberDied(NPC_ANCIENT_WATER_SPIRIT);
                break;
            case NPC_STORM_LASHER:
                if (!deforestationCompleted)
                    DeforestationTrioMemberDied(NPC_STORM_LASHER);
                break;
            case NPC_SNAPLASHER:
                if (!deforestationCompleted)
                    DeforestationTrioMemberDied(NPC_SNAPLASHER);
                break;
            }
        }

        void DeforestationTrioMemberDied(uint32 type)
        {
            DeadTrioMember* creatureDied = new DeadTrioMember;
            creatureDied->time = encounterTime;
            creatureDied->type = type;
            DeadTrioMemberData.push_front(creatureDied);
            CheckDeforestationSuccess();
        }

        void CheckDeforestationSuccess()
        {
            int64 before10Seconds = encounterTime - (10 * IN_MILLISECONDS);
            uint32 count_ANCIENT_WATER_SPIRIT = 0;
            uint32 count_STORM_LASHER = 0;
            uint32 count_SNAPLASHER = 0;
            for (std::list<DeadTrioMember*>::iterator iter = DeadTrioMemberData.begin(); iter != DeadTrioMemberData.end(); ++iter)
            {
                if ((*iter)->time >= before10Seconds) //Only count TrioMembers which where killed in the last 10 seconds
                    switch ((*iter)->type)
					{
						case NPC_ANCIENT_WATER_SPIRIT:
							count_ANCIENT_WATER_SPIRIT++;
							break;
						case NPC_STORM_LASHER:
							count_STORM_LASHER++;
							break;
						case NPC_SNAPLASHER:
							count_SNAPLASHER++;
							break;
					}
            }
            if (count_ANCIENT_WATER_SPIRIT >= 2 && count_STORM_LASHER >= 2 && count_SNAPLASHER >= 2)
            {
                events.ScheduleEvent(EVENT_ACHIEV_CANADIAN_DEFOREST, 1);
                deforestationCompleted = true;
            }
        }


        void SetGuidData(ObjectGuid guid, uint32 type) override
        {
            if (type == GUID_TRIO_MEMBER_DIED)
            {
                TrioWave* wave = FindTrio(guid);
                wave->alive -= 1;
                if (wave->alive == 0)
                {
                    if (Creature* creature = ObjectAccessor::GetCreature(*me, wave->one))
                    {
                        creature->CastSpell(me, SPELL_REMOVE_10STACK, true);
                        creature->DespawnOrUnsummon(3 * IN_MILLISECONDS);

                    }

                    if (Creature* creature = ObjectAccessor::GetCreature(*me, wave->two))
                    {
                        creature->CastSpell(me, SPELL_REMOVE_10STACK, true);
                        creature->DespawnOrUnsummon(3 * IN_MILLISECONDS);

                    }

                    if (Creature* creature = ObjectAccessor::GetCreature(*me, wave->three))
                    {
                        creature->CastSpell(me, SPELL_REMOVE_10STACK, true);
                        creature->DespawnOrUnsummon(12 * IN_MILLISECONDS);

                    }
                    activeWaves--;
                    TimeCheck();
                }
                else if (wave->alive == 2)
                {
                    uint8 index = TrioRespawnData.size();
                    TrioRespawnData.push_back(wave);
                    events.ScheduleEvent(EVENT_TRIO_RESPAWN + index, 12 * IN_MILLISECONDS);
                }

            }
        }

        uint32 GetData(uint32 type) const
        {
            switch (type)
            {
            case DATA_GETTING_BACK_TO_NATURE:
                return AttunedToNatureStack;
            case DATA_KNOCK_ON_WOOD:
                return ElderCount;
            }
            return 0;
        }

        void DespawnRoots()
        {
            std::list<Creature*> roots;
            me->GetCreatureListWithEntryInGrid(roots, NPC_IRON_ROOTS, 1000.0f);

            if (roots.empty())
                return;

            for (std::list<Creature*>::iterator iter = roots.begin(); iter != roots.end(); ++iter)
                (*iter)->DespawnOrUnsummon();

            std::list<Creature*> strengthenedRoots;
            me->GetCreatureListWithEntryInGrid(strengthenedRoots, NPC_STRENGTHENED_IRON_ROOTS, 1000.0f);

            if (strengthenedRoots.empty())
                return;

            for (std::list<Creature*>::iterator iter = strengthenedRoots.begin(); iter != strengthenedRoots.end(); ++iter)
                (*iter)->DespawnOrUnsummon();
        }

        TrioWave* FindTrio(ObjectGuid guid)
        {
            for (std::list<TrioWave*>::iterator itr = TrioWaveData.begin(); itr != TrioWaveData.end(); ++itr)
            {
                if ((*itr)->one == guid || (*itr)->two == guid || (*itr)->three == guid)
                    return (*itr);
            }
            return NULL;
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            BossAI::EnterEvadeMode();
            for (uint8 i = 0; i < 3; ++i)
            {
                if (Creature* elder = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_ELDER_BRIGHTLEAF + i))))
                    elder->AI()->EnterEvadeMode();
            }
        }

        void TimeCheck()
        {
            if ((WaveCount >= MAX_WAVE_COUNT) || activeWaves)
                return;

            uint32 timeDifference = WAVE_DURATION_MAX - waveTime;
            if (timeDifference <= WAVE_TIMEDIFF_MAX)
                events.RescheduleEvent(EVENT_WAVE, timeDifference);
            else
                events.RescheduleEvent(EVENT_WAVE, WAVE_TIMEDIFF_MAX);
        }

    private:
        Phases Phase;
        uint8 ElderCount;

        uint32 waveTime;
        uint8 activeWaves;
        uint8 CurrentWave;
        uint8 WaveCount;
        std::list<uint8> Waves;

        uint8 LasherDeadCount;
//        uint8 TrioDeadCount;

        std::list<TrioWave*> TrioWaveData;
        std::vector<TrioWave*> TrioRespawnData;

        // For Achievement "Back to nature"
        uint8 AttunedToNatureStack;

        //For Achievement "Deforestation"
        bool deforestationCompleted;
        std::list<DeadTrioMember*> DeadTrioMemberData;
        uint64 encounterTime;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_freyaAI(creature);
    }
};

class npc_iron_roots : public CreatureScript
{
public:
    npc_iron_roots() : CreatureScript("npc_iron_roots") { }

    struct npc_iron_rootsAI : public ScriptedAI
    {
        npc_iron_rootsAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            me->SetReactState(REACT_PASSIVE);
            summonerGUID.Clear();
        }

        void IsSummonedBy(Unit* summoner)
        {
            summonerGUID = summoner->GetGUID();
            me->SetFacingToObject(summoner);
            summoner->SetFacingToObject(me);
            me->SetInCombatWith(summoner);
            summoner->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
        }

        void JustDied(Unit* /*who*/)
        {
            if (Player* target = ObjectAccessor::GetPlayer(*me, summonerGUID))
            {
                target->RemoveAurasDueToSpell(SPELL_ROOTS_IRONBRANCH_DOT);
                target->RemoveAurasDueToSpell(SPELL_ROOTS_IRONBRANCH_DOT_25);
                target->RemoveAurasDueToSpell(SPELL_IRON_ROOTS_DOT);
                target->RemoveAurasDueToSpell(SPELL_IRON_ROOTS_DOT_25);
                target->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, false);
            }

            me->DespawnOrUnsummon();
        }

    private:
        ObjectGuid summonerGUID;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_iron_rootsAI(creature);
    }
};

class boss_elder_brightleaf : public CreatureScript
{
public:
    boss_elder_brightleaf() : CreatureScript("boss_elder_brightleaf") { }

    struct boss_elder_brightleafAI : public BossAI
    {
        boss_elder_brightleafAI(Creature* creature) : BossAI(creature, BOSS_ELDER_BRIGHTLEAF)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            BossAI::Reset();

            me->RemoveAurasDueToSpell(SPELL_DRAINED_OF_POWER);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Talk(RAND(SAY_BRIGHTLEAF_SLAY_1, SAY_BRIGHTLEAF_SLAY_2));
        }

        void JustDied(Unit* killer)
        {
            BossAI::JustDied(killer);
            instance->SetBossState(BOSS_ELDER_BRIGHTLEAF, DONE);
            Talk(SAY_BRIGHTLEAF_DEATH);
        }

        void EnterCombat(Unit* who)
        {
            BossAI::EnterCombat(who);

            if (!me->HasAura(SPELL_DRAINED_OF_POWER))
                Talk(SAY_BRIGHTLEAF_AGGRO);

            events.ScheduleEvent(EVENT_SOLAR_FLARE, urand(5, 7)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_UNSTABLE_SUN_BEAM, urand(7, 12)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_FLUX, 5 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim() || me->HasAura(SPELL_DRAINED_OF_POWER))
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_UNSTABLE_SUN_BEAM:
                    DoCast(SPELL_UNSTABLE_SUN_BEAM_SUMMON);
                    events.ScheduleEvent(EVENT_UNSTABLE_SUN_BEAM, urand(10, 15)*IN_MILLISECONDS);
                    break;
                case EVENT_SOLAR_FLARE:
                    me->CastCustomSpell(SPELL_SOLAR_FLARE, SPELLVALUE_MAX_TARGETS, me->GetAuraCount(SPELL_FLUX_AURA), me, false);
                    events.ScheduleEvent(EVENT_SOLAR_FLARE, urand(5, 10)*IN_MILLISECONDS);
                    break;
                case EVENT_FLUX:
                    DoCast(SPELL_FLUX);
                    events.ScheduleEvent(EVENT_FLUX, 5 * IN_MILLISECONDS);
                    break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void DoAction(int32 action)
        {
            switch (action)
            {
            case ACTION_FREYA_START_FIGHT:
                DoCastSelf(SPELL_DRAINED_OF_POWER, true);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->RemoveLootMode(LOOT_MODE_DEFAULT);
                me->SetInCombatWithZone();
                break;
            }
        }
    private:
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_elder_brightleafAI(creature);
    }
};

class boss_elder_stonebark : public CreatureScript
{
public:
    boss_elder_stonebark() : CreatureScript("boss_elder_stonebark") { }

    struct boss_elder_stonebarkAI : public BossAI
    {
        boss_elder_stonebarkAI(Creature* creature) : BossAI(creature, BOSS_ELDER_STONEBARK)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            BossAI::Reset();

            me->RemoveAurasDueToSpell(SPELL_DRAINED_OF_POWER);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Talk(RAND(SAY_STONEBARK_SLAY_1, SAY_STONEBARK_SLAY_2));
        }

        void JustDied(Unit* killer)
        {
            BossAI::JustDied(killer);
            instance->SetBossState(BOSS_ELDER_STONEBARK, DONE);
            Talk(SAY_STONEBARK_DEATH);
        }

        void EnterCombat(Unit* who)
        {
            BossAI::EnterCombat(who);

            if (!me->HasAura(SPELL_DRAINED_OF_POWER))
                Talk(SAY_STONEBARK_AGGRO);

            events.ScheduleEvent(EVENT_TREMOR, urand(10, 12)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_FISTS, urand(25, 35)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_BARK, urand(37, 40)*IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim() || me->HasAura(SPELL_DRAINED_OF_POWER))
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_BARK:
                    DoCastSelf(SPELL_PETRIFIED_BARK);
                    events.ScheduleEvent(EVENT_BARK, urand(30, 50)*IN_MILLISECONDS);
                    break;
                case EVENT_FISTS:
                    DoCastVictim(SPELL_FISTS_OF_STONE);
                    events.ScheduleEvent(EVENT_FISTS, urand(20, 30)*IN_MILLISECONDS);
                    break;
                case EVENT_TREMOR:
                    if (!me->HasAura(SPELL_FISTS_OF_STONE))
                        DoCastVictim(SPELL_GROUND_TREMOR);
                    events.ScheduleEvent(EVENT_TREMOR, urand(10, 20)*IN_MILLISECONDS);
                    break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void DoAction(int32 action)
        {
            switch (action)
            {
            case ACTION_FREYA_START_FIGHT:
                DoCastSelf(SPELL_DRAINED_OF_POWER, true);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->RemoveLootMode(LOOT_MODE_DEFAULT);
                me->SetInCombatWithZone();
                break;
            }
        }
    private:
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_elder_stonebarkAI(creature);
    }
};

class boss_elder_ironbranch : public CreatureScript
{
public:
    boss_elder_ironbranch() : CreatureScript("boss_elder_ironbranch") { }

    struct boss_elder_ironbranchAI : public BossAI
    {
        boss_elder_ironbranchAI(Creature* creature) : BossAI(creature, BOSS_ELDER_IRONBRANCH)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            BossAI::Reset();

            me->RemoveAurasDueToSpell(SPELL_DRAINED_OF_POWER);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Talk(RAND(SAY_IRONBRANCH_SLAY_1, SAY_IRONBRANCH_SLAY_2));
        }

        void JustDied(Unit* killer)
        {
            BossAI::JustDied(killer);
            instance->SetBossState(BOSS_ELDER_IRONBRANCH, DONE);
            Talk(SAY_IRONBRANCH_DEATH);
        }

        void EnterCombat(Unit* who)
        {
            BossAI::EnterCombat(who);

            if (!me->HasAura(SPELL_DRAINED_OF_POWER))
                Talk(SAY_IRONBRANCH_AGGRO);

            events.ScheduleEvent(EVENT_IMPALE, urand(18, 22)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_IRON_ROOTS, urand(12, 17)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_THORN_SWARM, urand(7, 12)*IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim() || me->HasAura(SPELL_DRAINED_OF_POWER))
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_IMPALE:
                    DoCastVictim(SPELL_IMPALE);
                    events.ScheduleEvent(EVENT_IMPALE, urand(15, 25)*IN_MILLISECONDS);
                    break;
                case EVENT_IRON_ROOTS:
                    DoCast(SPELL_ROOTS_IRONBRANCH_SUMMON);
                    events.ScheduleEvent(EVENT_IRON_ROOTS, urand(10, 20)*IN_MILLISECONDS);
                    break;
                case EVENT_THORN_SWARM:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                        DoCast(target, SPELL_THORN_SWARM);
                    events.ScheduleEvent(EVENT_THORN_SWARM, urand(8, 13)*IN_MILLISECONDS);
                    break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void DoAction(int32 action)
        {
            switch (action)
            {
            case ACTION_FREYA_START_FIGHT:
                DoCastSelf(SPELL_DRAINED_OF_POWER, true);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->RemoveLootMode(LOOT_MODE_DEFAULT);
                me->SetInCombatWithZone();
                break;
            }
        }
    private:
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_elder_ironbranchAI(creature);
    }
};

class npc_detonating_lasher : public CreatureScript
{
public:
    npc_detonating_lasher() : CreatureScript("npc_detonating_lasher") { }

    struct npc_detonating_lasherAI : public ScriptedAI
    {
        npc_detonating_lasherAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset()
        {
            LashTimer = urand(4, 7)*IN_MILLISECONDS;
            ChangeTargetTimer = 5 * IN_MILLISECONDS;
            StandUpTimer = 2 * IN_MILLISECONDS;
        }

        void UpdateAI(uint32 diff)
        {
            if (me->GetReactState() == REACT_PASSIVE)
            {
                if (StandUpTimer <= diff)
                {
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->SetInCombatWithZone();
                    me->SetStandState(UNIT_STAND_STATE_STAND);
                }
                else
                    StandUpTimer -= diff;
            }
            if (!UpdateVictim())
                return;

            if (LashTimer <= diff)
            {
                DoCast(SPELL_FLAME_LASH);
                LashTimer = urand(5, 10)*IN_MILLISECONDS;
            }
            else
                LashTimer -= diff;

            if (ChangeTargetTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                {
                    // Switching to other target - clear threat list and attack a new random target
                    me->GetThreatManager().resetAllAggro();
                    me->AI()->AttackStart(target);
                }
                ChangeTargetTimer = urand(5, 7)*IN_MILLISECONDS;
            }
            else
                ChangeTargetTimer -= diff;

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/)
        {
            DoCast(SPELL_DETONATE);
        }

    private:
        uint32 LashTimer;
        uint32 ChangeTargetTimer;
        uint32 StandUpTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_detonating_lasherAI(creature);
    }
};

struct TrioAI : ScriptedAI
{
    TrioAI(Creature* creature) : ScriptedAI(creature)
    {
        instance = me->GetInstanceScript();
    }

    void JustDied(Unit* /*killer*/)
    {
        if (Creature* freya = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(BOSS_FREYA)) : ObjectGuid::Empty))
            freya->AI()->SetGuidData(me->GetGUID(), GUID_TRIO_MEMBER_DIED);
    }

private:
    InstanceScript* instance;
};

class npc_ancient_water_spirit : public CreatureScript
{
public:
    npc_ancient_water_spirit() : CreatureScript("npc_ancient_water_spirit") { }

    struct npc_ancient_water_spiritAI : public TrioAI
    {
        npc_ancient_water_spiritAI(Creature* creature) : TrioAI(creature) {}

        void Reset()
        {
            TidalWaveTimer = 10 * IN_MILLISECONDS;
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (TidalWaveTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    DoCast(target, SPELL_TIDAL_WAVE);

                TidalWaveTimer = urand(12, 25)*IN_MILLISECONDS;
            }
            else
                TidalWaveTimer -= diff;

            DoMeleeAttackIfReady();
        }

    private:
        uint32 TidalWaveTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ancient_water_spiritAI(creature);
    }
};

class npc_storm_lasher : public CreatureScript
{
public:
    npc_storm_lasher() : CreatureScript("npc_storm_lasher") { }

    struct npc_storm_lasherAI : public TrioAI
    {
        npc_storm_lasherAI(Creature* creature) : TrioAI(creature) {}

        void Reset()
        {
            LightningLashTimer = 10 * IN_MILLISECONDS;
            StormboltTimer = 5 * IN_MILLISECONDS;
            DoCastSelf(SPELL_STORM_LASHER_VISUAL, true);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (LightningLashTimer <= diff)
            {
                DoCast(SPELL_LIGHTNING_LASH);
                LightningLashTimer = urand(7, 14)*IN_MILLISECONDS;
            }
            else
                LightningLashTimer -= diff;

            if (StormboltTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    DoCast(target, SPELL_STORMBOLT);
                StormboltTimer = urand(8, 12)*IN_MILLISECONDS;
            }
            else
                StormboltTimer -= diff;

            DoMeleeAttackIfReady();
        }

    private:
        uint32 LightningLashTimer;
        uint32 StormboltTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_storm_lasherAI(creature);
    }
};

class npc_snaplasher : public CreatureScript
{
public:
    npc_snaplasher() : CreatureScript("npc_snaplasher") { }

    struct npc_snaplasherAI : public TrioAI
    {
        npc_snaplasherAI(Creature* creature) : TrioAI(creature) {}

        void UpdateAI(uint32 /*diff*/)
        {
            if (!UpdateVictim())
                return;

            if (!me->HasAura(RAID_MODE(SPELL_HARDENED_BARK, SPELL_HARDENED_BARK_25)))
                DoCast(SPELL_HARDENED_BARK);

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_snaplasherAI(creature);
    }
};

class npc_ancient_conservator : public CreatureScript
{
public:
    npc_ancient_conservator() : CreatureScript("npc_ancient_conservator") { }

    struct npc_ancient_conservatorAI : public ScriptedAI
    {
        npc_ancient_conservatorAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset()
        {
            NatureFuryTimer = 7 * IN_MILLISECONDS;
            GripActive = false;
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoCast(SPELL_SUMMON_SPORE_PERIODIC);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (!GripActive)
            {
                DoCast(SPELL_CONSERVATOR_GRIP);
                GripActive = true;
            }

            if (NatureFuryTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true, true, -SPELL_NATURE_FURY))
                    DoCast(target, SPELL_NATURE_FURY);
                NatureFuryTimer = 5 * IN_MILLISECONDS;
            }
            else
                NatureFuryTimer -= diff;

            DoMeleeAttackIfReady();
        }

    private:
        uint32 NatureFuryTimer;
        bool GripActive;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ancient_conservatorAI(creature);
    }
};

//! Casted by freya, SPELL_FREYA_UNSTABLE_SUNBEAM
class npc_sun_beam : public CreatureScript
{
public:
    npc_sun_beam() : CreatureScript("npc_sun_beam") { }

    struct npc_sun_beamAI : public ScriptedAI
    {
        npc_sun_beamAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            me->SetDisplayId(me->GetCreatureTemplate()->Modelid2);
            me->SetReactState(REACT_PASSIVE);
            DoCastSelf(SPELL_FREYA_UNSTABLE_ENERGY_VISUAL, true);
            DoCastSelf(SPELL_FREYA_UNSTABLE_ENERGY, true);
        }

        void EnterEvadeMode(EvadeReason /*why*/) override {}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_sun_beamAI(creature);
    }
};

class npc_healthy_spore : public CreatureScript
{
public:
    npc_healthy_spore() : CreatureScript("npc_healthy_spore") { }

    struct npc_healthy_sporeAI : public ScriptedAI
    {
        npc_healthy_sporeAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            me->SetReactState(REACT_PASSIVE);
            DoCastSelf(SPELL_HEALTHY_SPORE_VISUAL);
            DoCastSelf(SPELL_POTENT_PHEROMONES);
            DoCastSelf(SPELL_GROW);
            LifeTimer = urand(22, 30)*IN_MILLISECONDS;
        }

        void UpdateAI(uint32 diff)
        {
            if (LifeTimer <= diff)
            {
                me->RemoveAurasDueToSpell(SPELL_GROW);
                me->DespawnOrUnsummon(2200);
                LifeTimer = urand(22, 30)*IN_MILLISECONDS;
            }
            else
                LifeTimer -= diff;
        }

    private:
        uint32 LifeTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_healthy_sporeAI(creature);
    }
};

class npc_eonars_gift : public CreatureScript
{
public:
    npc_eonars_gift() : CreatureScript("npc_eonars_gift") { }

    struct npc_eonars_giftAI : public ScriptedAI
    {
        npc_eonars_giftAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            LifeBindersGiftTimer = 12 * IN_MILLISECONDS;
            DoCastSelf(SPELL_GROW);
            DoCastSelf(SPELL_EONAR_VISUAL, true);
        }

        void UpdateAI(uint32 diff)
        {
            if (LifeBindersGiftTimer <= diff)
            {
                me->RemoveAurasDueToSpell(SPELL_GROW);
                DoCast(SPELL_LIFEBINDERS_GIFT);
                me->DespawnOrUnsummon(2500);
                LifeBindersGiftTimer = 12 * IN_MILLISECONDS;
            }
            else
                LifeBindersGiftTimer -= diff;
        }

    private:
        uint32 LifeBindersGiftTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_eonars_giftAI(creature);
    }
};

class npc_nature_bomb : public CreatureScript
{
public:
    npc_nature_bomb() : CreatureScript("npc_nature_bomb") { }

    struct npc_nature_bombAI : public ScriptedAI
    {
        npc_nature_bombAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            DoCast(SPELL_OBJECT_BOMB);
            BombTimer = urand(8, 10)*IN_MILLISECONDS;
        }

        void UpdateAI(uint32 diff)
        {
            if (BombTimer <= diff)
            {
                if (GameObject* go = me->FindNearestGameObject(GO_NATURE_BOMB, 1.0f))
                {
                    DoCastSelf(SPELL_NATURE_BOMB);
                    me->RemoveGameObject(go, true);
                    me->DespawnOrUnsummon();
                }

                BombTimer = 10 * IN_MILLISECONDS;
            }
            else
                BombTimer -= diff;
        }

    private:
        uint32 BombTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_nature_bombAI(creature);
    }
};

class npc_unstable_sun_beam : public CreatureScript
{
public:
    npc_unstable_sun_beam() : CreatureScript("npc_unstable_sun_beam") { }

    struct npc_unstable_sun_beamAI : public ScriptedAI
    {
        npc_unstable_sun_beamAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            DoCastSelf(SPELL_UNSTABLE_SUN_BEAM, true);
            DoCastSelf(SPELL_PHOTOSYNTHESIS);
        }

        void Reset()
        {
            UnstableEnergyTimer = urand(15, 25)*IN_MILLISECONDS;
        }

        void UpdateAI(uint32 diff)
        {
            if (UnstableEnergyTimer <= diff)
            {
                DoCastAOE(SPELL_UNSTABLE_ENERGY);
                me->DespawnOrUnsummon();
            }
            else
                UnstableEnergyTimer -= diff;
        }

    private:
        uint32 UnstableEnergyTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_unstable_sun_beamAI(creature);
    }
};

class npc_achievement_trigger : public CreatureScript
{
public:
    npc_achievement_trigger() : CreatureScript("npc_achievement_trigger") { }

    struct npc_achievement_triggerAI : public ScriptedAI
    {
        npc_achievement_triggerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset() override
        {
            StartCounter = false;
            LumberJackCount = 0;
        }

        void SetData(uint32 type, uint32 data) override
        {
            if (type == DATA_LUMBER_JACK && data == DATA_LUMBER_JACK_COUNT)
            {
                ++LumberJackCount;
                if (!StartCounter)
                {
                    StartCounter = true;
                    _events.ScheduleEvent(EVENT_RESET_LUMBER_JACK_COUNTER, 15 * IN_MILLISECONDS);
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_RESET_LUMBER_JACK_COUNTER:
                    if (LumberJackCount == 3)
                        instance->DoCompleteAchievement(RAID_MODE(ACHIEVEMENT_LUMBERJACK_10, ACHIEVEMENT_LUMBERJACK_25));
                    LumberJackCount = 0;
                    StartCounter = false;
                    break;
                default:
                    break;
                }
            }
        }
    private:
        InstanceScript* instance;
        EventMap _events;
        uint8 LumberJackCount;
        bool StartCounter;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_achievement_triggerAI(creature);
    }
};

class spell_freya_attuned_to_nature_dose_reduction : public SpellScriptLoader
{
public:
    spell_freya_attuned_to_nature_dose_reduction() : SpellScriptLoader("spell_freya_attuned_to_nature_dose_reduction") {}

    class spell_freya_attuned_to_nature_dose_reduction_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_freya_attuned_to_nature_dose_reduction_SpellScript)

            void HandleScript(SpellEffIndex /*effIndex*/)
        {
            Unit* target = GetHitUnit();

            Aura* attundedToNature = target->GetAura(GetEffectValue());
            if (!attundedToNature)
                return;

            switch (GetSpellInfo()->Id)
            {
            case SPELL_REMOVE_2STACK:
                attundedToNature->ModStackAmount(-2);
                break;
            case SPELL_REMOVE_10STACK:
                attundedToNature->ModStackAmount(-10);
                break;
            case SPELL_REMOVE_25STACK:
                attundedToNature->ModStackAmount(-25);
                break;
            }
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_freya_attuned_to_nature_dose_reduction_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_freya_attuned_to_nature_dose_reduction_SpellScript();
    }
};

class spell_unstable_energy : public SpellScriptLoader
{
public:
    spell_unstable_energy() : SpellScriptLoader("spell_unstable_energy") { }

    class spell_unstable_energy_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_unstable_energy_SpellScript);

        void CalculateDamage(SpellEffIndex /*effIndex*/)
        {
            uint32 sunbeamstack = GetHitUnit()->GetAuraCount(SPELL_UNSTABLE_SUN_BEAM_TRIGGERED);

            SetHitDamage(int32(GetHitDamage() * (sunbeamstack * 0.10f + 0.5f)));
        }

        void HandleAfterHit()
        {
            if (GetHitUnit()->HasAura(SPELL_UNSTABLE_SUN_BEAM_TRIGGERED))
                GetHitUnit()->RemoveAurasDueToSpell(SPELL_UNSTABLE_SUN_BEAM_TRIGGERED);
        }
        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_unstable_energy_SpellScript::CalculateDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            AfterHit += SpellHitFn(spell_unstable_energy_SpellScript::HandleAfterHit);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_unstable_energy_SpellScript();
    }
};

class spell_tidal_wave : public SpellScriptLoader
{
public:
    spell_tidal_wave() : SpellScriptLoader("spell_tidal_wave") {}

    class spell_tidal_wave_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_tidal_wave_SpellScript);

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            GetCaster()->CastSpell(GetHitUnit(),
                GetCaster()->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL ? SPELL_TIDAL_WAVE_EFFECT : SPELL_TIDAL_WAVE_EFFECT_25,
                true);
        }

        void Register()
        {
            OnEffectLaunch += SpellEffectFn(spell_tidal_wave_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_tidal_wave_SpellScript();
    }
};

class spell_brightleaf_flux_apply : public SpellScriptLoader
{
public:
    spell_brightleaf_flux_apply() : SpellScriptLoader("spell_brightleaf_flux_apply") {}

    class spell_brightleaf_flux_apply_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_brightleaf_flux_apply_SpellScript);

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            GetCaster()->RemoveAurasDueToSpell(SPELL_FLUX_AURA);
            GetCaster()->CastCustomSpell(SPELL_FLUX_AURA, SPELLVALUE_AURA_STACK, urand(1, 10));
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_brightleaf_flux_apply_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_brightleaf_flux_apply_SpellScript();
    }
};

class spell_conservator_healty_spore : public SpellScriptLoader
{
public:
    spell_conservator_healty_spore() : SpellScriptLoader("spell_conservator_healty_spore") {}

    class spell_conservator_healty_spore_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_conservator_healty_spore_AuraScript);

        void OnPeriodic(AuraEffect const* /*aurEff*/)
        {
            PreventDefaultAction();

            Unit* owner = GetUnitOwner();

            owner->CastSpell(owner, SPELL_SPORE_SUMMON_NE);
            owner->CastSpell(owner, SPELL_SPORE_SUMMON_NW);
            owner->CastSpell(owner, SPELL_SPORE_SUMMON_SE);
            owner->CastSpell(owner, SPELL_SPORE_SUMMON_SW);
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_conservator_healty_spore_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_conservator_healty_spore_AuraScript();
    }
};

class spell_freya_summon_lashers : public SpellScriptLoader
{
public:
    spell_freya_summon_lashers() : SpellScriptLoader("spell_freya_summon_lashers") {}

    class spell_freya_summon_lashers_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_freya_summon_lashers_SpellScript);

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            for (uint8 i = 0; i < 10; ++i)
                GetCaster()->CastSpell(GetCaster(), SPELL_SUMMON_LASHER_SINGLE, true);
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_freya_summon_lashers_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_freya_summon_lashers_SpellScript();
    }
};

class achievement_knock_on_wood : public AchievementCriteriaScript
{
public:
    achievement_knock_on_wood() : AchievementCriteriaScript("achievement_knock_on_wood") {}

    bool OnCheck(Player* /*player*/, Unit* target)
    {
        if (!target)
            return false;

        if (Creature* Freya = target->ToCreature())
            if (Freya->AI()->GetData(DATA_KNOCK_ON_WOOD) >= 1)
                return true;

        return false;
    }
};

class achievement_knock_knock_on_wood : public AchievementCriteriaScript
{
public:
    achievement_knock_knock_on_wood() : AchievementCriteriaScript("achievement_knock_knock_on_wood") {}

    bool OnCheck(Player* /*player*/, Unit* target)
    {
        if (!target)
            return false;

        if (Creature* Freya = target->ToCreature())
            if (Freya->AI()->GetData(DATA_KNOCK_ON_WOOD) >= 2)
                return true;

        return false;
    }
};

class achievement_knock_knock_knock_on_wood : public AchievementCriteriaScript
{
public:
    achievement_knock_knock_knock_on_wood() : AchievementCriteriaScript("achievement_knock_knock_knock_on_wood") {}

    bool OnCheck(Player* /*player*/, Unit* target)
    {
        if (!target)
            return false;

        if (Creature* Freya = target->ToCreature())
            if (Freya->AI()->GetData(DATA_KNOCK_ON_WOOD) == 3)
                return true;

        return false;
    }
};

class achievement_get_back_to_nature : public AchievementCriteriaScript
{
public:
    achievement_get_back_to_nature() : AchievementCriteriaScript("achievement_get_back_to_nature") {}

    bool OnCheck(Player* /*player*/, Unit* target)
    {
        if (!target)
            return false;

        if (Creature* Freya = target->ToCreature())
            if (Freya->AI()->GetData(DATA_GETTING_BACK_TO_NATURE) >= 25)
                return true;

        return false;
    }
};

void AddSC_boss_freya()
{
    new boss_freya();
    new boss_elder_brightleaf();
    new boss_elder_ironbranch();
    new boss_elder_stonebark();

    new npc_ancient_conservator();
    new npc_snaplasher();
    new npc_storm_lasher();
    new npc_ancient_water_spirit();
    new npc_detonating_lasher();
    new npc_sun_beam();
    new npc_nature_bomb();
    new npc_eonars_gift();
    new npc_healthy_spore();
    new npc_unstable_sun_beam();
    new npc_iron_roots();
    new npc_achievement_trigger();

    new spell_unstable_energy();
    new spell_freya_attuned_to_nature_dose_reduction();
    new spell_brightleaf_flux_apply();
    new spell_conservator_healty_spore();
    new spell_tidal_wave();
    new spell_freya_summon_lashers();

    new achievement_knock_on_wood();
    new achievement_knock_knock_on_wood();
    new achievement_knock_knock_knock_on_wood();
    new achievement_get_back_to_nature();
}
