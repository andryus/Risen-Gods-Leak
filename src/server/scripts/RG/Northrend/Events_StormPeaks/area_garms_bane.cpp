#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellInfo.h"
#include "SpellScript.h"

enum GarmsBaneMisc
{
    NPC_SNOWBLIND_FOLLOWER  = 29618,
    NPC_GARM_INVADER        = 29619,
    NPC_IMPROVED_LAND_MINE  = 29475,

    EVENT_SPAWN_NEW_WAVE    = 1,
    EVENT_START_PATH        = 2,
    EVENT_RANDOM_MOVEMENT   = 3,

    // Quest "Gormok Wants His Snobolds" stuff
    SAY_THROW_NET           = 0,
    QUEST_KILL_CREDIT       = 34899,
    SPELL_THROW_NET         = 66474,
};

uint32 path_id[] { 31115200,31115210,31115220,31115230,31115240,31115250,31115260,31115270,31115280,31115290,31115300,31115310,31115340,31115350,31115320,31115330 };

const Position SpawnWavePosition[16] =
{
    // NPC_SNOWBLIND_FOLLOWER
    { 6313.563f, -1407.678f, 425.313f, 1.663f }, // 0  Grp 1
    { 6317.369f, -1406.111f, 424.980f, 1.663f }, // 1  Grp 1
    { 6321.471f, -1406.988f, 424.962f, 1.663f }, // 2  Grp 1
    { 6340.827f, -1402.212f, 426.215f, 1.663f }, // 3  Grp 1
    { 6345.709f, -1405.566f, 426.676f, 1.663f }, // 4  Grp 1
    { 6348.994f, -1402.905f, 426.711f, 1.663f }, // 5  Grp 1

    { 6313.695f, -1420.125f, 426.549f, 1.663f }, // 6  Grp 2
    { 6318.068f, -1422.423f, 426.561f, 1.663f }, // 7  Grp 2
    { 6322.502f, -1420.909f, 426.248f, 1.663f }, // 8  Grp 2
    { 6342.580f, -1416.413f, 426.475f, 1.663f }, // 9  Grp 2
    { 6345.939f, -1412.825f, 426.811f, 1.663f }, // 10 Grp 2
    { 6350.982f, -1413.323f, 427.362f, 1.663f }, // 11 Grp 2

    // NPC_GARM_INVADER
    { 6329.399f, -1422.37f, 426.016f, 1.663f }, // 12 Grp 1
    { 6337.370f, -1420.68f, 426.086f, 1.663f }, // 13 Grp 1
    { 6327.521f, -1407.25f, 425.045f, 1.663f }, // 14 Grp 2
    { 6335.230f, -1405.97f, 425.678f, 1.663f }, // 15 Grp 2
};

const Position SpawnLandminePosition[9] =
{
    { 6311.70f, -1351.14f, 427.794f, 5.60299f },
    { 6311.61f, -1303.69f, 428.651f, 3.85547f },
    { 6334.09f, -1345.80f, 427.993f, 4.57803f },
    { 6331.79f, -1199.55f, 426.764f, 1.82676f },
    { 6342.13f, -1209.97f, 427.626f, 1.30211f },
    { 6295.59f, -1226.30f, 426.088f, 4.96835f },
    { 6247.54f, -1258.50f, 428.332f, 2.09536f },
    { 6348.27f, -1264.86f, 428.689f, 3.60415f },
    { 6332.76f, -1238.31f, 427.644f, 1.51652f },
};

struct npc_garms_bane_runnerAI : public ScriptedAI
{
    npc_garms_bane_runnerAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        me->SetReactState(REACT_PASSIVE);
        me->setActive(true);
    }

    void SendWave(uint32 group, uint32 path)
    {
        _events.Reset();
        _lastWP = 0;

        if (me->GetEntry() == NPC_SNOWBLIND_FOLLOWER)
        {
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MoveRandom(7.0f);
        }

        if (path)
            _pathId = path;

        if (group == 1)
            _events.ScheduleEvent(EVENT_START_PATH, Seconds(1));
        else if (group == 2)
            _events.ScheduleEvent(EVENT_START_PATH, Seconds(5));
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type != WAYPOINT_MOTION_TYPE)
            return;

        if (id == _lastWP)
        {
            me->SetWalk(true);
            me->SetHomePosition(me->GetPosition());
            _events.ScheduleEvent(EVENT_RANDOM_MOVEMENT, 500);
        }
    }

    void UpdateAI(uint32 diff) override 
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_START_PATH:
                    me->GetMotionMaster()->MovePath(_pathId, false);
                    me->SetSpeedRate(MOVE_RUN, frand(1.19048f, 1.3f));
                    if (WaypointPath const* path = sWaypointMgr->GetPath(_pathId))
                        _lastWP = path->size() - 1;
                    break;
                case EVENT_RANDOM_MOVEMENT:
                    me->GetMotionMaster()->MoveRandom(10.0f);
                    break;
                default:
                    break;
            }
        }
    }

    void SpellHit(Unit* caster, const SpellInfo* spellInfo) override
    {
        if (caster && spellInfo)
        {
            if (spellInfo->Id == SPELL_THROW_NET && me->GetEntry() == NPC_SNOWBLIND_FOLLOWER)
            {
                if (Player* player = caster->ToPlayer())
                    player->KilledMonsterCredit(QUEST_KILL_CREDIT);

                Talk(SAY_THROW_NET);
                me->DespawnOrUnsummon(5 * IN_MILLISECONDS);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            }
        }
    }

    private:
        EventMap    _events;
        uint8       _lastWP;
        uint32      _pathId;
};

class spell_garms_bane_energy_surge_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_garms_bane_energy_surge_SpellScript);

    SpellCastResult CheckRequirement()
    {
        if (Unit* caster = GetCaster())
            if (!caster->FindNearestCreature(NPC_SNOWBLIND_FOLLOWER, 20.0f) && !caster->FindNearestCreature(NPC_GARM_INVADER, 20.0f))
                return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_garms_bane_energy_surge_SpellScript::CheckRequirement);
    }
};

struct npc_garms_bane_main_controllerAI : public ScriptedAI
{
    npc_garms_bane_main_controllerAI(Creature* creature) : ScriptedAI(creature) { SpawnLandmines(3); }

    void Reset() override
    {
        me->setActive(true);

        _events.Reset();
        _events.ScheduleEvent(EVENT_SPAWN_NEW_WAVE, Seconds(1));
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        if (summon && summon->GetEntry() == NPC_IMPROVED_LAND_MINE)
            SpawnLandmines(1);
    }

    void SpawnLandmines(uint8 amount)
    {
        for (uint8 i = 0; i < amount; ++i)
        {
            uint8 y = urand(0, 8);
            me->SummonCreature(NPC_IMPROVED_LAND_MINE, SpawnLandminePosition[y], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 1 * IN_MILLISECONDS);
        }
    }

    void SpawnWaves()
    {
        uint8 i = 0;

        // group 1
        for (i = 0; i < 6; ++i)
            if (Creature* add = me->SummonCreature(NPC_SNOWBLIND_FOLLOWER, SpawnWavePosition[i], TEMPSUMMON_TIMED_DESPAWN, 180 * IN_MILLISECONDS))
                ENSURE_AI(npc_garms_bane_runnerAI, add->AI())->SendWave(1, path_id[i]);

        for (i = 14; i < 16; ++i)
            if (Creature* add = me->SummonCreature(NPC_GARM_INVADER, SpawnWavePosition[i], TEMPSUMMON_TIMED_DESPAWN, 180 * IN_MILLISECONDS))
                ENSURE_AI(npc_garms_bane_runnerAI, add->AI())->SendWave(1, path_id[i]);

        // group 2
        for (i = 6; i < 12; ++i)
            if (Creature* add = me->SummonCreature(NPC_SNOWBLIND_FOLLOWER, SpawnWavePosition[i], TEMPSUMMON_TIMED_DESPAWN, 180 * IN_MILLISECONDS))
                ENSURE_AI(npc_garms_bane_runnerAI, add->AI())->SendWave(2, path_id[i]);

        for (i = 12; i < 14; ++i)
            if (Creature* add = me->SummonCreature(NPC_GARM_INVADER, SpawnWavePosition[i], TEMPSUMMON_TIMED_DESPAWN, 180 * IN_MILLISECONDS))
                ENSURE_AI(npc_garms_bane_runnerAI, add->AI())->SendWave(2, path_id[i]);
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SPAWN_NEW_WAVE:
                    SpawnWaves();
                    _events.ScheduleEvent(EVENT_SPAWN_NEW_WAVE, Seconds(15));
                    break;
                default:
                    break;
            }
        }
    }

private:
    EventMap    _events;
};

void AddSC_area_garms_bane_rg()
{
    new CreatureScriptLoaderEx<npc_garms_bane_main_controllerAI>("npc_garms_bane_main_controller");
    new CreatureScriptLoaderEx<npc_garms_bane_runnerAI>("npc_garms_bane_runnerAI");
    new SpellScriptLoaderEx<spell_garms_bane_energy_surge_SpellScript>("spell_garms_bane_energy_surge");
}
