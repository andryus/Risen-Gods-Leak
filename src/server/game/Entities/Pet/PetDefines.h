/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#ifndef TRINITYCORE_PET_DEFINES_H
#define TRINITYCORE_PET_DEFINES_H

enum PetType
{
    SUMMON_PET                          = 0,
    HUNTER_PET                          = 1,
    MAX_PET_TYPE                        = 4
};

#define MAX_PET_STABLES                 4

// stored in character_pet.slot
enum PetSaveMode
{
    PET_SAVE_AS_DELETED                 = -1,                        // not saved in fact
    PET_SAVE_AS_CURRENT                 =  0,                        // in current slot (with player)
    PET_SAVE_FIRST_STABLE_SLOT          =  1,
    PET_SAVE_LAST_STABLE_SLOT           =  MAX_PET_STABLES,          // last in DB stable slot index (including), all higher have same meaning as PET_SAVE_NOT_IN_SLOT
    PET_SAVE_NOT_IN_SLOT                =  100                       // for avoid conflict with stable size grow will use 100
};

enum HappinessState
{
    UNHAPPY                             = 1,
    CONTENT                             = 2,
    HAPPY                               = 3
};

enum PetSpellState
{
    PETSPELL_UNCHANGED                  = 0,
    PETSPELL_CHANGED                    = 1,
    PETSPELL_NEW                        = 2,
    PETSPELL_REMOVED                    = 3
};

enum PetSpellType
{
    PETSPELL_NORMAL                     = 0,
    PETSPELL_FAMILY                     = 1,
    PETSPELL_TALENT                     = 2
};

enum ActionFeedback
{
    FEEDBACK_NONE                       = 0,
    FEEDBACK_PET_DEAD                   = 1,
    FEEDBACK_NOTHING_TO_ATT             = 2,
    FEEDBACK_CANT_ATT_TARGET            = 3
};

enum PetTalk
{
    PET_TALK_SPECIAL_SPELL              = 0,
    PET_TALK_ATTACK                     = 1
};

enum PetTypes
{
    // Hunter
    NPC_TRAP_SNAKE_VENOMUS              = 19833,
    NPC_TRAP_SNAKE_VIPER                = 19921,
    // Warlock                          
    NPC_FELGUARD                        = 17252,
    NPC_VOIDWALKER                      = 1860,
    NPC_FELHUNTER                       = 417,
    NPC_SUCCUBUS                        = 1863,
    NPC_IMP                             = 416,
    NPC_INFERNAL                        = 89,
    NPC_DOOMGUARD                       = 11859,
    // Mage                             
    NPC_WATER_ELEMENTAL                 = 510,
    NPC_WATER_ELEMENTAL_P               = 37994,
    NPC_MIRROR_IMAGE                    = 31216,
    // Druid                            
    NPC_TREANT                          = 1964,
    // Shaman                           
    NPC_EARTH_ELEMENTAL                 = 15352,
    NPC_FIRE_ELEMENTAL                  = 15438,
    // Death Knight                     
    NPC_GHOUL                           = 26125,
    NPC_EBON_GARGOYLE                   = 27829,
    NPC_BLOODWORM                       = 28017,
    NPC_ARMY_OF_DEAD_GHOUL              = 24207,
    NPC_DANCING_RUNE_WEAPON             = 27893,
    // Shaman                           
    NPC_FERAL_SPIRIT                    = 29264,
    // Priest                           
    NPC_SHADOWNFIEND                    = 19668
};                                      
                                        
enum GeneralPetCalculate
{
	SPELL_TAMED_PET_PASSIVE_08_HIT      = 34666,
	SPELL_TAMED_PET_PASSIVE_11          = 55566, // is removed from pet
	SPELL_TAMED_PET_PASSIVE_DND_RESIST  = 19007, // is removed from pet
	SPELL_TAMED_PET_PASSIVE_06          = 19591,
	SPELL_TAMED_PET_PASSIVE_07          = 20784,
	SPELL_TAMED_PET_PASSIVE_09          = 34667
};

enum HunterPetCalculate
{
	SPELL_HUNTER_PET_SCALING_04         = 61017,
	SPELL_HUNTER_PET_SCALING_01         = 34902,
	SPELL_HUNTER_PET_SCALING_02         = 34903,
	SPELL_HUNTER_PET_SCALING_03         = 34904,
	SPELL_HUNTER_ANIMAL_HANDLER         = 34453
};

enum WarlockPetCalculate
{
	SPELL_WARLOCK_PET_SCALING_05        = 61013,

	SPELL_WARLOCK_DEMONIC_KNOWLEGE_R1   = 35691,
	SPELL_WARLOCK_DEMONIC_KNOWLEGE      = 35696,
	SPELL_DEMONIC_IMMOLATION_PASSIVE    = 4525,

	// unused                           
	SPELL_PET_PASSIVE_CRIT              = 35695,
	SPELL_PET_PASSIVE_DAMAGE_TAKEN      = 35697,
	SPELL_WARLOCK_PET_SCALING_01        = 34947,
	SPELL_WARLOCK_PET_SCALING_02        = 34956,
	SPELL_WARLOCK_PET_SCALING_03        = 34957,
	SPELL_WARLOCK_PET_SCALING_04        = 34958,
	SPELL_WARLOCK_GLYPH_OF_VOIDWALKER   = 56247,
	SPELL_GLYPH_OF_FELGUARD             = 56246,
	SPELL_INFERNAL_SCALING_01           = 36186,
	SPELL_INFERNAL_SCALING_02           = 36188,
	SPELL_INFERNAL_SCALING_03           = 36189,
	SPELL_INFERNAL_SCALING_04           = 36190,
	SPELL_RITUAL_ENSLAVEMENT            = 22987
};

enum PriestPetCalculate
{
	SPELL_MANA_LEECH                    = 28305,
	SPELL_SHADOWFIEND_SCALING_01        = 35661,
	SPELL_SHADOWFIEND_SCALING_02        = 35662,
	SPELL_SHADOWFIEND_SCALING_03        = 35663,
	SPELL_SHADOWFIEND_SCALING_04        = 35664
};

enum DKPetCalculate
{
	SPELL_DEATH_KNIGHT_PET_SCALING_03   = 61697,
	SPELL_DEATH_KNIGHT_AOEDAMAGE_REDUCE = 62137,
	SPELL_NIGHT_OF_THE_DEAD             = 55620,
	SPELL_ORC_RACIAL_COMMAND            = 65221,
	SPELL_DEATH_KNIGHT_RUNE_WEAPON_02   = 51906,
	SPELL_DEATH_KNIGHT_PET_SCALING_01   = 54566,
	SPELL_DEATH_KNIGHT_PET_SCALING_02   = 51996
};

enum MagePetCalculate
{
	SPELL_MAGE_PET_SCALING_01           = 35657,
	SPELL_MAGE_PET_SCALING_02           = 35658,
	SPELL_MAGE_PET_SCALING_03           = 35659,
	SPELL_MAGE_PET_SCALING_04           = 35660
};

enum DruidPetCalculate
{
	SPELL_TREANT_SCALING_01             = 35669,
	SPELL_TREANT_SCALING_02             = 35670,
	SPELL_TREANT_SCALING_03             = 35671,
	SPELL_TREANT_SCALING_04             = 35672
};

enum ShamanPetCalculate
{
	SPELL_FIRE_ELEMENTAL_SCALING_01     = 35665,
	SPELL_FIRE_ELEMENTAL_SCALING_02     = 35666,
	SPELL_FIRE_ELEMENTAL_SCALING_03     = 35667,
	SPELL_FIRE_ELEMENTAL_SCALING_04     = 35668,

	SPELL_FERAL_SPIRIT_PET_SCALING_04   = 61783,
	SPELL_FERAL_SPIRIT_SPIRIT_HUNT      = 58877,
	SPELL_FERAL_SPIRIT_SCALING_01       = 35674,
	SPELL_FERAL_SPIRIT_SCALING_02       = 35675,
	SPELL_FERAL_SPIRIT_SCALING_03       = 35676,

	SPELL_EARTH_ELEMENTAL_SCALING_01    = 65225,
	SPELL_EARTH_ELEMENTAL_SCALING_02    = 65226,
	SPELL_EARTH_ELEMENTAL_SCALING_03    = 65227,
	SPELL_EARTH_ELEMENTAL_SCALING_04    = 65228
};

enum MiscPetCalculate
{
	SPELL_MAGE_PET_PASSIVE_ELEMENTAL    = 44559,
	SPELL_PET_HEALTH_SCALING            = 61679,
	SPELL_PET_UNK_01                    = 67561,
	SPELL_PET_UNK_02                    = 67557
};

enum PetMisc
{
	SPELL_AVOIDANCE                     = 32233,
	SPELL_PET_BLOODLUST                 = 2825,
	SPELL_PET_HEROISM                   = 32182
};

#define PET_FOLLOW_DIST  1.0f
#define PET_FOLLOW_ANGLE static_cast<float>(M_PI_2)

#endif
