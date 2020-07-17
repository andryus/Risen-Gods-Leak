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
#include "utgarde_pinnacle.h"
#include "Player.h"
#include "SpellInfo.h"

//Yell
enum eYells
{
    SAY_AGGRO                           = 0,
    SAY_KILL                            = 1,
    EMOTE_RANGE                         = 2,
    SAY_DEATH                           = 3,
    SAY_DRAKE_DEATH                     = 4,
    EMOTE_BREATH                        = 5,
    //SAY_DRAKE_BREATH                    = 6 //only wowwiki says that he has somthing on breath, all other sources have no text exept Bossemote "Grauf take a deep breath."
};
#define BOSS_EMOTE_RANGE "Skadi der Skrupellose ist innerhalb der Reichweite der Harpunenkanonen!" //"Skadi the Ruthless in within range of the harpoon launchers!"
#define BOSS_EMOTE_BREATH "Grauf holt tief Luft." //"Grauf take a deep breath."

static Position SpawnLoc = {488.887f, -484.645f, 104.704f, 0};
static Position Location[]=
{
    // Boss
    {341.740997f, -516.955017f, 104.66900f,  0}, // 0
    {293.299f,    -505.95f,     142.03f,     0}, // 1
    {301.569f,    -543.423f,    146.097f,    0}, // 2
    {526.895f,    -546.387f,    119.209f,    0}, // 3
    {477.311981f, -509.296814f, 104.72308f,  0}, // 4
    {341.740997f, -516.955017f, 104.66900f,  0}, // 5
    {341.740997f, -516.955017f, 104.66900f,  0}, // 6
    {341.740997f, -516.955017f, 104.66900f,  0}, // 7
    // Triggers Left
    {469.661f,    -484.546f,    104.712f,    0}, // 8
    {483.315f,    -485.028f,    104.718f,    0}, // 9
    {476.87f,     -487.994f,    104.735f,    0}, //10
    {477.512f,    -497.772f,    104.728f,    0}, //11
    {486.287f,    -500.759f,    104.722f,    0}, //12
    {480.1f,      -503.895f,    104.722f,    0}, //13
    {472.391f,    -505.103f,    104.723f,    0}, //14
    {478.885f,    -510.803f,    104.723f,    0}, //15
    {489.529f,    -508.615f,    104.723f,    0}, //16
    {484.272f,    -508.589f,    104.723f,    0}, //17
    {465.328f,    -506.495f,    104.427f,    0}, //18
    {456.885f,    -508.104f,    104.447f,    0}, //19
    {450.177f,    -507.989f,    105.247f,    0}, //20
    {442.273f,    -508.029f,    104.813f,    0}, //21
    {434.225f,    -508.19f,     104.787f,    0}, //22
    {423.902f,    -508.525f,    104.274f,    0}, //23
    {414.551f,    -508.645f,    105.136f,    0}, //24
    {405.787f,    -508.755f,    104.988f,    0}, //25
    {398.812f,    -507.224f,    104.82f,     0}, //26
    {389.702f,    -506.846f,    104.729f,    0}, //27
    {381.856f,    -506.76f,     104.756f,    0}, //28
    {372.881f,    -507.254f,    104.779f,    0}, //29
    {364.978f,    -508.182f,    104.673f,    0}, //30
    {357.633f,    -508.075f,    104.647f,    0}, //31
    {350.008f,    -506.826f,    104.588f,    0}, //32
    {341.69f,     -506.77f,     104.499f,    0}, //33
    {335.31f,     -505.745f,    105.18f,     0}, //34
    {471.178f,    -510.74f,     104.723f,    0}, //35
    {461.759f,    -510.365f,    104.199f,    0}, //36
    {424.07287f,  -510.082916f, 104.711082f, 0}, //37
    // Triggers Right
    {489.46f,     -513.297f,    105.413f,    0}, //38
    {485.706f,    -517.175f,    104.724f,    0}, //39
    {480.98f,     -519.313f,    104.724f,    0}, //40
    {475.05f,     -520.52f,     104.724f,    0}, //41
    {482.97f,     -512.099f,    104.724f,    0}, //42
    {477.082f,    -514.172f,    104.724f,    0}, //43
    {468.991f,    -516.691f,    104.724f,    0}, //44
    {461.722f,    -517.063f,    104.627f,    0}, //45
    {455.88f,     -517.681f,    104.707f,    0}, //46
    {450.499f,    -519.099f,    104.701f,    0}, //47
    {444.889f,    -518.963f,    104.82f,     0}, //48
    {440.181f,    -518.893f,    104.861f,    0}, //49
    {434.393f,    -518.758f,    104.891f,    0}, //50
    {429.328f,    -518.583f,    104.904f,    0}, //51
    {423.844f,    -518.394f,    105.004f,    0}, //52
    {418.707f,    -518.266f,    105.135f,    0}, //53
    {413.377f,    -518.085f,    105.153f,    0}, //54
    {407.277f,    -517.844f,    104.893f,    0}, //55
    {401.082f,    -517.443f,    104.723f,    0}, //56
    {394.933f,    -514.64f,     104.724f,    0}, //57
    {388.917f,    -514.688f,    104.734f,    0}, //58
    {383.814f,    -515.834f,    104.73f,     0}, //59
    {377.887f,    -518.653f,    104.777f,    0}, //60
    {371.376f,    -518.289f,    104.781f,    0}, //61
    {365.669f,    -517.822f,    104.758f,    0}, //62
    {359.572f,    -517.314f,    104.706f,    0}, //63
    {353.632f,    -517.146f,    104.647f,    0}, //64
    {347.998f,    -517.038f,    104.538f,    0}, //65
    {341.803f,    -516.98f,     104.584f,    0}, //66
    {335.879f,    -516.674f,    104.628f,    0}, //67
    {329.871f,    -515.92f,     104.711f,    0}, //68
    // Breach Zone
    {503.6110f,   -527.6420f,   115.3011f,   0}, //69
    {435.1892f,   -514.5232f,   118.6719f,   0}, //70
    {413.9327f,   -540.9407f,   138.2614f,   0}, //71
    // Adds walk
    {477.514f,    -484.145f,    104.738f,    0}, //72 after spawn first pos
    {476.677f,    -506.362f,    104.723f,    0}, //73 second pos
    {469.068f,    -507.505f,    104.723f,    0}, //74 1st standing pos
    {468.765f,    -515.831f,    104.721f,    0}, //75 2nd standing pos
    {480.027f,    -507.184f,    104.723f,    0}, //76 3d standing pos
    {480.545f,    -514.267f,    104.723f,    0}, //77 4th standing pos
    {446.691f,    -509.988f,    104.700f,    0}, //78 5th standing pos
    {446.375f,    -518.023f,    104.803f,    0}, //79 6th standing pos
    {422.818f,    -510.261f,    104.764f,    0}, //80 7th standing pos
    {422.924f,    -517.455f,    105.083f,    0}, //81 8th standing pos
    {402.248f,    -509.108f,    105.003f,    0}, //82 9th standing pos
    {401.450f,    -515.710f,    104.725f,    0}, //83 10th standing pos
};

enum eCombatPhase
{
    FLYING = 1,
    SKADI = 2,
    WHIRLING = 3,
};

enum eSpells
{
    //Skadi Spells
    SPELL_CRUSH             = 50234,
    SPELL_POISONED_SPEAR    = 59331,
    SPELL_WHIRLWIND         = 50228, //random target, but not the tank approx. every 20s
    SPELL_RAPID_FIRE        = 56570,
    SPELL_HARPOON_DAMAGE    = 56578,
    SPELL_FREEZING_CLOUD    = 47579,
    SPELL_SUMMON_HARPOONS   = 56789,
    //Add Spells
    SPELL_STRIKE            = 48640,
    SPELL_HAMSTRING         = 48639,
    SPELL_SHRINK            = 49089,
    SPELL_SHADOW_BOLT       = 49084,
    SPELL_THROW             = 49091,
    SPELL_NET               = 49092,
};

enum eCreature
{
    NPC_YMIRJAR_WARRIOR       = 26690,
    NPC_YMIRJAR_WITCH_DOCTOR  = 26691,
    NPC_YMIRJAR_HARPOONER     = 26692,
    NPC_GRAUF                 = 26893,
    NPC_TRIGGER1              = 28351,
    NPC_TRIGGER2              = 22515,
    GO_HARPOON_LAUNCHER_1     = 192175,
};

enum eAchievments
{
    ACHIEV_TIMED_START_EVENT    = 17726,
    ACHIEV_GIRL_LOVES_SKADI     = 2156,
};

enum Events
{
    EVENT_NULL, //to prevent bugs, events can't have id == 0
    EVENT_CRUSH,
    EVENT_POISONED_SPEAR,
    EVENT_START_WHIRLWIND,
    EVENT_STOP_WHIRLWIND,
    EVENT_MOVE,
    EVENT_MOUNT,
    EVENT_SUMMON
};

class boss_skadi : public CreatureScript
{
public:
    boss_skadi() : CreatureScript("boss_skadi") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_skadiAI (creature);
    }

    struct boss_skadiAI : public ScriptedAI
    {
        boss_skadiAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        SummonList summons;
        ObjectGuid graufGUID;
        std::vector<ObjectGuid> triggersGUID;

        uint32 waypointId;
        uint8  spellHitCount;
        bool firstWaveDone;
        uint32 loopCounter[3];
        
        EventMap events;

        void Reset()
        {
            triggersGUID.clear();

            waypointId = 0;
            spellHitCount = 0;
            firstWaveDone = false;

            events.Reset();
            events.ScheduleEvent(EVENT_MOVE, 2 *IN_MILLISECONDS, 0, FLYING);
            events.ScheduleEvent(EVENT_MOUNT, 1 * IN_MILLISECONDS, 0, FLYING);
            events.ScheduleEvent(EVENT_SUMMON, 5 * IN_MILLISECONDS, 0, FLYING);
            
            events.SetPhase(SKADI);

            for(uint8 i = 0; i < 3; i++)
                loopCounter[i] = 0;

            summons.DespawnAll();
            me->SetSpeedRate(MOVE_FLIGHT, 3.0f);

            me->SetCanFly(false);
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_HARPOON_DAMAGE, true); 
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_RAPID_FIRE, true);

            if ((ObjectAccessor::GetCreature((*me), graufGUID) == NULL) && !me->IsMounted())
                 me->SummonCreature(NPC_GRAUF, Location[0].GetPositionX(), Location[0].GetPositionY(), Location[0].GetPositionZ(), 3.0f);

            if (Creature* grauf = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_MOB_GRAUF))))
                grauf->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);

            if (instance)
            {
                instance->SetData(DATA_SKADI_THE_RUTHLESS_EVENT, NOT_STARTED);
                instance->DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);
            }
            
            for (uint8 i = 0; i < 3; ++i)
                if (GameObject* go = me->FindNearestGameObject(GO_HARPOON_LAUNCHER_1 + i, 200.0f)) // 192175-192177 are the launchers for get down skadi
                    go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
        }

        void JustReachedHome()
        {
            me->Dismount();
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetReactState(REACT_AGGRESSIVE);
            if (ObjectAccessor::GetCreature((*me), graufGUID) == NULL)
                me->SummonCreature(NPC_GRAUF, Location[0].GetPositionX(), Location[0].GetPositionY(), Location[0].GetPositionZ(), 3.0f);
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(SAY_AGGRO);

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetReactState(REACT_PASSIVE);

            events.SetPhase(FLYING);

            me->SetInCombatWithZone();
            if (instance)
            {
                instance->SetData(DATA_SKADI_THE_RUTHLESS_EVENT, IN_PROGRESS);
                instance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);
            }
        }

        void JustSummoned(Creature* summoned)
        {
            switch (summoned->GetEntry())
            {
                case NPC_GRAUF:
                    graufGUID = summoned->GetGUID();
                    summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                    summoned->SetReactState(REACT_PASSIVE);
                    break;
                case NPC_YMIRJAR_WARRIOR:
                case NPC_YMIRJAR_WITCH_DOCTOR:
                case NPC_YMIRJAR_HARPOONER:
                    summoned->setActive(true);
                    break;
                case NPC_TRIGGER1:
                    summoned->CastSpell((Unit*)NULL, SPELL_FREEZING_CLOUD, true);
                    summoned->DespawnOrUnsummon(10000);
                    break;
                default:
                    break;
            }
            summons.Summon(summoned);
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/)
        {
            if (summon->GetEntry() == NPC_GRAUF)
                DoAction(2);
        }

        void SummonedCreatureDespawn(Creature* summoned)
        {
            if (summoned->GetEntry() == NPC_GRAUF)
                graufGUID.Clear();
            summons.Despawn(summoned);
        }

        void DoAction(int32 action)
        {
            if (action == 1)
            {
                if (spellHitCount == 0)
                    loopCounter[1] = loopCounter[0];

                spellHitCount++;

                /*if (spellHitCount >= DUNGEON_MODE(4, 6))
                {
                    DoAction(2);
                }*/
            }

            else if (action == 2)
            {
                loopCounter[2] = loopCounter[0];

                events.SetPhase(SKADI);
                events.RescheduleEvent(EVENT_MOUNT, 1 * IN_MILLISECONDS, 0, SKADI);
                events.RescheduleEvent(EVENT_CRUSH, 8 * IN_MILLISECONDS, 0, SKADI);
                events.RescheduleEvent(EVENT_POISONED_SPEAR, 10 * IN_MILLISECONDS, 0, SKADI);
                events.RescheduleEvent(EVENT_START_WHIRLWIND, 20 * IN_MILLISECONDS, 0, SKADI);

            }

            else if (action == 3)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                me->SetReactState(REACT_AGGRESSIVE);
                Talk(SAY_DRAKE_DEATH);
                
                //bedder then blizz, on blizz skadi simply spawn and doesn't jump to position
                me->GetMotionMaster()->MoveJump(Location[4], 25.0f, 10.0f);
                me->SetFullHealth();

                for (uint8 i = 0; i < 3; ++i)
                    if (GameObject* go = me->FindNearestGameObject(GO_HARPOON_LAUNCHER_1 + i, 200.0f)) // 192175-192177 are the launchers for get down skadi
                        go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);
            
            switch (events.ExecuteEvent())
            {
                case EVENT_MOUNT:
                    if (events.GetPhaseMask() == FLYING)
                    {
                        if (Creature* grauf = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_MOB_GRAUF))))
                        {
                            me->EnterVehicle(grauf, 0);
                            grauf->SetSpeedRate(MOVE_RUN, 3.0f);
                            grauf->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                        }
                    } else DoAction(3);
                    break;
                case EVENT_SUMMON:
                    if (!firstWaveDone)
                        SpawnMobs(true);
                    else SpawnMobs(false);
                    events.RescheduleEvent(EVENT_SUMMON, 18 * IN_MILLISECONDS, 0, FLYING);
                    break;
                case EVENT_MOVE:
                    if (Creature* grauf = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_MOB_GRAUF))))
                    {
                        uint32 _time = 0;
                        switch(waypointId)
                        {
                            case 0:
                                grauf->GetMotionMaster()->MovePoint(1, Location[1].GetPositionX(), Location[1].GetPositionY(), Location[1].GetPositionZ());
                                _time = 5 * IN_MILLISECONDS;
                                break;
                            case 1:
                                grauf->GetMotionMaster()->MovePoint(2, Location[2].GetPositionX(), Location[2].GetPositionY(), Location[2].GetPositionZ());
                                _time = 2 * IN_MILLISECONDS;
                                break;
                            case 2:
                                grauf->GetMotionMaster()->MovePoint(3, Location[3].GetPositionX(), Location[3].GetPositionY(), Location[3].GetPositionZ());
                                if (loopCounter[0] == 0)
                                    _time = 11 * IN_MILLISECONDS;
                                else _time = 3 * IN_MILLISECONDS;
                                break;
                            case 3:
                                loopCounter[0]++;
                            if (Creature* grauf = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_MOB_GRAUF))))
                                    grauf->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                                //Talk(EMOTE_RANGE); //Talk with Boss Emotes doesn't work. Don't know why.
                                if (Creature* trigger = me->SummonCreature(NPC_TRIGGER2, Location[4].GetPositionX(), Location[4].GetPositionY(), Location[4].GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN, 3000))
                                    trigger->MonsterTextEmote(BOSS_EMOTE_RANGE, trigger, true);
                                _time = 7 * IN_MILLISECONDS;
                                break;
                            case 4:
                                grauf->GetMotionMaster()->MovePoint(4, Location[69].GetPositionX(), Location[69].GetPositionY(), Location[69].GetPositionZ());
                            if (Creature* grauf = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_MOB_GRAUF))))
                                    grauf->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                            
                                //Talk(EMOTE_BREATH); //Talk with Boss Emotes doesn't work. Don't know why.
                                if (Creature* trigger = me->SummonCreature(NPC_TRIGGER2, Location[4].GetPositionX(), Location[4].GetPositionY(), Location[4].GetPositionZ(), 0.0f, TEMPSUMMON_MANUAL_DESPAWN, 3000))
                                    trigger->MonsterTextEmote(BOSS_EMOTE_BREATH, trigger, true);
                                //Talk(SAY_DRAKE_BREATH); //only wowwiki says that he has somthing on breath, all other sources have no text exept Bossemote "Grauf take a deep breath."
                                _time = 2500;
                                break;
                            case 5:
                                grauf->GetMotionMaster()->MovePoint(5, Location[70].GetPositionX(), Location[70].GetPositionY(), Location[70].GetPositionZ());
                                _time = 2 * IN_MILLISECONDS;
                                SpawnTrigger();
                                break;
                            case 6:
                                grauf->GetMotionMaster()->MovePoint(6, Location[71].GetPositionX(), Location[71].GetPositionY(), Location[71].GetPositionZ());
                                _time = 12 * IN_MILLISECONDS;
                                break;
                            case 7:
                                grauf->GetMotionMaster()->MovePoint(3, Location[3].GetPositionX(), Location[3].GetPositionY(), Location[3].GetPositionZ());
                                waypointId = 2;
                                _time = 6 * IN_MILLISECONDS;
                                break;
                        }
                        events.RescheduleEvent(EVENT_MOVE, _time, 0, FLYING);
                        waypointId++;
                    }
                    break;
                case EVENT_CRUSH:
                    DoCastVictim(SPELL_CRUSH);
                    events.RescheduleEvent(EVENT_CRUSH, urand(6, 10) * IN_MILLISECONDS, 0, SKADI);
                    break;
                case EVENT_POISONED_SPEAR:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                        DoCast(target, SPELL_POISONED_SPEAR, true);
                    events.RescheduleEvent(EVENT_POISONED_SPEAR, urand(8, 12) * IN_MILLISECONDS, 0, SKADI);
                    break;
                case EVENT_START_WHIRLWIND:
                    DoCast(SPELL_WHIRLWIND);
                    events.SetPhase(WHIRLING);
                    events.RescheduleEvent(EVENT_STOP_WHIRLWIND, 10 * IN_MILLISECONDS, 0, WHIRLING);
                    break;
                case EVENT_STOP_WHIRLWIND:
                    events.SetPhase(SKADI);
                    events.RescheduleEvent(EVENT_POISONED_SPEAR, urand(8, 12) * IN_MILLISECONDS, 0, SKADI);
                    events.RescheduleEvent(EVENT_CRUSH, urand(6, 10) * IN_MILLISECONDS, 0, SKADI);
                    events.RescheduleEvent(EVENT_START_WHIRLWIND, 20 * IN_MILLISECONDS, 0, SKADI);
                    break;
            }
            if (events.GetPhaseMask() == SKADI)
                DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(SAY_DEATH);
            summons.DespawnAll();
            if (instance)
            {
                instance->SetData(DATA_SKADI_THE_RUTHLESS_EVENT, DONE);

                if (loopCounter[1] == loopCounter[2] && IsHeroic())
                    instance->DoCompleteAchievement(ACHIEV_GIRL_LOVES_SKADI);
            }

            me->ClearInCombatWithZone();
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Talk(SAY_KILL);
        }

        void SpawnMobs(bool firstWave)
        {
            for (uint8 i = 0; i < (!firstWave ? 2 : DUNGEON_MODE(6, 10)); ++i)
            {
                if (Creature* c = me->SummonCreature(RAND(NPC_YMIRJAR_WARRIOR, NPC_YMIRJAR_WITCH_DOCTOR, NPC_YMIRJAR_HARPOONER), 
                    SpawnLoc.GetPositionX(), SpawnLoc.GetPositionY(), SpawnLoc.GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000))
                    c->AI()->DoAction(i);
            }
            firstWaveDone = true;
        }

        void SpawnTrigger()
        {
            uint8 start = 0, end = 0;
            switch (urand(0, 1))
            {
                case 0:
                    start = 8;
                    end = 37;
                    break;
                case 1:
                    start = 38;
                    end = 68;
                    break;
                default:
                    break;
            }
            for (uint32 i = start; i < end; ++i)
                me->SummonCreature(NPC_TRIGGER1, Location[i]);
        }
    };

};

class go_harpoon_launcher : public GameObjectScript
{
public:
    go_harpoon_launcher() : GameObjectScript("go_harpoon_launcher") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        InstanceScript* instance = go->GetInstanceScript();
        if (!instance) return false;

        if (Creature* skadi = ObjectAccessor::GetCreature((*go), ObjectGuid(instance->GetGuidData(DATA_SKADI_THE_RUTHLESS))))
        {
            if (skadi->GetVehicleBase() && !skadi->GetVehicleBase()->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1))
            {
                player->CastSpell(skadi->GetVehicleBase(), SPELL_RAPID_FIRE, true);
                skadi->AI()->DoAction(1);
            }
        }
        return false;
    }
};

class npc_skadi_adds : public CreatureScript
{
public:
    npc_skadi_adds() : CreatureScript("npc_skadi_adds") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_skadi_addsAI (creature);
    }

    struct npc_skadi_addsAI : public ScriptedAI
    {
        npc_skadi_addsAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        uint32 wp;
        uint32 standingPoint;
        uint32 castOneTimer;
        uint32 castTwoTimer;

        void Reset()
        {
            wp = 72;
            standingPoint = 74;
            castOneTimer = urand(2000, 25000);
            castTwoTimer = urand(3000, 35000);
        }

        void DoAction(int32 action)
        {
            standingPoint += action;
        }

        void JustDied(Unit* /*killer*/)
        {
            float x, y, z;
            me->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE);

             //Horde Guids say tha each kind of add have chance to drop a harpoon, but it is unlogic because only harpooner equip harpoons
            if (me->GetEntry() == NPC_YMIRJAR_HARPOONER)
                //if (rand_chance() < 33.3f)
                    me->CastSpell(x, y, z, SPELL_SUMMON_HARPOONS, true);
        }

        void UpdateAI(uint32 diff)
        {
            //out fight actions
            if (!UpdateVictim())
            {
                if (wp <= standingPoint)
                    if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
                    {
                        if (wp <= 73)
                            me->GetMotionMaster()->MovePoint(wp, Location[wp].GetPositionX(), Location[wp].GetPositionY(), Location[wp].GetPositionZ());
                        else me->GetMotionMaster()->MovePoint(standingPoint, Location[standingPoint].GetPositionX(), Location[standingPoint].GetPositionY(), Location[standingPoint].GetPositionZ());
                        wp++;
                    }
                return;
            }
            //in fight actions
            if (castOneTimer <= diff)
            {
                Cast(true);
                castOneTimer = urand(2000, 25000);
            }
            else castOneTimer -= diff;
            
            if (castTwoTimer <= diff)
            {
                Cast(false);
                castTwoTimer = urand(3000, 35000);
            }
            else castTwoTimer -= diff;

            DoMeleeAttackIfReady();
        }

        void Cast(bool one) //first or second spell, seen by castOneTimer or castTwoTimer
        {
            switch (me->GetEntry())
            {
                case NPC_YMIRJAR_WARRIOR:
                    one ? DoCast(SPELL_STRIKE) : DoCast(SPELL_HAMSTRING);
                    break;
                case NPC_YMIRJAR_WITCH_DOCTOR:
                    one ? DoCast(SPELL_SHRINK) : DoCast(SPELL_SHADOW_BOLT);
                    break;
                case NPC_YMIRJAR_HARPOONER:
                    one ?  DoCast(SPELL_THROW) : DoCast(SPELL_NET);
            }
        }
    };
};

void AddSC_boss_skadi()
{
    new boss_skadi();
    new go_harpoon_launcher();
    new npc_skadi_adds();
}
