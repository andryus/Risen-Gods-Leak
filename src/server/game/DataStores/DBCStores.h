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

#ifndef TRINITY_DBCSTORES_H
#define TRINITY_DBCSTORES_H

#include "Common.h"
#include "DBCStore.h"
#include "DBCStructure.h"

#include <list>

typedef std::list<uint32> SimpleFactionsList;
GAME_API SimpleFactionsList const* GetFactionTeamList(uint32 faction);

GAME_API char* GetPetName(uint32 petfamily, uint32 dbclang);
GAME_API uint32 GetTalentSpellCost(uint32 spellId);
GAME_API TalentSpellPos const* GetTalentSpellPos(uint32 spellId);

GAME_API char const* GetRaceName(uint8 race, uint8 locale);
GAME_API char const* GetClassName(uint8 class_, uint8 locale);

GAME_API WMOAreaTableEntry const* GetWMOAreaTableEntryByTripple(int32 rootid, int32 adtid, int32 groupid);

GAME_API uint32 GetVirtualMapForMapAndZone(uint32 mapid, uint32 zoneId);

enum ContentLevels : uint8
{
    CONTENT_1_60 = 0,
    CONTENT_61_70,
    CONTENT_71_80
};
GAME_API ContentLevels GetContentLevelsForMapAndZone(uint32 mapid, uint32 zoneId);

GAME_API bool IsTotemCategoryCompatiableWith(uint32 itemTotemCategoryId, uint32 requiredTotemCategoryId);

GAME_API void Zone2MapCoordinates(float &x, float &y, uint32 zone);
GAME_API void Map2ZoneCoordinates(float &x, float &y, uint32 zone);

typedef std::map<uint32/*pair32(map, diff)*/, MapDifficulty> MapDifficultyMap;
GAME_API MapDifficulty const* GetMapDifficultyData(uint32 mapId, Difficulty difficulty);
GAME_API MapDifficulty const* GetDownscaledMapDifficultyData(uint32 mapId, Difficulty &difficulty);

GAME_API uint32 const* /*[MAX_TALENT_TABS]*/ GetTalentTabPages(uint8 cls);

GAME_API uint32 GetLiquidFlags(uint32 liquidType);

GAME_API PvPDifficultyEntry const* GetBattlegroundBracketByLevel(uint32 mapid, uint32 level);
GAME_API PvPDifficultyEntry const* GetBattlegroundBracketById(uint32 mapid, BattlegroundBracketId id);

GAME_API CharStartOutfitEntry const* GetCharStartOutfitEntry(uint8 race, uint8 class_, uint8 gender);

GAME_API LFGDungeonEntry const* GetLFGDungeon(uint32 mapId, Difficulty difficulty);

GAME_API uint32 GetDefaultMapLight(uint32 mapId);

typedef std::unordered_multimap<uint32, SkillRaceClassInfoEntry const*> SkillRaceClassInfoMap;
typedef std::pair<SkillRaceClassInfoMap::iterator, SkillRaceClassInfoMap::iterator> SkillRaceClassInfoBounds;
GAME_API SkillRaceClassInfoEntry const* GetSkillRaceClassInfo(uint32 skill, uint8 race, uint8 class_);

GAME_API EmotesTextSoundEntry const* FindTextSoundEmoteFor(uint32 emote, uint32 race, uint32 gender);

GAME_API extern DBCStorage <AchievementEntry>             sAchievementStore;
GAME_API extern DBCStorage <AchievementCriteriaEntry>     sAchievementCriteriaStore;
GAME_API extern DBCStorage <AreaTableEntry>               sAreaTableStore;
GAME_API extern DBCStorage <AreaGroupEntry>               sAreaGroupStore;
GAME_API extern DBCStorage <AreaPOIEntry>                 sAreaPOIStore;
GAME_API extern DBCStorage <AreaTriggerEntry>             sAreaTriggerStore;
GAME_API extern DBCStorage <AuctionHouseEntry>            sAuctionHouseStore;
GAME_API extern DBCStorage <BankBagSlotPricesEntry>       sBankBagSlotPricesStore;
GAME_API extern DBCStorage <BannedAddOnsEntry>            sBannedAddOnsStore;
GAME_API extern DBCStorage <BarberShopStyleEntry>         sBarberShopStyleStore;
GAME_API extern DBCStorage <BattlemasterListEntry>        sBattlemasterListStore;
GAME_API extern DBCStorage <ChatChannelsEntry>            sChatChannelsStore;
GAME_API extern DBCStorage <CharStartOutfitEntry>         sCharStartOutfitStore;
GAME_API extern DBCStorage <CharTitlesEntry>              sCharTitlesStore;
GAME_API extern DBCStorage <ChrClassesEntry>              sChrClassesStore;
GAME_API extern DBCStorage <ChrRacesEntry>                sChrRacesStore;
GAME_API extern DBCStorage <CinematicSequencesEntry>      sCinematicSequencesStore;
GAME_API extern DBCStorage <CreatureDisplayInfoEntry>     sCreatureDisplayInfoStore;
GAME_API extern DBCStorage <CreatureDisplayInfoExtraEntry> sCreatureDisplayInfoExtraStore;
GAME_API extern DBCStorage <CreatureFamilyEntry>          sCreatureFamilyStore;
GAME_API extern DBCStorage <CreatureModelDataEntry>       sCreatureModelDataStore;
GAME_API extern DBCStorage <CreatureSpellDataEntry>       sCreatureSpellDataStore;
GAME_API extern DBCStorage <CreatureTypeEntry>            sCreatureTypeStore;
GAME_API extern DBCStorage <CurrencyTypesEntry>           sCurrencyTypesStore;
GAME_API extern DBCStorage <DestructibleModelDataEntry>   sDestructibleModelDataStore;
GAME_API extern DBCStorage <DungeonEncounterEntry>        sDungeonEncounterStore;
GAME_API extern DBCStorage <DurabilityCostsEntry>         sDurabilityCostsStore;
GAME_API extern DBCStorage <DurabilityQualityEntry>       sDurabilityQualityStore;
GAME_API extern DBCStorage <EmotesEntry>                  sEmotesStore;
GAME_API extern DBCStorage <EmotesTextEntry>              sEmotesTextStore;
GAME_API extern DBCStorage <EmotesTextSoundEntry>         sEmotesTextSoundStore;
GAME_API extern DBCStorage <FactionEntry>                 sFactionStore;
GAME_API extern DBCStorage <FactionTemplateEntry>         sFactionTemplateStore;
GAME_API extern DBCStorage <GameObjectDisplayInfoEntry>   sGameObjectDisplayInfoStore;
GAME_API extern DBCStorage <GemPropertiesEntry>           sGemPropertiesStore;
GAME_API extern DBCStorage <GlyphPropertiesEntry>         sGlyphPropertiesStore;
GAME_API extern DBCStorage <GlyphSlotEntry>               sGlyphSlotStore;

GAME_API extern DBCStorage <GtBarberShopCostBaseEntry>    sGtBarberShopCostBaseStore;
GAME_API extern DBCStorage <GtCombatRatingsEntry>         sGtCombatRatingsStore;
GAME_API extern DBCStorage <GtChanceToMeleeCritBaseEntry> sGtChanceToMeleeCritBaseStore;
GAME_API extern DBCStorage <GtChanceToMeleeCritEntry>     sGtChanceToMeleeCritStore;
GAME_API extern DBCStorage <GtChanceToSpellCritBaseEntry> sGtChanceToSpellCritBaseStore;
GAME_API extern DBCStorage <GtChanceToSpellCritEntry>     sGtChanceToSpellCritStore;
GAME_API extern DBCStorage <GtNPCManaCostScalerEntry>     sGtNPCManaCostScalerStore;
GAME_API extern DBCStorage <GtOCTClassCombatRatingScalarEntry> sGtOCTClassCombatRatingScalarStore;
GAME_API extern DBCStorage <GtOCTRegenHPEntry>            sGtOCTRegenHPStore;
//extern DBCStorage <GtOCTRegenMPEntry>            sGtOCTRegenMPStore; -- not used currently
GAME_API extern DBCStorage <GtRegenHPPerSptEntry>         sGtRegenHPPerSptStore;
GAME_API extern DBCStorage <GtRegenMPPerSptEntry>         sGtRegenMPPerSptStore;
GAME_API extern DBCStorage <HolidaysEntry>                sHolidaysStore;
GAME_API extern DBCStorage <ItemEntry>                    sItemStore;
GAME_API extern DBCStorage <ItemBagFamilyEntry>           sItemBagFamilyStore;
//extern DBCStorage <ItemDisplayInfoEntry>      sItemDisplayInfoStore; -- not used currently
GAME_API extern DBCStorage <ItemExtendedCostEntry>        sItemExtendedCostStore;
GAME_API extern DBCStorage <ItemLimitCategoryEntry>       sItemLimitCategoryStore;
GAME_API extern DBCStorage <ItemRandomPropertiesEntry>    sItemRandomPropertiesStore;
GAME_API extern DBCStorage <ItemRandomSuffixEntry>        sItemRandomSuffixStore;
GAME_API extern DBCStorage <ItemSetEntry>                 sItemSetStore;
GAME_API extern DBCStorage <LFGDungeonEntry>              sLFGDungeonStore;
GAME_API extern DBCStorage <LiquidTypeEntry>              sLiquidTypeStore;
GAME_API extern DBCStorage <LockEntry>                    sLockStore;
GAME_API extern DBCStorage <MailTemplateEntry>            sMailTemplateStore;
GAME_API extern DBCStorage <MapEntry>                     sMapStore;
//extern DBCStorage <MapDifficultyEntry>           sMapDifficultyStore; -- use GetMapDifficultyData insteed
GAME_API extern MapDifficultyMap                          sMapDifficultyMap;
GAME_API extern DBCStorage <MovieEntry>                   sMovieStore;
GAME_API extern DBCStorage <OverrideSpellDataEntry>       sOverrideSpellDataStore;
GAME_API extern DBCStorage <PowerDisplayEntry>            sPowerDisplayStore;
GAME_API extern DBCStorage <QuestSortEntry>               sQuestSortStore;
GAME_API extern DBCStorage <QuestXPEntry>                 sQuestXPStore;
GAME_API extern DBCStorage <QuestFactionRewEntry>         sQuestFactionRewardStore;
GAME_API extern DBCStorage <RandomPropertiesPointsEntry>  sRandomPropertiesPointsStore;
GAME_API extern DBCStorage <ScalingStatDistributionEntry> sScalingStatDistributionStore;
GAME_API extern DBCStorage <ScalingStatValuesEntry>       sScalingStatValuesStore;
GAME_API extern DBCStorage <SkillLineEntry>               sSkillLineStore;
GAME_API extern DBCStorage <SkillLineAbilityEntry>        sSkillLineAbilityStore;
GAME_API extern DBCStorage <SkillTiersEntry>              sSkillTiersStore;
GAME_API extern DBCStorage <SoundEntriesEntry>            sSoundEntriesStore;
GAME_API extern DBCStorage <SpellCastTimesEntry>          sSpellCastTimesStore;
GAME_API extern DBCStorage <SpellCategoryEntry>           sSpellCategoryStore;
GAME_API extern DBCStorage <SpellDifficultyEntry>         sSpellDifficultyStore;
GAME_API extern DBCStorage <SpellDurationEntry>           sSpellDurationStore;
GAME_API extern DBCStorage <SpellFocusObjectEntry>        sSpellFocusObjectStore;
GAME_API extern DBCStorage <SpellItemEnchantmentEntry>    sSpellItemEnchantmentStore;
GAME_API extern DBCStorage <SpellItemEnchantmentConditionEntry> sSpellItemEnchantmentConditionStore;
GAME_API extern SpellCategoryStore                        sSpellsByCategoryStore;
GAME_API extern PetFamilySpellsStore                      sPetFamilySpellsStore;
GAME_API extern DBCStorage <SpellRadiusEntry>             sSpellRadiusStore;
GAME_API extern DBCStorage <SpellRangeEntry>              sSpellRangeStore;
GAME_API extern DBCStorage <SpellRuneCostEntry>           sSpellRuneCostStore;
GAME_API extern DBCStorage <SpellShapeshiftEntry>         sSpellShapeshiftStore;
GAME_API extern DBCStorage <SpellEntry>                   sSpellStore;
GAME_API extern DBCStorage <StableSlotPricesEntry>        sStableSlotPricesStore;
GAME_API extern DBCStorage <SummonPropertiesEntry>        sSummonPropertiesStore;
GAME_API extern DBCStorage <TalentEntry>                  sTalentStore;
GAME_API extern DBCStorage <TalentTabEntry>               sTalentTabStore;
GAME_API extern DBCStorage <TaxiNodesEntry>               sTaxiNodesStore;
GAME_API extern DBCStorage <TaxiPathEntry>                sTaxiPathStore;
GAME_API extern TaxiMask                                  sTaxiNodesMask;
GAME_API extern TaxiMask                                  sOldContinentsNodesMask;
GAME_API extern TaxiMask                                  sHordeTaxiNodesMask;
GAME_API extern TaxiMask                                  sAllianceTaxiNodesMask;
GAME_API extern TaxiMask                                  sDeathKnightTaxiNodesMask;
GAME_API extern TaxiPathSetBySource                       sTaxiPathSetBySource;
GAME_API extern TaxiPathNodesByPath                       sTaxiPathNodesByPath;
GAME_API extern DBCStorage <TeamContributionPointsEntry>  sTeamContributionPointsStore;
GAME_API extern DBCStorage <TotemCategoryEntry>           sTotemCategoryStore;
GAME_API extern DBCStorage <VehicleEntry>                 sVehicleStore;
GAME_API extern DBCStorage <VehicleSeatEntry>             sVehicleSeatStore;
GAME_API extern DBCStorage <WMOAreaTableEntry>            sWMOAreaTableStore;
//extern DBCStorage <WorldMapAreaEntry>           sWorldMapAreaStore; -- use Zone2MapCoordinates and Map2ZoneCoordinates
GAME_API extern DBCStorage <WorldMapOverlayEntry>         sWorldMapOverlayStore;
GAME_API extern DBCStorage <WorldSafeLocsEntry>           sWorldSafeLocsStore;

void LoadDBCStores(const std::string& dataPath);

#endif
