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
#include "AchievementMgr.h"
#include "DBCStores.h"

enum
{
    QUEST_TRIAL_OF_THE_NAARU_MAGTHERIDON    = 10888,
    QUEST_THE_CUDGEL_OF_KARDESH             = 10901,
    QUEST_THE_VIALS_OF_ETERNITY             = 10445,
    QUEST_A_DISTRACTION_FOR_AKAMA           = 10985,
    
    TITLE_CHAMPION_OF_THE_NAARU             = 53,
    TITLE_HAND_OF_ADAL                      = 64,
    ACHIEVEMENT_CHAMPION_OF_THE_NAARU       = 432,
    ACHIEVEMENT_HAND_OF_ADAL                = 431,
};

class npc_tbc_title_reward : public CreatureScript
{
public:
    npc_tbc_title_reward() : CreatureScript("npc_tbc_title_reward") { }

    bool OnQuestReward(Player* player, Creature* /*creature*/, Quest const* quest, uint32 /*opt*/) override
    { 
        if ((quest->GetQuestId() == QUEST_TRIAL_OF_THE_NAARU_MAGTHERIDON && player->IsQuestRewarded(QUEST_THE_CUDGEL_OF_KARDESH)) ||
            (quest->GetQuestId() == QUEST_THE_CUDGEL_OF_KARDESH && player->IsQuestRewarded(QUEST_TRIAL_OF_THE_NAARU_MAGTHERIDON)))
        {
            if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(TITLE_CHAMPION_OF_THE_NAARU))
                player->SetTitle(titleEntry, false);
            if (auto achievement = sAchievementStore.LookupEntry(ACHIEVEMENT_CHAMPION_OF_THE_NAARU))
                player->CompletedAchievement(achievement);
        }
        if ((quest->GetQuestId() == QUEST_THE_VIALS_OF_ETERNITY && player->IsQuestRewarded(QUEST_A_DISTRACTION_FOR_AKAMA)) ||
            (quest->GetQuestId() == QUEST_A_DISTRACTION_FOR_AKAMA && player->IsQuestRewarded(QUEST_THE_VIALS_OF_ETERNITY)))
        {
            if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(TITLE_HAND_OF_ADAL))
                player->SetTitle(titleEntry, false);
            if (auto achievement = sAchievementStore.LookupEntry(ACHIEVEMENT_HAND_OF_ADAL))
                player->CompletedAchievement(achievement);
        }
        return false; 
    }
};

#define EVENT_TURN_AROUND 0

class npc_image_daily_hero_nonhero : public CreatureScript
{
public:
    npc_image_daily_hero_nonhero() : CreatureScript("npc_image_daily_hero_nonhero") { }

    struct npc_image_daily_hero_nonheroAI : public ScriptedAI
    {
        npc_image_daily_hero_nonheroAI(Creature* creature) : ScriptedAI(creature) { }

        void InitializeAI() override
        {
            events.ScheduleEvent(EVENT_TURN_AROUND, 200);
            me->SetHover(true, true);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case EVENT_TURN_AROUND:
                    if (me->GetOrientation() < (2 * M_PI_F))
                        me->SetFacingTo(me->GetOrientation() + 0.01f); 
                    else
                        me->SetFacingTo(0.0f);
                    events.ScheduleEvent(EVENT_TURN_AROUND, 50);
                    break;
                default:
                    break;
            }
        }

    private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_image_daily_hero_nonheroAI(creature);
    }
};

void AddSC_shattrath_city_rg()
{
    new npc_tbc_title_reward();
    new npc_image_daily_hero_nonhero();
}
