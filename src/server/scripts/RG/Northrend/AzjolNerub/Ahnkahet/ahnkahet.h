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

#ifndef DEF_AHNKAHET_H
#define DEF_AHNKAHET_H

#define AhnKahetScriptName "instance_ahnkahet"
#define DataHeader         "AK"

enum DataGuid
{
    DATA_ELDER_NADOX                = 0,
    DATA_PRINCE_TALDARAM,
    DATA_JEDOGA_SHADOWSEEKER,
    DATA_HERALD_VOLAZJ,
    DATA_AMANITAR,
    DATA_SPHERE_1,
    DATA_SPHERE_2,
    DATA_PRINCE_TALDARAM_PLATFORM,
    DATA_JEDOGA_VOLUNTEER
};

enum Data
{
    DATA_ELDER_NADOX_EVENT          = 0,
    DATA_PRINCE_TALDARAM_EVENT,
    DATA_JEDOGA_SHADOWSEEKER_EVENT,
    DATA_HERALD_VOLAZJ_EVENT,
    DATA_AMANITAR_EVENT,
    DATA_SPHERE1_EVENT,
    DATA_SPHERE2_EVENT,
    DATA_JEDOGA_RESET_INITIATES,
    DATA_JEDOGA_ALL_INITIATES_DEAD,
    DATA_JEDOGA_SPAWN_VOLUNTEERS,
    DATA_JEDOGA_RESET_VOLUNTEERS
};

enum GameObjectIds
{
    GO_PRINCE_TALDARAM_GATE     = 192236,
    GO_PRINCE_TALDARAM_PLATFORM = 193564,
    GO_SPHERE_1                 = 193093,
    GO_SPHERE_2                 = 193094
};

template<class AI>
AI* GetAhnKahetAI(Creature* creature)
{
    return GetInstanceAI<AI>(creature, AhnKahetScriptName);
}

#endif
