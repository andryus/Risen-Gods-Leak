/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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
#include "InstanceScript.h"
#include "forge_of_souls.h"
#include "Player.h"

#define MAX_ENCOUNTER 2

/* Forge of Souls encounters:
0- Bronjahm, The Godfather of Souls
1- The Devourer of Souls
*/

class instance_forge_of_souls : public InstanceMapScript
{
    public:
        instance_forge_of_souls() : InstanceMapScript(FoSScriptName, 632) { }

        struct instance_forge_of_souls_InstanceScript : public InstanceScript
        {
            instance_forge_of_souls_InstanceScript(Map* map) : InstanceScript(map)
            {
                SetHeaders(DataHeader);
                SetBossNumber(MAX_ENCOUNTER);

                teamInInstance = 0;
            }

            void OnCreatureCreate(Creature* creature)
            {
                for (Player& player : instance->GetPlayers())
                    teamInInstance = player.GetTeam();

                creature->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, true);

                switch (creature->GetEntry())
                {
                    case CREATURE_BRONJAHM:
                        bronjahm = creature->GetGUID();
                        break;
                    case CREATURE_DEVOURER:
                        devourerOfSouls = creature->GetGUID();
                        break;
                    case NPC_SYLVANAS_PART1:
                        if (teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_JAINA_PART1);
                        break;
                    case NPC_SYLVANAS_PART2:
                        if (teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_JAINA_PART2);
                        break;
                    case NPC_LORALEN:
                        if (teamInInstance == ALLIANCE)
                        {
                            creature->UpdateEntry(NPC_ELANDRA);
                            creature->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                        }                            
                        creature->SetVisible(false);
                        break;
                    case NPC_KALIRA:
                        if (teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_KORELN);
                        creature->SetVisible(false);
                        break;
                    case NPC_CHAMPION_1_HORDE:
                        if (teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_CHAMPION_1_ALLIANCE);
                        creature->SetVisible(false);
                        break;
                    case NPC_CHAMPION_2_HORDE:
                        if (teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_CHAMPION_2_ALLIANCE);
                        creature->SetVisible(false);
                        break;
                    case NPC_CHAMPION_3_HORDE:
                        if (teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_CHAMPION_3_ALLIANCE);
                        creature->SetVisible(false);
                        break;
                }
            }

            uint32 GetData(uint32 type) const
            {
                switch (type)
                {
                    case DATA_TEAM_IN_INSTANCE:
                        return teamInInstance;
                    default:
                        break;
                }

                return 0;
            }

            ObjectGuid GetGuidData(uint32 type) const
            {
                switch (type)
                {
                    case DATA_BRONJAHM:
                        return bronjahm;
                    case DATA_DEVOURER:
                        return devourerOfSouls;
                    default:
                        break;
                }

                return ObjectGuid::Empty;
            }

        private:
            ObjectGuid bronjahm;
            ObjectGuid devourerOfSouls;

            uint32 teamInInstance;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_forge_of_souls_InstanceScript(map);
        }
};

void AddSC_instance_forge_of_souls()
{
    new instance_forge_of_souls();
}
