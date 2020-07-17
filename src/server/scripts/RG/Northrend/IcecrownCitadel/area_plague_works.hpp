#pragma once

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "icecrown_citadel.h"

enum DummyEvents
{
    // Festergut
    EVENT_FESTERGUT_GOO = 1,

    // Rotface
    EVENT_ROTFACE_OOZE_FLOOD,

    EVENT_HANDLE_EMOTE,
};

enum DummyYells
{
    // Festergut
    SAY_FESTERGUT_GASEOUS_BLIGHT = 0,

    // Rotface
    SAY_ROTFACE_OOZE_FLOOD = 2,

    SAY_KILL_PROF = 11,
};

enum DummySpells
{
    // Festergut
    SPELL_RELEASE_GAS_VISUAL                = 69125,
    SPELL_GASEOUS_BLIGHT_LARGE              = 69157,
    SPELL_MALLABLE_GOO_H                    = 72296,

    // Rotface
    SPELL_OOZE_FLOOD                        = 69782,
};

enum DummyPoints
{
    POINT_FESTERGUT = 1,
    POINT_ROTFACE   = 2
};

constexpr Position festergutWatchPos = { 4324.820f, 3166.03f, 389.3831f, 3.316126f }; //emote 432 (release gas)
constexpr Position rotfaceWatchPos = { 4390.371f, 3164.50f, 389.3890f, 5.497787f }; //emote 432 (release ooze)

inline auto HeightAndRotfaceDistCheck(Creature* rotface)
{
    return [rotface](Unit* trigger)
    {
        return trigger->GetPositionZ() < rotface->GetPositionZ() + 5.0f ||
            trigger->GetDistance(rotface->GetHomePosition()) > 55.0f;
    };
};

inline auto AlreadySelectedNear(Creature* searcher, std::vector<ObjectGuid>& selected)
{
    return [searcher, &selected](Unit* trigger)
    {
        for (auto possible : selected)
        {
            auto possibleNearTrigger = searcher->GetMap()->GetCreature(possible);
            return possibleNearTrigger->GetDistance(trigger) < 30;
        }
        return false;
    };
};

inline void BossDistanceCheck(Unit* boss, std::list<WorldObject*>& targets)
{
    float highestCombatReach = 0.f;
    for (auto target : targets)
    {
        highestCombatReach = std::max(highestCombatReach, target->GetObjectSize());
    }

    bool ranges = false;
    float range = std::max(8.f, boss->GetObjectSize() + highestCombatReach);
    uint8 count = boss->GetMap()->Is25ManRaid() ? 8 : 3;

    targets.sort(Trinity::ObjectDistanceOrderPred(boss, false));

    if (targets.size() > count)
    {
        uint8 itrCount = 0;
        for (WorldObject* target : targets)
        {
            ++itrCount;
            if (itrCount > count)
            {
                ranges = true;
                break;
            }

            if (boss->GetExactDist(target) < range)
                break;
        }
    }

    if (ranges)
        targets.remove_if([boss, range](WorldObject* target) { return target->GetExactDist(boss) < range; });
};

struct boss_professor_putricide_dummyAI : public ScriptedAI
{
    boss_professor_putricide_dummyAI(Creature* creature) : ScriptedAI(creature)
    {
        instance = creature->GetInstanceScript();
    }

    void StartEncounter()
    {
        events.Reset();
        me->setActive(true);
        me->SetWalk(false);
        me->SetImmuneToPC(true);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        events.ScheduleEvent(EVENT_HANDLE_EMOTE, 5s);
    }

    void KilledUnit(Unit* victim)
    {
        if (victim->GetTypeId() == TYPEID_PLAYER)
            Talk(SAY_KILL_PROF);
    }

    void MovementInform(uint32 type, uint32 id)
    {
        if (type != POINT_MOTION_TYPE)
            return;

        switch (id)
        {
            case POINT_FESTERGUT:
                me->SetSpeedRate(MOVE_RUN, 2.0f);
                DoAction(ACTION_FESTERGUT_GAS);
                me->HandleEmoteCommand(EMOTE_ONESHOT_USE_STANDING);
                if (Creature* festergut = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_FESTERGUT)))
                    festergut->CastSpell(festergut, SPELL_GASEOUS_BLIGHT_LARGE, false, NULL, NULL, festergut->GetGUID());
                break;
            case POINT_ROTFACE:
                me->SetSpeedRate(MOVE_RUN, 2.0f);
                me->HandleEmoteCommand(EMOTE_ONESHOT_USE_STANDING);
                break;
            default:
                break;
        }
    }

    void DoAction(int32 action)
    {
        switch (action)
        {
            case ACTION_FESTERGUT_COMBAT:
                StartEncounter();
                me->SetSpeedRate(MOVE_RUN, 4.0f);
                me->GetMotionMaster()->MovePoint(POINT_FESTERGUT, festergutWatchPos);
                me->SetReactState(REACT_PASSIVE);
                DoZoneInCombat();
                if (IsHeroic())
                    events.ScheduleEvent(EVENT_FESTERGUT_GOO, 15s, 17s);
                break;
            case ACTION_FESTERGUT_GAS:
                Talk(SAY_FESTERGUT_GASEOUS_BLIGHT);
                DoCastSelf(SPELL_RELEASE_GAS_VISUAL, true);
                break;
            case ACTION_ROTFACE_COMBAT:
                StartEncounter();
                me->SetSpeedRate(MOVE_RUN, 4.0f);
                me->GetMotionMaster()->MovePoint(POINT_ROTFACE, rotfaceWatchPos);
                me->SetReactState(REACT_PASSIVE);
                DoZoneInCombat();
                oozeFloodDummys.clear();
                events.ScheduleEvent(EVENT_ROTFACE_OOZE_FLOOD, 25s, 30s);
                break;
            case ACTION_ROTFACE_OOZE:
                Talk(SAY_ROTFACE_OOZE_FLOOD);

                // Find suitable new ooze flood trigger
                if (Creature* rotface = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_ROTFACE)))
                {
                    // Get all possible ooze flood triggers in a large range to detect them all
                    std::list<Creature*> list;
                    rotface->GetCreatureListWithEntryInGrid(list, NPC_PUDDLE_STALKER, 150.0f);

                    // Avoid finding ooze flood trigger in the wrong height
                    // Avoid finding ooze flood trigger in too much distance
                    list.remove_if(HeightAndRotfaceDistCheck(rotface));

                    // remove ooze near by already selected ooze
                    list.remove_if(AlreadySelectedNear(rotface, oozeFloodDummys));

                    // Pick one random ooze flood trigger
                    if (!list.empty())
                    {
                        auto primary = Trinity::Containers::SelectRandomContainerElement(list);

                        oozeFloodDummys.push_back(primary->GetGUID());
                        if (oozeFloodDummys.size() >= 4)
                            oozeFloodDummys.clear();

                        list.remove(primary);
                        list.sort(Trinity::ObjectDistanceOrderPred(primary));

                        if (!list.empty() && list.front()->GetDistance2d(primary) < 30)
                        {
                            // cast from self for LoS (with prof's GUID for logs)
                            primary->CastSpell(primary, SPELL_OOZE_FLOOD, true, nullptr, nullptr, me->GetGUID());
                            list.front()->CastSpell(list.front(), SPELL_OOZE_FLOOD, true, nullptr, nullptr, me->GetGUID());
                        }
                        else
                        {
                            TC_LOG_ERROR("scripts", "Rotface: no secondary ooze found");
                        }
                    }
                }
                break;
            default:
                break;
        }
    }

    void UpdateAI(uint32 diff)
    {
        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_FESTERGUT_GOO:
                    if (Creature* festergut = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_FESTERGUT)))
                        if (Unit* target = festergut->AI()->SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true, false))
                            DoCast(target, SPELL_MALLABLE_GOO_H, false);

                    Is25ManRaid() ? events.Repeat(9s, 11s) : events.Repeat(15s, 17s);
                    break;
                case EVENT_ROTFACE_OOZE_FLOOD:
                    DoAction(ACTION_ROTFACE_OOZE);
                    events.Repeat(20s);
                    break;
                case EVENT_HANDLE_EMOTE:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_USE_STANDING);
                    events.Repeat(1s);
                    break;
                default:
                    break;
            }
        }
        // never Melee!
    }
private:
    EventMap events;
    InstanceScript* instance;
    std::vector<ObjectGuid> oozeFloodDummys;
};

enum Phases : uint32;

struct boss_professor_putricideAI : public BossAI
{
    boss_professor_putricideAI(Creature* creature);

    void Reset();

    void MoveInLineOfSight(Unit* who);

    void EnterCombat(Unit* who);

    void EnterEvadeMode(EvadeReason /*why*/);

    void KilledUnit(Unit* victim);

    void JustDied(Unit* killer);

    void JustSummoned(Creature* summon);

    void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/);

    void MovementInform(uint32 type, uint32 id);

    void StopOozeGrow();

    void DoAction(int32 action);

    uint32 GetData(uint32 type) const;

    void SetData(uint32 id, uint32 data);

    void UpdateAI(uint32 diff);

private:
    void SetPhase(Phases newPhase);

    Phases _phase;          // external of EventMap because event phase gets reset on evade
    float const _baseSpeed;
    bool _experimentState;
    uint8 VaribaleCounter;
};
