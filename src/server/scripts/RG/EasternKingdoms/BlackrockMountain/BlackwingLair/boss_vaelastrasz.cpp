/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
#include "blackwing_lair.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

enum Says
{
   SAY_LINE1           = 0,
   SAY_LINE2           = 1,
   SAY_LINE3           = 2,
   SAY_HALFLIFE        = 3,
   SAY_KILLTARGET      = 4
};

enum Gossip
{
    GOSSIP_START = 6021,
    GOSSIP_STEP = 6101,
};

enum Spells
{
   SPELL_ESSENCEOFTHERED          = 23513,
   SPELL_FLAMEBREATH              = 23461,
   SPELL_FIRENOVA                 = 23462,
   SPELL_TAILSWIPE                = 15847,
   SPELL_BURNINGADRENALINE_TANK   = 18173,
   SPELL_BURNINGADRENALINE_CASTER = 23620,
   SPELL_BURNINGADRENALINE_DAMAGE = 23478,
   SPELL_CLEAVE                   = 19983
};

enum Events
{
    EVENT_ESSENCEOFTHERED = 1,
    EVENT_FLAMEBREATH,
    EVENT_FIRENOVA,
    EVENT_TAILSWIPE,
    EVENT_CLEAVE,
    EVENT_BURNINGADRENALINE,
};

enum VaelastraszFactions
{
    FACTION_START_FIGHT = 103
};

struct BurningAdrenalinTargetSelector
{
    BurningAdrenalinTargetSelector() { }

    bool operator()(WorldObject* object) const
    {
        if (Player* player = object->ToPlayer())
            return player->getPowerType() == POWER_MANA && !player->HasAura(SPELL_BURNINGADRENALINE_CASTER) &&
            !player->HasAura(SPELL_BURNINGADRENALINE_TANK);
        return false;
    }
};

class boss_vaelastrasz : public CreatureScript
{
    public:
        boss_vaelastrasz() : CreatureScript("boss_vaelastrasz") { }
    
        bool OnGossipHello(Player* player, Creature* creature) override
        {
            if (InstanceScript* instance = player->GetInstanceScript())
                if (!instance->CheckRequiredBosses(BOSS_VAELASTRAZ, player))
                    return false;
    
            player->PrepareGossipMenu(creature, GOSSIP_START, true);
            player->SendPreparedGossip(creature);
    
            return true;
        }
    
        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
        {
            if (player->PlayerTalkClass->GetGossipMenu().GetMenuId() == GOSSIP_START)
            {
                player->PrepareGossipMenu(creature, GOSSIP_STEP);
                player->SendPreparedGossip(creature);
            }
            else
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
                creature->setFaction(35);
            }
    
            void Reset() override
            {
                BossAI::Reset();
    
                me->SetStandState(UNIT_STAND_STATE_DEAD);
                PlayerGUID.Clear();
                Step = 0;
                burningAdrenalinCount = 0;
    
                HasYelled = false;
                DoingSpeech = false;
            }
    
            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
    
                DoCastSelf(SPELL_ESSENCEOFTHERED);
                me->SetHealth(me->CountPctFromMaxHealth(30));
                // now drop damage requirement to be able to take loot
                me->ResetPlayerDamageReq();
    
                events.ScheduleEvent(EVENT_CLEAVE,                   5  * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FLAMEBREATH,              8  * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FIRENOVA,                 3  * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_TAILSWIPE,                15 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_BURNINGADRENALINE,        15 * IN_MILLISECONDS);
                me->SetStandState(UNIT_STAND_STATE_STAND);
            }
    
            void BeginSpeech(Unit* target)
            {
                PlayerGUID = target->GetGUID();
    
                DoingSpeech = true;
                PhaseTimer = 1000;
    
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                me->SetFacingTo(2.164f);
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
                // Speech
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
                                JumpToNextStep(12 * IN_MILLISECONDS);
                                break;
                            case 1:
                                Talk(SAY_LINE2);
                                me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                                JumpToNextStep(12 * IN_MILLISECONDS);
                                break;
                            case 2:
                                Talk(SAY_LINE3);
                                me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                                JumpToNextStep(16 * IN_MILLISECONDS);
                                break;
                            case 3:
                                me->setFaction(FACTION_START_FIGHT);
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
                            events.ScheduleEvent(EVENT_CLEAVE, 10 * IN_MILLISECONDS);
                            DoCastVictim(SPELL_CLEAVE);
                            break;
                        case EVENT_FLAMEBREATH:
                            DoCastVictim(SPELL_FLAMEBREATH);
                            events.ScheduleEvent(EVENT_FLAMEBREATH, urand(8, 10) * IN_MILLISECONDS);
                            break;
                        case EVENT_FIRENOVA:
                            DoCastVictim(SPELL_FIRENOVA);
                            events.ScheduleEvent(EVENT_FIRENOVA, 3 * IN_MILLISECONDS);
                            break;
                        case EVENT_TAILSWIPE:
                            DoCastSelf(SPELL_TAILSWIPE);
                            events.ScheduleEvent(EVENT_TAILSWIPE, 10 * IN_MILLISECONDS);
                            break;
                        case EVENT_BURNINGADRENALINE:
                            if (burningAdrenalinCount == 2)
                            {
                                DoCastVictim(SPELL_BURNINGADRENALINE_TANK);
                                burningAdrenalinCount = 0;
                            }
                            else
                            {
                                Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, BurningAdrenalinTargetSelector());
                                if (target)
                                    DoCast(target, SPELL_BURNINGADRENALINE_CASTER);
                                else
                                {
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 250.0f, true))
                                        DoCast(target, SPELL_BURNINGADRENALINE_CASTER);
                                }
                                ++burningAdrenalinCount;
                            }
                            events.ScheduleEvent(EVENT_BURNINGADRENALINE, 15 * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }

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
            uint8 burningAdrenalinCount;
    
            bool HasYelled;
            bool DoingSpeech;
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_vaelAI(creature);
        }
};

class spell_vaelastrasz_burning_adrenalin : public SpellScriptLoader
{
public:
    spell_vaelastrasz_burning_adrenalin() : SpellScriptLoader("spell_vaelastrasz_burning_adrenalin") { }

    class spell_vaelastrasz_burning_adrenalin_AuraScripts : public AuraScript
    {
        PrepareAuraScript(spell_vaelastrasz_burning_adrenalin_AuraScripts);

        void HandleOnEffectRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            GetTarget()->CastSpell(GetTarget(), SPELL_BURNINGADRENALINE_DAMAGE, true, nullptr, aurEff, GetTarget()->GetGUID());
        }

        void Register() override
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_vaelastrasz_burning_adrenalin_AuraScripts::HandleOnEffectRemove, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_vaelastrasz_burning_adrenalin_AuraScripts();
    }
};

class at_bwl_vaelastrasz_event : public AreaTriggerScript
{
public:
    at_bwl_vaelastrasz_event() : AreaTriggerScript("at_bwl_vaelastrasz_event") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/)
    {
        if (player->IsGameMaster())
            return true;

        if (InstanceScript* instance = player->GetInstanceScript())
        {
            if (instance->GetData(DATA_VAELASTRAZ_PRE_EVENT) == NOT_STARTED)
                instance->SetData(DATA_VAELASTRAZ_PRE_EVENT, DONE);
        }
        return true;
    }
};

void AddSC_boss_vaelastrasz()
{
    new boss_vaelastrasz();
    new spell_vaelastrasz_burning_adrenalin();
    new at_bwl_vaelastrasz_event();
}
