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
#include "ScriptedGossip.h"

/*####
 ## npc_deathgate
 ####*/

enum DeathgateMisc
{
    GOSSIP_MENU_OPTION_ID   = 60222,
    // Quests
    QUEST_COUNTERATTACK     = 4021,
    
    // Events
    EVENT_START             = 1,
    EVENT_FIRST_WAVE        = 2,
    EVENT_SECOND_WAVE       = 3,
    EVENT_LAST_WAVE         = 4,
    EVENT_RESET_TIMER       = 5,
    
    // Texts
    SAY_START_ONE           = 0,
    
    // Friendly
    NPC_DEFENDER            = 9457,
    NPC_AXE_THROWER         = 9458,
    
    // Hostile
    NPC_INVADER             = 9524,
    NPC_STORMSEER           = 9523,
    NPC_WARLORD             = 9456,
    
    // Actions
    ACTION_START_BARRENS    = 0,
};

struct SpawnPos
{
    uint32   entry;
    Position pos;
};

static const SpawnPos counterattackFirstWave[]
{
    { NPC_DEFENDER,     { -280.703f, -1908.01f, 91.6668f, 1.77351f  } },
    { NPC_AXE_THROWER,  { -286.384f, -1910.99f, 91.6668f, 1.59444f  } },
    { NPC_DEFENDER,     { -297.373f, -1917.11f, 91.6746f, 1.81435f  } },
    { NPC_AXE_THROWER,  { -293.212f, -1912.51f, 91.6673f, 1.42794f  } },
    { NPC_INVADER,      { -280.037f, -1888.35f, 92.2549f, 2.28087f  } },
    { NPC_INVADER,      { -292.107f, -1899.54f, 91.667f,  4.78158f  } },
    { NPC_INVADER,      { -305.57f,  -1869.88f, 92.7754f, 2.45131f  } },
    { NPC_INVADER,      { -289.972f, -1882.76f, 92.5714f, 3.43148f  } },
    { NPC_INVADER,      { -277.454f, -1873.39f, 92.7773f, 4.75724f  } },
    { NPC_INVADER,      { -271.581f, -1873.39f, 92.7773f, 4.75724f  } },
    { NPC_INVADER,      { -269.982f, -1828.6f,  92.4754f, 4.68655f  } },
    { NPC_INVADER,      { -269.982f, -1828.6f,  92.4754f, 4.68655f  } },
    { NPC_STORMSEER,    { -279.267f, -1827.92f, 92.3128f, 1.35332f  } },
    { NPC_STORMSEER,    { -297.42f,  -1847.41f, 93.2295f, 5.80967f  } },
    { NPC_STORMSEER,    { -310.607f, -1831.89f, 95.9363f, 0.371571f } },
    { NPC_STORMSEER,    { -329.177f, -1842.43f, 95.3891f, 0.516085f } },
    { NPC_STORMSEER,    { -324.448f, -1860.63f, 94.3221f, 4.97793f  } },
    { NPC_STORMSEER,    { -324.448f, -1860.63f, 94.3221f, 4.97793f  } }
};

static const SpawnPos counterattackSecondWave[]
{
    { NPC_INVADER,      { -290.588f, -1858.11f, 92.5026f, 4.14698f  } },
    { NPC_INVADER,      { -304.978f, -1844.7f,  94.4432f, 1.61721f  } },
    { NPC_INVADER,      { -297.089f, -1867.68f, 92.5601f, 2.21804f  } },
    { NPC_STORMSEER,    { -286.103f, -1846.18f, 92.544f,  6.11047f  } },
    { NPC_STORMSEER,    { -308.105f, -1859.08f, 93.8039f, 2.80709f  } },
    { NPC_STORMSEER,    { -286.988f, -1876.47f, 92.7447f, 1.39494f  } }
};

static const SpawnPos counterattackLastWave[]
{
    { NPC_INVADER,      { -291.86f,  -1893.04f, 92.0213f, 1.96121f  } },
    { NPC_INVADER,      { -298.297f, -1846.85f, 93.3672f, 4.97792f  } },
    { NPC_INVADER,      { -294.942f, -1845.88f, 93.0999f, 4.86797f  } },
    { NPC_INVADER,      { -294.942f, -1845.88f, 93.0999f, 4.86797f  } },
    { NPC_WARLORD,      { -296.718f, -1846.38f, 93.2334f, 5.02897f  } }
};

class npc_deathgate : public CreatureScript
{
    public:
        npc_deathgate() : CreatureScript("npc_deathgate") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
        {
            if (quest->GetQuestId() == QUEST_COUNTERATTACK)
            {
                creature->AI()->Talk(SAY_START_ONE, player);
                creature->AI()->DoAction(ACTION_START_BARRENS);
            }

            return true;
        }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            if (creature->IsQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());

            if (ENSURE_AI(npc_deathgate::npc_deathgateAI, creature->AI())->_phase == 0)
                if (player->GetQuestStatus(QUEST_COUNTERATTACK) == QUEST_STATUS_INCOMPLETE)
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_MENU_OPTION_ID, 0), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 0);

            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
        {
            switch (uiAction)
            {
                case GOSSIP_ACTION_INFO_DEF + 0:
                    player->CLOSE_GOSSIP_MENU();
                    creature->AI()->Talk(SAY_START_ONE, player);
                    creature->AI()->DoAction(ACTION_START_BARRENS);
                    break;
            }
            return true;
        }
    
    struct npc_deathgateAI : public ScriptedAI
    {
        npc_deathgateAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _phase   = 0;
            _counter = 0;
            summons.clear();
            _events.Reset();
        }

        void DoAction(int32 action) override
        {
            if (action == ACTION_START_BARRENS)
            {
                if (_phase == 0)
                {
                    summons.clear();
                    _events.ScheduleEvent(EVENT_START,       0);
                    _events.ScheduleEvent(EVENT_RESET_TIMER, 5 * MINUTE * SECOND * IN_MILLISECONDS);
                }
            }
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
        {
            if (summon)
            {
                std::set<ObjectGuid>::iterator itr = summons.find(summon->GetGUID());
                if (itr != summons.end())
                    summons.erase(itr);

                if (summon->GetEntry() == NPC_DEFENDER)
                {
                    if (Creature* add = me->SummonCreature(NPC_DEFENDER, summon->GetPositionX(), summon->GetPositionY(), summon->GetPositionZ(), summon->GetOrientation(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300 * IN_MILLISECONDS))
                        summons.insert(add->GetGUID());
                }
                else if (summon->GetEntry() == NPC_AXE_THROWER)
                {
                    if (Creature* add = me->SummonCreature(NPC_AXE_THROWER, summon->GetPositionX(), summon->GetPositionY(), summon->GetPositionZ(), summon->GetOrientation(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300 * IN_MILLISECONDS))
                        summons.insert(add->GetGUID());
                }

                if (_phase != 0 && _counter > 0)
                {
                    if (summon->GetEntry() == NPC_INVADER || summon->GetEntry() == NPC_STORMSEER || summon->GetEntry() == NPC_WARLORD)
                        _counter--;
                }
                else
                {
                    if (_phase == 1)
                    {
                        _phase = 2;
                        _counter = 5;
                        _events.ScheduleEvent(EVENT_SECOND_WAVE, 10 * IN_MILLISECONDS);
                    }
                    else if (_phase == 2)
                    {
                        _phase = 3;
                        _counter = 5;
                        _events.ScheduleEvent(EVENT_LAST_WAVE, 10 * IN_MILLISECONDS);
                    }
                    else if (_phase == 3)
                    {
                        CleanUp();
                    }
                }
            }
        }

        void CleanUp()
        {
            _phase = 0;

            for (std::set<ObjectGuid>::iterator itr = summons.begin(); itr != summons.end(); ++itr)
                if (Creature* add = ObjectAccessor::GetCreature(*me, *itr))
                    if (add->IsAlive())
                        add->DespawnOrUnsummon(30 * IN_MILLISECONDS);

            summons.clear();
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);
            
            switch (_events.ExecuteEvent())
            {
                case EVENT_RESET_TIMER:
                    CleanUp();
                    break;
                case EVENT_START:
                    _phase = 1;
                    _counter = 5;
                    _events.ScheduleEvent(EVENT_FIRST_WAVE, 0);
                    break;
                case EVENT_FIRST_WAVE:
                    for (const auto& var : counterattackFirstWave)
                        if (Creature* add = me->SummonCreature(var.entry, var.pos, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300 * IN_MILLISECONDS))
                        {
                            if (add->GetEntry() == NPC_INVADER || add->GetEntry() == NPC_STORMSEER || add->GetEntry() == NPC_WARLORD)
                                add->GetMotionMaster()->MoveRandom(10.0f);

                            summons.insert(add->GetGUID());
                        }  
                    break;                    
                case EVENT_SECOND_WAVE:
                    for (const auto& var : counterattackSecondWave)
                        if (Creature* add = me->SummonCreature(var.entry, var.pos, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300 * IN_MILLISECONDS))
                        {
                            if (add->GetEntry() == NPC_INVADER || add->GetEntry() == NPC_STORMSEER || add->GetEntry() == NPC_WARLORD)
                                add->GetMotionMaster()->MoveRandom(10.0f);

                            summons.insert(add->GetGUID());
                        }
                    break;
                case EVENT_LAST_WAVE:
                    for (const auto& var : counterattackLastWave)
                        if (Creature* add = me->SummonCreature(var.entry, var.pos, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300 * IN_MILLISECONDS))
                        {
                            if (add->GetEntry() == NPC_INVADER || add->GetEntry() == NPC_STORMSEER || add->GetEntry() == NPC_WARLORD)
                                add->GetMotionMaster()->MoveRandom(10.0f);

                            summons.insert(add->GetGUID());
                        }
                    break;
                default:
                    break;
            }

            if (UpdateVictim())
                DoMeleeAttackIfReady();
        }
    public:
        uint8    _phase;
    private:
        EventMap _events;
        uint8    _counter;
        std::set<ObjectGuid> summons;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_deathgateAI(creature);
    }
};

void AddSC_the_barrens_rg()
{
    new npc_deathgate();
}
