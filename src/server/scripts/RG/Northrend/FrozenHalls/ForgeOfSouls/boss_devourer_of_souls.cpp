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
#include "forge_of_souls.h"
#include "Player.h"
#include "SpellInfo.h"

/*
 * @todo
 * - Fix model id during unleash soul -> seems DB issue 36503 is missing (likewise 36504 is also missing).
 */

enum Yells
{
    SAY_FACE_AGGRO                              = 0,
    SAY_FACE_ANGER_SLAY                         = 1,
    SAY_FACE_SORROW_SLAY                        = 2,
    SAY_FACE_DESIRE_SLAY                        = 3,
    SAY_FACE_DEATH                              = 4,
    EMOTE_MIRRORED_SOUL                         = 5,
    EMOTE_UNLEASH_SOUL                          = 6,
    SAY_FACE_UNLEASH_SOUL                       = 7,
    EMOTE_WAILING_SOUL                          = 8,
    SAY_FACE_WAILING_SOUL                       = 9,

    SAY_JAINA_OUTRO                             = 0,
    SAY_SYLVANAS_OUTRO                          = 0
};

enum Spells
{
    SPELL_PHANTOM_BLAST                         = 68982,
    H_SPELL_PHANTOM_BLAST                       = 70322,
    SPELL_MIRRORED_SOUL                         = 69051,
    SPELL_WELL_OF_SOULS                         = 68820,
    SPELL_UNLEASHED_SOULS                       = 68939,
    SPELL_WAILING_SOULS_STARTING                = 68899,  // Initial spell cast at begining of wailing souls phase
    SPELL_WAILING_SOULS_BEAM                    = 68875,  // the beam visual
    SPELL_WAILING_SOULS                         = 68873,  // the actual spell
    H_SPELL_WAILING_SOULS                       = 70324,
    SPELL_MIRRORED_COSTUM                       = 69034
//    68871, 68873, 68875, 68876, 68899, 68912, 70324,
// 68899 trigger 68871
};

enum Events
{
    EVENT_PHANTOM_BLAST          = 1,
    EVENT_MIRRORED_SOUL          = 2,
    EVENT_WELL_OF_SOULS          = 3,
    EVENT_UNLEASHED_SOULS        = 4,
    EVENT_WAILING_SOULS          = 5,
    EVENT_WAILING_SOULS_TICK     = 6,
    EVENT_FACE_ANGER             = 7,
    EVENT_WAILING_SOULS_START    = 8,
    EVENT_UNLEASHED_SOULS_SUMMON = 9
};

enum eEnum
{
    DISPLAY_ANGER               = 30148,
    DISPLAY_SORROW              = 30149,
    DISPLAY_DESIRE              = 30150,
};

struct outroPosition
{
    uint32 entry[2];
    Position movePosition;
} outroPositions[] = 
{
    { { NPC_JAINA_PART2, NPC_SYLVANAS_PART2 }, { 5655.031738f, 2499.035889f, 708.829102f, 2.901970f } },

    { { 0, 0 }, { 0.0f, 0.0f, 0.0f, 0.0f } } 
};

Position const CrucibleSummonPos = {5672.294f, 2520.686f, 713.4386f, 0.9599311f};

#define DATA_THREE_FACED        1

class boss_devourer_of_souls : public CreatureScript
{
    public:
        boss_devourer_of_souls() : CreatureScript("boss_devourer_of_souls") { }

        struct boss_devourer_of_soulsAI : public BossAI
        {
            boss_devourer_of_soulsAI(Creature* creature) : BossAI(creature, DATA_DEVOURER_EVENT)
            {
            }

            void InitializeAI()
            {
                if (!instance || static_cast<InstanceMap*>(me->GetMap())->GetScriptId() != sObjectMgr->GetScriptId(FoSScriptName))
                    me->IsAIEnabled = false;
                else if (!me->isDead())
                    Reset();
            }

            void Reset()
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);
                me->SetDisplayId(DISPLAY_ANGER);
                me->SetReactState(REACT_AGGRESSIVE);
                me->RemoveAura(SPELL_WAILING_SOULS_STARTING);

                events.Reset();
                summons.DespawnAll();

                threeFaced = true;
                AllowMelee = false;
                mirroredSoulTarget.Clear();

                instance->SetData(DATA_DEVOURER_EVENT, NOT_STARTED);
            }

            void EnterCombat(Unit* /*who*/)
            {
                Talk(SAY_FACE_AGGRO);

                if (!me->FindNearestCreature(NPC_CRUCIBLE_OF_SOULS, 60)) // Prevent double spawn
                    instance->instance->SummonCreature(NPC_CRUCIBLE_OF_SOULS, CrucibleSummonPos);
                events.ScheduleEvent(EVENT_PHANTOM_BLAST,             5*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_MIRRORED_SOUL,             8*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_WELL_OF_SOULS,            30*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_UNLEASHED_SOULS,          20*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_WAILING_SOULS, urand(60, 70)*IN_MILLISECONDS);

                instance->SetData(DATA_DEVOURER_EVENT, IN_PROGRESS);
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true))
                {
                    AddThreat(target, 500000.0f, summon);
                    summon->AI()->AttackStart(target);
                }
            }

            void DamageTaken(Unit* /*pDoneBy*/, uint32 &uiDamage)
            {
                if (mirroredSoulTarget && me->HasAura(SPELL_MIRRORED_SOUL))
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, mirroredSoulTarget))
                    {
                        if (player->GetAura(SPELL_MIRRORED_SOUL))
                        {
                            int32 mirrorDamage = (uiDamage* 45)/100;
                            me->CastCustomSpell(player, SPELL_MIRRORED_COSTUM, &mirrorDamage, 0, 0, true);
                        }
                        else
                            mirroredSoulTarget.Clear();
                    }
                }
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() != TYPEID_PLAYER)
                    return;

                int32 textId = 0;
                switch (me->GetDisplayId())
                {
                    case DISPLAY_ANGER:
                        textId = SAY_FACE_ANGER_SLAY;
                        break;
                    case DISPLAY_SORROW:
                        textId = SAY_FACE_SORROW_SLAY;
                        break;
                    case DISPLAY_DESIRE:
                        textId = SAY_FACE_DESIRE_SLAY;
                        break;
                    default:
                        break;
                }

                if (textId)
                    Talk(textId);
            }

            void JustDied(Unit* /*killer*/)
            {
                summons.DespawnAll();

                Position spawnPoint = {5618.139f, 2451.873f, 705.854f, 0};

                Talk(SAY_FACE_DEATH);

                instance->SetData(DATA_DEVOURER_EVENT, DONE);

                int32 entryIndex;
                if (instance->GetData(DATA_TEAM_IN_INSTANCE) == ALLIANCE)
                    entryIndex = 0;
                else
                    entryIndex = 1; 

                for (int8 i = 0; outroPositions[i].entry[entryIndex] != 0; ++i)
                {
                    if (Creature* summon = me->SummonCreature(outroPositions[i].entry[entryIndex], spawnPoint, TEMPSUMMON_DEAD_DESPAWN))
                    {
                        summon->GetMotionMaster()->MovePoint(0, outroPositions[i].movePosition);
                        if (summon->GetEntry() == NPC_JAINA_PART2)
                            summon->AI()->Talk(SAY_JAINA_OUTRO);
                        else if (summon->GetEntry() == NPC_SYLVANAS_PART2)
                            summon->AI()->Talk(SAY_SYLVANAS_OUTRO);
                    }
                }

                std::list<Creature*> ChampionList;
                me->GetCreatureListWithEntryInGrid(ChampionList, NPC_CHAMPION_1_HORDE,    300.0f);
                me->GetCreatureListWithEntryInGrid(ChampionList, NPC_CHAMPION_2_HORDE,    300.0f);
                me->GetCreatureListWithEntryInGrid(ChampionList, NPC_CHAMPION_3_HORDE,    300.0f);
                me->GetCreatureListWithEntryInGrid(ChampionList, NPC_CHAMPION_1_ALLIANCE, 300.0f);
                me->GetCreatureListWithEntryInGrid(ChampionList, NPC_CHAMPION_2_ALLIANCE, 300.0f);
                me->GetCreatureListWithEntryInGrid(ChampionList, NPC_CHAMPION_3_ALLIANCE, 300.0f);
                // main guards
                me->GetCreatureListWithEntryInGrid(ChampionList, NPC_KALIRA,              300.0f);
                me->GetCreatureListWithEntryInGrid(ChampionList, NPC_ELANDRA,             300.0f);
                me->GetCreatureListWithEntryInGrid(ChampionList, NPC_LORALEN,             300.0f);
                me->GetCreatureListWithEntryInGrid(ChampionList, NPC_KORELN,              300.0f);
                if (!ChampionList.empty())
                    for (std::list<Creature*>::iterator itr = ChampionList.begin(); itr != ChampionList.end(); itr++)
                    { 
                        (*itr)->AI()->SetData(6, 6);
                        (*itr)->SetVisible(true);
                    }                       
            }

            void SpellHitTarget(Unit* /*target*/, const SpellInfo* spell)
            {
                if (spell->Id == H_SPELL_PHANTOM_BLAST)
                    threeFaced = false;
            }

            uint32 GetData(uint32 type) const
            {
                if (type == DATA_THREE_FACED)
                    return threeFaced;

                return 0;
            }

            void UpdateAI(uint32 diff)
            {
                // Return since we have no target
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_PHANTOM_BLAST:
                            me->RemoveAura(SPELL_WAILING_SOULS_STARTING);
                            DoCastVictim(SPELL_PHANTOM_BLAST);
                            events.ScheduleEvent(EVENT_PHANTOM_BLAST, 5*IN_MILLISECONDS);
                            break;
                        case EVENT_MIRRORED_SOUL:
                            me->RemoveAura(SPELL_WAILING_SOULS_STARTING);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true))
                            {
                                mirroredSoulTarget = target->GetGUID();
                                DoCast(target, SPELL_MIRRORED_SOUL);
                                Talk(EMOTE_MIRRORED_SOUL);
                            }
                            events.ScheduleEvent(EVENT_MIRRORED_SOUL, urand(15, 30)*IN_MILLISECONDS);
                            break;
                        case EVENT_WELL_OF_SOULS:
                            me->RemoveAura(SPELL_WAILING_SOULS_STARTING);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                DoCast(target, SPELL_WELL_OF_SOULS);
                            events.ScheduleEvent(EVENT_WELL_OF_SOULS, 20*IN_MILLISECONDS);
                            break;
                        case EVENT_UNLEASHED_SOULS:
                            me->RemoveAura(SPELL_WAILING_SOULS_STARTING);                            
                            me->SetDisplayId(DISPLAY_SORROW);
                            Talk(SAY_FACE_UNLEASH_SOUL);
                            Talk(EMOTE_UNLEASH_SOUL);
                            events.ScheduleEvent(EVENT_UNLEASHED_SOULS, 30*IN_MILLISECONDS);
                            events.ScheduleEvent(EVENT_FACE_ANGER, 5*IN_MILLISECONDS);
                            events.ScheduleEvent(EVENT_UNLEASHED_SOULS_SUMMON, 4*IN_MILLISECONDS);
                            break;
                        case EVENT_UNLEASHED_SOULS_SUMMON:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                DoCast(target, SPELL_UNLEASHED_SOULS);
                            break;
                        case EVENT_FACE_ANGER:
                            me->RemoveAura(SPELL_WAILING_SOULS_STARTING);
                            me->SetDisplayId(DISPLAY_ANGER);
                            break;
                        case EVENT_WAILING_SOULS:
                            Talk(SAY_FACE_WAILING_SOUL);
                            Talk(EMOTE_WAILING_SOUL);
                            DoCastSelf(SPELL_WAILING_SOULS_STARTING);                           
                            events.ScheduleEvent(EVENT_WAILING_SOULS_START, 3*IN_MILLISECONDS);
                            break;
                        case EVENT_WAILING_SOULS_START:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            {
                                me->SetOrientation(me->GetAngle(target));
                                me->SendMovementFlagUpdate();
                                DoCastSelf(SPELL_WAILING_SOULS_BEAM);
                            }

                            beamAngle = me->GetOrientation();

                            beamAngleDiff = M_PI_F / 30.0f; // PI/2 in 15 sec = PI/30 per tick
                            if (RAND(true, false))
                                beamAngleDiff = -beamAngleDiff;

                            me->InterruptNonMeleeSpells(false);
                            me->SetReactState(REACT_PASSIVE);
                            AllowMelee = true;

                            //Remove any target
                            me->SetTarget(ObjectGuid::Empty);

                            me->GetMotionMaster()->Clear();
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);

                            wailingSoulTick = 15;
                            events.DelayEvents(18000); // no other events during wailing souls
                            events.ScheduleEvent(EVENT_WAILING_SOULS_TICK, 3*IN_MILLISECONDS); // first one after 3 secs.
                            break;
                        case EVENT_WAILING_SOULS_TICK:
                            beamAngle += beamAngleDiff;
                            me->SetOrientation(beamAngle);
                            me->SendMovementFlagUpdate();
                            me->StopMoving();

                            DoCastSelf(SPELL_WAILING_SOULS);

                            if (--wailingSoulTick)
                                events.ScheduleEvent(EVENT_WAILING_SOULS_TICK, 1*IN_MILLISECONDS);
                            else
                            {
                                AllowMelee = false;
                                me->SetReactState(REACT_AGGRESSIVE);
                                me->SetDisplayId(DISPLAY_ANGER);
                                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);
                                me->GetMotionMaster()->MoveChase(me->GetVictim());
                                events.ScheduleEvent(EVENT_WAILING_SOULS, urand(60, 70)*IN_MILLISECONDS);
                            }
                            break;
                    }
                }

                if (!AllowMelee)
                    DoMeleeAttackIfReady();
            }

        private:
            bool threeFaced;
            bool AllowMelee;

            // wailing soul event
            float beamAngle;
            float beamAngleDiff;
            int8 wailingSoulTick;

            ObjectGuid mirroredSoulTarget;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_devourer_of_soulsAI(creature);
        }
};

class achievement_three_faced : public AchievementCriteriaScript
{
    public:
        achievement_three_faced() : AchievementCriteriaScript("achievement_three_faced")
        {
        }

        bool OnCheck(Player* /*player*/, Unit* target)
        {
            if (!target)
                return false;

            if (Creature* Devourer = target->ToCreature())
                if (Devourer->AI()->GetData(DATA_THREE_FACED))
                    return true;

            return false;
        }
};

void AddSC_boss_devourer_of_souls()
{
    new boss_devourer_of_souls();
    new achievement_three_faced();
}
