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
#include "Player.h"
#include "SmartAI.h"

enum PumpCreditmarkerMisc
{
    // Spells
    SPELL_IRONVINE_SEEDS    = 31736,

    // Creatures
    NPC_STEAM_PUMP_OVERSEER = 18340
};

class npc_steam_pump_credit_marker : public CreatureScript
{
    public:
        npc_steam_pump_credit_marker() : CreatureScript("npc_steam_pump_credit_marker") { }

        struct npc_steam_pump_credit_markerAI : public SmartAI
        {
            npc_steam_pump_credit_markerAI(Creature* creature) : SmartAI(creature) { }

            void SpellHit(Unit* unit, const SpellInfo* spellInfo)
            {
                if (spellInfo->Id == SPELL_IRONVINE_SEEDS)
                {
                    std::list<Creature*> creaturelist;
                    me->GetCreatureListWithEntryInGrid(creaturelist, NPC_STEAM_PUMP_OVERSEER, 10.0f);
                    if (creaturelist.size() <= 1)
                    {
                        if (Creature* overseer = me->SummonCreature(NPC_STEAM_PUMP_OVERSEER, me->GetPositionX() + 1.0f, me->GetPositionY() + 1.0f, me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 600000))
                            if (Player* player = overseer->FindNearestPlayer(50.0f))
                                overseer->AI()->AttackStart(player);
                    }
                }
                SmartAI::SpellHit(unit, spellInfo);
            }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_steam_pump_credit_markerAI(creature);
    }
};

void AddSC_zangarmarsh_rg()
{
    new npc_steam_pump_credit_marker();
}
