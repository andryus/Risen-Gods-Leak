/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Arcatraz
SD%Complete: 60
SDComment: Warden Mellichar, event controller for Skyriss event. Millhouse Manastorm. @todo make better combatAI for Millhouse.
SDCategory: Tempest Keep, The Arcatraz
EndScriptData */

/* ContentData
npc_millhouse_manastorm
npc_warden_mellichar
npc_zerekethvoidzone
EndContentData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "arcatraz.h"
#include "SpellScript.h"

enum MillhouseSays
{
    SAY_INTRO_1                   = 0,
    SAY_INTRO_2                   = 1,
    SAY_WATER                     = 2,
    SAY_BUFFS                     = 3,
    SAY_DRINK                     = 4,
    SAY_READY                     = 5,
    SAY_KILL                      = 6,
    SAY_PYRO                      = 7,
    SAY_ICEBLOCK                  = 8,
    SAY_LOWHP                     = 9,
    SAY_DEATH                     = 10,
    SAY_COMPLETE                  = 11,
    SAY_INTRO_3                   = 12,
    SAY_INTRO_4                   = 13,
};

enum MillhouseSpells
{
    SPELL_CONJURE_WATER           = 36879,
    SPELL_ARCANE_INTELLECT        = 36880,
    SPELL_ICE_ARMOR               = 36881,
    SPELL_ARCANE_MISSILES         = 33832,
    SPELL_CONE_OF_COLD            = 12611,
    SPELL_FIRE_BLAST              = 13341,
    SPELL_FIREBALL                = 14034,
    SPELL_FROSTBOLT               = 15497,
    SPELL_PYROBLAST               = 33975,
    SPELL_ICEBLOCK                = 36911,
};

enum MillhouseEvents
{
    EVENT_MILLHOUSE_INTRO1        = 1,
    EVENT_MILLHOUSE_INTRO2        = 2,
    EVENT_MILLHOUSE_INTRO3        = 3,
    EVENT_MILLHOUSE_INTRO4        = 4,
    EVENT_MILLHOUSE_INTRO5        = 5,
    EVENT_MILLHOUSE_INTRO6        = 6,
    EVENT_MILLHOUSE_INTRO7        = 7,
    EVENT_MILLHOUSE_INTRO8        = 8,
    EVENT_MILLHOUSE_INTRO9        = 9,
    EVENT_SEARCH_FIGHT            = 10,
    EVENT_TELEPORT_VISUAL         = 11,
                                  
    EVENT_MILL_CHECK_HEALTH       = 20,
    EVENT_MILL_PYROBLAST          = 21,
    EVENT_MILL_BASE_SPELL         = 22,
    EVENT_MILL_ICEBLOCK           = 23
};

class npc_millhouse_manastorm : public CreatureScript
{
    public:
        npc_millhouse_manastorm() : CreatureScript("npc_millhouse_manastorm") { }
    
        struct npc_millhouse_manastormAI : public ScriptedAI
        {
            npc_millhouse_manastormAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }
                   
            void InitializeAI()
            {
                ScriptedAI::InitializeAI();
    
                me->SetReactState(REACT_PASSIVE);
                me->SetImmuneToAll(true);
                events2.Reset();
                events2.ScheduleEvent(EVENT_TELEPORT_VISUAL, 0);
                events2.ScheduleEvent(EVENT_MILLHOUSE_INTRO1, 3 * IN_MILLISECONDS);
            }
    
            void Reset() override
            {
                events.Reset();
            }
    
            void AttackStart(Unit* who) override
            {
                if (who && me->Attack(who, true))
                    me->GetMotionMaster()->MoveChase(who, 20.0f);
            }
    
            void KilledUnit(Unit* /*who*/) override
            {
                Talk(SAY_KILL);
            }
    
            void JustDied(Unit* /*killer*/) override
            {
                Talk(SAY_DEATH);
            }
    
            void EnterCombat(Unit*) override
            {
                events.ScheduleEvent(EVENT_MILL_CHECK_HEALTH, 1 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_MILL_PYROBLAST,   30 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_MILL_BASE_SPELL,   2 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_MILL_ICEBLOCK,     1 * IN_MILLISECONDS);
            }
    
            void UpdateAI(uint32 diff) override
            {
                events2.Update(diff);

                switch (events2.ExecuteEvent())
                {
                    case EVENT_TELEPORT_VISUAL:
                        DoCastSelf(SPELL_TELEPORT_VISUAL, true);
                        break;
                    case EVENT_MILLHOUSE_INTRO1:
                        Talk(SAY_INTRO_1);
                        events2.ScheduleEvent(EVENT_MILLHOUSE_INTRO2, 18 * IN_MILLISECONDS);
                        break;
                    case EVENT_MILLHOUSE_INTRO2:
                        Talk(SAY_INTRO_2);
                        events2.ScheduleEvent(EVENT_MILLHOUSE_INTRO3, 8 * IN_MILLISECONDS);
                        break;
                    case EVENT_MILLHOUSE_INTRO3:
                        Talk(SAY_INTRO_3);
                        events2.ScheduleEvent(EVENT_MILLHOUSE_INTRO4, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_MILLHOUSE_INTRO4:
                        Talk(SAY_INTRO_4);
                        events2.ScheduleEvent(EVENT_MILLHOUSE_INTRO5, 8 * IN_MILLISECONDS);
                        break;
                    case EVENT_MILLHOUSE_INTRO5:
                        Talk(SAY_WATER);
                        DoCastSelf(SPELL_CONJURE_WATER, false);
                        events2.ScheduleEvent(EVENT_MILLHOUSE_INTRO6, 7 * IN_MILLISECONDS);
                        break;
                    case EVENT_MILLHOUSE_INTRO6:
                        Talk(SAY_BUFFS);
                        DoCastSelf(SPELL_ICE_ARMOR, false);
                        events2.ScheduleEvent(EVENT_MILLHOUSE_INTRO7, 7 * IN_MILLISECONDS);
                        break;
                    case EVENT_MILLHOUSE_INTRO7:
                        Talk(SAY_DRINK);
                        DoCastSelf(SPELL_ARCANE_INTELLECT, false);
                        events2.ScheduleEvent(EVENT_MILLHOUSE_INTRO8, 7 * IN_MILLISECONDS);
                        break;
                    case EVENT_MILLHOUSE_INTRO8:
                        Talk(SAY_READY);
                        me->GetMotionMaster()->MovePoint(1, 445.82f, -158.38f, 43.067f);
                        events2.ScheduleEvent(EVENT_MILLHOUSE_INTRO9, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_MILLHOUSE_INTRO9:
                        me->SetFacingTo(M_PI_F * 1.5f);
                        me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), M_PI_F * 1.5f);
                        me->SetImmuneToAll(false);
                        me->SetReactState(REACT_AGGRESSIVE);
                        events2.ScheduleEvent(EVENT_SEARCH_FIGHT, 1 * IN_MILLISECONDS);
                        break;
                    case EVENT_SEARCH_FIGHT:
                        if (!me->IsInCombat() && !me->IsInEvadeMode())
                            if (Unit* target = me->SelectNearbyTarget(NULL, 30.0f))
                                AttackStart(target);
                        events2.ScheduleEvent(EVENT_SEARCH_FIGHT, 1 * IN_MILLISECONDS);
                        break;
    
                }
    
                if (!UpdateVictim())
                    return;
    
                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
    
                switch (events.ExecuteEvent())
                {
                    case EVENT_MILL_CHECK_HEALTH:
                        if (HealthBelowPct(20))
                        {
                            Talk(SAY_LOWHP);
                            break;
                        }
                        events.ScheduleEvent(EVENT_MILL_CHECK_HEALTH, 1 * IN_MILLISECONDS);
                        break;
                    case EVENT_MILL_PYROBLAST:
                        Talk(SAY_PYRO);
                        me->CastSpell(me->GetVictim(), SPELL_PYROBLAST, false);
                        events.ScheduleEvent(EVENT_MILL_PYROBLAST, 30 * IN_MILLISECONDS);
                        break;
                    case EVENT_MILL_ICEBLOCK:
                        if (me->GetDistance(me->GetVictim()) < 5.0f)
                        {
                            Talk(SAY_ICEBLOCK);
                            DoCastSelf(SPELL_ICEBLOCK, true);
                            break;
                        }
                        events.ScheduleEvent(EVENT_MILL_ICEBLOCK, 1 * IN_MILLISECONDS);
                        break;
                    case EVENT_MILL_BASE_SPELL:
                        switch (RAND(SPELL_FIREBALL, SPELL_ARCANE_MISSILES, SPELL_FROSTBOLT))
                        {
                        case SPELL_FIREBALL:
                            me->CastSpell(me->GetVictim(), SPELL_FIREBALL, false);
                            events.ScheduleEvent(EVENT_MILL_BASE_SPELL, 4 * IN_MILLISECONDS);
                            break;
                        case SPELL_ARCANE_MISSILES:
                            me->CastSpell(me->GetVictim(), SPELL_ARCANE_MISSILES, false);
                            events.ScheduleEvent(EVENT_MILL_BASE_SPELL, 9 * IN_MILLISECONDS);
                            break;
                        case SPELL_FROSTBOLT:
                            me->CastSpell(me->GetVictim(), SPELL_FROSTBOLT, false);
                            events.ScheduleEvent(EVENT_MILL_BASE_SPELL, 4 * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                        }
                        break;
                    
                }
    
                DoMeleeAttackIfReady();
            }

            private:
                InstanceScript* instance;
                EventMap events;
                EventMap events2;
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetArcatrazAI<npc_millhouse_manastormAI>(creature);
        }
};

enum WardenSays
{
    YELL_INTRO1                = 0,
    YELL_INTRO2                = 1,
    YELL_RELEASE1              = 2,
    YELL_RELEASE2A             = 3,
    YELL_RELEASE2B             = 4,
    YELL_RELEASE3              = 5,
    YELL_RELEASE4              = 6,
    YELL_WELCOME               = 7,
                               
    SAY_HARBINGER_INTRO        = 0,
};                             
                               
enum WardenUnits               
{                              
    NPC_TRICKSTER              = 20905,
    NPC_PH_HUNTER              = 20906,
    NPC_MILLHOUSE              = 20977,
    NPC_AKKIRIS                = 20908,
    NPC_SULFURON               = 20909,
    NPC_TW_DRAK                = 20910,
    NPC_BL_DRAK                = 20911,
};                             
                               
enum WardenSpells              
{                              
    SPELL_TARGET_ALPHA         = 36858,
    SPELL_TARGET_BETA          = 36854,
    SPELL_TARGET_DELTA         = 36856,
    SPELL_TARGET_GAMMA         = 36857,
    SPELL_TARGET_OMEGA         = 36852,
    SPELL_BUBBLE_VISUAL        = 36849,
                               
    SPELL_MIND_REND            = 36859,
    SPELL_QID10886             = 39564 // Trial of the Naaru: Tenacity
};                             
                               
enum WardenEvents              
{                              
    EVENT_WARDEN_CHECK_PLAYERS = 1,
    EVENT_WARDEN_INTRO1        = 2,
    EVENT_WARDEN_INTRO2,
    EVENT_WARDEN_INTRO3,
    EVENT_WARDEN_INTRO4,
    EVENT_WARDEN_INTRO5,
    EVENT_WARDEN_INTRO6,
    EVENT_WARDEN_INTRO7,
    EVENT_WARDEN_INTRO8,
    EVENT_WARDEN_INTRO9,
    EVENT_WARDEN_INTRO10,
    EVENT_WARDEN_INTRO11,
    EVENT_WARDEN_INTRO12,
    EVENT_WARDEN_INTRO13,
    EVENT_WARDEN_INTRO14,
    EVENT_WARDEN_INTRO15,
    EVENT_WARDEN_INTRO16,
    EVENT_WARDEN_INTRO17,
    EVENT_WARDEN_INTRO18,
    EVENT_WARDEN_INTRO19,
    EVENT_WARDEN_INTRO20,
    EVENT_WARDEN_INTRO21,
    EVENT_WARDEN_INTRO22,
    EVENT_WARDEN_INTRO23,
    EVENT_WARDEN_INTRO24,
    EVENT_WARDEN_INTRO25,
    EVENT_WARDEN_INTRO26,
    EVENT_WARDEN_INTRO27,
    EVENT_WARDEN_INTRO28,
    EVENT_WARDEN_INTRO29
};

class npc_warden_mellichar : public CreatureScript
{
    public:
        npc_warden_mellichar() : CreatureScript("npc_warden_mellichar") { }
    
        struct npc_warden_mellicharAI : public BossAI
        {
            npc_warden_mellicharAI(Creature* creature) : BossAI(creature, DATA_WARDEN_MELLICHAR)
            {
            }
    
            void JustSummoned(Creature* summon) override
            {
                summons.Summon(summon);
            }
    
            void SummonedCreatureDies(Creature* summon, Unit*) override
            {
                if (summon->GetEntry() == NPC_HARBINGER_SKYRISS)
                {
                    me->KillSelf();
                    me->setActive(false);
                    instance->SetBossState(DATA_WARDEN_MELLICHAR, DONE);
                    if (Creature* creature = me->FindNearestCreature(NPC_MILLHOUSE, 500.0f))
                    {
                        instance->DoCastSpellOnPlayers(SPELL_QID10886);
                        creature->AI()->Talk(SAY_COMPLETE);
                        creature->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    }
                }
            }
    
            void MoveInLineOfSight(Unit*) override { }
            void AttackStart(Unit*) override { }
            void EnterCombat(Unit*) override { }
    
            void JustDied(Unit*) override
            {
                me->setActive(false);
            }
    
            void Reset() override
            {
                BossAI::Reset();
                me->setActive(false);
                me->SetImmuneToAll(false);
                me->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);
                me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
                me->CastSpell((Unit*)NULL, SPELL_TARGET_OMEGA, false);
                instance->HandleGameObject(ObjectGuid(instance->GetGuidData(DATA_WARDENS_SHIELD)), true);
                instance->SetBossState(DATA_WARDEN_MELLICHAR, NOT_STARTED);
    
            }
    
            void DamageTaken(Unit* attacker, uint32& damage) override
            {
                if (attacker && attacker->GetCharmerOrOwnerOrOwnGUID().IsPlayer() && damage > 0 && !me->isActiveObject())
                {
                    me->setActive(true);
                    me->InterruptNonMeleeSpells(false);
                    me->SetImmuneToAll(true);
                    events.ScheduleEvent(EVENT_WARDEN_INTRO1, 1.5 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_WARDEN_CHECK_PLAYERS, 1 * IN_MILLISECONDS);
                    instance->SetBossState(DATA_WARDEN_MELLICHAR, IN_PROGRESS);
                }
                damage = 0;
            }
    
            void SetData(uint32 type, uint32 data) override
            {
                if (data == FAIL)
                {
                    CreatureAI::EnterEvadeMode();
                    return;
                }
                if (data != DONE)
                    return;
    
                switch (type)
                {
                    case DATA_WARDEN_1:
                        events.ScheduleEvent(EVENT_WARDEN_INTRO8, 2 * IN_MILLISECONDS);
                        break;
                    case DATA_WARDEN_3:
                        events.ScheduleEvent(EVENT_WARDEN_INTRO19, 2 * IN_MILLISECONDS);
                        break;
                    case DATA_WARDEN_4:
                        events.ScheduleEvent(EVENT_WARDEN_INTRO24, 2 * IN_MILLISECONDS);
                        break;
                }
            }
    
            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);
                switch (events.ExecuteEvent())
                {
                    case EVENT_WARDEN_CHECK_PLAYERS:
                        if (!me->FindNearestPlayer(100.0f))
                        {
                            CreatureAI::EnterEvadeMode();
                            return;
                        }
                        events.ScheduleEvent(EVENT_WARDEN_CHECK_PLAYERS, 1 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO1:
                        Talk(YELL_INTRO1);
                        me->SetFacingTo(M_PI_F / 2.0f);
                        DoCastSelf(SPELL_BUBBLE_VISUAL, false);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO2, 1.4 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO2:
                        instance->HandleGameObject(ObjectGuid(instance->GetGuidData(DATA_WARDENS_SHIELD)), false);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO3, 20 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO3:
                        Talk(YELL_INTRO2);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO4, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO4:
                        me->SetFacingTo(0.5f);
                        me->CastSpell((Unit*)NULL, SPELL_TARGET_ALPHA, false);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO5, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO5:
                        instance->SetData(DATA_WARDEN_1, IN_PROGRESS);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO6, 3 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO6:
                        me->SetFacingTo(M_PI_F * 1.5f);
                        me->CastSpell((Unit*)NULL, SPELL_TARGET_OMEGA, false);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO7, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO7:
                        me->SummonCreature(RAND(NPC_TRICKSTER, NPC_PH_HUNTER), 478.326f, -148.505f, 42.56f, 3.19f, TEMPSUMMON_MANUAL_DESPAWN);
                        // Wait for kill
                        break;
                    case EVENT_WARDEN_INTRO8:
                        Talk(YELL_RELEASE1);
                        me->InterruptNonMeleeSpells(false);
                        me->SetFacingTo(2.6f);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO10, 4 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO10:
                        me->CastSpell((Unit*)NULL, SPELL_TARGET_BETA, false);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO11, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO11:
                        Talk(YELL_RELEASE2A);
                        instance->SetData(DATA_WARDEN_2, IN_PROGRESS);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO12, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO12:
                        me->SetFacingTo(M_PI_F * 1.5f);
                        me->CastSpell((Unit*)NULL, SPELL_TARGET_OMEGA, false);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO13, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO13:
                        me->SummonCreature(NPC_MILLHOUSE, 413.292f, -148.378f, 42.56f, 6.27f, TEMPSUMMON_MANUAL_DESPAWN);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO14, 14 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO14:
                        Talk(YELL_RELEASE2B);
                        me->InterruptNonMeleeSpells(false);
                        me->SetFacingTo(3.3f);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO15, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO15:
                        me->CastSpell((Unit*)NULL, SPELL_TARGET_DELTA, false);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO16, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO16:
                        instance->SetData(DATA_WARDEN_3, IN_PROGRESS);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO17, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO17:
                        me->SetFacingTo(M_PI_F * 1.5f);
                        me->CastSpell((Unit*)NULL, SPELL_TARGET_OMEGA, false);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO18, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO18:
                        me->SummonCreature(RAND(NPC_AKKIRIS, NPC_SULFURON), 420.179f, -174.396f, 42.58f, 0.02f, TEMPSUMMON_MANUAL_DESPAWN);
                        // Wait for kill
                        break;
                    case EVENT_WARDEN_INTRO19:
                        Talk(YELL_RELEASE3);
                        me->InterruptNonMeleeSpells(false);
                        me->SetFacingTo(6.05f);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO20, 4 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO20:
                        me->CastSpell((Unit*)NULL, SPELL_TARGET_GAMMA, false);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO21, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO21:
                        instance->SetData(DATA_WARDEN_4, IN_PROGRESS);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO22, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO22:
                        me->SetFacingTo(M_PI_F * 1.5f);
                        me->CastSpell((Unit*)NULL, SPELL_TARGET_OMEGA, false);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO23, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO23:
                        me->SummonCreature(RAND(NPC_TW_DRAK, NPC_BL_DRAK), 471.795f, -174.58f, 42.58f, 3.06f, TEMPSUMMON_MANUAL_DESPAWN);
                        // Wait for kill
                        break;
                    case EVENT_WARDEN_INTRO24:
                        instance->SetData(DATA_WARDEN_5, IN_PROGRESS);
                        Talk(YELL_RELEASE4);
                        me->InterruptNonMeleeSpells(false);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO25, 8 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO25:
                        if (Creature* cr = me->SummonCreature(NPC_HARBINGER_SKYRISS, 445.763f, -191.639f, 44.64f, 1.60f, TEMPSUMMON_MANUAL_DESPAWN))
                        {
                            cr->SetImmuneToAll(true);
                            cr->CastSpell(cr, SPELL_TELEPORT_VISUAL, true);
                        }
                        events.ScheduleEvent(EVENT_WARDEN_INTRO26, 1 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO26:
                        if (Creature* creature = me->FindNearestCreature(NPC_HARBINGER_SKYRISS, 500.0f))
                            creature->AI()->Talk(SAY_HARBINGER_INTRO);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO27, 23 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO27:
                        Talk(YELL_WELCOME);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO28, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_WARDEN_INTRO28:
                        instance->HandleGameObject(ObjectGuid(instance->GetGuidData(DATA_WARDENS_SHIELD)), true);
                        if (Creature* creature = me->FindNearestCreature(NPC_HARBINGER_SKYRISS, 500.f))
                            creature->CastSpell((Unit*)NULL, SPELL_MIND_REND, false);
                        events.ScheduleEvent(EVENT_WARDEN_INTRO29, 4 * IN_MILLISECONDS);
                        break;               
                    case EVENT_WARDEN_INTRO29:
                        events.Reset();
                        me->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);
                        me->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
                        if (Creature* creature = me->FindNearestCreature(NPC_HARBINGER_SKYRISS, 500.0f))
                        {
                            creature->SetImmuneToAll(false);
                            if (Player* player = creature->FindNearestPlayer(100.0f))
                                AttackStart(player);
                        }
                        break;
                }
            }
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetArcatrazAI<npc_warden_mellicharAI>(creature);
        }
};

enum ZerekethSpell
{
    SPELL_VOID_ZONE_DAMAGE = 36120,
};

struct npc_zerekethvoidzoneAI : public ScriptedAI
{
    npc_zerekethvoidzoneAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        SetCombatMovement(false);
        me->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
        me->setFaction(FACTION_HOSTILE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

        DoCastSelf(SPELL_VOID_ZONE_DAMAGE);
    }

    void EnterCombat(Unit* /*who*/) override { }

    void UpdateAI(uint32 diff) override { }
};

class spell_arcatraz_soul_steal : public SpellScriptLoader
{
    public:
        spell_arcatraz_soul_steal() : SpellScriptLoader("spell_arcatraz_soul_steal") { }
    
        class spell_arcatraz_soul_steal_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_arcatraz_soul_steal_AuraScript)
    
            void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    caster->CastSpell(caster, SPELL_SOUL_STEAL, true);
            }
    
            void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    caster->RemoveAurasDueToSpell(SPELL_SOUL_STEAL);
            }
    
            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_arcatraz_soul_steal_AuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_arcatraz_soul_steal_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, AURA_EFFECT_HANDLE_REAL);
            }
        };
    
        AuraScript* GetAuraScript() const
        {
            return new spell_arcatraz_soul_steal_AuraScript();
        }
};

void AddSC_arcatraz()
{
    new npc_millhouse_manastorm();
    new npc_warden_mellichar();
    new CreatureScriptLoaderEx<npc_zerekethvoidzoneAI>("npc_zerekethvoidzone");
    new spell_arcatraz_soul_steal();
}
