/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2012-2015 Rising-Gods <http://www.rising-gods.de/>
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
#include "ScriptedEscortAI.h"
#include "trial_of_the_champion.h"
#include "SpellScript.h"
#include "DBCStores.h"

enum Yells
{
    // Eadric the Pure
    //SAY_INTRO                        = 0,
    SAY_EADRIC_AGGRO                   = 1,
    EMOTE_EADRIC_RADIANCE              = 2,
    EMOTE_EADRIC_HAMMER_RIGHTEOUS      = 3,
    SAY_EADRIC_HAMMER_RIGHTEOUS        = 4,
    SAY_EADRIC_KILL_PLAYER             = 5,
    SAY_EADRIC_DEFEATED                = 6,

    // Argent Confessor Paletress
    //SAY_INTRO_1                      = 0,
    //SAY_INTRO_2                      = 1,
    SAY_PALETRESS_AGGRO                = 2,
    SAY_PALETRESS_MEMORY_SUMMON        = 3,
    SAY_PALETRESS_MEMORY_DEATH         = 4,
    SAY_PALETRESS_KILL_PLAYER          = 5,
    SAY_PALETRESS_DEFEATED             = 6,

    // Memory of X
    EMOTE_WAKING_NIGHTMARE      = 0
};

enum Spells
{
    // Eadric the Pure
    SPELL_EADRIC_ACHIEVEMENT    = 68197,
    SPELL_HAMMER_JUSTICE        = 66863,
    SPELL_HAMMER_JUSTICE_AURA   = 66940,
    SPELL_HAMMER_RIGHTEOUS      = 66867,
    SPELL_HAMMER_RIGHTEOUS_AURA = 66904,
    SPELL_HAMMER_RIGHTEOUS_PLAYER = 66905,
    SPELL_RADIANCE              = 66935,
    SPELL_VENGEANCE             = 66865,

    //Paletress
    SPELL_SMITE                 = 66536,
    SPELL_SMITE_H               = 67674,
    SPELL_HOLY_FIRE             = 66538,
    SPELL_HOLY_FIRE_H           = 67676,
    SPELL_RENEW                 = 66537,
    SPELL_RENEW_H               = 67675,
    SPELL_HOLY_NOVA             = 66546,
    SPELL_SHIELD                = 66515,
    SPELL_CONFESS               = 66680,
    SPELL_SUMMON_RANDOM_MEMORY  = 66545,

    //Monk
    SPELL_DIVINE                = 67251,
    SPELL_FINAL                 = 67255,
    SPELL_FLURRY                = 67233,
    SPELL_PUMMEL                = 67235,

    //Priestess
    SPELL_SSMITE                = 36176,
    SPELL_SSMITE_H              = 67289,
    SPELL_PAIN                  = 34941,
    SPELL_PAIN_H                = 34941,
    SPELL_MIND                  = 67229,

    //Lightwielder
    SPELL_LIGHT                 = 67247,
    SPELL_LIGHT_H               = 67290,
    SPELL_STRIKE                = 67237,
    SPELL_CLEAVE                = 15284,

    SPELL_FONT                  = 67194,

    // Memory
    SPELL_OLD_WOUNDS            = 66620,
    SPELL_SHADOWS_PAST          = 66619,
    SPELL_WAKING_NIGHTMARE      = 66552
};

enum Achievments
{
    ACHIEV_FACEROLLER           = 3803,
    ACHIEV_CONF                 = 3802
};

enum AchievementCriteria
{
    ACHIEVEMENT_CRITERIA_HOGGER = 11863,
    ACHIEVEMENT_CRITERIA_VANCLEEF = 11904,
    ACHIEVEMENT_CRITERIA_MUTANUS = 11905,
    ACHIEVEMENT_CRITERIA_HEROD = 11906,
    ACHIEVEMENT_CRITERIA_LUCIFRON = 11907,
    ACHIEVEMENT_CRITERIA_THUNDERAAN = 11908,
    ACHIEVEMENT_CRITERIA_CHROMAGGUS = 11909,
    ACHIEVEMENT_CRITERIA_HAKKAR = 11910,
    ACHIEVEMENT_CRITERIA_VEKNILASH = 11911,
    ACHIEVEMENT_CRITERIA_KALITHRESH = 11912,
    ACHIEVEMENT_CRITERIA_MALCHEZAAR = 11913,
    ACHIEVEMENT_CRITERIA_GRUUL = 11914,
    ACHIEVEMENT_CRITERIA_VASHJ = 11915,
    ACHIEVEMENT_CRITERIA_ARCHIMONDE = 11916,
    ACHIEVEMENT_CRITERIA_ILLIDAN = 11917,
    ACHIEVEMENT_CRITERIA_DELRISSA = 11918,
    ACHIEVEMENT_CRITERIA_MURU = 11919,
    ACHIEVEMENT_CRITERIA_INGVAR = 11920,
    ACHIEVEMENT_CRITERIA_CYANIGOSA = 11921,
    ACHIEVEMENT_CRITERIA_ECK = 11922,
    ACHIEVEMENT_CRITERIA_ONYXIA = 11923,
    ACHIEVEMENT_CRITERIA_HEIGAN = 11924,
    ACHIEVEMENT_CRITERIA_IGNIS = 11924,
    ACHIEVEMENT_CRITERIA_VEZAX = 11926,
    ACHIEVEMENT_CRITERIA_ALGALON = 11927,
    ACHIEVEMENT_CRITERIA_PALETRESS_ALLY = 12307,
    ACHIEVEMENT_CRITERIA_PALETRESS_ALLY_HERO = 12323,
    ACHIEVEMENT_CRITERIA_PALETRESS_HORDE = 11559,
    ACHIEVEMENT_CRITERIA_PALETRESS_HORDE_HERO = 12315,
    ACHIEVEMENT_CRITERIA_EADRIC_ALLY = 12308,
    ACHIEVEMENT_CRITERIA_EADRIC_ALLY_HERO = 12324,
    ACHIEVEMENT_CRITERIA_EADRIC_HORDE = 11560,
    ACHIEVEMENT_CRITERIA_EADRIC_HORDE_HERO = 12316
};

enum Memories
{
    NPC_MEMORY_OF_HOGGER = 34942,
    NPC_MEMORY_OF_VANCLEEF = 35028,
    NPC_MEMORY_OF_MUTANUS = 35029,
    NPC_MEMORY_OF_HEROD = 35030,
    NPC_MEMORY_OF_LUCIFRON = 35031,
    NPC_MEMORY_OF_THUNDERAAN = 35032,
    NPC_MEMORY_OF_CHROMAGGUS = 35033,
    NPC_MEMORY_OF_HAKKAR = 35034,
    NPC_MEMORY_OF_VEKNILASH = 35036,
    NPC_MEMORY_OF_KALITHRESH = 35037,
    NPC_MEMORY_OF_MALCHEZAAR = 35038,
    NPC_MEMORY_OF_GRUUL = 35039,
    NPC_MEMORY_OF_VASHJ = 35040,
    NPC_MEMORY_OF_ARCHIMONDE = 35041,
    NPC_MEMORY_OF_ILLIDAN = 35042,
    NPC_MEMORY_OF_DELRISSA = 35043,
    NPC_MEMORY_OF_ENTROPIUS = 35044,
    NPC_MEMORY_OF_INGVAR = 35045,
    NPC_MEMORY_OF_CYANIGOSA = 35046,
    NPC_MEMORY_OF_ECK = 35047,
    NPC_MEMORY_OF_ONYXIA = 35048,
    NPC_MEMORY_OF_HEIGAN = 35049,
    NPC_MEMORY_OF_IGNIS = 35050,
    NPC_MEMORY_OF_VEZAX = 35051,
    NPC_MEMORY_OF_ALGALON = 35052
};

enum Events
{
    // boss eadric
    EVENT_VEGANCE           = 1,
    EVENT_RAIDANCE          = 2,
    EVENT_HAMMER_OF_JUSTICE = 3,

    // boss paletress
    EVENT_HOLY_FIRE         = 1,
    EVENT_HOLY_SMITE        = 2,
    EVENT_RENEW             = 3,
    EVENT_STUN              = 4,
    EVENT_SUM_MEMORY        = 5,

    // npc_memory
    EVENT_OLD_WOUNDS        = 30,
    EVENT_SHADDOW_PAST,
    EVENT_WAKING_NIGHTMARE,
};

enum Actions
{
    ACTION_REMOVE_SHIELD    = 1
};

enum Datas
{
    DATA_CRITERIA_ID        = 1
};

class boss_eadric : public CreatureScript
{
public:
    boss_eadric() : CreatureScript("boss_eadric") { }

    struct boss_eadricAI : public BossAI
    {
        boss_eadricAI(Creature* Creature) : BossAI(Creature, BOSS_ARGENT_CHALLENGE) { }

        bool defeated;

        void Reset() override
        {
            BossAI::Reset();

            defeated = false;
        }

        void EnterCombat(Unit* who) override
        {
            BossAI::EnterCombat(who);
            Talk(SAY_EADRIC_AGGRO);
            events.ScheduleEvent(EVENT_VEGANCE, 10000);
            events.ScheduleEvent(EVENT_RAIDANCE, 16000);
            events.ScheduleEvent(EVENT_HAMMER_OF_JUSTICE, 25000);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == 3)
            {
                if (instance)
                    instance->DoUseDoorOrButton(ObjectGuid(instance->GetGuidData(GO_MAIN_GATE)));
                SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true)->Kill(me);
            }
        }

        void DamageTaken(Unit* /*who*/, uint32 &damage)
        {
            if (damage >= me->GetHealth())
            {
                damage = me->GetHealth() - 1;
                if (!defeated)
                {
                    defeated = true;
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->GetMotionMaster()->MovePoint(3, 746.843f, 695.68f, 412.339f);
                    events.Reset();
                    Talk(SAY_EADRIC_DEFEATED);

                    if (instance)
                    {
                        instance->DoUseDoorOrButton(ObjectGuid(instance->GetGuidData(GO_MAIN_GATE)));

                        // normal achievements
                        if (instance->GetData(DATA_TEAM_IN_INSTANCE) == ALLIANCE)
                            instance->DoCompleteCriteria(ACHIEVEMENT_CRITERIA_EADRIC_ALLY);
                        else
                            instance->DoCompleteCriteria(ACHIEVEMENT_CRITERIA_EADRIC_HORDE);

                        // hero achievements
                        if (IsHeroic())
                        {
                            if (instance->GetData(DATA_TEAM_IN_INSTANCE) == ALLIANCE)
                                instance->DoCompleteCriteria(ACHIEVEMENT_CRITERIA_EADRIC_ALLY_HERO);
                            else
                                instance->DoCompleteCriteria(ACHIEVEMENT_CRITERIA_EADRIC_HORDE_HERO);
                        }
                    }
                }
            }
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_EADRIC_KILL_PLAYER);
        }

        void SpellHit(Unit* /*caster*/, SpellInfo const* spellInfo) override
        {
            if (spellInfo->Id == SPELL_HAMMER_RIGHTEOUS_PLAYER && me->GetHealth() == 1 && IsHeroic())
                instance->DoCompleteAchievement(ACHIEV_FACEROLLER);
        }

        void ExecuteEvent(uint32 eventId) override
        {
            switch (eventId)
            {
            case EVENT_VEGANCE:
                DoCastSelf(SPELL_VENGEANCE);
                events.ScheduleEvent(EVENT_VEGANCE, 10000);
                break;
            case EVENT_RAIDANCE:
                DoCastAOE(SPELL_RADIANCE);
                Talk(EMOTE_EADRIC_RADIANCE);
                events.ScheduleEvent(EVENT_RAIDANCE, 16000);
                break;
            case EVENT_HAMMER_OF_JUSTICE:
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                {
                    Talk(SAY_EADRIC_HAMMER_RIGHTEOUS);
                    Talk(EMOTE_EADRIC_HAMMER_RIGHTEOUS, target);
                    DoCast(target, SPELL_HAMMER_JUSTICE);
                    DoCast(target, SPELL_HAMMER_RIGHTEOUS);
                }
                events.ScheduleEvent(EVENT_HAMMER_OF_JUSTICE, 25000);
                break;
            }
        }

        bool CanAIAttack(Unit const* target) const override
        {
            return !target->HasAura(SPELL_MIND) && BossAI::CanAIAttack(target);
        }
    };

    CreatureAI* GetAI(Creature* Creature) const override
    {
        return new boss_eadricAI(Creature);
    }
};

// Summon Memories Spells
uint32 SummonMemory[] = {66543, 66691, 66692, 66694, 66695, 66696, 66697, 66698, 66699, 66700, 66701, 66702, 66703, 66704, 66705, 66706, 66707, 66708, 66709, 66710, 66711, 66712, 66713, 66714, 66715};

class boss_paletress : public CreatureScript
{
public:
    boss_paletress() : CreatureScript("boss_paletress") { }

    struct boss_paletressAI : public BossAI
    {
        boss_paletressAI(Creature* Creature) : BossAI(Creature, BOSS_ARGENT_CHALLENGE) { }

        bool defeated;

        void EnterCombat(Unit* who) override
        {
            BossAI::EnterCombat(who);
            Talk(SAY_PALETRESS_AGGRO);
            events.ScheduleEvent(EVENT_HOLY_FIRE, urand(9000,12000));
            events.ScheduleEvent(EVENT_HOLY_SMITE, urand(5000,7000));
            events.ScheduleEvent(EVENT_RENEW, urand(2000,5000));
        }

        void Reset() override
        {
            BossAI::Reset();

            defeated = false;
            HealthOver50Pct = true;
            MemoryGUID.Clear();
            AchievementCriteriaId = 0;
        }

        void DoAction(int32 action) override
        {
            if (action == ACTION_REMOVE_SHIELD)
            {
                Talk(SAY_PALETRESS_MEMORY_DEATH);
                me->RemoveAurasDueToSpell(SPELL_SHIELD);
            }
        }

        void JustSummoned(Creature* summon) override
        {
            BossAI::JustSummoned(summon);
            MemoryGUID = summon->GetGUID();
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_PALETRESS_KILL_PLAYER);
        }

        void DamageTaken(Unit* /*who*/, uint32 &damage) override
        {
            if (damage >= me->GetHealth())
            {
                damage = me->GetHealth() - 1;
                if (!defeated)
                {
                    defeated = true;
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->GetMotionMaster()->MovePoint(3, 746.843f, 695.68f, 412.339f);
                    events.Reset();
                    Talk(SAY_PALETRESS_DEFEATED);

                    if (instance)
                    {
                        instance->DoUseDoorOrButton(ObjectGuid(instance->GetGuidData(GO_MAIN_GATE)));

                        if (instance->GetData(DATA_TEAM_IN_INSTANCE) == ALLIANCE)
                            instance->DoCompleteCriteria(ACHIEVEMENT_CRITERIA_PALETRESS_ALLY);
                        else
                            instance->DoCompleteCriteria(ACHIEVEMENT_CRITERIA_PALETRESS_HORDE);

                        if (IsHeroic())
                        {
                            instance->DoCompleteCriteria(AchievementCriteriaId);

                            if (instance->GetData(DATA_TEAM_IN_INSTANCE) == ALLIANCE)
                                instance->DoCompleteCriteria(ACHIEVEMENT_CRITERIA_PALETRESS_ALLY_HERO);
                            else
                                instance->DoCompleteCriteria(ACHIEVEMENT_CRITERIA_PALETRESS_HORDE_HERO);
                        }
                    }
                }
            }
            else if (HealthOver50Pct && me->GetHealthPct() <= 50.0f)
            {
                HealthOver50Pct = false;
                events.ScheduleEvent(EVENT_STUN, 0);
            }
        }

        void SpellHit(Unit* /*caster*/, SpellInfo const* spellInfo) override
        {
            if (spellInfo->HasEffect(SPELL_EFFECT_KNOCK_BACK))
                me->InterruptNonMeleeSpells(true);
        }

        void SetData(uint32 type, uint32 data) override
        {
            if (type == DATA_CRITERIA_ID)
            {
                AchievementCriteriaId = data;
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == 3)
            {
                if (instance)
                    instance->DoUseDoorOrButton(ObjectGuid(instance->GetGuidData(GO_MAIN_GATE)));
                BossAI::JustDied(nullptr);
                SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true)->Kill(me);
            }
        }

        void ExecuteEvent(uint32 eventId) override
        {
            switch (eventId)
            {
            case EVENT_HOLY_FIRE:
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    DoCast(target, SPELL_HOLY_FIRE);

                if (me->HasAura(SPELL_SHIELD))
                    events.ScheduleEvent(EVENT_HOLY_FIRE, 13000);
                else
                    events.ScheduleEvent(EVENT_HOLY_FIRE, urand(9000, 12000));
                break;
            case EVENT_HOLY_SMITE:
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    DoCast(target, SPELL_SMITE);

                if (me->HasAura(SPELL_SHIELD))
                    events.ScheduleEvent(EVENT_HOLY_FIRE, 9000);
                else
                    events.ScheduleEvent(EVENT_HOLY_FIRE, urand(5000, 7000));
                break;
            case EVENT_RENEW:
                switch (urand(0, 1))
                {
                case 0:
                    DoCastSelf(SPELL_RENEW);
                    break;
                case 1:
                    if (Creature* Memory = ObjectAccessor::GetCreature(*me, MemoryGUID))
                    {
                        if (Memory->IsAlive())
                            DoCast(Memory, SPELL_RENEW);
                    }
                    else
                        DoCastSelf(SPELL_RENEW);
                    break;
                }
                events.ScheduleEvent(EVENT_RENEW, urand(15000, 17000));
                break;
            case EVENT_STUN:
                DoCastAOE(SPELL_HOLY_NOVA);
                DoCastSelf(SPELL_SHIELD);
                DoCastAOE(SPELL_CONFESS);
                events.ScheduleEvent(EVENT_SUM_MEMORY, 1);
                break;
            case EVENT_SUM_MEMORY:
                Talk(SAY_PALETRESS_MEMORY_SUMMON);
                DoCastSelf(SummonMemory[urand(0, 24)]);
                break;
            }
        }

    private:
        bool HealthOver50Pct;
        ObjectGuid MemoryGUID;
        uint32 AchievementCriteriaId;
    };

    CreatureAI* GetAI(Creature* Creature) const override
    {
        return new boss_paletressAI(Creature);
    }
};

class npc_memory : public CreatureScript
{
public:
    npc_memory() : CreatureScript("npc_memory") { }

    struct npc_memoryAI : public ScriptedAI
    {
        npc_memoryAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void EnterCombat(Unit* /*who*/) override
        {
            events.ScheduleEvent(EVENT_OLD_WOUNDS, 6*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_SHADDOW_PAST, 3*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_WAKING_NIGHTMARE, 12*IN_MILLISECONDS);
        }

        void JustDied(Unit* /*Killer*/) override
        {
            if (Creature* paletress = ObjectAccessor::GetCreature(*me, ObjectGuid(instance ? ObjectGuid(instance->GetGuidData(NPC_PALETRESS)) : ObjectGuid::Empty)))
            {
                paletress->AI()->DoAction(ACTION_REMOVE_SHIELD);

                uint32 criteriaID = 0;

                switch(me->GetEntry())
                {
                    case NPC_MEMORY_OF_HOGGER:
                        criteriaID = ACHIEVEMENT_CRITERIA_HOGGER;
                        break;
                    case NPC_MEMORY_OF_VANCLEEF:
                        criteriaID = ACHIEVEMENT_CRITERIA_VANCLEEF;
                        break;
                    case NPC_MEMORY_OF_MUTANUS:
                        criteriaID = ACHIEVEMENT_CRITERIA_MUTANUS;
                        break;
                    case NPC_MEMORY_OF_HEROD:
                        criteriaID = ACHIEVEMENT_CRITERIA_HEROD;
                        break;
                    case NPC_MEMORY_OF_LUCIFRON:
                        criteriaID = ACHIEVEMENT_CRITERIA_LUCIFRON;
                        break;
                    case NPC_MEMORY_OF_THUNDERAAN:
                        criteriaID = ACHIEVEMENT_CRITERIA_THUNDERAAN;
                        break;
                    case NPC_MEMORY_OF_CHROMAGGUS:
                        criteriaID = ACHIEVEMENT_CRITERIA_CHROMAGGUS;
                        break;
                    case NPC_MEMORY_OF_HAKKAR:
                        criteriaID = ACHIEVEMENT_CRITERIA_HAKKAR;
                        break;
                    case NPC_MEMORY_OF_VEKNILASH:
                        criteriaID = ACHIEVEMENT_CRITERIA_VEKNILASH;
                        break;
                    case NPC_MEMORY_OF_KALITHRESH:
                        criteriaID = ACHIEVEMENT_CRITERIA_KALITHRESH;
                        break;
                    case NPC_MEMORY_OF_MALCHEZAAR:
                        criteriaID = ACHIEVEMENT_CRITERIA_MALCHEZAAR;
                        break;
                    case NPC_MEMORY_OF_GRUUL:
                        criteriaID = ACHIEVEMENT_CRITERIA_GRUUL;
                        break;
                    case NPC_MEMORY_OF_VASHJ:
                        criteriaID = ACHIEVEMENT_CRITERIA_VASHJ;
                        break;
                    case NPC_MEMORY_OF_ARCHIMONDE:
                        criteriaID = ACHIEVEMENT_CRITERIA_ARCHIMONDE;
                        break;
                    case NPC_MEMORY_OF_ILLIDAN:
                        criteriaID = ACHIEVEMENT_CRITERIA_ILLIDAN;
                        break;
                    case NPC_MEMORY_OF_DELRISSA:
                        criteriaID = ACHIEVEMENT_CRITERIA_DELRISSA;
                        break;
                    case NPC_MEMORY_OF_ENTROPIUS:
                        criteriaID = ACHIEVEMENT_CRITERIA_MURU; // Entropius = M'uru ?
                        break;
                    case NPC_MEMORY_OF_INGVAR:
                        criteriaID = ACHIEVEMENT_CRITERIA_INGVAR;
                        break;
                    case NPC_MEMORY_OF_CYANIGOSA:
                        criteriaID = ACHIEVEMENT_CRITERIA_CYANIGOSA;
                        break;
                    case NPC_MEMORY_OF_ECK:
                        criteriaID = ACHIEVEMENT_CRITERIA_ECK;
                        break;
                    case NPC_MEMORY_OF_ONYXIA:
                        criteriaID = ACHIEVEMENT_CRITERIA_ONYXIA;
                        break;
                    case NPC_MEMORY_OF_HEIGAN:
                        criteriaID = ACHIEVEMENT_CRITERIA_HEIGAN;
                        break;
                    case NPC_MEMORY_OF_IGNIS:
                        criteriaID = ACHIEVEMENT_CRITERIA_IGNIS;
                        break;
                    case NPC_MEMORY_OF_VEZAX:
                        criteriaID = ACHIEVEMENT_CRITERIA_VEZAX;
                        break;
                    case NPC_MEMORY_OF_ALGALON:
                        criteriaID = ACHIEVEMENT_CRITERIA_ALGALON;
                        break;
                }

                if (criteriaID > 0)
                {
                    paletress->AI()->SetData(DATA_CRITERIA_ID, criteriaID);
                }
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
            {
                switch (eventId)
                {
                    case EVENT_OLD_WOUNDS:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            DoCast(target, SPELL_OLD_WOUNDS);
                        events.ScheduleEvent(EVENT_OLD_WOUNDS, 6*IN_MILLISECONDS);
                        break;
                    case EVENT_SHADDOW_PAST:
                        DoCastSelf(SPELL_WAKING_NIGHTMARE);
                        Talk(EMOTE_WAKING_NIGHTMARE);
                        events.ScheduleEvent(EVENT_SHADDOW_PAST, 7*IN_MILLISECONDS);
                        break;
                    case EVENT_WAKING_NIGHTMARE:
                        if (Unit* Target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                            DoCast(Target, SPELL_SHADOWS_PAST);
                        events.ScheduleEvent(EVENT_WAKING_NIGHTMARE, 12*IN_MILLISECONDS);
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* Creature) const override
    {
        return new npc_memoryAI(Creature);
    }
};

class npc_argent_soldier : public CreatureScript
{
public:
    npc_argent_soldier() : CreatureScript("npc_argent_soldier") { }

    struct npc_argent_soldierAI : public npc_escortAI
    {
        npc_argent_soldierAI(Creature* Creature) : npc_escortAI(Creature)
        {
            Instance = Creature->GetInstanceScript();
            me->SetReactState(REACT_DEFENSIVE);
            if (GameObject* GO = GameObject::GetGameObject(*me, ObjectGuid(Instance->GetGuidData(GO_MAIN_GATE))))
                Instance->HandleGameObject(GO->GetGUID(), true);

            SetDespawnAtEnd(false);
            Waypoint = 0;
            me->setFaction(FACTION_FRIENDLY_TO_ALL);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        }

        InstanceScript* Instance;

        uint8 Waypoint;
        uint32 CleaveTimer;
        uint32 StrikeTimer;
        uint32 blazingLightTimer;
        uint32 FlurryTimer;
        uint32 PummelTimer;
        uint32 PainTimer;
        uint32 MindTimer;
        uint32 SsmiteTimer;
        uint32 FontTimer;
        uint32 LightTimer;

        uint32 FinalTimer;
        uint32 DivineTimer;
        uint32 ResetTimer;

        uint32 HolySmiteTimer;
        uint32 FountainTimer;
        bool Shielded;

        void Reset() override
        {
            CleaveTimer       = 10000;
            StrikeTimer       = 12000;
            blazingLightTimer = 9000;
            FlurryTimer       = 8000;
            PummelTimer       = 10000;
            Shielded          = false;
            PainTimer         = 8000;
            MindTimer         = 7000;
            SsmiteTimer       = 9000;
            ResetTimer        = 3000;
            LightTimer        = 2000;
            FontTimer         = 10000;

            FinalTimer        = 21000;
            DivineTimer       = 20000;
        }

        void WaypointReached(uint32 pointId) override
        {
            if (pointId == 0)
            {
                switch (Waypoint)
                {
                    case 0:
                        me->SetOrientation(5.81f);
                        break;
                    case 1:
                        me->SetOrientation(4.60f);
                        me->setFaction(FACTION_HOSTILE);
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
                        break;
                }
            }
            if (pointId == 1)
            {
                switch (Waypoint)
                {
                    case 0:
                        me->SetOrientation(5.81f);
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->setFaction(FACTION_HOSTILE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
                        break;
                    case 2:
                        me->SetOrientation(3.39f);
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->setFaction(FACTION_HOSTILE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
                        if (GameObject* GO = GameObject::GetGameObject(*me, ObjectGuid(Instance->GetGuidData(GO_MAIN_GATE))))
                            Instance->HandleGameObject(GO->GetGUID(),false);
                        break;
                }
                me->SendMovementFlagUpdate();
            }
        }

        void SetData(uint32 type, uint32 /*data*/) override
        {
            switch (me->GetEntry())
            {
                case NPC_ARGENT_LIGHWIELDER:
                    switch (type)
                    {
                        case 0:
                            AddWaypoint(0,737.14f,655.42f,412.88f);
                            AddWaypoint(1,712.14f,628.42f,411.88f);
                            break;
                        case 1:
                            AddWaypoint(0,742.44f,650.29f,411.79f);
                            break;
                        case 2:
                            AddWaypoint(0,756.14f,655.42f,411.88f);
                            AddWaypoint(1,781.626f, 629.383f, 411.892f);
                            break;
                    }
                    break;
                case NPC_ARGENT_MONK:
                    switch (type)
                    {
                        case 0:
                            AddWaypoint(0,737.14f,655.42f,412.88f);
                            AddWaypoint(1,713.12f,632.97f,411.90f);
                            break;
                        case 1:
                            AddWaypoint(0,746.73f,650.24f,411.56f);
                            break;
                        case 2:
                            AddWaypoint(0,756.14f,655.42f,411.88f);
                            AddWaypoint(1,781.351f, 633.146f, 411.907f);
                            break;
                    }
                    break;
                case NPC_PRIESTESS:
                    switch (type)
                    {
                        case 0:
                            AddWaypoint(0,737.14f,655.42f,412.88f);
                            AddWaypoint(1,715.06f,637.07f,411.91f);
                            break;
                        case 1:
                            AddWaypoint(0,750.72f,650.20f,411.77f);
                            break;
                        case 2:
                            AddWaypoint(0,756.14f,655.42f,411.88f);
                            AddWaypoint(1,780.439f, 636.681f, 411.918f);
                            break;
                    }
                    break;
            }

            Start(false, true, ObjectGuid::Empty);
            Waypoint = type;
        }

        void DamageTaken(Unit* /*who*/, uint32 &damage) override
        {
            if (me->GetEntry() == NPC_ARGENT_MONK && IsHeroic() && Shielded == false)
            {
                if (damage >= me->GetHealth())
                {
                    DoCastSelf(SPELL_DIVINE);
                    DoCastSelf(SPELL_FINAL);
                    me->SetHealth(1);
                    damage = 0;
                    Shielded = true;
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            npc_escortAI::UpdateAI(diff);

            if (!UpdateVictim())
                return;

            switch(me->GetEntry())
            {
                case NPC_ARGENT_LIGHWIELDER:
                {
                    if (CleaveTimer <= diff)
                    {
                        if (Unit* Target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            DoCast(Target, SPELL_STRIKE);
                            CleaveTimer = 12000;
                    } else
                          CleaveTimer -= diff;

                    if (StrikeTimer <= diff)
                    {
                        if (Unit* Target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            DoCast(Target, SPELL_CLEAVE);
                            StrikeTimer = 10000;
                    } else
                          StrikeTimer -= diff;

                    if (LightTimer <= diff)
                    {
                            DoCastSelf(SPELL_LIGHT);
                            LightTimer = urand(12000, 15000);
                    } else
                          LightTimer -= diff;
                    break;
                }
                case NPC_ARGENT_MONK:
                {
                    if (DivineTimer <= diff)
                    {
                            DoCastSelf(SPELL_DIVINE);
                            DivineTimer = 85000;
                    } else
                          DivineTimer -= diff;

                    if (FinalTimer <= diff)
                    {
                            DoCastSelf(SPELL_FINAL);
                            FinalTimer = 70000;
                    } else
                          FinalTimer -= diff;

                    if (PummelTimer <= diff)
                    {
                        if (Unit* Target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            DoCast(Target, SPELL_PUMMEL);
                            PummelTimer = 15000;
                    } else
                          PummelTimer -= diff;

                    if (FlurryTimer <= diff)
                    {
                            DoCastSelf(SPELL_FLURRY);
                            FlurryTimer = 15000;
                    } else
                          FlurryTimer -= diff;
                    break;
                }
                case NPC_PRIESTESS:
                {
                    if (FontTimer <= diff)
                    {
                            DoCastSelf(SPELL_FONT);
                            FontTimer = urand(15000, 17000);
                    } else
                          FontTimer -= diff;

                    if (PainTimer <= diff)
                    {
                        if (Unit* Target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            DoCast(Target,SPELL_PAIN);
                            PainTimer = 25000;
                    } else
                          PainTimer -= diff;

                    if (MindTimer <= diff)
                    {
                        if (Unit* Target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            DoCast(Target,SPELL_MIND);
                            MindTimer = 90000;
                    } else
                          MindTimer -= diff;

                    if (SsmiteTimer <= diff)
                    {
                        if (Unit* Target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            DoCast(Target,SPELL_SSMITE);
                            SsmiteTimer = 9000;
                    } else
                          SsmiteTimer -= diff;
                    break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            if (Instance)
                Instance->SetData(DATA_ARGENT_SOLDIER_DEFEATED, 0);
        }
    };

    CreatureAI* GetAI(Creature* Creature) const override
    {
        return new npc_argent_soldierAI(Creature);
    }
};

class OrientationCheck
{
public:
    explicit OrientationCheck(Unit* _caster) : caster(_caster) { }
    bool operator()(WorldObject* object)
    {
        return !object->isInFront(caster, 2.5f) || !object->IsWithinDist(caster, 40.0f);
    }

private:
    Unit* caster;
};

class spell_eadric_radiance : public SpellScriptLoader
{
public:
    spell_eadric_radiance() : SpellScriptLoader("spell_eadric_radiance") { }

    class spell_eadric_radiance_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_eadric_radiance_SpellScript);

        void FilterTargets(std::list<WorldObject*>& unitList)
        {
            unitList.remove_if(OrientationCheck(GetCaster()));
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_eadric_radiance_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_eadric_radiance_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_eadric_radiance_SpellScript();
    }
};

class spell_eadric_hammer_of_righteous : public SpellScriptLoader
{
public:
    spell_eadric_hammer_of_righteous() : SpellScriptLoader("spell_eadric_hammer_of_righteous") { }

    class spell_eadric_hammer_of_righteous_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_eadric_hammer_of_righteous_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return sSpellMgr->GetSpellInfo(SPELL_HAMMER_JUSTICE_AURA) && sSpellMgr->GetSpellInfo(SPELL_HAMMER_RIGHTEOUS_AURA);
        }

        bool Load() override
        {
            return GetCaster()->GetTypeId() == TYPEID_UNIT;
        }

        void HandleHammerOfRighteous(SpellEffIndex /*effIndex*/)
        {
            if (Unit* target = GetHitUnit())
                if (!target->HasAura(SPELL_HAMMER_JUSTICE_AURA))
                {
                    PreventHitDefaultEffect(EFFECT_0);
                    GetCaster()->CastSpell(target, SPELL_HAMMER_RIGHTEOUS_AURA, true);
                }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_eadric_hammer_of_righteous_SpellScript::HandleHammerOfRighteous, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_eadric_hammer_of_righteous_SpellScript();
    }
};

enum ReflectiveShield
{
    SPELL_REFLECTIVE_SHIELD_TRIGGERED = 33619,
};

// Reflective Shield 66515
class spell_gen_reflective_shield : public SpellScriptLoader
{
public:
    spell_gen_reflective_shield() : SpellScriptLoader("spell_gen_reflective_shield") { }

    class spell_gen_reflective_shield_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_gen_reflective_shield_AuraScript);

        bool Validate(SpellInfo const * /*spellEntry*/) override
        {
            return sSpellStore.LookupEntry(SPELL_REFLECTIVE_SHIELD_TRIGGERED);
        }

        void HandleAfterEffectAbsorb(AuraEffect* aurEff, DamageInfo & dmgInfo, uint32 & absorbAmount)
        {
            Unit* target = dmgInfo.GetAttacker();
            if (!target)
                return;
            Unit* caster = GetCaster();
            if (!caster)
                return;
            int32 bp = CalculatePct(absorbAmount, 25);
            target->CastCustomSpell(target, SPELL_REFLECTIVE_SHIELD_TRIGGERED, &bp, NULL, NULL, true, NULL, aurEff);
        }

        void Register() override
        {
            AfterEffectAbsorb += AuraEffectAbsorbFn(spell_gen_reflective_shield_AuraScript::HandleAfterEffectAbsorb, EFFECT_0);
        }
    };

    AuraScript *GetAuraScript() const override
    {
        return new spell_gen_reflective_shield_AuraScript();
    }
};

void AddSC_boss_argent_challenge()
{
    new boss_eadric;
    new boss_paletress;

    new npc_argent_soldier;

    new npc_memory;

    new spell_eadric_radiance;
    new spell_eadric_hammer_of_righteous();
    new spell_gen_reflective_shield;
}
