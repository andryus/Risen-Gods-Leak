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
#include "naxxramas.h"

enum Says
{
    SAY_AGGRO                       = 0,
    SAY_SUMMON                      = 1,
    SAY_SLAY                        = 2,
    SAY_DEATH                       = 3,

    EMOTE_SUMMON                    = 4, // ground phase
    EMOTE_SUMMON_WAVE               = 5, // balcony phase

    SOUND_DEATH                     = 8848,
};

enum Spells
{
    SPELL_CURSE_PLAGUEBRINGER       = 29213,
    SPELL_CURSE_PLAGUEBRINGER_H     = 54835,
    SPELL_BLINK                     = 29208,
    SPELL_CRIPPLE                   = 29212,
    SPELL_CRIPPLE_H                 = 54814,
    SPELL_TELEPORT                  = 29216
};

enum Summons
{
    MOB_WARRIOR         = 16984,
    MOB_CHAMPION        = 16983,
    MOB_GUARDIAN        = 16981
};

enum Events
{
    EVENT_NONE,
    EVENT_BERSERK,
    EVENT_CURSE,
    EVENT_BLINK,
    EVENT_WARRIOR,
    EVENT_BALCONY,
    EVENT_WAVE,
    EVENT_GROUND,
};

#define MAX_SUMMON_POS 5

const float SummonPos[MAX_SUMMON_POS][4] =
{
    {2728.12f, -3544.43f, 261.91f, 6.04f},
    {2729.05f, -3544.47f, 261.91f, 5.58f},
    {2728.24f, -3465.08f, 264.20f, 3.56f},
    {2704.11f, -3456.81f, 265.53f, 4.51f},
    {2663.56f, -3464.43f, 262.66f, 5.20f},
};

const Position TeleportPos = {2631.370f, -3529.680f, 274.040f, 6.277f};

class boss_noth : public CreatureScript
{
public:
    boss_noth() : CreatureScript("boss_noth") { }

    struct boss_nothAI : public BossAI
    {
        boss_nothAI(Creature* c) : BossAI(c, BOSS_NOTH) {}

        uint32 waveCount, balconyCount;

        void Reset() override
        {
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            BossAI::Reset();
        }

        void EnterCombat(Unit* who) override
        {
            BossAI::EnterCombat(who);
            Talk(SAY_AGGRO);
            balconyCount = 0;
            EnterPhaseGround();
        }

        void EnterPhaseGround() 
        {
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            DoZoneInCombat();
            if (me->GetThreatManager().isThreatListEmpty())
                EnterEvadeMode();
            else
            {
                events.ScheduleEvent(EVENT_BALCONY, 110000);
                events.ScheduleEvent(EVENT_CURSE, 10000+rand32()%15000);
                events.ScheduleEvent(EVENT_WARRIOR, 30000);
                events.ScheduleEvent(EVENT_BLINK, 20000 + rand32()%20000);
            }
        }

        void KilledUnit(Unit* victim) override
        {
            if (!(rand32()%5))
                Talk(SAY_SLAY);
        }

        void JustSummoned(Creature* summon) override
        {
            summons.Summon(summon);
            summon->SetInCombatWithZone();
        }

        void JustDied(Unit* killer) override
        {
            BossAI::JustDied(killer);
            Talk(SAY_DEATH);
            if(instance)
                instance->SetBossState(BOSS_NOTH, DONE);
        }

        void SummonUndead(uint32 entry, uint32 num)
        {
            for (uint32 i = 0; i < num; ++i)
            {
                uint32 pos = rand32()%MAX_SUMMON_POS;
                me->SummonCreature(entry, SummonPos[pos][0], SummonPos[pos][1], SummonPos[pos][2],
                    SummonPos[pos][3], TEMPSUMMON_CORPSE_DESPAWN, 60000);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);


            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_CURSE:
                        DoCastAOE(SPELL_CURSE_PLAGUEBRINGER);
                        events.ScheduleEvent(EVENT_CURSE, 50000 + rand32()%10000);
                        return;
                    case EVENT_WARRIOR:
                        Talk(SAY_SUMMON);
                        Talk(EMOTE_SUMMON);
                        SummonUndead(MOB_WARRIOR, RAID_MODE(2, 3));
                        events.ScheduleEvent(EVENT_WARRIOR, 30000);
                        return;
                    case EVENT_BLINK:
                        DoCastAOE(RAID_MODE<uint32>(SPELL_CRIPPLE, SPELL_CRIPPLE_H), true);
                        if (GetDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL)
                        {
                            DoCastAOE(SPELL_BLINK);
                            ResetThreatList();
                        }
                        events.ScheduleEvent(EVENT_BLINK, 40000);
                        return;
                    case EVENT_BALCONY:
                        me->SetReactState(REACT_PASSIVE);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        me->AttackStop();
                        me->RemoveAllAuras();
                        me->NearTeleportTo(TeleportPos.GetPositionX(), TeleportPos.GetPositionY(), TeleportPos.GetPositionZ(), TeleportPos.GetOrientation());
                        events.Reset();
                        events.ScheduleEvent(EVENT_WAVE, 2000 + rand32()%3000);
                        waveCount = 0;
                        return;
                    case EVENT_WAVE:
                        Talk(EMOTE_SUMMON_WAVE);
                        switch(balconyCount)
                        {
                            case 0: SummonUndead(MOB_CHAMPION, RAID_MODE(2, 4)); break;
                            case 1: SummonUndead(MOB_CHAMPION, RAID_MODE(1, 2));
                                    SummonUndead(MOB_GUARDIAN, RAID_MODE(1, 2)); break;
                            case 2: SummonUndead(MOB_GUARDIAN, RAID_MODE(2, 4)); break;
                            default:SummonUndead(MOB_CHAMPION, RAID_MODE(5, 10));
                                    SummonUndead(MOB_GUARDIAN, RAID_MODE(5, 10));break;
                        }
                        ++waveCount;
                        events.ScheduleEvent(waveCount < 2 ? EVENT_WAVE : EVENT_GROUND, 30000 + rand32()%15000);
                        return;
                    case EVENT_GROUND:
                    {
                        ++balconyCount;
                        me->NearTeleportTo(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY(), me->GetHomePosition().GetPositionZ(), me->GetHomePosition().GetOrientation());
                        events.ScheduleEvent(EVENT_BALCONY, 110000);
                        EnterPhaseGround();
                        return;
                    }
                }
            }

            if (me->HasReactState(REACT_AGGRESSIVE))
                DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_nothAI (creature);
    }
};

void AddSC_boss_noth()
{
    new boss_noth();
}
