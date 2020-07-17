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
#include "SpellScript.h"

/*######
## Quest 6041: When Smokey Sings, I Get Violent
## spell_q6041_placing_smokeys_explosives
######*/

enum QuestWhenSmokeySingsIGetViolentMisc
{
    // Gameobjects
    GO_MARK_OF_DETONATION    = 177668,
     
     // Creatures
    NPC_SCOURGE_STRUCTURE_KC = 12247
};

class spell_q6041_placing_smokeys_explosives : public SpellScriptLoader
{
    public:
        spell_q6041_placing_smokeys_explosives() : SpellScriptLoader("spell_q6041_placing_smokeys_explosives") { }

        class spell_q6041_placing_smokeys_explosives_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q6041_placing_smokeys_explosives_SpellScript);
            
            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                {
                    if (GameObject* go = caster->FindNearestGameObject(GO_MARK_OF_DETONATION, 20.0f))
                    {
                        go->SetGoState(GO_STATE_READY);
                        go->SetLootState(GO_JUST_DEACTIVATED);
                        go->SendObjectDeSpawnAnim(go->GetGUID());
                        if (Player* player = caster->ToPlayer())
                            player->KilledMonsterCredit(NPC_SCOURGE_STRUCTURE_KC);
                    }
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_q6041_placing_smokeys_explosives_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_q6041_placing_smokeys_explosives_SpellScript();
        }
};

enum ErisHavenfireMisc
{
    // Quest
    QUEST_THE_BALANCE_OF_LIGHT_AND_SHADOW = 7622,

    // Creatureas
    NPC_PEASANT                           = 14485,
    NPC_FOOTSOLDIER                       = 14486,
    NPC_SCOURGE_ARCHER                    = 14489,
    NPC_SCOURGE_FOOTSOLDIER               = 14486,
    NPC_THE_CLEANER                       = 14503,

    // Action
    ACTION_START_SPAWN                    = 0,

    // Events
    EVENT_SPAWN_PEASANT                   = 1,
    EVENT_SPAWN_ARCHER                    = 2,

    // Texts
    SAY_WIN                               = 0,
    SAY_FAIL                              = 1,

    // Datatypes
    DATA_FAIL_COUNTER                     = 1,
    TYPE_FAIL_COUNTER                     = 1,
    DATA_WIN_COUNTER                      = 2,
    TYPE_WIN_COUNTER                      = 2,
};

Position const PeasantsPos[12] =
{   
    { 3364.47f,  -3048.50f,  165.17f,  1.86f   },
    { 3363.242f, -3052.06f,  165.264f, 2.095f  },
    { 3362.33f,  -3049.37f,  165.23f,  1.54f   },
    { 3360.13f,  -3052.63f,  165.31f,  1.88f   },
    { 3361.05f,  -3055.49f,  165.31f,  2.041f  },
    { 3363.92f,  -3046.96f,  165.09f,  2.13f   },
    { 3366.83f,  -3052.23f,  165.41f,  2.044f  },
    { 3367.79f,  -3047.80f,  165.16f,  2.08f   },
    { 3358.76f,  -3050.37f,  165.2f,   2.05f   },
    { 3366.63f,  -3045.29f,  164.99f,  2.19f   },
    { 3357.45f,  -3052.82f,  165.50f,  2.006f  },
    { 3363.00f,  -3044.21f,  164.80f,  2.182f  },
};

Position const ArcherPos[8] =
{
    { 3327.42f, -3021.11f, 170.57f, 6.01f },    
    { 3335.4f,  -3054.3f,  173.63f, 0.49f },
    { 3351.3f,  -3079.08f, 178.67f, 1.15f },
    { 3358.93f, -3076.1f,  174.87f, 1.57f },
    { 3371.58f, -3069.24f, 175.20f, 1.99f },
    { 3369.46f, -3023.11f, 171.83f, 3.69f },
    { 3383.25f, -3057.01f, 181.53f, 2.21f },
    { 3380.03f, -3062.73f, 181.90f, 2.31f },
};

class npc_eris_havenfire : public CreatureScript
{
    public:
        npc_eris_havenfire() : CreatureScript("npc_eris_havenfire") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) 
        {
            if (quest->GetQuestId() == QUEST_THE_BALANCE_OF_LIGHT_AND_SHADOW)
            {
                creature->AI()->DoAction(ACTION_START_SPAWN);
                creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                creature->AI()->SetGuidData(player->GetGUID());
            }
            return true;
        }
    
        struct npc_eris_havenfireAI : public ScriptedAI
        {
            npc_eris_havenfireAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                events.Reset();
                failCounter  = 0;
                winCounter   = 0;
                SavePeasants = 50;
                DeadPeasants = 15;
                uiPlayer     .Clear();
                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                me->setFaction(FACTION_FRIENDLY_TO_ALL);
            }

            void SpawnPeasants()
            {
                for (int i = 0; i < 12; ++i)
                    if (Creature* peasant = me->SummonCreature(NPC_PEASANT, PeasantsPos[i], TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 600000))
                        peasant->setActive(true);
            }

            void CompleteQuest()
            {
                if (Player* player = ObjectAccessor::GetPlayer(*me, uiPlayer))
                    player->CompleteQuest(QUEST_THE_BALANCE_OF_LIGHT_AND_SHADOW);
            }

            void FailQuest()
            {
                if (Player* player = ObjectAccessor::GetPlayer(*me, uiPlayer))
                    player->FailQuest(QUEST_THE_BALANCE_OF_LIGHT_AND_SHADOW);
            }

            void DespawnSummons()
            {
                std::list<Creature*> SummonList;
                me->GetCreatureListWithEntryInGrid(SummonList, NPC_PEASANT,             1000.0f);
                me->GetCreatureListWithEntryInGrid(SummonList, NPC_FOOTSOLDIER,         1000.0f);
                me->GetCreatureListWithEntryInGrid(SummonList, NPC_SCOURGE_ARCHER,      1000.0f); 
                me->GetCreatureListWithEntryInGrid(SummonList, NPC_SCOURGE_FOOTSOLDIER, 1000.0f);            
                if (!SummonList.empty())
                    for (std::list<Creature*>::iterator itr = SummonList.begin(); itr != SummonList.end(); itr++)
                        (*itr)->DespawnOrUnsummon();
            }

            void SetData(uint32 type, uint32 data) override
            {
                if (type == DATA_FAIL_COUNTER && data == TYPE_FAIL_COUNTER)
                    ++failCounter;

                if (type == DATA_WIN_COUNTER && data == TYPE_WIN_COUNTER)
                    ++winCounter;
            }

            void SetGuidData(ObjectGuid guid, uint32 /*id*/) override
            {
                uiPlayer = guid;
            }

            void DoAction(int32 action) override
            {
                if (action == ACTION_START_SPAWN)
                {
                    events.ScheduleEvent(EVENT_SPAWN_PEASANT, 2 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_SPAWN_ARCHER,  4 * IN_MILLISECONDS);
                    me->setActive(true);
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (winCounter == SavePeasants)
                {
                    CompleteQuest();
                    EnterEvadeMode();
                    DespawnSummons();
                    Talk(SAY_WIN);
                    me->SummonCreature(NPC_THE_CLEANER, 3331.817627f, -2975.350098f, 160.542480f, 4.870414f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30 * IN_MILLISECONDS);
                }

                if (failCounter == DeadPeasants)
                {
                    FailQuest();
                    EnterEvadeMode();
                    DespawnSummons();                    
                    Talk(SAY_FAIL);           
                }

                events.Update(diff);
            
                switch (events.ExecuteEvent())
                {
                    case EVENT_SPAWN_PEASANT:
                        SpawnPeasants();
                        events.ScheduleEvent(EVENT_SPAWN_PEASANT, 60 * IN_MILLISECONDS);
                        break;
                    case EVENT_SPAWN_ARCHER:
                        for (int i = 0; i < 8; ++i)
                            if (Creature* archer = me->SummonCreature(NPC_SCOURGE_ARCHER, ArcherPos[i], TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 600000))
                                archer->setActive(true);
                        break;
                    default:
                        break;
                }
            }
            
        private:
            EventMap events;
            uint8 failCounter;
            uint8 winCounter;
            uint8 SavePeasants;
            uint8 DeadPeasants;
            ObjectGuid uiPlayer;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_eris_havenfireAI(creature);
    }

};

void AddSC_eastern_plaguelands_rg()
{
    new spell_q6041_placing_smokeys_explosives();
    new npc_eris_havenfire();
}
