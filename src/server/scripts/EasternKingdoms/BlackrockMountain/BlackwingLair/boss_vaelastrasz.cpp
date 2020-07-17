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
SDName: Boss_Vaelastrasz
SD%Complete: 75
SDComment: Burning Adrenaline not correctly implemented in core
SDCategory: Blackwing Lair
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackwing_lair.h"
#include "ScriptedGossip.h"
#include "Player.h"

enum Says
{
   SAY_LINE1           = 0,
   SAY_LINE2           = 1,
   SAY_LINE3           = 2,
   SAY_HALFLIFE        = 3,
   SAY_KILLTARGET      = 4
};

#define GOSSIP_ITEM         "Start Event <Needs Gossip Text>"

enum Spells
{
   SPELL_ESSENCEOFTHERED       = 23513,
   SPELL_FLAMEBREATH           = 23461,
   SPELL_FIRENOVA              = 23462,
   SPELL_TAILSWIPE             = 15847,
   SPELL_BURNINGADRENALINE     = 23620,
   SPELL_CLEAVE                = 20684   //Chain cleave is most likely named something different and contains a dummy effect
};

enum Events
{
    EVENT_ESSENCEOFTHERED = 1,
    EVENT_FLAMEBREATH,
    EVENT_FIRENOVA,
    EVENT_TAILSWIPE,
    EVENT_CLEAVE,
    EVENT_BURNINGADRENALINE_CASTER,
    EVENT_BURNINGADRENALINE_TANK,
};

class boss_vaelastrasz : public CreatureScript
{
public:
    boss_vaelastrasz() : CreatureScript("boss_vaelastrasz") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (InstanceScript* instance = player->GetInstanceScript())
            if (instance->GetBossState(BOSS_RAZORGORE) != DONE)
                return false;

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        player->SEND_GOSSIP_MENU(907, creature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        if (action == GOSSIP_ACTION_INFO_DEF)
        {
            CAST_AI(boss_vaelastrasz::boss_vaelAI, creature->AI())->BeginSpeech(player);
            player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();
        }
        return true;
    }

    struct boss_vaelAI : public BossAI
    {
        boss_vaelAI(Creature* creature) : BossAI(creature, BOSS_VAELASTRAZ)
        {
            creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            creature->setFaction(35);
            creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

            PlayerGUID.Clear();
        void Reset() override
        {
            _Reset();

            me->SetStandState(UNIT_STAND_STATE_DEAD);
            PlayerGUID = 0;
            Step = 0;

            HasYelled = false;
            DoingSpeech = false;
        }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();

            DoCast(me, SPELL_ESSENCEOFTHERED);
            me->SetHealth(me->CountPctFromMaxHealth(30));
            // now drop damage requirement to be able to take loot
            me->ResetPlayerDamageReq();

            events.ScheduleEvent(EVENT_CLEAVE, 15000);
            events.ScheduleEvent(EVENT_FLAMEBREATH, 15000);
            events.ScheduleEvent(EVENT_FIRENOVA, 15000);
            events.ScheduleEvent(EVENT_TAILSWIPE, 15000);
            events.ScheduleEvent(EVENT_BURNINGADRENALINE_CASTER, 15000);
            events.ScheduleEvent(EVENT_BURNINGADRENALINE_TANK, 45000);
        }

        void BeginSpeech(Unit* target)
        {
            PlayerGUID = target->GetGUID();

            DoingSpeech = true;
            PhaseTimer = 1000;

            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        }

        void JumpToNextStep(uint32 Timer)
        {
            PhaseTimer = Timer;
            ++Step;
        }

        void KilledUnit(Unit* victim) override
        {
            if (rand32() % 5)
                return;

            Talk(SAY_KILLTARGET, victim);
        }

        void UpdateAI(uint32 diff) override
        {
            //Speech
            if (DoingSpeech)
            {
                if (PhaseTimer <= diff)
                {
                    switch (Step)
                    {
                        case 0:
                            Talk(SAY_LINE1);
                            me->SetStandState(UNIT_STAND_STATE_STAND);
                            me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                            JumpToNextStep(12000);
                            break;
                        case 1:
                            Talk(SAY_LINE2);
                            me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                            JumpToNextStep(12000);
                            break;
                        case 2:
                            Talk(SAY_LINE3);
                            me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                            JumpToNextStep(16000);
                            break;
                        case 3:
                            me->setFaction(103);
                            if (PlayerGUID && ObjectAccessor::GetUnit(*me, PlayerGUID))
                                AttackStart(ObjectAccessor::GetUnit(*me, PlayerGUID));
                            DoingSpeech = false;
                            break;
                    }
                }
                else
                    PhaseTimer -= diff;
            }

            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_CLEAVE:
                        events.ScheduleEvent(EVENT_CLEAVE, 15000);
                        DoCastVictim(SPELL_CLEAVE);
                        break;
                    case EVENT_FLAMEBREATH:
                        DoCastVictim(SPELL_FLAMEBREATH);
                        events.ScheduleEvent(EVENT_FLAMEBREATH, urand(8000, 14000));
                        break;
                    case EVENT_FIRENOVA:
                        DoCastVictim(SPELL_FIRENOVA);
                        events.ScheduleEvent(EVENT_FIRENOVA, 15000);
                        break;
                    case EVENT_TAILSWIPE:
                        //Only cast if we are behind
                        /*if (!me->HasInArc(M_PI, me->GetVictim()))
                        {
                        DoCast(me->GetVictim(), SPELL_TAILSWIPE);
                        }*/
                        events.ScheduleEvent(EVENT_TAILSWIPE, 15000);
                        break;
                    case EVENT_BURNINGADRENALINE_CASTER:
                        {
                            Unit* target = NULL;

                            uint8 i = 0;
                            while (i < 3)   // max 3 tries to get a random target with power_mana
                            {
                                ++i;
                                target = SelectTarget(SELECT_TARGET_RANDOM, 1, 100, true); // not aggro leader
                                if (target && target->getPowerType() == POWER_MANA)
                                    i = 3;
                            }
                            if (target)                                     // cast on self (see below)
                                target->CastSpell(target, SPELL_BURNINGADRENALINE, true);
                        }
                        events.ScheduleEvent(EVENT_BURNINGADRENALINE_CASTER, 15000);
                        break;
                    case EVENT_BURNINGADRENALINE_TANK:
                        // have the victim cast the spell on himself otherwise the third effect aura will be applied to Vael instead of the player
                        me->EnsureVictim()->CastSpell(me->GetVictim(), SPELL_BURNINGADRENALINE, true);
                        events.ScheduleEvent(EVENT_BURNINGADRENALINE_TANK, 45000);
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }

            // Yell if hp lower than 15%
            if (HealthBelowPct(15) && !HasYelled)
            {
                Talk(SAY_HALFLIFE);
                HasYelled = true;
            }

            DoMeleeAttackIfReady();
        }

    private:
        uint8 Step;
        uint32 PhaseTimer;
        ObjectGuid PlayerGUID;

        bool HasYelled;
        bool DoingSpeech;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_vaelAI(creature);
    }
};

void AddSC_boss_vaelastrasz()
{
    new boss_vaelastrasz();
}
