#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "naxxramas.h"

enum Yells
{
    // Anub'Rekhan
    SAY_AGGRO       = 0,
    SAY_GREET       = 1,
    SAY_SLAY        = 2,
    EMOTE_LOCUST    = 3,

    // Crypt Guard
    EMOTE_FRENZY    = 0,
    EMOTE_SPAWN     = 1,
    EMOTE_SCARAB    = 2
};

enum Events
{
    EVENT_IMPALE = 1,
    EVENT_IMPALE_DELAYED,
    EVENT_IMPALE_TURN_BACK,
    EVENT_LOCUST,
    EVENT_LOCUST_ENDS,
    EVENT_SPAWN_GUARD,
    EVENT_SCARABS,
    EVENT_BERSERK
};

enum Phases
{
    PHASE_NORMAL = 1,
    PHASE_SWARM
};

enum Spells
{
    SPELL_IMPALE                       = 28783, // SPELL_EFFECT_DUMMY | SPELL_EFFECT_SCHOOL_DAMAGE | SPELL_EFFECT_KNOCK_BACK

    SPELL_LOCUST_SWARM                 = 28785, // + SPELL_EFFECT_APPLY_AURA | SPELL_EFFECT_APPLY_AURA
    SPELL_LOCUST_SWARM_DAMAGE          = 28786, // +-> SPELL_EFFECT_APPLY_AURA | SPELL_EFFECT_APPLY_AURA | SPELL_EFFECT_APPLY_AURA

    SPELL_SUMMON_CORPSE_SCARABS_PLAYER = 29105, // SPELL_EFFECT_SUMMON

    SPELL_SUMMON_CORPSE_SCARABS_GUARD  = 28864, // SPELL_EFFECT_SUMMON

    SPELL_BERSERK                      = 27680, // SPELL_EFFECT_APPLY_AURA | SPELL_EFFECT_APPLY_AURA | SPELL_EFFECT_APPLY_AURA
};

enum Creatures
{
    NPC_CRYPT_GUARD                    = 16573
};

enum Misc
{
    ACHIEV_TIMED_START_EVENT           = 9891,

    GROUP_INITIAL_25M                  = 1,
    GROUP_SINGLE_SPAWN                 = 2
};

struct GuardCorspeData
{
    time_t DeathTime;
    ObjectGuid Guid;
};

struct boss_anubrekhanAI : public BossAI
{
    boss_anubrekhanAI(Creature* creature) : BossAI(creature, BOSS_ANUBREKHAN) { }

    void Reset() override
    {
        BossAI::Reset();

        if (Is25ManRaid())
            me->SummonCreatureGroup(GROUP_INITIAL_25M);
        _guardCorpses.clear();
    }

    void EnterCombat(Unit* who) override
    {
        BossAI::EnterCombat(who);
        Talk(SAY_AGGRO);
        summons.DoZoneInCombat();

        events.SetPhase(PHASE_NORMAL);
        events.ScheduleEvent(EVENT_IMPALE, 10s, 20s, PHASE_NORMAL);
        events.ScheduleEvent(EVENT_SCARABS, 5s, PHASE_NORMAL);
        events.ScheduleEvent(EVENT_LOCUST, 80s, 2min);
        events.ScheduleEvent(EVENT_BERSERK, 10min);

        if (!Is25ManRaid())
            events.ScheduleEvent(EVENT_SPAWN_GUARD, 15s, 20s);
    }

    void JustDied(Unit* killer) override
    {
        BossAI::JustDied(killer);

        instance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        _EnterEvadeMode();
        _DespawnAtEvade();
    }

    void JustSummoned(Creature* summon) override
    {
        BossAI::JustSummoned(summon);

        if (summon->GetEntry() == NPC_CRYPT_GUARD)
        {
            summon->SetCorpseDelay(HOUR);
            if (me->IsInCombat())
                summon->AI()->Talk(EMOTE_SPAWN, me);
        }
    }

    void SummonedCreatureDies(Creature* summon, Unit* killer) override
    {
        BossAI::SummonedCreatureDies(summon, killer);

        if (summon->GetEntry() == NPC_CRYPT_GUARD)
            _guardCorpses.push_back({ time(nullptr), summon->GetGUID() });
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        BossAI::SummonedCreatureDespawn(summon);

        if (summon->GetEntry() == NPC_CRYPT_GUARD)
        {
            for (std::list<GuardCorspeData>::iterator itr = _guardCorpses.begin(); itr != _guardCorpses.end(); ++itr)
                if (itr->Guid == summon->GetGUID())
                {
                    _guardCorpses.erase(itr);
                    break;
                }
        }
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->GetTypeId() == TYPEID_PLAYER && roll_chance_i(50))
            victim->CastSpell(victim, SPELL_SUMMON_CORPSE_SCARABS_PLAYER, true, nullptr, nullptr, me->GetGUID());

        Talk(SAY_SLAY);
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_IMPALE:
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 45.f, true))
                {
                    if (Is25ManRaid())
                    {
                        me->SetTarget(target->GetGUID());
                        me->SetInFront(target);
                        _impaleTarget = target->GetGUID();
                        events.ScheduleEvent(EVENT_IMPALE_DELAYED, 100ms);
                    }
                    else
                        DoCast(target, SPELL_IMPALE);
                }
                events.Repeat(10s, 20s);
                break;
            case EVENT_IMPALE_DELAYED:
                events.ScheduleEvent(EVENT_IMPALE_TURN_BACK, 400ms);

                if (Unit* target = ObjectAccessor::GetUnit(*me, _impaleTarget))
                    DoCast(target, SPELL_IMPALE);
                break;
            case EVENT_IMPALE_TURN_BACK:
                if (Unit* victim = me->GetVictim())
                    me->SetTarget(victim->GetGUID());
                else
                    me->SetTarget(ObjectGuid::Empty);
                break;
            case EVENT_SCARABS:
                if (!_guardCorpses.empty() && roll_chance_i(10))
                {
                    GuidList _guards;
                    for (auto val : _guardCorpses)
                        if (val.DeathTime + 15 < time(nullptr))
                            _guards.push_back(val.Guid);

                    if (!_guards.empty())
                        if (Creature* creatureTarget = ObjectAccessor::GetCreature(*me, Trinity::Containers::SelectRandomContainerElement(_guards)))
                        {
                            creatureTarget->CastSpell(creatureTarget, SPELL_SUMMON_CORPSE_SCARABS_GUARD, true, nullptr, nullptr, me->GetGUID());
                            creatureTarget->AI()->Talk(EMOTE_SCARAB);
                            creatureTarget->DespawnOrUnsummon();
                        }
                }
                events.Repeat(5s);
                break;
            case EVENT_LOCUST:
                Talk(EMOTE_LOCUST);
                DoCast(SPELL_LOCUST_SWARM);

                events.SetPhase(PHASE_SWARM);
                events.CancelEventGroup(PHASE_NORMAL);
                events.ScheduleEvent(EVENT_SPAWN_GUARD, 3s);
                events.ScheduleEvent(EVENT_LOCUST_ENDS, RAID_MODE(16s, 20s));
                events.Repeat(70s, 2min);
                break;
            case EVENT_LOCUST_ENDS:
                events.SetPhase(PHASE_NORMAL);
                events.ScheduleEvent(EVENT_IMPALE, 10s, 20s, PHASE_NORMAL);
                events.ScheduleEvent(EVENT_SCARABS, 5s, PHASE_NORMAL);
                break;
            case EVENT_SPAWN_GUARD:
                me->SummonCreatureGroup(GROUP_SINGLE_SPAWN);
                break;
            case EVENT_BERSERK:
                DoCastSelf(SPELL_BERSERK);
                break;
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
            ExecuteEvent(eventId);

        if (events.IsInPhase(PHASE_NORMAL))
            DoMeleeAttackIfReady();
    }

private:
    std::list<GuardCorspeData> _guardCorpses;
    ObjectGuid _impaleTarget; // only 25 man
};

class at_anubrekhan_entrance : public OnlyOnceAreaTriggerScript
{
public:
    at_anubrekhan_entrance() : OnlyOnceAreaTriggerScript("at_anubrekhan_entrance") { }

    bool _OnTrigger(Player* player, const AreaTriggerEntry* /*areaTrigger*/) override
    {
        InstanceScript* instance = player->GetInstanceScript();
        if (!instance || instance->GetBossState(BOSS_ANUBREKHAN) != NOT_STARTED)
            return true;

        if (Creature* anub = ObjectAccessor::GetCreature(*player, instance->GetGuidData(DATA_ANUBREKHAN)))
            anub->AI()->Talk(SAY_GREET);

        return true;
    }
};

void AddSC_boss_anubrekhan()
{
    new CreatureScriptLoaderEx<boss_anubrekhanAI>("boss_anubrekhan"); // 15956, 29249

    new at_anubrekhan_entrance(); // 4119
}
