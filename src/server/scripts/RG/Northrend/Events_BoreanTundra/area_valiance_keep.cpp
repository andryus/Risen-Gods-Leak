#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"

// Valiance Keep recruiting event
enum EventStuff
{
    NPC_MAIN_TRIGGER        = 1010907,
    NPC_CIVILIAN_RECRUIT    = 25220,
    NPC_RECRUIT_OFFICER     = 25222,
    NPC_QUEUE_TRIGGER       = 15800,

    EVENT_TIMER             = 1,

    SAY_CIVIL               = 0,
    SAY_OFFI_NEXT           = 3,
    SAY_OFFI_ASK_PROF       = 0,
    SAY_OFFI_CIVIL          = 1,
    SAY_OFFI_SOLDIER        = 2,
};

const Position PosSpecial[4] =
{
    // Spawn
    { 2254.100f, 5196.359f, 11.468f },
    // Talk
    { 2297.262f, 5241.838f, 11.387f },

    // End of Path
    { 2320.83f, 5259.26f, 11.2558f },
    { 2125.46f, 5331.81f, 24.6419f },
};

const Position PosTrigger[9] =
{
    // Spawn
    { 2277.82f, 5238.72f, 11.451f, 1.06f },
    { 2279.22f, 5241.41f, 11.451f, 1.08f },
    { 2280.84f, 5244.22f, 11.457f, 0.17f },
    { 2282.6f , 5245.74f, 11.363f, 5.86f },
    { 2284.87f, 5246.3f , 11.451f, 5.93f },
    { 2287.47f, 5245.93f, 11.451f, 5.86f },
    { 2289.47f, 5244.9f , 11.451f, 5.86f },
    { 2291.77f, 5243.93f, 11.451f, 5.89f },
    { 2294.13f, 5242.71f, 11.451f, 5.89f },
};

struct npc_borean_queue_civil_recruitAI : public ScriptedAI
{
    npc_borean_queue_civil_recruitAI(Creature* creature) : ScriptedAI(creature) { _myStatus = 0; }

    void InitializeAI() override
    {
        me->setActive(true);
    }

    void DoAction(int32 action) override
    {
        _myStatus = _myStatus + action;
        DoSomething();
    }

    void SetData(uint32 type, uint32 /*data*/) override
    {
        _myStatus = type;
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (who && who->ToCreature() && who->ToCreature()->GetEntry() == NPC_QUEUE_TRIGGER)
            if (who->GetDistance(me) <= 3.0f)
            {
                me->StopMoving();
                me->DespawnOrUnsummon(3 * IN_MILLISECONDS);
            }
    }

    void DoSomething()
    {
        switch (_myStatus)
        {
            case 101:
                me->GetMotionMaster()->MovePoint(1, PosTrigger[1]);
                break;
            case 102:
                me->GetMotionMaster()->MovePoint(1, PosTrigger[2]);
                break;
            case 103:
                me->GetMotionMaster()->MovePoint(1, PosTrigger[3]);
                break;
            case 104:
                me->GetMotionMaster()->MovePoint(1, PosTrigger[4]);
                break;
            case 105:
                me->GetMotionMaster()->MovePoint(1, PosTrigger[5]);
                break;
            case 106:
                me->GetMotionMaster()->MovePoint(1, PosTrigger[6]);
                break;
            case 107:
                me->GetMotionMaster()->MovePoint(1, PosTrigger[7]);
                break;
            case 108:
                me->GetMotionMaster()->MovePoint(1, PosTrigger[8]);
                break;
            case 109:
                me->GetMotionMaster()->MovePoint(1, PosSpecial[1]);
                if (Creature* offi = me->FindNearestCreature(NPC_MAIN_TRIGGER, 50.0f, true))
                    offi->AI()->SetGuidData(me->GetGUID(), 1);
                break;
            default:
                break;
        }
    }

    private:
        uint32  _myStatus;
};

struct npc_borean_queue_main_triggerAI : public ScriptedAI
{
    npc_borean_queue_main_triggerAI(Creature* creature) : ScriptedAI(creature) { }

    void InitializeAI() override
    {
        me->setActive(true);
        Reset();
        PrepareEvent();
    }

    void Reset() override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_TIMER, Seconds(10));

        _stepCount = 0;
    }

    void PrepareEvent()
    {
        y = 100;
        for (i = 0; i < 9; ++i)
            if (Creature* recruit = DoSummon(NPC_CIVILIAN_RECRUIT, PosTrigger[i]))
            {
                recruit->SetWalk(true);
                recruit->AI()->DoAction(y);
                y = y + 1;
                _recruitGUID[i] = recruit->GetGUID();
            }

        for (i = 2; i < 4; ++i)
            DoSummon(NPC_QUEUE_TRIGGER, PosSpecial[i]);
    }

    void StartMovement()
    {
        for (uint32 i = 0; i < 18; ++i)
            if (Creature* recruit = ObjectAccessor::GetCreature(*me, _recruitGUID[i]))
                recruit->AI()->DoAction(1);
    }

    void SetGuidData(ObjectGuid guid, uint32 dataId) override
    {
        if (dataId == 1)
        {
            _talkRecruit = guid;

            for (i = 0; i < 18; ++i)
            {
                if (Creature* recruit = ObjectAccessor::GetCreature(*me, _recruitGUID[i]))
                {
                    if (recruit->GetGUID() == guid)
                    {
                        if (Creature* recruit = DoSummon(NPC_CIVILIAN_RECRUIT, PosSpecial[0]))
                        {
                            recruit->SetWalk(true);
                            recruit->LoadEquipment(1, true);
                            recruit->GetMotionMaster()->MovePoint(1, PosTrigger[0]);
                            recruit->SetVisible(true);
                            recruit->AI()->SetData(100, 1);
                            _recruitGUID[i] = recruit->GetGUID();
                        }
                    }
                }
            }
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_TIMER:
                    switch (_stepCount)
                    {
                        case 0:
                            if (Creature* offi = me->FindNearestCreature(NPC_RECRUIT_OFFICER, 50.0f, true))
                                offi->AI()->Talk(SAY_OFFI_NEXT);
                            ++_stepCount;
                            _events.ScheduleEvent(EVENT_TIMER, Seconds(3));
                            break;
                        case 1:
                            StartMovement();
                            ++_stepCount;
                            _events.ScheduleEvent(EVENT_TIMER, Seconds(8));
                            break;
                        case 2:
                            if (Creature* offi = me->FindNearestCreature(NPC_RECRUIT_OFFICER, 50.0f, true))
                                offi->AI()->Talk(SAY_OFFI_ASK_PROF);
                            ++_stepCount;
                            _events.ScheduleEvent(EVENT_TIMER, Seconds(5));
                            break;
                        case 3:
                            if (Creature* recruit = ObjectAccessor::GetCreature(*me, _talkRecruit))
                                recruit->AI()->Talk(SAY_CIVIL);
                            ++_stepCount;
                            _events.ScheduleEvent(EVENT_TIMER, Seconds(5));
                            break;
                        case 4:
                            _profession = urand(1, 10);

                            if (_profession < 6)
                            {
                                if (Creature* offi = me->FindNearestCreature(NPC_RECRUIT_OFFICER, 50.0f, true))
                                    offi->AI()->Talk(SAY_OFFI_CIVIL);
                            }
                            else
                            {
                                if (Creature* offi = me->FindNearestCreature(NPC_RECRUIT_OFFICER, 50.0f, true))
                                    offi->AI()->Talk(SAY_OFFI_SOLDIER);
                            }

                            ++_stepCount;
                            _events.ScheduleEvent(EVENT_TIMER, Seconds(5));
                            break;
                        case 5:
                            if (_profession > 5)
                                if (Creature* recruit = ObjectAccessor::GetCreature(*me, _talkRecruit))
                                    recruit->LoadEquipment(2, true);

                            _events.ScheduleEvent(EVENT_TIMER, Seconds(5));
                            ++_stepCount;
                            break;
                        case 6:
                            if (Creature* recruit = ObjectAccessor::GetCreature(*me, _talkRecruit))
                                recruit->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);

                            _events.ScheduleEvent(EVENT_TIMER, Seconds(2));
                            ++_stepCount;
                            break;
                        case 7:
                            if (Creature* recruit = ObjectAccessor::GetCreature(*me, _talkRecruit))
                            {
                                if (_profession < 6)
                                {
                                    recruit->GetMotionMaster()->MovePath(recruit->GetEntry(), false);
                                    recruit->AI()->SetData(1, 0);
                                }
                                        
                                else
                                {
                                    recruit->GetMotionMaster()->MovePath(me->GetEntry(), false);
                                    recruit->AI()->SetData(2, 0);
                                }
                            }
                            _events.ScheduleEvent(EVENT_TIMER, Seconds(5));
                            _stepCount = 0;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }

            if (!UpdateVictim())
                return;
        }
    }
    private:
        EventMap    _events;
        uint8       _stepCount;
        uint8       _profession;
        ObjectGuid  _recruitGUID[18];
        ObjectGuid  _talkRecruit;
        uint32      i;
        uint32      y;
};

// Lets have a last drink event
enum LastDrink
{
    NPC_ROB         = 25258,
    NPC_GEORGE      = 25259,
    NPC_CHUCK       = 25261,
    NPC_MITCH       = 25260,
    NPC_BARKEEPER   = 25245,

    SAY_0           = 0,
    SAY_1           = 1,
    SAY_2           = 2,
    SAY_3           = 3,

    EVENT_ESCORT    = 1,
    EVENT_GET_DRUNK = 2,
};

const Position PosBros[1] = { { 2255.77f, 5186.26f, 11.4391f, 1.47479f }, };

struct npc_borean_footman_robAI : public npc_escortAI
{
    npc_borean_footman_robAI(Creature* creature) : npc_escortAI(creature) { }

    void InitializeAI() override
    {
        if (npc_escortAI* pEscortAI = CAST_AI(npc_borean_footman_robAI, me->AI()))
            pEscortAI->Start(false, false);
        Reset();
    }

    void Reset() override
    {
        _stepCount = 0;
        _getDrunK  = false;
    }

    void FindBros()
    {
        if (Creature* bro1 = me->FindNearestCreature(NPC_GEORGE, 15.0f))
        {
            _georgGUID = bro1->GetGUID();
            bro1->setActive(true);
        }
        if (Creature* bro2 = me->FindNearestCreature(NPC_CHUCK, 15.0f))
        {
            _chuckGUID = bro2->GetGUID();
            bro2->setActive(true);
        }
    }

    void WaypointReached(uint32 point) override
    {
        switch (point)
        {
            case 1:
                FindBros();
                break;
            case 11:
                SetEscortPaused(true);
                _events.ScheduleEvent(EVENT_ESCORT, Seconds(4));
                break;
            case 17:
                _events.ScheduleEvent(EVENT_ESCORT, Seconds(4));
                SetEscortPaused(true);
                break;
            case 18:
                _events.ScheduleEvent(EVENT_ESCORT, Seconds(4));
                SetEscortPaused(true);
                break;
            case 23:
                DoSummon(NPC_MITCH, PosBros[0]);
                break;
            case 27:
                if (Creature* bro1 = ObjectAccessor::GetCreature(*me, _chuckGUID))
                    if (Creature* bro2 = ObjectAccessor::GetCreature(*me, _georgGUID))
                    {
                        bro1->DespawnOrUnsummon(1 * IN_MILLISECONDS);
                        bro2->DespawnOrUnsummon(1 * IN_MILLISECONDS);
                        me->DespawnOrUnsummon(1 * IN_MILLISECONDS);
                    }
                SetEscortPaused(true);
                break;
            default:
                break;
        }
    }

    void UpdateAI(uint32 diff) override
    {
        npc_escortAI::UpdateAI(diff);

        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_ESCORT:
                    switch (_stepCount)
                    {
                        case 0:
                            // Shall we go into this strip club? Could be our last night!
                            Talk(SAY_0);
                            _events.ScheduleEvent(EVENT_ESCORT, Seconds(5));
                            ++_stepCount;
                            break;
                        case 1:
                            // Of course!
                            if (Creature* bro = ObjectAccessor::GetCreature(*me, _georgGUID))
                                bro->AI()->Talk(SAY_0);
                            _events.ScheduleEvent(EVENT_ESCORT, Seconds(5));
                            ++_stepCount;
                            break;
                        case 2:
                            SetEscortPaused(false);
                            ++_stepCount;
                            break;
                        case 3:
                            Talk(SAY_1);
                            _events.ScheduleEvent(EVENT_ESCORT, Seconds(5));
                            ++_stepCount;
                            break;
                        case 4:
                            if (Creature* barkeeper = me->FindNearestCreature(NPC_BARKEEPER, 15.0f))
                            {
                                _jamesGUID = barkeeper->GetGUID();
                                barkeeper->AI()->Talk(SAY_0);
                            }
                            _events.ScheduleEvent(EVENT_ESCORT, Seconds(5));
                            ++_stepCount;
                            break;
                        case 5:
                            Talk(SAY_2);
                            _events.ScheduleEvent(EVENT_ESCORT, Seconds(5));
                            ++_stepCount;
                            break;
                        case 6:
                            if (Creature* barkeeper = ObjectAccessor::GetCreature(*me, _jamesGUID))
                                barkeeper->AI()->Talk(SAY_1);
                            _events.ScheduleEvent(EVENT_ESCORT, Seconds(5));
                            ++_stepCount;
                            break;
                        case 7:
                            if (Creature* barkeeper = ObjectAccessor::GetCreature(*me, _jamesGUID))
                            {
                                barkeeper->HandleEmoteCommand(EMOTE_ONESHOT_POINT_NO_SHEATHE);
                                barkeeper->LoadEquipment(2, true);
                            }
                            _events.ScheduleEvent(EVENT_ESCORT, Seconds(3));
                            ++_stepCount;
                            break;
                        case 8:
                            if (Creature* barkeeper = ObjectAccessor::GetCreature(*me, _jamesGUID))
                                barkeeper->LoadEquipment(1, true);
                            _events.ScheduleEvent(EVENT_ESCORT, Seconds(3));
                            ++_stepCount;
                            break;
                        case 9:
                            if (Creature* bro1 = ObjectAccessor::GetCreature(*me, _chuckGUID))
                                if (Creature* bro2 = ObjectAccessor::GetCreature(*me, _georgGUID))
                                {
                                    me->LoadEquipment(2, true);
                                    bro1->LoadEquipment(2, true);
                                    bro2->LoadEquipment(2, true);
                                }
                            _events.ScheduleEvent(EVENT_ESCORT, Seconds(3));
                            ++_stepCount;
                            break;
                        case 10:
                            _getDrunK = true;
                            _events.ScheduleEvent(EVENT_GET_DRUNK, Seconds(1));
                            _events.ScheduleEvent(EVENT_ESCORT, Seconds(22));
                            ++_stepCount;
                            break;
                        case 11:
                            if (Creature* bro1 = ObjectAccessor::GetCreature(*me, _chuckGUID))
                                if (Creature* bro2 = ObjectAccessor::GetCreature(*me, _georgGUID))
                                {
                                    bro1->HandleEmoteCommand(EMOTE_ONESHOT_NONE);
                                    bro2->HandleEmoteCommand(EMOTE_ONESHOT_NONE);
                                    me->HandleEmoteCommand(EMOTE_ONESHOT_NONE);
                                    me->LoadEquipment(1, true);
                                    bro1->LoadEquipment(1, true);
                                    bro2->LoadEquipment(1, true);
                                    _getDrunK = false;
                                    _events.CancelEvent(EVENT_GET_DRUNK);
                                    SetEscortPaused(false);
                                }
                            ++_stepCount;
                            break;
                        case 12:
                            Talk(SAY_3);
                            _events.ScheduleEvent(EVENT_ESCORT, Seconds(5));
                            ++_stepCount;
                            break;
                        case 13:
                            if (Creature* bro1 = ObjectAccessor::GetCreature(*me, _chuckGUID))
                                bro1->AI()->Talk(SAY_0);
                            _events.ScheduleEvent(EVENT_ESCORT, Seconds(5));
                            ++_stepCount;
                            break;
                        case 14:
                            if (Creature* bro1 = ObjectAccessor::GetCreature(*me, _chuckGUID))
                                if (Creature* bro2 = ObjectAccessor::GetCreature(*me, _georgGUID))
                                {
                                    bro1->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH_NO_SHEATHE);
                                    bro2->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH_NO_SHEATHE);
                                    me->HandleEmoteCommand(EMOTE_ONESHOT_NO);
                                }
                            _events.ScheduleEvent(EVENT_ESCORT, Seconds(5));
                            ++_stepCount;
                            break;
                        case 15:
                            SetEscortPaused(false);
                            ++_stepCount;
                            break;
                        default:
                            break;
                    }
                    break;
                case EVENT_GET_DRUNK:
                    if (_getDrunK)
                    {
                        if (Creature* bro1 = ObjectAccessor::GetCreature(*me, _chuckGUID))
                            if (Creature* bro2 = ObjectAccessor::GetCreature(*me, _georgGUID))
                            {
                                me->HandleEmoteCommand(EMOTE_STATE_EAT_NO_SHEATHE);
                                bro1->HandleEmoteCommand(EMOTE_STATE_EAT_NO_SHEATHE);
                                bro2->HandleEmoteCommand(EMOTE_STATE_EAT_NO_SHEATHE);
                            }
                    }
                    _events.ScheduleEvent(EVENT_GET_DRUNK, Seconds(1));
                    break;
                default:
                    break;
            }
        }

        if (!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }
private:
    ObjectGuid  _georgGUID;
    ObjectGuid  _chuckGUID;
    ObjectGuid  _jamesGUID;
    EventMap    _events;
    uint16      _stepCount;
    bool        _getDrunK;
};

// Sands of Nasam
enum CryptCrawler
{
    SPELL_CRYPT_SCARABS     = 31600,
    SPELL_ROOT_SELF         = 42716,

    NPC_VALIANCE_FOOTMAN    = 25313,

    AREA_SANDS_OF_NASAM     = 4101,

    EVENT_SPAWN_FOOTMAN     = 1,
    EVENT_CHECK_DISTANCE    = 2,
};

struct npc_crypt_crawlerAI : public ScriptedAI
{
    npc_crypt_crawlerAI(Creature* creature) : ScriptedAI(creature) { }

    void InitializeAI() override
    {
        if (me->GetAreaId() == AREA_SANDS_OF_NASAM)
            SpawnVictim();

        Reset();
    }

    void Reset() override
    {
        if (me->GetAreaId() == AREA_SANDS_OF_NASAM)
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_SPAWN_FOOTMAN, Seconds(urand(15, 50)));
        }
        else
            me->GetMotionMaster()->MoveRandom(7.0f);

        me->RemoveAurasDueToSpell(SPELL_ROOT_SELF);
    }

    void EnterCombat(Unit* /*who*/) override
    {
        DoCast(SPELL_ROOT_SELF);
        _events.ScheduleEvent(EVENT_CHECK_DISTANCE, Milliseconds(500));
    }

    void SpawnVictim()
    {
        if (Creature* victim = DoSummon(NPC_VALIANCE_FOOTMAN, me, 30.0f, 10 * IN_MILLISECONDS, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT))
        {
            victim->SetHomePosition(victim->GetPositionX(), victim->GetPositionY(), victim->GetPositionZ(), victim->GetOrientation());
            victim->AI()->AttackStart(me);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SPAWN_FOOTMAN:
                    SpawnVictim();
                    break;
                case EVENT_CHECK_DISTANCE:
                    if (me->GetVictim())
                    {
                        if (me->GetExactDist(me->GetVictim()) < 40.0f)
                        {
                            if (!me->HasAura(SPELL_ROOT_SELF))
                                DoCastSelf(SPELL_ROOT_SELF);
                            else
                                if (me->HasAura(SPELL_ROOT_SELF))
                                    me->RemoveAurasDueToSpell(SPELL_ROOT_SELF);
                        }
                    }
                    _events.ScheduleEvent(EVENT_CHECK_DISTANCE, Milliseconds(500));
                    break;
                default:
                    break;
            }
        }

        if (!UpdateVictim())
            return;

        DoSpellAttackIfReady(SPELL_CRYPT_SCARABS);
        DoMeleeAttackIfReady();
    }

    private:
        EventMap   _events;
};

void AddSC_area_valiance_keep_rg()
{
    new CreatureScriptLoaderEx<npc_borean_queue_civil_recruitAI>("npc_borean_queue_civil_recruit");
    new CreatureScriptLoaderEx<npc_borean_queue_main_triggerAI>("npc_borean_queue_main_trigger");
    new CreatureScriptLoaderEx<npc_borean_footman_robAI>("npc_borean_footman_rob");
    new CreatureScriptLoaderEx<npc_crypt_crawlerAI>("npc_crypt_crawler");
}
