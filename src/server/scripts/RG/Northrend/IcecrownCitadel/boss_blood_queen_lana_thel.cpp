/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "GridNotifiers.h"
#include "icecrown_citadel.h"

enum Texts
{
    SAY_AGGRO                   = 0,
    SAY_VAMPIRIC_BITE           = 1,
    SAY_MIND_CONTROL            = 2,
    EMOTE_BLOODTHIRST           = 3,
    SAY_SWARMING_SHADOWS        = 4,
    EMOTE_SWARMING_SHADOWS      = 5,
    SAY_PACT_OF_THE_DARKFALLEN  = 6,
    SAY_AIR_PHASE               = 7,
    SAY_KILL                    = 8,
    SAY_WIPE                    = 9,
    SAY_BERSERK                 = 10,
    SAY_DEATH                   = 11,
    EMOTE_BERSERK_RAID          = 12
};

enum Spells
{
    SPELL_SHROUD_OF_SORROW                  = 70986,
    SPELL_FRENZIED_BLOODTHIRST_VISUAL       = 71949,
    SPELL_VAMPIRIC_BITE                     = 71726,
    SPELL_VAMPIRIC_BITE_DUMMY               = 71837,
    SPELL_ESSENCE_OF_THE_BLOOD_QUEEN_PLR    = 70879,
    SPELL_ESSENCE_OF_THE_BLOOD_QUEEN_HEAL   = 70872,
    SPELL_FRENZIED_BLOODTHIRST              = 70877,
    SPELL_UNCONTROLLABLE_FRENZY             = 70923,
    SPELL_PRESENCE_OF_THE_DARKFALLEN        = 70995,
    SPELL_PRESENCE_OF_THE_DARKFALLEN_2      = 71952,
    SPELL_BLOOD_MIRROR_DAMAGE               = 70821,
    SPELL_BLOOD_MIRROR_VISUAL               = 71510,
    SPELL_BLOOD_MIRROR_DUMMY                = 70838,
    SPELL_DELIRIOUS_SLASH                   = 71623,
    SPELL_PACT_OF_THE_DARKFALLEN_TARGET     = 71336,
    SPELL_PACT_OF_THE_DARKFALLEN            = 71340,
    SPELL_PACT_OF_THE_DARKFALLEN_DAMAGE     = 71341,
    SPELL_SWARMING_SHADOWS                  = 71264,
    SPELL_TWILIGHT_BLOODBOLT_TARGET         = 71445,
    SPELL_TWILIGHT_BLOODBOLT                = 71818,
    SPELL_TWILIGHT_BLOODBOLT_AIR            = 71446,
    SPELL_INCITE_TERROR                     = 73070,
    SPELL_BLOODBOLT_WHIRL                   = 71772,
    SPELL_ANNIHILATE                        = 71322,
    SPELL_ESSENCE_OF_THE_BLOODQUEEN         = 70871,

    // Blood Infusion
    SPELL_BLOOD_INFUSION_CREDIT             = 72934
};

enum Shadowmourne
{
    QUEST_BLOOD_INFUSION                    = 24756,

    SPELL_GUSHING_WOUND                     = 72132,
    SPELL_THIRST_QUENCHED                   = 72154,
};

uint32 const vampireAuras[3][MAX_DIFFICULTY] =
{
    {70867, 71473, 71532, 71533},
    {70879, 71525, 71530, 71531},
    {70877, 71474, 70877, 71474},
};

#define ESSENCE_OF_BLOOD_QUEEN      RAID_MODE<uint32>(70867, 71473, 71532, 71533)
#define ESSENCE_OF_BLOOD_QUEEN_PLR  RAID_MODE<uint32>(70879, 71525, 71530, 71531)
#define FRENZIED_BLOODTHIRST        RAID_MODE<uint32>(70877, 71474, 70877, 71474)
#define DELIRIOUS_SLASH             RAID_MODE<uint32>(71623, 71624, 71625, 71626)
#define PRESENCE_OF_THE_DARKFALLEN  RAID_MODE<uint32>(70994, 71962, 71963, 71964)

enum Events
{
    EVENT_BERSERK                   = 1,
    EVENT_VAMPIRIC_BITE             = 2,
    EVENT_BLOOD_MIRROR              = 3,
    EVENT_DELIRIOUS_SLASH           = 4,
    EVENT_PACT_OF_THE_DARKFALLEN    = 5,
    EVENT_SWARMING_SHADOWS          = 6,
    EVENT_TWILIGHT_BLOODBOLT        = 7,
    EVENT_AIR_PHASE                 = 8,
    EVENT_AIR_START_FLYING          = 9,
    EVENT_AIR_FLY_DOWN              = 10,

    EVENT_GROUP_NORMAL              = 1,
    EVENT_GROUP_CANCELLABLE         = 2,
};

enum Guids
{
    GUID_VAMPIRE    = 1,
    GUID_BLOODBOLT  = 2,
};

enum Points
{
    POINT_CENTER    = 1,
    POINT_AIR       = 2,
    POINT_GROUND    = 3,
    POINT_MINCHAR   = 4,
};

Position const centerPos  = {4595.7090f, 2769.4190f, 400.6368f, 0.000000f};
Position const airPos     = {4595.7090f, 2769.4190f, 422.3893f, 0.000000f};
Position const mincharPos = {4629.3711f, 2782.6089f, 424.6390f, 0.000000f};

bool IsVampire(Unit const* unit)
{
    for (uint8 i = 0; i < 3; ++i)
        if (unit->HasAura(vampireAuras[i][unit->GetMap()->GetSpawnMode()]))
            return true;
    return false;
}

class boss_blood_queen_lana_thel : public CreatureScript
{
    public:
        boss_blood_queen_lana_thel() : CreatureScript("boss_blood_queen_lana_thel") { }

        struct boss_blood_queen_lana_thelAI : public BossAI
        {
            boss_blood_queen_lana_thelAI(Creature* creature) : BossAI(creature, DATA_BLOOD_QUEEN_LANA_THEL)
            {
            }

            void Reset()
            {
                BossAI::Reset();
                me->SetReactState(REACT_DEFENSIVE);
                events.ScheduleEvent(EVENT_BERSERK,                          320 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_VAMPIRIC_BITE,                     15 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_BLOOD_MIRROR,                     2.5 * IN_MILLISECONDS, EVENT_GROUP_CANCELLABLE);
                events.ScheduleEvent(EVENT_DELIRIOUS_SLASH,        urand(20, 24) * IN_MILLISECONDS, EVENT_GROUP_NORMAL);
                events.ScheduleEvent(EVENT_PACT_OF_THE_DARKFALLEN,            15 * IN_MILLISECONDS, EVENT_GROUP_NORMAL);
                events.ScheduleEvent(EVENT_SWARMING_SHADOWS,                30.5 * IN_MILLISECONDS, EVENT_GROUP_NORMAL);
                events.ScheduleEvent(EVENT_TWILIGHT_BLOODBOLT,     urand(20, 25) * IN_MILLISECONDS, EVENT_GROUP_NORMAL);
                events.ScheduleEvent(EVENT_AIR_PHASE,                        135 * IN_MILLISECONDS);
                if (!IsHeroic())
                    me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_PRESENCE_OF_THE_DARKFALLEN, true);
                ClearAuras();
                SetTrashReaction(FACTION_HOSTILE);
                _offtank = NULL;
                _vampires.clear();
                _creditBloodQuickening = false;
                _killMinchar = false;
                Airphase = false;
            }

            void EnterCombat(Unit* who)
            {
                if (!instance->CheckRequiredBosses(DATA_BLOOD_QUEEN_LANA_THEL, who->ToPlayer()))
                {
                    EnterEvadeMode(EVADE_REASON_OTHER);
                    instance->DoCastSpellOnPlayers(LIGHT_S_HAMMER_TELEPORT);
                    return;
                }

                me->setActive(true);
                DoZoneInCombat();
                Talk(SAY_AGGRO);
                instance->SetBossState(DATA_BLOOD_QUEEN_LANA_THEL, IN_PROGRESS);
                ClearAuras();
                SetTrashReaction(FACTION_FRIENDLY_TO_ALL);

                DoCastSelf(SPELL_SHROUD_OF_SORROW, true);
                DoCastSelf(SPELL_FRENZIED_BLOODTHIRST_VISUAL, true);
                _creditBloodQuickening = instance->GetData(DATA_BLOOD_QUICKENING_STATE) == IN_PROGRESS;
            }

            void JustDied(Unit* killer)
            {
                BossAI::JustDied(killer);
                Talk(SAY_DEATH);

                if (Is25ManRaid() && me->HasAura(SPELL_SHADOWS_FATE))
                    DoCastAOE(SPELL_BLOOD_INFUSION_CREDIT, true);

                if (IsHeroic())
                {
                    if (instance->GetData(DATA_BLOOD_QUEEN_LANA_THEL_HC) == 0)
                        instance->SetData(DATA_BLOOD_QUEEN_LANA_THEL_HC, 1);
                }

                ClearAuras();
                SetTrashReaction(FACTION_HOSTILE);
                // Leave Combat
                for (Player& player : me->GetMap()->GetPlayers())
                    player.CombatStop();

                // credit the quest
                if (_creditBloodQuickening)
                {
                    instance->SetData(DATA_BLOOD_QUICKENING_STATE, DONE);
                    if (Player* player = killer->ToPlayer())
                        player->RewardPlayerAndGroupAtEvent(Is25ManRaid() ? NPC_INFILTRATOR_MINCHAR_BQ_25 : NPC_INFILTRATOR_MINCHAR_BQ, player);
                    if (Creature* minchar = me->FindNearestCreature(NPC_INFILTRATOR_MINCHAR_BQ, 200.0f))
                    {
                        minchar->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
                        minchar->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                        minchar->ForbidInhabit(INHABIT_AIR);
                        minchar->RemoveAllAuras();
                        minchar->GetMotionMaster()->MoveCharge(4629.3711f, 2782.6089f, 401.5301f, SPEED_CHARGE/3.0f);
                    }
                }
            }

            void ClearAuras()
            {
                instance->DoRemoveAurasDueToSpellOnPlayers(ESSENCE_OF_BLOOD_QUEEN);
                instance->DoRemoveAurasDueToSpellOnPlayers(ESSENCE_OF_BLOOD_QUEEN_PLR);
                instance->DoRemoveAurasDueToSpellOnPlayers(FRENZIED_BLOODTHIRST);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_UNCONTROLLABLE_FRENZY);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLOOD_MIRROR_DAMAGE);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLOOD_MIRROR_VISUAL);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLOOD_MIRROR_DUMMY);
                instance->DoRemoveAurasDueToSpellOnPlayers(DELIRIOUS_SLASH);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_PACT_OF_THE_DARKFALLEN);
                instance->DoRemoveAurasDueToSpellOnPlayers(PRESENCE_OF_THE_DARKFALLEN);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_ESSENCE_OF_THE_BLOODQUEEN);
            }

            void SetTrashReaction(uint32 faction)
            {
                std::list<Creature*> TrashList;
                me->GetCreatureListWithEntryInGrid(TrashList, NPC_DARKFALLEN_ARCHMAGE,     500.0f);
                me->GetCreatureListWithEntryInGrid(TrashList, NPC_DARKFALLEN_BLOOD_KNIGHT, 500.0f);
                me->GetCreatureListWithEntryInGrid(TrashList, NPC_DARKFALLEN_NOBLE,        500.0f);
                me->GetCreatureListWithEntryInGrid(TrashList, NPC_DARKFALLEN_ADVISOR,      500.0f);
                me->GetCreatureListWithEntryInGrid(TrashList, NPC_DARKFALLEN_LIEUTENANT,   500.0f);
                me->GetCreatureListWithEntryInGrid(TrashList, NPC_DARKFALLEN_TACTICIAN,    500.0f);
                me->GetCreatureListWithEntryInGrid(TrashList, NPC_DARKFALLEN_COMMANDER,    500.0f);
                if (!TrashList.empty())
                    for (std::list<Creature*>::iterator itr = TrashList.begin(); itr != TrashList.end(); itr++)
                        (*itr)->setFaction(faction);
            }

            void DoAction(int32 action)
            {
                if (action != ACTION_KILL_MINCHAR)
                    return;

                if (instance->GetBossState(DATA_BLOOD_QUEEN_LANA_THEL) == IN_PROGRESS)
                    _killMinchar = true;
                else
                {
                    me->AllowInhabit(INHABIT_AIR);
                    me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                    me->GetMotionMaster()->MovePoint(POINT_MINCHAR, mincharPos);
                }
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                _EnterEvadeMode();

                ClearAuras();
                if (_killMinchar)
                {
                    _killMinchar = false;
                    me->AllowInhabit(INHABIT_AIR);
                    me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                    me->SetCanFly(true);
                    me->GetMotionMaster()->MovePoint(POINT_MINCHAR, mincharPos);
                }
                else
                {
                    me->GetMotionMaster()->MoveTargetedHome();
                    Reset();
                }
            }

            void JustReachedHome()
            {
                me->ForbidInhabit(INHABIT_AIR);
                me->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                me->SetReactState(REACT_AGGRESSIVE);
                BossAI::JustReachedHome();
                Talk(SAY_WIPE);
                instance->SetBossState(DATA_BLOOD_QUEEN_LANA_THEL, FAIL);
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_KILL);
            }

            void SetGuidData(ObjectGuid guid, uint32 type) override
            {
                switch (type)
                {
                    case GUID_VAMPIRE:
                        _vampires.insert(guid);
                        break;
                    case GUID_BLOODBOLT:
                        _bloodboltedPlayers.insert(guid);
                        break;
                    default:
                        break;
                }
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (type != POINT_MOTION_TYPE && type != EFFECT_MOTION_TYPE)
                    return;

                switch (id)
                {
                    case POINT_CENTER:
                        DoCastSelf(SPELL_INCITE_TERROR);
                        events.ScheduleEvent(EVENT_AIR_PHASE,                 100 * IN_MILLISECONDS + uint32(Is25ManRaid() ? 0 : 20 * IN_MILLISECONDS));
                        events.RescheduleEvent(EVENT_SWARMING_SHADOWS,       30.5 * IN_MILLISECONDS, EVENT_GROUP_NORMAL);
                        events.RescheduleEvent(EVENT_PACT_OF_THE_DARKFALLEN, 25.5 * IN_MILLISECONDS, EVENT_GROUP_NORMAL);
                        events.ScheduleEvent(EVENT_AIR_START_FLYING,            3 * IN_MILLISECONDS);
                        break;
                    case POINT_AIR:
                        _bloodboltedPlayers.clear();
                        DoCastSelf(SPELL_BLOODBOLT_WHIRL);
                        Talk(SAY_AIR_PHASE);
                        events.ScheduleEvent(EVENT_AIR_FLY_DOWN, 10 * IN_MILLISECONDS);
                        break;
                    case POINT_GROUND:
                        me->ForbidInhabit(INHABIT_AIR);
                        me->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                        me->SetReactState(REACT_AGGRESSIVE);
                        if (Unit* victim = me->SelectVictim())
                            AttackStart(victim);
                        events.ScheduleEvent(EVENT_BLOOD_MIRROR, 2.5 * IN_MILLISECONDS, EVENT_GROUP_CANCELLABLE);
                        break;
                    case POINT_MINCHAR:
                        DoCastSelf(SPELL_ANNIHILATE, true);
                        // already in evade mode
                        me->GetMotionMaster()->MoveTargetedHome();
                        Reset();
                    default:
                        break;
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
                        case EVENT_BERSERK:
                            Talk(EMOTE_BERSERK_RAID);
                            Talk(SAY_BERSERK);
                            DoCastSelf(SPELL_BERSERK);
                            break;
                        case EVENT_VAMPIRIC_BITE:
                        {
                            std::list<Player*> targets;
                            SelectRandomTarget(false, &targets);
                            if (!targets.empty())
                            {
                                Unit* target = targets.front();
                                DoCast(target, SPELL_VAMPIRIC_BITE);
                                DoCastAOE(SPELL_VAMPIRIC_BITE_DUMMY, true);
                                if (IsHeroic())
                                    if (Player* player = me->FindNearestPlayer(100.0f))
                                        player->CastSpell(player, SPELL_PRESENCE_OF_THE_DARKFALLEN_2, true);
                                Talk(SAY_VAMPIRIC_BITE);
                                _vampires.insert(target->GetGUID());
                            }
                            break;
                        }
                        case EVENT_BLOOD_MIRROR:
                        {
                            // victim can be NULL when this is processed in the same update tick as EVENT_AIR_PHASE
                            if (me->GetVictim())
                            {
                                Player* newOfftank = SelectRandomTarget(true);
                                if (_offtank != newOfftank)
                                {
                                    _offtank = newOfftank;
                                    if (_offtank)
                                    {
                                        // both spells have SPELL_ATTR5_SINGLE_TARGET_SPELL, no manual removal needed
                                        _offtank->CastSpell(me->GetVictim(), SPELL_BLOOD_MIRROR_DAMAGE, true);
                                        me->GetVictim()->CastSpell(_offtank, SPELL_BLOOD_MIRROR_DUMMY, true);
                                        DoCastVictim(SPELL_BLOOD_MIRROR_VISUAL);
                                        if (Is25ManRaid() && _offtank->GetQuestStatus(QUEST_BLOOD_INFUSION) == QUEST_STATUS_INCOMPLETE &&
                                            _offtank->HasAura(SPELL_UNSATED_CRAVING) && !_offtank->HasAura(SPELL_THIRST_QUENCHED) &&
                                            !_offtank->HasAura(SPELL_GUSHING_WOUND))
                                            _offtank->CastSpell(_offtank, SPELL_GUSHING_WOUND, TRIGGERED_FULL_MASK);

                                    }
                                }
                            }
                            events.ScheduleEvent(EVENT_BLOOD_MIRROR, 2.5 * IN_MILLISECONDS, EVENT_GROUP_CANCELLABLE);
                            break;
                        }
                        case EVENT_DELIRIOUS_SLASH:
                            if (_offtank && !me->HasByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER))
                                DoCast(_offtank, SPELL_DELIRIOUS_SLASH);
                            events.ScheduleEvent(EVENT_DELIRIOUS_SLASH, urand(20, 24) * IN_MILLISECONDS, EVENT_GROUP_NORMAL);
                            break;
                        case EVENT_PACT_OF_THE_DARKFALLEN:
                        {
                            std::list<Player*> targets;
                            SelectRandomTarget(false, &targets);
                            Trinity::Containers::RandomResize(targets, Is25ManRaid() ? 3 : 2);
                            if (targets.size() > 1)
                            {
                                Talk(SAY_PACT_OF_THE_DARKFALLEN);
                                for (std::list<Player*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
                                    DoCast(*itr, SPELL_PACT_OF_THE_DARKFALLEN);
                            }
                            events.ScheduleEvent(EVENT_PACT_OF_THE_DARKFALLEN, 30.5 * IN_MILLISECONDS, EVENT_GROUP_NORMAL);
                            break;
                        }
                        case EVENT_SWARMING_SHADOWS:
                            if (Player* target = SelectRandomTarget(false))
                            {
                                Talk(EMOTE_SWARMING_SHADOWS, target);
                                Talk(SAY_SWARMING_SHADOWS);
                                DoCast(target, SPELL_SWARMING_SHADOWS);
                            }
                            events.ScheduleEvent(EVENT_SWARMING_SHADOWS, 30.5 * IN_MILLISECONDS, EVENT_GROUP_NORMAL);
                            break;
                        case EVENT_TWILIGHT_BLOODBOLT:
                        {
                            DoCastSelf(SPELL_TWILIGHT_BLOODBOLT_TARGET);
                            CastTwilightBloodbolt();
                            events.ScheduleEvent(EVENT_TWILIGHT_BLOODBOLT, urand(10, 15) * IN_MILLISECONDS, EVENT_GROUP_NORMAL);
                            break;
                        }
                        case EVENT_AIR_PHASE:
                            Airphase = true;
                            DoStopAttack();
                            me->SetReactState(REACT_PASSIVE);
                            events.DelayEvents(10000, EVENT_GROUP_NORMAL);
                            events.CancelEventGroup(EVENT_GROUP_CANCELLABLE);
                            me->GetMotionMaster()->MovePoint(POINT_CENTER, centerPos);
                            break;
                        case EVENT_AIR_START_FLYING:
                            me->AllowInhabit(INHABIT_AIR);
                            me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                            me->GetMotionMaster()->MovePoint(POINT_AIR, airPos);
                            break;
                        case EVENT_AIR_FLY_DOWN:
                            Airphase = false;
                            me->GetMotionMaster()->MoveLand(POINT_GROUND, centerPos);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            bool WasVampire(ObjectGuid guid)
            {
                return _vampires.count(guid) != 0;
            }

            bool WasBloodbolted(ObjectGuid guid)
            {
                return _bloodboltedPlayers.count(guid) != 0;
            }

            void CastTwilightBloodbolt()
            {
                // cast Twilight Bloodbolt on two differenz targets and prefer non-melee player as targets
                std::list<Player*> plrList;
                me->GetPlayerListInGrid(plrList, 1000.f);
                std::list<Player*> rangedList;
                std::list<Player*>::iterator playeritr;

                for (std::list<Player*>::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                {
                    if (Player* player = (*itr)->ToPlayer())
                    {
                        float distance = player->GetExactDist(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
                        if (distance >= 15.0f)
                            rangedList.push_back(player);
                    }
                }

                if (rangedList.size() >= RAID_MODE<std::size_t>(4, 9, 4, 9))
                    plrList = rangedList;

                for (uint8 bolt = 1; bolt <= 2; bolt++)
                {
                    if (!plrList.empty())
                    {
                        playeritr = plrList.begin();
                        std::advance(playeritr, urand(0, plrList.size() - 1));
                        DoCast(*playeritr, SPELL_TWILIGHT_BLOODBOLT);
                        plrList.remove(*playeritr);
                    }
                }
            }

        private:
            // offtank for this encounter is the player standing closest to main tank
            Player* SelectRandomTarget(bool includeOfftank, std::list<Player*>* targetList = NULL)
            {
                std::list<HostileReference*> const& threatlist = me->GetThreatManager().getThreatList();
                std::list<Player*> tempTargets;

                if (threatlist.empty())
                    return NULL;

                for (std::list<HostileReference*>::const_iterator itr = threatlist.begin(); itr != threatlist.end(); ++itr)
                    if (Unit* refTarget = (*itr)->getTarget())
                        if (refTarget != me->GetVictim() && refTarget->GetTypeId() == TYPEID_PLAYER && (includeOfftank || (refTarget != _offtank)))
                            tempTargets.push_back(refTarget->ToPlayer());

                if (tempTargets.empty())
                    return NULL;

                if (targetList)
                {
                    *targetList = tempTargets;
                    return NULL;
                }

                if (includeOfftank)
                {
                    tempTargets.sort(Trinity::ObjectDistanceOrderPred(me->GetVictim()));
                    return tempTargets.front();
                }

                return Trinity::Containers::SelectRandomContainerElement(tempTargets);
            }            
            GuidSet _vampires;
            GuidSet _bloodboltedPlayers;
            Player* _offtank;
            bool _creditBloodQuickening;
            bool _killMinchar;
            bool Airphase;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<boss_blood_queen_lana_thelAI>(creature);
        }
};

// helper for shortened code
typedef boss_blood_queen_lana_thel::boss_blood_queen_lana_thelAI LanaThelAI;

class spell_blood_queen_vampiric_bite : public SpellScriptLoader
{
    public:
        spell_blood_queen_vampiric_bite() : SpellScriptLoader("spell_blood_queen_vampiric_bite") { }

        class spell_blood_queen_vampiric_bite_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_blood_queen_vampiric_bite_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_ESSENCE_OF_THE_BLOOD_QUEEN_PLR))
                    return false;
                if (!sSpellMgr->GetSpellInfo(SPELL_FRENZIED_BLOODTHIRST))
                    return false;
                if (!sSpellMgr->GetSpellInfo(SPELL_PRESENCE_OF_THE_DARKFALLEN))
                    return false;
                return true;
            }

            SpellCastResult CheckTarget()
            {
                if (IsVampire(GetExplTargetUnit()))
                {
                    SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_CANT_TARGET_VAMPIRES);
                    return SPELL_FAILED_CUSTOM_ERROR;
                }

                if (!GetCaster()->FindNearestCreature(NPC_BLOOD_QUEEN_LANA_THEL, 100.0f))
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

                return SPELL_CAST_OK;
            }

            void OnCast()
            {
                if (!GetCaster()->FindNearestCreature(NPC_BLOOD_QUEEN_LANA_THEL, 100.0f))
                    return;

                if (GetCaster()->GetTypeId() != TYPEID_PLAYER)
                    return;

                uint32 spellId = sSpellMgr->GetSpellIdForDifficulty(SPELL_FRENZIED_BLOODTHIRST, GetCaster());
                GetCaster()->RemoveAura(spellId, ObjectGuid::Empty, 0, AURA_REMOVE_BY_ENEMY_SPELL);
                GetCaster()->CastSpell(GetCaster(), SPELL_ESSENCE_OF_THE_BLOOD_QUEEN_PLR, TRIGGERED_FULL_MASK);

                if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                    if (Creature* bloodQueen = ObjectAccessor::GetCreature(*GetCaster(), ObjectGuid(instance->GetGuidData(DATA_BLOOD_QUEEN_LANA_THEL))))
                        bloodQueen->AI()->SetGuidData(GetHitUnit()->GetGUID(), GUID_VAMPIRE);

                // Shadowmourne questline
                if (Aura* aura = GetCaster()->GetAura(SPELL_GUSHING_WOUND))
                {
                    if (aura->GetStackAmount() == 3)
                    {
                        GetCaster()->CastSpell(GetCaster(), SPELL_THIRST_QUENCHED, TRIGGERED_FULL_MASK);
                        GetCaster()->RemoveAura(aura);
                    }
                    else
                        GetCaster()->CastSpell(GetCaster(), SPELL_GUSHING_WOUND, TRIGGERED_FULL_MASK);
                }

                if (GetCaster()->GetMap()->IsHeroic())
                    GetCaster()->CastSpell(GetCaster(), SPELL_PRESENCE_OF_THE_DARKFALLEN, true);
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_blood_queen_vampiric_bite_SpellScript::CheckTarget);
                BeforeHit += SpellHitFn(spell_blood_queen_vampiric_bite_SpellScript::OnCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_blood_queen_vampiric_bite_SpellScript();
        }
};

class spell_blood_queen_frenzied_bloodthirst : public SpellScriptLoader
{
    public:
        spell_blood_queen_frenzied_bloodthirst() : SpellScriptLoader("spell_blood_queen_frenzied_bloodthirst") { }

        class spell_blood_queen_frenzied_bloodthirst_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_blood_queen_frenzied_bloodthirst_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (InstanceScript* instance = GetTarget()->GetInstanceScript())
                    if (Creature* bloodQueen = ObjectAccessor::GetCreature(*GetTarget(), ObjectGuid(instance->GetGuidData(DATA_BLOOD_QUEEN_LANA_THEL))))
                        bloodQueen->AI()->Talk(EMOTE_BLOODTHIRST, GetTarget());
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
                    if (InstanceScript* instance = target->GetInstanceScript())
                        if (Creature* bloodQueen = ObjectAccessor::GetCreature(*target, ObjectGuid(instance->GetGuidData(DATA_BLOOD_QUEEN_LANA_THEL))))
                        {
                            // this needs to be done BEFORE charm aura or we hit an assert in Unit::SetCharmedBy
                            if (target->GetVehicleKit())
                                target->RemoveVehicleKit();

                            bloodQueen->AI()->Talk(SAY_MIND_CONTROL);
                            bloodQueen->CastSpell(target, SPELL_UNCONTROLLABLE_FRENZY, true);
                        }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_blood_queen_frenzied_bloodthirst_AuraScript::OnApply, EFFECT_0, SPELL_AURA_OVERRIDE_SPELLS, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_blood_queen_frenzied_bloodthirst_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_OVERRIDE_SPELLS, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_blood_queen_frenzied_bloodthirst_AuraScript();
        }
};

class BloodboltHitCheck
{
    public:
        explicit BloodboltHitCheck(LanaThelAI* ai) : _ai(ai) {}

        bool operator()(WorldObject* object) const
        {
            return _ai->WasBloodbolted(object->GetGUID());
        }

    private:
        LanaThelAI* _ai;
};

class spell_blood_queen_bloodbolt : public SpellScriptLoader
{
    public:
        spell_blood_queen_bloodbolt() : SpellScriptLoader("spell_blood_queen_bloodbolt") { }

        class spell_blood_queen_bloodbolt_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_blood_queen_bloodbolt_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_TWILIGHT_BLOODBOLT))
                    return false;
                return true;
            }

            bool Load()
            {
                return GetCaster()->GetEntry() == NPC_BLOOD_QUEEN_LANA_THEL;
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                uint32 targetCount = (targets.size() + 2) / 3;
                targets.remove_if(BloodboltHitCheck(static_cast<LanaThelAI*>(GetCaster()->GetAI())));
                Trinity::Containers::RandomResize(targets, targetCount);
                // mark targets now, effect hook has missile travel time delay (might cast next in that time)
                for (std::list<WorldObject*>::const_iterator itr = targets.begin(); itr != targets.end(); ++itr)
                    GetCaster()->GetAI()->SetGuidData((*itr)->GetGUID(), GUID_BLOODBOLT);
            }

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                GetCaster()->CastSpell(GetHitUnit(), SPELL_TWILIGHT_BLOODBOLT, true);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_blood_queen_bloodbolt_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
                OnEffectHitTarget += SpellEffectFn(spell_blood_queen_bloodbolt_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_blood_queen_bloodbolt_SpellScript();
        }
};

// 70871 - Essence of the Blood Queen
class spell_blood_queen_essence_of_the_blood_queen : public SpellScriptLoader
{
    public:
        spell_blood_queen_essence_of_the_blood_queen() : SpellScriptLoader("spell_blood_queen_essence_of_the_blood_queen") { }

        class spell_blood_queen_essence_of_the_blood_queen_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_blood_queen_essence_of_the_blood_queen_AuraScript);

            bool Validate(SpellInfo const* /*spellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_ESSENCE_OF_THE_BLOOD_QUEEN_HEAL))
                    return false;
                return true;
            }

            void OnProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
                int32 heal = CalculatePct(int32(eventInfo.GetDamageInfo()->GetDamage()), aurEff->GetAmount());
                GetTarget()->CastCustomSpell(SPELL_ESSENCE_OF_THE_BLOOD_QUEEN_HEAL, SPELLVALUE_BASE_POINT0, heal, GetTarget(), TRIGGERED_FULL_MASK, NULL, aurEff);
            }

            void Register()
            {
                OnEffectProc += AuraEffectProcFn(spell_blood_queen_essence_of_the_blood_queen_AuraScript::OnProc, EFFECT_1, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_blood_queen_essence_of_the_blood_queen_AuraScript();
        }
};

class spell_blood_queen_pact_of_the_darkfallen : public SpellScriptLoader
{
    public:
        spell_blood_queen_pact_of_the_darkfallen() : SpellScriptLoader("spell_blood_queen_pact_of_the_darkfallen") { }

        class spell_blood_queen_pact_of_the_darkfallen_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_blood_queen_pact_of_the_darkfallen_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if(Trinity::UnitAuraCheck(false, SPELL_PACT_OF_THE_DARKFALLEN));

                bool remove = true;
                std::list<WorldObject*>::const_iterator itrEnd = targets.end(), itr, itr2;
                // we can do this, unitList is MAX 4 in size
                for (itr = targets.begin(); itr != itrEnd && remove; ++itr)
                {
                    if (!GetCaster()->IsWithinDist(*itr, 5.0f, false))
                        remove = false;

                    for (itr2 = targets.begin(); itr2 != itrEnd && remove; ++itr2)
                        if (itr != itr2 && !(*itr2)->IsWithinDist(*itr, 5.0f, false))
                            remove = false;
                }

                if (remove)
                {
                    if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                    {
                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_PACT_OF_THE_DARKFALLEN);
                        targets.clear();
                    }
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_blood_queen_pact_of_the_darkfallen_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ALLY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_blood_queen_pact_of_the_darkfallen_SpellScript();
        }
};

class spell_blood_queen_pact_of_the_darkfallen_dmg : public SpellScriptLoader
{
    public:
        spell_blood_queen_pact_of_the_darkfallen_dmg() : SpellScriptLoader("spell_blood_queen_pact_of_the_darkfallen_dmg") { }

        class spell_blood_queen_pact_of_the_darkfallen_dmg_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_blood_queen_pact_of_the_darkfallen_dmg_AuraScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_PACT_OF_THE_DARKFALLEN_DAMAGE))
                    return false;
                return true;
            }

            // this is an additional effect to be executed
            void PeriodicTick(AuraEffect const* aurEff)
            {
                SpellInfo const* damageSpell = sSpellMgr->GetSpellInfo(SPELL_PACT_OF_THE_DARKFALLEN_DAMAGE);
                int32 damage = damageSpell->Effects[EFFECT_0].CalcValue();
                float multiplier = 0.3375f + 0.1f * uint32(aurEff->GetTickNumber()/10); // do not convert to 0.01f - we need tick number/10 as INT (damage increases every 10 ticks)
                damage = int32(damage * multiplier);
                GetTarget()->CastCustomSpell(SPELL_PACT_OF_THE_DARKFALLEN_DAMAGE, SPELLVALUE_BASE_POINT0, damage, GetTarget(), true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_blood_queen_pact_of_the_darkfallen_dmg_AuraScript::PeriodicTick, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_blood_queen_pact_of_the_darkfallen_dmg_AuraScript();
        }
};

class spell_blood_queen_pact_of_the_darkfallen_dmg_target : public SpellScriptLoader
{
    public:
        spell_blood_queen_pact_of_the_darkfallen_dmg_target() : SpellScriptLoader("spell_blood_queen_pact_of_the_darkfallen_dmg_target") { }

        class spell_blood_queen_pact_of_the_darkfallen_dmg_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_blood_queen_pact_of_the_darkfallen_dmg_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if(Trinity::UnitAuraCheck(true, SPELL_PACT_OF_THE_DARKFALLEN));
                unitList.push_back(GetCaster());
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_blood_queen_pact_of_the_darkfallen_dmg_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ALLY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_blood_queen_pact_of_the_darkfallen_dmg_SpellScript();
        }
};


class achievement_once_bitten_twice_shy : public AchievementCriteriaScript
{
    public:
        achievement_once_bitten_twice_shy(const char* name, uint8 spawnMode, bool wasVampire) : AchievementCriteriaScript(name), _spawnMode(spawnMode), _wasVampire(wasVampire) { }
    
        bool OnCheck(Player* source, Unit* target)
        {
            if (!target || !target->FindMap())
                return false;
    
            if (LanaThelAI* lanaThelAI = CAST_AI(LanaThelAI, target->GetAI()))
                return (target->GetMap()->GetSpawnMode() % 2) == _spawnMode && lanaThelAI->WasVampire(source->GetGUID()) == _wasVampire;
            return false;
        }
    
        uint8 _spawnMode;
        bool _wasVampire;
};

void AddSC_boss_blood_queen_lana_thel()
{
    new boss_blood_queen_lana_thel();
    new spell_blood_queen_vampiric_bite();
    new spell_blood_queen_frenzied_bloodthirst();
    new spell_blood_queen_bloodbolt();
    new spell_blood_queen_essence_of_the_blood_queen();
    new spell_blood_queen_pact_of_the_darkfallen();
    new spell_blood_queen_pact_of_the_darkfallen_dmg();
    new spell_blood_queen_pact_of_the_darkfallen_dmg_target();
    new achievement_once_bitten_twice_shy("achievement_once_bitten_twice_shy_n_10", 0, false);
    new achievement_once_bitten_twice_shy("achievement_once_bitten_twice_shy_v_10", 0, true);
    new achievement_once_bitten_twice_shy("achievement_once_bitten_twice_shy_n_25", 1, false);
    new achievement_once_bitten_twice_shy("achievement_once_bitten_twice_shy_v_25", 1, true);
}
