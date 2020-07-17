/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Boss_Hydross_The_Unstable
SD%Complete: 95
SDComment: Some details and adjustments left to do, probably nothing major. Spawns may be spawned in different way/location.
SDCategory: Coilfang Resevoir, Serpent Shrine Cavern
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Pet.h"
#include "serpent_shrine.h"

enum HydrossTheUnstable
{
    SAY_AGGRO                   = 0,
    SAY_SWITCH_TO_CLEAN         = 1,
    SAY_CLEAN_SLAY              = 2,
    SAY_CLEAN_DEATH             = 3,
    SAY_SWITCH_TO_CORRUPT       = 4,
    SAY_CORRUPT_SLAY            = 5,
    SAY_CORRUPT_DEATH           = 6,

    SWITCH_RADIUS               = 18,

    MODEL_CORRUPT               = 20609,
    MODEL_CLEAN                 = 20162,

    SPELL_WATER_TOMB            = 38235,
    SPELL_MARK_OF_HYDROSS1      = 38215,
    SPELL_MARK_OF_HYDROSS2      = 38216,
    SPELL_MARK_OF_HYDROSS3      = 38217,
    SPELL_MARK_OF_HYDROSS4      = 38218,
    SPELL_MARK_OF_HYDROSS5      = 38231,
    SPELL_MARK_OF_HYDROSS6      = 40584,
    SPELL_MARK_OF_CORRUPTION1   = 38219,
    SPELL_MARK_OF_CORRUPTION2   = 38220,
    SPELL_MARK_OF_CORRUPTION3   = 38221,
    SPELL_MARK_OF_CORRUPTION4   = 38222,
    SPELL_MARK_OF_CORRUPTION5   = 38230,
    SPELL_MARK_OF_CORRUPTION6   = 40583,
    SPELL_VILE_SLUDGE           = 38246,
    SPELL_ENRAGE                = 27680,                   //this spell need verification
    SPELL_SUMMON_WATER_ELEMENT  = 36459,                   //not in use yet(in use ever?)
    SPELL_ELEMENTAL_SPAWNIN     = 25035,
    SPELL_BLUE_BEAM             = 40227,                   //channeled Hydross Beam Helper (not in use yet)

    ENTRY_PURE_SPAWN            = 22035,
    ENTRY_TAINTED_SPAWN         = 22036,
    ENTRY_BEAM_DUMMY            = 21934
};

enum Events 
{
	EVENT_NONE                  = 0,
	EVENT_CORRUPTION_MARK		= 1,
	EVENT_HYDROSS_MARK			= 2,
	EVENT_WATER_TOMB			= 3,
	EVENT_VILE_SLUDGE			= 4,
	EVENT_POS_CHECK_CORRUPT     = 5,
	EVENT_POS_CHECK_CLEAN       = 6,
	EVENT_ENRAGE                = 7,
};


#define HYDROSS_X                   -239.439f
#define HYDROSS_Y                   -363.481f

const uint32 MarkOfCorruptionRanks[6] = 
{
    SPELL_MARK_OF_CORRUPTION1,
    SPELL_MARK_OF_CORRUPTION2,
    SPELL_MARK_OF_CORRUPTION3,
    SPELL_MARK_OF_CORRUPTION4,
    SPELL_MARK_OF_CORRUPTION5,
    SPELL_MARK_OF_CORRUPTION6
};

const uint32 MarkOfHydrossRanks[6] = 
{
    SPELL_MARK_OF_HYDROSS1,
    SPELL_MARK_OF_HYDROSS2,
    SPELL_MARK_OF_HYDROSS3,
    SPELL_MARK_OF_HYDROSS4,
    SPELL_MARK_OF_HYDROSS5,
    SPELL_MARK_OF_HYDROSS6
};

const Position SpawnPositions[4] = 
{
	{ 6.934003f, -11.255012f, 0.0f, },
	{ -6.934003f, 11.255012f, 0.0f, },
	{ -12.577011f, -4.72702f, 0.0f, },
	{ 12.577011f,  4.72702f, 0.0f},
};

class boss_hydross_the_unstable : public CreatureScript
{
public:
    boss_hydross_the_unstable() : CreatureScript("boss_hydross_the_unstable") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetInstanceAI<boss_hydross_the_unstableAI>(creature);
    }

    struct boss_hydross_the_unstableAI : public ScriptedAI
    {
        boss_hydross_the_unstableAI(Creature* creature) : ScriptedAI(creature), Summons(me)
        {
            instance = creature->GetInstanceScript();
            beams[0].Clear();
            beams[1].Clear();
        }

        InstanceScript* instance;

        ObjectGuid beams[2];
        uint32 MarkOfHydross_Count;
        uint32 MarkOfCorruption_Count;
        bool CorruptedForm;
        bool beam;
        SummonList Summons;

        void Reset() override
        {
            DeSummonBeams();
            beams[0].Clear();
            beams[1].Clear();
            MarkOfHydross_Count = 0;
            MarkOfCorruption_Count = 0;

            CorruptedForm = false;
            me->SetMeleeDamageSchool(SPELL_SCHOOL_FROST);
            me->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, true);
            me->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_NATURE, false);

            me->SetDisplayId(MODEL_CLEAN);

            instance->SetData(DATA_HYDROSSTHEUNSTABLEEVENT, NOT_STARTED);

            beam = false;
            Summons.DespawnAll();

			events.Reset();
        }

        void SummonBeams()
        {
            Creature* beamer = me->SummonCreature(ENTRY_BEAM_DUMMY, -258.333f, -356.34f, 22.0499f, 5.90835f, TEMPSUMMON_CORPSE_DESPAWN, 0);
            if (beamer)
            {
                beamer->CastSpell(me, SPELL_BLUE_BEAM, true);
                beamer->SetDisplayId(11686);  //invisible
                beamer->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                beams[0]=beamer->GetGUID();
            }
            beamer = me->SummonCreature(ENTRY_BEAM_DUMMY, -219.918f, -371.308f, 22.0042f, 2.73072f, TEMPSUMMON_CORPSE_DESPAWN, 0);
            if (beamer)
            {
                beamer->CastSpell(me, SPELL_BLUE_BEAM, true);
                beamer->SetDisplayId(11686);  //invisible
                beamer->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                beams[1]=beamer->GetGUID();
            }
        }
        void DeSummonBeams()
        {
            for (uint8 i = 0; i < 2; ++i)
            {
                if (Creature* mob = ObjectAccessor::GetCreature(*me, beams[i]))
                {
                    mob->setDeathState(DEAD);
                    mob->RemoveCorpse();
                }
            }
        }
        void EnterCombat(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);

            instance->SetData(DATA_HYDROSSTHEUNSTABLEEVENT, IN_PROGRESS);

			events.ScheduleEvent(EVENT_HYDROSS_MARK, 15000);
			events.ScheduleEvent(EVENT_WATER_TOMB, 7000);
			events.ScheduleEvent(EVENT_POS_CHECK_CLEAN, 2500);
			events.ScheduleEvent(EVENT_ENRAGE, 600000);
        }

        void KilledUnit(Unit* /*victim*/) override
        {
            Talk(CorruptedForm ? SAY_CORRUPT_SLAY : SAY_CLEAN_SLAY);
        }

        void JustSummoned(Creature* summoned) override
        {
            if (summoned->GetEntry() == ENTRY_PURE_SPAWN)
            {
                summoned->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, true);
                summoned->CastSpell(summoned, SPELL_ELEMENTAL_SPAWNIN, true);
                Summons.Summon(summoned);
            }
            if (summoned->GetEntry() == ENTRY_TAINTED_SPAWN)
            {
                summoned->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_NATURE, true);
                summoned->CastSpell(summoned, SPELL_ELEMENTAL_SPAWNIN, true);
                Summons.Summon(summoned);
            }
        }

        void SummonedCreatureDespawn(Creature* summon) override
        {
            Summons.Despawn(summon);
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(CorruptedForm ? SAY_CORRUPT_DEATH : SAY_CLEAN_DEATH);

            instance->SetData(DATA_HYDROSSTHEUNSTABLEEVENT, DONE);

            Summons.DespawnAll();
        }

        void UpdateAI(uint32 diff) override
        {
            if (!beam)
            {
                SummonBeams();
                beam=true;
            }
            //Return since we have no target
            if (!UpdateVictim())
                return;

			events.Update(diff);

			if(CorruptedForm)
			{
				while(uint32 eventId = events.ExecuteEvent())
				{
					switch(eventId)
					{
						case EVENT_CORRUPTION_MARK:
							if (MarkOfCorruption_Count <= 5)
							{
								//remove last mark spell
								if(MarkOfCorruption_Count > 0)
								{
									uint32 old_mark_spell = MarkOfCorruptionRanks[MarkOfCorruption_Count - 1];
                                    for (Player& player : me->GetMap()->GetPlayers())
									{
										player.RemoveAura(old_mark_spell);
										if (Pet* pet = player.GetPet())
											pet->RemoveAura(old_mark_spell);
									}
								}

								DoCastVictim(MarkOfCorruptionRanks[MarkOfCorruption_Count]);

								if (MarkOfCorruption_Count < 5)
									++MarkOfCorruption_Count;
							}
							events.ScheduleEvent(EVENT_CORRUPTION_MARK, 15000);
							break;
						case EVENT_VILE_SLUDGE: 
							if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0)) 
							{
								//do not cast this on aggro holder
								if (target && target != me->GetVictim())
									DoCast(target, SPELL_VILE_SLUDGE);
							}
							events.ScheduleEvent(EVENT_VILE_SLUDGE, 15000);
							break;
						case EVENT_POS_CHECK_CORRUPT:
							if (me->IsWithinDist2d(HYDROSS_X, HYDROSS_Y, SWITCH_RADIUS))
								EnterCleanPhase();
							else
								events.ScheduleEvent(EVENT_POS_CHECK_CORRUPT, 2500);
							break;
						case EVENT_ENRAGE:
							DoCastSelf(SPELL_ENRAGE);
							break;
					}
				}
			}
			else //clean form
			{	
				while(uint32 eventId = events.ExecuteEvent())
				{
					switch(eventId)
					{
						case EVENT_HYDROSS_MARK:
							if (MarkOfHydross_Count <= 5)
							{
								//remove last mark spell
								if(MarkOfHydross_Count > 0)
								{
									uint32 old_mark_spell = MarkOfHydrossRanks[MarkOfHydross_Count - 1];
                                    for (Player& player : instance->instance->GetPlayers())
									{
										player.RemoveAura(old_mark_spell);
										if (Pet* pet = player.GetPet())
											pet->RemoveAura(old_mark_spell);
									}
								}

                                DoCastVictim(MarkOfHydrossRanks[MarkOfHydross_Count]);

								if (MarkOfHydross_Count < 5)
									++MarkOfHydross_Count;
							}
							events.ScheduleEvent(EVENT_HYDROSS_MARK, 15000);
							break;
						case EVENT_WATER_TOMB: 
							if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
							{
								//do not cast this on aggro holder
								if (target && target != me->GetVictim())
									DoCast(target, SPELL_WATER_TOMB);
							}
							events.ScheduleEvent(EVENT_WATER_TOMB, 7000);
							break;
						case EVENT_POS_CHECK_CLEAN:
							if (!me->IsWithinDist2d(HYDROSS_X, HYDROSS_Y, SWITCH_RADIUS))
								EnterCorruptedPhase();
							else
								events.ScheduleEvent(EVENT_POS_CHECK_CLEAN, 2500);
							break;
						case EVENT_ENRAGE:
							DoCastSelf(SPELL_ENRAGE);
							break;
					}
				}
			}

            DoMeleeAttackIfReady();
        }
		private:
			EventMap events;

			void EnterCorruptedPhase(void)
			{
				// switch to corrupted form
				me->SetDisplayId(MODEL_CORRUPT);
				MarkOfCorruption_Count = 0;
				CorruptedForm = true;

				Talk(SAY_SWITCH_TO_CORRUPT);
				ResetThreatList();
				DeSummonBeams();

				// spawn 4 adds
				for ( int i = 0; i < 4; i++)
				{
					DoSpawnCreature(ENTRY_TAINTED_SPAWN, SpawnPositions[i].GetPositionX(), SpawnPositions[i].GetPositionX(), 3, 0, TEMPSUMMON_CORPSE_DESPAWN, 0);
				}

				me->SetMeleeDamageSchool(SPELL_SCHOOL_NATURE);
				me->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_NATURE, true);
				me->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, false);

				events.CancelEvent(EVENT_WATER_TOMB);
				events.CancelEvent(EVENT_HYDROSS_MARK);
				events.ScheduleEvent(EVENT_CORRUPTION_MARK, 15000);
				events.ScheduleEvent(EVENT_VILE_SLUDGE, 15000);
				events.ScheduleEvent(EVENT_POS_CHECK_CORRUPT, 2500);
			};

			void EnterCleanPhase(void)
			{
				// switch to clean form
				me->SetDisplayId(MODEL_CLEAN);
				CorruptedForm = false;
				MarkOfHydross_Count = 0;

				Talk(SAY_SWITCH_TO_CLEAN);
				ResetThreatList();
				SummonBeams();

				// spawn 4 adds
				for ( int i = 0; i < 4; i++)
				{
					DoSpawnCreature(ENTRY_PURE_SPAWN, SpawnPositions[i].GetPositionX(), SpawnPositions[i].GetPositionX(), 3, 0, TEMPSUMMON_CORPSE_DESPAWN, 0);
				}

				me->SetMeleeDamageSchool(SPELL_SCHOOL_FROST);
				me->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, true);
				me->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_NATURE, false);

				events.CancelEvent(EVENT_VILE_SLUDGE);
				events.CancelEvent(EVENT_CORRUPTION_MARK);
				events.ScheduleEvent(EVENT_HYDROSS_MARK, 15000);
				events.ScheduleEvent(EVENT_WATER_TOMB, 15000);
				events.ScheduleEvent(EVENT_POS_CHECK_CLEAN, 2500);
			};
    };
};

void AddSC_boss_hydross_the_unstable()
{
    new boss_hydross_the_unstable();
}
