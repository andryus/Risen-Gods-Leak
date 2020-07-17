/*
 * Copyright (C) 2008-2018 TrinityCore <http://www.trinitycore.org/>
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

enum jandisBarovMisc
{
    SPELL_CURSE_OF_BLOOD        = 24673,
    SPELL_ILLUSION              = 17773,
    SPELL_CLEAVE                = 15584,

    NPC_ILLUSION                = 11439,

    DISPLAY_ID_VISIBLE          = 11073,
    DISPLAY_ID_INVISIBLE        = 11686,
};

enum Events
{
    EVENT_CURSE_OF_BLOOD = 1,
    EVENT_ILLUSION,
    EVENT_SET_VISIBILITY
};

struct boss_jandice_barovAI : public ScriptedAI
{
    boss_jandice_barovAI(Creature* creature) : ScriptedAI(creature), Summons(me) { }

    void Reset() override
    {
        _events.Reset();
        me->SetDisplayId(DISPLAY_ID_VISIBLE);
        Summons.DespawnAll();
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        _isVisible = true;
    }

    void JustSummoned(Creature* summoned) override
    {
        // Illusions should attack a random target.
        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
            summoned->AI()->AttackStart(target);

        summoned->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_MAGIC, true);
        Summons.Summon(summoned);
    }

    void EnterCombat(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_CURSE_OF_BLOOD, Seconds(15));
        _events.ScheduleEvent(EVENT_ILLUSION, Seconds(30));
    }

    void JustDied(Unit* /*killer*/) override
    {
        me->SetDisplayId(DISPLAY_ID_VISIBLE);
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);     
        Summons.DespawnAll();
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage) override
    {
        if (!_isVisible)
            damage = 0;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_CURSE_OF_BLOOD:
                    DoCastVictim(SPELL_CURSE_OF_BLOOD);
                    _events.ScheduleEvent(EVENT_CURSE_OF_BLOOD, Seconds(30));
                    break;
                case EVENT_ILLUSION:
                    DoCast(SPELL_ILLUSION);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    me->SetDisplayId(DISPLAY_ID_INVISIBLE);
                    _isVisible = false;
                    me->GetThreatManager().ModifyThreatByPercent(me->GetVictim(), -99);
                    _events.ScheduleEvent(EVENT_SET_VISIBILITY, Seconds(3));
                    _events.ScheduleEvent(EVENT_ILLUSION, Seconds(25));
                    break;
                case EVENT_SET_VISIBILITY:
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    me->SetDisplayId(DISPLAY_ID_VISIBLE);
                    _isVisible = true;
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        if (_isVisible)
            DoMeleeAttackIfReady();
    }

    private:
        EventMap _events;
        SummonList Summons;

        bool    _isVisible;
};

void AddSC_boss_jandicebarov()
{
    new CreatureScriptLoaderEx<boss_jandice_barovAI>("boss_jandice_barov");
}
