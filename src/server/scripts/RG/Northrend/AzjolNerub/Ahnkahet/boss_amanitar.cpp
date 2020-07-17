/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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

/*
 * Comment:  Find correct mushrooms spell to make them visible - buffs of the mushrooms not ever applied to the users...
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ahnkahet.h"
#include "Player.h"
#include "Pet.h"

enum Spells
{
    SPELL_BASH                                    = 57094, // Victim
    SPELL_ENTANGLING_ROOTS                        = 57095, // Random Victim 100Y
    SPELL_MINI                                    = 57055, // Self
    SPELL_VENOM_BOLT_VOLLEY                       = 57088, // Random Victim 100Y
    SPELL_HEALTHY_MUSHROOM_POTENT_FUNGUS          = 56648, // Killer 3Y
    SPELL_POISONOUS_MUSHROOM_POISON_CLOUD         = 57061, // Self - Duration 8 Sec
    SPELL_POISONOUS_MUSHROOM_VISUAL_AREA          = 61566, // Self
    SPELL_POISONOUS_MUSHROOM_VISUAL_AURA          = 56741, // Self
    SPELL_PUTRID_MUSHROOM                         = 31690, // To make the mushrooms visible
};

enum Creatures
{
    NPC_HEALTHY_MUSHROOM                          = 30391,
    NPC_POISONOUS_MUSHROOM                        = 30435
};

enum Events
{
    EVENT_SPAWN                                   = 1,
    EVENT_ROOT,
    EVENT_BASH,
    EVENT_BOLT,
};

class boss_amanitar : public CreatureScript
{
public:
    boss_amanitar() : CreatureScript("boss_amanitar") { }

    struct boss_amanitarAI : public ScriptedAI
    {
        boss_amanitarAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            bFirstTime = true;
        }

        void Reset() override
        {
            events.Reset();
            events.ScheduleEvent(EVENT_ROOT,   urand(5, 9) * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_BASH, urand(10, 14) * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_BOLT, urand(15, 20) * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_SPAWN,            5 * IN_MILLISECONDS);

            me->SetMeleeDamageSchool(SPELL_SCHOOL_NATURE);
            me->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_NATURE, true);
            
            if (instance)
            {
                RemoveMini();
                if (!bFirstTime)
                    instance->SetData(DATA_AMANITAR_EVENT, FAIL);
                else
                    bFirstTime = false;
            }
        }

        void JustDied(Unit* /*killer*/) override
        {
            if (instance)
            {
                instance->SetData(DATA_AMANITAR_EVENT, DONE);
                RemoveMini();
            }

        }

        void EnterCombat(Unit* /*who*/) override
        {
            if (instance)
                instance->SetData(DATA_AMANITAR_EVENT, IN_PROGRESS);

            DoCastSelf(SPELL_MINI, false);
        }

        void RemoveMini()
        {
            for (Player& player : instance->instance->GetPlayers())
            {
                player.RemoveAura(SPELL_MINI);
                if (Pet* pet = player.GetPet())
                    pet->RemoveAura(SPELL_MINI);
            }
        }

        void SpawnAdds()
        {
            for (uint8 i = 0; i < 15; ++i)
            {
                Unit* victim = SelectTarget(SELECT_TARGET_RANDOM, 0);

                if (victim)
                {
                    Position pos;
                    victim->GetPosition(&pos);
                    me->GetRandomNearPosition(pos, float(urand(5, 80)));
                    me->SummonCreature(NPC_POISONOUS_MUSHROOM, pos, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 30*IN_MILLISECONDS);
                    me->GetRandomNearPosition(pos, float(urand(5, 80)));
                    me->SummonCreature(NPC_HEALTHY_MUSHROOM, pos, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 30*IN_MILLISECONDS);
                    DoCastSelf(SPELL_MINI, false);
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SPAWN:
                        SpawnAdds();
                        events.ScheduleEvent(EVENT_SPAWN, urand(35, 40) * IN_MILLISECONDS);
                        break;
                    case EVENT_ROOT:
                        DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true), SPELL_ENTANGLING_ROOTS, true);
                        events.ScheduleEvent(EVENT_ROOT, urand(15, 30) * IN_MILLISECONDS);
                        break;
                    case EVENT_BASH:
                        DoCastVictim(SPELL_BASH);
                        events.ScheduleEvent(EVENT_BASH, urand(15, 30) * IN_MILLISECONDS);
                        break;
                    case EVENT_BOLT:
                        DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true), SPELL_VENOM_BOLT_VOLLEY, true);
                        events.ScheduleEvent(EVENT_BOLT, urand(18, 22) * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
        private:
            EventMap events;
            InstanceScript* instance;
            bool bFirstTime;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_amanitarAI(creature);
    }
};

class npc_amanitar_mushrooms : public CreatureScript
{
public:
    npc_amanitar_mushrooms() : CreatureScript("npc_amanitar_mushrooms") { }

    struct npc_amanitar_mushroomsAI : public ScriptedAI
    {
        npc_amanitar_mushroomsAI(Creature* creature) : ScriptedAI(creature)
		{
			me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
		}

        uint32 uiAuraTimer;
        uint32 uiDeathTimer;

        void Reset()
        {
            DoCastSelf(SPELL_PUTRID_MUSHROOM, true); // Hack, to make the mushrooms visible, can't find orig. spell...
            if (me->GetEntry() == NPC_POISONOUS_MUSHROOM)
                DoCastSelf(SPELL_POISONOUS_MUSHROOM_VISUAL_AURA, true);

            uiAuraTimer = 0;
            uiDeathTimer = 30*IN_MILLISECONDS;
        }

        void JustDied(Unit* killer)
        {
            if (!killer)
                return;

            if (me->GetEntry() == NPC_HEALTHY_MUSHROOM && killer->GetTypeId() == TYPEID_PLAYER)
            {
                me->InterruptNonMeleeSpells(false);
                if(!killer->GetAuraCount(SPELL_HEALTHY_MUSHROOM_POTENT_FUNGUS))
                    killer->AddAura(SPELL_HEALTHY_MUSHROOM_POTENT_FUNGUS,killer);
                killer->RemoveAurasDueToSpell(SPELL_MINI);
            }
        }

        void EnterCombat(Unit* /*who*/) {}
        void AttackStart(Unit* /*victim*/) {}

        void UpdateAI(uint32 diff)
        {
            if (me->GetEntry() == NPC_POISONOUS_MUSHROOM)
            {
                if (uiAuraTimer <= diff)
                {
                    DoCastSelf(SPELL_POISONOUS_MUSHROOM_VISUAL_AREA, true);
                    DoCastSelf(SPELL_POISONOUS_MUSHROOM_POISON_CLOUD, false);
                    uiAuraTimer = 7*IN_MILLISECONDS;
                } else uiAuraTimer -= diff;
            }
            if (uiDeathTimer <= diff)
                me->DisappearAndDie();
            else uiDeathTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_amanitar_mushroomsAI(creature);
    }
};

void AddSC_boss_amanitar()
{
    new boss_amanitar;
    new npc_amanitar_mushrooms;
}
