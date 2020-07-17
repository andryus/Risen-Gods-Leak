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

/*######
## npc_oox09hl_rg
######*/

enum OOXMisc
{
    // Texts
    SAY_OOX_START          = 0,
    SAY_OOX_AGGRO          = 1,
    SAY_OOX_AMBUSH         = 2,
    SAY_OOX_AMBUSH_REPLY   = 3,
    SAY_OOX_END            = 4,

    // Quests
    QUEST_RESQUE_OOX_09    = 836,

    // Creatures
    NPC_MARAUDING_OWL      = 7808,
    NPC_VILE_AMBUSHER      = 7809,

    // Factions
    FACTION_ESCORTEE_A     = 774,
    FACTION_ESCORTEE_H     = 775,

    // Spells
    SPELL_OOX_LIFT_OFF     = 68499
};

class npc_oox09hl_rg : public CreatureScript
{
    public:
        npc_oox09hl_rg() : CreatureScript("npc_oox09hl_rg") { }

        struct npc_oox09hl_rgAI : public npc_escortAI
        {
            npc_oox09hl_rgAI(Creature* creature) : npc_escortAI(creature) { }

            void Reset() override
            {
                if (!HasEscortState(STATE_ESCORT_ESCORTING))
                    SummonCount = 0;
            }

            void EnterCombat(Unit* who) override
            {
                if (who->GetEntry() == NPC_MARAUDING_OWL)
                    return;

                Talk(SAY_OOX_AGGRO);
            }

            void JustSummoned(Creature* summoned) override
            {
                summoned->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
            }

            void sQuestAccept(Player* player, Quest const* quest)
            {
                if (quest->GetQuestId() == QUEST_RESQUE_OOX_09)
                {
                    me->SetStandState(UNIT_STAND_STATE_STAND);
                    me->setFaction(player->GetTeam() == ALLIANCE ? FACTION_ESCORTEE_A : FACTION_ESCORTEE_H);
                    Talk(SAY_OOX_START, player);
                    npc_escortAI::Start(false, false, player->GetGUID(), quest);
                }
            }

            void WaypointReached(uint32 waypointId) override
            {
                switch (waypointId) 
                {
                    case 30:
                        Talk(SAY_OOX_END);
                        DoCastSelf(SPELL_OOX_LIFT_OFF, true);
                        if (Player* player = GetPlayerForEscort())
                            player->GroupEventHappens(QUEST_RESQUE_OOX_09, me);
                        break;
                    default:
                        break;
                }
            }

            void WaypointStart(uint32 pointId) override
            {
                switch (pointId)
                {
                    case 27:
                        if (SummonCount >= 3)
                            break;

                        for (uint8 i = 0; i < 3; ++i)
                        {
                            const Position src = { 147.927444f, -3851.513428f, 130.893f, 0 };
                            Position dst;
                            me->GetRandomPoint(src, 7.0f, dst);
                            me->SummonCreature(NPC_MARAUDING_OWL, dst, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 25000);
                            ++SummonCount;
                        }
                        break;
                    default:
                        break;
                }
            }
            
        private:
            uint8 SummonCount;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_oox09hl_rgAI(creature);
        }
};

void AddSC_hinterlands_rg()
{
    new npc_oox09hl_rg();
}
