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
#include "GridNotifiers.h"
#include "CombatAI.h"
#include "naxxramas.h"

#define SPELL_DEATH_GRIP           49576

enum Yells
{
    SAY_SPEECH                  = 0,
    SAY_KILL                    = 1,
    SAY_DEATH                   = 2,
    SAY_TELEPORT                = 3
};

//Gothik
enum Spells
{
    SPELL_HARVEST_SOUL          = 28679,
    SPELL_SHADOW_BOLT           = 29317,
    H_SPELL_SHADOW_BOLT         = 56405,
    SPELL_INFORM_LIVE_TRAINEE   = 27892,
    SPELL_INFORM_LIVE_KNIGHT    = 27928,
    SPELL_INFORM_LIVE_RIDER     = 27935,
    SPELL_INFORM_DEAD_TRAINEE   = 27915,
    SPELL_INFORM_DEAD_KNIGHT    = 27931,
    SPELL_INFORM_DEAD_RIDER     = 27937,

    SPELL_SHADOW_MARK           = 27825
};

enum Creatures
{
    MOB_LIVE_TRAINEE    = 16124,
    MOB_LIVE_KNIGHT     = 16125,
    MOB_LIVE_RIDER      = 16126,
    MOB_DEAD_TRAINEE    = 16127,
    MOB_DEAD_KNIGHT     = 16148,
    MOB_DEAD_RIDER      = 16150,
    MOB_DEAD_HORSE      = 16149
};

struct Waves { uint32 entry, time, mode; };
// wave setups are not the same in heroic and normal difficulty,
// mode is 0 only normal, 1 both and 2 only heroic
// but this is handled in DoGothikSummon function
const Waves waves[] =
{
    {MOB_LIVE_TRAINEE, 20000, 1},
    {MOB_LIVE_TRAINEE, 20000, 1},
    {MOB_LIVE_TRAINEE, 10000, 1},
    {MOB_LIVE_KNIGHT, 10000, 1},
    {MOB_LIVE_TRAINEE, 15000, 1},
    {MOB_LIVE_KNIGHT, 5000, 1},
    {MOB_LIVE_TRAINEE, 20000, 1},
    {MOB_LIVE_TRAINEE, 0, 1},
    {MOB_LIVE_KNIGHT, 10000, 1},
    {MOB_LIVE_TRAINEE, 10000, 2},
    {MOB_LIVE_RIDER, 10000, 0},
    {MOB_LIVE_RIDER, 5000, 2},
    {MOB_LIVE_TRAINEE, 5000, 0},
    {MOB_LIVE_TRAINEE, 15000, 2},
    {MOB_LIVE_KNIGHT, 15000, 0},
    {MOB_LIVE_TRAINEE, 0, 0},
    {MOB_LIVE_RIDER, 10000, 1},
    {MOB_LIVE_KNIGHT, 10000, 1},
    {MOB_LIVE_TRAINEE, 10000, 0},
    {MOB_LIVE_RIDER, 10000, 2},
    {MOB_LIVE_TRAINEE, 0, 2},
    {MOB_LIVE_RIDER, 5000, 1},
    {MOB_LIVE_TRAINEE, 0, 2},
    {MOB_LIVE_KNIGHT, 5000, 1},
    {MOB_LIVE_RIDER, 0, 2},
    {MOB_LIVE_TRAINEE, 20000, 1},
    {MOB_LIVE_RIDER, 0, 1},
    {MOB_LIVE_KNIGHT, 0, 1},
    {MOB_LIVE_TRAINEE, 25000, 2},
    {MOB_LIVE_TRAINEE, 15000, 0},
    {MOB_LIVE_TRAINEE, 25000, 0},
    {0, 0, 1},
};

#define POS_Y_GATE  -3360.78f
#define POS_Y_WEST  -3285.0f
#define POS_Y_EAST  -3434.0f
#define POS_X_NORTH  2750.49f
#define POS_X_SOUTH  2633.84f

#define IN_LIVE_SIDE(who) (who->GetPositionY() < POS_Y_GATE)

enum Events
{
    EVENT_NONE,
    EVENT_SUMMON,
    EVENT_HARVEST,
    EVENT_BOLT,
    EVENT_TELEPORT
};
enum Pos
{
   POS_LIVE = 6,
   POS_DEAD = 5
};

const Position PosSummonLive[POS_LIVE] =
{
    {2669.7f, -3428.76f, 268.56f, 1.6f},
    {2692.1f, -3428.76f, 268.56f, 1.6f},
    {2714.4f, -3428.76f, 268.56f, 1.6f},
    {2669.7f, -3431.67f, 268.56f, 1.6f},
    {2692.1f, -3431.67f, 268.56f, 1.6f},
    {2714.4f, -3431.67f, 268.56f, 1.6f},
};

const Position PosSummonDead[POS_DEAD] =
{
    {2725.1f, -3310.0f, 268.85f, 3.4f},
    {2699.3f, -3322.8f, 268.60f, 3.3f},
    {2733.1f, -3348.5f, 268.84f, 3.1f},
    {2682.8f, -3304.2f, 268.85f, 3.9f},
    {2664.8f, -3340.7f, 268.23f, 3.7f},
};

const Position PosGroundLiveSide(2691.2f, -3387.0f, 267.68f, 1.52f);
const Position PosGroundDeadSide(2693.5f, -3334.6f, 267.68f, 4.67f);
const Position PosPlatform(2640.5f, -3360.6f, 285.26f, 0.0f);

// Predicate function to check that the r   efzr unit is NOT on the same side as the source.
struct NotOnSameSide : public std::unary_function<Unit *, bool> {
    bool m_inLiveSide;
    NotOnSameSide(Unit *pSource) : m_inLiveSide(IN_LIVE_SIDE(pSource)) {}
    bool operator() (const Unit *pTarget) {
      return (m_inLiveSide != IN_LIVE_SIDE(pTarget));
    }
};

class boss_gothik : public CreatureScript
{
public:
    boss_gothik() : CreatureScript("boss_gothik") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_gothikAI (pCreature);
    }

    struct boss_gothikAI : public BossAI
    {
        boss_gothikAI(Creature *c) : BossAI(c, BOSS_GOTHIK) { }

        uint32 waveCount;
        typedef std::vector<Creature*> TriggerVct;
        TriggerVct liveTrigger, deadTrigger;
        bool mergedSides;
        bool phaseTwo;
        bool thirtyPercentReached;

        std::vector<ObjectGuid> LiveTriggerGUID;
        std::vector<ObjectGuid> DeadTriggerGUID;

        void Reset()
        {
            LiveTriggerGUID.clear();
            DeadTriggerGUID.clear();

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_1|UNIT_FLAG_REMOVE_CLIENT_CONTROL);
            me->SetReactState(REACT_PASSIVE);
            if (instance)
                instance->SetData(DATA_GOTHIK_GATE, GO_STATE_ACTIVE);
            BossAI::Reset();
            mergedSides = false;
            phaseTwo = false;
            thirtyPercentReached = false;
        }

        void EnterCombat(Unit* who)
        {
            for (uint32 i = 0; i < POS_LIVE; ++i)
                if (Creature *trigger = DoSummon(WORLD_TRIGGER, PosSummonLive[i]))
                    LiveTriggerGUID.push_back(trigger->GetGUID());
            for (uint32 i = 0; i < POS_DEAD; ++i)
                if (Creature *trigger = DoSummon(WORLD_TRIGGER, PosSummonDead[i]))
                    DeadTriggerGUID.push_back(trigger->GetGUID());

            if (LiveTriggerGUID.size() < POS_LIVE || DeadTriggerGUID.size() < POS_DEAD)
            {
                TC_LOG_ERROR("scripts", "Script Gothik: cannot summon triggers!");
                EnterEvadeMode();
                return;
            }

            BossAI::EnterCombat(who);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_1|UNIT_FLAG_REMOVE_CLIENT_CONTROL);
            waveCount = 0;
            events.ScheduleEvent(EVENT_SUMMON, 30000);
            DoTeleportTo(PosPlatform);
            Talk(SAY_SPEECH);
            if (instance)
                instance->SetData(DATA_GOTHIK_GATE, GO_STATE_READY);
        }

        void JustSummoned(Creature *summon)
        {
            if (summon->GetEntry() == WORLD_TRIGGER)
                summon->setActive(true);
            else if (!mergedSides)
            {
                summon->AI()->DoAction(me->HasReactState(REACT_PASSIVE) ? 1 : 0);
                summon->AI()->EnterEvadeMode();
            }
            else
            {
                summon->AI()->DoAction(0);
                summon->AI()->DoZoneInCombat();
            }
            summons.Summon(summon);
        }

        void SummonedCreatureDespawn(Creature *summon)
        {
            summons.Despawn(summon);
        }

        void KilledUnit(Unit* victim)
        {
            if (!(rand32()%5))
                    Talk(SAY_KILL);
        }

        void JustDied(Unit* killer)
        {
            LiveTriggerGUID.clear();
            DeadTriggerGUID.clear();
            BossAI::JustDied(killer);
            Talk(SAY_DEATH);
            if (instance)
            {
                instance->SetData(DATA_GOTHIK_GATE, GO_STATE_ACTIVE);
                instance->SetBossState(BOSS_GOTHIK, DONE);
            }

            if (Player* nearest = me->FindNearestPlayer(500.0f))
            {
                me->NearTeleportTo(nearest->GetPositionX(), nearest->GetPositionY(), nearest->GetPositionZ(), nearest->GetOrientation());
                me->SetHomePosition(nearest->GetPositionX(), nearest->GetPositionY(), nearest->GetPositionZ(), nearest->GetOrientation());
            }
        }

        void DoGothikSummon(uint32 entry)
        {
            if (GetDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL)
            {
                switch (entry)
                {
                    case MOB_LIVE_TRAINEE:
                    {
                        if (Creature *LiveTrigger0 = ObjectAccessor::GetCreature(*me, LiveTriggerGUID[0]))
                            DoSummon(MOB_LIVE_TRAINEE, LiveTrigger0, 1);
                        if (Creature *LiveTrigger1 = ObjectAccessor::GetCreature(*me, LiveTriggerGUID[1]))
                            DoSummon(MOB_LIVE_TRAINEE, LiveTrigger1, 1);
                        if (Creature *LiveTrigger2 = ObjectAccessor::GetCreature(*me, LiveTriggerGUID[2]))
                            DoSummon(MOB_LIVE_TRAINEE, LiveTrigger2, 1);
                        break;
                    }
                    case MOB_LIVE_KNIGHT:
                    {
                        if (Creature *LiveTrigger3 = ObjectAccessor::GetCreature(*me, LiveTriggerGUID[3]))
                            DoSummon(MOB_LIVE_KNIGHT, LiveTrigger3, 1);
                        if (Creature *LiveTrigger5 = ObjectAccessor::GetCreature(*me, LiveTriggerGUID[5]))
                            DoSummon(MOB_LIVE_KNIGHT, LiveTrigger5, 1);
                        break;
                    }
                    case MOB_LIVE_RIDER:
                    {
                        if (Creature *LiveTrigger4 = ObjectAccessor::GetCreature(*me, LiveTriggerGUID[4]))
                            DoSummon(MOB_LIVE_RIDER, LiveTrigger4, 1);
                        break;
                    }
                }
            }
            else
            {
                switch(entry)
                {
                    case MOB_LIVE_TRAINEE:
                    {
                        if (Creature *LiveTrigger0 = ObjectAccessor::GetCreature(*me, LiveTriggerGUID[4]))
                            DoSummon(MOB_LIVE_TRAINEE, LiveTrigger0, 1);
                        if (Creature *LiveTrigger1 = ObjectAccessor::GetCreature(*me, LiveTriggerGUID[4]))
                            DoSummon(MOB_LIVE_TRAINEE, LiveTrigger1, 1);
                        break;
                    }
                    case MOB_LIVE_KNIGHT:
                    {
                        if (Creature *LiveTrigger5 = ObjectAccessor::GetCreature(*me, LiveTriggerGUID[4]))
                            DoSummon(MOB_LIVE_KNIGHT, LiveTrigger5, 1);
                        break;
                    }
                    case MOB_LIVE_RIDER:
                    {
                        if (Creature *LiveTrigger4 = ObjectAccessor::GetCreature(*me, LiveTriggerGUID[4]))
                            DoSummon(MOB_LIVE_RIDER, LiveTrigger4, 1);
                        break;
                    }
                }
            }
        }

        bool CheckGroupSplitted()
        {
            Map* pMap = me->GetMap();
            if (pMap && pMap->IsDungeon())
            {
                bool checklife = false;
                bool checkdead = false;
                for (Player& player : pMap->GetPlayers())
                {
                    if (player.IsAlive() &&
                        player.GetPositionX() <= POS_X_NORTH &&
                        player.GetPositionX() >= POS_X_SOUTH &&
                        player.GetPositionY() <= POS_Y_GATE &&
                        player.GetPositionY() >= POS_Y_EAST)
                        {
                            checklife = true;
                        }
                    else if (player.IsAlive() &&
                            player.GetPositionX() <= POS_X_NORTH &&
                            player.GetPositionX() >= POS_X_SOUTH &&
                            player.GetPositionY() >= POS_Y_GATE &&
                            player.GetPositionY() <= POS_Y_WEST)
                        {
                            checkdead = true;
                        }

                        if (checklife && checkdead)
                            return true;
                }
            }

            return false;
        }

        void SpellHit(Unit * /*caster*/, const SpellInfo *spell)
        {
            uint32 spellId = 0;
            switch(spell->Id)
            {
                case SPELL_INFORM_LIVE_TRAINEE: spellId = SPELL_INFORM_DEAD_TRAINEE;    break;
                case SPELL_INFORM_LIVE_KNIGHT:  spellId = SPELL_INFORM_DEAD_KNIGHT;     break;
                case SPELL_INFORM_LIVE_RIDER:   spellId = SPELL_INFORM_DEAD_RIDER;      break;

            }
            if (spellId && me->IsInCombat())
            {
                me->HandleEmoteCommand(EMOTE_ONESHOT_SPELL_CAST);
                if (Creature *pRandomDeadTrigger = ObjectAccessor::GetCreature(*me, DeadTriggerGUID[rand32() % POS_DEAD]))
                    me->CastSpell(pRandomDeadTrigger, spellId, true);
            }
        }

        void SpellHitTarget(Unit *pTarget, const SpellInfo *spell)
        {
            if (!me->IsInCombat())
                return;

            switch(spell->Id)
            {
                case SPELL_INFORM_DEAD_TRAINEE:
                    DoSummon(MOB_DEAD_TRAINEE, pTarget, 0);
                    break;
                case SPELL_INFORM_DEAD_KNIGHT:
                    DoSummon(MOB_DEAD_KNIGHT, pTarget, 0);
                    break;
                case SPELL_INFORM_DEAD_RIDER:
                    DoSummon(MOB_DEAD_RIDER, pTarget, 1.0f);
                    DoSummon(MOB_DEAD_HORSE, pTarget, 1.0f);
                    break;
            }
        }


        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (!thirtyPercentReached && HealthBelowPct(30) && phaseTwo)
            {
                thirtyPercentReached = true;
                if (instance)
                    instance->SetData(DATA_GOTHIK_GATE, GO_STATE_ACTIVE);
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_SUMMON:
                        if (waves[waveCount].entry)
                        {
                            if ((waves[waveCount].mode == 2) && (GetDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL))
                               DoGothikSummon(waves[waveCount].entry);
                            else if ((waves[waveCount].mode == 0) && (GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL))
                                DoGothikSummon(waves[waveCount].entry);
                            else if (waves[waveCount].mode == 1)
                                DoGothikSummon(waves[waveCount].entry);

                            // if group is not splitted, open gate and merge both sides at ~ 2 minutes (wave 11)
                            if (waveCount == 11)
                            {
                                if (!CheckGroupSplitted())
                                {
                                    if (instance)
                                        instance->SetData(DATA_GOTHIK_GATE, GO_STATE_ACTIVE);
                                    DummyEntryCheckPredicate pred;
                                    summons.DoAction(0, pred);
                                    summons.DoZoneInCombat();
                                    mergedSides = true;
                                }
                            }

                            if (waves[waveCount].mode == 1)
                                events.ScheduleEvent(EVENT_SUMMON, waves[waveCount].time);
                            else if ((waves[waveCount].mode == 2) && (GetDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL))
                                events.ScheduleEvent(EVENT_SUMMON, waves[waveCount].time);
                            else if ((waves[waveCount].mode == 0) && (GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL))
                                events.ScheduleEvent(EVENT_SUMMON, waves[waveCount].time);
                            else
                                events.ScheduleEvent(EVENT_SUMMON, 0);

                            ++waveCount;
                        }
                        else
                        {
                            phaseTwo = true;
                            Talk(SAY_TELEPORT);
                            DoTeleportTo(PosGroundLiveSide);
                            me->SetReactState(REACT_AGGRESSIVE);
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_1);
                            DummyEntryCheckPredicate pred;
                            summons.DoAction(0, pred);
                            summons.DoZoneInCombat();
                            events.ScheduleEvent(EVENT_BOLT, 1000);
                            events.ScheduleEvent(EVENT_HARVEST, urand(3000, 15000));
                            events.ScheduleEvent(EVENT_TELEPORT, 20000);
                        }
                        break;
                    case EVENT_BOLT:
                        DoCastVictim(RAID_MODE(SPELL_SHADOW_BOLT, H_SPELL_SHADOW_BOLT));
                        events.ScheduleEvent(EVENT_BOLT, 3000);
                        break;
                    case EVENT_HARVEST:
                        DoCastVictim(SPELL_HARVEST_SOUL, true);
                        events.ScheduleEvent(EVENT_HARVEST, urand(20000, 25000));
                        break;
                    case EVENT_TELEPORT:
                        if (!thirtyPercentReached)
                        {
                            me->AttackStop();
                            if (IN_LIVE_SIDE(me))
                            {
                                DoTeleportTo(PosGroundDeadSide);
                            }
                            else
                            {
                                DoTeleportTo(PosGroundLiveSide);
                            }

                            me->GetThreatManager().resetAggro(NotOnSameSide(me));
                            if (Unit *pTarget = SelectTarget(SELECT_TARGET_MINDISTANCE, 0))
                            {
                                me->GetThreatManager().addThreat(pTarget, 100.0f);
                                AttackStart(pTarget);
                            }

                            events.ScheduleEvent(EVENT_TELEPORT, 20000);
                        }
                        break;
                }
            }
        }
    };

};

class npc_gothik_minion : public CreatureScript
{
public:
    npc_gothik_minion() : CreatureScript("npc_gothik_minion") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_gothik_minionAI (pCreature);
    }

    struct npc_gothik_minionAI : public CombatAI
    {
        npc_gothik_minionAI(Creature *c) : CombatAI(c)
        {
            liveSide = IN_LIVE_SIDE(me);
        }

        bool liveSide;
        bool gateClose;

        bool isOnSameSide(const Unit *pWho) const
        {
            return (liveSide == IN_LIVE_SIDE(pWho));
        }


        void DoAction(int32 param)
        {
            gateClose = param;
        }

        void DamageTaken(Unit *attacker, uint32 &damage)
        {
            if (gateClose && !isOnSameSide(attacker))
                damage = 0;
        }

        void JustDied(Unit * /*killer*/)
        {
            if (me->IsSummon())
                if (Unit* owner = me->ToTempSummon()->GetSummoner())
                    CombatAI::JustDied(owner);
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            if (!gateClose)
            {
                CombatAI::EnterEvadeMode();
                return;
            }

            if (!_EnterEvadeMode())
                return;

            Map* pMap = me->GetMap();
            if (pMap->IsDungeon())
            {
                for (Player& player : pMap->GetPlayers())
                {
                    if (player.IsAlive() && isOnSameSide(&player))
                    {
                        AttackStart(&player);
                        return;
                    }
                }
            }

            me->GetMotionMaster()->MoveIdle();
            Reset();
        }

        void UpdateAI(uint32 diff)
        {
            if (gateClose && (!isOnSameSide(me) || (me->GetVictim() && !isOnSameSide(me->GetVictim()))))
            {
                EnterEvadeMode(EVADE_REASON_OTHER);
                return;
            }

            CombatAI::UpdateAI(diff);
        }
    };

};

class spell_gothik_shadow_bolt_volley : public SpellScriptLoader
{
    public:
        spell_gothik_shadow_bolt_volley() : SpellScriptLoader("spell_gothik_shadow_bolt_volley") { }

        class spell_gothik_shadow_bolt_volley_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gothik_shadow_bolt_volley_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if(Trinity::UnitAuraCheck(false, SPELL_SHADOW_MARK));
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_gothik_shadow_bolt_volley_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gothik_shadow_bolt_volley_SpellScript();
        }
};

void AddSC_boss_gothik()
{
    new boss_gothik();
    new npc_gothik_minion();
    new spell_gothik_shadow_bolt_volley();
}
