/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
SDName: Npcs_Special
SD%Complete: 100
SDComment: To be used for special NPCs that are located globally.
SDCategory: NPCs
EndScriptData
*/

/* ContentData
npc_air_force_bots       80%    support for misc (invisible) guard bots in areas where player allowed to fly. Summon guards after a preset time if tagged by spell
npc_lunaclaw_spirit      80%    support for quests 6001/6002 (Body and Heart)
npc_chicken_cluck       100%    support for quest 3861 (Cluck!)
npc_dancing_flames      100%    midsummer event NPC
npc_guardian            100%    guardianAI used to prevent players from accessing off-limits areas. Not in use by SD2
npc_garments_of_quests   80%    NPC's related to all Garments of-quests 5621, 5624, 5625, 5648, 565
npc_injured_patient     100%    patients for triage-quests (6622 and 6624)
npc_doctor              100%    Gustaf Vanhowzen and Gregory Victor, quest 6622 and 6624 (Triage)
npc_sayge               100%    Darkmoon event fortune teller, buff player based on answers given
npc_snake_trap_serpents  80%    AI for snakes that summoned by Snake Trap
npc_shadowfiend         100%   restore 5% of owner's mana when shadowfiend die from damage
npc_firework            100%    NPC's summoned by rockets and rocket clusters, for making them cast visual
npc_train_wrecker       100%    Wind-Up Train Wrecker that kills train set
EndContentData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"
#include "ScriptedFollowerAI.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "World.h"
#include "PassiveAI.h"
#include "GameEventMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "SpellHistory.h"
#include "SpellAuras.h"
#include "PetAI.h"
#include "Pet.h"
#include "SmartAI.h"
#include "CreatureTextMgr.h"
#include "Battleground.h"
#include "Packets/WorldPacket.h"

/*########
# npc_air_force_bots
#########*/

enum SpawnType
{
    SPAWNTYPE_TRIPWIRE_ROOFTOP,                             // no warning, summon Creature at smaller range
    SPAWNTYPE_ALARMBOT,                                     // cast guards mark and summon npc - if player shows up with that buff duration < 5 seconds attack
};

struct SpawnAssociation
{
    uint32 thisCreatureEntry;
    uint32 spawnedCreatureEntry;
    SpawnType spawnType;
};

enum AirFoceBots
{
    SPELL_GUARDS_MARK               = 38067,
    AURA_DURATION_TIME_LEFT         = 5000
};

float const RANGE_TRIPWIRE          = 15.0f;
float const RANGE_GUARDS_MARK       = 50.0f;

SpawnAssociation spawnAssociations[] =
{
    {2614,  15241, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Alliance)
    {2615,  15242, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Horde)
    {21974, 21976, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Area 52)
    {21993, 15242, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Horde - Bat Rider)
    {21996, 15241, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Alliance - Gryphon)
    {21997, 21976, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Goblin - Area 52 - Zeppelin)
    {21999, 15241, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Alliance)
    {22001, 15242, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Horde)
    {22002, 15242, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Ground (Horde)
    {22003, 15241, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Ground (Alliance)
    {22063, 21976, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Goblin - Area 52)
    {22065, 22064, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Ethereal - Stormspire)
    {22066, 22067, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Scryer - Dragonhawk)
    {22068, 22064, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Ethereal - Stormspire)
    {22069, 22064, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Stormspire)
    {22070, 22067, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Scryer)
    {22071, 22067, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Scryer)
    {22078, 22077, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Aldor)
    {22079, 22077, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Aldor - Gryphon)
    {22080, 22077, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Aldor)
    {22086, 22085, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Sporeggar)
    {22087, 22085, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Sporeggar - Spore Bat)
    {22088, 22085, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Sporeggar)
    {22090, 22089, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Toshley's Station - Flying Machine)
    {22124, 22122, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Cenarion)
    {22125, 22122, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Cenarion - Stormcrow)
    {22126, 22122, SPAWNTYPE_ALARMBOT}                      //Air Force Trip Wire - Rooftop (Cenarion Expedition)
};

class npc_air_force_bots : public CreatureScript
{
public:
    npc_air_force_bots() : CreatureScript("npc_air_force_bots") { }

    struct npc_air_force_botsAI : public ScriptedAI
    {
        npc_air_force_botsAI(Creature* creature) : ScriptedAI(creature)
        {
            SpawnAssoc = NULL;
            SpawnedGUID.Clear();

            // find the correct spawnhandling
            static uint32 entryCount = sizeof(spawnAssociations) / sizeof(SpawnAssociation);

            for (uint8 i = 0; i < entryCount; ++i)
            {
                if (spawnAssociations[i].thisCreatureEntry == creature->GetEntry())
                {
                    SpawnAssoc = &spawnAssociations[i];
                    break;
                }
            }

            if (!SpawnAssoc)
                TC_LOG_ERROR("sql.sql", "TCSR: Creature template entry %u has ScriptName npc_air_force_bots, but it's not handled by that script", creature->GetEntry());
            else
            {
                CreatureTemplate const* spawnedTemplate = sObjectMgr->GetCreatureTemplate(SpawnAssoc->spawnedCreatureEntry);

                if (!spawnedTemplate)
                {
                    TC_LOG_ERROR("sql.sql", "TCSR: Creature template entry %u does not exist in DB, which is required by npc_air_force_bots", SpawnAssoc->spawnedCreatureEntry);
                    SpawnAssoc = NULL;
                    return;
                }
            }

            checkTimer = 1000;
        }

        SpawnAssociation* SpawnAssoc;
        ObjectGuid SpawnedGUID;
        uint32 checkTimer;

        void Reset() override { }

        Creature* SummonGuard()
        {
            Creature* summoned = me->SummonCreature(SpawnAssoc->spawnedCreatureEntry, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 300000);

            if (summoned)
                SpawnedGUID = summoned->GetGUID();
            else
            {
                TC_LOG_ERROR("sql.sql", "TCSR: npc_air_force_bots: wasn't able to spawn Creature %u", SpawnAssoc->spawnedCreatureEntry);
                SpawnAssoc = NULL;
            }

            return summoned;
        }

        Creature* GetSummonedGuard()
        {
            Creature* creature = ObjectAccessor::GetCreature(*me, SpawnedGUID);

            if (creature && creature->IsAlive())
                return creature;

            return NULL;
        }

        void Update()
        {
            if (!SpawnAssoc)
                return;

            std::list<Player*> players;
            me->GetPlayerListInGrid(players, RANGE_GUARDS_MARK);
            for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
            {
                Player* who = *itr;

                if (me->IsValidAttackTarget(who))
                {
                    Creature* lastSpawnedGuard = SpawnedGUID.IsEmpty() ? NULL : GetSummonedGuard();

                    // prevent calling Unit::GetUnit at next MoveInLineOfSight call - speedup
                    if (!lastSpawnedGuard)
                        SpawnedGUID = ObjectGuid::Empty;

                    switch (SpawnAssoc->spawnType)
                    {
                        case SPAWNTYPE_ALARMBOT:
                        {
                            if (!who->IsWithinDistInMap(me, RANGE_GUARDS_MARK))
                                return;

                            Aura* markAura = who->GetAura(SPELL_GUARDS_MARK);
                            if (markAura)
                            {
                                // the target wasn't able to move out of our range within 25 seconds
                                if (!lastSpawnedGuard)
                                {
                                    lastSpawnedGuard = SummonGuard();

                                    if (!lastSpawnedGuard)
                                        return;
                                }

                                if (markAura->GetDuration() < AURA_DURATION_TIME_LEFT)
                                    if (!lastSpawnedGuard->GetVictim())
                                        lastSpawnedGuard->AI()->AttackStart(who);
                            }
                            else
                            {
                                if (!lastSpawnedGuard)
                                    lastSpawnedGuard = SummonGuard();

                                if (!lastSpawnedGuard)
                                    return;

                                lastSpawnedGuard->CastSpell(who, SPELL_GUARDS_MARK, true);
                            }
                            break;
                        }
                        case SPAWNTYPE_TRIPWIRE_ROOFTOP:
                        {
                            if (!who->IsWithinDistInMap(me, RANGE_TRIPWIRE))
                                return;

                            if (!lastSpawnedGuard)
                                lastSpawnedGuard = SummonGuard();

                            if (!lastSpawnedGuard)
                                return;

                            // ROOFTOP only triggers if the player is on the ground
                            if (!who->IsFlying() && !lastSpawnedGuard->GetVictim())
                                lastSpawnedGuard->AI()->AttackStart(who);

                            break;
                        }
                    }
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (checkTimer <= diff)
            {
                Update();
                checkTimer = 1000;
            }
            else
                checkTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_air_force_botsAI(creature);
    }
};

/*######
## npc_lunaclaw_spirit
######*/

enum LunaclawSpirit
{
    QUEST_BODY_HEART_A      = 6001,
    QUEST_BODY_HEART_H      = 6002,

    TEXT_ID_DEFAULT         = 4714,
    TEXT_ID_PROGRESS        = 4715
};

class npc_lunaclaw_spirit : public CreatureScript
{
public:
    npc_lunaclaw_spirit() : CreatureScript("npc_lunaclaw_spirit") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (player->GetQuestStatus(QUEST_BODY_HEART_A) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(QUEST_BODY_HEART_H) == QUEST_STATUS_INCOMPLETE)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(60079, 0), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

        player->SEND_GOSSIP_MENU(TEXT_ID_DEFAULT, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF + 1)
        {
            player->SEND_GOSSIP_MENU(TEXT_ID_PROGRESS, creature->GetGUID());
            player->AreaExploredOrEventHappens(player->GetTeam() == ALLIANCE ? QUEST_BODY_HEART_A : QUEST_BODY_HEART_H);
        }
        return true;
    }
};

/*########
# npc_chicken_cluck
#########*/

enum ChickenCluck
{
    EMOTE_HELLO_A       = 0,
    EMOTE_HELLO_H       = 1,
    EMOTE_CLUCK_TEXT    = 2,

    QUEST_CLUCK         = 3861,
    FACTION_FRIENDLY    = 35,
    FACTION_CHICKEN     = 31
};

class npc_chicken_cluck : public CreatureScript
{
public:
    npc_chicken_cluck() : CreatureScript("npc_chicken_cluck") { }

    struct npc_chicken_cluckAI : public ScriptedAI
    {
        npc_chicken_cluckAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 ResetFlagTimer;

        void Reset() override
        {
            ResetFlagTimer = 120000;
            me->setFaction(FACTION_CHICKEN);
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
        }

        void EnterCombat(Unit* /*who*/) override { }

        void UpdateAI(uint32 diff) override
        {
            // Reset flags after a certain time has passed so that the next player has to start the 'event' again
            if (me->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER))
            {
                if (ResetFlagTimer <= diff)
                {
                    EnterEvadeMode();
                    return;
                }
                else
                    ResetFlagTimer -= diff;
            }

            if (UpdateVictim())
                DoMeleeAttackIfReady();
        }

        void ReceiveEmote(Player* player, uint32 emote) override
        {
            switch (emote)
            {
                case TEXT_EMOTE_CHICKEN:
                    if (player->GetQuestStatus(QUEST_CLUCK) == QUEST_STATUS_NONE && rand32() % 30 == 1)
                    {
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                        me->setFaction(FACTION_FRIENDLY);
                        Talk(player->GetTeam() == HORDE ? EMOTE_HELLO_H : EMOTE_HELLO_A);
                    }
                    break;
                case TEXT_EMOTE_CHEER:
                    if (player->GetQuestStatus(QUEST_CLUCK) == QUEST_STATUS_COMPLETE)
                    {
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                        me->setFaction(FACTION_FRIENDLY);
                        Talk(EMOTE_CLUCK_TEXT);
                    }
                    break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_chicken_cluckAI(creature);
    }

    bool OnQuestAccept(Player* /*player*/, Creature* creature, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_CLUCK)
            ENSURE_AI(npc_chicken_cluck::npc_chicken_cluckAI, creature->AI())->Reset();

        return true;
    }

    bool OnQuestReward(Player* /*player*/, Creature* creature, Quest const* quest, uint32 /*opt*/) override
    {
        if (quest->GetQuestId() == QUEST_CLUCK)
            ENSURE_AI(npc_chicken_cluck::npc_chicken_cluckAI, creature->AI())->Reset();

        return true;
    }
};

/*######
## npc_dancing_flames
######*/

enum DancingFlames
{
    SPELL_BRAZIER           = 45423,
    SPELL_SEDUCTION         = 47057,
    SPELL_FIERY_AURA        = 45427
};

class npc_dancing_flames : public CreatureScript
{
public:
    npc_dancing_flames() : CreatureScript("npc_dancing_flames") { }

    struct npc_dancing_flamesAI : public ScriptedAI
    {
        npc_dancing_flamesAI(Creature* creature) : ScriptedAI(creature) { }

        bool Active;
        uint32 CanIteract;

        void Reset() override
        {
            Active = true;
            CanIteract = 3500;
            DoCastSelf(SPELL_BRAZIER, true);
            DoCastSelf(SPELL_FIERY_AURA, false);
            float x, y, z;
            me->GetPosition(x, y, z);
            me->Relocate(x, y, z + 0.94f);
            me->SetDisableGravity(true);
            me->HandleEmoteCommand(EMOTE_ONESHOT_DANCE);
            //send update position to client
            WorldPacket data = me->BuildHeartBeatMsg();
            me->SendMessageToSet(data, true);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!Active)
            {
                if (CanIteract <= diff)
                {
                    Active = true;
                    CanIteract = 3500;
                    me->HandleEmoteCommand(EMOTE_ONESHOT_DANCE);
                }
                else
                    CanIteract -= diff;
            }
        }

        void EnterCombat(Unit* /*who*/) override { }

        void ReceiveEmote(Player* player, uint32 emote) override
        {
            if (me->IsWithinLOS(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ()) && me->IsWithinDistInMap(player, 30.0f))
            {
                me->SetInFront(player);
                Active = false;

                WorldPacket data = me->BuildHeartBeatMsg();
                me->SendMessageToSet(data, true);
                switch (emote)
                {
                    case TEXT_EMOTE_KISS:
                        me->HandleEmoteCommand(EMOTE_ONESHOT_SHY);
                        break;
                    case TEXT_EMOTE_WAVE:
                        me->HandleEmoteCommand(EMOTE_ONESHOT_WAVE);
                        break;
                    case TEXT_EMOTE_BOW:
                        me->HandleEmoteCommand(EMOTE_ONESHOT_BOW);
                        break;
                    case TEXT_EMOTE_JOKE:
                        me->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH);
                        break;
                    case TEXT_EMOTE_DANCE:
                        if (!player->HasAura(SPELL_SEDUCTION))
                            DoCast(player, SPELL_SEDUCTION, true);
                        break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_dancing_flamesAI(creature);
    }
};

/*######
## Triage quest
######*/

enum TriageQuest
{
    SAY_DOC                     = 0,
    SAY_SPAWN                   = 0,
    SAY_SAVED                   = 1,
    SAY_DEATH                   = 2,

    QUEST_TRIAGE_A              = 6624,
    QUEST_TRIAGE_H              = 6622,

    SPELL_TRIAGE                = 20804,

    ALLIANCE_COORDS             = 7,
    HORDE_COORDS                = 6,

    NPC_A_DOC                   = 12939,
    NPC_H_DOC                   = 12920,

    NPC_A_INJURED_SOLDIER       = 12938,
    NPC_A_BADLY_INJURED_SOLDIER = 12936,
    NPC_A_CRIT_INJURED_SOLDIER  = 12937,

    NPC_H_INJURED_SOLDIER       = 12923,
    NPC_H_BADLY_INJURED_SOLDIER = 12924,
    NPC_H_CRIT_INJURED_SOLDIER  = 12925,

    EVENT_REDUCE_HEALTH         = 1,
    EVENT_SPAWN_NEW_PATIENTS    = 2,
    EVENT_GET_VISIBLE           = 3,
    EVENT_CHECK_CANCEL_QUEST    = 4,

    ACTION_CHECK_PATIENT        = 1,
    ACTION_DEAD_PATIENT         = 2,
};

const Position AllianceCoords[7] =
{
    { -3757.38f, -4533.05f, 14.16f, 3.62f },                      // Top-far-right bunk as seen from entrance
    { -3754.36f, -4539.13f, 14.16f, 5.13f },                      // Top-far-left bunk
    { -3749.54f, -4540.25f, 14.28f, 3.34f },                      // Far-right bunk
    { -3742.10f, -4536.85f, 14.28f, 3.64f },                      // Right bunk near entrance
    { -3755.89f, -4529.07f, 14.05f, 0.57f },                      // Far-left bunk
    { -3749.51f, -4527.08f, 14.07f, 5.26f },                      // Mid-left bunk
    { -3746.37f, -4525.35f, 14.16f, 5.22f },                      // Left bunk near entrance
};

const Position HordeCoords[6] =
{
    { -1013.75f, -3492.59f, 62.62f, 4.34f },                      // Left, Behind
    { -1017.72f, -3490.92f, 62.62f, 4.34f },                      // Right, Behind
    { -1015.77f, -3497.15f, 62.82f, 4.34f },                      // Left, Mid
    { -1019.51f, -3495.49f, 62.82f, 4.34f },                      // Right, Mid
    { -1017.25f, -3500.85f, 62.98f, 4.34f },                      // Left, front
    { -1020.95f, -3499.21f, 62.98f, 4.34f }                       // Right, Front
};

/*######
## npc_doctor (handles both Gustaf Vanhowzen and Gregory Victor)
######*/

class npc_doctor : public CreatureScript
{
public:
    npc_doctor() : CreatureScript("npc_doctor") { }

    struct npc_doctorAI : public ScriptedAI
    {
        npc_doctorAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            isEventRunning = false;
        }

        void StartEvent(Player* player)
        {
            if (!player || isEventRunning)
                return;

            patients.clear();
            patientDiedCount    = 0;
            patientSavedCount   = 0;
            isEventRunning      = true;
            playerGUID.Clear();
            playerGUID = player->GetGUID();
            SpawnNewPatient(true, 0);
            _events.Reset();
            _events.ScheduleEvent(EVENT_CHECK_CANCEL_QUEST, Seconds(1));
        }

        void DoAction(int32 action) override
        {
            if (action == ACTION_CHECK_PATIENT)
            {
                if (++patientSavedCount == 15 && isEventRunning)
                    if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
                        if ((player->GetQuestStatus(QUEST_TRIAGE_A) == QUEST_STATUS_INCOMPLETE) || (player->GetQuestStatus(QUEST_TRIAGE_H) == QUEST_STATUS_INCOMPLETE))
                        {
                            if (player->GetQuestStatus(QUEST_TRIAGE_A) == QUEST_STATUS_INCOMPLETE)
                                player->AreaExploredOrEventHappens(QUEST_TRIAGE_A);
                            else if (player->GetQuestStatus(QUEST_TRIAGE_H) == QUEST_STATUS_INCOMPLETE)
                                player->AreaExploredOrEventHappens(QUEST_TRIAGE_H);

                            for (auto patient : patients)
                                if (Creature* despawn = ObjectAccessor::GetCreature(*me, patient))
                                    despawn->DespawnOrUnsummon();

                            Reset();
                            return;
                        }
            }
            else if (action == ACTION_DEAD_PATIENT)
                ++patientDiedCount;
        }

        void SpawnNewPatient(bool init, uint32 position)
        {
            if (!init && isEventRunning)
            {
                if (patientDiedCount > 5)
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
                    {
                        if ((player->GetQuestStatus(QUEST_TRIAGE_A) == QUEST_STATUS_INCOMPLETE) || (player->GetQuestStatus(QUEST_TRIAGE_H) == QUEST_STATUS_INCOMPLETE))
                        {
                            if (player->GetQuestStatus(QUEST_TRIAGE_A) == QUEST_STATUS_INCOMPLETE)
                                player->FailQuest(QUEST_TRIAGE_A);
                            else if (player->GetQuestStatus(QUEST_TRIAGE_H) == QUEST_STATUS_INCOMPLETE)
                                player->FailQuest(QUEST_TRIAGE_H);

                            for (auto patient : patients)
                                if (Creature* despawn = ObjectAccessor::GetCreature(*me, patient))
                                    despawn->DespawnOrUnsummon();

                            Talk(SAY_DOC);

                            isEventRunning = false;
                            Reset();
                            return;
                        }
                    }
                }

                if (me->GetEntry() == NPC_A_DOC)
                {
                    if (Creature* patient = me->SummonCreature(GetEntry(true), AllianceCoords[position]))
                    {
                        patients.insert(patient->GetGUID());
                        patient->AI()->DoAction(position);
                    }
                }
                else
                {
                    if (Creature* patient = me->SummonCreature(GetEntry(false), HordeCoords[position]))
                    {
                        patients.insert(patient->GetGUID());
                        patient->AI()->DoAction(position);
                    }
                }
            }
            else if (init && isEventRunning)
            {
                if (me->GetEntry() == NPC_A_DOC)
                {
                    for (uint32 i = 0; i < 7; ++i)
                    {
                        if (Creature* patient = me->SummonCreature(GetEntry(true), AllianceCoords[i]))
                        {
                            patients.insert(patient->GetGUID());
                            patient->AI()->DoAction(i);
                        }
                    }
                }
                else
                {
                    for (uint32 i = 0; i < 6; ++i)
                    {
                        if (Creature* patient = me->SummonCreature(GetEntry(false), HordeCoords[i]))
                        {
                            patients.insert(patient->GetGUID());
                            patient->AI()->DoAction(i);
                        }
                    }
                }
            }
        }

        uint32 GetEntry(bool isAlly)
        {
            uint32 chance = urand(0, 650);

            if (chance < 451)
                return (isAlly ? NPC_A_INJURED_SOLDIER : NPC_H_INJURED_SOLDIER);
            else if (chance > 450 && chance < 601)
                return (isAlly ? NPC_A_BADLY_INJURED_SOLDIER : NPC_H_BADLY_INJURED_SOLDIER);
            else
                return (isAlly ? NPC_A_CRIT_INJURED_SOLDIER : NPC_H_CRIT_INJURED_SOLDIER);
        }

        void UpdateAI(uint32 diff) override
        {
            ScriptedAI::UpdateAI(diff);

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CHECK_CANCEL_QUEST:
                        if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
                        {
                            if (!player->IsActiveQuest(QUEST_TRIAGE_A) && !player->IsActiveQuest(QUEST_TRIAGE_H))
                            {
                                isEventRunning = false;
                                for (auto patient : patients)
                                    if (Creature* despawn = ObjectAccessor::GetCreature(*me, patient))
                                        despawn->DespawnOrUnsummon();
                                Reset();
                                return;
                            }
                        }
                        _events.ScheduleEvent(EVENT_CHECK_CANCEL_QUEST, Seconds(1));
                        break;
                    default:
                        break;
                }
            }
        }

        private:
            ObjectGuid  playerGUID;
            GuidSet     patients;
            uint32      patientDiedCount;
            uint32      patientSavedCount;
            bool        isEventRunning;
            EventMap    _events;
    };

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_TRIAGE_A || quest->GetQuestId() == QUEST_TRIAGE_H)
            ENSURE_AI(npc_doctor::npc_doctorAI, creature->AI())->StartEvent(player);

        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_doctorAI(creature);
    }
};

class npc_injured_patient : public CreatureScript
{
public:
    npc_injured_patient() : CreatureScript("npc_injured_patient") { }

    struct npc_injured_patientAI : public ScriptedAI
    {
        npc_injured_patientAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            if (Creature* allyDoc = me->FindNearestCreature(NPC_A_DOC, 40.0f))
                doctorGUID = allyDoc->GetGUID();
            else if (Creature* hordeDoc = me->FindNearestCreature(NPC_H_DOC, 40.0f))
                doctorGUID = hordeDoc->GetGUID();

            me->SetVisible(false);
            //no select
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

            //no regen health
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);

            //to make them lay with face down
            me->SetStandState(UNIT_STAND_STATE_DEAD);

            switch (me->GetEntry())
            {                                                   //lower max health
                case NPC_H_INJURED_SOLDIER:
                case NPC_A_INJURED_SOLDIER:                     //Injured Soldier
                    myHealth = 55100;
                    break;
                case NPC_H_BADLY_INJURED_SOLDIER:
                case NPC_A_BADLY_INJURED_SOLDIER:               //Badly injured Soldier
                    myHealth = 40100;
                    break;
                case NPC_A_CRIT_INJURED_SOLDIER:
                case NPC_H_CRIT_INJURED_SOLDIER:                //Critically injured Soldier
                    myHealth = 20100;
                    break;
                default:
                    break;
            }
            _events.Reset();
            _events.ScheduleEvent(EVENT_GET_VISIBLE, Seconds(5));

            myPosition = 0;
        }

        void DoAction(int32 action) override
        {
            myPosition = action;
        }

        void SpellHit(Unit* caster, SpellInfo const* spell) override
        {
            Player* player = caster->ToPlayer();
            if (!player || spell->Id != SPELL_TRIAGE)
                return;

            if (player->GetQuestStatus(QUEST_TRIAGE_A) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(QUEST_TRIAGE_H) == QUEST_STATUS_INCOMPLETE)
                if (Creature* doctor = ObjectAccessor::GetCreature(*me, doctorGUID))
                    doctor->AI()->DoAction(ACTION_CHECK_PATIENT);

            _events.CancelEvent(EVENT_REDUCE_HEALTH);

            //make not selectable
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            //stand up
            me->SetStandState(UNIT_STAND_STATE_STAND);

            Talk(SAY_SAVED);
            me->SetWalk(false);

            if (Creature* doctor = ObjectAccessor::GetCreature((*me), doctorGUID))
            {
                ENSURE_AI(npc_doctor::npc_doctorAI, doctor->AI())->SpawnNewPatient(false, myPosition);
                me->GetMotionMaster()->MoveFollow(doctor, 0.0f, 0.0f);
            }

            me->DespawnOrUnsummon(5000);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_REDUCE_HEALTH:
                        if (me->IsAlive() && myHealth > 100)
                            myHealth = myHealth - 1000;
                        _events.ScheduleEvent(EVENT_REDUCE_HEALTH, Seconds(1));
                        break;
                    case EVENT_GET_VISIBLE:
                        me->SetVisible(true);
                        Talk(SAY_SPAWN);
                        _events.ScheduleEvent(EVENT_REDUCE_HEALTH, Seconds(1));
                        break;
                default:
                    break;
                }
            }

            if (myHealth <= 100)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->setDeathState(JUST_DIED);
                me->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);

                Talk(SAY_DEATH);

                if (Creature* doctor = ObjectAccessor::GetCreature((*me), doctorGUID))
                {
                    ENSURE_AI(npc_doctor::npc_doctorAI, doctor->AI())->SpawnNewPatient(false, myPosition);
                    doctor->AI()->DoAction(ACTION_DEAD_PATIENT);
                }

                me->DespawnOrUnsummon(2000);
            }
        }

        private:
            ObjectGuid doctorGUID;
            EventMap   _events;
            uint32     myHealth;
            uint32     myPosition;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_injured_patientAI(creature);
    }
};

/*######
## npc_garments_of_quests
######*/

/// @todo get text for each NPC

enum Garments
{
    SPELL_LESSER_HEAL_R1    = 2050,
    SPELL_LESSER_HEAL_R2    = 2052,
    SPELL_FORTITUDE_R1      = 1243,

    QUEST_MOON              = 5621,
    QUEST_LIGHT_1           = 5624,
    QUEST_LIGHT_2           = 5625,
    QUEST_SPIRIT            = 5648,
    QUEST_DARKNESS          = 5650,
    QUEST_TAVARA            = 9586,

    ENTRY_SHAYA             = 12429,
    ENTRY_ROBERTS           = 12423,
    ENTRY_DOLF              = 12427,
    ENTRY_KORJA             = 12430,
    ENTRY_DG_KEL            = 12428,
    ENTRY_TAVARA            = 17551,

    // used by 12429, 12423, 12427, 12430, 12428, but signed for 12429
    SAY_THANKS              = 0,
    SAY_GOODBYE             = 1,
    SAY_HEALED              = 2,
};

class npc_garments_of_quests : public CreatureScript
{
public:
    npc_garments_of_quests() : CreatureScript("npc_garments_of_quests") { }

    struct npc_garments_of_questsAI : public npc_escortAI
    {
        npc_garments_of_questsAI(Creature* creature) : npc_escortAI(creature)
        {
            Reset();
        }

        ObjectGuid CasterGUID;

        bool IsHealed;
        bool CanRun;

        uint32 RunAwayTimer;

        void Reset() override
        {
            CasterGUID.Clear();

            IsHealed = false;
            CanRun = false;

            RunAwayTimer = 5000;

            me->SetStandState(UNIT_STAND_STATE_KNEEL);
            // expect database to have RegenHealth=0
            me->SetHealth(me->CountPctFromMaxHealth(70));
        }

        void EnterCombat(Unit* /*who*/) override { }

        void SpellHit(Unit* caster, SpellInfo const* spell) override
        {
            if (spell->Id == SPELL_LESSER_HEAL_R2 || spell->Id == SPELL_FORTITUDE_R1)
            {
                //not while in combat
                if (me->IsInCombat())
                    return;

                //nothing to be done now
                if (IsHealed && CanRun)
                    return;

                if (Player* player = caster->ToPlayer())
                {
                    switch (me->GetEntry())
                    {
                        case ENTRY_TAVARA:
                            if (player->GetQuestStatus(QUEST_TAVARA) == QUEST_STATUS_INCOMPLETE)
                            {
                                if (spell->Id == SPELL_LESSER_HEAL_R1)
                                {
                                    me->SetStandState(UNIT_STAND_STATE_STAND);
                                    Talk(SAY_HEALED, player);
                                    CanRun = true;
                                }
                            }
                            break;
                        case ENTRY_SHAYA:
                            if (player->GetQuestStatus(QUEST_MOON) == QUEST_STATUS_INCOMPLETE)
                            {
                                if (IsHealed && !CanRun && spell->Id == SPELL_FORTITUDE_R1)
                                {
                                    Talk(SAY_THANKS, caster);
                                    CanRun = true;
                                }
                                else if (!IsHealed && spell->Id == SPELL_LESSER_HEAL_R2)
                                {
                                    CasterGUID = caster->GetGUID();
                                    me->SetStandState(UNIT_STAND_STATE_STAND);
                                    Talk(SAY_HEALED, caster);
                                    IsHealed = true;
                                }
                            }
                            break;
                        case ENTRY_ROBERTS:
                            if (player->GetQuestStatus(QUEST_LIGHT_1) == QUEST_STATUS_INCOMPLETE)
                            {
                                if (IsHealed && !CanRun && spell->Id == SPELL_FORTITUDE_R1)
                                {
                                    Talk(SAY_THANKS, caster);
                                    CanRun = true;
                                }
                                else if (!IsHealed && spell->Id == SPELL_LESSER_HEAL_R2)
                                {
                                    CasterGUID = caster->GetGUID();
                                    me->SetStandState(UNIT_STAND_STATE_STAND);
                                    Talk(SAY_HEALED, caster);
                                    IsHealed = true;
                                }
                            }
                            break;
                        case ENTRY_DOLF:
                            if (player->GetQuestStatus(QUEST_LIGHT_2) == QUEST_STATUS_INCOMPLETE)
                            {
                                if (IsHealed && !CanRun && spell->Id == SPELL_FORTITUDE_R1)
                                {
                                    Talk(SAY_THANKS, caster);
                                    CanRun = true;
                                }
                                else if (!IsHealed && spell->Id == SPELL_LESSER_HEAL_R2)
                                {
                                    CasterGUID = caster->GetGUID();
                                    me->SetStandState(UNIT_STAND_STATE_STAND);
                                    Talk(SAY_HEALED, caster);
                                    IsHealed = true;
                                }
                            }
                            break;
                        case ENTRY_KORJA:
                            if (player->GetQuestStatus(QUEST_SPIRIT) == QUEST_STATUS_INCOMPLETE)
                            {
                                if (IsHealed && !CanRun && spell->Id == SPELL_FORTITUDE_R1)
                                {
                                    Talk(SAY_THANKS, caster);
                                    CanRun = true;
                                }
                                else if (!IsHealed && spell->Id == SPELL_LESSER_HEAL_R2)
                                {
                                    CasterGUID = caster->GetGUID();
                                    me->SetStandState(UNIT_STAND_STATE_STAND);
                                    Talk(SAY_HEALED, caster);
                                    IsHealed = true;
                                }
                            }
                            break;
                        case ENTRY_DG_KEL:
                            if (player->GetQuestStatus(QUEST_DARKNESS) == QUEST_STATUS_INCOMPLETE)
                            {
                                if (IsHealed && !CanRun && spell->Id == SPELL_FORTITUDE_R1)
                                {
                                    Talk(SAY_THANKS, caster);
                                    CanRun = true;
                                }
                                else if (!IsHealed && spell->Id == SPELL_LESSER_HEAL_R2)
                                {
                                    CasterGUID = caster->GetGUID();
                                    me->SetStandState(UNIT_STAND_STATE_STAND);
                                    Talk(SAY_HEALED, caster);
                                    IsHealed = true;
                                }
                            }
                            break;
                    }

                    // give quest credit, not expect any special quest objectives
                    if (CanRun)
                        player->TalkedToCreature(me->GetEntry(), me->GetGUID());
                }
            }
        }

        void WaypointReached(uint32 /*waypointId*/) override
        {

        }

        void UpdateAI(uint32 diff) override
        {
            if (CanRun && !me->IsInCombat())
            {
                if (RunAwayTimer <= diff)
                {
                    if (Unit* unit = ObjectAccessor::GetUnit(*me, CasterGUID))
                    {
                        switch (me->GetEntry())
                        {
                            case ENTRY_SHAYA:
                            case ENTRY_ROBERTS:
                            case ENTRY_DOLF:
                            case ENTRY_KORJA:
                            case ENTRY_DG_KEL:
                                Talk(SAY_GOODBYE, unit);
                                break;
                            case ENTRY_TAVARA:
                                Talk(SAY_GOODBYE, unit);
                                break; // wrong, but who cares
                        }

                        Start(false, true, ObjectGuid::Empty);
                    }
                    else
                        EnterEvadeMode();                       //something went wrong

                    RunAwayTimer = 30000;
                }
                else
                    RunAwayTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_garments_of_questsAI(creature);
    }
};

/*######
## npc_guardian
######*/

enum GuardianSpells
{
    SPELL_DEATHTOUCH            = 5
};

class npc_guardian : public CreatureScript
{
public:
    npc_guardian() : CreatureScript("npc_guardian") { }

    struct npc_guardianAI : public ScriptedAI
    {
        npc_guardianAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        void EnterCombat(Unit* /*who*/) override
        {
        }

        void UpdateAI(uint32 /*diff*/) override
        {
            if (!UpdateVictim())
                return;

            if (me->isAttackReady())
            {
                DoCastVictim(SPELL_DEATHTOUCH, true);
                me->resetAttackTimer();
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_guardianAI(creature);
    }
};

/*######
## npc_kingdom_of_dalaran_quests
######*/

enum eKingdomDalaran
{
    SPELL_TELEPORT_DALARAN  = 53360,
    ITEM_KT_SIGNET          = 39740,
    QUEST_MAGICAL_KINGDOM_A = 12794,
    QUEST_MAGICAL_KINGDOM_H = 12791,
    QUEST_MAGICAL_KINGDOM_N = 12796
};

#define GOSSIP_ITEM_TELEPORT_TO "I am ready to be teleported to Dalaran."

class npc_kingdom_of_dalaran_quests : public CreatureScript
{
public:
    npc_kingdom_of_dalaran_quests() : CreatureScript("npc_kingdom_of_dalaran_quests") { }
    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (player->HasItemCount(ITEM_KT_SIGNET, 1) && (!player->GetQuestRewardStatus(QUEST_MAGICAL_KINGDOM_A) ||
            !player->GetQuestRewardStatus(QUEST_MAGICAL_KINGDOM_H) || !player->GetQuestRewardStatus(QUEST_MAGICAL_KINGDOM_N)))
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_TELEPORT_TO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*uiSender*/, uint32 uiAction)
    {
        player->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
        {
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, SPELL_TELEPORT_DALARAN, false);
        }
        return true;
    }
};

/*######
## npc_rogue_trainer
######*/

#define GOSSIP_HELLO_ROGUE1 "I wish to unlearn my talents"
#define GOSSIP_HELLO_ROGUE2 "<Take the letter>"
#define GOSSIP_HELLO_ROGUE3 "Purchase a Dual Talent Specialization."

class npc_rogue_trainer : public CreatureScript
{
public:
    npc_rogue_trainer() : CreatureScript("npc_rogue_trainer") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (player->getClass() == CLASS_ROGUE)
        {
            if (creature->IsTrainer())
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, GOSSIP_TEXT_TRAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRAIN);

            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, GOSSIP_HELLO_ROGUE1, GOSSIP_SENDER_MAIN, GOSSIP_OPTION_UNLEARNTALENTS);

            if (player->GetSpecsCount() == 1 && player->getLevel() >= sWorld->getIntConfig(CONFIG_MIN_DUALSPEC_LEVEL))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, GOSSIP_HELLO_ROGUE3, GOSSIP_SENDER_MAIN, GOSSIP_OPTION_LEARNDUALSPEC);

            if (player->getLevel() >= 24 && !player->HasItemCount(17126) && !player->GetQuestRewardStatus(6681))
            {
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HELLO_ROGUE2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                player->SEND_GOSSIP_MENU(5996, creature->GetGUID());
            } else
                player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        }
        else
        {
            player->SEND_GOSSIP_MENU(4796, creature->GetGUID());
        }

        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
                player->CLOSE_GOSSIP_MENU();
                player->CastSpell(player, 21100, false);
                break;
            case GOSSIP_ACTION_TRAIN:
                player->GetSession()->SendTrainerList(creature->GetGUID());
                break;
            case GOSSIP_OPTION_UNLEARNTALENTS:
                player->CLOSE_GOSSIP_MENU();
                player->SendTalentWipeConfirm(creature->GetGUID());
                break;
            case GOSSIP_OPTION_LEARNDUALSPEC:
                if (player->GetSpecsCount() == 1 && !(player->getLevel() < sWorld->getIntConfig(CONFIG_MIN_DUALSPEC_LEVEL)))
                {
                    if (!player->HasEnoughMoney(10000000))
                    {
                        player->SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
                        player->PlayerTalkClass->SendCloseGossip();
                        break;
                    }
                    else
                    {
                        player->ModifyMoney(-10000000, ::RG::Logging::MoneyLogType::DUAL_SPEC);

                        // Cast spells that teach dual spec
                        // Both are also ImplicitTarget self and must be cast by player
                        player->CastSpell(player, 63680, true, NULL, NULL, player->GetGUID());
                        player->CastSpell(player, 63624, true, NULL, NULL, player->GetGUID());

                        // Should show another Gossip text with "Congratulations..."
                        player->PlayerTalkClass->SendCloseGossip();
                    }
                }
                break;
        }
        return true;
    }
};

/*######
## npc_sayge
######*/

enum Sayge
{
    GOSSIP_MENU_OPTION_ID_ANSWER_1   = 0,
    GOSSIP_MENU_OPTION_ID_ANSWER_2   = 1,
    GOSSIP_MENU_OPTION_ID_ANSWER_3   = 2,
    GOSSIP_MENU_OPTION_ID_ANSWER_4   = 3,
    GOSSIP_I_AM_READY_TO_DISCOVER    = 6186,
    GOSSIP_MENU_OPTION_SAYGE1        = 6185,
    GOSSIP_MENU_OPTION_SAYGE2        = 6185,
    GOSSIP_MENU_OPTION_SAYGE3        = 6185,
    GOSSIP_MENU_OPTION_SAYGE4        = 6185,
    GOSSIP_MENU_OPTION_SAYGE5        = 6187,
    GOSSIP_MENU_OPTION_SAYGE6        = 6187,
    GOSSIP_MENU_OPTION_SAYGE7        = 6187,
    GOSSIP_MENU_OPTION_SAYGE8        = 6208,
    GOSSIP_MENU_OPTION_SAYGE9        = 6208,
    GOSSIP_MENU_OPTION_SAYGE10       = 6208,
    GOSSIP_MENU_OPTION_SAYGE11       = 6209,
    GOSSIP_MENU_OPTION_SAYGE12       = 6209,
    GOSSIP_MENU_OPTION_SAYGE13       = 6209,
    GOSSIP_MENU_OPTION_SAYGE14       = 6210,
    GOSSIP_MENU_OPTION_SAYGE15       = 6210,
    GOSSIP_MENU_OPTION_SAYGE16       = 6210,
    GOSSIP_MENU_OPTION_SAYGE17       = 6211,
    GOSSIP_MENU_I_HAVE_LONG_KNOWN    = 7339,
    GOSSIP_MENU_YOU_HAVE_BEEN_TASKED = 7340,
    GOSSIP_MENU_SWORN_EXECUTIONER    = 7341,
    GOSSIP_MENU_DIPLOMATIC_MISSION   = 7361,
    GOSSIP_MENU_YOUR_BROTHER_SEEKS   = 7362,
    GOSSIP_MENU_A_TERRIBLE_BEAST     = 7363,
    GOSSIP_MENU_YOUR_FORTUNE_IS_CAST = 7364,
    GOSSIP_MENU_HERE_IS_YOUR_FORTUNE = 7365,
    GOSSIP_MENU_CANT_GIVE_YOU_YOUR   = 7393,

    SPELL_STRENGTH                   = 23735, // +10% Strength
    SPELL_AGILITY                    = 23736, // +10% Agility
    SPELL_STAMINA                    = 23737, // +10% Stamina
    SPELL_SPIRIT                     = 23738, // +10% Spirit
    SPELL_INTELLECT                  = 23766, // +10% Intellect
    SPELL_ARMOR                      = 23767, // +10% Armor
    SPELL_DAMAGE                     = 23768, // +10% Damage
    SPELL_RESISTANCE                 = 23769, // +25 Magic Resistance (All)
    SPELL_FORTUNE                    = 23765  // Darkmoon Faire Fortune
};

class npc_sayge : public CreatureScript
{
public:
    npc_sayge() : CreatureScript("npc_sayge") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (player->GetSpellHistory()->HasCooldown(SPELL_INTELLECT) ||
            player->GetSpellHistory()->HasCooldown(SPELL_ARMOR) ||
            player->GetSpellHistory()->HasCooldown(SPELL_DAMAGE) ||
            player->GetSpellHistory()->HasCooldown(SPELL_RESISTANCE) ||
            player->GetSpellHistory()->HasCooldown(SPELL_STRENGTH) ||
            player->GetSpellHistory()->HasCooldown(SPELL_AGILITY) ||
            player->GetSpellHistory()->HasCooldown(SPELL_STAMINA) ||
            player->GetSpellHistory()->HasCooldown(SPELL_SPIRIT))
            player->SEND_GOSSIP_MENU(GOSSIP_MENU_CANT_GIVE_YOU_YOUR, creature->GetGUID());
        else
        {
            player->ADD_GOSSIP_ITEM_DB(GOSSIP_I_AM_READY_TO_DISCOVER, GOSSIP_MENU_OPTION_ID_ANSWER_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            player->SEND_GOSSIP_MENU(GOSSIP_MENU_I_HAVE_LONG_KNOWN, creature->GetGUID());
        }

        return true;
    }

    void SendAction(Player* player, Creature* creature, uint32 action)
    {
        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE1, GOSSIP_MENU_OPTION_ID_ANSWER_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE2, GOSSIP_MENU_OPTION_ID_ANSWER_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE3, GOSSIP_MENU_OPTION_ID_ANSWER_3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE4, GOSSIP_MENU_OPTION_ID_ANSWER_4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
                player->SEND_GOSSIP_MENU(GOSSIP_MENU_YOU_HAVE_BEEN_TASKED, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF + 2:
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE5, GOSSIP_MENU_OPTION_ID_ANSWER_1, GOSSIP_SENDER_MAIN + 1, GOSSIP_ACTION_INFO_DEF);
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE6, GOSSIP_MENU_OPTION_ID_ANSWER_2, GOSSIP_SENDER_MAIN + 2, GOSSIP_ACTION_INFO_DEF);
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE7, GOSSIP_MENU_OPTION_ID_ANSWER_3, GOSSIP_SENDER_MAIN + 3, GOSSIP_ACTION_INFO_DEF);
                player->SEND_GOSSIP_MENU(GOSSIP_MENU_SWORN_EXECUTIONER, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF + 3:
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE8, GOSSIP_MENU_OPTION_ID_ANSWER_1, GOSSIP_SENDER_MAIN + 4, GOSSIP_ACTION_INFO_DEF);
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE9, GOSSIP_MENU_OPTION_ID_ANSWER_2, GOSSIP_SENDER_MAIN + 5, GOSSIP_ACTION_INFO_DEF);
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE10,GOSSIP_MENU_OPTION_ID_ANSWER_3, GOSSIP_SENDER_MAIN + 2, GOSSIP_ACTION_INFO_DEF);
                player->SEND_GOSSIP_MENU(GOSSIP_MENU_DIPLOMATIC_MISSION, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF + 4:
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE11, GOSSIP_MENU_OPTION_ID_ANSWER_1, GOSSIP_SENDER_MAIN + 6, GOSSIP_ACTION_INFO_DEF);
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE12, GOSSIP_MENU_OPTION_ID_ANSWER_2, GOSSIP_SENDER_MAIN + 7, GOSSIP_ACTION_INFO_DEF);
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE13, GOSSIP_MENU_OPTION_ID_ANSWER_3, GOSSIP_SENDER_MAIN + 8, GOSSIP_ACTION_INFO_DEF);
                player->SEND_GOSSIP_MENU(GOSSIP_MENU_YOUR_BROTHER_SEEKS, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF + 5:
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE14, GOSSIP_MENU_OPTION_ID_ANSWER_1, GOSSIP_SENDER_MAIN + 5, GOSSIP_ACTION_INFO_DEF);
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE15, GOSSIP_MENU_OPTION_ID_ANSWER_2, GOSSIP_SENDER_MAIN + 4, GOSSIP_ACTION_INFO_DEF);
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE16, GOSSIP_MENU_OPTION_ID_ANSWER_3, GOSSIP_SENDER_MAIN + 3, GOSSIP_ACTION_INFO_DEF);
                player->SEND_GOSSIP_MENU(GOSSIP_MENU_A_TERRIBLE_BEAST, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF:
                player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_OPTION_SAYGE17, GOSSIP_MENU_OPTION_ID_ANSWER_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);
                player->SEND_GOSSIP_MENU(GOSSIP_MENU_YOUR_FORTUNE_IS_CAST, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF + 6:
                creature->CastSpell(player, SPELL_FORTUNE, false);
                player->SEND_GOSSIP_MENU(GOSSIP_MENU_HERE_IS_YOUR_FORTUNE, creature->GetGUID());
                break;
        }
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        uint32 spellId = 0;
        switch (sender)
        {
            case GOSSIP_SENDER_MAIN:
                SendAction(player, creature, action);
                break;
            case GOSSIP_SENDER_MAIN + 1:
                spellId = SPELL_DAMAGE;
                break;
            case GOSSIP_SENDER_MAIN + 2:
                spellId = SPELL_RESISTANCE;
                break;
            case GOSSIP_SENDER_MAIN + 3:
                spellId = SPELL_ARMOR;
                break;
            case GOSSIP_SENDER_MAIN + 4:
                spellId = SPELL_SPIRIT;
                break;
            case GOSSIP_SENDER_MAIN + 5:
                spellId = SPELL_INTELLECT;
                break;
            case GOSSIP_SENDER_MAIN + 6:
                spellId = SPELL_STAMINA;
                break;
            case GOSSIP_SENDER_MAIN + 7:
                spellId = SPELL_STRENGTH;
                break;
            case GOSSIP_SENDER_MAIN + 8:
                spellId = SPELL_AGILITY;
                break;
        }

        if (spellId)
        {
            creature->CastSpell(player, spellId, false);
            player->GetSpellHistory()->AddCooldown(spellId, 0, std::chrono::hours(2));
            SendAction(player, creature, action);
        }
        return true;
    }
};

class npc_steam_tonk : public CreatureScript
{
public:
    npc_steam_tonk() : CreatureScript("npc_steam_tonk") { }

    struct npc_steam_tonkAI : public ScriptedAI
    {
        npc_steam_tonkAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override { }
        void EnterCombat(Unit* /*who*/) override { }

        void OnPossess(bool apply)
        {
            if (apply)
            {
                // Initialize the action bar without the melee attack command
                me->InitCharmInfo();
                me->GetCharmInfo()->InitEmptyActionBar(false);

                me->SetReactState(REACT_PASSIVE);
            }
            else
                me->SetReactState(REACT_AGGRESSIVE);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_steam_tonkAI(creature);
    }
};

enum TonkMine
{
    SPELL_TONK_MINE_DETONATE    = 25099
};

class npc_tonk_mine : public CreatureScript
{
public:
    npc_tonk_mine() : CreatureScript("npc_tonk_mine") { }

    struct npc_tonk_mineAI : public ScriptedAI
    {
        npc_tonk_mineAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
        }

        uint32 ExplosionTimer;

        void Reset() override
        {
            ExplosionTimer = 3000;
        }

        void EnterCombat(Unit* /*who*/) override { }
        void AttackStart(Unit* /*who*/) override { }
        void MoveInLineOfSight(Unit* /*who*/) override { }


        void UpdateAI(uint32 diff) override
        {
            if (ExplosionTimer <= diff)
            {
                DoCastSelf(SPELL_TONK_MINE_DETONATE, true);
                me->setDeathState(DEAD); // unsummon it
            }
            else
                ExplosionTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_tonk_mineAI(creature);
    }
};

enum TrainingDummy
{
    NPC_ADVANCED_TARGET_DUMMY = 2674,
    NPC_TARGET_DUMMY          = 2673,
    EVENT_TD_CHECK_COMBAT     = 1,
    EVENT_TD_DESPAWN          = 2
};

class npc_training_dummy : public CreatureScript
{
public:
    npc_training_dummy() : CreatureScript("npc_training_dummy") { }

    struct npc_training_dummyAI : ScriptedAI
    {
        npc_training_dummyAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
        }

        EventMap _events;
        uint32 ResetHealthTimer;
        std::unordered_map<ObjectGuid, time_t> _damageTimes;

        void Reset() override
        {
            me->SetControlled(true, UNIT_STATE_STUNNED);//disable rotate

            _events.Reset();
            _damageTimes.clear();
            ResetHealthTimer = 5 * IN_MILLISECONDS;
            if (me->GetEntry() != NPC_ADVANCED_TARGET_DUMMY && me->GetEntry() != NPC_TARGET_DUMMY)
                _events.ScheduleEvent(EVENT_TD_CHECK_COMBAT, 1 * IN_MILLISECONDS);
            else
                _events.ScheduleEvent(EVENT_TD_DESPAWN, 15 * IN_MILLISECONDS);
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            if (!_EnterEvadeMode())
            return;

            Reset();
        }

        void DamageTaken(Unit* doneBy, uint32& damage) override
        {
            AddThreat(doneBy, float(damage)); // just to create threat reference
            _damageTimes[doneBy->GetGUID()] = time(NULL);
        }

        void UpdateAI(uint32 diff) override
        {
            if (ResetHealthTimer <= diff)
            {
                me->SetHealth(me->GetMaxHealth());
                ResetHealthTimer = 5 * IN_MILLISECONDS;
            }
            else
                ResetHealthTimer -= diff;

            if (!me->IsInCombat())
                return;

            if (!me->HasUnitState(UNIT_STATE_STUNNED))
                me->SetControlled(true, UNIT_STATE_STUNNED); //disable rotate

            _events.Update(diff);

            if (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_TD_CHECK_COMBAT:
                {
                    time_t now = time(NULL);
                    for (std::unordered_map<ObjectGuid, time_t>::iterator itr = _damageTimes.begin(); itr != _damageTimes.end();)
                    {
                        // If unit has not dealt damage to training dummy for 5 seconds, remove him from combat
                        if (itr->second < now - 5)
                        {
                            if (Unit* unit = ObjectAccessor::GetUnit(*me, itr->first))
                                unit->getHostileRefManager().deleteReference(me);
                            itr = _damageTimes.erase(itr);
                        }
                        else
                            ++itr;
                    }
                    _events.ScheduleEvent(EVENT_TD_CHECK_COMBAT, 1 * IN_MILLISECONDS);
                    break;
                }
                case EVENT_TD_DESPAWN:
                    me->DespawnOrUnsummon(1);
                    break;
                default:
                    break;
                }
            }
        }
        void MoveInLineOfSight(Unit* /*who*/) override{}

    };
    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_training_dummyAI(creature);
    }
};

/*######
# npc_wormhole
######*/

enum WormholeMisc
{
    SPELL_BOREAN_TUNDRA         = 67834,
    SPELL_SHOLAZAR_BASIN        = 67835,
    SPELL_ICECROWN              = 67836,
    SPELL_STORM_PEAKS           = 67837,
    SPELL_HOWLING_FJORD         = 67838,
    SPELL_UNDERGROUND           = 68081,

    TEXT_WORMHOLE               = 907,

    DATA_SHOW_UNDERGROUND       = 1,
    GOSSIP_WORMHOLE             = 60081
};

class npc_wormhole : public CreatureScript
{
    public:
        npc_wormhole() : CreatureScript("npc_wormhole") { }

        struct npc_wormholeAI : public PassiveAI
        {
            npc_wormholeAI(Creature* creature) : PassiveAI(creature) { }

            void InitializeAI() override
            {
                _showUnderground = urand(0, 100) == 0; // Guessed value, it is really rare though
            }

            uint32 GetData(uint32 type) const override
            {
                return (type == DATA_SHOW_UNDERGROUND && _showUnderground) ? 1 : 0;
            }

        private:
            bool _showUnderground;
        };

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            if (creature->IsSummon())
            {
                if (player == creature->ToTempSummon()->GetSummoner())
                {
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_WORMHOLE, 0), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_WORMHOLE, 1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_WORMHOLE, 2), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_WORMHOLE, 3), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_WORMHOLE, 4), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);

                    if (creature->AI()->GetData(DATA_SHOW_UNDERGROUND))
                        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_WORMHOLE, 5), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);

                    player->PlayerTalkClass->SendGossipMenu(TEXT_WORMHOLE, creature->GetGUID());
                }
            }

            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
        {
            player->PlayerTalkClass->ClearMenus();

            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF + 1: // Borean Tundra
                    player->CLOSE_GOSSIP_MENU();
                    creature->CastSpell(player, SPELL_BOREAN_TUNDRA, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 2: // Howling Fjord
                    player->CLOSE_GOSSIP_MENU();
                    creature->CastSpell(player, SPELL_HOWLING_FJORD, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 3: // Sholazar Basin
                    player->CLOSE_GOSSIP_MENU();
                    creature->CastSpell(player, SPELL_SHOLAZAR_BASIN, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 4: // Icecrown
                    player->CLOSE_GOSSIP_MENU();
                    creature->CastSpell(player, SPELL_ICECROWN, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 5: // Storm peaks
                    player->CLOSE_GOSSIP_MENU();
                    creature->CastSpell(player, SPELL_STORM_PEAKS, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 6: // Underground
                    player->CLOSE_GOSSIP_MENU();
                    creature->CastSpell(player, SPELL_UNDERGROUND, false);
                    break;
            }

            return true;
        }

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_wormholeAI(creature);
        }
};

/*######
## npc_pet_trainer
######*/

enum PetTrainer
{
    TEXT_ISHUNTER               = 5838,
    TEXT_NOTHUNTER              = 5839,
    TEXT_PETINFO                = 13474,
    TEXT_CONFIRM                = 7722
};

#define GOSSIP_PET1             "How do I train my pet?"
#define GOSSIP_PET2             "I wish to untrain my pet."
#define GOSSIP_PET_CONFIRM      "Yes, please do."

class npc_pet_trainer : public CreatureScript
{
public:
    npc_pet_trainer() : CreatureScript("npc_pet_trainer") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (player->getClass() == CLASS_HUNTER)
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_PET1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            if (player->GetPet() && player->GetPet()->getPetType() == HUNTER_PET)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_PET2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);

            player->PlayerTalkClass->SendGossipMenu(TEXT_ISHUNTER, creature->GetGUID());
            return true;
        }
        player->PlayerTalkClass->SendGossipMenu(TEXT_NOTHUNTER, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
                player->PlayerTalkClass->SendGossipMenu(TEXT_PETINFO, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF + 2:
                {
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_PET_CONFIRM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                    player->PlayerTalkClass->SendGossipMenu(TEXT_CONFIRM, creature->GetGUID());
                }
                break;
            case GOSSIP_ACTION_INFO_DEF + 3:
                {
                    player->ResetPetTalents();
                    player->CLOSE_GOSSIP_MENU();
                }
                break;
        }
        return true;
    }
};

/*######
## npc_tabard_vendor
######*/

enum
{
    QUEST_TRUE_MASTERS_OF_LIGHT = 9737,
    QUEST_THE_UNWRITTEN_PROPHECY = 9762,
    QUEST_INTO_THE_BREACH = 10259,
    QUEST_BATTLE_OF_THE_CRIMSON_WATCH = 10781,
    QUEST_SHARDS_OF_AHUNE = 11972,

    ACHIEVEMENT_EXPLORE_NORTHREND = 45,
    ACHIEVEMENT_TWENTYFIVE_TABARDS = 1021,
    ACHIEVEMENT_THE_LOREMASTER_A = 1681,
    ACHIEVEMENT_THE_LOREMASTER_H = 1682,

    ITEM_TABARD_OF_THE_HAND = 24344,
    ITEM_TABARD_OF_THE_BLOOD_KNIGHT = 25549,
    ITEM_TABARD_OF_THE_PROTECTOR = 28788,
    ITEM_OFFERING_OF_THE_SHATAR = 31408,
    ITEM_GREEN_TROPHY_TABARD_OF_THE_ILLIDARI = 31404,
    ITEM_PURPLE_TROPHY_TABARD_OF_THE_ILLIDARI = 31405,
    ITEM_TABARD_OF_THE_SUMMER_SKIES = 35279,
    ITEM_TABARD_OF_THE_SUMMER_FLAMES = 35280,
    ITEM_TABARD_OF_THE_ACHIEVER = 40643,
    ITEM_LOREMASTERS_COLORS = 43300,
    ITEM_TABARD_OF_THE_EXPLORER = 43348,

    SPELL_TABARD_OF_THE_BLOOD_KNIGHT = 54974,
    SPELL_TABARD_OF_THE_HAND = 54976,
    SPELL_GREEN_TROPHY_TABARD_OF_THE_ILLIDARI = 54977,
    SPELL_PURPLE_TROPHY_TABARD_OF_THE_ILLIDARI = 54982,
    SPELL_TABARD_OF_THE_ACHIEVER = 55006,
    SPELL_TABARD_OF_THE_PROTECTOR = 55008,
    SPELL_LOREMASTERS_COLORS = 58194,
    SPELL_TABARD_OF_THE_EXPLORER = 58224,
    SPELL_TABARD_OF_SUMMER_SKIES = 62768,
    SPELL_TABARD_OF_SUMMER_FLAMES = 62769
};

#define GOSSIP_LOST_TABARD_OF_BLOOD_KNIGHT "I've lost my Tabard of Blood Knight."
#define GOSSIP_LOST_TABARD_OF_THE_HAND "I've lost my Tabard of the Hand."
#define GOSSIP_LOST_TABARD_OF_THE_PROTECTOR "I've lost my Tabard of the Protector."
#define GOSSIP_LOST_GREEN_TROPHY_TABARD_OF_THE_ILLIDARI "I've lost my Green Trophy Tabard of the Illidari."
#define GOSSIP_LOST_PURPLE_TROPHY_TABARD_OF_THE_ILLIDARI "I've lost my Purple Trophy Tabard of the Illidari."
#define GOSSIP_LOST_TABARD_OF_SUMMER_SKIES "I've lost my Tabard of Summer Skies."
#define GOSSIP_LOST_TABARD_OF_SUMMER_FLAMES "I've lost my Tabard of Summer Flames."
#define GOSSIP_LOST_LOREMASTERS_COLORS "I've lost my Loremaster's Colors."
#define GOSSIP_LOST_TABARD_OF_THE_EXPLORER "I've lost my Tabard of the Explorer."
#define GOSSIP_LOST_TABARD_OF_THE_ACHIEVER "I've lost my Tabard of the Achiever."

class npc_tabard_vendor : public CreatureScript
{
public:
    npc_tabard_vendor() : CreatureScript("npc_tabard_vendor") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        bool lostBloodKnight = false;
        bool lostHand = false;
        bool lostProtector = false;
        bool lostIllidari = false;
        bool lostSummer = false;
        bool lostExplorer = false;
        bool lostAchiever = false;
        bool lostLoremaster = false;

        //Tabard of the Blood Knight
        if (player->GetQuestRewardStatus(QUEST_TRUE_MASTERS_OF_LIGHT) && !player->HasItemCount(ITEM_TABARD_OF_THE_BLOOD_KNIGHT, 1, true))
            lostBloodKnight = true;

        //Tabard of the Hand
        if (player->GetQuestRewardStatus(QUEST_THE_UNWRITTEN_PROPHECY) && !player->HasItemCount(ITEM_TABARD_OF_THE_HAND, 1, true))
            lostHand = true;

        //Tabard of the Protector
        if (player->GetQuestRewardStatus(QUEST_INTO_THE_BREACH) && !player->HasItemCount(ITEM_TABARD_OF_THE_PROTECTOR, 1, true))
            lostProtector = true;

        //Green Trophy Tabard of the Illidari
        //Purple Trophy Tabard of the Illidari
        if (player->GetQuestRewardStatus(QUEST_BATTLE_OF_THE_CRIMSON_WATCH) &&
            (!player->HasItemCount(ITEM_GREEN_TROPHY_TABARD_OF_THE_ILLIDARI, 1, true) &&
            !player->HasItemCount(ITEM_PURPLE_TROPHY_TABARD_OF_THE_ILLIDARI, 1, true) &&
            !player->HasItemCount(ITEM_OFFERING_OF_THE_SHATAR, 1, true)))
            lostIllidari = true;

        //Tabard of Summer Skies
        //Tabard of Summer Flames
        if (player->GetQuestRewardStatus(QUEST_SHARDS_OF_AHUNE) &&
            !player->HasItemCount(ITEM_TABARD_OF_THE_SUMMER_SKIES, 1, true) &&
            !player->HasItemCount(ITEM_TABARD_OF_THE_SUMMER_FLAMES, 1, true))
            lostSummer = true;

        //Tabard of the Explorer
        if (player->HasAchieved(ACHIEVEMENT_EXPLORE_NORTHREND) &&
            !player->HasItemCount(ITEM_TABARD_OF_THE_EXPLORER, 1, true))
            lostExplorer = true;

        //Tabard of the Achiever
        if (player->HasAchieved(ACHIEVEMENT_TWENTYFIVE_TABARDS) &&
            !player->HasItemCount(ITEM_TABARD_OF_THE_ACHIEVER, 1, true))
            lostAchiever = true;

        //Loremaster's Colors
        if ((player->HasAchieved(ACHIEVEMENT_THE_LOREMASTER_A) || player->HasAchieved(ACHIEVEMENT_THE_LOREMASTER_H))
            && !player->HasItemCount(ITEM_LOREMASTERS_COLORS, 1, true))
            lostLoremaster = true;


        if (lostBloodKnight || lostHand || lostProtector || lostIllidari || lostSummer || lostExplorer || lostAchiever || lostLoremaster)
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, player->GetOptionTextWithEntry(GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_TEXT_BROWSE_GOODS_ID), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);

            if (lostBloodKnight)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_TABARD_OF_BLOOD_KNIGHT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

            if (lostHand)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_TABARD_OF_THE_HAND, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);

            if (lostProtector)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_TABARD_OF_THE_PROTECTOR, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);

            if (lostIllidari)
            {
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_GREEN_TROPHY_TABARD_OF_THE_ILLIDARI, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_PURPLE_TROPHY_TABARD_OF_THE_ILLIDARI, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
            }

            if (lostSummer)
            {
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_TABARD_OF_SUMMER_SKIES, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_TABARD_OF_SUMMER_FLAMES, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);
            }

            if (lostExplorer)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_TABARD_OF_THE_EXPLORER, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 8);

            if (lostAchiever)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_TABARD_OF_THE_ACHIEVER, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 9);

            if(lostLoremaster)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_LOREMASTERS_COLORS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 10);

            player->SEND_GOSSIP_MENU(13583, creature->GetGUID());
        }
        else
            player->GetSession()->SendListInventory(creature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        switch (action)
        {
            case GOSSIP_ACTION_TRADE:
                player->GetSession()->SendListInventory(creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF + 1:
                player->CLOSE_GOSSIP_MENU();
                player->CastSpell(player, SPELL_TABARD_OF_THE_BLOOD_KNIGHT, false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 2:
                player->CLOSE_GOSSIP_MENU();
                player->CastSpell(player, SPELL_TABARD_OF_THE_HAND, false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 3:
                player->CLOSE_GOSSIP_MENU();
                player->CastSpell(player, SPELL_TABARD_OF_THE_PROTECTOR, false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 4:
                player->CLOSE_GOSSIP_MENU();
                player->CastSpell(player, SPELL_GREEN_TROPHY_TABARD_OF_THE_ILLIDARI, false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 5:
                player->CLOSE_GOSSIP_MENU();
                player->CastSpell(player, SPELL_PURPLE_TROPHY_TABARD_OF_THE_ILLIDARI, false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 6:
                player->CLOSE_GOSSIP_MENU();
                player->CastSpell(player, SPELL_TABARD_OF_SUMMER_SKIES, false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 7:
                player->CLOSE_GOSSIP_MENU();
                player->CastSpell(player, SPELL_TABARD_OF_SUMMER_FLAMES, false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 8:
                player->CLOSE_GOSSIP_MENU();
                player->CastSpell(player, SPELL_TABARD_OF_THE_EXPLORER, false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 9:
                player->CLOSE_GOSSIP_MENU();
                player->CastSpell(player, SPELL_TABARD_OF_THE_ACHIEVER, false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 10:
                player->CLOSE_GOSSIP_MENU();
                player->CastSpell(player, SPELL_LOREMASTERS_COLORS, false);
                break;
        }
        return true;
    }
};

/*######
## npc_experience
######*/

#define EXP_COST                100000 //10 00 00 copper (10golds)
#define GOSSIP_TEXT_EXP         14736

class npc_experience : public CreatureScript
{
public:
    npc_experience() : CreatureScript("npc_experience") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN))
            player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(10638, 0), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1, player->GetOptionBoxTextWithEntry(10638, 0), player->GetOptionBoxMoneyWithEntry(10638, 0), false);
        else
            player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(10638, 1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2, player->GetOptionBoxTextWithEntry(10638, 1), player->GetOptionBoxMoneyWithEntry(10638, 1), false);
        
        if (!player->HasCustomFlag(Player::XP_NORMAL))
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(10638, 2), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        else
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(10638, 3), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
        
        player->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_EXP, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();

        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF + 1://xp off
                if (!player->HasEnoughMoney(EXP_COST))
                    player->SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
                else 
                {
                    player->ModifyMoney(-EXP_COST);
                    player->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN);
                }
                break;
            case GOSSIP_ACTION_INFO_DEF + 2://xp off
                if (!player->HasEnoughMoney(EXP_COST))
                    player->SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
                else 
                {
                    player->ModifyMoney(-EXP_COST);
                    player->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN);
                }
                break;
            case GOSSIP_ACTION_INFO_DEF + 3: // double quest xp off
                player->SetCustomFlag(Player::XP_NORMAL);
                break;
            case GOSSIP_ACTION_INFO_DEF + 4: // double quest xp on
                player->RemoveCustomFlag(Player::XP_NORMAL);
                break;
        }
        player->PlayerTalkClass->SendCloseGossip();
        return true;
    }
};

enum Fireworks
{
    NPC_OMEN                = 15467,
    NPC_MINION_OF_OMEN      = 15466,
    NPC_FIREWORK_BLUE       = 15879,
    NPC_FIREWORK_GREEN      = 15880,
    NPC_FIREWORK_PURPLE     = 15881,
    NPC_FIREWORK_RED        = 15882,
    NPC_FIREWORK_YELLOW     = 15883,
    NPC_FIREWORK_WHITE      = 15884,
    NPC_FIREWORK_BIG_BLUE   = 15885,
    NPC_FIREWORK_BIG_GREEN  = 15886,
    NPC_FIREWORK_BIG_PURPLE = 15887,
    NPC_FIREWORK_BIG_RED    = 15888,
    NPC_FIREWORK_BIG_YELLOW = 15889,
    NPC_FIREWORK_BIG_WHITE  = 15890,

    NPC_CLUSTER_BLUE        = 15872,
    NPC_CLUSTER_RED         = 15873,
    NPC_CLUSTER_GREEN       = 15874,
    NPC_CLUSTER_PURPLE      = 15875,
    NPC_CLUSTER_WHITE       = 15876,
    NPC_CLUSTER_YELLOW      = 15877,
    NPC_CLUSTER_BIG_BLUE    = 15911,
    NPC_CLUSTER_BIG_GREEN   = 15912,
    NPC_CLUSTER_BIG_PURPLE  = 15913,
    NPC_CLUSTER_BIG_RED     = 15914,
    NPC_CLUSTER_BIG_WHITE   = 15915,
    NPC_CLUSTER_BIG_YELLOW  = 15916,
    NPC_CLUSTER_ELUNE       = 15918,

    GO_FIREWORK_LAUNCHER_1  = 180771,
    GO_FIREWORK_LAUNCHER_2  = 180868,
    GO_FIREWORK_LAUNCHER_3  = 180850,
    GO_CLUSTER_LAUNCHER_1   = 180772,
    GO_CLUSTER_LAUNCHER_2   = 180859,
    GO_CLUSTER_LAUNCHER_3   = 180869,
    GO_CLUSTER_LAUNCHER_4   = 180874,

    SPELL_ROCKET_BLUE       = 26344,
    SPELL_ROCKET_GREEN      = 26345,
    SPELL_ROCKET_PURPLE     = 26346,
    SPELL_ROCKET_RED        = 26347,
    SPELL_ROCKET_WHITE      = 26348,
    SPELL_ROCKET_YELLOW     = 26349,
    SPELL_ROCKET_BIG_BLUE   = 26351,
    SPELL_ROCKET_BIG_GREEN  = 26352,
    SPELL_ROCKET_BIG_PURPLE = 26353,
    SPELL_ROCKET_BIG_RED    = 26354,
    SPELL_ROCKET_BIG_WHITE  = 26355,
    SPELL_ROCKET_BIG_YELLOW = 26356,
    SPELL_LUNAR_FORTUNE     = 26522,

    ANIM_GO_LAUNCH_FIREWORK = 3,
    ZONE_MOONGLADE          = 493,
};

Position omenSummonPos = {7558.993f, -2839.999f, 450.0214f, 4.46f};

class npc_firework : public CreatureScript
{
public:
    npc_firework() : CreatureScript("npc_firework") { }

    struct npc_fireworkAI : public ScriptedAI
    {
        npc_fireworkAI(Creature* creature) : ScriptedAI(creature) { }

        bool isCluster()
        {
            switch (me->GetEntry())
            {
                case NPC_FIREWORK_BLUE:
                case NPC_FIREWORK_GREEN:
                case NPC_FIREWORK_PURPLE:
                case NPC_FIREWORK_RED:
                case NPC_FIREWORK_YELLOW:
                case NPC_FIREWORK_WHITE:
                case NPC_FIREWORK_BIG_BLUE:
                case NPC_FIREWORK_BIG_GREEN:
                case NPC_FIREWORK_BIG_PURPLE:
                case NPC_FIREWORK_BIG_RED:
                case NPC_FIREWORK_BIG_YELLOW:
                case NPC_FIREWORK_BIG_WHITE:
                    return false;
                case NPC_CLUSTER_BLUE:
                case NPC_CLUSTER_GREEN:
                case NPC_CLUSTER_PURPLE:
                case NPC_CLUSTER_RED:
                case NPC_CLUSTER_YELLOW:
                case NPC_CLUSTER_WHITE:
                case NPC_CLUSTER_BIG_BLUE:
                case NPC_CLUSTER_BIG_GREEN:
                case NPC_CLUSTER_BIG_PURPLE:
                case NPC_CLUSTER_BIG_RED:
                case NPC_CLUSTER_BIG_YELLOW:
                case NPC_CLUSTER_BIG_WHITE:
                case NPC_CLUSTER_ELUNE:
                default:
                    return true;
            }
        }

        GameObject* FindNearestLauncher()
        {
            GameObject* launcher = NULL;

            if (isCluster())
            {
                GameObject* launcher1 = me->FindNearestGameObject(GO_CLUSTER_LAUNCHER_1, 0.5f);
                GameObject* launcher2 = me->FindNearestGameObject(GO_CLUSTER_LAUNCHER_2, 0.5f);
                GameObject* launcher3 = me->FindNearestGameObject(GO_CLUSTER_LAUNCHER_3, 0.5f);
                GameObject* launcher4 = me->FindNearestGameObject(GO_CLUSTER_LAUNCHER_4, 0.5f);

                if (launcher1)
                    launcher = launcher1;
                else if (launcher2)
                    launcher = launcher2;
                else if (launcher3)
                    launcher = launcher3;
                else if (launcher4)
                    launcher = launcher4;
            }
            else
            {
                GameObject* launcher1 = me->FindNearestGameObject(GO_FIREWORK_LAUNCHER_1, 0.5f);
                GameObject* launcher2 = me->FindNearestGameObject(GO_FIREWORK_LAUNCHER_2, 0.5f);
                GameObject* launcher3 = me->FindNearestGameObject(GO_FIREWORK_LAUNCHER_3, 0.5f);

                if (launcher1)
                    launcher = launcher1;
                else if (launcher2)
                    launcher = launcher2;
                else if (launcher3)
                    launcher = launcher3;
            }

            return launcher;
        }

        uint32 GetFireworkSpell(uint32 entry)
        {
            switch (entry)
            {
                case NPC_FIREWORK_BLUE:
                    return SPELL_ROCKET_BLUE;
                case NPC_FIREWORK_GREEN:
                    return SPELL_ROCKET_GREEN;
                case NPC_FIREWORK_PURPLE:
                    return SPELL_ROCKET_PURPLE;
                case NPC_FIREWORK_RED:
                    return SPELL_ROCKET_RED;
                case NPC_FIREWORK_YELLOW:
                    return SPELL_ROCKET_YELLOW;
                case NPC_FIREWORK_WHITE:
                    return SPELL_ROCKET_WHITE;
                case NPC_FIREWORK_BIG_BLUE:
                    return SPELL_ROCKET_BIG_BLUE;
                case NPC_FIREWORK_BIG_GREEN:
                    return SPELL_ROCKET_BIG_GREEN;
                case NPC_FIREWORK_BIG_PURPLE:
                    return SPELL_ROCKET_BIG_PURPLE;
                case NPC_FIREWORK_BIG_RED:
                    return SPELL_ROCKET_BIG_RED;
                case NPC_FIREWORK_BIG_YELLOW:
                    return SPELL_ROCKET_BIG_YELLOW;
                case NPC_FIREWORK_BIG_WHITE:
                    return SPELL_ROCKET_BIG_WHITE;
                default:
                    return 0;
            }
        }

        uint32 GetFireworkGameObjectId()
        {
            uint32 spellId = 0;

            switch (me->GetEntry())
            {
                case NPC_CLUSTER_BLUE:
                    spellId = GetFireworkSpell(NPC_FIREWORK_BLUE);
                    break;
                case NPC_CLUSTER_GREEN:
                    spellId = GetFireworkSpell(NPC_FIREWORK_GREEN);
                    break;
                case NPC_CLUSTER_PURPLE:
                    spellId = GetFireworkSpell(NPC_FIREWORK_PURPLE);
                    break;
                case NPC_CLUSTER_RED:
                    spellId = GetFireworkSpell(NPC_FIREWORK_RED);
                    break;
                case NPC_CLUSTER_YELLOW:
                    spellId = GetFireworkSpell(NPC_FIREWORK_YELLOW);
                    break;
                case NPC_CLUSTER_WHITE:
                    spellId = GetFireworkSpell(NPC_FIREWORK_WHITE);
                    break;
                case NPC_CLUSTER_BIG_BLUE:
                    spellId = GetFireworkSpell(NPC_FIREWORK_BIG_BLUE);
                    break;
                case NPC_CLUSTER_BIG_GREEN:
                    spellId = GetFireworkSpell(NPC_FIREWORK_BIG_GREEN);
                    break;
                case NPC_CLUSTER_BIG_PURPLE:
                    spellId = GetFireworkSpell(NPC_FIREWORK_BIG_PURPLE);
                    break;
                case NPC_CLUSTER_BIG_RED:
                    spellId = GetFireworkSpell(NPC_FIREWORK_BIG_RED);
                    break;
                case NPC_CLUSTER_BIG_YELLOW:
                    spellId = GetFireworkSpell(NPC_FIREWORK_BIG_YELLOW);
                    break;
                case NPC_CLUSTER_BIG_WHITE:
                    spellId = GetFireworkSpell(NPC_FIREWORK_BIG_WHITE);
                    break;
                case NPC_CLUSTER_ELUNE:
                    spellId = GetFireworkSpell(urand(NPC_FIREWORK_BLUE, NPC_FIREWORK_WHITE));
                    break;
            }

            const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellId);

            if (spellInfo && spellInfo->Effects[0].Effect == SPELL_EFFECT_SUMMON_OBJECT_WILD)
                return spellInfo->Effects[0].MiscValue;

            return 0;
        }

        void Reset() override
        {
            if (GameObject* launcher = FindNearestLauncher())
            {
                launcher->SendCustomAnim(ANIM_GO_LAUNCH_FIREWORK);
                me->SetOrientation(launcher->GetOrientation() + float(M_PI) / 2);
            }
            else
                return;

            if (isCluster())
            {
                // Check if we are near Elune'ara lake south, if so try to summon Omen or a minion
                if (me->GetZoneId() == ZONE_MOONGLADE)
                {
                    if (!me->FindNearestCreature(NPC_OMEN, 100.0f) && me->GetDistance2d(omenSummonPos.GetPositionX(), omenSummonPos.GetPositionY()) <= 100.0f)
                    {
                        switch (urand(0, 9))
                        {
                            case 0:
                            case 1:
                            case 2:
                            case 3:
                                if (Creature* minion = me->SummonCreature(NPC_MINION_OF_OMEN, me->GetPositionX()+frand(-5.0f, 5.0f), me->GetPositionY()+frand(-5.0f, 5.0f), me->GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 20000))
                                    minion->AI()->AttackStart(me->FindNearestPlayer(20.0f));
                                break;
                            case 9:
                                me->SummonCreature(NPC_OMEN, omenSummonPos);
                                break;
                        }
                    }
                }
                if (me->GetEntry() == NPC_CLUSTER_ELUNE)
                {
                    if (Player* sub = me->FindNearestPlayer(20.0f))
                        sub->CastSpell(sub, SPELL_LUNAR_FORTUNE, false);
                    else
                        DoCast(SPELL_LUNAR_FORTUNE); // Only works with spellfocus 180859 or 180874 for some reason
                }

                float displacement = 0.7f;
                for (uint8 i = 0; i < 4; i++)
                    me->SummonGameObject(GetFireworkGameObjectId(), me->GetPositionX() + (i%2 == 0 ? displacement : -displacement), me->GetPositionY() + (i > 1 ? displacement : -displacement), me->GetPositionZ() + 4.0f, me->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 1);
            }
            else
                //DoCastSelf(GetFireworkSpell(me->GetEntry()), true);
                me->CastSpell(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), GetFireworkSpell(me->GetEntry()), true);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_fireworkAI(creature);
    }
};

/*######
## npc_ropper_racer
######*/

class npc_ropper_racer : public CreatureScript
{
public:
    npc_ropper_racer() : CreatureScript("npc_ropper_racer") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ropper_racerAI(creature);
    }

    struct npc_ropper_racerAI : public ScriptedAI
    {
        npc_ropper_racerAI(Creature* c) : ScriptedAI(c) {}

        uint32 startTimer;
        bool active;

        void Reset()
        {
            startTimer = 1000;
            active = true;
            me->SetSpeedRate(MOVE_RUN, 2.0f);
        }

        void UpdateAI(uint32 diff)
        {
            if (active)
            {
                if (startTimer <= diff)
                {
                    float x, y, z;
                    me->GetNearPoint2D(x, y, 20.0f, me->GetOrientation());
                    z = me->GetMap()->GetHeight(x, y, me->GetPositionZ());

                    me->GetMotionMaster()->MovePoint(0, x, y, z);

                    active = false;
                }
                else
                    startTimer-=diff;
           }
        }
    };
};

/*#####
## Eye of Kilrogg - Warlock (Entry: 4277)
######*/

enum EyeMisc
{
    GLYPH_OF_EYE_OF_KILROGG = 58081,
    EYE_CRIT_MARKER         = 400,
    EYE_HEALTH_MOD          = 39,
    EYE_MANA_MOD            = 36
};

class npc_eye_of_kilrogg : public CreatureScript
{
public:
    npc_eye_of_kilrogg() : CreatureScript("npc_eye_of_kilrogg") {}

    struct npc_eye_of_kilroggAI : public PetAI
    {
        npc_eye_of_kilroggAI(Creature* creature) : PetAI(creature)
        {
            if (Unit* owner = me->GetCharmerOrOwner())
            {
                me->SetMaxHealth(EYE_HEALTH_MOD * owner->getLevel());
                me->SetHealth(me->GetMaxHealth());
                me->SetMaxPower(POWER_MANA, EYE_MANA_MOD * owner->getLevel());
                me->SetPower(POWER_MANA, me->GetMaxPower(POWER_MANA));
                me->SetLevel(owner->getLevel());
                if (owner->HasAura(GLYPH_OF_EYE_OF_KILROGG))
                {
                    me->SetSpeedRate(MOVE_WALK, 2.0f);
                    me->SetSpeedRate(MOVE_RUN, 2.0f);
                    me->SetSpeedRate(MOVE_FLIGHT, 2.0f);
                    if (owner->GetMap()->GetEntry()->addon > 1)
                        me->SetCanFly(true);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_eye_of_kilroggAI(creature);
    }
};

enum ValkyrProtectorMisc
{
    // Spells
    SPELL_SMITE          = 71841,
    SPELL_SMITE_H        = 71842,

    // Events
    EVENT_TARGET_SELECT  = 1,
    EVENT_SMITE          = 2,

    // Creatures
    NPC_VALKYR_PROTECTOR = 38392
};

class npc_pet_val_kyr_protector : public CreatureScript
{
    public:
        npc_pet_val_kyr_protector() : CreatureScript("npc_pet_val_kyr_protector") { }

        struct npc_pet_val_kyr_protectorAI : public ScriptedAI
        {
            npc_pet_val_kyr_protectorAI(Creature* creature) : ScriptedAI(creature) { }
            
            void Reset() override
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_SMITE, 2 * IN_MILLISECONDS);
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                if (me->IsInEvadeMode() || !me->IsAlive())
                return;

                Unit* owner = me->GetCharmerOrOwner();

                me->CombatStop(true);
                if (owner && !me->HasUnitState(UNIT_STATE_FOLLOW))
                {
                    me->GetMotionMaster()->Clear(false);
                    me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, me->GetFollowAngle(), MOTION_SLOT_ACTIVE);
                }
            }

            bool CanAIAttack(Unit const* victim) const override
            {
                if (Player* player = me->GetOwner() ? me->GetOwner()->ToPlayer() : nullptr)
                    if (!player->IsPvP() && victim->IsPvP())
                        return false;

                return true;
            }

            void UpdateAI(uint32 diff) override
            {  
                if (Player* player = me->GetOwner() ? me->GetOwner()->ToPlayer() : nullptr)
                    if (Unit* target = player->GetSelectedUnit())
                        if (me->IsValidAttackTarget(target))
                            AttackStart(target);

                if (!UpdateVictim())
                    return;

                _events.Update(diff);                

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SMITE:
                            if (me->GetEntry() == NPC_VALKYR_PROTECTOR)
                                DoCastVictim(SPELL_SMITE_H);
                            else
                                DoCastVictim(SPELL_SMITE);
                            _events.ScheduleEvent(EVENT_SMITE, 1.9 * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }
                // No Melee Attack
            }

        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pet_val_kyr_protectorAI(creature);
        }
};

enum GoblinLandMineMisc
{
    SPELL_DETONATION = 4043
};

class npc_goblin_land_mine : public CreatureScript
{
    public:
        npc_goblin_land_mine() : CreatureScript("npc_goblin_land_mine") { }
    
        struct npc_goblin_land_mineAI : public ScriptedAI
        {
            npc_goblin_land_mineAI(Creature* creature) : ScriptedAI(creature) { }

            void AttackStart(Unit* /*who*/) override { }
            
            void MoveInLineOfSight(Unit* who) override
            {
                if (Creature* creature = who->ToCreature())
                    if (me->IsValidAttackTarget(who) && me->IsWithinMeleeRange(who))
                    {
                        DoCastSelf(SPELL_DETONATION);
                        me->DespawnOrUnsummon();
                    }                    
            }
            
            void UpdateAI(uint32 /*diff*/) override { }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_goblin_land_mineAI(creature);
        }
};

enum elderClearwater
{
    EVENT_CLEARWATER_ANNOUNCE   = 1,

    CLEARWATER_SAY_PRE          = 0,
    CLEARWATER_SAY_START        = 1,
    CLEARWATER_SAY_WINNER       = 2,
    CLEARWATER_SAY_END          = 3,

    QUEST_FISHING_DERBY         = 24803,

    DATA_DERBY_FINISHED         = 1,
};

class npc_elder_clearwater : public CreatureScript
{
    public:
        npc_elder_clearwater() : CreatureScript("npc_elder_clearwater") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            QuestRelationBounds pObjectQR;
            QuestRelationBounds pObjectQIR;

            // pets also can have quests
            if (creature)
            {
                pObjectQR = sObjectMgr->GetCreatureQuestRelationBounds(creature->GetEntry());
                pObjectQIR = sObjectMgr->GetCreatureQuestInvolvedRelationBounds(creature->GetEntry());
            }
            else
                return true;

            QuestMenu &qm = player->PlayerTalkClass->GetQuestMenu();
            qm.ClearMenu();

            for (QuestRelations::const_iterator i = pObjectQIR.first; i != pObjectQIR.second; ++i)
            {
                uint32 quest_id = i->second;
                Quest const* quest = sObjectMgr->GetQuestTemplate(quest_id);
                if (!quest)
                    continue;

                if (!creature->AI()->GetData(DATA_DERBY_FINISHED))
                {
                    if (quest_id == QUEST_FISHING_DERBY)
                        player->PlayerTalkClass->SendQuestGiverRequestItems(quest, creature->GetGUID(), player->CanRewardQuest(quest, false), true);
                }
                else
                {
                    if (quest_id != QUEST_FISHING_DERBY)
                        player->PlayerTalkClass->SendQuestGiverRequestItems(quest, creature->GetGUID(), player->CanRewardQuest(quest, false), true);
                }
            }

            return true;
        }

        bool OnQuestReward(Player* player, Creature* creature, Quest const* quest, uint32 opt)
        {
            if (!creature->AI()->GetData(DATA_DERBY_FINISHED) && quest->GetQuestId() == QUEST_FISHING_DERBY)
            {
                creature->AI()->DoAction(DATA_DERBY_FINISHED);
                sCreatureTextMgr->SendChat(creature, CLEARWATER_SAY_WINNER, player, CHAT_MSG_MONSTER_YELL, LANG_UNIVERSAL, TEXT_RANGE_MAP);
            }
            return true;
        }

        struct npc_elder_clearwaterAI : public ScriptedAI
        {
            npc_elder_clearwaterAI(Creature *c) : ScriptedAI(c)
            {
                events.Reset();
                events.ScheduleEvent(EVENT_CLEARWATER_ANNOUNCE, 55 * MINUTE * IN_MILLISECONDS, 1, 0); // Clearwater should spawn 1h before the event
                active = false;
                completed = false;
            }
           
           uint32 GetData(uint32 type) const
           {
               if (type == DATA_DERBY_FINISHED)
                   return (uint32)completed;

               return 0;
           }

           void DoAction(int32 param)
           {
               if (param == DATA_DERBY_FINISHED)
                   completed = true;
           }

           void UpdateAI(uint32 diff)
           {
               events.Update(diff);
               switch (events.ExecuteEvent())
               {
               case EVENT_CLEARWATER_ANNOUNCE:
                   sCreatureTextMgr->SendChat(me, CLEARWATER_SAY_PRE, 0, CHAT_MSG_MONSTER_YELL, LANG_UNIVERSAL, TEXT_RANGE_MAP);
               }
               if (!active && IsHolidayActive(HOLIDAY_KALU_AK_FISHING_DERBY))
               {
                   sCreatureTextMgr->SendChat(me, CLEARWATER_SAY_START, 0, CHAT_MSG_MONSTER_YELL, LANG_UNIVERSAL, TEXT_RANGE_MAP);
                   active = true;
               }
               if (active && !IsHolidayActive(HOLIDAY_KALU_AK_FISHING_DERBY))
               {
                   sCreatureTextMgr->SendChat(me, CLEARWATER_SAY_END, 0, CHAT_MSG_MONSTER_YELL, LANG_UNIVERSAL, TEXT_RANGE_MAP);
                   active = false;
               }
           }

        private:
            EventMap events;
            bool active; // Main event active (14:00 - 15:00)
            bool completed; // First prize taken
        };
        
        CreatureAI* GetAI(Creature* pCreature) const override
        {
            return new npc_elder_clearwaterAI(pCreature);
        }
};

enum TrainWrecker
{
    GO_TOY_TRAIN          = 193963,
    SPELL_TOY_TRAIN_PULSE =  61551,
    SPELL_WRECK_TRAIN     =  62943,
    ACTION_WRECKED        =      1,
    EVENT_DO_JUMP         =      1,
    EVENT_DO_FACING       =      2,
    EVENT_DO_WRECK        =      3,
    EVENT_DO_DANCE        =      4,
    MOVEID_CHASE          =      1,
    MOVEID_JUMP           =      2
};
class npc_train_wrecker : public CreatureScript
{
    public:
        npc_train_wrecker() : CreatureScript("npc_train_wrecker") { }

        struct npc_train_wreckerAI : public NullCreatureAI
        {
            npc_train_wreckerAI(Creature* creature) : NullCreatureAI(creature), _isSearching(true), _nextAction(0), _timer(1 * IN_MILLISECONDS) { }

            GameObject* VerifyTarget() const
            {
                if (GameObject* target = ObjectAccessor::GetGameObject(*me, _target))
                    return target;
                me->HandleEmoteCommand(EMOTE_ONESHOT_RUDE);
                me->DespawnOrUnsummon(3 * IN_MILLISECONDS);
                return nullptr;
            }

            void UpdateAI(uint32 diff) override
            {
                if (_isSearching)
                {
                    if (diff < _timer)
                        _timer -= diff;
                    else
                    {
                        if (GameObject* target = me->FindNearestGameObject(GO_TOY_TRAIN, 15.0f))
                        {
                            _isSearching = false;
                            _target = target->GetGUID();
                            me->SetWalk(true);
                            me->GetMotionMaster()->MovePoint(MOVEID_CHASE, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), target->GetOrientation());
                        }
                        else
                            _timer = 3 * IN_MILLISECONDS;
                    }
                }
                else
                {
                    switch (_nextAction)
                    {
                        case EVENT_DO_JUMP:
                            if (GameObject* target = VerifyTarget())
                                me->GetMotionMaster()->MoveJump(*target, 5.0, 10.0, MOVEID_JUMP);
                            _nextAction = 0;
                            break;
                        case EVENT_DO_FACING:
                            if (GameObject* target = VerifyTarget())
                            {
                                me->SetFacingTo(target->GetOrientation());
                                me->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK1H);
                                _timer = 1.5 * IN_MILLISECONDS;
                                _nextAction = EVENT_DO_WRECK;
                            }
                            else
                                _nextAction = 0;
                            break;
                        case EVENT_DO_WRECK:
                            if (diff < _timer)
                            {
                                _timer -= diff;
                                break;
                            }
                            if (GameObject* target = VerifyTarget())
                            {
                                me->CastSpell(target, SPELL_WRECK_TRAIN, false);
                                target->AI()->DoAction(ACTION_WRECKED);
                                _timer = 2 * IN_MILLISECONDS;
                                _nextAction = EVENT_DO_DANCE;
                            }
                            else
                                _nextAction = 0;
                            break;
                        case EVENT_DO_DANCE:
                            if (diff < _timer)
                            {
                                _timer -= diff;
                                break;
                            }
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_DANCE);
                            me->DespawnOrUnsummon(5 * IN_MILLISECONDS);
                            _nextAction = 0;
                            break;
                        default:
                            break;
                    }
                }
            }

            void MovementInform(uint32 /*type*/, uint32 id)
            {
                if (id == MOVEID_CHASE)
                    _nextAction = EVENT_DO_JUMP;
                else if (id == MOVEID_JUMP)
                    _nextAction = EVENT_DO_FACING;
            }

        private:
            bool _isSearching;
            uint8 _nextAction;
            uint32 _timer;
            ObjectGuid _target;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_train_wreckerAI(creature);
        }
};

class npc_pvp_rank_officer : public CreatureScript
{
public:
    npc_pvp_rank_officer() : CreatureScript("npc_pvp_rank_officer") {}

    bool OnGossipHello(Player* player, Creature* creature) 
    {
        player->pvpData.SendToPlayer(player);

        const uint32 PVP_RANK_GOSSIP_MENU = 60213;

        player->PrepareGossipMenu(creature, player->GetDefaultGossipMenuForSource(creature), true);
        player->ADD_GOSSIP_ITEM_DB(PVP_RANK_GOSSIP_MENU, (player->GetTeam() == ALLIANCE) ? 0 : 1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        player->ADD_GOSSIP_ITEM_DB(PVP_RANK_GOSSIP_MENU, 2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);

        player->SendPreparedGossip(creature);

        return true; 
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        const uint32 GOSSIP_RANK_INFO = 60214;
        const uint32 GOSSIP_RANK_PROGRESS = 60215;
        const uint32 GOSSIP_RANK_PROGRESS_NO_STANDING = 60216;

        if (action == GOSSIP_ACTION_INFO_DEF + 1)
        {
            player->PlayerTalkClass->ClearMenus();
            player->PrepareGossipMenu(creature, GOSSIP_RANK_INFO);
            player->SendPreparedGossip(creature);
            return true;
        }
        else if (action == GOSSIP_ACTION_INFO_DEF + 2)
        {
            player->PlayerTalkClass->ClearMenus();
            player->PrepareGossipMenu(creature, player->pvpData.HasStanding() ? GOSSIP_RANK_PROGRESS : GOSSIP_RANK_PROGRESS_NO_STANDING);
            player->SendPreparedGossip(creature);
            return true;
        }

        return false;
    }
};

class npc_bg_spirit_guide : public CreatureScript
{
public:
    npc_bg_spirit_guide() : CreatureScript("npc_bg_spirit_guide") { }

    struct npc_bg_spirit_guideAI : public ScriptedAI
    {
        npc_bg_spirit_guideAI(Creature* creature) : ScriptedAI(creature) { }

        void UpdateAI(uint32 diff) override
        {
            if (!me->IsInWorld())
                return;

            if (BattlegroundMap* bgMap = me->GetMap()->ToBattlegroundMap())
            {
                if (Battleground* bg = bgMap->GetBG())
                {
                    if (!me->HasUnitState(UNIT_STATE_CASTING) && bg->GetStatus() == STATUS_IN_PROGRESS)
                    {
                        DoCastSelf(SPELL_SPIRIT_HEAL_CHANNEL);
                        bgMap->GetBG()->SetLastResurrectTime(me->GetGUID());
                    }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_bg_spirit_guideAI(creature);
    }
};

void AddSC_npcs_special()
{
    new npc_air_force_bots();
    new npc_lunaclaw_spirit();
    new npc_chicken_cluck();
    new npc_dancing_flames();
    new npc_doctor();
    new npc_injured_patient();
    new npc_garments_of_quests();
    new npc_guardian();
    new npc_sayge();
    new npc_steam_tonk();
    new npc_tonk_mine();
    new npc_training_dummy();
    new npc_wormhole();
    new npc_pet_trainer();
    new npc_tabard_vendor();
    new npc_experience();
    new npc_firework();
    new npc_train_wrecker();
    new npc_kingdom_of_dalaran_quests();
    new npc_ropper_racer();
    new npc_eye_of_kilrogg();
    new npc_pet_val_kyr_protector();
    new npc_goblin_land_mine();
    new npc_elder_clearwater();
    new npc_pvp_rank_officer();
    new npc_bg_spirit_guide();
}
