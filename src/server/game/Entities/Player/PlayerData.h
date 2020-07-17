#pragma once

#include <string>

#include "Define.h"
#include "WorldPacket.h"


////////////////////////////////////////////////////////////
// Contains data that is not required to be defined in Player.h
// TODO: move other structs from Player.h to here
////////////////////////////////////////////////////////////


// Proxy structure to contain data passed to callback function,
// only to prevent bloating the parameter list
struct CharacterCreateInfo
{
    CharacterCreateInfo(std::string_view name, uint8 race, uint8 cclass, uint8 gender, uint8 skin, uint8 face, uint8 hairStyle, uint8 hairColor, uint8 facialHair, uint8 outfitId,
        WorldPacket& data) : Name(name), Race(race), Class(cclass), Gender(gender), Skin(skin), Face(face), HairStyle(hairStyle), HairColor(hairColor), FacialHair(facialHair),
        OutfitId(outfitId), Data(std::move(data)), CharCount(0)
    { }

    /// User specified variables
    std::string Name;
    uint8 Race;
    uint8 Class;
    uint8 Gender;
    uint8 Skin;
    uint8 Face;
    uint8 HairStyle;
    uint8 HairColor;
    uint8 FacialHair;
    uint8 OutfitId;
    WorldPacket Data;

    /// Server side data
    uint8 CharCount;
    uint8 MaxLevel;
    uint8 MaxRealmLevel;
};