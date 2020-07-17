/*
 * Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
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

#ifndef DEF_SHEEP_OF_DEATH_H
#define DEF_SHEEP_OF_DEATH_H

enum Data
{
    DATA_SHEEP_DEATH,
    DATA_TOUCH,
    DATA_POINTS_CHANGE,
    DATA_ACTIVE,
    DATA_START,
    DATA_SPECTATOR_SAVED,
    DATA_PLAYER_CONTROLL,
    DATA_CREATURE_CONTROLL,
    DATA_SEND_POINTS,
};

enum Type
{
    DONE_SHEEP,
    DONE_SPECTATOR,
};

enum sodMisc
{
    ENTRY_BIG_SHEEP         = 1010672,
    ENTRY_LIL_SHEEP         = 1010670,
    ENTRY_MID_SHEEP         = 1010671,
    ENTRY_SPECTATOR         = 1010673,
    ENTRY_LIVE_SPARK        = 1010675,
    ENTRY_COMMANDER         = 1010674,

    GO_INVISABLE_WALL       = 180322,
    GO_SPEAR_WALL           = 179694,
    
    SPELL_SPECTATOR_VISUAL  = 47292,
    SPELL_SHEEP_VISUAL      = 37692,
    SPELL_SHEEP_EXPLOSION   = 64079,
    SPELL_SPEED_NERF        = 100007,


    GOSSIP_ITEM_ID          = 60068,
    GOSSIP_ITEM_PHASE       = 0,
    GOSSIP_ITEM_START       = 1,
    GOSSIP_ITEM_GUID        = 2,
    GOSSIP_ITEM_TIPS        = 3,
    GOSSIP_ITEM_SHOW_POINTS = 4,
    GOSSIP_COMMANDER_BASE   = 60067,
    GOSSIP_COMMANDER_GUIDE  = 60069,
    GOSSIP_COMMANDER_TIPS   = 60070,
    GOSSIP_PHASE_SETTER     = 60066,

    SPECIAL_FALSE           = 1,
    SPECIAL_TRUE            = 2,
};

#define SPAWN_TWO_CHANCE 30.0f
#define SPAWN_THREE_CHANCE 10.0f
#define MAX_SPEED 3.0f

#define ORIENTATION_SOUTH static_cast<float>(M_PI)
#define MID_LANE 1287.1199f //mide

Position const Positions[39] =
{
    //stones of bridge
    //right side
    {1919.76f, 1275.83f, 143.323f, 1.57696f},  //0
    {1924.11f, 1275.94f, 144.342f, 1.50520f},  //1
    {1928.50f, 1275.87f, 145.051f, 1.57054f},  //2
    {1932.91f, 1275.92f, 145.501f, 1.53377f},  //3
    {1937.29f, 1275.85f, 145.947f, 1.51806f},  //4
    {1941.66f, 1275.81f, 146.386f, 1.52591f},  //5
    {1945.88f, 1275.78f, 146.340f, 1.60053f},  //6
    {1950.33f, 1275.70f, 146.291f, 1.46701f},  //7
    {1954.75f, 1275.81f, 146.241f, 1.66728f},  //8
    {1958.95f, 1275.80f, 146.205f, 1.54948f},  //9
    {1963.40f, 1275.80f, 146.173f, 1.57696f},  //10
    {1967.75f, 1275.74f, 146.143f, 1.55733f},  //11
    {1972.09f, 1275.65f, 146.114f, 1.61623f},  //12
    {1976.34f, 1275.88f, 146.115f, 1.62801f},  //13
    //left side
    {1919.58f, 1298.86f, 143.279f, 4.77746f},  //14
    {1924.14f, 1299.00f, 144.349f, 4.68714f},  //15
    {1928.50f, 1298.96f, 145.051f, 4.80887f},  //16
    {1932.73f, 1298.93f, 145.482f, 4.67143f},  //17
    {1937.01f, 1298.89f, 145.919f, 4.69892f},  //18
    {1941.38f, 1298.82f, 146.365f, 4.69499f},  //19
    {1945.79f, 1299.00f, 146.340f, 4.82066f},  //20
    {1950.10f, 1298.95f, 146.292f, 4.76175f},  //21
    {1954.50f, 1298.84f, 146.245f, 4.73741f},  //22
    {1958.76f, 1298.91f, 146.205f, 4.76175f},  //23
    {1963.27f, 1299.00f, 146.174f, 4.66751f},  //24
    {1967.59f, 1298.94f, 146.144f, 4.70285f},  //25
    {1971.92f, 1298.90f, 146.115f, 4.70285f},  //26
    {1976.39f, 1298.85f, 146.115f, 4.68321f},  //27
    //spawn positions
    {1942.57f, MID_LANE, 145.80f, ORIENTATION_SOUTH}, //28
    //Player port position
    {1920.86f, MID_LANE, 142.930f, 6.2556f}, //29
    //spear wall positions
    {1912.45f, 1286.72f, 142.104f, 1.62749f}, //30
    {1912.21f, 1296.05f, 142.381f, 1.56466f}, //31
    {1912.41f, 1279.51f, 142.184f, 1.61178f}, //32
    //invis wall positions
    {1914.55f, MID_LANE, 130.842f, ORIENTATION_SOUTH},          //33 south end of bridge
    {1924.62f, MID_LANE, 130.842f, ORIENTATION_SOUTH},          //34 north end of bridge
    {1914.49f, (MID_LANE + 11.0f), 130.842f, static_cast<float>(M_PI/2)},    //35 left side of bridge game area
    {1924.62f, (MID_LANE + 11.0f), 130.842f, ORIENTATION_SOUTH},//36 left side of bridge 
    {1914.49f, (MID_LANE - 11.0f), 130.842f, static_cast<float>(M_PI/2)},    //37 right side of bridge game area
    {1924.62f, (MID_LANE - 11.0f), 130.842f, ORIENTATION_SOUTH},//38 right side of bridge
};

#endif
