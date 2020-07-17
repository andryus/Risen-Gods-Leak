/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2012-2015 Rising-Gods <http://www.rising-gods.de/>
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
#include "InstanceScript.h"
#include "trial_of_the_champion.h"

struct FxTriggerRace
{
    Races race;
    uint32 triggerEntry;
};

FxTriggerRace FxTriggerRaces[] =
{
    { RACE_NONE,          0                       },
    { RACE_HUMAN,         NPC_FX_TRIGGER_HUMAN    },
    { RACE_ORC,           NPC_FX_TRIGGER_ORC      },
    { RACE_DWARF,         NPC_FX_TRIGGER_DWARF    },
    { RACE_NIGHTELF,      NPC_FX_TRIGGER_NIGHTELF },
    { RACE_UNDEAD_PLAYER, NPC_FX_TRIGGER_UNDEAD   },
    { RACE_TAUREN,        NPC_FX_TRIGGER_TAURE    },
    { RACE_GNOME,         NPC_FX_TRIGGER_GNOME    },
    { RACE_TROLL,         NPC_FX_TRIGGER_TROLL    },
    { RACE_NONE,          0                       },
    { RACE_BLOODELF,      NPC_FX_TRIGGER_BLOODELF },
    { RACE_DRAENEI,       NPC_FX_TRIGGER_DRAENEI  }
};

class instance_trial_of_the_champion : public InstanceMapScript
{
public:
    instance_trial_of_the_champion() : InstanceMapScript("instance_trial_of_the_champion", 650) { }

    struct instance_trial_of_the_champion_InstanceMapScript : public InstanceScript
    {
        instance_trial_of_the_champion_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            SetBossNumber(MAX_ENCOUNTER);
            SetHeaders(DataHeader);
            // Grand Champions
            GrandChampionGUID[0].Clear();
            GrandChampionGUID[1].Clear();
            GrandChampionGUID[2].Clear();
            GrandChampionsChestGUID.Clear();
            LesserChampionDeathCount = 0;
            GrandChampionsMovement = 0;
            VehicleList.clear();
            AllSummonsList.clear();

            // Argent Challange
            ArgentSoldierDeaths = 0;
            ArgentChallangeChestGUID.Clear();
            ArgentChampionGUID.Clear();
            ArgentChampion = RAND(NPC_EADRIC, NPC_PALETRESS);

            // Black Knight
            BlackKnightGUID.Clear();
            KnightSpawned = false;

            // instance / misc
            AnnouncerGUID.Clear();
            MainGateGUID.Clear();
            NorthPortcullisGUID.Clear();

            EventStage = 0;
            EventTimer = 0;
            TeamInInstance = HORDE; // make sure TeamInInstance has always a correct value, to prevent bugs with stupid gms

            if (GetBossState(BOSS_GRAND_CHAMPIONS) != DONE)
            {
                _events.RescheduleEvent(EVENT_CHEER_AT_RANDOM_PLAYER, urand(20, 30)*IN_MILLISECONDS);
                _events.RescheduleEvent(EVENT_FIREWORK, urand(15, 25)*IN_MILLISECONDS);
            }
        }

        void OnPlayerEnter(Player* player)
        {
            if (player && !player->IsGameMaster())
                TeamInInstance = player->GetTeam();

            // this is necessary because OnPlayerEnter is called after OnCreatureCreate
            if (Creature* announcer = ObjectAccessor::GetCreature(*player, AnnouncerGUID))
                if (TeamInInstance == ALLIANCE)
                    announcer->UpdateEntry(NPC_ARELAS);
        }

        void OnPlayerRemove(Player* /*player*/)
        {
            for (Player& player : instance->GetPlayers())
                if (player.IsAlive() && !player.IsGameMaster())
                    return;

            for (uint8 i = 0; i <= 2; i++)
            {
                if (Creature* grandchampion = instance->GetCreature(GrandChampionGUID[i]))
                {
                    grandchampion->SetControlled(false, UNIT_STATE_STUNNED);
                    grandchampion->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    grandchampion->SetStandState(UNIT_STAND_STATE_STAND);                    
                    grandchampion->RestoreFaction();
                    grandchampion->AI()->EnterEvadeMode();
                }
            }

        }

        void OnPlayerEquipItem(Player* player, Item* /*item*/, EquipmentSlots slot)
        {
            if (slot != EQUIPMENT_SLOT_MAINHAND && slot != EQUIPMENT_SLOT_OFFHAND)
                return;

            if (player->HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT) && !player->HasItemOrGemWithIdEquipped(ITEM_ARGENT_LANCE, 1))
                player->ExitVehicle();
        }

        void DoCheerAtRandomUnit(Unit* unit, Team team, Races race)
        {
            uint32 fxTriggerEntry1 = team == HORDE ? NPC_FX_TRIGGER_HORDE : NPC_FX_TRIGGER_ALLIANCE;
            uint32 fxTriggerEntry2 = FxTriggerRaces[race].triggerEntry;

            if (Creature* fxTrigger = instance->SummonCreature(RAND(fxTriggerEntry1, fxTriggerEntry2), ToCCommonPos[0]))
            {
                fxTrigger->AI()->Talk(0, unit);
                fxTrigger->DespawnOrUnsummon(IN_MILLISECONDS);
            }
        }

        void DoCheerAtRandomPlayer()
        {
            std::list<Player*> PlayerList;
            for (Player& player : instance->GetPlayers())
                PlayerList.push_back(&player);

            if (PlayerList.empty())
                return;

            if (Player* player = Trinity::Containers::SelectRandomContainerElement(PlayerList))
                DoCheerAtRandomUnit(player, Team(player->GetTeam()), Races(player->getRace()));
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case NPC_JAEREN:
                    if (KnightSpawned)
                        creature->SetPhaseMask(2, true);
                    if (TeamInInstance == ALLIANCE)
                        creature->UpdateEntry(NPC_ARELAS);
                    creature->SetImmuneToAll(true);
                    AnnouncerGUID = creature->GetGUID();
                    break;
                // Grand Champions
                case NPC_MOKRA:
                case NPC_ERESSEA:
                case NPC_RUNOK:
                case NPC_ZULTORE:
                case NPC_VISCERI:
                case NPC_JACOB:
                case NPC_AMBROSE:
                case NPC_COLOSOS:
                case NPC_JAELYNE:
                case NPC_LANA:
                // Grand Champions' vehicle
                case VEHICLE_MOKRA_SKILLCRUSHER_MOUNT:
                case VEHICLE_ERESSEA_DAWNSINGER_MOUNT:
                case VEHICLE_RUNOK_WILDMANE_MOUNT:
                case VEHICLE_ZUL_TORE_MOUNT:
                case VEHICLE_DEATHSTALKER_VESCERI_MOUNT:
                case VEHICLE_MARSHAL_JACOB_ALERIUS_MOUNT:
                case VEHICLE_AMBROSE_BOLTSPARK_MOUNT:
                case VEHICLE_COLOSOS_MOUNT:
                case VEHICLE_JAELYNES_MOUNT:
                case VEHICLE_LANA_STOUTHAMMER_MOUNT:
                // Grand Champions' Lesser Champion
                case NPC_ORGRIMAR_CHAMPION:
                case NPC_SILVERMOON_CHAMPION:
                case NPC_THUNDER_CHAMPION:
                case NPC_TROLL_CHAMPION:
                case NPC_UNDERCITY_CHAMPION:
                case NPC_STORMWIND_CHAMPION:
                case NPC_GNOMERAGN_CHAMPION:
                case NPC_EXODAR_CHAMPION:
                case NPC_DRNASSUS_CHAMPION:
                case NPC_IRONFORGE_CHAMPION:
                // Lesser Champions' vehicle
                case VEHICLE_FORSAKE_WARHORSE:
                case VEHICLE_THUNDER_BLUFF_KODO:
                case VEHICLE_ORGRIMMAR_WOLF:
                case VEHICLE_SILVERMOON_HAWKSTRIDER:
                case VEHICLE_DARKSPEAR_RAPTOR:
                case VEHICLE_DARNASSIA_NIGHTSABER:
                case VEHICLE_EXODAR_ELEKK:
                case VEHICLE_STORMWIND_STEED:
                case VEHICLE_GNOMEREGAN_MECHANOSTRIDER:
                case VEHICLE_IRONFORGE_RAM:
                    AllSummonsList.push_back(creature->GetGUID());
                    break;
                case VEHICLE_ARGENT_WARHORSE:
                case VEHICLE_ARGENT_BATTLEWORG:
                    creature->setRegeneratingHealth(false);
                    VehicleList.push_back(creature->GetGUID());
                    break;
                case NPC_EADRIC:
                case NPC_PALETRESS:
                    ArgentChampionGUID = creature->GetGUID();;
                    break;
                case NPC_BLACK_KNIGHT:
                    BlackKnightGUID = creature->GetGUID();
                    break;
                case NPC_RISEN_JAEREN:
                case NPC_RISEN_ARELAS:
                    if (Creature* knight = instance->GetCreature(BlackKnightGUID))
                        knight->AI()->JustSummoned(creature);
                    break;
                default:
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GO_MAIN_GATE:
                    MainGateGUID = go->GetGUID();
                    break;
                case GO_NORTH_PORTCULLIS:
                    NorthPortcullisGUID = go->GetGUID();
                    break;
                case GO_CHAMPIONS_LOOT:
                case GO_CHAMPIONS_LOOT_H:
                    GrandChampionsChestGUID = go->GetGUID();
                    break;
                case GO_EADRIC_LOOT:
                case GO_EADRIC_LOOT_H:
                case GO_PALETRESS_LOOT:
                case GO_PALETRESS_LOOT_H:
                    ArgentChallangeChestGUID = go->GetGUID();
                    break;
            }
        }

        bool SetBossState(uint32 type, EncounterState state)
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;

            switch (type)
            {
                case BOSS_GRAND_CHAMPIONS:
                    switch (state)
                    {
                        case NOT_STARTED:
                            LesserChampionDeathCount = 0;
                            GrandChampionsMovement = 0;
                            break;
                        case DONE:
                            if (Creature* announcer = instance->GetCreature(AnnouncerGUID))
                            {
                                announcer->SummonGameObject(instance->IsHeroic() ? GO_CHAMPIONS_LOOT_H : GO_CHAMPIONS_LOOT,
                                ToCCommonPos[0].GetPositionX(), ToCCommonPos[0].GetPositionY(), ToCCommonPos[0].GetPositionZ(), 0, 0, 0, 0, 0, 9000000);

                                if (Player* player = announcer->FindNearestPlayer(100.f))
                                    ((InstanceMap*)instance)->PermBindAllPlayers(player);
                            }
                            _events.CancelEvent(EVENT_FIREWORK);
                            break;
                        default:
                            break;
                    }
                    break;
                case BOSS_ARGENT_CHALLENGE:
                    switch (state)
                    {
                        case NOT_STARTED:
                            ArgentSoldierDeaths = 0;
                            break;
                        case IN_PROGRESS:
                            // cleanup chest
                            if (GameObject* cache = instance->GetGameObject(GrandChampionsChestGUID))
                                cache->Delete();
                            break;
                        case DONE:
                            if (Creature* announcer = instance->GetCreature(AnnouncerGUID))
                            {
                                if (ArgentChampion == NPC_EADRIC)
                                    announcer->SummonGameObject(instance->IsHeroic() ? GO_EADRIC_LOOT_H : GO_EADRIC_LOOT,
                                    ToCCommonPos[0].GetPositionX(), ToCCommonPos[0].GetPositionY(), ToCCommonPos[0].GetPositionZ(), 0, 0, 0, 0, 0, 9000000);
                                else
                                    announcer->SummonGameObject(instance->IsHeroic() ? GO_PALETRESS_LOOT_H : GO_PALETRESS_LOOT,
                                    ToCCommonPos[0].GetPositionX(), ToCCommonPos[0].GetPositionY(), ToCCommonPos[0].GetPositionZ(), 0, 0, 0, 0, 0, 9000000);
                            }
                            _events.CancelEvent(EVENT_CHEER_AT_RANDOM_PLAYER);
                            break;
                        default:
                            break;
                    }
                    break;
                case BOSS_BLACK_KNIGHT:
                    switch (state)
                    {
                        case IN_PROGRESS:
                            // cleanup chest
                            if (GameObject* cache = instance->GetGameObject(ArgentChallangeChestGUID))
                                cache->Delete();

                            EventStage = TeamInInstance == HORDE ? EVENT_STAGE_BLACK_KNIGHT_035 : EVENT_STAGE_BLACK_KNIGHT_036;
                            EventTimer = IN_MILLISECONDS;
                            break;
                        case DONE:
                            EventStage = EVENT_STAGE_BLACK_KNIGHT_040;
                            EventTimer = IN_MILLISECONDS;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }

            if (type < MAX_ENCOUNTER)
            {
                if (state == DONE || state == NOT_STARTED)
                {
                    HandleGameObject(NorthPortcullisGUID, true);
                    if (Creature* announcer = instance->GetCreature(AnnouncerGUID))
                    {
                        announcer->GetMotionMaster()->MovePoint(0, ToCCommonPos[1]);
                        announcer->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    }
                }
                else if (state == IN_PROGRESS)
                {
                    HandleGameObject(NorthPortcullisGUID, false);
                    if (Creature* announcer = instance->GetCreature(AnnouncerGUID))
                        announcer->GetMotionMaster()->MovePoint(0, ToCCommonPos[2]);
                }
            }
            return false;
        }

        bool CheckAchievementCriteriaMeet(uint32 criteria_id, Player const* /*source*/, Unit const* /*target = NULL*/, uint32 /*miscvalue1 = 0*/)
        {
            if (criteria_id == ACHIEVEMENT_CRITERIA_HAD_WORSE)
                if (Creature* knight = instance->GetCreature(BlackKnightGUID))
                    return knight->AI()->GetData(0) == 1;

            return false;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_LESSER_CHAMPIONS_DEFEATED: // first encounter
                    ++LesserChampionDeathCount;
                    switch (LesserChampionDeathCount)
                    {
                        case 3:
                            SetData(DATA_EVENT_TIMER, 3000);
                            SetData(DATA_EVENT_STAGE, EVENT_STAGE_CHAMPIONS_WAVE_2_START);
                            break;
                        case 6:
                            SetData(DATA_EVENT_TIMER, 3000);
                            SetData(DATA_EVENT_STAGE, EVENT_STAGE_CHAMPIONS_WAVE_3_START);
                            break;
                        case 9:
                            SetData(DATA_EVENT_TIMER, 5000);
                            SetData(DATA_EVENT_STAGE, EVENT_STAGE_CHAMPIONS_WAVE_BOSS);
                            break;
                    }
                    break;
                case DATA_PHASE_TWO_MOVEMENT: // first encounter
                    if (++GrandChampionsMovement == 3)
                    {
                        EventStage = EVENT_STAGE_CHAMPIONS_WAVE_BOSS_P2;
                        EventTimer = 10000;
                    }
                    break;
                case DATA_CANCEL_FIRST_ENCOUNTER:
                    for (std::list<ObjectGuid>::const_iterator itr = AllSummonsList.begin(); itr != AllSummonsList.end(); ++itr)
                    {
                        if (Creature* temp = instance->GetCreature(*itr))
                            temp->DespawnOrUnsummon();
                    }
                    SetBossState(BOSS_GRAND_CHAMPIONS, NOT_STARTED);
                    break;
                case DATA_ARGENT_SOLDIER_DEFEATED: // second encounter
                    if (++ArgentSoldierDeaths == 9)
                    {
                        if (Creature* champion = instance->GetCreature(ArgentChampionGUID))
                        {
                            champion->GetMotionMaster()->MovePoint(0, 746.88f, 618.74f, 411.06f);
                            champion->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                            champion->SetImmuneToAll(false);
                            champion->setFaction(FACTION_HOSTILE);
                        }
                    }
                    break;
                case DATA_BLACK_KNIGHT_SPAWNED: // third encounter
                    KnightSpawned = true;
                    SaveToDB();
                    break;
                case DATA_EVENT_STAGE: // instance stuff
                    EventStage = data;
                    break;
                case DATA_EVENT_TIMER: // instance stuff
                    EventTimer = data;
                    break;
            }
        }

        void SetGuidData(uint32 type, ObjectGuid data) override
        {
            switch (type)
            {
                case DATA_GRAND_CHAMPION_1:
                    GrandChampionGUID[0] = data;
                    break;
                case DATA_GRAND_CHAMPION_2:
                    GrandChampionGUID[1] = data;
                    break;
                case DATA_GRAND_CHAMPION_3:
                    GrandChampionGUID[2] = data;
                    break;
            }
        }

        uint32 GetData(uint32 data) const
        {
            switch (data)
            {
                case DATA_EVENT_STAGE:
                    return EventStage;
                case DATA_EVENT_TIMER:
                    return EventTimer;
                case DATA_TEAM_IN_INSTANCE:
                    return TeamInInstance;
                case DATA_ARGENT_CHAMPION:
                    return ArgentChampion;

                case DATA_EVENT_NPC:
                    switch (EventStage)
                    {
                        case EVENT_STAGE_CHAMPIONS_010:
                        case EVENT_STAGE_CHAMPIONS_040:
                        case EVENT_STAGE_CHAMPIONS_WAVE_1:
                        case EVENT_STAGE_CHAMPIONS_WAVE_2:
                        case EVENT_STAGE_CHAMPIONS_WAVE_3:
                        case EVENT_STAGE_CHAMPIONS_050:
                        case EVENT_STAGE_CHAMPIONS_WAVE_1_START:
                        case EVENT_STAGE_CHAMPIONS_WAVE_2_START:
                        case EVENT_STAGE_CHAMPIONS_WAVE_3_START:
                        case EVENT_STAGE_CHAMPIONS_WAVE_BOSS:
                        case EVENT_STAGE_CHAMPIONS_WAVE_BOSS_P2:

                        case EVENT_STAGE_ARGENT_CHALLENGE_000:
                        case EVENT_STAGE_ARGENT_CHALLENGE_010:
                        case EVENT_STAGE_ARGENT_CHALLENGE_030:
                        case EVENT_STAGE_ARGENT_CHALLENGE_040:
                        case EVENT_STAGE_ARGENT_CHALLENGE_050:

                        case EVENT_STAGE_BLACK_KNIGHT_000:
                        case EVENT_STAGE_BLACK_KNIGHT_010:
                        case EVENT_STAGE_BLACK_KNIGHT_030:
                        case EVENT_STAGE_BLACK_KNIGHT_040:
                        case EVENT_STAGE_BLACK_KNIGHT_050:
                            return NPC_TIRION;

                        case EVENT_STAGE_CHAMPIONS_000:
                        case EVENT_STAGE_ARGENT_CHALLENGE_020:
                        case EVENT_STAGE_BLACK_KNIGHT_020:
                            return TeamInInstance == HORDE ? NPC_JAEREN : NPC_ARELAS;

                        case EVENT_STAGE_CHAMPIONS_015:
                        case EVENT_STAGE_BLACK_KNIGHT_060:
                            return NPC_THRALL;

                        case EVENT_STAGE_CHAMPIONS_020:
                        case EVENT_STAGE_BLACK_KNIGHT_035:
                            return NPC_GARROSH;

                        case EVENT_STAGE_CHAMPIONS_030:
                        case EVENT_STAGE_BLACK_KNIGHT_036:
                        case EVENT_STAGE_BLACK_KNIGHT_070:
                            return NPC_VARIAN;

                        default:
                            break;
                    }
                default:
                    break;
            }

            return 0;
        }

        ObjectGuid GetGuidData(uint32 data) const
        {
            switch (data)
            {
                case GO_MAIN_GATE:
                    return MainGateGUID;
                case DATA_ANNOUNCER:
                    return AnnouncerGUID;

                case DATA_GRAND_CHAMPION_1:
                    return GrandChampionGUID[0];
                case DATA_GRAND_CHAMPION_2:
                    return GrandChampionGUID[1];
                case DATA_GRAND_CHAMPION_3:
                    return GrandChampionGUID[2];

                case NPC_PALETRESS:
                case NPC_EADRIC:
                    return ArgentChampionGUID;

                case NPC_BLACK_KNIGHT:
                    return BlackKnightGUID;
            }

            return ObjectGuid::Empty;
        }

        void Update(uint32 diff)
        {
            if (_events.Empty())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CHEER_AT_RANDOM_PLAYER:
                        DoCheerAtRandomPlayer();
                        _events.RescheduleEvent(EVENT_CHEER_AT_RANDOM_PLAYER, urand(20, 25)*IN_MILLISECONDS);
                        break;
                    case EVENT_FIREWORK:
                        for (uint8 i=0; i<urand(2, 5); ++i)
                            if (Creature* trigger = instance->SummonCreature(NPC_TRIGGER_FIREWORK, FireworkPos[i]))
                            {
                                trigger->DespawnOrUnsummon(10*IN_MILLISECONDS);
                                trigger->CastSpell(trigger, SPELL_FIREWORK_TRIGGER);
                            }
                        _events.RescheduleEvent(EVENT_FIREWORK, urand(15, 20)*IN_MILLISECONDS);
                         break;
                    default:
                        break;
                }
            }
        }

        std::string GetSaveData()
        {
            OUT_SAVE_INST_DATA;

            std::ostringstream saveStream;
            saveStream << "T 5 "            // Header
                << ArgentChampion << " "    // Argent Champion
                << (uint32) KnightSpawned;  // Black Knight

            OUT_SAVE_INST_DATA_COMPLETE;
            return saveStream.str();
        }

        void Load(const char* strIn)
        {
            if (!strIn)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(strIn);

            std::istringstream loadStream(strIn);

            char dataHead1, dataHead2;
            loadStream >> dataHead1 >> dataHead2;

            if (dataHead1 == 'T' && dataHead2 == '5')
            {
                for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                {
                    uint32 tmpState;
                    loadStream >> tmpState;
                    if (tmpState == IN_PROGRESS || tmpState > SPECIAL)
                        tmpState = NOT_STARTED;

                    SetBossState(i, EncounterState(tmpState));
                }
            }

            loadStream >> ArgentChampion;
            loadStream >> KnightSpawned;

            if (KnightSpawned && GetBossState(BOSS_BLACK_KNIGHT) != DONE)
                if (Creature* knight = instance->SummonCreature(NPC_BLACK_KNIGHT, BlackKnight[1]))
                    knight->AI()->JustReachedHome();

            OUT_LOAD_INST_DATA_COMPLETE;
        }

    protected:
        // Grand Champions
        ObjectGuid GrandChampionGUID[3];
        ObjectGuid GrandChampionsChestGUID;
        uint32 LesserChampionDeathCount;
        uint32 GrandChampionsMovement;
        std::list<ObjectGuid> VehicleList;
        std::list<ObjectGuid> AllSummonsList;

        // Argent Challange
        ObjectGuid ArgentChallangeChestGUID;
        ObjectGuid ArgentChampionGUID;
        uint32 ArgentChampion;
        uint8 ArgentSoldierDeaths;

        // Black Knight
        ObjectGuid BlackKnightGUID;
        bool KnightSpawned;

        // instance / misc
        ObjectGuid AnnouncerGUID;
        ObjectGuid MainGateGUID;
        ObjectGuid NorthPortcullisGUID;

        uint32 EventStage;
        uint32 EventTimer;
        uint32 TeamInInstance;
        EventMap _events;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_trial_of_the_champion_InstanceMapScript(map);
    }
};

void AddSC_instance_trial_of_the_champion()
{
    new instance_trial_of_the_champion();
}
