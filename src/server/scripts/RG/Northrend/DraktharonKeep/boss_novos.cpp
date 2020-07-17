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
#include "SpellScript.h"
#include "ScriptedCreature.h"
#include "drak_tharon_keep.h"

enum Yells
{
    SAY_AGGRO = 0,              // 0: The chill that you feel is the herald of your doom!
    SAY_KILL = 1,               // 1: Such is the fate of all who oppose the Lich King.
    SAY_DEATH = 2,              // 2: Your efforts... are in vain.
    SAY_SUMMONING_ADDS = 3,     // 3: Bolster my defenses! Hurry, curse you!
    SAY_ARCANE_FIELD = 4        // 4: Surely you can see the futility of it all!
                                // 4: Just give up and die already!
};

enum Misc
{
    ACTION_RESET_CRYSTALS,
    ACTION_ACTIVATE_CRYSTAL,
    ACTION_DEACTIVATE,
    EVENT_ATTACK,
    EVENT_SUMMON_MINIONS,
    DATA_NOVOS_ACHIEV
};

enum Creatures
{
    NPC_CRYSTAL_CHANNEL_TARGET      = 26712,
    NPC_FETID_TROLL_CORPSE          = 27598,
    NPC_RISEN_SHADOWCASTER          = 27600,
    NPC_HULKING_CORPSE              = 27597
};

enum Spells
{
    SPELL_BEAM_CHANNEL              = 52106,
    SPELL_ARCANE_FIELD              = 47346,

    SPELL_SUMMON_RISEN_SHADOWCASTER = 49105,
    SPELL_SUMMON_FETID_TROLL_CORPSE = 49103,
    SPELL_SUMMON_HULKING_CORPSE     = 49104,
    SPELL_SUMMON_CRYSTAL_HANDLER    = 49179,

    SPELL_ARCANE_BLAST              = 49198,
    SPELL_BLIZZARD                  = 49034,
    SPELL_FROSTBOLT                 = 49037,
    SPELL_WRATH_OF_MISERY           = 50089,
    SPELL_SUMMON_MINIONS            = 59910
};

struct SummonerInfo
{
    uint32 data, spell, timer;
};

const SummonerInfo summoners[] =
{
    { DATA_NOVOS_SUMMONER_1, SPELL_SUMMON_RISEN_SHADOWCASTER, 15000 },
    { DATA_NOVOS_SUMMONER_2, SPELL_SUMMON_FETID_TROLL_CORPSE, 5000 },
    { DATA_NOVOS_SUMMONER_3, SPELL_SUMMON_HULKING_CORPSE, 30000 },
    { DATA_NOVOS_SUMMONER_4, SPELL_SUMMON_CRYSTAL_HANDLER, 30000 },
    { DATA_NOVOS_SUMMONER_5, SPELL_SUMMON_CRYSTAL_HANDLER, 30000 }
};

#define MAX_Y_COORD_OH_NOVOS        -771.95f

class boss_novos : public CreatureScript
{
public:
    boss_novos() : CreatureScript("boss_novos") { }

    struct boss_novosAI : public BossAI
    {
        boss_novosAI(Creature* creature) : BossAI(creature, DATA_NOVOS_EVENT) {}

        void InitializeAI() override
        {
            SetCombatMovement(false);
        }

        void Reset() override
        {
            BossAI::Reset();

            _ohNovos = true;
            _crystalHandlerCount = 0;
            SetCrystalsStatus(false);
            SetSummonerStatus(false);
            SetBubbled(false);
        }

        void EnterCombat(Unit* victim) override
        {
            BossAI::EnterCombat(victim);

            Talk(SAY_AGGRO); 

            SetCrystalsStatus(true);
            SetSummonerStatus(true);
            SetBubbled(true);
        }

        void KilledUnit(Unit* who) override
        {
            if (who->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_KILL);
        }

        void JustDied(Unit* killer) override
        {
            BossAI::JustDied(killer);
            Talk(SAY_DEATH);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim() || _bubbled)
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
                ExecuteEvent(eventId);
        }

        void ExecuteEvent(uint32 eventId) override
        {
            switch (eventId)
            {
                case EVENT_SUMMON_MINIONS:  // Hero only
                    DoCast(SPELL_SUMMON_MINIONS);
                    Talk(SAY_SUMMONING_ADDS);
                    events.ScheduleEvent(EVENT_SUMMON_MINIONS, 15000);
                    break;
                case EVENT_ATTACK:
                    if (Unit* victim = SelectTarget(SELECT_TARGET_RANDOM))
                        DoCast(victim, RAND(SPELL_ARCANE_BLAST, SPELL_BLIZZARD, SPELL_FROSTBOLT, SPELL_WRATH_OF_MISERY));
                    events.ScheduleEvent(EVENT_ATTACK, 3000);
                    break;
                default:
                    break;
            }
        }

        void DoAction(int32 action) override
        {
            if (action == ACTION_CRYSTAL_HANDLER_DIED)
                CrystalHandlerDied();
        }

        void MoveInLineOfSight(Unit* who) override
        {
            BossAI::MoveInLineOfSight(who);

            if (!_ohNovos || !who || who->GetTypeId() != TYPEID_UNIT || who->GetPositionY() > MAX_Y_COORD_OH_NOVOS)
                return;

            uint32 entry = who->GetEntry();

            if (me->GetExactDist(who) < 34.5f)
                if (entry == NPC_HULKING_CORPSE || entry == NPC_RISEN_SHADOWCASTER || entry == NPC_FETID_TROLL_CORPSE)
                    _ohNovos = false;
        }

        uint32 GetData(uint32 type) const override
        {
            return type == DATA_NOVOS_ACHIEV && _ohNovos ? 1 : 0;
        }

        void JustSummoned(Creature* summon) override
        {
            summons.Summon(summon);
        }

    private:
        void SetBubbled(bool state)
        {
            _bubbled = state;
            if (!state)
            {
                if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                if (me->IsImmuneToPC())
                    me->SetImmuneToPC(false);
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    me->CastStop();
            }
            else
            {
                if (!me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                if (!me->IsImmuneToPC())
                    me->SetImmuneToPC(true);
                DoCast(SPELL_ARCANE_FIELD);
            }
        }

        void SetSummonerStatus(bool active)
        {
            uint8 rnd = rand32() % 2;
            for (uint8 i = 0; i < 3; i++)
                if (ObjectGuid guid = ObjectGuid(instance->GetGuidData(summoners[i].data)))
                    if (Creature* crystalChannelTarget = ObjectAccessor::GetCreature(*me, guid))
                    {
                        if (active)
                            crystalChannelTarget->AI()->SetData(summoners[i].spell, summoners[i].timer);
                        else
                            crystalChannelTarget->AI()->Reset();
                    }

            if (ObjectGuid guid = ObjectGuid(instance->GetGuidData(summoners[3+rnd].data)))
                if (Creature* crystalChannelTarget = instance->instance->GetCreature(guid))
                {
                    if (active)
                        crystalChannelTarget->AI()->SetData(summoners[3+rnd].spell, summoners[3+rnd].timer);
                    else
                        crystalChannelTarget->AI()->Reset();
                }
        }

        void SetCrystalsStatus(bool active)
        {
            for (uint8 i = 0; i < 4; i++)
                if (ObjectGuid guid = ObjectGuid(instance->GetGuidData(DATA_NOVOS_CRYSTAL_1 + i)))
                    if (GameObject* crystal = ObjectAccessor::GetGameObject(*me, guid))
                        SetCrystalStatus(crystal, active);
        }

        void SetCrystalStatus(GameObject* crystal, bool active)
        {
            if (!crystal)
                return;

            crystal->SetGoState(active ? GO_STATE_ACTIVE : GO_STATE_READY);
            if (Creature* crystalChannelTarget = crystal->FindNearestCreature(NPC_CRYSTAL_CHANNEL_TARGET, 5.0f))
            {
                if (active)
                    crystalChannelTarget->AI()->DoCastAOE(SPELL_BEAM_CHANNEL);
                else if (crystalChannelTarget->HasUnitState(UNIT_STATE_CASTING))
                    crystalChannelTarget->CastStop();
            }
        }

        void CrystalHandlerDied()
        {
            for (uint8 i = 0; i < 4; i++)
                if (ObjectGuid guid = ObjectGuid(instance->GetGuidData(DATA_NOVOS_CRYSTAL_1 + i)))
                    if (GameObject* crystal = ObjectAccessor::GetGameObject(*me, guid))
                        if (crystal->GetGoState() == GO_STATE_ACTIVE)
                        {
                            SetCrystalStatus(crystal, false);
                            break;
                        }

            uint8 rnd = rand32() % 2;

            if (++_crystalHandlerCount >= 4)
            {
                Talk(SAY_ARCANE_FIELD);
                SetSummonerStatus(false);
                SetBubbled(false);
                events.ScheduleEvent(EVENT_ATTACK, 3000);
                if (IsHeroic())
                    events.ScheduleEvent(EVENT_SUMMON_MINIONS, 15000);
            }
            else if (rnd == 1)
            {
                if (ObjectGuid guid = ObjectGuid(instance->GetGuidData(DATA_NOVOS_SUMMONER_4)))
                    if (Creature* crystalChannelTarget = ObjectAccessor::GetCreature(*me, guid))
                        crystalChannelTarget->AI()->SetData(SPELL_SUMMON_CRYSTAL_HANDLER, 15000);
            } 
            else 
            {
                if (ObjectGuid guid = ObjectGuid(instance->GetGuidData(DATA_NOVOS_SUMMONER_5)))
                    if (Creature* crystalChannelTarget = ObjectAccessor::GetCreature(*me, guid))
                        crystalChannelTarget->AI()->SetData(SPELL_SUMMON_CRYSTAL_HANDLER, 15000);
            }
      }

        uint8 _crystalHandlerCount;
        bool _ohNovos;
        bool _bubbled;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_novosAI(creature);
    }
};

class npc_crystal_channel_target : public CreatureScript
{
public:
    npc_crystal_channel_target() : CreatureScript("npc_crystal_channel_target") {}

    struct npc_crystal_channel_targetAI : public ScriptedAI
    {
        npc_crystal_channel_targetAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset() override
        {
            _spell = 0;
            _timer = 0;
            _temp = 0;
        }

        void UpdateAI(uint32 diff) override
        {
            if (_spell)
            {
                if (_temp <= diff)
                {
                    DoCast(_spell);
                    _temp = _timer;
                }
                else
                    _temp -= diff;
            }
        }

        void SetData(uint32 id, uint32 value) override
        {
            _spell = id;
            _timer = value;
            _temp = value;
        }

        void JustSummoned(Creature* summon) override
        {
            if (InstanceScript* instance = me->GetInstanceScript())
                if (ObjectGuid guid = ObjectGuid(instance->GetGuidData(DATA_NOVOS)))
                    if (Creature* novos = ObjectAccessor::GetCreature(*me, guid))
                        novos->AI()->JustSummoned(summon);

            float x = summon->GetPositionX();
            float y = summon->GetPositionY();
            float z = summon->GetPositionZ();
            uint8 left = 0;    //if is the left one, than use summon->GetEntry() * 100 +1 otherwise summon->GetEntry() * 100 for path
                            // Only for Crystal Handler
            if (x < -415.0f && x > -419.0f && y > -723.0f && y < -719.0f && z < 30.0f && z > 26.0f)
                left = 1;


            if (summon)
                summon->GetMotionMaster()->MovePath(summon->GetEntry() * 100 + left, false);

            if (_spell == SPELL_SUMMON_CRYSTAL_HANDLER)
                Reset();
        }

    private:
        uint32 _spell;
        uint32 _timer;
        uint32 _temp;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_crystal_channel_targetAI(creature);
    }
};

class achievement_oh_novos : public AchievementCriteriaScript
{
public:
    achievement_oh_novos() : AchievementCriteriaScript("achievement_oh_novos") {}

    bool OnCheck(Player* /*player*/, Unit* target) override
    {
        return target && target->GetTypeId() == TYPEID_UNIT && target->ToCreature()->AI()->GetData(DATA_NOVOS_ACHIEV);
    }
};

enum SummonMinions
{
    SPELL_COPY_OF_SUMMON_MINIONS        = 59933
};

class spell_summon_minions : public SpellScriptLoader
{
public:
    spell_summon_minions() : SpellScriptLoader("spell_summon_minions") { }

    class spell_summon_minions_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_summon_minions_SpellScript);

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            GetCaster()->CastSpell((Unit*)NULL, SPELL_COPY_OF_SUMMON_MINIONS, true);
            GetCaster()->CastSpell((Unit*)NULL, SPELL_COPY_OF_SUMMON_MINIONS, true);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_summon_minions_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_summon_minions_SpellScript();
    }
};

void AddSC_boss_novos()
{
    new boss_novos();
    new npc_crystal_channel_target();
    new spell_summon_minions();
    new achievement_oh_novos();
}
