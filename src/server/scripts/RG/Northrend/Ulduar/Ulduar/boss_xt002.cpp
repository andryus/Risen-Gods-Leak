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
#include "SpellAuraEffects.h"
#include "ulduar.h"
#include "Vehicle.h"
#include "PassiveAI.h"
#include "Player.h"
#include "DBCStores.h"

// ###### Related Creatures/Object ######
enum Creatures
{
    NPC_VOID_ZONE                               = 34001,
    NPC_LIFE_SPARK                              = 34004,
    NPC_XT002_HEART                             = 33329,
    NPC_XS013_SCRAPBOT                          = 33343,
    NPC_XM024_PUMMELLER                         = 33344,
    NPC_XE321_BOOMBOT                           = 33346,
};

// ###### Texts ######
enum Yells
{
    SAY_AGGRO                                   = 0,
    SAY_HEART_OPENED                            = 1,
    SAY_HEART_CLOSED                            = 2,
    SAY_TYMPANIC_TANTRUM                        = 3,
    SAY_SLAY                                    = 4,
    SAY_BERSERK                                 = 5,
    SAY_DEATH                                   = 6,
    SAY_SUMMON                                  = 7
};

// ###### Datas ######
enum Actions
{
    ACTION_ENTER_HARD_MODE          = 1,
    ACTION_HANDLE_SUMMON            = 2,
    ACTION_INCREASE_SCRAPBOT_COUNT  = 3
};

enum XT002Data
{
    DATA_HARD_MODE                  = 2,
    DATA_HEALTH_RECOVERED           = 3,
    DATA_GRAVITY_BOMB_CASUALTY      = 4,
};

enum AchievementCredits
{
    ACHIEV_MUST_DECONSTRUCT_FASTER  = 21027,
};

enum Seats
{
    HEART_VEHICLE_SEAT              = 0,
    HEART_VEHICLE_SEAT_EXPOSED      = 1,
};

enum Phases
{
    PHASE_NORMAL                     = 1,
    PHASE_HEART                      = 2,
};

// ###### Event Controlling ######
enum Events
{
    EVENT_TYMPANIC_TANTRUM      = 1,
    EVENT_SEARING_LIGHT         = 2,
    EVENT_GRAVITY_BOMB          = 3,
    EVENT_HEART_PHASE           = 4,
    EVENT_ENERGY_ORB            = 5,
    EVENT_DISPOSE_HEART         = 6,
    EVENT_ENRAGE                = 7,
    EVENT_ENTER_HARD_MODE       = 8,
    EVENT_EXPOSE_HEART          = 9,
    EVENT_TYMPANIC_TANTRUM_END  = 10,
    EVENT_NERF_SCRAPBOTS        = 11
};

enum EventGroups
{
    GROUP_EVENT_BOMBS                = 1,
};

enum Timers
{
    TIMER_TYMPANIC_TANTRUM                      = 60000,
    TIMER_TYMPANIC_TANTRUM_DIFF                 = 12000,
    TIMER_SEARING_LIGHT                         = 10000,
    TIMER_GRAVITY_BOMB                          = 10000,
    TIMER_HEART_PHASE                           = 30000,
    TIMER_ENERGY_ORB_MIN                        = 9000,
    TIMER_ENERGY_ORB_MAX                        = 10000,
    TIMER_ENRAGE                                = 600000,

    TIMER_VOID_ZONE                             = 3000,

    // Life Spark
    TIMER_SHOCK                                 = 8000,

    // Pummeller
    // Timers may be off
    TIMER_ARCING_SMASH                          = 27000,
    TIMER_TRAMPLE                               = 22000,
    TIMER_UPPERCUT                              = 17000,

    TIMER_SPAWN_ADD                             = 12000,
};

// ###### Spells ######
enum Spells
{
    SPELL_TYMPANIC_TANTRUM                      = 62776,
    SPELL_SEARING_LIGHT                         = 63018,

    SPELL_SUMMON_LIFE_SPARK                     = 64210,
    SPELL_SUMMON_VOID_ZONE                      = 64203,

    SPELL_GRAVITY_BOMB                          = 63024,

    SPELL_HEARTBREAK                            = 65737,   // Script effect seems to be Heal to 100%

    // Cast by 33337 at Heartbreak:
    SPELL_RECHARGE_PUMMELER                     = 62831,    // Summons 33344
    SPELL_RECHARGE_SCRAPBOT                     = 62828,    // Summons 33343
    SPELL_RECHARGE_BOOMBOT                      = 62835,    // Summons 33346

    SPELL_LEVITATE                              = 27986,    // It's only the levitate effect (no icon)

    // Cast by 33329 on 33337 (visual?)
    SPELL_ENERGY_ORB                            = 62790,    // Triggers 62826 - needs spellscript for periodic tick to cast one of the random spells above
    SPELL_ENERGY_ORB_TRIGGERD                   = 62826,

    SPELL_HEAL_TO_FULL                          = 17683,
    SPELL_HEART_OVERLOAD                        = 62789,
    SPELL_HEART_OVERLOAD_TRIGGER                = 62791,    // entry in spell_dbc Table

    SPELL_HEART_LIGHTNING_TETHER                = 64799,    // Cast on self?
    SPELL_HEART_RIDE_VEHICLE                    = 63313,
    SPELL_ENRAGE                                = 26662,
    SPELL_STAND                                 = 37752,
    SPELL_SUBMERGE                              = 37751,

    //------------------VOID ZONE--------------------
    SPELL_VOID_ZONE                             = 64203,
    SPELL_VOID_ZONE_DAMAGE                      = 46264,    // Damage is spezified in 64203 Effect1, no fitting aura found for damage

    // Life Spark
    SPELL_STATIC_CHARGED                        = 64227,
    SPELL_SHOCK                                 = 64230,
    SPELL_ARCANE_POWER_STATE                    = 49411,

    //----------------XT-002 HEART-------------------
    SPELL_EXPOSED_HEART                         = 63849,
    // Channeled

    //---------------XM-024 PUMMELLER----------------
    SPELL_ARCING_SMASH                          = 8374,
    SPELL_TRAMPLE                               = 5568,
    SPELL_UPPERCUT                              = 10966,

    // Scrabot:
    SPELL_SCRAPBOT_RIDE_VEHICLE                 = 47020,
    SPELL_SUICIDE                               = 7,

    //------------------BOOMBOT-----------------------
    SPELL_AURA_BOOMBOT                          = 65032,
    SPELL_BOOM                                  = 62834,

    // Achievement-related spells
    SPELL_ACHIEVEMENT_CREDIT_NERF_SCRAPBOTS     = 65037
};

// ##### Additional Data #####
const float BotChances[3] =
{
    0.80f, // Scrapbot
    0.04f, // Pummler
    0.15f, // Boombot
};

const uint32 IndexToSpell[3] =
{
    SPELL_RECHARGE_SCRAPBOT,
    SPELL_RECHARGE_PUMMELER,
    SPELL_RECHARGE_BOOMBOT,
};

uint8 EntryToIndex(uint32 entry)
{
    switch (entry)
    {
        case NPC_XE321_BOOMBOT:
            return 2;
        case NPC_XM024_PUMMELLER:
            return 1;
        case NPC_XS013_SCRAPBOT:
            return 0;
        default:
            return 0xFF;
    }
}

class ToyPileToNearCheck
{
    public:
        ToyPileToNearCheck(Unit* unit) : me(unit) {}

    bool operator() (Unit* unit)
    {
        if (me->GetDistance2d(unit) < 45.0f)
            return true;

        return false;
    }
    private:
        Unit* me;
};


struct VictimFilter
{
    VictimFilter(Unit* unit) : _unit(unit) {}

    bool operator()(WorldObject* target)
    {
        return target == _unit->GetVictim();
    }

private:
    Unit* _unit;
};

class boss_xt002 : public CreatureScript
{
    public:
        boss_xt002() : CreatureScript("boss_xt002") { }

        struct boss_xt002_AI : public BossAI
        {
            boss_xt002_AI(Creature *creature) : BossAI(creature, BOSS_XT002)
            {
                ASSERT(instance);
            }

            void Reset() override
            {
                BossAI::Reset();

                // Resets submerge state and makes XT selectable
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SERVER_CONTROLLED | UNIT_FLAG_NOT_SELECTABLE);
                me->SetByteValue(UNIT_FIELD_BYTES_1, 0, UNIT_STAND_STATE_STAND);
                me->SetReactState(REACT_AGGRESSIVE);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);

                HealthRecovered = false;
                GravityBombCasualty = false;
                HardMode = false;

                HeartExposed = 0;
                _scrapbotCount = 0;
                NextBots.clear();

                instance->DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_MUST_DECONSTRUCT_FASTER);
            }

            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);

                Talk(SAY_AGGRO);

                events.SetPhase(PHASE_NORMAL);

                if (urand(0, 1))
                    events.RescheduleEvent(EVENT_GRAVITY_BOMB, TIMER_GRAVITY_BOMB, GROUP_EVENT_BOMBS, PHASE_NORMAL);
                else
                    events.RescheduleEvent(EVENT_SEARING_LIGHT, TIMER_SEARING_LIGHT, GROUP_EVENT_BOMBS, PHASE_NORMAL);

                //Tantrum is casted a bit slower the first time.
                events.RescheduleEvent(EVENT_TYMPANIC_TANTRUM, TIMER_TYMPANIC_TANTRUM, 0, PHASE_NORMAL);

                events.RescheduleEvent(EVENT_ENRAGE, TIMER_ENRAGE);

                instance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_MUST_DECONSTRUCT_FASTER);

                me->CallForHelp(200.0f);
            }

            void DoAction(int32 action) override
            {
                switch (action)
                {
                    case ACTION_ENTER_HARD_MODE:
                        events.ScheduleEvent(EVENT_ENTER_HARD_MODE, 1000);
                        break;
                    case ACTION_HANDLE_SUMMON:
                        HandleSummonPhase();
                        break;
                    case ACTION_INCREASE_SCRAPBOT_COUNT:
                        if (!_scrapbotCount)
                            events.ScheduleEvent(EVENT_NERF_SCRAPBOTS, 12000);

                        _scrapbotCount++;

                        if (_scrapbotCount >= 20)
                            instance->DoCastSpellOnPlayers(SPELL_ACHIEVEMENT_CREDIT_NERF_SCRAPBOTS);
                        break;
                }
            }

            void KilledUnit(Unit* /*victim*/) override
            {
                Talk(SAY_SLAY);
            }

            void JustDied(Unit* killer) override
            {
                BossAI::JustDied(killer);

                NextBots.clear();
                Talk(SAY_DEATH);

                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SEARING_LIGHT);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_GRAVITY_BOMB);
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell) override
            {
                if (target->GetEntry() == NPC_XT_TOY_PILE && spell->Id == SPELL_ENERGY_ORB_TRIGGERD && !NextBots.empty())
                {
                    std::list<uint8>::iterator nextBot = NextBots.begin();

                    for (uint8 i = 0; i < 5; ++i) // we cast this several times to incease the summoncount
                        target->CastSpell(target, IndexToSpell[*nextBot], true);

                    NextBots.erase(nextBot);
                }
            }

            void JustSummoned(Creature* summon) override
            {
                summons.Summon(summon);

                switch (summon->GetEntry())
                {
                    case NPC_XS013_SCRAPBOT:
                        summon->GetMotionMaster()->MoveFollow(me, 0.0f, 0.0f);
                        break;
                    case NPC_XE321_BOOMBOT:
                    {
                        summon->SetInCombatWithZone();
                        float x, y, z;
                        me->GetRandomPoint(*me, 10.0f, x, y, z);
                        summon->GetMotionMaster()->MovePoint(0, x, y, z);
                        break;
                    }
                    case NPC_LIFE_SPARK:
                    case NPC_XM024_PUMMELLER:
                        summon->SetInCombatWithZone();
                        break;
                }
            }


            void DamageTaken(Unit* /*attacker*/, uint32& damage) override
            {
                if (HardMode)
                    return;

                if (damage >= me->GetHealth())
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SERVER_CONTROLLED | UNIT_FLAG_NOT_SELECTABLE);
                    me->SetByteValue(UNIT_FIELD_BYTES_1, 0, UNIT_STAND_STATE_STAND);
                    return;
                }

                if (me->HealthBelowPctDamaged(100 - 25 * (HeartExposed + 1), damage))
                    events.ScheduleEvent(EVENT_EXPOSE_HEART, 0);
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
                        case EVENT_SEARING_LIGHT:
                            DoCastAOE(SPELL_SEARING_LIGHT);
                            events.RescheduleEvent(EVENT_GRAVITY_BOMB, TIMER_GRAVITY_BOMB, GROUP_EVENT_BOMBS, PHASE_NORMAL);
                            break;
                        case EVENT_GRAVITY_BOMB:
                            DoCastAOE(SPELL_GRAVITY_BOMB);
                            events.RescheduleEvent(EVENT_SEARING_LIGHT, TIMER_SEARING_LIGHT, GROUP_EVENT_BOMBS, PHASE_NORMAL);
                            break;
                        case EVENT_TYMPANIC_TANTRUM:
                            Talk(SAY_TYMPANIC_TANTRUM);
                            DoCast(SPELL_TYMPANIC_TANTRUM);
                            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                            events.RescheduleEvent(EVENT_TYMPANIC_TANTRUM_END, 10 * IN_MILLISECONDS, 0, PHASE_NORMAL);
                            events.RescheduleEvent(EVENT_TYMPANIC_TANTRUM, TIMER_TYMPANIC_TANTRUM + TIMER_TYMPANIC_TANTRUM_DIFF, 0, PHASE_NORMAL);
                            break;
                        case EVENT_TYMPANIC_TANTRUM_END:
                            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
                            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
                            break;
                        case EVENT_DISPOSE_HEART:
                            SetPhaseOne();
                            break;
                        case EVENT_EXPOSE_HEART:
                            ExposeHeart();
                            break;
                        case EVENT_ENTER_HARD_MODE:
                            DoCastSelf(SPELL_HEAL_TO_FULL, true);
                            DoCastSelf(SPELL_HEARTBREAK, true);
                            me->AddLootMode(LOOT_MODE_HARD_MODE_1);
                            HardMode = true;
                            events.RescheduleEvent(EVENT_DISPOSE_HEART, 0);
                            break;
                        case EVENT_ENRAGE:
                            DoCastSelf(SPELL_ENRAGE, true);
                            break;
                        case EVENT_NERF_SCRAPBOTS:
                            _scrapbotCount = 0;
                            events.CancelEvent(EVENT_NERF_SCRAPBOTS);
                            break;
                    }
                }

                if (events.IsInPhase(PHASE_NORMAL))
                    DoMeleeAttackIfReady();
            }

            void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply) override
            {
                if (apply && who->GetEntry() == NPC_XS013_SCRAPBOT)
                {
                    who->DealHeal(me, me->CountPctFromMaxHealth(1));    //! is there a spell?
                    HealthRecovered = true;
                }
            }

            uint32 GetData(uint32 type) const
            {
                switch (type)
                {
                    case DATA_HARD_MODE:
                        return HardMode ? 1 : 0;
                    case DATA_HEALTH_RECOVERED:
                        return HealthRecovered ? 1 : 0;
                    case DATA_GRAVITY_BOMB_CASUALTY:
                        return GravityBombCasualty ? 1 : 0;
                }

                return 0;
            }

            void SetData(uint32 type, uint32 data) override
            {
                switch (type)
                {
                    case DATA_GRAVITY_BOMB_CASUALTY:
                        GravityBombCasualty = (data > 0) ? true : false;
                        break;
                }
            }

            void ExposeHeart()
            {
                Talk(SAY_HEART_OPENED);

                events.CancelEvent(EVENT_EXPOSE_HEART); //! Event can be in list multiple times

                DoCast(SPELL_SUBMERGE);  // Will make creature untargetable
                me->AttackStop();
                me->SetReactState(REACT_PASSIVE);

                NextBots.clear();

                if (Unit* heart = me->GetVehicleKit()->GetPassenger(HEART_VEHICLE_SEAT))
                {
                    heart->ChangeSeat(HEART_VEHICLE_SEAT_EXPOSED);
                    heart->CastSpell(heart, SPELL_HEAL_TO_FULL, true);
                    heart->CastSpell(me, SPELL_HEART_LIGHTNING_TETHER, false); // Visual link between XT and Heart
                    heart->CastSpell(heart, SPELL_HEART_OVERLOAD, false);
                    heart->CastSpell(heart, SPELL_EXPOSED_HEART, false);    // Channeled

                    heart->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                }

                events.SetPhase(PHASE_HEART);
                events.CancelEventGroup(GROUP_EVENT_BOMBS);

                events.ScheduleEvent(EVENT_DISPOSE_HEART, TIMER_HEART_PHASE, 0, PHASE_HEART);

                ++HeartExposed;
            }

            void SetPhaseOne()
            {
                Talk(SAY_HEART_CLOSED);

                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);                
                me->SetReactState(REACT_AGGRESSIVE);
                DoCast(SPELL_STAND);

                events.SetPhase(PHASE_NORMAL);
                events.CancelEvent(EVENT_DISPOSE_HEART);

                if(urand(0,1))
                    events.ScheduleEvent(EVENT_SEARING_LIGHT, TIMER_SEARING_LIGHT, GROUP_EVENT_BOMBS, PHASE_NORMAL);
                else
                    events.ScheduleEvent(EVENT_GRAVITY_BOMB, TIMER_GRAVITY_BOMB, GROUP_EVENT_BOMBS, PHASE_NORMAL);

                events.RescheduleEvent(EVENT_TYMPANIC_TANTRUM, TIMER_TYMPANIC_TANTRUM, 0, PHASE_NORMAL);

                if (Unit* heart = me->GetVehicleKit()->GetPassenger(HEART_VEHICLE_SEAT_EXPOSED))
                {
                    heart->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    heart->RemoveAllAurasExceptType(SPELL_AURA_CONTROL_VEHICLE);
                    heart->ChangeSeat(HEART_VEHICLE_SEAT);

                    if (!HardMode)
                    {
                        int32 transferHealth = int32(heart->GetMaxHealth() - heart->GetHealth());

                        me->LowerPlayerDamageReq(transferHealth);
                        heart->DealDamage(me, transferHealth);
                    }
                }
            }

            void HandleSummonPhase()
            {
                bool randomSummon[2]; //! Stores which Bots passed the chance roll
                uint8 count = 0;      //! Stores how many Bots where chosen
                //! Rolling for Bots
                for (uint8 i = 0; i < 2; ++i)
                {
                    randomSummon[i] = urand(0, 100) < uint32(BotChances[i] * 100);
                    count += randomSummon[i] ? 1 : 0;
                }

                //! Finally summoned Bot Type
                uint8 bot = 0;

                switch (count)
                {
                    default:
                    case 0: //! No bot selected, no bot summoned
                        for (uint8 i = 0; i < 2; ++i)
                            if (randomSummon[i])
                            {
                                bot = i;
                                break;
                            }
                        break;
                    case 1: //! Take the selected one
                        for (uint8 i = 0; i < 2; ++i)
                            if (randomSummon[i])
                            {
                                bot = i;
                                break;
                            }
                        break;
                    case 2: //! Roll again between the two, equal chanced here
                    {
                        uint8 one = 255;
                        uint8 two = 255;
                        for (uint8 i = 0; i < 2; ++i)
                            if (randomSummon[i])
                            {
                                if (one == 255)
                                    one = i;
                                else
                                    two = i;
                            }

                        if (urand(0, 100) <= 50)
                            bot = one;
                        else
                            bot = two;
                        break;
                    }
                }

                std::list<Creature*> toyPiles;
                me->GetCreatureListWithEntryInGrid(toyPiles, NPC_XT_TOY_PILE, 300.0f);

                toyPiles.remove_if(ToyPileToNearCheck(me));

                std::list<Creature*>::iterator choosen = toyPiles.begin();
                std::advance(choosen, urand(0, toyPiles.size() - 1));

                DoCast(*choosen, SPELL_ENERGY_ORB, true);

                NextBots.push_back(bot);

                if (NextBots.size() % 4 == 0)
                    Talk(SAY_SUMMON);
            }

            private:
                uint8 HeartExposed;
                std::list<uint8> NextBots;

                // Achievement related
                bool HealthRecovered;       // Did a scrapbot recover XT-002's health during the encounter?
                bool HardMode;              // Are we in hard mode? Or: was the heart killed during phase 2?
                bool GravityBombCasualty;   // Did someone die because of Gravity Bomb damage?
                uint8 _scrapbotCount;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_xt002_AI(creature);
        }
};

class boss_xt002_heart : public CreatureScript
{
    public:
        boss_xt002_heart() : CreatureScript("boss_xt002_heart") { }

        struct boss_xt002_heartAI : public ScriptedAI
        {
            boss_xt002_heartAI(Creature* creature) : ScriptedAI(creature), Instance(me->GetInstanceScript())
            {
                ASSERT(Instance);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                me->SetReactState(REACT_PASSIVE);
            }

            void Reset() override
            {
                if (!me->FindNearestCreature(NPC_XT002, 100.0f))
                    me->DespawnOrUnsummon();
            }

            void UpdateAI(uint32 /*diff*/) override
            {
                if (!me->HasAura(SPELL_EXPOSED_HEART))
                    DoCastSelf(SPELL_EXPOSED_HEART, false);
            }

            void JustDied(Unit* /*killer*/) override
            {
                if (Creature* vehicle = me->GetVehicleCreatureBase())
                    vehicle->GetAI()->DoAction(ACTION_ENTER_HARD_MODE);
                else if (Creature* xt002 =  ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(BOSS_XT002))))
                    xt002->GetAI()->DoAction(ACTION_ENTER_HARD_MODE);
            }

            private:
                InstanceScript* Instance;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_xt002_heartAI(creature);
        }
};

class npc_scrapbot : public CreatureScript
{
    public:
        npc_scrapbot() : CreatureScript("npc_scrapbot") { }

        struct npc_scrapbotAI : public ScriptedAI
        {
            npc_scrapbotAI(Creature* creature) : ScriptedAI(creature), Instance(me->GetInstanceScript())
            {
                ASSERT(Instance);
            }

            void Reset() override
            {
                if (rand32() % 100 < 35)
                {
                    me->SummonCreature(NPC_XE321_BOOMBOT, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_CORPSE_DESPAWN, 7000);
                    me->DespawnOrUnsummon();
                }
                me->SetReactState(REACT_PASSIVE);
                DoCast(SPELL_LEVITATE);
                me->SetSpeedRate(MOVE_FLIGHT, me->GetCreatureTemplate()->speed_walk);

                RangeCheckTimer = 500;

                if (Creature* xt002 = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(BOSS_XT002))))
                    xt002->AI()->JustSummoned(me);
            }

            void JustDied(Unit* killer) override
            {
                if (killer->GetEntry() == NPC_XE321_BOOMBOT)
                    if (Creature* xt002 = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(BOSS_XT002))))
                        xt002->AI()->DoAction(ACTION_INCREASE_SCRAPBOT_COUNT);
            }

            void UpdateAI(uint32 diff) override
            {
                if (RangeCheckTimer <= diff)
                {
                    if (Creature* xt002 = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(BOSS_XT002))))
                    {
                        if (me->IsWithinMeleeRange(xt002))
                        {
                            DoCast(xt002, SPELL_SCRAPBOT_RIDE_VEHICLE);
                            me->DespawnOrUnsummon();
                        }
                    }
                }
                else
                    RangeCheckTimer -= diff;
            }

            private:
                InstanceScript* Instance;
                uint32 RangeCheckTimer;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_scrapbotAI(creature);
        }
};

class npc_pummeller : public CreatureScript
{
    public:
        npc_pummeller() : CreatureScript("npc_pummeller") { }

        struct npc_pummellerAI : public ScriptedAI
        {
            npc_pummellerAI(Creature* creature) : ScriptedAI(creature), Instance(me->GetInstanceScript())
            {
                ASSERT(Instance);
            }

            void Reset() override
            {
                ArcingSmashTimer = TIMER_ARCING_SMASH;
                TrampleTimer = TIMER_TRAMPLE;
                UppercutTimer = TIMER_UPPERCUT;

                if (Creature* xt002 = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(BOSS_XT002))))
                    xt002->AI()->JustSummoned(me);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                if (ArcingSmashTimer <= diff)
                {
                    DoCastVictim(SPELL_ARCING_SMASH);
                    ArcingSmashTimer = TIMER_ARCING_SMASH;
                }
                else
                    ArcingSmashTimer -= diff;

                if (TrampleTimer <= diff)
                {
                    DoCastVictim(SPELL_TRAMPLE);
                    TrampleTimer = TIMER_TRAMPLE;
                }
                else
                    TrampleTimer -= diff;

                if (UppercutTimer <= diff)
                {
                    DoCastVictim(SPELL_UPPERCUT);
                    UppercutTimer = TIMER_UPPERCUT;
                }
                else
                    UppercutTimer -= diff;

                DoMeleeAttackIfReady();
            }

            private:
                InstanceScript* Instance;
                uint32 ArcingSmashTimer;
                uint32 TrampleTimer;
                uint32 UppercutTimer;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pummellerAI(creature);
        }
};

class npc_boombot : public CreatureScript
{
    public:
        npc_boombot() : CreatureScript("npc_boombot") { }

        struct npc_boombotAI : public ScriptedAI
        {
            npc_boombotAI(Creature* creature) : ScriptedAI(creature), Instance(me->GetInstanceScript())
            {
                ASSERT(Instance);
            }

            void Reset() override
            {
                DoCast(SPELL_AURA_BOOMBOT); // For achievement

                DoCast(SPELL_LEVITATE);
                me->SetSpeedRate(MOVE_FLIGHT, me->GetCreatureTemplate()->speed_walk);

                if (Creature* xt002 = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(BOSS_XT002))))
                    xt002->AI()->JustSummoned(me);
            }

            void JustDied(Unit* /*killer*/) override
            {
                DoCast(SPELL_BOOM);
            }

            void UpdateAI(uint32 /*diff*/) override
            { 
                if (!UpdateVictim())
                    return;
                // No melee attack
            }

           private:
                InstanceScript* Instance;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_boombotAI(creature);
        }
};

class npc_life_spark : public CreatureScript
{
    public:
        npc_life_spark() : CreatureScript("npc_life_spark") { }

        struct npc_life_sparkAI : public ScriptedAI
        {
            npc_life_sparkAI(Creature* creature) : ScriptedAI(creature) {}

            void Reset() override
            {
                DoCastSelf(SPELL_STATIC_CHARGED);
                DoCastSelf(SPELL_ARCANE_POWER_STATE, true);
                ShockTimer = 0;
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                if (ShockTimer <= diff)
                {
                    DoCastVictim(SPELL_SHOCK);
                    ShockTimer = TIMER_SHOCK;
                }
                else ShockTimer -= diff;

                DoMeleeAttackIfReady();
            }

            private:
                uint32 ShockTimer;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_life_sparkAI(creature);
        }
};

//! This script is a complete hack
//! There seems to be no correct aura, with correct damage for Void Zones
class npc_void_zone : public CreatureScript
{
    public:
        npc_void_zone() : CreatureScript("npc_void_zone") { }

        struct npc_void_zoneAI : public PassiveAI
        {
            npc_void_zoneAI(Creature* creature) : PassiveAI(creature) {}

            void Reset() override
            {
                me->SetReactState(REACT_PASSIVE);
                VoidDamgeTimer = 1000;
            }

            void UpdateAI(uint32 diff) override
            {
                if (VoidDamgeTimer <= diff)
                {
                    me->CastCustomSpell(SPELL_VOID_ZONE_DAMAGE, SPELLVALUE_BASE_POINT0, me->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL ? 5000 : 7500);
                    VoidDamgeTimer = 1000;
                }
                else VoidDamgeTimer -= diff;
            }

            private:
                uint32 VoidDamgeTimer;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_void_zoneAI(creature);
        }
};

class spell_xt002_searing_light_spawn_life_spark : public SpellScriptLoader
{
    public:
        spell_xt002_searing_light_spawn_life_spark() : SpellScriptLoader("spell_xt002_searing_light_spawn_life_spark") { }

        class spell_xt002_searing_light_spawn_life_spark_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_xt002_searing_light_spawn_life_spark_AuraScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellStore.LookupEntry(SPELL_SUMMON_LIFE_SPARK))
                    return false;
                return true;
            }

            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Player* player = GetOwner()->ToPlayer())
                    if (Unit* xt002 = GetCaster())
                        if (xt002->HasAura(aurEff->GetAmount()))   // Heartbreak aura indicating hard mode
                            xt002->CastSpell(player, SPELL_SUMMON_LIFE_SPARK, true);
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_xt002_searing_light_spawn_life_spark_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_xt002_searing_light_spawn_life_spark_AuraScript();
        }
};

class spell_xt002_gravity_bomb_aura : public SpellScriptLoader
{
    public:
        spell_xt002_gravity_bomb_aura() : SpellScriptLoader("spell_xt002_gravity_bomb_aura") { }

        class spell_xt002_gravity_bomb_aura_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_xt002_gravity_bomb_aura_AuraScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellStore.LookupEntry(SPELL_SUMMON_VOID_ZONE))
                    return false;
                return true;
            }

            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
                    if (Player* player = GetOwner()->ToPlayer())
                        if (Unit* xt002 = GetCaster())
                            if (xt002->HasAura(aurEff->GetAmount()))   // Heartbreak aura indicating hard mode
                                xt002->CastSpell(player, SPELL_SUMMON_VOID_ZONE, true);
            }

            void OnPeriodic(AuraEffect const* aurEff)
            {
                Unit* xt002 = GetCaster();
                if (!xt002)
                    return;

                Unit* owner = GetOwner()->ToUnit();
                if (!owner)
                    return;

                if (aurEff->GetAmount() >= int32(owner->GetHealth()))
                    if (xt002->GetAI())
                        xt002->GetAI()->SetData(DATA_GRAVITY_BOMB_CASUALTY, 1);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_xt002_gravity_bomb_aura_AuraScript::OnPeriodic, EFFECT_2, SPELL_AURA_PERIODIC_DAMAGE);
                AfterEffectRemove += AuraEffectRemoveFn(spell_xt002_gravity_bomb_aura_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_xt002_gravity_bomb_aura_AuraScript();
        }
};

class spell_xt002_gravity_bomb_damage : public SpellScriptLoader
{
    public:
        spell_xt002_gravity_bomb_damage() : SpellScriptLoader("spell_xt002_gravity_bomb_damage") { }

        class spell_xt002_gravity_bomb_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_xt002_gravity_bomb_damage_SpellScript);

            void HandleScript(SpellEffIndex /*eff*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                if (GetHitDamage() >= int32(GetHitUnit()->GetHealth()))
                    if (caster->GetAI())
                        caster->GetAI()->SetData(DATA_GRAVITY_BOMB_CASUALTY, 1);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_xt002_gravity_bomb_damage_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_xt002_gravity_bomb_damage_SpellScript();
        }
};

class spell_xt002_heart_overload_periodic : public SpellScriptLoader
{
    public:
        spell_xt002_heart_overload_periodic() : SpellScriptLoader("spell_xt002_heart_overload_periodic") { }

        class spell_xt002_heart_overload_periodic_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_xt002_heart_overload_periodic_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                    if (caster->GetVehicleBase())
                        caster->GetVehicleBase()->GetAI()->DoAction(ACTION_HANDLE_SUMMON);
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_xt002_heart_overload_periodic_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_xt002_heart_overload_periodic_SpellScript();
        }
};

class spell_xt002_tympanic_tantrum : public SpellScriptLoader
{
    public:
        spell_xt002_tympanic_tantrum() : SpellScriptLoader("spell_xt002_tympanic_tantrum") { }

        class spell_xt002_tympanic_tantrum_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_xt002_tympanic_tantrum_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if(PlayerOrPetCheck());
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_xt002_tympanic_tantrum_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_xt002_tympanic_tantrum_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_xt002_tympanic_tantrum_SpellScript();
        }
};

class spell_xt002_tympanic_tantrum_damage : public SpellScriptLoader
{
    public:
        spell_xt002_tympanic_tantrum_damage() : SpellScriptLoader("spell_xt002_tympanic_tantrum_damage") { }

        class spell_xt002_tympanic_tantrum_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_xt002_tympanic_tantrum_damage_SpellScript);

            void HandleDamage(SpellEffIndex /*effIndex*/)
            {
                Unit* target = GetHitUnit();
                if (!target)
                    return;

                SetHitDamage(target->SpellDamageBonusTaken(GetCaster(), GetSpellInfo(), CalculatePct(target->GetMaxHealth(), GetSpellInfo()->Effects[EFFECT_0].CalcValue()), SPELL_DIRECT_DAMAGE));
            }

            void Register()
            {
               OnEffectHitTarget += SpellEffectFn(spell_xt002_tympanic_tantrum_damage_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_xt002_tympanic_tantrum_damage_SpellScript();
        }
};

class spell_xt002_submerged : public SpellScriptLoader
{
    public:
        spell_xt002_submerged() : SpellScriptLoader("spell_xt002_submerged") { }

        class spell_xt002_submerged_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_xt002_submerged_SpellScript);

            void HandleScript(SpellEffIndex /*eff*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                caster->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SERVER_CONTROLLED | UNIT_FLAG_NOT_SELECTABLE);
                caster->SetByteValue(UNIT_FIELD_BYTES_1, 0, UNIT_STAND_STATE_SUBMERGED);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_xt002_submerged_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_xt002_submerged_SpellScript();
        }
};

class spell_xt002_bomb_selector : public SpellScriptLoader
{
    public:
        spell_xt002_bomb_selector() : SpellScriptLoader("spell_xt002_bomb_selector") { }

        class spell_xt002_bomb_selector_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_xt002_bomb_selector_SpellScript);

            void SelectTarget(std::list<WorldObject*>& targets)
            {
                targets.remove_if(VictimFilter(GetCaster()));
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_xt002_bomb_selector_SpellScript::SelectTarget, EFFECT_ALL, TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_xt002_bomb_selector_SpellScript();
        }
};

class achievement_nerf_engineering : public AchievementCriteriaScript
{
    public:
        achievement_nerf_engineering() : AchievementCriteriaScript("achievement_nerf_engineering") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (!target || !target->GetAI())
                return false;

            return !(target->GetAI()->GetData(DATA_HEALTH_RECOVERED));
        }
};

class achievement_heartbreaker : public AchievementCriteriaScript
{
    public:
        achievement_heartbreaker() : AchievementCriteriaScript("achievement_heartbreaker") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (!target || !target->GetAI())
                return false;

            return target->GetAI()->GetData(DATA_HARD_MODE);
        }
};

class achievement_nerf_gravity_bombs : public AchievementCriteriaScript
{
    public:
        achievement_nerf_gravity_bombs() : AchievementCriteriaScript("achievement_nerf_gravity_bombs") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (!target || !target->GetAI())
                return false;

            return !(target->GetAI()->GetData(DATA_GRAVITY_BOMB_CASUALTY));
        }
};

void AddSC_boss_xt002()
{
    new boss_xt002();
    new boss_xt002_heart();

    new npc_scrapbot();
    new npc_pummeller();
    new npc_boombot();
    new npc_life_spark();
    new npc_void_zone();

    new spell_xt002_searing_light_spawn_life_spark();
    new spell_xt002_gravity_bomb_aura();
    new spell_xt002_gravity_bomb_damage();
    new spell_xt002_heart_overload_periodic();
    new spell_xt002_tympanic_tantrum();
    new spell_xt002_tympanic_tantrum_damage();
    new spell_xt002_submerged();
    new spell_xt002_bomb_selector();

    new achievement_nerf_engineering();
    new achievement_heartbreaker();
    new achievement_nerf_gravity_bombs();
}
