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

/*######
## npc_cork_gizelton
######*/

enum Caravan
{
    QUEST_BODYGUARD_FOR_HIRE     = 5821,
    QUEST_GIZELTON_CARAVAN       = 5943,

    EVENT_RESUME_PATH            = 1,
    EVENT_WAIT_FOR_ASSIST        = 2,
    EVENT_START_ESCORT           = 3,

    NPC_CORK_GIZELTON            = 11625,
    NPC_RIGGER_GIZELTON          = 11626,
    NPC_CARAVAN_KODO             = 11564,
    NPC_VENDOR_TRON              = 12245,
    NPC_SUPER_SELLER             = 12246,

    // For both Cork and Rigger
    SAY_CARAVAN_LEAVE            = 0,
    SAY_CARAVAN_HIRE             = 1,

    SAY_CORK_AMBUSH_1            = 2,
    SAY_CORK_AMBUSH_2            = 3,
    SAY_CORK_AMBUSH_3            = 4,
    SAY_CORK_END                 = 5,

    SAY_RIGGER_AMBUSH_1          = 2,
    SAY_RIGGER_AMBUSH_2          = 3,
    SAY_RIGGER_END               = 4,

    MAX_CARAVAN_SUMMONS          = 3,

    TIME_SHOP_STOP               = 10 * MINUTE*IN_MILLISECONDS,
    TIME_HIRE_STOP               = 4 * MINUTE*IN_MILLISECONDS,
};

enum KolkarAmbusher
{
    NPC_KOLKAR_WAYLAYER = 12976,
    NPC_KOLKAR_AMBUSHER = 12977
};

enum DemonAmbusher
{
    NPC_NETHER_SORCERESS = 4684,
    NPC_LESSER_INFERNAL = 4676,
    NPC_DOOMWARDER = 4677
};

#define NUM_AMBUSHES_BODYGUARD_FOR_HIRE 3
#define NUM_KOLKAR_AMBUSHERS 4

#define NUM_AMBUSHES_GIZELTON_CARAVAN 3
#define NUM_DEMON_AMBUSHERS 3

/// @todo Add values for orientation
const std::pair<KolkarAmbusher, Position> KOLKAR_SPAWNPOINTS[NUM_AMBUSHES_BODYGUARD_FOR_HIRE][NUM_KOLKAR_AMBUSHERS] = // QUEST_BODYGUARD_FOR_HIRE
{
    {
        std::make_pair(NPC_KOLKAR_WAYLAYER, Position(-969.05f, 1174.91f, 90.39f)),
        std::make_pair(NPC_KOLKAR_WAYLAYER, Position(-983.01f, 1192.88f, 90.01f)),

        std::make_pair(NPC_KOLKAR_AMBUSHER, Position(-985.71f, 1173.95f, 91.02f)),
        std::make_pair(NPC_KOLKAR_AMBUSHER, Position(-965.51f, 1193.58f, 92.15f))
    },

    {
        std::make_pair(NPC_KOLKAR_WAYLAYER, Position(-1147.83f, 1180.87f, 91.38f)),
        std::make_pair(NPC_KOLKAR_WAYLAYER, Position(-1160.97f, 1201.36f, 93.15f)),

        std::make_pair(NPC_KOLKAR_AMBUSHER, Position(-1163.96f, 1183.72f, 93.79f)),
        std::make_pair(NPC_KOLKAR_AMBUSHER, Position(-1146.20f, 1199.75f, 91.37f))
    },

    {
        std::make_pair(NPC_KOLKAR_WAYLAYER, Position(-1277.78f, 1218.56f, 109.30f)),
        std::make_pair(NPC_KOLKAR_WAYLAYER, Position(-1289.25f, 1239.20f, 108.79f)),

        std::make_pair(NPC_KOLKAR_AMBUSHER, Position(-1292.65f, 1221.28f, 109.99f)),
        std::make_pair(NPC_KOLKAR_AMBUSHER, Position(-1272.91f, 1234.39f, 108.14f))
    }
};

const std::pair<DemonAmbusher, Position> DEMON_SPAWNPOINTS[NUM_AMBUSHES_GIZELTON_CARAVAN][NUM_DEMON_AMBUSHERS] = // QUEST_GIZELTON_CARAVAN
{
    {
        std::make_pair(NPC_NETHER_SORCERESS, Position(-1823.7f, 2060.88f, 62.0925f, 0.252946f)),
        std::make_pair(NPC_LESSER_INFERNAL, Position(-1814.46f, 2060.13f, 62.4916f, 2.00647f)),
        std::make_pair(NPC_DOOMWARDER, Position(-1814.87f, 2080.6f, 63.6323f, 6.13453f))
    },

    {
        std::make_pair(NPC_NETHER_SORCERESS, Position(-1782.92f, 1942.55f, 60.2205f, 3.8722f)),
        std::make_pair(NPC_LESSER_INFERNAL, Position(-1786.5f, 1926.05f, 59.7502f, 3.2329f)),
        std::make_pair(NPC_DOOMWARDER, Position(-1805.74f, 1942.77f, 60.791f, 0.829597f))
    },

    {
        std::make_pair(NPC_NETHER_SORCERESS, Position(-1677.56f, 1835.67f, 58.9269f, 1.30894f)),
        std::make_pair(NPC_LESSER_INFERNAL, Position(-1675.66f, 1863.0f, 59.0008f, 0.4827f)),
        std::make_pair(NPC_DOOMWARDER, Position(-1692.31f, 1862.69f, 58.9553f, 1.45773f))
    }
};

class npc_cork_gizelton : public CreatureScript
{
public:
    npc_cork_gizelton() : CreatureScript("npc_cork_gizelton") { }

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_BODYGUARD_FOR_HIRE)
            creature->AI()->SetGuidData(player->GetGUID(), player->getFaction());

        return true;
    }
    
    struct npc_cork_gizeltonAI : public npc_escortAI
    {
        npc_cork_gizeltonAI(Creature* creature) : npc_escortAI(creature)
        {
        }
        
        void Initialize() 
        {
            _playerGUID.Clear();
            _faction = 35;
            headNorth = true;
            me->setActive(true);
            events.ScheduleEvent(EVENT_START_ESCORT, 0);
        }

        void JustRespawned() override
        {
            npc_escortAI::JustRespawned();
            Initialize();
        }

        void InitializeAI() override
        {
            npc_escortAI::InitializeAI();
            Initialize();
        }

        void JustDied(Unit* killer) override
        {
            RemoveSummons();
            npc_escortAI::JustDied(killer);
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            SummonsFollow();
            ImmuneFlagSet(false, 35);
            npc_escortAI::EnterEvadeMode();
        }

        void CheckPlayer()
        {
            if (_playerGUID)
                if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID))
                    if (me->IsWithinDist(player, 60.0f))
                        return;

            _playerGUID.Clear();
            _faction = 35;
            ImmuneFlagSet(false, _faction);
        }

        void SetGuidData(ObjectGuid playerGUID, uint32 faction) override
        {
            _playerGUID = playerGUID;
            _faction = faction;
            SetEscortPaused(false);
            if (Creature* active = !headNorth ? me : ObjectAccessor::GetCreature(*me, summons[0]))
                active->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            events.CancelEvent(EVENT_WAIT_FOR_ASSIST);
        }

        void SetData(uint32 field, uint32 data) override
        {
            if (field == 1 && data == 1)
                if (Player* player = me->FindNearestPlayer(50.0f))
                    SetGuidData(player->GetGUID(), player->getFaction());
        }

        bool CheckCaravan()
        {
            bool summonTooFar = false;

            for (uint8 i = 0; i < MAX_CARAVAN_SUMMONS; ++i)
            {
                if (summons[i].IsEmpty())
                {
                    SummonHelpers();
                    return false;
                }

                Creature* summon = ObjectAccessor::GetCreature(*me, summons[i]);
                if (!summon)
                {
                    SummonHelpers();
                    return false;
                }

                if (me->GetDistance2d(summon) > 25.0f)
                    summonTooFar = true;
            }

            if (summonTooFar) // Most likely caused by a bug
                SummonHelpers();

            return true;
        }

        void RemoveSummons()
        {
            for (uint8 i = 0; i < MAX_CARAVAN_SUMMONS; ++i)
            {
                if (Creature* summon = ObjectAccessor::GetCreature(*me, summons[i]))
                    summon->DespawnOrUnsummon();

                summons[i].Clear();
            }
        }

        void SummonHelpers()
        {
            RemoveSummons();
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);

            Creature* cr = NULL;
            if ((cr = me->SummonCreature(NPC_RIGGER_GIZELTON, *me)))
            {
                cr->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                summons[0] = cr->GetGUID();
            }
            if ((cr = me->SummonCreature(NPC_CARAVAN_KODO, *me)))
                summons[1] = cr->GetGUID();
            if ((cr = me->SummonCreature(NPC_CARAVAN_KODO, *me)))
                summons[2] = cr->GetGUID();

            SummonsFollow();
        }

        void SummonedCreatureDies(Creature* creature, Unit*)
        {
            if (creature->GetGUID() == summons[0])
                summons[0].Clear();
            else if (creature->GetGUID() == summons[1])
                summons[1].Clear();
            else if (creature->GetGUID() == summons[2])
                summons[2].Clear();
        }

        void SummonedCreatureDespawn(Creature* creature)
        {
            if (creature->GetGUID() == summons[0])
                summons[0].Clear();
            else if (creature->GetGUID() == summons[1])
                summons[1].Clear();
            else if (creature->GetGUID() == summons[2])
                summons[2].Clear();
        }

        void SummonsFollow()
        {
            float dist = 1.0f;
            for (uint8 i = 0; i < MAX_CARAVAN_SUMMONS; ++i)
                if (Creature* summon = ObjectAccessor::GetCreature(*me, summons[i]))
                {
                    summon->GetMotionMaster()->Clear(false);
                    summon->StopMoving();
                    summon->setActive(true);
                    summon->GetMotionMaster()->MoveFollow(me, dist, M_PI_F, MOTION_SLOT_ACTIVE);
                    dist += (i == 1 ? 9.5f : 3.0f);
                }
        }

        void RelocateSummons()
        {
            for (uint8 i = 0; i < MAX_CARAVAN_SUMMONS; ++i)
                if (Creature* summon = ObjectAccessor::GetCreature(*me, summons[i]))
                    summon->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
        }

        void ImmuneFlagSet(bool remove, uint32 faction)
        {
            for (uint8 i = 0; i < MAX_CARAVAN_SUMMONS; ++i)
                if (Creature* summon = ObjectAccessor::GetCreature(*me, summons[i]))
                {
                    summon->SetImmuneToNPC(!remove);
                    summon->setFaction(faction);
                }
            me->SetImmuneToNPC(!remove);
            me->setFaction(faction);
        }

        void WaypointReached(uint32 waypointId) override
        {
            RelocateSummons();
            switch (waypointId)
            {
                // Finished north path
                case 52:
                    me->SummonCreature(NPC_VENDOR_TRON, -694.61f, 1460.7f, 90.794f, 2.4f, TEMPSUMMON_TIMED_DESPAWN, TIME_SHOP_STOP + 15 * IN_MILLISECONDS);
                    SetEscortPaused(true);
                    events.ScheduleEvent(EVENT_RESUME_PATH, TIME_SHOP_STOP);
                    CheckCaravan();
                    break;
                    // Finished south path
                case 193:
                    me->SummonCreature(NPC_SUPER_SELLER, -1905.5f, 2463.3f, 61.52f, 5.87f, TEMPSUMMON_TIMED_DESPAWN, TIME_SHOP_STOP + 15 * IN_MILLISECONDS);
                    SetEscortPaused(true);
                    events.ScheduleEvent(EVENT_RESUME_PATH, TIME_SHOP_STOP);
                    CheckCaravan();
                    break;
                    // North -> South - hire
                case 76:
                    SetEscortPaused(true);
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    Talk(SAY_CARAVAN_HIRE);
                    events.ScheduleEvent(EVENT_WAIT_FOR_ASSIST, TIME_HIRE_STOP);
                    break;
                    // Sout -> North - hire
                case 208:
                    SetEscortPaused(true);
                    if (Creature* rigger = ObjectAccessor::GetCreature(*me, summons[0]))
                    {
                        rigger->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                        rigger->AI()->Talk(SAY_CARAVAN_HIRE);
                    }
                    events.ScheduleEvent(EVENT_WAIT_FOR_ASSIST, TIME_HIRE_STOP);
                    break;
                    // North -> South - complete
                case 115:
                    if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID))
                    {
                        if (CheckCaravan())
                        {
                            player->GroupEventHappens(QUEST_BODYGUARD_FOR_HIRE, player);
                            Talk(SAY_CORK_END);
                        }
                        else
                            player->FailQuest(QUEST_BODYGUARD_FOR_HIRE);
                    }
                    _playerGUID.Clear();
                    CheckPlayer();
                    break;
                    // South -> North - complete
                case 240:
                    if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID))
                    {
                        if (CheckCaravan())
                        {
                            player->GroupEventHappens(QUEST_GIZELTON_CARAVAN, player);
                            if (Creature* rigger = ObjectAccessor::GetCreature(*me, summons[0]))
                                rigger->AI()->Talk(SAY_RIGGER_END);
                        }
                        else
                            player->FailQuest(QUEST_GIZELTON_CARAVAN);
                    }
                    _playerGUID.Clear();
                    CheckPlayer();
                    break;
                // North -> South - spawn attackers
                case 95:
                    KolkarAmbush(0);
                    break;
                case 102:
                    KolkarAmbush(1);
                    break;
                case 110:
                    KolkarAmbush(2);
                    break;
                // South -> North - spawn attackers
                case 217:
                    DemonAmbush(0);
                    break;
                case 224:
                    DemonAmbush(1);
                    break;
                case 234:
                    DemonAmbush(2);
                    break;
                case 282:
                    SetNextWaypoint(1, false, true);
                    break;
            }
        }

        void KolkarAmbush(uint8 ambushNum)
        {
            if (!_playerGUID)
                return;

            ImmuneFlagSet(true, _faction);
            Creature* cr = nullptr;

            for (uint8 i = 0; i < NUM_KOLKAR_AMBUSHERS; ++i)
            {
                std::pair<KolkarAmbusher, Position> const& spawn = KOLKAR_SPAWNPOINTS[ambushNum][i];
                if ((cr = me->SummonCreature(spawn.first, spawn.second, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, MINUTE * IN_MILLISECONDS)))
                    cr->Attack(me, false);
            }

            if (cr)
            {
                AttackStart(cr);
                me->CallForHelp(50.0f);
            }

            switch (ambushNum)
            {
            case 0:
                Talk(SAY_CORK_AMBUSH_1);
                break;
            case 1:
                Talk(SAY_CORK_AMBUSH_2);
                break;
            case 2:
                Talk(SAY_CORK_AMBUSH_3);
                break;
            }
        }

        void DemonAmbush(uint8 ambushNum)
        {
            if (!_playerGUID)
                return;

            ImmuneFlagSet(true, _faction);
            Creature* cr = nullptr;

            for (uint8 i = 0; i < NUM_DEMON_AMBUSHERS; ++i)
            {
                std::pair<DemonAmbusher, Position> const& spawn = DEMON_SPAWNPOINTS[ambushNum][i];
                if ((cr = me->SummonCreature(spawn.first, spawn.second, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, MINUTE * IN_MILLISECONDS)))
                    cr->Attack(me, false);
            }

            if (cr)
            {
                AttackStart(cr);
                me->CallForHelp(50.0f);
            }

            uint8 textId = ambushNum == 1 ? SAY_RIGGER_AMBUSH_2 : SAY_RIGGER_AMBUSH_1;

            if (Creature* rigger = ObjectAccessor::GetCreature(*me, summons[0]))
                rigger->AI()->Talk(textId);

        }

        void UpdateEscortAI(uint32 const diff)
        {
            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case EVENT_RESUME_PATH:
                    SetEscortPaused(false);
                    if (Creature* talker = headNorth ? me : ObjectAccessor::GetCreature(*me, summons[0]))
                        talker->AI()->Talk(SAY_CARAVAN_LEAVE);

                    headNorth = !headNorth;
                    break;
                case EVENT_WAIT_FOR_ASSIST:
                    SetEscortPaused(false);
                    if (Creature* active = !headNorth ? me : ObjectAccessor::GetCreature(*me, summons[0]))
                        active->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    break;
                case EVENT_START_ESCORT:
                    CheckCaravan();
                    SetDespawnAtEnd(false);
                    if(me->GetVictim()) // Cannot start in combat
                        events.ScheduleEvent(EVENT_START_ESCORT, 1 * IN_MILLISECONDS);
                    else
                        Start(true, true, ObjectGuid::Empty, 0, false, false, true);
                    break;
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

    private:
        EventMap events;
        ObjectGuid summons[MAX_CARAVAN_SUMMONS];
        bool headNorth;

        ObjectGuid _playerGUID;
        uint32 _faction;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_cork_gizeltonAI(creature);
    }
};

void AddSC_desolace_rg()
{
    new npc_cork_gizelton();
}
