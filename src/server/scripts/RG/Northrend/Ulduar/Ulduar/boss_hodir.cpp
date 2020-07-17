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
#include "PassiveAI.h"
#include "ulduar.h"
#include "GridNotifiers.h"
#include "RG/Logging/LogManager.hpp"

// ###### Related Creatures/Object ######
enum HodirNPC
{
    NPC_ICE_BLOCK                                = 32938,
    NPC_FLASH_FREEZE                             = 32926,
    NPC_SNOWPACKED_ICICLE                        = 33174,
    NPC_ICICLE                                   = 33169,
    NPC_ICICLE_SNOWDRIFT                         = 33173,
    NPC_TOASTY_FIRE                              = 33342,
};

enum HodirGameObjects
{
    GO_TOASTY_FIRE                               = 194300,
    GO_SNOWDRIFT                                 = 194173,
};

// ###### Texts ######
enum HodirYells
{
    SAY_AGGRO                                    = 0,
    SAY_SLAY                                     = 1,
    SAY_FLASH_FREEZE                             = 2,
    SAY_STALACTITE                               = 3,
    SAY_DEATH                                    = 4,
    SAY_BERSERK                                  = 5,
    SAY_YS_HELP                                  = 9, // not mentioned in script
    SAY_HARD_MODE_FAILED                         = 6,
    // Emotes
    EMOTE_FREEZE                                 = 7,
    EMOTE_BLOWS                                  = 8
};

// ###### Datas ######
enum HodirActions
{
    ACTION_I_HAVE_THE_COOLEST_FRIENDS            = 1,
    ACTION_CHEESE_THE_FREEZE                     = 2,
    ACTION_GETTING_COLD_IN_HERE                  = 3,
};

enum Achievements
{
    ACHIEVEMENT_CHEESE_THE_FREEZE_10             = 2961,
    ACHIEVEMENT_CHEESE_THE_FREEZE_25             = 2962,
    ACHIEVEMENT_GETTING_COLD_IN_HERE_10          = 2967,
    ACHIEVEMENT_GETTING_COLD_IN_HERE_25          = 2968,
    ACHIEVEMENT_THIS_CACHE_WAS_RARE_10           = 3182,
    ACHIEVEMENT_THIS_CACHE_WAS_RARE_25           = 3184,
    ACHIEVEMENT_COOLEST_FRIENDS_10               = 2963,
    ACHIEVEMENT_COOLEST_FRIENDS_25               = 2965,
};

enum Misc
{
    MISC_FRIENDS_COUNT_10                        = 4,
    MISC_FRIENDS_COUNT_25                        = 8,
    MISC_ICICLE_TIMER_10                         = 5500,
    MISC_ICICLE_TIMER_25                         = 2000,
    MISC_ICICLE_TARGETS_10                       = 1,
    MISC_ICICLE_TARGETS_25                       = 1, // also 1 to handle not-simultaneos spawn together with a low value of MISC_ICICLE_TIMER_25
};

// ###### Event Controlling ######
enum HodirEvents
{
    // Hodir
    EVENT_FREEZE                                 = 1,
    EVENT_FLASH_FREEZE                           = 2,
    EVENT_FLASH_FREEZE_EFFECT                    = 3,
    EVENT_BLOWS                                  = 4,
    EVENT_RARE_CACHE                             = 5,
    EVENT_BERSERK                                = 6,

    // Priest
    EVENT_HEAL,
    EVENT_DISPEL_MAGIC,

    // Shaman
    EVENT_STORM_CLOUD,
    EVENT_END_DELAY,

    // Druid
    EVENT_STARLIGHT,

    // Mage
    EVENT_CONJURE_FIRE,
    EVENT_MELT_ICE
};

// ###### Spells ######
enum HodirSpells
{
    // Hodir
    SPELL_FROZEN_BLOWS                           = 62478,
    SPELL_FLASH_FREEZE                           = 61968, // Effect 1 Periodic Dummy Aura???, Effect 2 Send Event 20896
    SPELL_FLASH_FREEZE_VISUAL                    = 62148,
    SPELL_BITING_COLD                            = 62038,
    SPELL_BITING_COLD_TRIGGERED                  = 62039, // Needed for Achievement Getting Cold In Here
    SPELL_BITING_COLD_DAMAGE                     = 62188,
    SPELL_FREEZE                                 = 62469,
    SPELL_ICICLE                                 = 62234,
    SPELL_ICICLE_SNOWDRIFT                       = 62462,
    SPELL_BLOCK_OF_ICE                           = 61969, // Player + Helper
    SPELL_SUMMON_FLASH_FREEZE_HELPER             = 61989, // Helper
    SPELL_SUMMON_BLOCK_OF_ICE                    = 61970, // Player + Helper
    SPELL_FLASH_FREEZE_HELPER                    = 61990, // Helper
    SPELL_FLASH_FREEZE_KILL                      = 62226,
    SPELL_ICICLE_FALL                            = 69428,
    SPELL_FALL_DAMAGE                            = 62236,
    SPELL_FALL_SNOWDRIFT                         = 62460,
    SPELL_BERSERK                                = 47008,
    SPELL_ICE_SHARD                              = 62457,
    SPELL_ICE_SHARD_HIT                          = 65370,
    SPELL_SHATTER_CHEST                          = 65272, // timer aura to destroy hm chest
    SPELL_SHATTER_CHEST_TRIGGER                  = 62501,

    SPELL_CREDIT_MARKER                          = 64899,

    SPELL_ICICLE_PERIODIC_SUMMON                 = 62227,
    SPELL_FLASH_FREEZE_SUMMON_ICICLE             = 62476,

    // Druids
    SPELL_WRATH                                  = 62793,
    SPELL_STARLIGHT                              = 62807,

    // Shamans
    SPELL_LAVA_BURST                             = 61924,
    SPELL_STORM_CLOUD_SELECTOR                   = 62797,
    SPELL_STORM_CLOUD                            = 65123,
    SPELL_STORM_CLOUD_25                         = 65133,
    SPELL_STORM_CLOUD_BUFF                       = 63711,
    SPELL_STORM_CLOUD_BUFF_25                    = 65134,

    // Mages
    SPELL_FIREBALL                               = 61909,
    SPELL_CONJURE_FIRE                           = 62823,
    SPELL_MELT_ICE                               = 64528,
    SPELL_SINGED                                 = 62821,

    // Priests
    SPELL_SMITE                                  = 61923,
    SPELL_GREATER_HEAL                           = 62809,
    SPELL_DISPEL_MAGIC                           = 63499,
};

// ##### Additional Data #####
const Position SummonPositions[8] =
{
    { 1983.75f, -243.36f, 432.767f, 1.57f }, // Field Medic Penny    &&  Battle-Priest Eliza
    { 1999.90f, -230.49f, 432.767f, 1.57f }, // Eivi Nightfeather    &&  Tor Greycloud
    { 2010.06f, -243.45f, 432.767f, 1.57f }, // Elementalist Mahfuun &&  Spiritwalker Tara
    { 2021.12f, -236.65f, 432.767f, 1.57f }, // Missy Flamecuffs     &&  Amira Blazeweaver
    { 2028.10f, -244.66f, 432.767f, 1.57f }, // Field Medic Jessi    &&  Battle-Priest Gina
    { 2014.18f, -232.80f, 432.767f, 1.57f }, // Ellie Nightfeather   &&  Kar Greycloud
    { 1992.90f, -237.54f, 432.767f, 1.57f }, // Elementalist Avuun   &&  Spiritwalker Yona
    { 1976.60f, -233.53f, 432.767f, 1.57f }, // Sissy Flamecuffs     &&  Veesha Blazeweaver
};

const uint32 AllianzEntry[8] =
{
    NPC_FIELD_MEDIC_PENNY, NPC_EIVI_NIGHTFEATHER, NPC_ELEMENTALIST_MAHFUUN, NPC_MISSY_FLAMECUFFS,
    NPC_FIELD_MEDIC_JESSI, NPC_ELLIE_NIGHTFEATHER, NPC_ELEMENTALIST_AVUUN, NPC_SISSY_FLAMECUFFS,
};

const uint32 HordeEntry[8] =
{
    NPC_BATTLE_PRIEST_ELIZA, NPC_TOR_GREYCLOUD, NPC_SPIRITWALKER_TARA, NPC_AMIRA_BLAZEWEAVER,
    NPC_BATTLE_PRIEST_GINA, NPC_KAR_GREYCLOUD, NPC_SPIRITWALKER_YONA, NPC_VEESHA_BLAZEWEAVER,
};

class boss_hodir : public CreatureScript
{
    public:
        boss_hodir() : CreatureScript("boss_hodir") { }

        struct boss_hodirAI : public BossAI
        {
            boss_hodirAI(Creature* creature) : BossAI(creature, BOSS_HODIR)
            {
                ASSERT(instance);
                hodirDefeated = false;
            }

            void Reset()
            {
                BossAI::Reset();
                me->SetReactState(REACT_PASSIVE);
                me->SetHealth(me->GetMaxHealth());
                me->RemoveAurasDueToSpell(SPELL_BITING_COLD);
                me->RemoveAurasDueToSpell(SPELL_ICICLE_PERIODIC_SUMMON);

                DespawnFlashFreeze();

                for (uint8 n = 0; n < RAID_MODE(MISC_FRIENDS_COUNT_10, MISC_FRIENDS_COUNT_25); ++n)
                    if (Creature* FrozenHelper = me->SummonCreature(instance->GetData(DATA_RAID_IS_ALLIANZ) ? AllianzEntry[n] : HordeEntry[n], SummonPositions[n], TEMPSUMMON_MANUAL_DESPAWN))
                        FrozenHelper->CastSpell(FrozenHelper, SPELL_SUMMON_FLASH_FREEZE_HELPER, true);

                if (GameObject* chest = ObjectAccessor::GetGameObject(*me, ObjectGuid(instance->GetGuidData(DATA_HODIR_RARE_CACHE))))
                    chest->SetPhaseMask(PHASEMASK_NORMAL, true);
                if (GameObject* chest = ObjectAccessor::GetGameObject(*me, ObjectGuid(instance->GetGuidData(DATA_HODIR_RARE_CACHE_HERO))))
                    chest->SetPhaseMask(PHASEMASK_NORMAL, true);
            }

            void EnterCombat(Unit* who)
            {
                BossAI::EnterCombat(who);
                Talk(SAY_AGGRO);

                DoCastSelf(SPELL_BITING_COLD, true);
                DoCastSelf(SPELL_ICICLE_PERIODIC_SUMMON, true);
                DoCastSelf(SPELL_SHATTER_CHEST, true);

                GettingColdInHereTimer = 1000;
                AchievGettingColdInHere = true;
                AchievCheeseTheFreeze = true;
                AchievIHaveTheCoolestFriends = true;
                AchievICouldSayThatThisCacheWasRare = true;

                me->SetReactState(REACT_AGGRESSIVE);

                events.ScheduleEvent(EVENT_FREEZE,        25*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_BLOWS,         urand(60*IN_MILLISECONDS, 65*IN_MILLISECONDS));
                events.ScheduleEvent(EVENT_FLASH_FREEZE,  63*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_RARE_CACHE,   180*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_BERSERK,      540*IN_MILLISECONDS);
            }

            void KilledUnit(Unit* /*victim*/)
            {
                if (!urand(0, 3))
                    Talk(SAY_SLAY);
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                // sometimes DamageTaken is called several times even if boss is already friendly
                if (hodirDefeated)
                    return;

                if (damage >= me->GetHealth())
                {
                    damage = 0;
                    hodirDefeated = true;

                    Talk(SAY_DEATH);
                    if (AchievICouldSayThatThisCacheWasRare)
                    {
                        if (GameObject* chest = ObjectAccessor::GetGameObject(*me, ObjectGuid(instance->GetGuidData(DATA_HODIR_RARE_CACHE))))
                            chest->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        if (GameObject* chest = ObjectAccessor::GetGameObject(*me, ObjectGuid(instance->GetGuidData(DATA_HODIR_RARE_CACHE_HERO))))
                            chest->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        instance->DoCompleteAchievement(RAID_MODE(ACHIEVEMENT_THIS_CACHE_WAS_RARE_10, ACHIEVEMENT_THIS_CACHE_WAS_RARE_25));
                    }
                    DoCastSelf(SPELL_CREDIT_MARKER, true);

                    me->RemoveAllAuras();
                    me->RemoveAllAttackers();
                    me->AttackStop();
                    me->SetReactState(REACT_PASSIVE);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_REMOVE_CLIENT_CONTROL);
                    me->InterruptNonMeleeSpells(true);
                    me->StopMoving();
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MoveIdle();
                    me->SetControlled(true, UNIT_STATE_STUNNED);
                    me->CombatStop(true);

                    me->setFaction(35);
                    me->DespawnOrUnsummon(10000);

                    if (GameObject* chest = ObjectAccessor::GetGameObject(*me, ObjectGuid(instance->GetGuidData(DATA_HODIR_CHEST))))
                        chest->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    if (GameObject* chest = ObjectAccessor::GetGameObject(*me, ObjectGuid(instance->GetGuidData(DATA_HODIR_CHEST_HERO))))
                        chest->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);

                    if (AchievIHaveTheCoolestFriends)
                        instance->DoCompleteAchievement(RAID_MODE(ACHIEVEMENT_COOLEST_FRIENDS_10, ACHIEVEMENT_COOLEST_FRIENDS_25));
                    if (AchievCheeseTheFreeze)
                        instance->DoCompleteAchievement(RAID_MODE(ACHIEVEMENT_CHEESE_THE_FREEZE_10, ACHIEVEMENT_CHEESE_THE_FREEZE_25));
                    if (AchievGettingColdInHere)
                        instance->DoCompleteAchievement(RAID_MODE(ACHIEVEMENT_GETTING_COLD_IN_HERE_10, ACHIEVEMENT_GETTING_COLD_IN_HERE_25));

                    BossAI::JustDied(attacker);
                    RG_LOG<EncounterLogModule>(me);
                    me->KillSelf();
                }
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
                        case EVENT_FREEZE:
                            DoCastAOE(SPELL_FREEZE);
                            events.ScheduleEvent(EVENT_FREEZE, urand(30*IN_MILLISECONDS, 45*IN_MILLISECONDS));
                            break;
                        case EVENT_FLASH_FREEZE:
                            Talk(SAY_FLASH_FREEZE);
                            Talk(EMOTE_FREEZE);

                            DoCast(SPELL_FLASH_FREEZE_SUMMON_ICICLE);
                            DoCast(SPELL_FLASH_FREEZE);

                            events.ScheduleEvent(EVENT_FLASH_FREEZE, 63*IN_MILLISECONDS);
                            break;
                        case EVENT_BLOWS:
                            Talk(SAY_STALACTITE);
                            Talk(EMOTE_BLOWS);

                            DoCastSelf(SPELL_FROZEN_BLOWS);
                            events.ScheduleEvent(EVENT_BLOWS, urand(60*IN_MILLISECONDS, 65*IN_MILLISECONDS));
                            break;
                        case EVENT_RARE_CACHE:
                            Talk(SAY_HARD_MODE_FAILED);
                            AchievICouldSayThatThisCacheWasRare = false;
                            break;
                        case EVENT_BERSERK:
                            Talk(SAY_BERSERK);
                            DoCastSelf(SPELL_BERSERK, true);
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    case ACTION_I_HAVE_THE_COOLEST_FRIENDS:
                        AchievIHaveTheCoolestFriends = false;
                        break;
                    case ACTION_CHEESE_THE_FREEZE:
                        AchievCheeseTheFreeze = false;
                        break;
                    case ACTION_GETTING_COLD_IN_HERE:
                        AchievGettingColdInHere = false;
                        break;
                    default:
                        break;
                }
            }

            void DespawnFlashFreeze()
            {
                std::list<Creature*> creatures;
                me->GetCreatureListWithEntryInGrid(creatures, NPC_FLASH_FREEZE, 50.0f);
                me->GetCreatureListWithEntryInGrid(creatures, NPC_ICE_BLOCK,    50.0f);

                if (creatures.empty())
                    return;

                for (std::list<Creature*>::iterator itr = creatures.begin(); itr != creatures.end(); ++itr)
                    (*itr)->DespawnOrUnsummon();
            }

            private:
                uint32 GettingColdInHereTimer;
                bool AchievGettingColdInHere;
                bool AchievCheeseTheFreeze;
                bool AchievIHaveTheCoolestFriends;
                bool AchievICouldSayThatThisCacheWasRare;
                bool hodirDefeated;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_hodirAI(creature);
        };
};

//! 32926 Flash Freeze, for Player and Helper
class npc_flash_freeze : public CreatureScript
{
    public:
        npc_flash_freeze() : CreatureScript("npc_flash_freeze") { }

        struct npc_flash_freezeAI : public NullCreatureAI
        {
            npc_flash_freezeAI(Creature* creature) : NullCreatureAI(creature), Instance(me->GetInstanceScript())
            {
                ASSERT(Instance);
                me->SetDisplayId(me->GetCreatureTemplate()->Modelid2);
                me->SetReactState(REACT_PASSIVE);
            }

            void UpdateAI(uint32 diff)
            {
                if (CheckTimer <= diff)
                    if (!me->GetVictim())
                        me->SetReactState(REACT_PASSIVE);
                    else
                        CheckTimer = 1*IN_MILLISECONDS;
                else
                    CheckTimer -= diff;
            }

            void IsSummonedBy(Unit* summoner)
            {
                me->Attack(summoner, false);
                CheckTimer = 1*IN_MILLISECONDS;

                DoCast(summoner, SPELL_BLOCK_OF_ICE, true);

                if (summoner->GetTypeId() == TYPEID_PLAYER)
                    if (Creature* Hodir = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(BOSS_HODIR))))
                        Hodir->AI()->DoAction(ACTION_CHEESE_THE_FREEZE);
            }

            private:
                InstanceScript* Instance;
                uint32 CheckTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_flash_freezeAI(creature);
        }
};

//! 32938 Flash Freeze, for Helper
class npc_ice_block : public CreatureScript
{
    public:
        npc_ice_block() : CreatureScript("npc_ice_block") { }

        struct npc_ice_blockAI : public NullCreatureAI
        {
            npc_ice_blockAI(Creature* creature) : NullCreatureAI(creature), Instance(me->GetInstanceScript())
            {
                ASSERT(Instance);
                me->SetDisplayId(me->GetCreatureTemplate()->Modelid2);
                me->SetReactState(REACT_PASSIVE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            }

            void UpdateAI(uint32 diff)
            {
                if (CheckTimer <= diff)
                    if (!me->GetVictim())
                        me->SetReactState(REACT_PASSIVE);
                    else
                        CheckTimer = 1*IN_MILLISECONDS;
                else
                    CheckTimer -= diff;
            }

            void IsSummonedBy(Unit* summoner)
            {
                me->SetUInt64Value(UNIT_FIELD_SUMMONEDBY, summoner->GetGUID().GetRawValue());

                me->Attack(summoner, false);
                CheckTimer = 1*IN_MILLISECONDS;

                DoCast(summoner, SPELL_FLASH_FREEZE_HELPER, true);
            }

            void JustDied(Unit* /*killer*/)
            {
                if (Unit* owner = me->GetOwner())
                {
                    owner->RemoveAurasDueToSpell(SPELL_FLASH_FREEZE_HELPER);
                    if (Unit* hodir = ObjectAccessor::GetUnit(*me, ObjectGuid(Instance->GetGuidData(BOSS_HODIR))))
                        owner->GetAI()->AttackStart(hodir);
                }
            }

            void DamageTaken(Unit* who, uint32& /*damage*/)
            {
                if (Creature* hodir = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(BOSS_HODIR))))
                    if (!hodir->IsInCombat())
                    {
                        hodir->AI()->AttackStart(who);
                        hodir->SetInCombatWithZone();
                    }
            }

            private:
                InstanceScript* Instance;
                uint32 CheckTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_ice_blockAI(creature);
        }
};

class npc_icicle : public CreatureScript
{
    public:
        npc_icicle() : CreatureScript("npc_icicle") { }

        struct npc_icicleAI : public PassiveAI
        {
            npc_icicleAI(Creature* creature) : PassiveAI(creature)
            {
                me->SetDisplayId(me->GetCreatureTemplate()->Modelid1);
            }

            void Reset()
            {
                IcicleTimer = 4000;
            }

            void UpdateAI(uint32 diff)
            {
                if (IcicleTimer <= diff)
                {
                    DoCastSelf(SPELL_ICICLE_FALL);
                    switch (me->GetEntry())
                    {
                        case NPC_ICICLE_SNOWDRIFT:
                            DoCastSelf(SPELL_FALL_SNOWDRIFT, true);
                            break;
                        case NPC_ICICLE:
                            DoCastSelf(SPELL_FALL_DAMAGE, true);
                            break;
                    }
                    IcicleTimer = 10*IN_MILLISECONDS;
                }
                else
                    IcicleTimer -= diff;
            }

            private:
                uint32 IcicleTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_icicleAI(creature);
        };
};

class npc_snowpacked_icicle : public CreatureScript
{
    public:
        npc_snowpacked_icicle() : CreatureScript("npc_snowpacked_icicle") { }

        struct npc_snowpacked_icicleAI : public ScriptedAI
        {
            npc_snowpacked_icicleAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetDisplayId(me->GetCreatureTemplate()->Modelid2);
            }

            void Reset()
            {
                DespawnTimer = 12*IN_MILLISECONDS;
            }

            void UpdateAI(uint32 diff)
            {
                if (DespawnTimer <= diff)
                {
                    if (GameObject* snowdrift = me->FindNearestGameObject(GO_SNOWDRIFT, 2.0f))
                        snowdrift->Delete();
                    me->DespawnOrUnsummon();
                }
                else
                    DespawnTimer -= diff;
            }

            private:
                uint32 DespawnTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_snowpacked_icicleAI(creature);
        };
};

void ApplyHelperImmunities(Creature* me)
{
    me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_BITING_COLD_TRIGGERED, true);
    me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_ICICLE_SNOWDRIFT, true);
    me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_FALL_DAMAGE, true);
    me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_ICE_SHARD, true);
    me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_ICE_SHARD_HIT, true);
}

class npc_hodir_priest : public CreatureScript
{
    public:
        npc_hodir_priest() : CreatureScript("npc_hodir_priest") { }

        struct npc_hodir_priestAI : public ScriptedAI
        {
            npc_hodir_priestAI(Creature* creature) : ScriptedAI(creature), Instance(me->GetInstanceScript())
            {
                ASSERT(Instance);
                ApplyHelperImmunities(me);
            }

            void Reset()
            {
                events.Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                events.ScheduleEvent(EVENT_HEAL,         urand( 4*IN_MILLISECONDS,  8*IN_MILLISECONDS));
                events.ScheduleEvent(EVENT_DISPEL_MAGIC, urand(15*IN_MILLISECONDS, 20*IN_MILLISECONDS));
            }

            void JustDied(Unit* /*who*/)
            {
                if (Creature* hodir = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(BOSS_HODIR))))
                    hodir->AI()->DoAction(ACTION_I_HAVE_THE_COOLEST_FRIENDS);

                events.Reset();
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_STUNNED))
                    return;

                if (Instance->GetBossState(BOSS_HODIR) == DONE)
                    EnterEvadeMode();

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (HealthBelowPct(30))
                    DoCastSelf(SPELL_GREATER_HEAL);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_HEAL:
                            DoCastAOE(SPELL_GREATER_HEAL);
                            events.ScheduleEvent(EVENT_HEAL, urand(7500, 10000));
                            break;
                        case EVENT_DISPEL_MAGIC:
                            if (me->HasAura(SPELL_FREEZE))
                                DoCastAOE(SPELL_DISPEL_MAGIC);
                            events.ScheduleEvent(EVENT_DISPEL_MAGIC, urand(15*IN_MILLISECONDS, 20*IN_MILLISECONDS));
                            break;
                    }
                }

                DoSpellAttackIfReady(SPELL_SMITE);
            }

            bool CanAIAttack(Unit const* target) const override
            {
                return (me->GetPositionZ() < 429.0f) == (target->GetPositionZ() < 429.0f);
            }

        private:
            InstanceScript* Instance;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_hodir_priestAI(creature);
        };
};

class npc_hodir_shaman : public CreatureScript
{
    public:
        npc_hodir_shaman() : CreatureScript("npc_hodir_shaman") { }

        struct npc_hodir_shamanAI : public ScriptedAI
        {
            npc_hodir_shamanAI(Creature* creature) : ScriptedAI(creature), Instance(me->GetInstanceScript())
            {
                ASSERT(Instance);
                ApplyHelperImmunities(me);
            }

            void Reset()
            {
                events.Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                delay = false;
                events.ScheduleEvent(EVENT_STORM_CLOUD, urand(0, 2000));
            }

            void AttackStart(Unit* victim)
            {
                me->Attack(victim, false);
            }

            void JustDied(Unit* /*who*/)
            {
                if (Creature* hodir = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(BOSS_HODIR))))
                    hodir->AI()->DoAction(ACTION_I_HAVE_THE_COOLEST_FRIENDS);

                events.Reset();
            }

            bool ChaseIfNecessary(uint32 spell)
            {
                if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell))
                {
                    if (me->IsWithinCombatRange(me->GetVictim(), spellInfo->GetMaxRange(false)))
                        return false;
                    else
                        DoStartMovement(me->GetVictim(), 15.0f);
                }
                return true;
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_STUNNED))
                    return;

                if (Instance->GetBossState(BOSS_HODIR) == DONE)
                    EnterEvadeMode();

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_STORM_CLOUD:
                            DoCastAOE(SPELL_STORM_CLOUD_SELECTOR);
                            delay = true;
                            events.ScheduleEvent(EVENT_STORM_CLOUD, urand(30000, 35000));
                            events.ScheduleEvent(EVENT_END_DELAY, 3*IN_MILLISECONDS);
                            break;
                        case EVENT_END_DELAY:
                            delay = false;
                            break;
                    } 
                }

                if (!delay)
                {
                    if (!ChaseIfNecessary(SPELL_LAVA_BURST))
                        DoSpellAttackIfReady(SPELL_LAVA_BURST);
                }
            }

            bool CanAIAttack(Unit const* target) const override
            {
                return (me->GetPositionZ() < 429.0f) == (target->GetPositionZ() < 429.0f);
            }

        private:
            InstanceScript* Instance;
            EventMap events;
            bool delay;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_hodir_shamanAI(creature);
        };
};

class npc_hodir_druid : public CreatureScript
{
    public:
        npc_hodir_druid() : CreatureScript("npc_hodir_druid") { }

        struct npc_hodir_druidAI : public ScriptedAI
        {
            npc_hodir_druidAI(Creature* creature) : ScriptedAI(creature), Instance(me->GetInstanceScript())
            {
                ASSERT(Instance);
                ApplyHelperImmunities(me);
            }

            void Reset()
            {
                events.Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                events.ScheduleEvent(EVENT_STARLIGHT, urand(15000, 17500));
            }

            void JustDied(Unit* /*who*/)
            {
                if (Creature* hodir = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(BOSS_HODIR))))
                    hodir->AI()->DoAction(ACTION_I_HAVE_THE_COOLEST_FRIENDS);

                events.Reset();
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_STUNNED))
                    return;

                if (Instance->GetBossState(BOSS_HODIR) == DONE)
                    EnterEvadeMode();

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_STARLIGHT:
                            DoCast(SPELL_STARLIGHT);
                            events.ScheduleEvent(EVENT_STARLIGHT, urand(15000, 17500));
                            break;
                        default:
                            break;
                    }
                }

                DoSpellAttackIfReady(SPELL_WRATH);
            }

            bool CanAIAttack(Unit const* target) const override
            {
                return (me->GetPositionZ() < 429.0f) == (target->GetPositionZ() < 429.0f);
            }

        private:
            InstanceScript* Instance;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_hodir_druidAI(creature);
        };
};

class npc_hodir_mage : public CreatureScript
{
    public:
        npc_hodir_mage() : CreatureScript("npc_hodir_mage") { }

        struct npc_hodir_mageAI : public ScriptedAI
        {
            npc_hodir_mageAI(Creature* creature) : ScriptedAI(creature), Instance(me->GetInstanceScript()), summons(me)
            {
                ASSERT(Instance);
                ApplyHelperImmunities(me);
            }

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();
            }

            void EnterCombat(Unit* /*who*/)
            {
                events.ScheduleEvent(EVENT_CONJURE_FIRE, 2*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_MELT_ICE, 5*IN_MILLISECONDS);
            }

            void JustDied(Unit* /*who*/)
            {
                if (Creature* hodir = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(BOSS_HODIR))))
                    hodir->AI()->DoAction(ACTION_I_HAVE_THE_COOLEST_FRIENDS);

                events.Reset();
                summons.DespawnAll();
            }

            void JustSummoned(Creature* summoned)
            {
                if (summoned->GetEntry() == NPC_TOASTY_FIRE)
                    summons.Summon(summoned);
            }

            void SummonedCreatureDespawn(Creature* summoned)
            {
                if (summoned->GetEntry() == NPC_TOASTY_FIRE)
                    summons.Despawn(summoned);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_STUNNED))
                    return;

                if (Instance->GetBossState(BOSS_HODIR) == DONE)
                    EnterEvadeMode();

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CONJURE_FIRE:
                            DoCast(SPELL_CONJURE_FIRE);
                            events.ScheduleEvent(EVENT_CONJURE_FIRE, 14*IN_MILLISECONDS);
                            break;
                        case EVENT_MELT_ICE:
                            events.ScheduleEvent(EVENT_MELT_ICE, 1000);
                            if (Creature* FlashFreeze = me->FindNearestCreature(NPC_FLASH_FREEZE, 100.0f))
                                if (FlashFreeze->HasAura(SPELL_MELT_ICE))
                                    break;
                            std::list<Creature*> FlashFreeze;
                            me->GetCreatureListWithEntryInGrid(FlashFreeze, NPC_FLASH_FREEZE, 100.0f);
                            for (std::list<Creature*>::iterator itr = FlashFreeze.begin(); itr != FlashFreeze.end(); ++itr)
                                (*itr)->AddAura(SPELL_MELT_ICE, *itr);
                            break;
                    }
                }

                DoSpellAttackIfReady(SPELL_FIREBALL);
            }

            bool CanAIAttack(Unit const* target) const override
            {
                return (me->GetPositionZ() < 429.0f) == (target->GetPositionZ() < 429.0f);
            }

        private:
            InstanceScript* Instance;
            EventMap events;
            SummonList summons;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_hodir_mageAI(creature);
        };
};

class npc_toasty_fire : public CreatureScript
{
    public:
        npc_toasty_fire() : CreatureScript("npc_toasty_fire") { }

        struct npc_toasty_fireAI : public ScriptedAI
        {
            npc_toasty_fireAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetDisplayId(me->GetCreatureTemplate()->Modelid2);
            }

            void Reset()
            {
                DoCastSelf(SPELL_SINGED, true);
            }

            void SpellHit(Unit* /*who*/, const SpellInfo* spell)
            {
                if (spell->Id == SPELL_BLOCK_OF_ICE || spell->Id == SPELL_ICE_SHARD || spell->Id == SPELL_ICE_SHARD_HIT)
                {
                    if (GameObject* fire = me->FindNearestGameObject(GO_TOASTY_FIRE, 1.0f))
                        fire->Delete();
                    me->DespawnOrUnsummon();
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_toasty_fireAI(creature);
        };
};

class spell_biting_cold : public SpellScriptLoader
{
    public:
        spell_biting_cold() : SpellScriptLoader("spell_biting_cold") { }

        class spell_biting_cold_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_biting_cold_AuraScript);

            void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
            {
                Unit* target = GetTarget();
                bool found = false;

                for (TargetList::iterator itr = listOfTargets.begin(); itr != listOfTargets.end(); ++itr)
                {
                    if (ObjectGuid(itr->first) != target->GetGUID())
                        continue;

                    if (itr->second >= 4)
                    {
                        target->CastSpell(target, SPELL_BITING_COLD_TRIGGERED, true);
                        itr->second = 1;

                        if (target->GetAuraCount(SPELL_BITING_COLD_TRIGGERED) > 2 && target->GetMap()->IsDungeon())
                            if (Unit* hodir = ObjectAccessor::GetUnit(*target, ObjectGuid(target->GetInstanceScript()->GetGuidData(BOSS_HODIR))))
                                hodir->GetAI()->DoAction(ACTION_GETTING_COLD_IN_HERE);
                    }
                    else
                    {
                        if (target->isMoving())
                            itr->second = 1;
                        else
                            itr->second++;
                    }

                    found = true;
                    break;
                }

                if (!found)
                    listOfTargets.push_back(std::make_pair(target->GetGUID().GetRawValue(), 1));
            }

            bool CheckAreaTarget(Unit* target)
            {
                // Workaround: SPELL_ATTR3_ONLY_TARGET_PLAYERS
                return target->GetTypeId() == TYPEID_PLAYER;
            }

            void Register()
            {
                if (m_scriptSpellId == SPELL_BITING_COLD)
                    DoCheckAreaTarget += AuraCheckAreaTargetFn(spell_biting_cold_AuraScript::CheckAreaTarget);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_biting_cold_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }

        private:
            typedef std::list< std::pair<uint64, uint8> > TargetList;
            TargetList listOfTargets;
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_biting_cold_AuraScript();
        }
};

class spell_biting_cold_dot : public SpellScriptLoader
{
public:
    spell_biting_cold_dot() : SpellScriptLoader("spell_biting_cold_dot") { }

    class spell_biting_cold_dot_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_biting_cold_dot_AuraScript);

        void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            int32 damage = int32(200 * pow(2.0f, GetStackAmount()));
            caster->CastCustomSpell(caster, SPELL_BITING_COLD_DAMAGE, &damage, NULL, NULL, true);

            if (caster->isMoving())
                caster->RemoveAuraFromStack(SPELL_BITING_COLD_TRIGGERED);
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_biting_cold_dot_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_biting_cold_dot_AuraScript();
    }
};

class spell_storm_cloud_selector : public SpellScriptLoader
{
    public:
        spell_storm_cloud_selector() : SpellScriptLoader("spell_storm_cloud_selector") { }

        class spell_storm_cloud_selector_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_storm_cloud_selector_SpellScript);

            void OnHitTarget(SpellEffIndex /*effIndex*/)
            {
                if (GetCaster()->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL)
                    GetCaster()->CastSpell(GetHitUnit(), SPELL_STORM_CLOUD, false);
                else
                    GetCaster()->CastSpell(GetHitUnit(), SPELL_STORM_CLOUD_25, false);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_storm_cloud_selector_SpellScript::OnHitTarget, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_storm_cloud_selector_SpellScript();
        }
};

class spell_storm_cloud : public SpellScriptLoader
{
    public:
        spell_storm_cloud() : SpellScriptLoader("spell_storm_cloud") { }

        class spell_storm_cloud_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_storm_cloud_SpellScript);

            void ModAuraStack()
            {
                if (Aura* aura = GetHitAura())
                    aura->SetStackAmount(GetSpellInfo()->StackAmount);
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_storm_cloud_SpellScript::ModAuraStack);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_storm_cloud_SpellScript();
        }
};

class spell_storm_cloud_buff : public SpellScriptLoader
{
    public:
        spell_storm_cloud_buff() : SpellScriptLoader("spell_storm_cloud_buff") { }

        class spell_storm_cloud_buff_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_storm_cloud_buff_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if(Trinity::UnitAuraCheck(true, SPELL_STORM_CLOUD_BUFF));
                targets.remove_if(Trinity::UnitAuraCheck(true, SPELL_STORM_CLOUD_BUFF_25));
            }

            void OnHit(SpellEffIndex /*effIndex*/)
            {
                Aura* aura = GetCaster()->GetAura(SPELL_STORM_CLOUD);
                if (!aura)
                    aura = GetCaster()->GetAura(SPELL_STORM_CLOUD_25);
                if (aura)
                    aura->ModStackAmount(-1);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_storm_cloud_buff_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ALLY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_storm_cloud_buff_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ALLY);
                OnEffectHitTarget += SpellEffectFn(spell_storm_cloud_buff_SpellScript::OnHit, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_storm_cloud_buff_SpellScript();
        }
};

class FlashFreezeCheck
{
    public:
        bool operator() (WorldObject* object)
        {
            Unit* unit = object->ToUnit();

            if (!unit->IsAlive())   // Ignore dead units
                return true;

            if (unit->GetEntry() == NPC_TOASTY_FIRE)
                return false;

            if (unit->FindNearestCreature(NPC_SNOWPACKED_ICICLE, 5.0f)) // Icicle prevents freeze
                return true;

            for (uint8 i = 0; i < 8; ++i)   // Helper aren't excluded
                if (unit->GetEntry() == AllianzEntry[i] || unit->GetEntry() == HordeEntry[i])
                    return false;

            if (unit->GetTypeId() == TYPEID_PLAYER) // Players can always be hitten
                return false;

            return true;
        }
};

class spell_flash_freeze_targeting : public SpellScriptLoader
{
    public:
        spell_flash_freeze_targeting() : SpellScriptLoader("spell_flash_freeze_targeting") { }

        class spell_flash_freeze_targeting_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_flash_freeze_targeting_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if(FlashFreezeCheck());
            }

            void HandleVisual(SpellEffIndex effIndex)
            {
                std::list<Creature*> IcicleSnowdriftList;
                GetCaster()->GetCreatureListWithEntryInGrid(IcicleSnowdriftList, NPC_SNOWPACKED_ICICLE, 100.0f);

                for (std::list<Creature*>::iterator itr = IcicleSnowdriftList.begin(); itr != IcicleSnowdriftList.end(); ++itr)
                    (*itr)->CastSpell(*itr, SPELL_FLASH_FREEZE_VISUAL, true);

                PreventHitDefaultEffect(effIndex);
            }

            void HandleFreeze(SpellEffIndex /*effIndex*/)
            {
                if (Unit* target = GetHitUnit())
                {
                    if (target->HasAura(SPELL_FLASH_FREEZE_HELPER) || target->HasAura(SPELL_BLOCK_OF_ICE))
                        target->CastSpell(target, SPELL_FLASH_FREEZE_KILL, true);
                    else
                        target->CastSpell(target, SPELL_SUMMON_BLOCK_OF_ICE, true);
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_flash_freeze_targeting_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_flash_freeze_targeting_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
                OnEffectHitTarget += SpellEffectFn(spell_flash_freeze_targeting_SpellScript::HandleFreeze, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
                OnEffectHit += SpellEffectFn(spell_flash_freeze_targeting_SpellScript::HandleVisual, EFFECT_2, SPELL_EFFECT_SEND_EVENT);
            }

        };

        SpellScript* GetSpellScript() const
        {
            return new spell_flash_freeze_targeting_SpellScript();
        }
};

class spell_icicle_summon_trigger : public SpellScriptLoader
{
    public:
        spell_icicle_summon_trigger() : SpellScriptLoader("spell_icicle_summon_trigger") { }

        class spell_icicle_summon_trigger_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_icicle_summon_trigger_AuraScript);

            void HandleCalcPeriodic(AuraEffect const* /*aurEff*/, bool& isPeriodic, int32& amplitude)
            {
                if (Unit* owner = GetUnitOwner())
                {
                    isPeriodic = true;
                    if (owner->GetMap()->GetDifficulty() & RAID_DIFFICULTY_25MAN_NORMAL)
                        amplitude = MISC_ICICLE_TIMER_25;
                    else
                        amplitude = MISC_ICICLE_TIMER_10;
                }
            }

            void Register()
            {
                DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_icicle_summon_trigger_AuraScript::HandleCalcPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_icicle_summon_trigger_AuraScript();
        }
};

class spell_icicle_summon : public SpellScriptLoader
{
    public:
        spell_icicle_summon() : SpellScriptLoader("spell_icicle_summon") { }

        class spell_icicle_summon_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_icicle_summon_SpellScript);

            void SelectTargets(std::list<WorldObject*>& unitList)
            {
                if (Spell* spell = GetCaster()->GetCurrentSpell(CURRENT_GENERIC_SPELL))
                    if (spell->GetSpellInfo()->Id == SPELL_FLASH_FREEZE)
                    {
                        unitList.clear();
                        return;
                    }

                uint8 size = MISC_ICICLE_TARGETS_10;
                if (GetCaster()->GetMap()->GetDifficulty() & RAID_DIFFICULTY_25MAN_NORMAL)
                    size = MISC_ICICLE_TARGETS_25;

                Trinity::Containers::RandomResize(unitList, size);
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* target = GetHitUnit())
                    GetCaster()->CastSpell(target, GetSpellInfo()->Effects[EFFECT_0].CalcValue(), true);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_icicle_summon_SpellScript::SelectTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnEffectHitTarget += SpellEffectFn(spell_icicle_summon_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_icicle_summon_SpellScript();
        }
};

class spell_hodir_shatter_chest : public SpellScriptLoader
{
    public:
        spell_hodir_shatter_chest() : SpellScriptLoader("spell_hodir_shatter_chest") { }

        class spell_hodir_shatter_chest_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hodir_shatter_chest_AuraScript);

            void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                // only trigger the spell if the aura expires
                if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
                    return;

                GetUnitOwner()->ToCreature()->AI()->DoCast(SPELL_SHATTER_CHEST_TRIGGER);

                Unit* caster = GetCaster();
                if (InstanceScript* instance = caster->GetInstanceScript())
                { 
                    if (GameObject* chest = ObjectAccessor::GetGameObject(*caster, ObjectGuid(instance->GetGuidData(DATA_HODIR_RARE_CACHE))))
                        chest->SetPhaseMask(2, true);
                    if (GameObject* chest = ObjectAccessor::GetGameObject(*caster, ObjectGuid(instance->GetGuidData(DATA_HODIR_RARE_CACHE_HERO))))
                        chest->SetPhaseMask(2, true);
                }
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_hodir_shatter_chest_AuraScript::HandleOnEffectRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_hodir_shatter_chest_AuraScript();
        }
};

void AddSC_boss_hodir()
{
    new boss_hodir();

    new npc_icicle();
    new npc_snowpacked_icicle();
    new npc_ice_block();
    new npc_flash_freeze();
    new npc_hodir_priest();
    new npc_hodir_shaman();
    new npc_hodir_druid();
    new npc_hodir_mage();
    new npc_toasty_fire();
    new spell_biting_cold();
    new spell_biting_cold_dot();
    new spell_storm_cloud_selector();
    new spell_storm_cloud();
    new spell_storm_cloud_buff();
    new spell_flash_freeze_targeting();
    new spell_icicle_summon_trigger();
    new spell_icicle_summon();
    new spell_hodir_shatter_chest();
}
