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
#include "vault_of_archavon.h"
#include "SpellScript.h"

enum Emotes
{
    EMOTE_BERSERK            = 0,
    EMOTE_IMPALE             = 1
};

enum Spells
{
    //! Archavon
    SPELL_ROCK_SHARDS           = 58678,
    SPELL_ROCK_SHARDS_VISUAL_L  = 58689,
    SPELL_ROCK_SHARDS_VISUAL_R  = 58692,
    SPELL_ROCK_SHARDS_DAMAGE_L  = 58695,
    SPELL_ROCK_SHARDS_DAMAGE_R  = 58696,
    SPELL_CRUSHING_LEAP         = 58960,
    SPELL_STOMP                 = 58663,
    SPELL_IMPALE                = 58666,
    SPELL_BERSERK               = 47008,

    //! Archavon Warder
    SPELL_ROCK_SHOWER           = 60919,
    SPELL_SHIELD_CRUSH          = 60897,
    SPELL_WHIRL                 = 60902
};

enum Events
{
    //! Archavon
    EVENT_ROCK_SHARDS       = 1,
    EVENT_CRUSHING_LEAP     = 2,
    EVENT_RUN_BACK          = 3,
    EVENT_IMPALE            = 4,    // 3sec after Stomp
    EVENT_CHOKING_CLOUD     = 5,    // after Crushing Leap
    EVENT_BERSERK           = 6,
    EVENT_STOMP             = 7,

    //! Archavon Stone Warder
    EVENT_ROCK_SHOWER       = 8,
    EVENT_SHIELD_CRUSH      = 9,
    EVENT_WHIRL             = 10
};

class boss_archavon : public CreatureScript
{
    public:
        boss_archavon() : CreatureScript("boss_archavon") { }

        struct boss_archavonAI : public BossAI
        {
            boss_archavonAI(Creature* creature) : BossAI(creature, BOSS_ARCHAVON) { }

            void Reset()
            {
                BossAI::Reset();
                PlayerGUID.Clear();
                me->GetHomePosition().GetPosition(&pos);
            }

            void EnterCombat(Unit* who)
            {
                BossAI::EnterCombat(who);

                events.ScheduleEvent(EVENT_ROCK_SHARDS,   12*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_CRUSHING_LEAP, 30*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_STOMP,         45*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_BERSERK,       300*IN_MILLISECONDS);
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                _DespawnAtEvade();
            }

            Unit* GetPlayerInRange(float RangeMin, float RangeMax)
            {
                if (!me->GetMap()->IsDungeon())
                    return NULL;

                std::list<Player*> playerList;
                for (Player& player : me->GetMap()->GetPlayers())
                {
                    if (!player.IsAlive() || player.IsGameMaster())
                        continue;

                    float distance = me->GetDistance(player);
                    if (distance < RangeMin || distance > RangeMax)
                        continue;

                    if (!me->IsWithinLOS(player.GetPositionX(), player.GetPositionY(), player.GetPositionZ()))
                        continue;

                    playerList.push_back(&player);
                }

                if (playerList.empty())
                    return NULL;

                return Trinity::Containers::SelectRandomContainerElement(playerList);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING) || me->IsNonMeleeSpellCast(true))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ROCK_SHARDS:
                            if (Unit* target = GetPlayerInRange(0.0f, 50.0f))
                                DoCast(target, SPELL_ROCK_SHARDS);
                            events.ScheduleEvent(EVENT_ROCK_SHARDS, 12*IN_MILLISECONDS);
                            return;
                        case EVENT_CRUSHING_LEAP:
                            me->GetPosition(&pos);
                            if (Unit* target = GetPlayerInRange(10.0f, 80.0f))
                                DoCast(target, SPELL_CRUSHING_LEAP);
                            //events.ScheduleEvent(EVENT_RUN_BACK, 5*IN_MILLISECONDS);
                            events.ScheduleEvent(EVENT_CRUSHING_LEAP, 30*IN_MILLISECONDS);
                            return;
                        case EVENT_RUN_BACK:
                            me->GetMotionMaster()->MovePoint(1, pos);
                            return;
                        case EVENT_STOMP:
                            PlayerGUID = me->GetVictim() ? me->GetVictim()->GetGUID() : ObjectGuid::Empty;
                            DoCastVictim(SPELL_STOMP);
                            events.ScheduleEvent(EVENT_IMPALE, 0);
                            events.ScheduleEvent(EVENT_STOMP, 45*IN_MILLISECONDS);                            
                            Talk(EMOTE_IMPALE, me->GetVictim());
                            return;
                        case EVENT_IMPALE:
                            if (Player* target = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                            {
                                Talk(EMOTE_IMPALE, target);
                                DoCast(target, SPELL_IMPALE, true);
                                ModifyThreatByPercent(target, -100);
                            }
                            return; 
                        case EVENT_BERSERK:
                            DoCastSelf(SPELL_BERSERK);
                            Talk(EMOTE_BERSERK);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            ObjectGuid PlayerGUID;
            Position pos;

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_archavonAI(creature);
        }
};

// 58941 - Rock Shards
class spell_archavon_rock_shards : public SpellScriptLoader
{
    public:
        spell_archavon_rock_shards() : SpellScriptLoader("spell_archavon_rock_shards") { }

        class spell_archavon_rock_shards_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_archavon_rock_shards_SpellScript);

            bool Validate(SpellInfo const* /*spellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_ROCK_SHARDS_VISUAL_L)
                    || !sSpellMgr->GetSpellInfo(SPELL_ROCK_SHARDS_VISUAL_R)
                    || !sSpellMgr->GetSpellInfo(SPELL_ROCK_SHARDS_DAMAGE_L)
                    || !sSpellMgr->GetSpellInfo(SPELL_ROCK_SHARDS_DAMAGE_R))
                    return false;
                return true;
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();

                for (uint8 i = 0; i < 3; ++i)
                {
                    caster->CastSpell((Unit*)NULL, SPELL_ROCK_SHARDS_VISUAL_L, true);
                    caster->CastSpell((Unit*)NULL, SPELL_ROCK_SHARDS_VISUAL_R, true);
                }

                caster->CastSpell((Unit*)NULL, SPELL_ROCK_SHARDS_DAMAGE_L, true);
                caster->CastSpell((Unit*)NULL, SPELL_ROCK_SHARDS_DAMAGE_R, true);
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_archavon_rock_shards_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_archavon_rock_shards_SpellScript();
        }
};

class npc_archavon_warder : public CreatureScript
{
    public:
        npc_archavon_warder() : CreatureScript("npc_archavon_warder") { }

        struct npc_archavon_warderAI : public ScriptedAI
        {
            npc_archavon_warderAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                events.Reset();
                events.ScheduleEvent(EVENT_ROCK_SHOWER,  2*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SHIELD_CRUSH, 20*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_WHIRL,        7*IN_MILLISECONDS);
            }

            void EnterCombat(Unit* /*who*/)
            {
                DoZoneInCombat();
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
                        case EVENT_ROCK_SHOWER:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                                DoCast(target, SPELL_ROCK_SHOWER);
                            events.ScheduleEvent(EVENT_ROCK_SHARDS, 6*IN_MILLISECONDS);
                            break;
                        case EVENT_SHIELD_CRUSH:
                            DoCastVictim(SPELL_SHIELD_CRUSH);
                            events.ScheduleEvent(EVENT_SHIELD_CRUSH, 20*IN_MILLISECONDS);
                            break;
                        case EVENT_WHIRL:
                            DoCastVictim(SPELL_WHIRL);
                            events.ScheduleEvent(EVENT_WHIRL, 8*IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap events;

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_archavon_warderAI(creature);
        }
};

void AddSC_boss_archavon()
{
    new boss_archavon();
    new spell_archavon_rock_shards();

    new npc_archavon_warder();
}
