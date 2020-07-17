/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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
#include "ScriptedEscortAI.h"
#include "SpellScript.h"

/*######
## npc_OOX17_rg
######*/

enum Npc00X17Misc
{
    // Texts
    SAY_OOX_START = 0,
    SAY_OOX_AGGRO = 1,
    SAY_OOX_END   = 3,

    // Quests
    Q_OOX17       = 648,

    // Spells
    SPELL_OOX_OFF = 68499
};

class npc_OOX17_rg : public CreatureScript
{
    public:
        npc_OOX17_rg() : CreatureScript("npc_OOX17_rg") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
        {
            if (quest->GetQuestId() == Q_OOX17)
            {
                creature->setFaction(FACTION_ESCORT_N_NEUTRAL_PASSIVE);
                creature->SetFullHealth();
                creature->SetUInt32Value(UNIT_FIELD_BYTES_1, 0);
                creature->SetImmuneToPC(false);
                creature->AI()->Talk(SAY_OOX_START);

                if (npc_escortAI* pEscortAI = CAST_AI(npc_OOX17_rg::npc_OOX17_rgAI, creature->AI()))
                    pEscortAI->Start(true, false, player->GetGUID());
            }
            return true;
        }
        
        struct npc_OOX17_rgAI : public npc_escortAI
        {
            npc_OOX17_rgAI(Creature* creature) : npc_escortAI(creature) { }

            void WaypointReached(uint32 waypointId) override
            {
                if (Player* player = GetPlayerForEscort())
                {
                    switch (waypointId)
                    { 
                        case 24:
                            Talk(SAY_OOX_END);
                            DoCastSelf(SPELL_OOX_OFF, true);
                            player->GroupEventHappens(Q_OOX17, me);
                            break;
                        default:
                            break;
                    }
                }
            }

            void Reset() override { }

            void EnterCombat(Unit* /*who*/) override
            {
                Talk(SAY_OOX_AGGRO);
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_OOX17_rgAI(creature);
        }
};

enum CentiparrTunneler
{
    SPELL_PIERCE_ARMOR  = 6016,
    SPELL_TRASH         = 3391,

    EVENT_PIERCE_ARMOR  = 1,
    EVENT_TRASH         = 2,
};

class npc_centipaar_tunneler : public CreatureScript
{
public:
    npc_centipaar_tunneler() : CreatureScript("npc_centipaar_tunneler") { }

    struct npc_centipaar_tunnelerAI : public ScriptedAI
    {
        npc_centipaar_tunnelerAI(Creature* creature) : ScriptedAI(creature)  { }

        void InitializeAI() override
        {
            if (me->ToTempSummon())
            {
                if (Unit* target = me->ToTempSummon()->GetSummoner())
                    me->GetAI()->AttackStart(target);
            }
            else ScriptedAI::InitializeAI();

            Reset();
        }

        void Reset() override
        {
            if (me->ToTempSummon())
                me->GetMotionMaster()->MoveRandom(15.0f);

            _events.Reset();

            _events.ScheduleEvent(EVENT_PIERCE_ARMOR, urand(6000, 10000));
            _events.ScheduleEvent(EVENT_TRASH, urand(3500, 6000));
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
                    case EVENT_PIERCE_ARMOR:
                        DoCastVictim(SPELL_PIERCE_ARMOR);
                        _events.ScheduleEvent(EVENT_PIERCE_ARMOR, urand(18000, 25000));
                        break;
                    case EVENT_TRASH:
                        DoCastSelf(SPELL_TRASH);
                        _events.ScheduleEvent(EVENT_TRASH, urand(3500, 6000));
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

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_centipaar_tunnelerAI(creature);
    }
};

/*####
## spell_q4005_force_create_totem - 13910
####*/

enum Aquementas 
{
    SPELL_CREATE_ELEMENTAL_TOTEM    = 13909
};

class spell_q4005_force_create_totem_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_q4005_force_create_totem_AuraScript);

    void HandleOnEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/) 
    {
        if (Unit* target = GetTarget())
            if (target->GetTypeId() == TYPEID_PLAYER) 
            {
                target->CastSpell(target, SPELL_CREATE_ELEMENTAL_TOTEM, true);
            }
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_q4005_force_create_totem_AuraScript::HandleOnEffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }

};

void AddSC_tanaris_rg()
{
    new npc_OOX17_rg();
    new npc_centipaar_tunneler();
    new SpellScriptLoaderEx<spell_q4005_force_create_totem_AuraScript>("spell_q4005_force_create_totem");
}
