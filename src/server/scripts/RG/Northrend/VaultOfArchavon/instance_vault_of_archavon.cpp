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

#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "CreatureAI.h"
#include "InstanceScript.h"
#include "BattlefieldMgr.h"
#include "Battlefield.h"
#include "Player.h"
#include "vault_of_archavon.h"

/* Vault of Archavon encounters:
1 - Archavon the Stone Watcher
2 - Emalon the Storm Watcher
3 - Koralon the Flame Watcher
4 - Toravon the Ice Watcher
*/

class instance_vault_of_archavon : public InstanceMapScript
{
    public:
        instance_vault_of_archavon() : InstanceMapScript("instance_vault_of_archavon", 624) {}

        struct instance_vault_of_archavon_InstanceMapScript : public InstanceScript
        {
            instance_vault_of_archavon_InstanceMapScript(Map* map) : InstanceScript(map)
            {
                SetHeaders(DataHeader);
                SetBossNumber(MAX_ENCOUNTER);
                ArchavonGUID.Clear();
                EmalonGUID.Clear();
                KoralonGUID.Clear();
                ToravonGUID.Clear();

                ArchavonDeath = 0;
                EmalonDeath = 0;
                KoralonDeath = 0;

                if (sWorld->getBoolConfig(CONFIG_WINTERGRASP_ENABLE))
                    Events.ScheduleEvent(EVENT_CKECK_WG_TIMER, 5 * IN_MILLISECONDS);
            }

            void OnCreatureCreate(Creature* creature)
            {
                switch (creature->GetEntry())
                {
                    case NPC_ARCHAVON:
                        ArchavonGUID = creature->GetGUID();
                        break;
                    case NPC_EMALON:
                        EmalonGUID = creature->GetGUID();
                        break;
                    case NPC_KORALON:
                        KoralonGUID = creature->GetGUID();
                        break;
                    case NPC_TORAVON:
                        ToravonGUID = creature->GetGUID();
                        break;
                    default:
                        break;
                }
            }

            ObjectGuid GetGuidData(uint32 identifier) const
            {
                switch (identifier)
                {
                    case NPC_EMALON:
                        return EmalonGUID;
                    case NPC_TORAVON:
                        return ToravonGUID;
                    default:
                        break;
                }
                return ObjectGuid::Empty;
            }

            bool SetBossState(uint32 type, EncounterState state)
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;

                if (state != DONE)
                   return true;

                ObjectGuid killedBoss;

                switch (type)
                {
                    case BOSS_ARCHAVON:
                        ArchavonDeath = time(NULL);
                        killedBoss = ArchavonGUID;
                        break;
                    case BOSS_EMALON:
                        EmalonDeath = time(NULL);
                        killedBoss = EmalonGUID;
                        break;
                    case BOSS_KORALON:
                        KoralonDeath = time(NULL);
                        killedBoss = KoralonGUID;
                        break;
                    case BOSS_TORAVON:
                        killedBoss = ToravonGUID;
                        break;
                    default:
                        return true;
                }

                // on every death of Archavon, Emalon and Koralon check our achievement
                DoCastSpellOnPlayers(SPELL_EARTH_WIND_FIRE_ACHIEVEMENT_CHECK);

                constexpr uint32 HONOR_REWARD = 161u;
                if (Creature* victim = instance->GetCreature(killedBoss))
                    for (auto& player : victim->GetMap()->GetPlayers())
                        player.RewardHonorCreatureKill(victim, HONOR_REWARD);

                return true;
            }

            bool CheckAchievementCriteriaMeet(uint32 criteria_id, Player const* /*source*/, Unit const* /*target*/, uint32 /*miscvalue1*/)
            {
                switch (criteria_id)
                {
                    case CRITERIA_EARTH_WIND_FIRE_10:
                    case CRITERIA_EARTH_WIND_FIRE_25:
                        if (ArchavonDeath && EmalonDeath && KoralonDeath)
                        {
                            // instance difficulty check is already done in db (achievement_criteria_data)
                            // int() for Visual Studio, compile errors with abs(time_t)
                            return (abs(int(ArchavonDeath-EmalonDeath)) < MINUTE && \
                                abs(int(EmalonDeath-KoralonDeath)) < MINUTE && \
                                abs(int(KoralonDeath-ArchavonDeath)) < MINUTE);
                        }
                        break;
                    default:
                        break;
                }

                return false;
            }

            void OnPlayerRemove(Player* player)
            {
                player->RemoveAura(SPELL_CLOSE_WARNING);
            }

            void Update(uint32 diff)
            {
                if (Events.Empty())
                    return;

                Events.Update(diff);

                if (Events.ExecuteEvent() == EVENT_CKECK_WG_TIMER)
                {
                    if (Battlefield* wintergrasp = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG))
                    {
                        uint32 timer = wintergrasp->GetTimer();

                        if (!wintergrasp->IsWarTime() && timer <= 15 * MINUTE * IN_MILLISECONDS)
                            DoCastWarningSpellOnPlayersVOA(timer);

                        if (wintergrasp->IsWarTime())
                            DoStoneEncounter(timer);
                        else
                            DoFreeEncounter();
                    }

                    Events.ScheduleEvent(EVENT_CKECK_WG_TIMER, 20 * IN_MILLISECONDS);
                }
            }

        private:
            EventMap Events;
            ObjectGuid ArchavonGUID;
            ObjectGuid EmalonGUID;
            ObjectGuid KoralonGUID;
            ObjectGuid ToravonGUID;
            time_t ArchavonDeath;
            time_t EmalonDeath;
            time_t KoralonDeath;

            void DoStoneEncounter(uint32 timer)
            {
                ObjectGuid tmp[] = { ArchavonGUID, EmalonGUID, KoralonGUID, ToravonGUID };
                for (uint8 i = 0; i < 4; ++i)
                    if (Creature* boss = instance->GetCreature(tmp[i]))
                        if (boss->IsAlive())
                        {
                            if (!boss->HasAura(SPELL_STONED))
                                if (Aura* a = boss->AddAura(SPELL_STONED, boss))
                                    a->SetDuration(timer);

                            if (boss->IsInCombat())
                            {
                                boss->AI()->EnterEvadeMode();
                                Events.ScheduleEvent(EVENT_CKECK_WG_TIMER, 5 * IN_MILLISECONDS);
                            }
                        }
            }

            void DoFreeEncounter()
            {
                ObjectGuid tmp[] = { ArchavonGUID, EmalonGUID, KoralonGUID, ToravonGUID };
                for (uint8 i = 0; i < 4; ++i)
                    if (Creature* boss = instance->GetCreature(tmp[i]))
                        if (boss->IsAlive())
                            boss->RemoveAura(SPELL_STONED);
            }

            void DoCastWarningSpellOnPlayersVOA(uint32 timer)
            {
                for (Player& player : instance->GetPlayers())
                    if (!player.HasAura(SPELL_CLOSE_WARNING))
                        if (Aura* a = player.AddAura(SPELL_CLOSE_WARNING, &player))
                            a->SetDuration(timer);
            }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_vault_of_archavon_InstanceMapScript(map);
        }
};

void AddSC_instance_vault_of_archavon()
{
    new instance_vault_of_archavon();
}
