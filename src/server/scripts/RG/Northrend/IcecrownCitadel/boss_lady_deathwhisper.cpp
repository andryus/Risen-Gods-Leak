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

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "PoolMgr.h"
#include "Group.h"
#include "icecrown_citadel.h"
#include "SpellInfo.h"
#include "Player.h"
#include "SpellAuraEffects.h"

enum ScriptTexts
{
    // Lady Deathwhisper
    SAY_INTRO_1                 = 0,
    SAY_INTRO_2                 = 1,
    SAY_INTRO_3                 = 2,
    SAY_INTRO_4                 = 3,
    SAY_INTRO_5                 = 4,
    SAY_INTRO_6                 = 5,
    SAY_INTRO_7                 = 6,
    SAY_AGGRO                   = 7,
    SAY_PHASE_2                 = 8,
    EMOTE_PHASE_2               = 9,
    SAY_DOMINATE_MIND           = 10,
    SAY_DARK_EMPOWERMENT        = 11,
    SAY_DARK_TRANSFORMATION     = 12,
    SAY_ANIMATE_DEAD            = 13,
    SAY_KILL                    = 14,
    SAY_BERSERK                 = 15,
    SAY_DEATH                   = 16,

    // Darnavan
    SAY_DARNAVAN_AGGRO          = 0,
    SAY_DARNAVAN_RESCUED        = 1,
};

enum Spells
{
    // Lady Deathwhisper
    SPELL_MANA_BARRIER              = 70842,
    SPELL_SHADOW_BOLT               = 71254,
    SPELL_DEATH_AND_DECAY           = 71001,
    SPELL_DOMINATE_MIND_H           = 71289,
    SPELL_FROSTBOLT                 = 71420,
    SPELL_FROSTBOLT_VOLLEY          = 72905,
    SPELL_TOUCH_OF_INSIGNIFICANCE   = 71204,
    SPELL_SUMMON_SHADE              = 71363,
    SPELL_SUMMON_SHADE_DUMMY        = 72478,
    SPELL_SHADOW_CHANNELING         = 43897, // Prefight, during intro
    SPELL_DARK_TRANSFORMATION_T     = 70895,
    SPELL_DARK_EMPOWERMENT_T        = 70896,
    SPELL_DARK_MARTYRDOM_T          = 70897,

    // Achievement
    SPELL_FULL_HOUSE                = 72827, // does not exist in dbc but still can be used for criteria check

    // Both Adds
    SPELL_TELEPORT_VISUAL           = 41236,

    // Fanatics
    SPELL_DARK_TRANSFORMATION       = 70900,
    SPELL_NECROTIC_STRIKE           = 70659,
    SPELL_SHADOW_CLEAVE             = 70670,
    SPELL_VAMPIRIC_MIGHT            = 70674,
    SPELL_VAMPIRIC_MIGHT_HEAL       = 70677,
    SPELL_FANATIC_S_DETERMINATION   = 71235,
    SPELL_DARK_MARTYRDOM_FANATIC    = 71236,

    //  Adherents
    SPELL_DARK_EMPOWERMENT          = 70901,
    SPELL_FROST_FEVER               = 67767,
    SPELL_DEATHCHILL_BOLT           = 70594,
    SPELL_DEATHCHILL_BLAST          = 70906,
    SPELL_CURSE_OF_TORPOR           = 71237,
    SPELL_SHORUD_OF_THE_OCCULT      = 70768,
    SPELL_ADHERENT_S_DETERMINATION  = 71234,
    SPELL_DARK_MARTYRDOM_ADHERENT   = 70903,

    // Vengeful Shade
    SPELL_VENGEFUL_BLAST            = 71544,
    SPELL_VENGEFUL_BLAST_PASSIVE    = 71494,
    SPELL_VENGEFUL_BLAST_25N        = 72010,
    SPELL_VENGEFUL_BLAST_10H        = 72011,
    SPELL_VENGEFUL_BLAST_25H        = 72012,

    // Darnavan
    SPELL_BLADESTORM                = 65947,
    SPELL_CHARGE                    = 65927,
    SPELL_INTIMIDATING_SHOUT        = 65930,
    SPELL_MORTAL_STRIKE             = 65926,
    SPELL_SHATTERING_THROW          = 65940,
    SPELL_SUNDER_ARMOR              = 65936,
};

enum EventTypes
{
    // Lady Deathwhisper
    EVENT_INTRO_2                       = 1,
    EVENT_INTRO_3                       = 2,
    EVENT_INTRO_4                       = 3,
    EVENT_INTRO_5                       = 4,
    EVENT_INTRO_6                       = 5,
    EVENT_INTRO_7                       = 6,
    EVENT_BERSERK                       = 7,
    EVENT_DEATH_AND_DECAY               = 8,
    EVENT_DOMINATE_MIND_H               = 9,

    // Phase 1 only
    EVENT_P1_SUMMON_WAVE                = 10,
    EVENT_P1_SHADOW_BOLT                = 11,
    EVENT_P1_EMPOWER_CULTIST            = 12,
    EVENT_P1_REANIMATE_CULTIST          = 13,

    // Phase 2 only
    EVENT_P2_SUMMON_WAVE                = 14,
    EVENT_P2_FROSTBOLT                  = 15,
    EVENT_P2_FROSTBOLT_VOLLEY           = 16,
    EVENT_P2_TOUCH_OF_INSIGNIFICANCE    = 17,
    EVENT_P2_SUMMON_SHADE               = 18,

    // Shared adds events
    EVENT_CULTIST_DARK_MARTYRDOM        = 19,

    // Cult Fanatic
    EVENT_FANATIC_NECROTIC_STRIKE       = 20,
    EVENT_FANATIC_SHADOW_CLEAVE         = 21,
    EVENT_FANATIC_VAMPIRIC_MIGHT        = 22,

    // Cult Adherent
    EVENT_ADHERENT_FROST_FEVER          = 23,
    EVENT_ADHERENT_DEATHCHILL           = 24,
    EVENT_ADHERENT_CURSE_OF_TORPOR      = 25,
    EVENT_ADHERENT_SHORUD_OF_THE_OCCULT = 26,

    // Darnavan
    EVENT_DARNAVAN_BLADESTORM           = 27,
    EVENT_DARNAVAN_CHARGE               = 28,
    EVENT_DARNAVAN_INTIMIDATING_SHOUT   = 29,
    EVENT_DARNAVAN_MORTAL_STRIKE        = 30,
    EVENT_DARNAVAN_SHATTERING_THROW     = 31,
    EVENT_DARNAVAN_SUNDER_ARMOR         = 32,

    // Vengful Shade
    EVENT_VENGEFUL_SHADE_ATTACK_START   = 34,
};

enum Phases
{
    PHASE_ALL       = 0,
    PHASE_INTRO     = 1,
    PHASE_ONE       = 2,
    PHASE_TWO       = 3
};

enum DeprogrammingData
{
    NPC_DARNAVAN_10         = 38472,
    NPC_DARNAVAN_25         = 38485,
    NPC_DARNAVAN_CREDIT_10  = 39091,
    NPC_DARNAVAN_CREDIT_25  = 39092,

    ACTION_COMPLETE_QUEST   = -384720,
    POINT_DESPAWN           = 384721,
};

#define NPC_DARNAVAN        RAID_MODE<uint32>(NPC_DARNAVAN_10, NPC_DARNAVAN_25, NPC_DARNAVAN_10, NPC_DARNAVAN_25)
#define NPC_DARNAVAN_CREDIT RAID_MODE<uint32>(NPC_DARNAVAN_CREDIT_10, NPC_DARNAVAN_CREDIT_25, NPC_DARNAVAN_CREDIT_10, NPC_DARNAVAN_CREDIT_25)
#define QUEST_DEPROGRAMMING RAID_MODE<uint32>(QUEST_DEPROGRAMMING_10, QUEST_DEPROGRAMMING_25, QUEST_DEPROGRAMMING_10, QUEST_DEPROGRAMMING_25)

uint32 const SummonEntries[2] = {NPC_CULT_FANATIC, NPC_CULT_ADHERENT};

#define GUID_CULTIST    1

Position const SummonPositions[7] =
{
    {-578.7066f, 2154.167f, 51.01529f, 1.692969f}, // 1 Left Door 1 (Cult Fanatic)
    {-598.9028f, 2155.005f, 51.01530f, 1.692969f}, // 2 Left Door 2 (Cult Adherent)
    {-619.2864f, 2154.460f, 51.01530f, 1.692969f}, // 3 Left Door 3 (Cult Fanatic)
    {-578.6996f, 2269.856f, 51.01529f, 4.590216f}, // 4 Right Door 1 (Cult Adherent)
    {-598.9688f, 2269.264f, 51.01529f, 4.590216f}, // 5 Right Door 2 (Cult Fanatic)
    {-619.4323f, 2268.523f, 51.01530f, 4.590216f}, // 6 Right Door 3 (Cult Adherent)
    {-524.2480f, 2211.920f, 62.90960f, 3.141592f}, // 7 Upper (Random Cultist)
};

class DaranavanMoveEvent : public BasicEvent
{
    public:
        DaranavanMoveEvent(Creature& darnavan) : _darnavan(darnavan) { }

        bool Execute(uint64 /*time*/, uint32 /*diff*/)
        {
            _darnavan.GetMotionMaster()->MovePoint(POINT_DESPAWN, SummonPositions[6]);
            return true;
        }

    private:
        Creature& _darnavan;
};

class boss_lady_deathwhisper : public CreatureScript
{
    public:
        boss_lady_deathwhisper() : CreatureScript("boss_lady_deathwhisper") { }

        struct boss_lady_deathwhisperAI : public BossAI
        {
            boss_lady_deathwhisperAI(Creature* creature) : BossAI(creature, DATA_LADY_DEATHWHISPER), _introDone(false)
            {
            }

            void Reset()
            {
                BossAI::Reset();
                me->SetReactState(REACT_AGGRESSIVE);
                me->SetPower(POWER_MANA, me->GetMaxPower(POWER_MANA));
                events.SetPhase(PHASE_ONE);
                _waveCounter = 0;
                _nextVengefulShadeTargetGUID.clear();
                _darnavanGUID.Clear();
                DoCastSelf(SPELL_SHADOW_CHANNELING);
                me->RemoveAurasDueToSpell(SPELL_BERSERK);
                me->RemoveAurasDueToSpell(SPELL_MANA_BARRIER);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
                me->setRegeneratingMana(false);

                constexpr uint32 SPELL_PRIEST_HYMN_OF_HOPE = 64904; // creature semms to be immune to this spell.
                me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_PRIEST_HYMN_OF_HOPE, true);
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (!_introDone && me->IsWithinDistInMap(who, 110.0f))
                {
                    _introDone = true;
                    Talk(SAY_INTRO_1);
                    events.SetPhase(PHASE_INTRO);
                    events.ScheduleEvent(EVENT_INTRO_2, 11000, 0, PHASE_INTRO);
                    events.ScheduleEvent(EVENT_INTRO_3, 21000, 0, PHASE_INTRO);
                    events.ScheduleEvent(EVENT_INTRO_4, 31500, 0, PHASE_INTRO);
                    events.ScheduleEvent(EVENT_INTRO_5, 39500, 0, PHASE_INTRO);
                    events.ScheduleEvent(EVENT_INTRO_6, 48500, 0, PHASE_INTRO);
                    events.ScheduleEvent(EVENT_INTRO_7, 58000, 0, PHASE_INTRO);
                }
                // We will enter combat
                ScriptedAI::MoveInLineOfSight(who);
            }

            void AttackStart(Unit* victim)
            {
                if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                    return;

                if (victim && me->Attack(victim, true) && !events.IsInPhase(PHASE_ONE))
                    me->GetMotionMaster()->MoveChase(victim);
            }

            void EnterCombat(Unit* who)
            {
                if (!instance->CheckRequiredBosses(DATA_LADY_DEATHWHISPER, who->ToPlayer()))
                {
                    EnterEvadeMode();
                    instance->DoCastSpellOnPlayers(LIGHT_S_HAMMER_TELEPORT);
                    return;
                }

                me->setActive(true);
                me->SetInCombatWithZone();
                me->SetCombatPulseDelay(5);

                events.Reset();
                events.SetPhase(PHASE_ONE);
                // phase-independent events
                events.ScheduleEvent(EVENT_BERSERK, 10min);
                events.ScheduleEvent(EVENT_DEATH_AND_DECAY, 12s, 18s);
                // phase one only
                events.ScheduleEvent(EVENT_P1_SUMMON_WAVE, 5s, 0, PHASE_ONE);
                events.ScheduleEvent(EVENT_P1_SHADOW_BOLT, 1s, 0, PHASE_ONE);
                events.ScheduleEvent(EVENT_P1_EMPOWER_CULTIST, 20s, 30s, 0, PHASE_ONE);
                if (GetDifficulty() != RAID_DIFFICULTY_10MAN_NORMAL)
                    events.ScheduleEvent(EVENT_DOMINATE_MIND_H, 27s, 32s);

                Talk(SAY_AGGRO);
                DoStartNoMovement(who);
                me->RemoveAurasDueToSpell(SPELL_SHADOW_CHANNELING);
                DoCastSelf(SPELL_MANA_BARRIER, true);

                instance->SetBossState(DATA_LADY_DEATHWHISPER, IN_PROGRESS);
            }

            void JustDied(Unit* killer)
            {
                Talk(SAY_DEATH);

                std::set<uint32> livingAddEntries;
                // Full House achievement
                for (SummonList::iterator itr = summons.begin(); itr != summons.end(); ++itr)
                    if (Unit* unit = ObjectAccessor::GetUnit(*me, *itr))
                        if (unit->IsAlive() && unit->GetEntry() != NPC_VENGEFUL_SHADE)
                            livingAddEntries.insert(unit->GetEntry());

                if (livingAddEntries.size() >= 5)
                    instance->DoUpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, SPELL_FULL_HOUSE, 0, me);

                if (Creature* darnavan = ObjectAccessor::GetCreature(*me, _darnavanGUID))
                {
                    if (darnavan->IsAlive())
                    {
                        darnavan->setFaction(35);
                        darnavan->CombatStop(true);
                        darnavan->GetMotionMaster()->MoveIdle();
                        darnavan->SetReactState(REACT_PASSIVE);
                        darnavan->m_Events.AddEvent(new DaranavanMoveEvent(*darnavan), darnavan->m_Events.CalculateTime(10000));
                        darnavan->AI()->Talk(SAY_DARNAVAN_RESCUED);
                        if (Player* owner = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
                        {
                            if (Group* group = owner->GetGroup())
                            {
                                for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
                                    if (Player* member = itr->GetSource())
                                        if (member->GetQuestStatus(QUEST_DEPROGRAMMING) == QUEST_STATUS_INCOMPLETE)
                                            member->KilledMonsterCredit(NPC_DARNAVAN_CREDIT);
                            }
                            else
                            {
                                if (owner->GetQuestStatus(QUEST_DEPROGRAMMING) == QUEST_STATUS_INCOMPLETE)
                                    owner->KilledMonsterCredit(NPC_DARNAVAN);
                            }
                        }
                    }
                }

                BossAI::JustDied(killer);
            }

            void JustReachedHome()
            {
                BossAI::JustReachedHome();
                instance->SetBossState(DATA_LADY_DEATHWHISPER, FAIL);

                summons.DespawnAll();
                if (Creature* darnavan = ObjectAccessor::GetCreature(*me, _darnavanGUID))
                {
                    darnavan->DespawnOrUnsummon();
                    _darnavanGUID.Clear();
                }
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_KILL);
            }

            void DamageTaken(Unit* /*damageDealer*/, uint32& damage)
            {
                // phase transition
                if (events.IsInPhase(PHASE_ONE) && damage > me->GetPower(POWER_MANA))
                {
                    Talk(SAY_PHASE_2);
                    Talk(EMOTE_PHASE_2);
                    DoStartMovement(me->GetVictim());
                    damage -= me->GetPower(POWER_MANA);
                    me->SetPower(POWER_MANA, 0);
                    me->RemoveAurasDueToSpell(SPELL_MANA_BARRIER);
                    ResetThreatList();
                    events.SetPhase(PHASE_TWO);
                    events.ScheduleEvent(EVENT_P2_TOUCH_OF_INSIGNIFICANCE, 2s, 0, PHASE_TWO);
                    events.ScheduleEvent(EVENT_P2_FROSTBOLT, 5s, 0, PHASE_TWO);
                    events.ScheduleEvent(EVENT_P2_FROSTBOLT_VOLLEY, 19s, 21s, 0, PHASE_TWO);
                    events.ScheduleEvent(EVENT_P2_SUMMON_SHADE, 19s, 21s, 0, PHASE_TWO);
                    // on heroic mode Lady Deathwhisper is immune to taunt effects in phase 2 and continues summoning adds
                    if (IsHeroic())
                    {
                        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);

                        events.ScheduleEvent(EVENT_P2_SUMMON_WAVE, (events.GetNextEventTime(EVENT_P1_SUMMON_WAVE) - events.GetTimer()), 0, PHASE_TWO);
                    }
                    if (Player* player = me->FindNearestPlayer(100.0f))
                        AttackStart(player);
                }
            }

            void JustSummoned(Creature* summon)
            {
                summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, true);

                if (summon->GetEntry() == NPC_DARNAVAN)
                    _darnavanGUID = summon->GetGUID();
                else
                    summons.Summon(summon);

                Unit* target = NULL;                                                       // CAN be NULL
                if (summon->GetEntry() != NPC_VENGEFUL_SHADE)
                    summon->AI()->AttackStart(target);                                     // Wave adds

                if (summon->GetEntry() == NPC_REANIMATED_FANATIC)
                    summon->CastSpell(summon, SPELL_FANATIC_S_DETERMINATION, true);
                else if (summon->GetEntry() == NPC_REANIMATED_ADHERENT)
                    summon->CastSpell(summon, SPELL_ADHERENT_S_DETERMINATION, true);

                if (summon->GetEntry() == NPC_VENGEFUL_SHADE)
                {
                    if (!_nextVengefulShadeTargetGUID.empty())
                    {
                        summon->AI()->SetGuidData(_nextVengefulShadeTargetGUID.front());
                        _nextVengefulShadeTargetGUID.pop_front();
                    }
                }
            }

            void UpdateAI(uint32 diff)
            {
                if ((!UpdateVictim() && !events.IsInPhase(PHASE_INTRO)))
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING) && !events.IsInPhase(PHASE_INTRO))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_INTRO_2:
                            Talk(SAY_INTRO_2);
                            break;
                        case EVENT_INTRO_3:
                            Talk(SAY_INTRO_3);
                            break;
                        case EVENT_INTRO_4:
                            Talk(SAY_INTRO_4);
                            break;
                        case EVENT_INTRO_5:
                            Talk(SAY_INTRO_5);
                            break;
                        case EVENT_INTRO_6:
                            Talk(SAY_INTRO_6);
                            break;
                        case EVENT_INTRO_7:
                            Talk(SAY_INTRO_7);
                            break;
                        case EVENT_DEATH_AND_DECAY:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                DoCast(target, SPELL_DEATH_AND_DECAY);
                            events.Repeat(22s, 32s);
                            break;
                        case EVENT_DOMINATE_MIND_H:
                        {
                            Talk(SAY_DOMINATE_MIND);

                            std::list<Unit*> targetList;
                            SelectTargetList(targetList, RAID_MODE<uint8>(0, 1, 1, 3), SELECT_TARGET_RANDOM, 0, [this](Unit* target)
                            {
                                return target->GetTypeId() == TYPEID_PLAYER &&
                                    me->GetVictim() != target &&
                                    !target->ToPlayer()->IsTank() &&
                                    !target->IsCharmed();

                            });

                            for (Unit* target : targetList)
                                DoCast(target, SPELL_DOMINATE_MIND_H, true);

                            events.Repeat(40s, 45s);
                            break;
                        }
                        case EVENT_P1_SUMMON_WAVE:
                            SummonWaveP1();
                            events.Repeat(IsHeroic() ? 45s : 60s);
                            break;
                        case EVENT_P1_REANIMATE_CULTIST:
                            ReanimateCultist();
                            break;
                        case EVENT_P1_EMPOWER_CULTIST:
                            EmpowerCultist();
                            events.Repeat(18s, 25s);
                            break;
                        case EVENT_P1_SHADOW_BOLT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                DoCast(target, SPELL_SHADOW_BOLT);
                            events.Repeat(2s);
                            break;
                        case EVENT_P2_FROSTBOLT:
                            DoCastVictim(SPELL_FROSTBOLT);
                            events.Repeat(Is25ManRaid() ? 7s : 11s);
                            break;
                        case EVENT_P2_FROSTBOLT_VOLLEY:
                            DoCastAOE(SPELL_FROSTBOLT_VOLLEY);
                            events.Repeat(20s);
                            break;
                        case EVENT_P2_TOUCH_OF_INSIGNIFICANCE:
                            DoCastVictim(SPELL_TOUCH_OF_INSIGNIFICANCE);
                            events.Repeat(9s, 13s);
                            break;
                        case EVENT_P2_SUMMON_SHADE:
                        {
                            std::list<Unit*> targetList;
                            SelectTargetList(targetList, RAID_MODE<uint8>(1, 3, 1, 3), SELECT_TARGET_RANDOM, 1, 0.0f, true, false);
                            for (Unit* target : targetList)
                                DoCast(target, SPELL_SUMMON_SHADE);

                            events.Repeat(12s);
                            break;
                        }
                        case EVENT_P2_SUMMON_WAVE:
                            SummonWaveP2();
                            events.Repeat(45s);
                            break;
                        case EVENT_BERSERK:
                            DoCastSelf(SPELL_BERSERK);
                            Talk(SAY_BERSERK);
                            break;
                        default:
                            break;
                    }
                }

                // We should not melee attack when barrier is up
                if (me->HasAura(SPELL_MANA_BARRIER))
                    return;

                DoMeleeAttackIfReady(true);
            }

            // summoning function for first phase
            void SummonWaveP1()
            {
                uint8 addIndex = _waveCounter & 1;
                uint8 addIndexOther = uint8(addIndex ^ 1);

                // Summon first add, replace it with Darnavan if weekly quest is active
                if (_waveCounter || (me->GetInstanceId() % 5 != 0))
                    Summon(SummonEntries[addIndex], SummonPositions[addIndex * 3]);
                else
                    Summon(NPC_DARNAVAN, SummonPositions[addIndex * 3]);

                Summon(SummonEntries[addIndexOther], SummonPositions[addIndex * 3 + 1]);
                Summon(SummonEntries[addIndex], SummonPositions[addIndex * 3 + 2]);
                if (Is25ManRaid())
                {
                    Summon(SummonEntries[addIndexOther], SummonPositions[addIndexOther * 3]);
                    Summon(SummonEntries[addIndex], SummonPositions[addIndexOther * 3 + 1]);
                    Summon(SummonEntries[addIndexOther], SummonPositions[addIndexOther * 3 + 2]);
                    Summon(SummonEntries[urand(0, 1)], SummonPositions[6]);
                }

                ++_waveCounter;
            }

            // summoning function for second phase
            void SummonWaveP2()
            {
                if (Is25ManRaid())
                {
                    uint8 addIndex = _waveCounter & 1;
                    Summon(SummonEntries[addIndex], SummonPositions[addIndex * 3]);
                    Summon(SummonEntries[addIndex ^ 1], SummonPositions[addIndex * 3 + 1]);
                    Summon(SummonEntries[addIndex], SummonPositions[addIndex * 3+ 2]);
                }
                else
                    Summon(SummonEntries[urand(0, 1)], SummonPositions[6]);

                ++_waveCounter;
            }

            // helper for summoning wave mobs
            void Summon(uint32 entry, const Position& pos)
            {
                if (TempSummon* summon = me->SummonCreature(entry, pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10*IN_MILLISECONDS))
                    summon->AI()->DoCast(summon, SPELL_TELEPORT_VISUAL);
            }

            void SetGuidData(ObjectGuid guid, uint32 id) override
            {
                switch (id)
                {
                    case GUID_CULTIST:
                        _reanimationQueue.push_back(guid);
                        events.ScheduleEvent(EVENT_P1_REANIMATE_CULTIST, 3s, 0, PHASE_ONE);
                        break;
                    case SPELL_SUMMON_SHADE:
                        _nextVengefulShadeTargetGUID.push_back(guid);
                        break;
                    default:
                        break;
                }
            }

            void ReanimateCultist()
            {
                if (_reanimationQueue.empty())
                    return;

                ObjectGuid cultistGUID = _reanimationQueue.front();
                Creature* cultist = ObjectAccessor::GetCreature(*me, cultistGUID);
                _reanimationQueue.pop_front();
                if (!cultist)
                    return;

                Talk(SAY_ANIMATE_DEAD);
                DoCast(cultist, SPELL_DARK_MARTYRDOM_T);
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_DARK_MARTYRDOM_T)
                {
                    Position pos;
                    target->GetPosition(&pos);
                    if (target->GetEntry() == NPC_CULT_FANATIC)
                        me->SummonCreature(NPC_REANIMATED_FANATIC, pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10*IN_MILLISECONDS);
                    else
                        me->SummonCreature(NPC_REANIMATED_ADHERENT, pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10*IN_MILLISECONDS);

                    if (TempSummon* summon = target->ToTempSummon())
                        summon->UnSummon();
                }
            }

            void EmpowerCultist()
            {
                if (summons.empty())
                    return;

                std::list<Creature*> temp;
                for (SummonList::iterator itr = summons.begin(); itr != summons.end(); ++itr)
                    if (Creature* cre = ObjectAccessor::GetCreature(*me, *itr))
                        if (cre->IsAlive() && (cre->GetEntry() == NPC_CULT_FANATIC || cre->GetEntry() == NPC_CULT_ADHERENT) && cre->CanFreeMoveNoRoot())
                            temp.push_back(cre);

                // noone to empower
                if (temp.empty())
                    return;

                // select random cultist
                Creature* cultist = Trinity::Containers::SelectRandomContainerElement(temp);
                DoCast(cultist, cultist->GetEntry() == NPC_CULT_FANATIC ? SPELL_DARK_TRANSFORMATION_T : SPELL_DARK_EMPOWERMENT_T);
                Talk(uint8(cultist->GetEntry() == NPC_CULT_FANATIC ? SAY_DARK_TRANSFORMATION : SAY_DARK_EMPOWERMENT));
            }

        private:
            ObjectGuid _darnavanGUID;
            std::deque<ObjectGuid> _reanimationQueue;
            GuidList _nextVengefulShadeTargetGUID;
            uint32 _waveCounter;
            bool _introDone;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<boss_lady_deathwhisperAI>(creature);
        }
};

typedef boss_lady_deathwhisper::boss_lady_deathwhisperAI DeathwisperAI;

class npc_cult_fanatic : public CreatureScript
{
    public:
        npc_cult_fanatic() : CreatureScript("npc_cult_fanatic") { }

        struct npc_cult_fanaticAI : public ScriptedAI
        {
            npc_cult_fanaticAI(Creature* creature) : ScriptedAI(creature) {}

            void Reset()
            {
                Events.Reset();
                Events.ScheduleEvent(EVENT_FANATIC_NECROTIC_STRIKE, 10s, 12s);
                Events.ScheduleEvent(EVENT_FANATIC_SHADOW_CLEAVE, 14s, 16s);
                Events.ScheduleEvent(EVENT_FANATIC_VAMPIRIC_MIGHT, 20s, 27s);
                if (me->GetEntry() == NPC_CULT_FANATIC)
                    Events.ScheduleEvent(EVENT_CULTIST_DARK_MARTYRDOM, 18s, 32s);
                me->SetInCombatWithZone();
            }

            void SpellHit(Unit* /*caster*/, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_DARK_TRANSFORMATION)
                    me->UpdateEntry(NPC_DEFORMED_FANATIC);
                else if (spell->Id == SPELL_DARK_TRANSFORMATION_T)
                {
                    Events.CancelEvent(EVENT_CULTIST_DARK_MARTYRDOM);
                    me->InterruptNonMeleeSpells(true);
                    DoCastSelf(SPELL_DARK_TRANSFORMATION);
                    DoCastSelf(SPELL_FANATIC_S_DETERMINATION);
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                Events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = Events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_FANATIC_NECROTIC_STRIKE:
                            DoCastVictim(SPELL_NECROTIC_STRIKE);
                            Events.Repeat(11s, 13s);
                            break;
                        case EVENT_FANATIC_SHADOW_CLEAVE:
                            DoCastVictim(SPELL_SHADOW_CLEAVE);
                            Events.Repeat(10s, 11s);
                            break;
                        case EVENT_FANATIC_VAMPIRIC_MIGHT:
                            DoCastSelf(SPELL_VAMPIRIC_MIGHT);
                            Events.Repeat(20s, 27s);
                            break;
                        case EVENT_CULTIST_DARK_MARTYRDOM:
                            DoCastSelf(SPELL_DARK_MARTYRDOM_FANATIC);
                            Events.Repeat(16s, 21s);
                            break;
                    }
                }

                DoMeleeAttackIfReady(true);
            }

        protected:
            EventMap Events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_cult_fanaticAI>(creature);
        }
};

class npc_cult_adherent : public CreatureScript
{
    public:
        npc_cult_adherent() : CreatureScript("npc_cult_adherent") { }

        struct npc_cult_adherentAI : public ScriptedAI
        {
            npc_cult_adherentAI(Creature* creature) : ScriptedAI(creature) {}

            void Reset()
            {
                Events.Reset();
                Events.ScheduleEvent(EVENT_ADHERENT_FROST_FEVER, 10s, 12s);
                Events.ScheduleEvent(EVENT_ADHERENT_DEATHCHILL, 14s, 16s);
                Events.ScheduleEvent(EVENT_ADHERENT_CURSE_OF_TORPOR, 14s, 16s);
                Events.ScheduleEvent(EVENT_ADHERENT_SHORUD_OF_THE_OCCULT, 32s, 39s);
                if (me->GetEntry() == NPC_CULT_ADHERENT)
                    Events.ScheduleEvent(EVENT_CULTIST_DARK_MARTYRDOM, 18s, 32s);
                me->SetInCombatWithZone();
            }

            void SpellHit(Unit* /*caster*/, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_DARK_EMPOWERMENT)
                {
                    me->UpdateEntry(NPC_EMPOWERED_ADHERENT);
                    me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);
                }
                else if (spell->Id == SPELL_DARK_EMPOWERMENT_T)
                {
                    Events.CancelEvent(EVENT_CULTIST_DARK_MARTYRDOM);
                    me->InterruptNonMeleeSpells(true);
                    DoCastSelf(SPELL_DARK_EMPOWERMENT);
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                Events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = Events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ADHERENT_FROST_FEVER:
                            DoCastVictim(SPELL_FROST_FEVER);
                            Events.Repeat(9s, 13s);
                            break;
                        case EVENT_ADHERENT_DEATHCHILL:
                            if (me->GetEntry() == NPC_EMPOWERED_ADHERENT)
                                DoCastVictim(SPELL_DEATHCHILL_BLAST);
                            else
                                DoCastVictim(SPELL_DEATHCHILL_BOLT);
                            Events.Repeat(2s, 3s);
                            break;
                        case EVENT_ADHERENT_CURSE_OF_TORPOR:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_CURSE_OF_TORPOR);
                            Events.Repeat(9s, 13s);
                            break;
                        case EVENT_ADHERENT_SHORUD_OF_THE_OCCULT:
                            DoCastSelf(SPELL_SHORUD_OF_THE_OCCULT);
                            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);
                            Events.Repeat(27s, 32s);
                            break;
                        case EVENT_CULTIST_DARK_MARTYRDOM:
                            DoCastSelf(SPELL_DARK_MARTYRDOM_ADHERENT);
                            Events.Repeat(16s, 21s);
                            break;
                    }
                }

                DoMeleeAttackIfReady(true);
            }

        protected:
            EventMap Events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_cult_adherentAI>(creature);
        }
};

struct npc_vengeful_shadeAI : public ScriptedAI
{
    npc_vengeful_shadeAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        me->SetReactState(REACT_PASSIVE);
        DoCastSelf(SPELL_VENGEFUL_BLAST_PASSIVE);

        _events.ScheduleEvent(EVENT_VENGEFUL_SHADE_ATTACK_START, 1s, 2s);
    }

    void SpellHitTarget(Unit* /*target*/, SpellInfo const* spell) override
    {
        switch (spell->Id)
        {
            case SPELL_VENGEFUL_BLAST:
            case SPELL_VENGEFUL_BLAST_25N:
            case SPELL_VENGEFUL_BLAST_10H:
            case SPELL_VENGEFUL_BLAST_25H:
                me->KillSelf();
                break;
            default:
                break;
        }
    }

    void SetGuidData(ObjectGuid guid, uint32 /*data*/) override
    {
        _targetGUID = guid;
    }

    bool CanAIAttack(Unit const* target) const override
    {
        return target->GetTypeId() == TYPEID_PLAYER;
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        DoZoneInCombat();
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_VENGEFUL_SHADE_ATTACK_START:
                    if (Unit* target = ObjectAccessor::GetUnit(*me, _targetGUID))
                    {
                        bool charmed = target->IsCharmed();
                        if (!me->CanStartAttack(target, false) && !charmed)
                            return;

                        if (charmed)
                        {
                            me->SetReactState(REACT_AGGRESSIVE);
                            DoZoneInCombat();
                        }
                        else
                        {
                            me->SetReactState(REACT_AGGRESSIVE);
                            me->AI()->AttackStart(target);
                            me->GetThreatManager().AddThreat(target, 500000.0f);
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        if (!UpdateVictim())
            return;

        DoMeleeAttackIfReady(true);
    }

private:
    EventMap _events;
    ObjectGuid _targetGUID;
};

class npc_darnavan : public CreatureScript
{
    public:
        npc_darnavan() : CreatureScript("npc_darnavan") { }

        struct npc_darnavanAI : public ScriptedAI
        {
            npc_darnavanAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            void Reset()
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_DARNAVAN_BLADESTORM, 10s);
                _events.ScheduleEvent(EVENT_DARNAVAN_INTIMIDATING_SHOUT, 20s, 25s);
                _events.ScheduleEvent(EVENT_DARNAVAN_MORTAL_STRIKE, 25s, 30s);
                _events.ScheduleEvent(EVENT_DARNAVAN_SUNDER_ARMOR, 5s, 8s);
                _canCharge = true;
                _canShatter = true;
                me->SetInCombatWithZone();
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, true);
            }

            void JustDied(Unit* killer)
            {
                _events.Reset();
                if (Player* owner = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
                {
                    if (Group* group = owner->GetGroup())
                    {
                        for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
                            if (Player* member = itr->GetSource())
                                if (member->GetQuestStatus(QUEST_DEPROGRAMMING) == QUEST_STATUS_INCOMPLETE)
                                    member->FailQuest(QUEST_DEPROGRAMMING);
                    }
                    else
                    {
                        if (owner->GetQuestStatus(QUEST_DEPROGRAMMING) == QUEST_STATUS_INCOMPLETE)
                            owner->FailQuest(QUEST_DEPROGRAMMING);
                    }
                }
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (type != POINT_MOTION_TYPE || id != POINT_DESPAWN)
                    return;

                me->DespawnOrUnsummon();
            }

            void EnterCombat(Unit* /*victim*/)
            {
                Talk(SAY_DARNAVAN_AGGRO);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (_canShatter && me->GetVictim() && me->GetVictim()->IsImmunedToDamage(SPELL_SCHOOL_MASK_NORMAL))
                {
                    DoCastVictim(SPELL_SHATTERING_THROW);
                    _canShatter = false;
                    _events.ScheduleEvent(EVENT_DARNAVAN_SHATTERING_THROW, 30s);
                    return;
                }

                if (_canCharge && !me->IsWithinMeleeRange(me->GetVictim()))
                {
                    DoCastVictim(SPELL_CHARGE);
                    _canCharge = false;
                    _events.ScheduleEvent(EVENT_DARNAVAN_CHARGE, 20s);
                    return;
                }

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_DARNAVAN_BLADESTORM:
                            DoCast(SPELL_BLADESTORM);
                            _events.Repeat(90s, 100s);
                            break;
                        case EVENT_DARNAVAN_CHARGE:
                            _canCharge = true;
                            break;
                        case EVENT_DARNAVAN_INTIMIDATING_SHOUT:
                            DoCast(SPELL_INTIMIDATING_SHOUT);
                            _events.Repeat(90s, 120s);
                            break;
                        case EVENT_DARNAVAN_MORTAL_STRIKE:
                            DoCastVictim(SPELL_MORTAL_STRIKE);
                            _events.Repeat(15s, 30s);
                            break;
                        case EVENT_DARNAVAN_SHATTERING_THROW:
                            _canShatter = true;
                            break;
                        case EVENT_DARNAVAN_SUNDER_ARMOR:
                            DoCastVictim(SPELL_SUNDER_ARMOR);
                            _events.Repeat(3s, 7s);
                            break;
                    }
                }

                DoMeleeAttackIfReady(true);
            }

        private:
            EventMap _events;
            bool _canCharge;
            bool _canShatter;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_darnavanAI>(creature);
        }
};

class spell_deathwhisper_mana_barrier : public SpellScriptLoader
{
    public:
        spell_deathwhisper_mana_barrier() : SpellScriptLoader("spell_deathwhisper_mana_barrier") { }

        class spell_deathwhisper_mana_barrier_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_deathwhisper_mana_barrier_AuraScript);

            void HandlePeriodicTick(AuraEffect const* /*aurEff*/)
            {
                PreventDefaultAction();
                if (Unit* caster = GetCaster())
                {
                    int32 missingHealth = int32(caster->GetMaxHealth() - caster->GetHealth());
                    caster->ModifyHealth(missingHealth);
                    caster->ModifyPower(POWER_MANA, -missingHealth);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_deathwhisper_mana_barrier_AuraScript::HandlePeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_deathwhisper_mana_barrier_AuraScript();
        }
};

class spell_cultist_dark_martyrdom : public SpellScriptLoader
{
    public:
        spell_cultist_dark_martyrdom() : SpellScriptLoader("spell_cultist_dark_martyrdom") { }

        class spell_cultist_dark_martyrdom_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_cultist_dark_martyrdom_SpellScript);

            void HandleEffect(SpellEffIndex /*effIndex*/)
            {
                if (GetCaster()->IsSummon())
                    if (Unit* owner = GetCaster()->ToTempSummon()->GetSummoner())
                        owner->GetAI()->SetGuidData(GetCaster()->GetGUID(), GUID_CULTIST);

                GetCaster()->Kill(GetCaster());
                GetCaster()->SetDisplayId(uint32(GetCaster()->GetEntry() == NPC_CULT_FANATIC ? 38009 : 38010));
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_cultist_dark_martyrdom_SpellScript::HandleEffect, EFFECT_2, SPELL_EFFECT_FORCE_DESELECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_cultist_dark_martyrdom_SpellScript();
        }
};

class spell_deathwhisper_vampiric_might : public SpellScriptLoader
{
    public:
        spell_deathwhisper_vampiric_might() : SpellScriptLoader("spell_deathwhisper_vampiric_might") { }

        class spell_deathwhisper_vampiric_might_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_deathwhisper_vampiric_might_AuraScript);

            void HandleEffectProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                uint32 damage = eventInfo.GetDamageInfo()->GetDamage();
                ApplyPct(damage, aurEff->GetAmount());
                GetUnitOwner()->CastCustomSpell(SPELL_VAMPIRIC_MIGHT_HEAL, SPELLVALUE_BASE_POINT0, damage, GetUnitOwner(), TRIGGERED_FULL_MASK);

                PreventDefaultAction();
            }

            void Register()
            {
                OnEffectProc += AuraEffectProcFn(spell_deathwhisper_vampiric_might_AuraScript::HandleEffectProc, EFFECT_1, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_deathwhisper_vampiric_might_AuraScript();
        }
};

class spell_dominate_mind_icc : public SpellScriptLoader
{
public:
    spell_dominate_mind_icc() : SpellScriptLoader("spell_dominate_mind_icc") { }

    class spell_dominate_mind_icc_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_dominate_mind_icc_AuraScript);

        void HandleOnEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            target->SetObjectScale(3.0f);
        }

        void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            target->SetObjectScale(1.0f);
            // set back in combat with Lady deathwhisper
            if (Creature* lady = target->FindNearestCreature(NPC_LADY_DEATHWHISPER, 500.0f))
            {
                target->EngageWithTarget(lady);
            }
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_dominate_mind_icc_AuraScript::HandleOnEffectApply, EFFECT_0, SPELL_AURA_AOE_CHARM, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(spell_dominate_mind_icc_AuraScript::HandleOnEffectRemove, EFFECT_0, SPELL_AURA_AOE_CHARM, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_dominate_mind_icc_AuraScript();
    }
};

class spell_deathwhisper_summon_shade_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_deathwhisper_summon_shade_SpellScript);

    void HandleEffect(SpellEffIndex effIndex)
    {
        if (Creature* creature = GetCaster()->To<Creature>())
            creature->AI()->SetGuidData(GetExplTargetUnit()->GetGUID(), SPELL_SUMMON_SHADE);
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_deathwhisper_summon_shade_SpellScript::HandleEffect, EFFECT_1, SPELL_EFFECT_TRIGGER_MISSILE);
    }
};

void AddSC_boss_lady_deathwhisper()
{
    new boss_lady_deathwhisper();
    new npc_cult_fanatic();
    new npc_cult_adherent();
    new CreatureScriptLoaderEx<npc_vengeful_shadeAI>("npc_vengeful_shade");
    new npc_darnavan();
    new spell_deathwhisper_mana_barrier();
    new spell_cultist_dark_martyrdom();
    new spell_deathwhisper_vampiric_might();
    new spell_dominate_mind_icc();
    new SpellScriptLoaderEx<spell_deathwhisper_summon_shade_SpellScript>("spell_deathwhisper_summon_shade");
}
