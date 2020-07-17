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
#include "culling_of_stratholme.h"

enum corruptorData
{
    SPELL_VOID_STIKE = 60590,
    SPELL_CORRUPTING_BLIGHT = 60588,
    SPELL_CHANNEL_VISUAL = 31387,

    NPC_TIME_RIFT = 28409,
};

enum Yells
{
    SAY_AGGRO                                   = 0,
    SAY_DEATH                                   = 1,
    SAY_FAIL                                    = 2
};

class boss_infinite_corruptor : public CreatureScript
{
public:
    boss_infinite_corruptor() : CreatureScript("boss_infinite_corruptor") { }

    struct boss_infinite_corruptorAI : public ScriptedAI
    {
        boss_infinite_corruptorAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = me->GetInstanceScript();
            timeRiftGUID.Clear();
            JustReachedHome();

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
			me->SetImmuneToAll(true);
            me->SetReactState(REACT_PASSIVE);
        }

        InstanceScript* instance;
        uint32 uiStrikeTimer;
        uint32 uiBlightTimer;
        ObjectGuid timeRiftGUID;
        ObjectGuid timeGuardianGUID;

        void JustReachedHome() override
        {
            if (!timeRiftGUID)
            {
                if(Creature* timeRift = me->SummonCreature(NPC_TIME_RIFT, me->GetPositionX() - 10.0f, me->GetPositionY(), me->GetPositionZ()))
                    timeRiftGUID = timeRift->GetGUID();

            }
            else if(Creature* timeRift = ObjectAccessor::GetCreature(*me,timeRiftGUID))
            {
                timeRift->DisappearAndDie();
                timeRiftGUID.Clear();
                if((timeRift = me->SummonCreature(NPC_TIME_RIFT, me->GetPositionX() - 10.0f, me->GetPositionY(), me->GetPositionZ())))
                    timeRiftGUID = timeRift->GetGUID();
            }
        }

        void EnterCombat(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);
            uiStrikeTimer = urand(1000,3000);
            uiBlightTimer = urand(5000,8000);
        }

        void JustDied(Unit* /*who*/)
        {
            Talk(SAY_DEATH);
            if (instance)
            {
                instance->SetData(DATA_INFINITE_EVENT, DONE);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_CORRUPTING_BLIGHT);
            }
            if(Creature* timeRift = ObjectAccessor::GetCreature(*me,timeRiftGUID))
            {
                timeRift->DisappearAndDie();
                timeRiftGUID.Clear();
            }
            if(Creature* timeGuardian = ObjectAccessor::GetCreature(*me,timeGuardianGUID))
            {
                timeGuardian->RemoveAllAuras();
            }
        }

        void DoAction(int32 action) override
        {
            switch (action)
            {
                case ACTION_DESPAWN:
                    Talk(SAY_FAIL);
                    me->DespawnOrUnsummon(1000);
                    if(Creature* timeRift = ObjectAccessor::GetCreature(*me,timeRiftGUID))
                    {
                        timeRift->DisappearAndDie();
                        timeRiftGUID.Clear();
                    }
                    if(Creature* timeGuardian = ObjectAccessor::GetCreature(*me,timeGuardianGUID))
                        me->Kill(timeGuardian, true);
                case ACTION_SET_AGGRESSIVE:
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
					me->SetImmuneToAll(false);
                    me->SetReactState(REACT_AGGRESSIVE);
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if(!UpdateVictim())
                return;

            if(uiStrikeTimer < diff)
            {
                if(Unit *target = SelectTarget(SELECT_TARGET_MAXTHREAT))
                   DoCast(target, SPELL_VOID_STIKE);
                uiStrikeTimer = urand(3000, 8000);
            }
            else uiStrikeTimer -= diff;

            if(uiBlightTimer < diff)
            {
                if(Unit *target = SelectTarget(SELECT_TARGET_RANDOM))
                   DoCast(target, SPELL_CORRUPTING_BLIGHT);
                uiBlightTimer = urand(6000, 9000);
            }
            else uiBlightTimer -= diff;

            DoMeleeAttackIfReady();
        }

        void Reset() override
        {
            if(Creature* timeGuardian = me->FindNearestCreature(NPC_TIME_GUARDIAN, 50.0f))
                timeGuardianGUID = timeGuardian->GetGUID();
            if(Creature* timeGuardian = ObjectAccessor::GetCreature(*me,timeGuardianGUID))
                me->CastSpell(timeGuardian, SPELL_CHANNEL_VISUAL, true);
        }
    };
    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_infinite_corruptorAI(creature);
    }
};

void AddSC_boss_infinite_corruptor()
{
    new boss_infinite_corruptor();
}
