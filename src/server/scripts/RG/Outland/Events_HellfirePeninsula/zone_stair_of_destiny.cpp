/* 
 * Copyright (C) 2006-2008 ScriptDev2  <https://scriptdev2.svn.sourceforge.net/>
 * Copyright (C) 2008-2014 Hellground  <http://hellground.net/>
 * Copyright (C) 2010-2017 Rising Gods <http://www.rising-gods.de/>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

enum DarkPortalMisc
{
    AGGRO_YELL                      = 0,
};

enum DarkPortalCreatures 
{
    NPC_WRATH_MASTER                = 19005,
    NPC_FEL_SOLDIER                 = 18944,
    NPC_INFERNAL_SIEGEBREAKER       = 18946,

    // misc
    NPC_INFERNAL_RELAY              = 19215,
    NPC_INFERNAL_TARGET             = 21075,
    NPC_CASTER_RIGHT                = 1010916,
    NPC_CASTER_LEFT                 = 1010917,
    NPC_TARGET_RIGHT                = 1010918,
    NPC_TARGET_LEFT                 = 1010919,
    FINAL_POS_TRIGGER               = 1010920,

    NPC_STRENGTH_TOTEM              = 19225,
};

enum DarkPortalSpells
{
    // npc_pit_commander
    SPELL_CLEAVE_COMMANDER          = 16044,
    SPELL_RAIN_OF_FIRE              = 33627,
    SPELL_SUMMON_INFERNALS          = 33393,
    SPELL_INFERNALS_RAIN            = 32785,

    // npc_melgromm_highmountain
    SPELL_CHAIN_HEAL                = 33642,
    SPELL_CHAIN_LIGHTNING           = 33643,
    SPELL_MAGMA_FLOW_TOTEM          = 33560,
    SPELL_STORM_TOTEM               = 33570,
    SPELL_EARTH_SHOCK               = 22885,
    SPELL_REVIVE_SELF               = 32343,

    // npc_justinus_the_harbinger
    SPELL_DIVINE_SHIELD             = 33581,
    SPELL_JUDGEMENT                 = 33554,
    SPELL_BLESSING_MIGHT            = 33564,
    SPELL_CONSECRATION              = 33559,
    SPELL_FLASH_OF_LIGHT            = 33641,

    // npc_stormwind_mage && npc_undercity_mage
    SPELL_FIREBALL                  = 33417,
    SPELL_ARCANE_E                  = 33623,
    SPELL_BLIZZARD                  = 33624,
    SPELL_ICEBOLT                   = 33463,

    // npc_dark_portal_invader
    SPELL_CLEAVE_INVADER            = 15496,
    SPELL_CUTDOWN                   = 32009,

    // npc_stormwind_soldier && npc_orgrimmar_grunt
    SPELL_STRIKE                    = 33626,

    // npc_ironforge_paladin
    SPELL_EXORCISM                  = 33632,
    SPELL_HAMMER_OF_JUSTICE         = 13005,
    SPELL_SEAL_OF_SACRIFICE         = 13903,

    // npc_darnassian_archer
    SPELL_SHOT                      = 15547,  // Sure not correct?

    // npc_darkspear_axe_thrower
	SPELL_THROW                     = 29582, // Sure not correct?

    // npc_orgrimmar_shaman
	SPELL_FLAME_SHOCK               = 15616,

    // npc_infernal_siegebreaker
    SPELL_INFERNAL                  = 33637,
};

enum DarkPortalEvents
{
    // npc_pit_commander
    EVENT_INFERNAL_TIMER = 1,
    EVENT_INVADERS_TIMER,
    EVENT_CLEAVE_TIMER,
    EVENT_RAIN_OF_FIRE_TIMER,

    // npc_melgromm_highmountain
    EVENT_STORM_TOTEM_TIMER,
    EVENT_CHAIN_HEAL_TIMER,
    EVENT_CHAIN_LIGHT_TIMER,
    EVENT_MAGMA_FLOW_TIMER,
    EVENT_EARTH_SHOCK_TIMER,

    // npc_justinus_the_harbinger
    EVENT_DIVINE_SHIELD_TIMER,
    EVENT_JUDGEMENT_TIMER,
    EVENT_BLESSING_MIGHT_TIMER,
    EVENT_CONSECRATION_TIMER,
    EVENT_FLASH_OF_LIGHT_TIMER,

    // npc_stormwind_mage && npc_undercity_mage
    EVENT_READY_TO_CAST,

    // npc_stormwind_soldier && npc_orgrimmar_grunt
    EVENT_STRIKE_TIMER,

    // npc_ironforge_paladin
    EVENT_EXORCISM_TIMER,
    EVENT_JUSTICE_TIMER,
    EVENT_SACRIFICE_TIMER,

    // npc_darnassian_archer
    EVENT_SHOOT_TIMER,

    // npc_darkspear_axe_thrower
    EVENT_THROW_TIMER,

    // npc_orgrimmar_shaman
    EVENT_SHOCK_TIMER,

    // npc_dark_portal_invader
    EVENT_CUT_DOWN_TIMER,
    EVENT_READY_TO_INVADE_TIMER,
};

const float FirstFormation[5][4] =
{
    {-218.895f, 1502.76f, 20.7800f, 4.2106f},  // Left  1 - Fel Soldier
    {-209.949f, 1508.24f, 21.9920f, 4.4965f},  // Left  2 - Fel Soldier
    {-236.579f, 1506.04f, 21.4575f, 4.6764f},  // Right 1 - Fel Soldier
    {-241.711f, 1514.15f, 22.6739f, 4.4965f},  // Right 2 - Fel Soldier
    {-229.307f, 1497.77f, 20.1869f, 4.4832f}   // Center - Wrath Master
};

const float SecondFormation[8][4] =
{ 
    {-213.005f, 1496.72f, 20.9439f, 4.4745f},  // 1 1 - Fel Soldier
    {-223.101f, 1499.17f, 20.3675f, 4.4745f},  // 1 2 - Fel Soldier
    {-234.240f, 1501.86f, 20.8176f, 4.4745f},  // 1 3 - Fel Soldier
    {-243.119f, 1503.89f, 21.7681f, 4.4745f},  // 1 4 - Fel Soldier
    {-241.454f, 1514.69f, 22.7010f, 4.4745f},  // 2 1 - Fel Soldier
    {-231.970f, 1512.72f, 22.0880f, 4.4745f},  // 2 2 - Fel Soldier
    {-220.767f, 1509.81f, 21.4782f, 4.4745f},  // 2 3 - Fel Soldier
    {-210.363f, 1507.31f, 21.8889f, 4.4745f}   // 2 4 - Fel Soldier
};

// Waypoints
const uint32 FirstPathList[5]  = {98250, 98251, 98252, 98253, 98254};
const uint32 SecondPathList[8] = {98255, 98256, 98257, 98258, 98259, 98260, 98261, 98262};

struct npc_pit_commanderAI : public ScriptedAI
{
    npc_pit_commanderAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_INFERNAL_TIMER,      1 * IN_MILLISECONDS);
        _events.ScheduleEvent(EVENT_INVADERS_TIMER,      1 * IN_MILLISECONDS);
        _events.ScheduleEvent(EVENT_CLEAVE_TIMER,        5 * IN_MILLISECONDS);
        _events.ScheduleEvent(EVENT_RAIN_OF_FIRE_TIMER, 15 * IN_MILLISECONDS);
        me->setActive(true);
    }

    void SummonInvaders()
    {
        switch (urand(0,1))
        {
            // First Formation
            case 0:
            {
                for (int i = 0; i < 4; i++)
                {
                    if (Unit* Solider = me->SummonCreature(NPC_FEL_SOLDIER, FirstFormation[i][0], FirstFormation[i][1], FirstFormation[i][2], FirstFormation[i][3], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10 * IN_MILLISECONDS))
                    {
                        Solider->setActive(true);
                        Solider->AddUnitState(UNIT_STATE_IGNORE_PATHFINDING);
                        Solider->GetMotionMaster()->MovePath(FirstPathList[i], false);
                    }
                }

                if (Unit* WrathMaster = me->SummonCreature(NPC_WRATH_MASTER, FirstFormation[4][0], FirstFormation[4][1], FirstFormation[4][2], FirstFormation[4][3], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10 * IN_MILLISECONDS))
                {
                    WrathMaster->setActive(true);
                    WrathMaster->AddUnitState(UNIT_STATE_IGNORE_PATHFINDING);
                    WrathMaster->GetMotionMaster()->MovePath(FirstPathList[4], false);
                }
            } break;
            // Second Formation
            case 1:
            { 
                for (int i = 0; i < 8; i++)
                {
                    if (Unit* Solider = me->SummonCreature(NPC_FEL_SOLDIER, SecondFormation[i][0], SecondFormation[i][1], SecondFormation[i][2], SecondFormation[i][3], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10 * IN_MILLISECONDS))
                    {
                        Solider->setActive(true);
                        Solider->AddUnitState(UNIT_STATE_IGNORE_PATHFINDING);
                        Solider->GetMotionMaster()->MovePath(SecondPathList[i], false);
                    }
                }
            } break;
        } 
    }

    void InfernalEvent()
    {
        if (Unit* CasterRight = me->FindNearestCreature(NPC_CASTER_RIGHT, 150.0f))
            if (Unit* TargetRight = me->FindNearestCreature(NPC_TARGET_RIGHT, 150.0f))
                CasterRight->CastSpell(TargetRight, SPELL_INFERNALS_RAIN, true);

        if (Unit* CasterLeft = me->FindNearestCreature(NPC_CASTER_LEFT, 150.0f))
            if (Unit* TargetLeft = me->FindNearestCreature(NPC_TARGET_LEFT, 150.0f))
                CasterLeft->CastSpell(TargetLeft, SPELL_INFERNALS_RAIN, true);
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (who->GetTypeId() == TYPEID_UNIT)
            return;
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_INFERNAL_TIMER:
                    DoCastSelf(SPELL_SUMMON_INFERNALS);
                    InfernalEvent();
                    _events.ScheduleEvent(EVENT_INFERNAL_TIMER, 45 * IN_MILLISECONDS);
                    break;
                case EVENT_INVADERS_TIMER:
                    SummonInvaders();
                    _events.ScheduleEvent(EVENT_INVADERS_TIMER, 60 * IN_MILLISECONDS);
                    break;
                case EVENT_CLEAVE_TIMER:
                    DoCastVictim(SPELL_CLEAVE_COMMANDER);
                    _events.ScheduleEvent(EVENT_CLEAVE_TIMER, 20 * IN_MILLISECONDS);
                    break;
                case EVENT_RAIN_OF_FIRE_TIMER:
                    DoCastVictim(SPELL_RAIN_OF_FIRE);
                    _events.ScheduleEvent(EVENT_RAIN_OF_FIRE_TIMER, 30 * IN_MILLISECONDS);
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
        EventMap _events;
};

struct npc_melgromm_highmountainAI : public ScriptedAI
{
    npc_melgromm_highmountainAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_STORM_TOTEM_TIMER,  15 * IN_MILLISECONDS);
        _events.ScheduleEvent(EVENT_CHAIN_HEAL_TIMER,   12 * IN_MILLISECONDS);
        _events.ScheduleEvent(EVENT_CHAIN_LIGHT_TIMER,  10 * IN_MILLISECONDS);
        _events.ScheduleEvent(EVENT_MAGMA_FLOW_TIMER,    5 * IN_MILLISECONDS);
        _events.ScheduleEvent(EVENT_EARTH_SHOCK_TIMER,   8 * IN_MILLISECONDS);

        me->setActive(true);
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void EnterCombat(Unit* who) override
    {
        if (who->GetTypeId() == TYPEID_PLAYER)
            return;

        if (who->GetEntry() == NPC_INFERNAL_SIEGEBREAKER && roll_chance_i(30))
            Talk(AGGRO_YELL);
    }

    void DamageTaken(Unit* done_by, uint32 &damage) override
    {
        if (done_by->GetTypeId() == TYPEID_PLAYER)
            damage = 0;
    }

    void JustDied(Unit* Killer) override
    {
        // WoWhead: Respawns instantly if killed, so does Justinius. 
        me->Respawn();
    } 

    void UpdateAI(uint32 diff) override
    {
        if (!me->IsInCombat())
            if (Unit* AliveInvader = me->SelectNearestTarget(45.0f))
                me->AI()->AttackStart(AliveInvader);

        if (!UpdateVictim())
            return;

        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_STORM_TOTEM_TIMER:
                    if (!me->IsInCombat())
                    {
                        Unit * Totem = me->FindNearestCreature(NPC_STRENGTH_TOTEM, 20.0f);

                        if (!Totem)
                            DoCastSelf(SPELL_STORM_TOTEM);
                    }
                    _events.ScheduleEvent(EVENT_STORM_TOTEM_TIMER, 15 * IN_MILLISECONDS);
                    break;
                case EVENT_CHAIN_HEAL_TIMER:
                    DoCastSelf(SPELL_CHAIN_HEAL);
                    _events.ScheduleEvent(EVENT_CHAIN_HEAL_TIMER, 12 * IN_MILLISECONDS);
                    break;
                case EVENT_CHAIN_LIGHT_TIMER:
                    DoCastVictim(SPELL_CHAIN_LIGHTNING);
                    _events.ScheduleEvent(EVENT_CHAIN_LIGHT_TIMER, 10 * IN_MILLISECONDS);
                    break;
                case EVENT_MAGMA_FLOW_TIMER:
                    DoCastSelf(SPELL_MAGMA_FLOW_TOTEM);
                    _events.ScheduleEvent(EVENT_MAGMA_FLOW_TIMER, 5 * IN_MILLISECONDS);
                    break;
                case EVENT_EARTH_SHOCK_TIMER:
                    DoCastVictim(SPELL_EARTH_SHOCK);
                    _events.ScheduleEvent(EVENT_EARTH_SHOCK_TIMER, 8 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

    private:
        EventMap _events;
};

struct npc_justinus_the_harbingerAI : public ScriptedAI
{
    npc_justinus_the_harbingerAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_DIVINE_SHIELD_TIMER,  20 * IN_MILLISECONDS);
        _events.ScheduleEvent(EVENT_JUDGEMENT_TIMER,       5 * IN_MILLISECONDS);
        _events.ScheduleEvent(EVENT_BLESSING_MIGHT_TIMER, 30 * IN_MILLISECONDS);
        _events.ScheduleEvent(EVENT_CONSECRATION_TIMER,    8 * IN_MILLISECONDS);
        _events.ScheduleEvent(EVENT_FLASH_OF_LIGHT_TIMER, 12 * IN_MILLISECONDS);

        me->setActive(true);
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void EnterCombat(Unit* who) override
    {
        if (who->GetTypeId() == TYPEID_PLAYER)
            return;

        if (who->GetEntry() == NPC_INFERNAL_SIEGEBREAKER && roll_chance_i(30))
            Talk(AGGRO_YELL);
    }

    void DamageTaken(Unit* done_by, uint32 &damage) override
    {
        if (done_by->GetTypeId() == TYPEID_PLAYER)
            damage = 0;
    }

    void JustDied(Unit* Killer) override
    {
        // WoWhead: Respawns instantly if killed, so does Mulgromm. 
        me->Respawn();
    } 

    void UpdateAI(uint32 diff) override
    {
        if (!me->IsInCombat())
            if (Unit* AliveInvader = me->SelectNearestTarget(45.0f))
                me->AI()->AttackStart(AliveInvader);

        if (!UpdateVictim())
            return;

        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_BLESSING_MIGHT_TIMER:
                    DoCastSelf(SPELL_BLESSING_MIGHT);
                    _events.ScheduleEvent(EVENT_BLESSING_MIGHT_TIMER, 30 * IN_MILLISECONDS);
                    break;
                case EVENT_DIVINE_SHIELD_TIMER:
                    DoCastSelf(SPELL_DIVINE_SHIELD);
                    _events.ScheduleEvent(EVENT_DIVINE_SHIELD_TIMER, 20 * IN_MILLISECONDS);
                    break;
                case EVENT_JUDGEMENT_TIMER:
                    DoCastVictim(SPELL_JUDGEMENT);
                    _events.ScheduleEvent(EVENT_JUDGEMENT_TIMER, 5 * IN_MILLISECONDS);
                    break;
                case EVENT_CONSECRATION_TIMER:
                    DoCastSelf(SPELL_CONSECRATION);
                    _events.ScheduleEvent(EVENT_CONSECRATION_TIMER, 8 * IN_MILLISECONDS);
                    break;
                case EVENT_FLASH_OF_LIGHT_TIMER:
                    DoCastSelf(SPELL_FLASH_OF_LIGHT);
                    _events.ScheduleEvent(EVENT_FLASH_OF_LIGHT_TIMER, 12 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

    private:
        EventMap _events;
};

struct npc_stormwind_mageAI : public ScriptedAI
{
    npc_stormwind_mageAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_READY_TO_CAST, 3 * IN_MILLISECONDS);

        me->setActive(true);
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void EnterCombat(Unit* who) override
    {
        if (who->GetTypeId() == TYPEID_PLAYER)
            return;
    }

    void DamageTaken(Unit* done_by, uint32 &damage) override
    {
        if (done_by->GetTypeId() == TYPEID_PLAYER)
            damage = 0;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!me->IsInCombat())
            if (Unit* AliveInvader = me->SelectNearestTarget(45.0f))
                me->AI()->AttackStart(AliveInvader);

        if (!UpdateVictim())
            return;

        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_READY_TO_CAST:
                    DoCastVictim(urand(0, 1) ? SPELL_FIREBALL : SPELL_BLIZZARD);
                    _events.ScheduleEvent(EVENT_READY_TO_CAST, 3 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }
    private:
        EventMap _events;
};

struct npc_undercity_mageAI : public ScriptedAI
{
    npc_undercity_mageAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_READY_TO_CAST, 3 * IN_MILLISECONDS);

        me->setActive(true);
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void EnterCombat(Unit* who) override
    {
        if (who->GetTypeId() == TYPEID_PLAYER)
            return;
    }

    void DamageTaken(Unit* done_by, uint32 &damage) override
    {
        if (done_by->GetTypeId() == TYPEID_PLAYER)
            damage = 0;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!me->IsInCombat())
            if (Unit* AliveInvader = me->SelectNearestTarget(45.0f))
                me->AI()->AttackStart(AliveInvader);

        if (!UpdateVictim())
            return;

        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_READY_TO_CAST:
                    DoCastVictim(urand(0, 1) ? SPELL_ICEBOLT : SPELL_BLIZZARD);
                    _events.ScheduleEvent(EVENT_READY_TO_CAST, 3 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

    private:
        EventMap _events;
};

const float runPosition[2][3] =
{ 
    {-240.848f, 1094.25f, 41.7500f},
    {-261.527f, 1093.32f, 41.7500f}
};

struct npc_dark_portal_invaderAI : public ScriptedAI
{
    npc_dark_portal_invaderAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {  
        _events.Reset();
        _events.ScheduleEvent(EVENT_CLEAVE_TIMER,   urand(3, 5) * IN_MILLISECONDS);
        _events.ScheduleEvent(EVENT_CUT_DOWN_TIMER, urand(6, 8) * IN_MILLISECONDS);

        me->setActive(true);
        me->SetReactState(REACT_AGGRESSIVE);
        _atFinalPosition = false;
        me->SetWalk(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->FindNearestCreature(FINAL_POS_TRIGGER, 3.0f) && !_atFinalPosition)
        {
            _events.ScheduleEvent(EVENT_READY_TO_INVADE_TIMER, 15 * IN_MILLISECONDS);
            _atFinalPosition = true;
        }

        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_READY_TO_INVADE_TIMER:
                    me->ClearUnitState(UNIT_STATE_IGNORE_PATHFINDING);
                    me->SetWalk(false);
                    me->GetMotionMaster()->Clear();

                    if (urand(1, 10) < 5)
                        me->GetMotionMaster()->MovePoint(1, runPosition[0][0], runPosition[0][1], runPosition[0][2]); 
                    else 
                        me->GetMotionMaster()->MovePoint(1, runPosition[1][0], runPosition[1][1], runPosition[1][2]);
                    break;
                case EVENT_CLEAVE_TIMER:
                    if (me->GetEntry() == NPC_FEL_SOLDIER)
                        DoCastVictim(SPELL_CLEAVE_INVADER);
                    _events.ScheduleEvent(EVENT_CLEAVE_TIMER, urand(3, 5) * IN_MILLISECONDS);
                    break;
                case EVENT_CUT_DOWN_TIMER:
                    if (me->GetEntry() == NPC_FEL_SOLDIER)
                        DoCastVictim(SPELL_CUTDOWN);
                    _events.ScheduleEvent(EVENT_CUT_DOWN_TIMER, urand(6, 8) * IN_MILLISECONDS);
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
        EventMap _events;
        bool     _atFinalPosition;
};

struct npc_stormwind_soldierAI : public ScriptedAI
{
    npc_stormwind_soldierAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {  
        _events.Reset();
        _events.ScheduleEvent(EVENT_STRIKE_TIMER, urand(3, 5) * IN_MILLISECONDS);

        me->setActive(true);
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void EnterCombat(Unit* who) override
    {
        if (who->GetTypeId() == TYPEID_PLAYER)
            return;
    }

    void DamageTaken(Unit* done_by, uint32 &damage) override
    {
        if (done_by->GetTypeId() == TYPEID_PLAYER)
            damage = 0;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_STRIKE_TIMER:
                    DoCastVictim(SPELL_STRIKE);
                    _events.ScheduleEvent(EVENT_STRIKE_TIMER, urand(5, 8) * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

    private:
        EventMap _events;   
};

struct npc_orgrimmar_gruntAI : public ScriptedAI
{
    npc_orgrimmar_gruntAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {  
        _events.Reset();
        _events.ScheduleEvent(EVENT_STRIKE_TIMER, urand(3, 5) * IN_MILLISECONDS);

        me->setActive(true);
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void EnterCombat(Unit* who) override
    {
        if (who->GetTypeId() == TYPEID_PLAYER)
            return;
    }

    void DamageTaken(Unit* done_by, uint32 &damage) override
    {
        if (done_by->GetTypeId() == TYPEID_PLAYER)
            damage = 0;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_STRIKE_TIMER:
                    DoCastVictim(SPELL_STRIKE);
                    _events.ScheduleEvent(EVENT_STRIKE_TIMER, urand(5, 8) * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

    private:
        EventMap _events;       
};

struct npc_ironforge_paladinAI : public ScriptedAI
{
    npc_ironforge_paladinAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {  
        _events.Reset();
        _events.ScheduleEvent(EVENT_EXORCISM_TIMER, urand(2, 3) * IN_MILLISECONDS);
        _events.ScheduleEvent(EVENT_JUSTICE_TIMER, urand(7, 9) * IN_MILLISECONDS);
        _events.ScheduleEvent(EVENT_SACRIFICE_TIMER, urand(10, 12) * IN_MILLISECONDS);

        me->setActive(true);
        me->SetReactState(REACT_AGGRESSIVE);
    }
    
    void EnterCombat(Unit* who) override
    {
        if (who->GetTypeId() == TYPEID_PLAYER)
            return;
    }

    void DamageTaken(Unit* done_by, uint32 &damage) override
    {
        if (done_by->GetTypeId() == TYPEID_PLAYER)
            damage = 0;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_EXORCISM_TIMER:
                    DoCastVictim(SPELL_EXORCISM);
                    _events.ScheduleEvent(EVENT_EXORCISM_TIMER, urand(2, 3) * IN_MILLISECONDS);
                    break;
                case EVENT_JUSTICE_TIMER:
                    DoCastVictim(SPELL_HAMMER_OF_JUSTICE);
                    _events.ScheduleEvent(EVENT_JUSTICE_TIMER, urand(7, 9) * IN_MILLISECONDS);
                    break;
                case EVENT_SACRIFICE_TIMER:
                    DoCastSelf(SPELL_SEAL_OF_SACRIFICE);
                    _events.ScheduleEvent(EVENT_SACRIFICE_TIMER, urand(10, 12) * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

    private:
        EventMap _events;    
};

struct npc_darnassian_archerAI : public ScriptedAI
{
    npc_darnassian_archerAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {  
        _events.Reset();
        _events.ScheduleEvent(EVENT_SHOOT_TIMER, 1500);

        me->setActive(true);
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void EnterCombat(Unit* who) override
    {
        if (who->GetTypeId() == TYPEID_PLAYER)
            return;
    }

    void DamageTaken(Unit* done_by, uint32 &damage) override
    {
        if (done_by->GetTypeId() == TYPEID_PLAYER)
            damage = 0;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!me->IsInCombat())
            if (Unit* AliveInvader = me->SelectNearestTarget(45.0f))
                me->AI()->AttackStart(AliveInvader);

        if (!UpdateVictim())
            return;

        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SHOOT_TIMER:
                    DoCastVictim(SPELL_SHOT);
                    _events.ScheduleEvent(EVENT_SHOOT_TIMER, 1500);
                    break;
                default:
                    break;
            }
        }

        DoStartNoMovement(me->GetVictim());

        DoMeleeAttackIfReady();
    }

    private:
        EventMap _events;
};

struct npc_darkspear_axe_throwerAI : public ScriptedAI
{
    npc_darkspear_axe_throwerAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {  
        _events.Reset();
        _events.ScheduleEvent(EVENT_THROW_TIMER, urand(2, 3) * IN_MILLISECONDS);
        me->setActive(true);
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void EnterCombat(Unit* who) override
    {
        if (who->GetTypeId() == TYPEID_PLAYER)
            return;
    }

    void DamageTaken(Unit* done_by, uint32 &damage) override
    {
        if (done_by->GetTypeId() == TYPEID_PLAYER)
            damage = 0;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!me->IsInCombat())
            if (Unit* AliveInvader = me->SelectNearestTarget(45.0f))
                me->AI()->AttackStart(AliveInvader);

        if (!UpdateVictim())
            return;

        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_THROW_TIMER:
                    DoCastVictim(SPELL_THROW);
                    _events.ScheduleEvent(EVENT_THROW_TIMER, 1500);
                    break;
                default:
                    break;
            }
        }

        DoStartNoMovement(me->GetVictim());

        DoMeleeAttackIfReady();
    }

    private:
        EventMap _events;
};

struct npc_orgrimmar_shamanAI : public ScriptedAI
{
    npc_orgrimmar_shamanAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {  
        _events.Reset();
        _events.ScheduleEvent(EVENT_SHOCK_TIMER, urand(2, 3) * IN_MILLISECONDS);
        me->setActive(true);
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void EnterCombat(Unit* who) override
    {
        if (who->GetTypeId() == TYPEID_PLAYER)
            return;
    }

    void DamageTaken(Unit* done_by, uint32 &damage) override
    {
        if (done_by->GetTypeId() == TYPEID_PLAYER)
            damage = 0;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SHOCK_TIMER:
                    DoCastVictim(SPELL_FLAME_SHOCK);
                    _events.ScheduleEvent(EVENT_SHOCK_TIMER, urand(5, 8) * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

    private:
        EventMap _events;
};

struct npc_infernal_siegebreakerAI : public ScriptedAI
{
    npc_infernal_siegebreakerAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {  
        me->setActive(true);
        me->SetReactState(REACT_AGGRESSIVE);
        _stunedEnemy = false;
    }

    void JustDied(Unit* /*who*/) override
    {
        me->DespawnOrUnsummon(10 * IN_MILLISECONDS);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!me->IsInCombat())
            if (Unit* AliveInvader = me->SelectNearestTarget(45.0f))
                me->AI()->AttackStart(AliveInvader);

        if (!_stunedEnemy)
        {
            // WoWhead: If you're hit by the large AoE blast that comes as they hit the ground, you take roughly 3500 damage.
            int32 basepoints0 = urand(3400,3600);
            me->CastCustomSpell(me, SPELL_INFERNAL, &basepoints0, NULL, NULL, true);
            _stunedEnemy = true;
        }

        if (!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }

    private:
        bool _stunedEnemy;
};

void AddSC_stair_of_destiny_rg()
{
    new CreatureScriptLoaderEx<npc_pit_commanderAI>("npc_pit_commander");
    new CreatureScriptLoaderEx<npc_melgromm_highmountainAI>("npc_melgromm_highmountain");
    new CreatureScriptLoaderEx<npc_justinus_the_harbingerAI>("npc_justinus_the_harbinger");
    new CreatureScriptLoaderEx<npc_stormwind_mageAI>("npc_stormwind_mage");
    new CreatureScriptLoaderEx<npc_undercity_mageAI>("npc_undercity_mage");
    new CreatureScriptLoaderEx<npc_dark_portal_invaderAI>("npc_dark_portal_invader");
    new CreatureScriptLoaderEx<npc_stormwind_soldierAI>("npc_stormwind_soldier");
    new CreatureScriptLoaderEx<npc_orgrimmar_gruntAI>("npc_orgrimmar_grunt");
    new CreatureScriptLoaderEx<npc_ironforge_paladinAI>("npc_ironforge_paladin");
    new CreatureScriptLoaderEx<npc_darnassian_archerAI>("npc_darnassian_archer");
    new CreatureScriptLoaderEx<npc_darkspear_axe_throwerAI>("npc_darkspear_axe_thrower");
    new CreatureScriptLoaderEx<npc_orgrimmar_shamanAI>("npc_orgrimmar_shaman");
    new CreatureScriptLoaderEx<npc_infernal_siegebreakerAI>("npc_infernal_siegebreaker");
}
