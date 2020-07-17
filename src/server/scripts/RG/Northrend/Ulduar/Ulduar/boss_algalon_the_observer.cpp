
/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
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
#include "MapManager.h"
#include "Pet.h"
#include "ulduar.h"
#include "Opcodes.h"
#include "PassiveAI.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "DBCStores.h"
#include "Packets/WorldPacket.h"
#include "RG/Logging/LogManager.hpp"

enum Spells
{
    //Visuals
    SPELL_BLUE_BEAM                 = 63773,
    SPELL_ARRIVAL                   = 64997,
    SPELL_REORIGINATION             = 64996,
    SPELL_SUMMON_AZEROTH            = 64994,
    SPELL_HOVER_FALL                = 60425,
    SPELL_TELEPORT_VISUAL           = 62940,

    SPELL_BIG_BANG                  = 64443,
    SPELL_H_BIG_BANG                = 64584,
    SPELL_REMOVE_FROM_PHASE         = 64445,
    H_SPELL_BIG_BANG                = 64584,
    SPELL_COSMIC_SMASH              = 62301,
    H_SPELL_COSMIC_SMASH            = 64598,
    SPELL_COSMIC_SMASH_MISSLE       = 62304,
    SPELL_COSMIC_SMASH_DAMAGE       = 62311,
    SPELL_QUANTUM_STRIKE            = 64395,
    H_SPELL_QUANTUM_STRIKE          = 64592,
    //summons

    SPELL_ARCANE_BARAGE             = 64599,
    H_SPELL_ARCANE_BARAGE           = 64607,
    SPELL_COSMIC_SMASH_VISUAL       = 62300,
    SPELL_SUMMON_BLACK_HOLE         = 62189,
    SPELL_DESPAWN_BLACK_HOLE        = 64391,
    SPELL_BLACK_HOLE_EXPLOSION      = 64122,
    SPELL_BLACK_HOLE_AURA           = 62168,
    SPELL_BLACK_HOLE_DAMAGE         = 62169,
    SPELL_BLACK_HOLE_COSMETIC       = 64135,
    SPELL_BLACK_HOLE_TRIGGER        = 62185, //Buggy. Seems to reset the Black hole after it hits a player.
    SPELL_BLACK_HOLE_KILL_CREDIT    = 65312,
    SPELL_BLACK_HOLE_SPAWN_VISUAL   = 62003,
    SPELL_VISUAL_VOID_ZONE          = 64469,
    SPELL_UNLEASHED_DARK_MATTER     = 64450,

    SPELL_PHASE_PUNCH               = 64412,
    SPELL_PHASE_PUNCH_SHIFT         = 64417,
    SPELL_PHASE_PUNCH_ALPHA_1       = 64435, //Transparency effects for Phase Punch.
    SPELL_PHASE_PUNCH_ALPHA_2       = 64434,
    SPELL_PHASE_PUNCH_ALPHA_3       = 64428,
    SPELL_PHASE_PUNCH_ALPHA_4       = 64421,

    SPELL_ASCEND                    = 64487,
    SPELL_BERSERK                   = 64238,
    SPELL_I_DIED                    = 65082, //Must be related to an achievement.
    SPELL_KILL_CREDIT               = 65184
};

enum Creatures
{
    CREATURE_ALGALON                = 32871,
    CREATURE_COLLAPSING_STAR        = 32955,
    CREATURE_BLACK_HOLE             = 32953,
    CREATURE_LIVING_CONSTELLATION   = 33052,
    CREATURE_DARK_MATTER            = 33089,
    CREATURE_UNLEASHED_DARK_MATTER  = 34097,
    CREATURE_AZEROTH_MODEL          = 34246,
    CREATURE_ALGALON_STALKER        = 34004, //possibly used to keep algalon in combat? unused.
    CREATURE_ALGALON_ASTEROID_2     = 33105
};

enum Texts
{   //Algalon
    SAY_SUMMON_1                                = 0,
    SAY_SUMMON_2                                = 1,
    SAY_SUMMON_3                                = 2,
    SAY_AGGRO                                   = 3,
    SAY_ENGAGED_FOR_FIRST_TIME                  = 4,
    SAY_SUMMON_COLLAPSING_STAR                  = 5,
    EMOTE_COLLAPSING_STARS                      = 6,
    SAY_BIG_BANG                                = 7,
    EMOTE_BIG_BANG                              = 8,
    SAY_BERSERK                                 = 9,
    EMOTE_COSMIC_SMASH                          = 10,
    SAY_PHASE_2                                 = 11,
    SAY_DEATH_1                                 = 12,
    SAY_DEATH_2                                 = 13,
    SAY_DEATH_3                                 = 14,
    SAY_DEATH_4                                 = 15,
    SAY_DEATH_5                                 = 16,
    SAY_TIMER_1                                 = 17,
    SAY_TIMER_2                                 = 18,
    SAY_TIMER_3                                 = 19,
    SAY_SLAY                                    = 20,

    //Brann
    SAY_BRANN_INTRO                             = 0,
    SAY_BRANN_LEAVE                             = 1,
    SAY_BRANN_OUTRO                             = 2
};

enum Events
{
    EVENT_BIG_BANG = 1,
    EVENT_REMOVE_FROM_PHASE,
    EVENT_RESTART_ATTACKING,

    EVENT_COSMIC_SMASH,
    EVENT_COLLAPSING_STAR,
    EVENT_PHASE_PUNCH,
    EVENT_SUMMON_CONSTELLATION,
    EVENT_SUMMON_UNLEASHED_DARK_MATTER,
    EVENT_QUANTUM_STRIKE,

    EVENT_BERSERK_BUFF,
    EVENT_BERSERK_ASCEND,
};

enum Phase
{
    PHASE_NONE,
    PHASE_SPAWN_INTRO,
    PHASE_PULL_INTRO,
    PHASE_1,
    PHASE_2,
    PHASE_WIN_OUTRO,
    PHASE_SEND_REPLYCODE_OUTRO,
};

enum Actions
{
  //ACTION_ALGALON_SPAWN_INTRO      =  100001,  // in ulduar.h
    ACTION_BLACK_HOLE_CLOSED        =  100002,
  //ACTION_ALGALON_SEND_REPLYCODE   =  100003,  // in ulduar.h
    ACTION_BRANN_INTRO              = -123457,
    ACTION_BRANN_LEAVE              = -123458,
    ACTION_BLACKHOLE_SUMMON         = -123459,
    ACTION_ACTIVATE_CONSTELLATION   = -123455,
};

enum Achievements
{
    ACHIEV_FEEDS_ON_TEARS_10           = 3004,
    ACHIEV_FEEDS_ON_TEARS_25           = 3005,
    ACHIEV_SUPERMASSIVE_10             = 3003,
    ACHIEV_SUPERMASSIVE_25             = 3002,
    ACHIEV_HEROLD_10                   = 3316,
};

enum Music
{
    MUSIC_INTRO         = 15878,
    MUSIC_BATTLE        = 15877
};

enum Constants
{
    MAXSTARS            = 4,
    SUPERMASSIVE_MAX    = 3,
    SEC                 = 1000,
};

enum Items
{
    CELESTIAL_PLANETARIUM_KEY      = 45796,
    CELESTIAL_PLANETARIUM_KEY_H    = 45798,
};

static const float GroundZ = 417.321198f;

static const float RoomCenter[3] = {1632.050049f, -307.36499f, GroundZ};

static const float OutroPos[3] = {1632.050049f, -319.36499f, GroundZ};

static const float BrannOutro[3] = {1632.050049f, -294.36499f, GroundZ};

//Brann
float WPs_ulduar[9][3] =
{
    //pos_x             pos_y       pos_z
    {1661.567261f, -155.395126f, 427.261810f},
    {1633.316650f, -176.056778f, 427.286011f},
    {1633.187744f, -190.987228f, 427.378632f},
    {1632.005371f, -214.134232f, 418.470459f},
    {1631.882324f, -228.378708f, 418.082733f},
    {1634.257446f, -230.407898f, 417.336182f},
    {1635.767700f, -266.664459f, 417.321991f},
    {1630.990845f, -266.863434f, 417.321991f},
    {1632.005371f, -214.134232f, 418.470459f}
};

class boss_algalon_the_observer : public CreatureScript
{
    public:
        boss_algalon_the_observer() : CreatureScript("boss_algalon_the_observer") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_algalon_the_observerAI(creature);
        }

        struct boss_algalon_the_observerAI : public BossAI
        {
            boss_algalon_the_observerAI(Creature *creature) : BossAI(creature, BOSS_ALGALON), summons(creature) {

                phase = PHASE_NONE;

                me->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); //deathgrip
                me->ApplySpellImmune(0, IMMUNITY_ID, 43263, true); // army of the dead - periodic taunt

                // Offhand damage: corehack, not implemented via DB
                const CreatureTemplate *cinfo = me->GetCreatureTemplate();
                float mindmg = cinfo->mindmg * cinfo->dmg_multiplier /2;
                float maxdmg = cinfo->maxdmg * cinfo->dmg_multiplier /2;
                me->SetBaseWeaponDamage(OFF_ATTACK, MINDAMAGE, mindmg);
                me->SetBaseWeaponDamage(OFF_ATTACK, MAXDAMAGE, maxdmg);
                me->SetFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE, mindmg);
                me->SetFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE, maxdmg);

            }


            uint32 phase;
            SummonList summons;

            // PHASE_SPAWN_INTRO
            uint32 spawn_intro_step;
            uint32 spawn_intro_timer;

            // PHASE_PULL_INTRO
            uint32 pull_intro_step;
            uint32 pull_intro_timer;

            // PHASE_1 & PHASE_2
            uint32 staramount;            // amount of collapsing stars which are alive
            bool meleehit;                // determine if algalon should try to meleehit

            bool supermassive;            // achiev: supermassive completed
            uint32 supermassive_count;    // how many holes closed?
            uint32 supermassive_timer;    // how much time remaining

            // PHASE_WIN_OUTRO
            uint32 win_outro_step;
            uint32 win_outro_timer;

            // PHASE_SEND_REPLYCODE_OUTRO
            uint32 replycode_outro_step;
            uint32 replycode_outro_timer;

            void EnterCombat(Unit* /*who*/)
            {
                me->SetSheath(SHEATH_STATE_MELEE);

                events.CancelEvent(EVENT_REMOVE_FROM_PHASE);
                events.CancelEvent(EVENT_SUMMON_UNLEASHED_DARK_MATTER);
                events.CancelEvent(EVENT_RESTART_ATTACKING);

                if (phase == PHASE_NONE) {

                    phase = PHASE_PULL_INTRO;
                    pull_intro_step = 0;
                    pull_intro_timer = 0;
                }
            }

            void KilledUnit(Unit* victim) {
                Talk(SAY_SLAY, victim);
            }

            void DoAction(int32 actionId)
            {
                switch(actionId)
                {
                    case ACTION_ALGALON_SPAWN_INTRO:

                        phase = PHASE_SPAWN_INTRO;
                        spawn_intro_step = 0;
                        spawn_intro_timer = 7*SEC;

                        break;
                    case ACTION_ALGALON_SEND_REPLYCODE:

                        phase = PHASE_SEND_REPLYCODE_OUTRO;
                        replycode_outro_step = 0;
                        replycode_outro_timer = 0;

                        break;
                    case ACTION_BLACK_HOLE_CLOSED:
                        if (supermassive) // already completed, nothing to do
                            return;

                        if (supermassive_timer) { // timer in progress

                            supermassive_count++;
                            if (supermassive_count >= SUPERMASSIVE_MAX) // achievement complete
                                    supermassive = true;

                        } else { // start timer
                            supermassive_count = 1;
                            supermassive_timer = 10*SEC;
                        }

                        break;
                }
            }

            void Reset()
            {
                BossAI::Reset();
                SetCombatMovement(false);
                phase = PHASE_NONE;
                summons.DespawnAll();

                me->setFaction(FACTION_NEUTRAL);
                me->AddUnitMovementFlag(MOVEMENTFLAG_HOVER);
                me->SendMovementFlagUpdate();
                me->SetSheath(SHEATH_STATE_UNARMED);
                me->SetReactState(REACT_AGGRESSIVE);

                events.CancelEvent(EVENT_REMOVE_FROM_PHASE);
                events.CancelEvent(EVENT_SUMMON_UNLEASHED_DARK_MATTER);
                events.CancelEvent(EVENT_RESTART_ATTACKING);
            }

            void DamageTaken(Unit * /*who*/, uint32 &Damage)
            {
                if(Damage > me->GetHealth())
                    Damage = me->GetHealth() - 1;
            }

            void JustSummoned(Creature* Summon)
            {
                if (!Summon)
                    return;

                summons.Summon(Summon);
                Summon->setFaction(me->getFaction());

                switch(Summon->GetEntry())
                {
                    case CREATURE_AZEROTH_MODEL:
                        Summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                        break;
                    case CREATURE_BLACK_HOLE:
                        --staramount;
                        Summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                        Summon->AddAura(SPELL_VISUAL_VOID_ZONE, Summon);
                        Summon->SetReactState(REACT_PASSIVE);
                    case CREATURE_COLLAPSING_STAR:
                        Summon->SetUInt64Value(UNIT_FIELD_SUMMONEDBY, me->GetGUID().GetRawValue());
                        break;
                    case CREATURE_UNLEASHED_DARK_MATTER:
                        Summon->AddUnitMovementFlag(MOVEMENTFLAG_HOVER);
                        Summon->SetInCombatWithZone();
                        if (Unit* Target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                            if (Summon->IsAIEnabled && Summon->AI())
                                Summon->AI()->AttackStart(Target);
                        break;
                }
            }

            void SummonCollapsingStars()
            {
                if (staramount == MAXSTARS)
                    return;

                Talk(SAY_SUMMON_COLLAPSING_STAR);
                Talk(EMOTE_COLLAPSING_STARS);
                Position pos;
                for (uint8 i = MAXSTARS - staramount; i>0; --i)
                {
                    pos.Relocate(RoomCenter[0], RoomCenter[1], RoomCenter[2]);
                    me->MovePosition(pos, 20, cos(static_cast<float>(2.0f*M_PI*(float)i/4.0f)));
                    if (me->SummonCreature(CREATURE_COLLAPSING_STAR, pos, TEMPSUMMON_CORPSE_DESPAWN))
                        ++staramount;

                }

            }

            void SummonLivingConstellations()
            {
                EntryCheckPredicate pred(CREATURE_LIVING_CONSTELLATION);
                summons.DoAction(ACTION_ACTIVATE_CONSTELLATION, pred);
            }

            void PopulateBlackHoles()
            {
                Position pos;
                for (uint8 i = 1; i<= 8; ++i)
                {
                    me->GetNearPosition(pos, urand(15,35), (float)rand_norm() * static_cast<float>(2 * M_PI));
                    DoSummon(CREATURE_DARK_MATTER,pos,1000);
                }
            }

            void SummonConstellationsInitial()
            {
                Position pos;
                float z;
                pos.m_orientation = 0;

                for (uint32 i = 0; i < 3; ++i)
                {
                    z = urand(20, 35);
                    pos.Relocate(RoomCenter[0], RoomCenter[1], RoomCenter[2]);
                    pos.m_positionX += 40 * cos((float)rand_norm() * static_cast<float>(2*M_PI));
                    pos.m_positionY += 40 * sin((float)rand_norm() * static_cast<float>(2*M_PI));
                    pos.m_positionZ += z;
                    me->SummonCreature(CREATURE_LIVING_CONSTELLATION, pos, TEMPSUMMON_CORPSE_DESPAWN);
                }

            }

            void SetEncounterMusic(uint32 MusicID) //wrong. I'm almost certain it has to do with zonemusic.dbc...
            {
                WorldPacket data(SMSG_PLAY_MUSIC, 4);
                data << uint32(MusicID);
                instance->instance->SendToPlayers(data);
            }

            void DoMeleeAttacksIfReady() // we need this function because DoMeleeAttackIfReady causes the mob to hit with both weapons at the same time (not blizzlike)
            {
                if (me->IsWithinMeleeRange(me->GetVictim()) && !me->IsNonMeleeSpellCast(false))
                {
                    if (me->isAttackReady() && me->GetVictim())
                    {
                        me->AttackerStateUpdate(me->GetVictim());
                        me->resetAttackTimer();
                        me->setAttackTimer(OFF_ATTACK, me->getAttackTimer(BASE_ATTACK)/2);
                    }

                    if (me->isAttackReady(OFF_ATTACK) && me->GetVictim())
                    {
                        me->AttackerStateUpdate(me->GetVictim(), OFF_ATTACK);
                        me->resetAttackTimer(OFF_ATTACK);
                    }
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (phase == PHASE_NONE)
                    return;

                // phase = intro or outro

                switch (phase) {
                    case PHASE_SPAWN_INTRO: {
                        if (spawn_intro_timer <= diff) {
                            DoSpawnIntro();
                        } else spawn_intro_timer -= diff;
                        return;
                    }

                    case PHASE_PULL_INTRO: {
                        if (pull_intro_timer <= diff) {
                            DoPullIntro();
                        } else pull_intro_timer -= diff;
                        return;
                    }

                    case PHASE_WIN_OUTRO: {
                        if (win_outro_timer <= diff) {
                            DoWinOutro();
                        } else win_outro_timer -= diff;
                        return;
                    }

                    case PHASE_SEND_REPLYCODE_OUTRO: {
                        if (replycode_outro_timer <= diff) {
                            DoReplycodeOutro();
                        } else replycode_outro_timer -= diff;
                        return;

                    }
                }

                // phase = combat (PHASE_1 or PHASE 2)

                // phase 1 -> phase 2 on 20% health
                if (phase == PHASE_1 && HealthBelowPct(20))
                {
                    phase = PHASE_2;

                    Talk(SAY_PHASE_2);

                    summons.DespawnEntry(CREATURE_BLACK_HOLE);
                    summons.DespawnEntry(CREATURE_LIVING_CONSTELLATION);
                    summons.DespawnEntry(CREATURE_COLLAPSING_STAR);

                    events.CancelEvent(EVENT_COLLAPSING_STAR);
                    events.CancelEvent(EVENT_SUMMON_CONSTELLATION);

                    Position pos;

                    for (uint8 i = 1; i <= 4; ++i) //Summoning the black Holes.
                    {
                        pos.Relocate(RoomCenter[0], RoomCenter[1], RoomCenter[2]);
                        pos.m_orientation = Position::NormalizeOrientation(2*M_PI*i/4 + M_PI/3);
                        pos.m_positionX += 20 * cos(pos.GetOrientation());
                        pos.m_positionY += 20 * sin(pos.GetOrientation());
                        DoSummon(CREATURE_BLACK_HOLE, pos, 0);
                    }
                    events.ScheduleEvent(EVENT_SUMMON_UNLEASHED_DARK_MATTER, 1500);
                }

                // phase 2 -> player win on 3% health
                if (phase == PHASE_2 && HealthBelowPct(3))
                {
                    phase = PHASE_WIN_OUTRO;
                    win_outro_step = 0;
                    win_outro_timer = 0;
                    me->DespawnOrUnsummon(60 * MINUTE * SEC);
                    me->SummonGameObject(RAID_MODE(GO_GIFT_OF_THE_OBSERVER_10, GO_GIFT_OF_THE_OBSERVER_25), RoomCenter[0], RoomCenter[1], RoomCenter[2], M_PI_F / 2.0f ,0,0,0,0, 3600*SEC);
                    instance->SetBossState(BOSS_ALGALON, DONE);
                    RG_LOG<EncounterLogModule>(me);
                }

                // replaces UpdateVictim() to avoid reset bugs
                if (meleehit)
                {
                    if (!me->HasReactState(REACT_PASSIVE))
                    {
                        if (Unit* victim = me->SelectVictim())
                            AttackStart(victim);
                    }
                    else if (me->GetThreatManager().isThreatListEmpty()) // this replaces updatevictim(), since he casts ascend to the heavens to wipe the raid instead of just respawning.
                    {
                        me->InterruptNonMeleeSpells(false);
                        EnterEvadeMode();
                    }
                }

                // check for supermassive timer
                if (supermassive_timer <= diff) { // try failed
                    supermassive_count = 0;
                    supermassive_timer = 0;
                } else supermassive_timer -= diff;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventID = events.ExecuteEvent()) {
                    switch (eventID) {
                        case EVENT_BIG_BANG:
                            Talk(SAY_BIG_BANG);
                            Talk(EMOTE_BIG_BANG);
                            DoCastAOE(RAID_MODE(SPELL_BIG_BANG,H_SPELL_BIG_BANG));
                            me->AttackStop();
                            me->SetReactState(REACT_PASSIVE);
                            meleehit = false;

                            events.RescheduleEvent(EVENT_QUANTUM_STRIKE, 16*SEC);
                            events.RescheduleEvent(EVENT_REMOVE_FROM_PHASE, 9*SEC);
                            events.RescheduleEvent(EVENT_BIG_BANG, 90*SEC);
                            break;

                        case EVENT_REMOVE_FROM_PHASE:
                        {
                            for (Player& player : me->GetMap()->GetPlayers())
                            {
                                player.RemoveAura(SPELL_BLACK_HOLE_AURA);
                                if (Pet* pet = player.GetPet())
                                    pet->RemoveAura(SPELL_BLACK_HOLE_AURA);
                            }

                            events.RescheduleEvent(EVENT_RESTART_ATTACKING, 2*SEC);
                            break;
                        }
                        case EVENT_RESTART_ATTACKING:
                            me->SetReactState(REACT_AGGRESSIVE);
                            meleehit = true;
                            break;

                        case EVENT_PHASE_PUNCH:
                            DoCastVictim(SPELL_PHASE_PUNCH);
                            events.RescheduleEvent(EVENT_PHASE_PUNCH, 16*SEC);
                            break;

                        case EVENT_COSMIC_SMASH:
                            DoCast(RAID_MODE(SPELL_COSMIC_SMASH,H_SPELL_COSMIC_SMASH));
                            Talk(EMOTE_COSMIC_SMASH);
                            events.RescheduleEvent(EVENT_COSMIC_SMASH, urand(25*SEC, 30*SEC));
                            break;

                        case EVENT_QUANTUM_STRIKE:
                            DoCastVictim(RAID_MODE(SPELL_QUANTUM_STRIKE,H_SPELL_QUANTUM_STRIKE));
                            events.RescheduleEvent(EVENT_QUANTUM_STRIKE, urand(4*SEC, 8*SEC));
                            break;

                        case EVENT_COLLAPSING_STAR:
                            SummonCollapsingStars();
                            events.RescheduleEvent(EVENT_COLLAPSING_STAR, 50*SEC);
                            break;

                        case EVENT_SUMMON_CONSTELLATION:
                            SummonLivingConstellations();
                            SummonConstellationsInitial();
                            events.RescheduleEvent(EVENT_SUMMON_CONSTELLATION, 50*SEC);
                            break;

                        case EVENT_SUMMON_UNLEASHED_DARK_MATTER:
                        {
                            EntryCheckPredicate pred(CREATURE_BLACK_HOLE);
                            summons.DoAction(ACTION_BLACKHOLE_SUMMON, pred);
                            events.RescheduleEvent(EVENT_SUMMON_UNLEASHED_DARK_MATTER, 30*SEC);
                            break;
                        }

                        case EVENT_BERSERK_BUFF: // grant enrage buff, one last big bang, then wipe the raid
                            DoCastSelf(SPELL_BERSERK, true);
                            Talk(SAY_BERSERK);
                            events.RescheduleEvent(EVENT_BERSERK_ASCEND, 10*SEC);
                            break;

                        case EVENT_BERSERK_ASCEND:
                            me->GetMotionMaster()->Clear(false);
                            me->GetMotionMaster()->MoveIdle();
                            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLACK_HOLE_AURA); //hack. Ascend to the heavens should hit players who are in black holes.
                            DoCastAOE(SPELL_ASCEND, true);
                            EnterEvadeMode();
                            break;

                    } // switch eventid
                } // while execute event

                if (meleehit && me->GetReactState() != REACT_PASSIVE)
                    DoMeleeAttacksIfReady();

            }

            // Intro on first spawn
            void DoSpawnIntro() {
                switch (spawn_intro_step) {
                      case 0:

                            float x,y;
                            me->SetOrientation(me->GetHomePosition().GetOrientation());
                            me->SendMovementFlagUpdate();
                            me->AddAura(SPELL_HOVER_FALL, me);
                            me->SetFlag(UNIT_FIELD_FLAGS,  UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_PACIFIED);
                            me->GetPosition(x,y);
                            me->GetMotionMaster()->MoveCharge(x,y,GroundZ,me->GetSpeed(MOVE_FLIGHT), 0);
                            DoCast(me,SPELL_ARRIVAL,true);
                            DoCast(me,SPELL_BLUE_BEAM,true);
                            me->SetReactState(REACT_PASSIVE);
                            me->SetVisible(true);

                            spawn_intro_step++;
                            spawn_intro_timer = 3.5*SEC;

                            break;
                        case 1:
                            me->RemoveAurasDueToSpell(SPELL_BLUE_BEAM);
                            Talk(SAY_SUMMON_1);

                            spawn_intro_step++;
                            spawn_intro_timer = 7*SEC;

                            break;
                        case 2:
                            me->m_positionZ = GroundZ;
                            me->SetOrientation(me->GetHomePosition().GetOrientation());
                            DoCast(SPELL_SUMMON_AZEROTH);

                            spawn_intro_step++;
                            spawn_intro_timer = 5*SEC;

                            break;
                        case 3:
                            Talk(SAY_SUMMON_2);
                            me->RemoveUnitMovementFlag(MOVEMENTFLAG_HOVER);
                            me->SendMovementFlagUpdate();
                            DoCastAOE(SPELL_REORIGINATION);

                            spawn_intro_step++;
                            spawn_intro_timer = 8*SEC;

                            break;
                        case 4:
                            Talk(SAY_SUMMON_3);

                            spawn_intro_step++;
                            spawn_intro_timer = 12*SEC;

                            break;
                        case 5:
                            if (Creature* Brann = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_BRANN_BRONZEBEARD_ALG))))
                                Brann->AI()->DoAction(ACTION_BRANN_LEAVE);
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_PACIFIED);
                            me->SetReactState(REACT_AGGRESSIVE);

                            // spawn intro done
                            phase = PHASE_NONE;
                            break;
                }
            }


            // Intro at the beginning of the fight
            void DoPullIntro() {
                switch(pull_intro_step) {
                        case 0:

                            Talk(SAY_AGGRO);

                            me->SetReactState(REACT_PASSIVE);
                            me->SetFlag(UNIT_FIELD_FLAGS,  UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_PACIFIED);
                            me->RemoveAllAuras();
                            me->GetThreatManager().ClearAllThreat();
                            me->CombatStop(true);
                            me->StopMoving();
                            me->SetOrientation(me->GetHomePosition().GetOrientation());
                            me->SendMovementFlagUpdate();
                            me->GetMotionMaster()->Clear();
                            me->GetMotionMaster()->MoveIdle();
                            summons.DespawnAll();

                            pull_intro_step++;
                            pull_intro_timer = 3*SEC;

                            break;
                        case 1:
                            Talk(SAY_ENGAGED_FOR_FIRST_TIME);
                            instance->SetBossState(BOSS_ALGALON, IN_PROGRESS);

                            pull_intro_step++;
                            pull_intro_timer = 6*SEC;

                            break;
                        case 2:
                            phase = PHASE_1;
                            SetEncounterMusic(MUSIC_BATTLE);

                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_PACIFIED);
                            me->setFaction(FACTION_HOSTILE);
                            me->SetReactState(REACT_AGGRESSIVE);
                            meleehit = true;

                            events.Reset();
                            events.RescheduleEvent(EVENT_BERSERK_BUFF, 6 * MINUTE * IN_MILLISECONDS);
                            events.RescheduleEvent(EVENT_BIG_BANG, 90000);
                            events.RescheduleEvent(EVENT_COSMIC_SMASH, 25000);
                            events.RescheduleEvent(EVENT_PHASE_PUNCH, 16000);
                            events.RescheduleEvent(EVENT_QUANTUM_STRIKE, urand(4000,8000));
                            events.RescheduleEvent(EVENT_COLLAPSING_STAR, 15000, PHASE_1);
                            events.RescheduleEvent(EVENT_SUMMON_CONSTELLATION, 50000, PHASE_1);

                            staramount = 0;
                            supermassive = false;
                            supermassive_count = 0;
                            supermassive_timer = 0;

                            PopulateBlackHoles();
                            SummonConstellationsInitial();
                            SetCombatMovement(true);

                            me->SetInCombatWithZone();

                            if (Unit* Target = SelectTarget(SELECT_TARGET_RANDOM, 0, 300.0f, true))
                                AttackStart(Target);

                            break;
                }
            }

            // Outro after defeated by players
            void DoWinOutro() {
                switch(win_outro_step) {

                        case 0:
                            events.Reset();
                            summons.DespawnAll();
                            me->GetThreatManager().ClearAllThreat();
                            me->CombatStop(true);
                            me->StopMoving();
                            me->SetOrientation(me->GetHomePosition().GetOrientation());
                            me->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
                            me->SendMovementFlagUpdate();
                            me->GetMotionMaster()->Clear();
                            me->GetMotionMaster()->MoveIdle();
                            me->setFaction(FACTION_FRIENDLY_TO_ALL);

                            win_outro_step++;
                            win_outro_timer = 9*SEC;

                        case 1:
                            me->SetOrientation(me->GetHomePosition().GetOrientation());
                            me->SetSpeedRate(MOVE_WALK, 2.0f);
                            me->GetMotionMaster()->MovePoint(1, OutroPos[0], OutroPos[1], OutroPos[2]);
                            me->SendMovementFlagUpdate();

                            win_outro_step++;
                            win_outro_timer = 14*SEC;

                            break;
                        case 2:

                            Talk(SAY_DEATH_1);
                            DoCastAOE(SPELL_KILL_CREDIT, true);

                            if (!(me->GetInstanceScript()->GetData(DATA_PLAYER_DEATH_MASK) & (1 << BOSS_ALGALON)))
                                me->GetInstanceScript()->DoCompleteAchievement(RAID_MODE(ACHIEV_FEEDS_ON_TEARS_10, ACHIEV_FEEDS_ON_TEARS_25));

                            if (supermassive)
                                me->GetInstanceScript()->DoCompleteAchievement(RAID_MODE(ACHIEV_SUPERMASSIVE_10, ACHIEV_SUPERMASSIVE_25));

                            me->SetStandState(UNIT_STAND_STATE_KNEEL);

                            win_outro_step++;
                            win_outro_timer = 40*SEC;

                            break;
                        case 3:
                            Talk(SAY_DEATH_2);

                            win_outro_step++;
                            win_outro_timer = 18*SEC;

                            break;
                        case 4:
                            Talk(SAY_DEATH_3);

                            win_outro_step++;
                            win_outro_timer = 11*SEC;

                            break;
                        case 5:
                            Talk(SAY_DEATH_4);

                            win_outro_step++;
                            win_outro_timer = 12*SEC;

                            break;
                        case 6:
                            if (Creature* Brann = me->SummonCreature(NPC_BRANN_BRONZBEARD_ALG, BrannOutro[0], BrannOutro[1], BrannOutro[2]))
                            {
                                Brann->AI()->Talk(SAY_BRANN_OUTRO);
                                Brann->SetFacingToObject(me);
                                Brann->DespawnOrUnsummon(30000);
                            }

                            win_outro_step++;
                            win_outro_timer = 7*SEC;

                            break;
                        case 7:
                            Talk(SAY_DEATH_5);

                            win_outro_step++;
                            win_outro_timer = 15*SEC;

                            break;
                        case 8:
                            me->SetStandState(UNIT_STAND_STATE_STAND);
                            DoCast(SPELL_TELEPORT_VISUAL);

                            win_outro_step++;
                            win_outro_timer = 2*SEC;

                            break;
                        case 9:
                            me->SetVisible(false);
                            break;
                    }
            }

            // Outro after one hour
            void DoReplycodeOutro()
            {
                switch(replycode_outro_step)
                {
                    case 0:

                        events.Reset();
                        summons.DespawnAll();

                        me->InterruptNonMeleeSpells(false);
                        me->SetReactState(REACT_PASSIVE);
                        me->SetRespawnTime(604800);

                        me->RemoveAllAuras();
                        me->GetThreatManager().ClearAllThreat();
                        me->CombatStop(true);
                        me->SetLootRecipient(NULL);
                        me->ResetPlayerDamageReq();
                        me->GetMotionMaster()->Clear();
                        me->GetMotionMaster()->MoveIdle();
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);

                        replycode_outro_step++;
                        replycode_outro_timer = 1*SEC;

                        break;
                    case 1:
                        me->SetStandState(UNIT_STAND_STATE_KNEEL);

                        replycode_outro_step++;
                        replycode_outro_timer = 1*SEC;

                        break;
                    case 2:
                        Talk(SAY_TIMER_1);

                        replycode_outro_step++;
                        replycode_outro_timer = 15*SEC;

                        break;
                    case 3:
                        me->SetStandState(UNIT_STAND_STATE_STAND);
                        Talk(SAY_TIMER_2);

                        replycode_outro_step++;
                        replycode_outro_timer = 10*SEC;

                        break;
                    case 4:
                        DoCastAOE(SPELL_ASCEND);
                        Talk(SAY_TIMER_3);

                        replycode_outro_step++;
                        replycode_outro_timer = 10*SEC;

                        break;
                    case 5:
                        DoCast(SPELL_TELEPORT_VISUAL);
                        me->DespawnOrUnsummon(2*SEC);
                        break;
                }
            }
        };
};

class npc_collapsing_star : public CreatureScript
{
    public:
        npc_collapsing_star() : CreatureScript("npc_collapsing_star") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_collapsing_starAI(creature);
        }

        struct npc_collapsing_starAI : public PassiveAI
        {
            npc_collapsing_starAI(Creature *creature) : PassiveAI(creature) {}

            bool exploded;
            uint32 Timer;
            uint32 Damage;

            void Reset()
            {
                Timer = 1000;
                exploded = false;
                Damage = me->GetMaxHealth()/100;
                me->GetMotionMaster()->MoveRandom(15.0f);
            }
            void DamageTaken(Unit * /*Who*/, uint32 &Damage)
            {
                if(Damage > me->GetHealth())
                {
                    Damage = me->GetHealth() - 1;
                    Explode();
                }
            }

            void Explode()
            {
                if (exploded)
                    return;

                exploded = true;
                Timer = 1500;
                Position pos;

                me->RemoveAllAuras();
                me->GetPosition(&pos);
                if (me->GetOwnerGUID())
                    me->GetOwner()->SummonCreature(CREATURE_BLACK_HOLE, pos);

                DoCastAOE(SPELL_BLACK_HOLE_EXPLOSION, true);
                DoCastAOE(SPELL_BLACK_HOLE_SPAWN_VISUAL, true);
                me->GetMotionMaster()->Clear(false);
                me->GetMotionMaster()->MoveIdle();
                me->DespawnOrUnsummon(2000);
            }

            void UpdateAI(uint32 diff)
            {
                if (Timer <= diff)
                    {
                        if (Damage < me->GetHealth())
                            me->DealDamage(me, Damage, NULL, SELF_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL);
                        else
                            Explode();

                        Timer = 1000;
                }
                else Timer -= diff;
            }
        };
};

class npc_black_hole : public CreatureScript
{
    public:
        npc_black_hole() : CreatureScript("npc_black_hole") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_black_holeAI(creature);
        }

        struct npc_black_holeAI : public ScriptedAI
        {
            npc_black_holeAI(Creature *creature) : ScriptedAI(creature) {
                 SetCombatMovement(false);
            }

            uint32 ShiftTimer;

            void Reset()
            {
                ShiftTimer = 1000;
            }

            void DoAction(int32 ID)
            {
                if (ID == ACTION_BLACKHOLE_SUMMON)
                    me->CastSpell(me, SPELL_UNLEASHED_DARK_MATTER, false, 0, 0, me->GetOwnerGUID());
            }

            void UpdateAI(uint32 diff)
            {
                if (ShiftTimer <= diff)
                {
                    std::list<Player*> players;
                    Trinity::AnyPlayerInObjectRangeCheck checker(me, 1.5f);

                    me->VisitAnyNearbyObject<Player, Trinity::ContainerAction>(1.5f, players, checker);

                    for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    {
                        if (!(*itr)->HasAura(SPELL_BLACK_HOLE_AURA))
                            (*itr)->AddAura(SPELL_BLACK_HOLE_AURA, (*itr));

                        if (InstanceScript* instance = me->GetInstanceScript())
                        {
                            if(Creature* algalon = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_ALGALON))))
                            {
                                algalon->getHostileRefManager().deleteReference(*itr);
                                (*itr)->getHostileRefManager().deleteReference(algalon);
                            }
                        }

                        if (Pet* pet = (*itr)->GetPet())
                        {
                            if (pet->HasAura(SPELL_BLACK_HOLE_AURA))
                                pet->AddAura(SPELL_BLACK_HOLE_AURA, pet);
                            if (InstanceScript* instance = me->GetInstanceScript())
                                if(Creature* algalon = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_ALGALON))))
                                {
                                    algalon->getHostileRefManager().deleteReference(pet);
                                    pet->getHostileRefManager().deleteReference(algalon);
                                }
                        }
                    }
                    ShiftTimer = 1000;

                }
                else ShiftTimer -= diff;
            }

        };
};

class npc_living_constellation : public CreatureScript
{
    public:
        npc_living_constellation() : CreatureScript("npc_living_constellation") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_living_constellationAI(creature);
        }

        struct npc_living_constellationAI : public ScriptedAI
        {
            npc_living_constellationAI(Creature *creature) : ScriptedAI(creature) {}

            uint32 Arcane_Timer;
            uint32 checkTimer;
            bool active;

            void InitializeAI()
            {
                active = false;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                me->SetCanFly(true);
                me->SendMovementFlagUpdate();
                me->SetReactState(REACT_PASSIVE);
                me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 6);
                me->SetFloatValue(UNIT_FIELD_COMBATREACH, 6);
                Reset();
            }

            void Reset()
            {
                Arcane_Timer = urand(1000, 6500);
                checkTimer = 500;
            }

            void DoAction(int32 ActionID)
            {
                switch(ActionID)
                {
                case ACTION_ACTIVATE_CONSTELLATION:
                    active = true;
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->RemoveFlag( UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    DoZoneInCombat();
                    if (Unit* Target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                        AttackStart(Target);

                    break;
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!active)
                    return;

                if (!UpdateVictim())
                    return;

                if (me->GetVictim()->GetTypeId() != TYPEID_PLAYER)
                    me->GetThreatManager().ModifyThreatByPercent(me->GetVictim(), -100);

                if (checkTimer <= diff)
                {
                    if (Creature* BH = me->FindNearestCreature(CREATURE_BLACK_HOLE, 0.1f, true))
                    {
                        me->DespawnOrUnsummon();
                        BH->DespawnOrUnsummon();
                        if (Creature* Algalon = ObjectAccessor::GetCreature(*me, ObjectGuid(me->GetInstanceScript()->GetGuidData(BOSS_ALGALON))))
                            CAST_AI(boss_algalon_the_observer::boss_algalon_the_observerAI, Algalon->AI())->DoAction(ACTION_BLACK_HOLE_CLOSED);

                    }

                    checkTimer = 500;
                }
                else
                    checkTimer -= diff;
                if (Arcane_Timer <= diff)
                {
                    DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true), RAID_MODE(SPELL_ARCANE_BARAGE, H_SPELL_ARCANE_BARAGE),true);
                    Arcane_Timer = (2000);
                }
                else Arcane_Timer -= diff;
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage)
            {
                damage = 0;
            }

        };
};

class npc_dark_matter_algalon : public CreatureScript
{
    public:
        npc_dark_matter_algalon() : CreatureScript("npc_dark_matter_algalon") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_dark_matter_algalonAI(creature);
        }

        struct npc_dark_matter_algalonAI : public ScriptedAI
        {
            npc_dark_matter_algalonAI(Creature *creature) : ScriptedAI(creature) {}


            void InitializeAI()
            {
                me->SetPhaseMask(16, true);
                me->AddUnitMovementFlag(MOVEMENTFLAG_HOVER);
                Reset();
            }

            void Reset()
            {
                me->GetMotionMaster()->MoveRandom(60);
            }

            void UpdateAI(uint32 /*diff*/)
            {
                if (!UpdateVictim())
                    return;

                if (me->GetVictim()->GetPhaseMask() != 16)
                    EnterEvadeMode();

                DoMeleeAttackIfReady();
            }

        };
};

class npc_asteroid_trigger_algalon : public CreatureScript
{
    public:
        npc_asteroid_trigger_algalon() : CreatureScript("npc_asteroid_trigger_algalon") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_asteroid_trigger_algalonAI(creature);
        }

        struct npc_asteroid_trigger_algalonAI : public PassiveAI
        {
            npc_asteroid_trigger_algalonAI(Creature *creature) : PassiveAI(creature) {}

            uint32 Event_Timer;
            uint8 Event_Phase;

            void Reset()
            {
                Event_Timer = 3500;
                Event_Phase = 1;
            }

            void UpdateAI(uint32 diff)
            {
                if (Event_Timer <= diff)
                {
                    if (Event_Phase == 1)
                    {
                        if (Creature* Stalker = me->FindNearestCreature(CREATURE_ALGALON_ASTEROID_2, 45))
                            Stalker->CastSpell(me,SPELL_COSMIC_SMASH_MISSLE,true);
                        Event_Timer = 500;
                        ++Event_Phase;
                    }
                    else
                    {
                        me->AddAura(SPELL_COSMIC_SMASH_VISUAL,me);
                        Event_Timer = 10000000;
                    }
                }
                else Event_Timer -= diff;
            }
        };
};

class npc_brann_bronzebeard_algalon : public CreatureScript
{
    public:
        npc_brann_bronzebeard_algalon() : CreatureScript("npc_brann_bronzebeard_algalon") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_brann_bronzebeard_algalonAI(creature);
        }

        struct npc_brann_bronzebeard_algalonAI : public PassiveAI
        {
            npc_brann_bronzebeard_algalonAI(Creature *creature) : PassiveAI(creature) {}

            uint8 CurrWP;
            uint32 delay;

            bool ContinueWP;
            bool leaving;
            bool outro;


            void Reset()
            {
                CurrWP = 0;
                ContinueWP = false;
                delay = 0;

                me->SetReactState(REACT_PASSIVE);
                me->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
            }

            void DoAction(int32 ActionID)
            {
                switch(ActionID)
                {
                    case ACTION_BRANN_INTRO:
                        CurrWP = 0;
                        ContinueWP = true;
                        delay = 0;
                        break;
                    case ACTION_BRANN_LEAVE:
                        me->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                        Talk(SAY_BRANN_LEAVE);
                        CurrWP = 8;
                        ContinueWP = true;
                        delay = 0;
                        break;
                }
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (type != POINT_MOTION_TYPE)
                    return;
                switch(id)
                {
                    case 1: //standing at the door
                        me->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
                        delay = 8000;
                        break;
                    case 7: //at the summon Point
                        if (Creature* algalon = me->GetMap()->SummonCreature(NPC_ALGALON, algalonSpawnLocation)) {
                            algalon->AI()->DoAction(ACTION_ALGALON_SPAWN_INTRO);
                            algalon->SetVisible(false);
                        }
                        Talk(SAY_BRANN_INTRO);

                        me->GetMotionMaster()->Clear(false);
                        me->GetMotionMaster()->MoveIdle();
                        return;
                    case 8: //despawn point
                        me->GetMotionMaster()->Clear(false);
                        me->GetMotionMaster()->MoveIdle();
                        me->DespawnOrUnsummon();
                        return;
                }

                ++CurrWP;
                ContinueWP = true;
            }

            void UpdateAI(uint32 diff)
            {
                if (delay <= diff)
                {
                    if (ContinueWP)
                    {
                        me->GetMotionMaster()->MovePoint(CurrWP, WPs_ulduar[CurrWP][0], WPs_ulduar[CurrWP][1], WPs_ulduar[CurrWP][2]);
                        ContinueWP = false;
                    }
                }
                else delay -= diff;
            }

        };
};

class spell_algalon_phase_punch : public SpellScriptLoader
{
    public:
        spell_algalon_phase_punch() : SpellScriptLoader("spell_algalon_phase_punch") { }

        class spell_algalon_phase_punch_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_algalon_phase_punch_AuraScript);

            void HandleDummyTick(AuraEffect const* aurEff)
            {
                Unit* caster = GetCaster();
                Unit* target = GetTarget();
                if(!caster || !target)
                    return;

                switch (target->GetAuraCount(aurEff->GetId()))
                {
                case 1:
                    caster->AddAura(SPELL_PHASE_PUNCH_ALPHA_1,target);
                    break;
                case 2:
                    caster->AddAura(SPELL_PHASE_PUNCH_ALPHA_2,target);
                    break;
                case 3:
                    caster->AddAura(SPELL_PHASE_PUNCH_ALPHA_3,target);
                    break;
                case 4:
                    caster->AddAura(SPELL_PHASE_PUNCH_ALPHA_4,target);
                    break;
                case 5:
                    target->RemoveAurasDueToSpell(aurEff->GetId());
                    caster->AddAura(SPELL_PHASE_PUNCH_SHIFT, target);
                    target->CombatStop();
                    caster->GetThreatManager().ModifyThreatByPercent(target, -100);
                    target->GetThreatManager().resetAllAggro();
                    target->UpdateObjectVisibility();
                    break;
                }
            }
            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                if(target)
                {
                    target->RemoveAurasDueToSpell(SPELL_PHASE_PUNCH_ALPHA_1);
                    target->RemoveAurasDueToSpell(SPELL_PHASE_PUNCH_ALPHA_2);
                    target->RemoveAurasDueToSpell(SPELL_PHASE_PUNCH_ALPHA_3);
                    target->RemoveAurasDueToSpell(SPELL_PHASE_PUNCH_ALPHA_4);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_algalon_phase_punch_AuraScript::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
                OnEffectRemove += AuraEffectRemoveFn(spell_algalon_phase_punch_AuraScript::OnRemove, EFFECT_0,SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_algalon_phase_punch_AuraScript();
        };
};

class go_celestial_planetarium_access : public GameObjectScript
{
    public:
        go_celestial_planetarium_access()
            : GameObjectScript("go_celestial_planetarium_access")
        { }

        bool OnGossipHello(Player* player, GameObject* go)
        {
            InstanceScript* instance = go->GetInstanceScript();

            if (instance && instance->GetBossState(BOSS_XT002) == DONE)
            {
                // 10p can be opened with both keys, 25p only with heroic key
                if ((go->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL && player->HasItemCount(CELESTIAL_PLANETARIUM_KEY, 1))
                    || (player->HasItemCount(CELESTIAL_PLANETARIUM_KEY_H, 1)))
                {
                    // Prevent double spawns
                    if (instance->GetData(DATA_ALGALON_SUMMONED))
                        return true;

                    instance->SetData(DATA_ALGALON_SUMMONED, 1);

                    if (Creature* Brann = go->SummonCreature(NPC_BRANN_BRONZBEARD_ALG, WPs_ulduar[0][0],WPs_ulduar[0][1], WPs_ulduar[0][2]))
                    {
                        go->SetFlag(GAMEOBJECT_FLAGS,  GO_FLAG_NOT_SELECTABLE);
                        Brann->AI()->DoAction(ACTION_BRANN_INTRO);
                        if (GameObject* door = ObjectAccessor::GetGameObject(*go, ObjectGuid(instance->GetGuidData(DATA_SIGILDOOR_01))))
                            door->SetGoState(GO_STATE_ACTIVE);
                    }
                }
            }
            return true;
        }
};

class spell_algalon_cosmic_smash : public SpellScriptLoader
{
    public:
        spell_algalon_cosmic_smash() : SpellScriptLoader("spell_algalon_cosmic_smash") { }

        class spell_algalon_cosmic_smash_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_algalon_cosmic_smash_SpellScript);

            bool Load()
            {
                if (GetCaster()->GetTypeId() != TYPEID_UNIT)
                    return false;
                return true;
            }

            void HandleForceCast(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);

                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();
                uint32 triggered_spell_id = GetSpellInfo()->Effects[effIndex].TriggerSpell;

                if (caster && target)
                    target->CastSpell(target, triggered_spell_id, true, NULL, NULL, caster->GetGUID());
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_algalon_cosmic_smash_SpellScript::HandleForceCast, EFFECT_0, SPELL_EFFECT_FORCE_CAST);
                OnEffectHitTarget += SpellEffectFn(spell_algalon_cosmic_smash_SpellScript::HandleForceCast, EFFECT_1, SPELL_EFFECT_FORCE_CAST);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_algalon_cosmic_smash_SpellScript();
        }
};

class spell_algalon_black_hole : public SpellScriptLoader
{
    public:
        spell_algalon_black_hole() : SpellScriptLoader("spell_algalon_black_hole") { }

        class spell_algalon_black_hole_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_algalon_black_hole_AuraScript);


            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetOwner()->ToUnit();

                if (target->GetTypeId() != TYPEID_PLAYER)
                {
                    PreventDefaultAction();
                    return;
                }

                target->UpdateObjectVisibility();
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                if (target->GetTypeId() != TYPEID_PLAYER)
                {
                    PreventDefaultAction();
                    return;
                }

                target->UpdateObjectVisibility();
                // Removal after big bang
                if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_DEFAULT)
                    target->GetThreatManager().resetAllAggro();
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_algalon_black_hole_AuraScript::OnApply, EFFECT_0, SPELL_AURA_PHASE, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_algalon_black_hole_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PHASE, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_algalon_black_hole_AuraScript();
        };
};

class spell_algalon_summon_asteroid_stalkers : public SpellScriptLoader
{
    public:
        spell_algalon_summon_asteroid_stalkers() : SpellScriptLoader("spell_algalon_summon_asteroid_stalkers") { }

        class spell_algalon_summon_asteroid_stalkers_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_algalon_summon_asteroid_stalkers_SpellScript);

            void SpellEffect(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                Unit* caster = GetCaster();

                if (!GetOriginalCaster() || GetOriginalCaster() == caster)
                    return;

                uint32 entry = uint32(GetSpellInfo()->Effects[effIndex].MiscValue);
                SummonPropertiesEntry const* properties = sSummonPropertiesStore.LookupEntry(uint32(GetSpellInfo()->Effects[effIndex].MiscValueB));
                uint32 duration = uint32(GetSpellInfo()->GetDuration());

                Position pos;
                caster->GetPosition(&pos);
                if (entry == CREATURE_ALGALON_ASTEROID_2)
                    pos.m_positionZ -= 40.0f;
                TempSummon* summon = caster->GetMap()->SummonCreature(entry, pos, properties, duration, caster);
                if (!summon)
                    return;

                summon->SetDisplayId(summon->GetCreatureTemplate()->Modelid2);
                summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                summon->SetReactState(REACT_PASSIVE);
                if (entry == CREATURE_ALGALON_ASTEROID_2)
                {
                    summon->SetCanFly(true);
                    summon->SendMovementFlagUpdate();
                }
                summon->setFaction(GetOriginalCaster()->getFaction());

            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_algalon_summon_asteroid_stalkers_SpellScript::SpellEffect, EFFECT_0, SPELL_EFFECT_SUMMON);
            }
        };


        SpellScript* GetSpellScript() const
        {
            return new spell_algalon_summon_asteroid_stalkers_SpellScript();
        }
};

class spell_algalon_cosmic_smash_damage : public SpellScriptLoader
{
    public:
        spell_algalon_cosmic_smash_damage() : SpellScriptLoader("spell_algalon_cosmic_smash_damage") { }

        class spell_algalon_cosmic_smash_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_algalon_cosmic_smash_damage_SpellScript);

            void RecalculateDamage()
            {
                if (!GetExplTargetDest() || !GetHitUnit())
                    return;

                float distance = GetHitUnit()->GetDistance2d(GetExplTargetDest()->GetPositionX(), GetExplTargetDest()->GetPositionY());
                if (distance >= 6.0f)
                    SetHitDamage( (int32) (GetHitDamage() / distance));
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_algalon_cosmic_smash_damage_SpellScript::RecalculateDamage);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_algalon_cosmic_smash_damage_SpellScript();
        }
};

void AddSC_boss_algalon_the_observer()
{
    new boss_algalon_the_observer();
    new npc_asteroid_trigger_algalon();
    new npc_collapsing_star();
    new npc_living_constellation();
    new npc_black_hole();
    new npc_dark_matter_algalon();
    new npc_brann_bronzebeard_algalon();
    new spell_algalon_phase_punch();
    new spell_algalon_summon_asteroid_stalkers();
    new spell_algalon_cosmic_smash();
    new spell_algalon_cosmic_smash_damage();
    new spell_algalon_black_hole();
    new go_celestial_planetarium_access();
}
