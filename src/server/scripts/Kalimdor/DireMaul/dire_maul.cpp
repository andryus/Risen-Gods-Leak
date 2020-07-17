/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2010-2014 Rising Gods <http://www.rising-gods.de/>
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
#include "dire_maul.h"

enum TendrisWarpwoodSpells
{
    SPELL_TRAMPLE        = 5568,
    SPELL_UPPERCUT       = 22916,
    SPELL_GRASPING_VINES = 22924,
    SPELL_ENTANGLE       = 22994
};

enum TendrisWarpwoodEvents
{
    EVENT_TRAMPLE        = 0,
    EVENT_UPPERCUT       = 1,
    EVENT_GRASPING_VINES = 2,
    EVENT_ENTANGLE       = 3
};

Position const HorseSummonPos =
{
    -12.565980f, 475.023438f, -23.300787f, 5.804300f
};

class boss_tendris_warpwood : public CreatureScript
{
public:
    boss_tendris_warpwood() : CreatureScript("boss_tendris_warpwood") { }
  
    struct boss_tendris_warpwoodAI : public ScriptedAI
    {
        boss_tendris_warpwoodAI(Creature* creature) : ScriptedAI(creature) 
        {
            instance = creature->GetInstanceScript();
        }
        
        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_TRAMPLE,         urand(5, 9) * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_UPPERCUT,       urand(9, 12) * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_GRASPING_VINES,  urand(2, 4) * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_ENTANGLE,        urand(3, 4) * IN_MILLISECONDS);
        }

        void EnterCombat(Unit* /*who*/) override
        {
            std::list<Creature*> WarpwoodList;
            me->GetCreatureListWithEntryInGrid(WarpwoodList, NPC_WARPWOOD_STOMPER,   300.0f);
            me->GetCreatureListWithEntryInGrid(WarpwoodList, NPC_PETRIFIED_TREANT,   300.0f);
            me->GetCreatureListWithEntryInGrid(WarpwoodList, NPC_PETRIFIED_GUARDIEN, 300.0f);
            me->GetCreatureListWithEntryInGrid(WarpwoodList, NPC_WARPWOOD_TREANT,    300.0f);
            me->GetCreatureListWithEntryInGrid(WarpwoodList, NPC_WARPWOOD_TANGLER,   300.0f);
            me->GetCreatureListWithEntryInGrid(WarpwoodList, NPC_WARPWOOD_CRUSHER,   300.0f);
            me->GetCreatureListWithEntryInGrid(WarpwoodList, NPC_WARPWOOD_GUARD,     300.0f);
            me->GetCreatureListWithEntryInGrid(WarpwoodList, NPC_STONED_GUARD,       300.0f);
            if (!WarpwoodList.empty())
                for (std::list<Creature*>::iterator itr = WarpwoodList.begin(); itr != WarpwoodList.end(); itr++)
                    (*itr)->AI()->AttackStart(me->GetVictim());
        }

        void JustDied(Unit* /*killer*/) override
        {
            if (instance)
                instance->SetBossState(DATA_TENDRIS_WARPWOOD, DONE);

            if (GameObject* door = me->FindNearestGameObject(GO_WARPWOOD_DOOR, 1000.0f))
                door->UseDoorOrButton();

            me->SummonCreature(NPC_HORSE, HorseSummonPos);
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
                    case EVENT_TRAMPLE:
                        DoCastSelf(SPELL_TRAMPLE, true);
                        _events.ScheduleEvent(EVENT_TRAMPLE, urand(9, 14) * IN_MILLISECONDS);
                        break;
                    case EVENT_UPPERCUT:
                        DoCastVictim(SPELL_UPPERCUT, true);
                        _events.ScheduleEvent(EVENT_UPPERCUT, urand(12, 15) * IN_MILLISECONDS);
                        break;
                    case EVENT_GRASPING_VINES:
                        DoCastSelf(SPELL_GRASPING_VINES, true);
                        _events.ScheduleEvent(EVENT_GRASPING_VINES, urand(17, 22) * IN_MILLISECONDS);
                        break;
                    case EVENT_ENTANGLE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 100, true))
                            DoCast(target, SPELL_ENTANGLE, true);
                        _events.ScheduleEvent(EVENT_ENTANGLE, urand(6, 7) * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    private:
        EventMap _events;
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_tendris_warpwoodAI(creature);
    }
};

enum BasketMisc
{
    TALK_AGGRO           = 0,
    WP_GORDOK_BUSHWACKER = 1435100
};

class go_ogre_tannin_basket : public GameObjectScript
{
    public:
        go_ogre_tannin_basket() : GameObjectScript("go_ogre_tannin_basket") { }
        
        bool OnGossipHello(Player* player, GameObject* go)
        {
            player->SendLoot(go->GetGUID(), LOOT_CORPSE);
            if (!(go->FindNearestCreature(NPC_GORDOK_BUSHWACKER, 30.0f)))
            {
                if (Creature* gordok = go->SummonCreature(NPC_GORDOK_BUSHWACKER, 561.7445f, 535.128f, 16.9456f, 0.004813f, TEMPSUMMON_TIMED_DESPAWN, 120000))
                { 
                    gordok->AI()->Talk(TALK_AGGRO);
                    gordok->GetMotionMaster()->MovePath(WP_GORDOK_BUSHWACKER, false);
                }
            }
            return true;
        }
};

void AddSC_dire_maul()
{
    new boss_tendris_warpwood();
    new go_ogre_tannin_basket();
}
