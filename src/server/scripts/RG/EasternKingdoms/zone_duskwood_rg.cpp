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

enum ElizaMisc
{
    // Creatures
    NPC_ELIZA                       = 314,
    
    // Quests
    QUEST_BRIDE_OF_THE_EMBALME      = 253,
    QUEST_DIGGING_THROUGH_THE_DIRT  = 254
};


class go_eliza_s_grave_dirt : public GameObjectScript
{
public:
    go_eliza_s_grave_dirt() : GameObjectScript("go_eliza_s_grave_dirt") { }

    bool OnQuestReward(Player* player, GameObject* go, Quest const* quest, uint32 /*opt*/) 
    {
        //if quest is complete, go will never spawn eliza
        if (player->GetQuestStatus(QUEST_BRIDE_OF_THE_EMBALME) == QUEST_STATUS_COMPLETE)
            return false;

        //if a eliza was spawned (dead or alive), go will not spawn a new eliza. Despawntime after death is 60s
        if (quest->GetQuestId() == QUEST_DIGGING_THROUGH_THE_DIRT)
        {
            std::list<Creature*> elizeList;
            go->GetCreatureListWithEntryInGrid(elizeList, NPC_ELIZA, 40.0f);
            if (elizeList.empty())
                go->SummonCreature(NPC_ELIZA, -10267.0f, 52.52f, 42.54f, 2.5f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60 * IN_MILLISECONDS);
        }

        return true; 
    }

};

enum TwilightCorrupterRGMisc 
{
    // Items
    ITEM_FRAGMENT                  = 21149,
    
    // Creatures
    NPC_TWILIGHT_CORRUPTER         = 15625,
    
    // Texts
    YELL_TWILIGHTCORRUPTOR_RESPAWN = 0,
};

/*######
# at_twilight_grove_rg
######*/

class at_twilight_grove_rg : public AreaTriggerScript
{
    public:
        at_twilight_grove_rg() : AreaTriggerScript("at_twilight_grove_rg") { }

        bool OnTrigger(Player* player, const AreaTriggerEntry* /*at*/)
        {
            if (player->HasQuestForItem(ITEM_FRAGMENT))
            {
                if (!player->FindNearestCreature(NPC_TWILIGHT_CORRUPTER, 50000.0f))
                {
                    if (Creature* corrupter = player->SummonCreature(NPC_TWILIGHT_CORRUPTER, -10328.16f, -489.57f, 50.0f, 0, TEMPSUMMON_MANUAL_DESPAWN, 600 * IN_MILLISECONDS))
                    {
                        corrupter->setFaction(FACTION_HOSTILE);
                        corrupter->SetMaxHealth(832750);
                    }
                    
                    if (Creature* CorrupterSpeaker = player->SummonCreature(NPC_TWILIGHT_CORRUPTER, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ() - 50, 0, TEMPSUMMON_MANUAL_DESPAWN, 10))
                    {
                        CorrupterSpeaker->AI()->Talk(YELL_TWILIGHTCORRUPTOR_RESPAWN, player);
                        CorrupterSpeaker->DespawnOrUnsummon();
                    }
                }
            }
            return false;
        };
};

void AddSC_duskwood_rg()
{
    new go_eliza_s_grave_dirt();
    new at_twilight_grove_rg();
}
