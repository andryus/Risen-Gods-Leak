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

/* ScriptData
SDName: Boss_Majordomo_Executus
SD%Complete: 30
SDComment: Correct spawning and Event NYI
SDCategory: Molten Core
EndScriptData */

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "molten_core.h"
#include "Player.h"
#include "RG/Logging/LogManager.hpp"

enum Texts
{
    SAY_AGGRO           = 0,
    SAY_SPAWN           = 1,
    SAY_SLAY            = 2,
    SAY_SPECIAL         = 3,
    SAY_DEFEAT          = 4,

    SAY_SUMMON_MAJ      = 5,
    SAY_ARRIVAL2_MAJ    = 6
};

enum Spells
{
    SPELL_SUMMON_RAGNAROS     = 19774,
    SPELL_BLAST_WAVE          = 20229,
    SPELL_TELEPORT            = 20618,
    SPELL_MAGIC_REFLECTION    = 20619,
    SPELL_AEGIS_OF_RAGNAROS   = 20620,
    SPELL_DAMAGE_REFLECTION   = 21075
};

enum Extras
{
    OPTION_ID_YOU_CHALLENGED_US   = 0,
    MENU_OPTION_YOU_CHALLENGED_US = 4108
};

enum Events
{
    EVENT_MAGIC_REFLECTION  = 1,
    EVENT_DAMAGE_REFLECTION = 2,
    EVENT_BLAST_WAVE        = 3,
    EVENT_TELEPORT          = 4,

    EVENT_OUTRO_1           = 5,
    EVENT_OUTRO_2           = 6,
    EVENT_OUTRO_3           = 7
};

class boss_majordomo : public CreatureScript
{
    public:
        boss_majordomo() : CreatureScript("boss_majordomo") { }

        struct boss_majordomoAI : public BossAI
        {
            boss_majordomoAI(Creature* creature) : BossAI(creature, BOSS_MAJORDOMO_EXECUTUS) { }

            void KilledUnit(Unit* /*victim*/) override
            {
                if (urand(0, 99) < 25)
                    Talk(SAY_SLAY);
            }

            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
                Talk(SAY_AGGRO);
                events.ScheduleEvent(EVENT_MAGIC_REFLECTION,  30 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_DAMAGE_REFLECTION, 15 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_BLAST_WAVE,        10 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_TELEPORT,          20 * IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff) override
            {
                if (instance && instance->GetBossState(BOSS_MAJORDOMO_EXECUTUS) != DONE)
                {
                    if (!UpdateVictim())
                        return;

                    events.Update(diff);

                    if (!me->FindNearestCreature(NPC_FLAMEWAKER_HEALER, 100.0f) && !me->FindNearestCreature(NPC_FLAMEWAKER_ELITE, 100.0f))
                    {
                        instance->UpdateEncounterState(ENCOUNTER_CREDIT_KILL_CREATURE, me->GetEntry(), me);
                        me->setFaction(FACTION_FRIENDLY_TO_ALL);
                        me->AI()->EnterEvadeMode();
                        Talk(SAY_DEFEAT);
                        BossAI::JustDied(nullptr);
                        events.ScheduleEvent(EVENT_OUTRO_1, 32 * IN_MILLISECONDS);
                        RG_LOG<EncounterLogModule>(me);
                        return;
                    }

                    if (me->HasUnitState(UNIT_STATE_CASTING))
                        return;

                    if (HealthBelowPct(50))
                        DoCastSelf(SPELL_AEGIS_OF_RAGNAROS, true);

                    while (uint32 eventId = events.ExecuteEvent())
                    {
                        switch (eventId)
                        {
                            case EVENT_MAGIC_REFLECTION:
                                DoCastSelf(SPELL_MAGIC_REFLECTION);
                                events.ScheduleEvent(EVENT_MAGIC_REFLECTION, 30 * IN_MILLISECONDS);
                                break;
                            case EVENT_DAMAGE_REFLECTION:
                                DoCastSelf(SPELL_DAMAGE_REFLECTION);
                                events.ScheduleEvent(EVENT_DAMAGE_REFLECTION, 30 * IN_MILLISECONDS);
                                break;
                            case EVENT_BLAST_WAVE:
                                DoCastVictim(SPELL_BLAST_WAVE);
                                events.ScheduleEvent(EVENT_BLAST_WAVE, 10 * IN_MILLISECONDS);
                                break;
                            case EVENT_TELEPORT:
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                                    DoCast(target, SPELL_TELEPORT);
                                events.ScheduleEvent(EVENT_TELEPORT, 20 * IN_MILLISECONDS);
                                break;
                            default:
                                break;
                        }
                    }

                    DoMeleeAttackIfReady();
                }
                else
                {
                    events.Update(diff);

                    while (uint32 eventId = events.ExecuteEvent())
                    {
                        switch (eventId)
                        {
                            case EVENT_OUTRO_1:
                                me->NearTeleportTo(RagnarosTelePos.GetPositionX(), RagnarosTelePos.GetPositionY(), RagnarosTelePos.GetPositionZ(), RagnarosTelePos.GetOrientation());
                                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                                break;
                            case EVENT_OUTRO_2:
                                instance->instance->SummonCreature(NPC_RAGNAROS, RagnarosSummonPos);
                                break;
                            case EVENT_OUTRO_3:
                                Talk(SAY_ARRIVAL2_MAJ);
                                break;
                            default:
                                break;
                        }
                    }
                }
            }

            void DoAction(int32 action) override
            {
                if (action == ACTION_START_RAGNAROS)
                {
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    Talk(SAY_SUMMON_MAJ);
                    events.ScheduleEvent(EVENT_OUTRO_2,  8 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_OUTRO_3, 24 * IN_MILLISECONDS);
                }
                else if (action == ACTION_START_RAGNAROS_ALT)
                {
                    me->setFaction(FACTION_FRIENDLY_TO_ALL);
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                }
            }

            void sGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
            {
                if (menuId == MENU_OPTION_YOU_CHALLENGED_US && gossipListId == OPTION_ID_YOU_CHALLENGED_US)
                {
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    player->CLOSE_GOSSIP_MENU();
                    DoAction(ACTION_START_RAGNAROS);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_majordomoAI>(creature);
        }
};

void AddSC_boss_majordomo()
{
    new boss_majordomo();
}
