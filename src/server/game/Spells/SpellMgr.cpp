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

#include "SpellMgr.h"
#include "SpellInfo.h"
#include "ObjectMgr.h"
#include "SpellAuraDefines.h"
#include "SharedDefines.h"
#include "DBCStores.h"
#include "Chat.h"
#include "BattlegroundMgr.h"
#include "BattlefieldWG.h"
#include "BattlefieldMgr.h"
#include "InstanceScript.h"
#include "Player.h"

bool IsPrimaryProfessionSkill(uint32 skill)
{
    SkillLineEntry const* pSkill = sSkillLineStore.LookupEntry(skill);
    if (!pSkill)
        return false;

    if (pSkill->categoryId != SKILL_CATEGORY_PROFESSION)
        return false;

    return true;
}

bool IsPartOfSkillLine(uint32 skillId, uint32 spellId)
{
    SkillLineAbilityMapBounds skillBounds = sSpellMgr->GetSkillLineAbilityMapBounds(spellId);
    for (SkillLineAbilityMap::const_iterator itr = skillBounds.first; itr != skillBounds.second; ++itr)
        if (itr->second->skillId == skillId)
            return true;

    return false;
}

DiminishingGroup GetDiminishingReturnsGroupForSpell(SpellInfo const* spellproto, bool triggered)
{
    if (spellproto->IsPositive())
        return DIMINISHING_NONE;

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (spellproto->Effects[i].ApplyAuraName == SPELL_AURA_MOD_TAUNT)
            return DIMINISHING_TAUNT;
    }

    // Explicit Diminishing Groups
    switch (spellproto->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            // Pet charge effects (Infernal Awakening, Demon Charge)
            if (spellproto->SpellVisual[0] == 2816 && spellproto->SpellIconID == 15)
                return DIMINISHING_CONTROLLED_STUN;
            // Frost Tomb
            else if (spellproto->Id == 48400)
                return DIMINISHING_NONE;
            // Gnaw
            else if (spellproto->Id == 47481)
                return DIMINISHING_CONTROLLED_STUN;
            // ToC Icehowl Arctic Breath
            else if (spellproto->SpellVisual[0] == 14153)
                return DIMINISHING_NONE;
            // Black Plague
            else if (spellproto->Id == 64155)
                return DIMINISHING_NONE;
            else if (spellproto->Id == 32752) // Summoning Disorientation
                return DIMINISHING_NONE;
            else if (spellproto->Id == 66407) // toc/snobold - head crack
                return DIMINISHING_NONE;
            else if (spellproto->Id == 25046) // Arcane Torrent
                return DIMINISHING_NONE;
            else if (spellproto->Id == 28730) // Arcane Torrent
                return DIMINISHING_NONE;
            else if (spellproto->Id == 50613) // Arcane Torrent
                return DIMINISHING_NONE;
            else if (spellproto->Id == 51750) // Screams of the Dead (King Ymiron)
                return DIMINISHING_NONE;
            break;
        }
        // Event spells
        case SPELLFAMILY_UNK1:
            return DIMINISHING_NONE;
        case SPELLFAMILY_MAGE:
        {
            // Frostbite
            if (spellproto->SpellFamilyFlags[1] & 0x80000000)
                return DIMINISHING_ROOT;
            // Shattered Barrier
            else if (spellproto->SpellVisual[0] == 12297)
                return DIMINISHING_ROOT;
            // Deep Freeze
            else if (spellproto->SpellIconID == 2939 && spellproto->SpellVisual[0] == 9963)
                return DIMINISHING_CONTROLLED_STUN;
            // Frost Nova / Freeze (Water Elemental)
            else if (spellproto->SpellIconID == 193)
                return DIMINISHING_CONTROLLED_ROOT;
            // Dragon's Breath
            else if (spellproto->SpellFamilyFlags[0] & 0x800000)
                return DIMINISHING_DRAGONS_BREATH;
            // BWL - Nefarian: Mage ClassCall / Wild Polymorph
            else if (spellproto->Id == 23603)
                return DIMINISHING_NONE;
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            // Hamstring - limit duration to 10s in PvP
            if (spellproto->SpellFamilyFlags[0] & 0x2)
                return DIMINISHING_LIMITONLY;
            // Charge Stun (own diminishing)
            else if (spellproto->SpellFamilyFlags[0] & 0x01000000)
                return DIMINISHING_CHARGE;
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // Curses/etc
            if ((spellproto->SpellFamilyFlags[0] & 0x80000000) || (spellproto->SpellFamilyFlags[1] & 0x200))
                return DIMINISHING_LIMITONLY;
            // Seduction
            else if (spellproto->SpellFamilyFlags[1] & 0x10000000)
                return DIMINISHING_FEAR;
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            // Pounce
            if (spellproto->SpellFamilyFlags[0] & 0x20000)
                return DIMINISHING_OPENING_STUN;
            // Cyclone
            else if (spellproto->SpellFamilyFlags[1] & 0x20)
                return DIMINISHING_CYCLONE;
            // Entangling Roots
            // Nature's Grasp
            else if (spellproto->SpellFamilyFlags[0] & 0x00000200)
                return DIMINISHING_CONTROLLED_ROOT;
            // Faerie Fire
            else if (spellproto->SpellFamilyFlags[0] & 0x400)
                return DIMINISHING_LIMITONLY;
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            // Gouge
            if (spellproto->SpellFamilyFlags[0] & 0x8)
                return DIMINISHING_DISORIENT;
            // Blind
            else if (spellproto->Id == 2094)
                return DIMINISHING_FEAR;
            // Cheap Shot
            else if (spellproto->SpellFamilyFlags[0] & 0x400)
                return DIMINISHING_OPENING_STUN;
            // Crippling poison - Limit to 10 seconds in PvP (No SpellFamilyFlags)
            else if (spellproto->SpellIconID == 163)
                return DIMINISHING_LIMITONLY;
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // Hunter's Mark
            if ((spellproto->SpellFamilyFlags[0] & 0x400) && spellproto->SpellIconID == 538)
                return DIMINISHING_LIMITONLY;
            // Scatter Shot (own diminishing)
            else if ((spellproto->SpellFamilyFlags[0] & 0x40000) && spellproto->SpellIconID == 132)
                return DIMINISHING_SCATTER_SHOT;
            // Entrapment (own diminishing)
            else if (spellproto->SpellVisual[0] == 7484 && spellproto->SpellIconID == 20)
                return DIMINISHING_ENTRAPMENT;
            // Wyvern Sting mechanic is MECHANIC_SLEEP but the diminishing is DIMINISHING_DISORIENT
            else if ((spellproto->SpellFamilyFlags[1] & 0x1000) && spellproto->SpellIconID == 1721)
                return DIMINISHING_DISORIENT;
            // Freezing Arrow
            else if (spellproto->SpellFamilyFlags[0] & 0x8)
                return DIMINISHING_DISORIENT;
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            // Judgement of Justice - limit duration to 10s in PvP
            if (spellproto->SpellFamilyFlags[0] & 0x100000)
                return DIMINISHING_LIMITONLY;
            // Turn Evil
            else if ((spellproto->SpellFamilyFlags[1] & 0x804000) && spellproto->SpellIconID == 309)
                return DIMINISHING_FEAR;
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            // Storm, Earth and Fire - Earthgrab
            if (spellproto->SpellFamilyFlags[2] & 0x4000)
                return DIMINISHING_NONE;
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Hungering Cold (no flags)
            if (spellproto->SpellIconID == 2797)
                return DIMINISHING_DISORIENT;
            // Mark of Blood
            else if ((spellproto->SpellFamilyFlags[0] & 0x10000000) && spellproto->SpellIconID == 2285)
                return DIMINISHING_LIMITONLY;
            break;
        }
        case SPELLFAMILY_POTION:
        {
            if (spellproto->Id == 15822) // Dreamless Sleep Potion
                return DIMINISHING_NONE;
            break;
        }
        default:
            break;
    }

    // Lastly - Set diminishing depending on mechanic
    uint32 mechanic = spellproto->GetAllEffectsMechanicMask();
    if (mechanic & (1 << MECHANIC_CHARM))
        return DIMINISHING_MIND_CONTROL;
    if (mechanic & (1 << MECHANIC_SILENCE))
        return DIMINISHING_SILENCE;
    if (mechanic & (1 << MECHANIC_SLEEP))
        return DIMINISHING_SLEEP;
    if (mechanic & ((1 << MECHANIC_SAPPED) | (1 << MECHANIC_POLYMORPH) | (1 << MECHANIC_SHACKLE)))
        return DIMINISHING_DISORIENT;
    // Mechanic Knockout, except Blast Wave
    if (mechanic & (1 << MECHANIC_KNOCKOUT) && spellproto->SpellIconID != 292)
        return DIMINISHING_DISORIENT;
    if (mechanic & (1 << MECHANIC_DISARM))
        return DIMINISHING_DISARM;
    if (mechanic & (1 << MECHANIC_FEAR))
        return DIMINISHING_FEAR;
    if (mechanic & (1 << MECHANIC_STUN))
        return triggered ? DIMINISHING_STUN : DIMINISHING_CONTROLLED_STUN;
    if (mechanic & (1 << MECHANIC_BANISH))
        return DIMINISHING_BANISH;
    if (mechanic & (1 << MECHANIC_ROOT))
        return triggered ? DIMINISHING_ROOT : DIMINISHING_CONTROLLED_ROOT;
    if (mechanic & (1 << MECHANIC_HORROR))
        return DIMINISHING_HORROR;

    return DIMINISHING_NONE;
}

DiminishingReturnsType GetDiminishingReturnsGroupType(DiminishingGroup group)
{
    switch (group)
    {
        case DIMINISHING_TAUNT:
        case DIMINISHING_CONTROLLED_STUN:
        case DIMINISHING_STUN:
        case DIMINISHING_OPENING_STUN:
        case DIMINISHING_CYCLONE:
        case DIMINISHING_CHARGE:
            return DRTYPE_ALL;
        case DIMINISHING_LIMITONLY:
        case DIMINISHING_NONE:
            return DRTYPE_NONE;
        default:
            return DRTYPE_PLAYER;
    }
}

DiminishingLevels GetDiminishingReturnsMaxLevel(DiminishingGroup group)
{
    switch (group)
    {
        case DIMINISHING_TAUNT:
            return DIMINISHING_LEVEL_TAUNT_IMMUNE;
        default:
            return DIMINISHING_LEVEL_IMMUNE;
    }
}

int32 GetDiminishingReturnsLimitDuration(DiminishingGroup group, SpellInfo const* spellproto)
{
    if (!IsDiminishingReturnsGroupDurationLimited(group))
        return 0;

    // Explicit diminishing duration
    switch (spellproto->SpellFamilyName)
    {
        case SPELLFAMILY_DRUID:
        {
            // Faerie Fire - limit to 40 seconds in PvP (3.1)
            if (spellproto->SpellFamilyFlags[0] & 0x400)
                return 40 * IN_MILLISECONDS;
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // Wyvern Sting
            if (spellproto->SpellFamilyFlags[1] & 0x1000)
                return 6 * IN_MILLISECONDS;
            // Hunter's Mark
            if (spellproto->SpellFamilyFlags[0] & 0x400)
                return 120 * IN_MILLISECONDS;
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            // Repentance - limit to 6 seconds in PvP
            if (spellproto->SpellFamilyFlags[0] & 0x4)
                return 6 * IN_MILLISECONDS;
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // Banish - limit to 6 seconds in PvP
            if (spellproto->SpellFamilyFlags[1] & 0x8000000)
                return 6 * IN_MILLISECONDS;
            // Curse of Tongues - limit to 12 seconds in PvP
            else if (spellproto->SpellFamilyFlags[2] & 0x800)
                return 12 * IN_MILLISECONDS;
            // Curse of Elements - limit to 120 seconds in PvP
            else if (spellproto->SpellFamilyFlags[1] & 0x200)
               return 120 * IN_MILLISECONDS;
            break;
        }
        default:
            break;
    }

    return 10 * IN_MILLISECONDS;
}

bool IsDiminishingReturnsGroupDurationLimited(DiminishingGroup group)
{
    switch (group)
    {
        case DIMINISHING_BANISH:
        case DIMINISHING_CONTROLLED_STUN:
        case DIMINISHING_CONTROLLED_ROOT:
        case DIMINISHING_CYCLONE:
        case DIMINISHING_DISORIENT:
        case DIMINISHING_ENTRAPMENT:
        case DIMINISHING_FEAR:
        case DIMINISHING_HORROR:
        case DIMINISHING_MIND_CONTROL:
        case DIMINISHING_OPENING_STUN:
        case DIMINISHING_ROOT:
        case DIMINISHING_STUN:
        case DIMINISHING_SLEEP:
        case DIMINISHING_LIMITONLY:
            return true;
        default:
            return false;
    }
}

SpellMgr::SpellMgr() { }

SpellMgr::~SpellMgr()
{
    UnloadSpellInfoStore();
}

SpellMgr* SpellMgr::instance()
{
    static SpellMgr instance;
    return &instance;
}

/// Some checks for spells, to prevent adding deprecated/broken spells for trainers, spell book, etc
bool SpellMgr::IsSpellValid(SpellInfo const* spellInfo, Player* player, bool msg)
{
    // not exist
    if (!spellInfo)
        return false;

    bool needCheckReagents = false;

    // check effects
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        switch (spellInfo->Effects[i].Effect)
        {
            case 0:
                continue;

            // craft spell for crafting non-existed item (break client recipes list show)
            case SPELL_EFFECT_CREATE_ITEM:
            case SPELL_EFFECT_CREATE_ITEM_2:
            {
                if (spellInfo->Effects[i].ItemType == 0)
                {
                    // skip auto-loot crafting spells, its not need explicit item info (but have special fake items sometime)
                    if (!spellInfo->IsLootCrafting())
                    {
                        if (msg)
                        {
                            if (player)
                                ChatHandler(player->GetSession()).PSendSysMessage("Craft spell %u not have create item entry.", spellInfo->Id);
                            else
                                TC_LOG_ERROR("sql.sql", "Craft spell %u not have create item entry.", spellInfo->Id);
                        }
                        return false;
                    }

                }
                // also possible IsLootCrafting case but fake item must exist anyway
                else if (!sObjectMgr->GetItemTemplate(spellInfo->Effects[i].ItemType))
                {
                    if (msg)
                    {
                        if (player)
                            ChatHandler(player->GetSession()).PSendSysMessage("Craft spell %u create not-exist in DB item (Entry: %u) and then...", spellInfo->Id, spellInfo->Effects[i].ItemType);
                        else
                            TC_LOG_ERROR("sql.sql", "Craft spell %u create not-exist in DB item (Entry: %u) and then...", spellInfo->Id, spellInfo->Effects[i].ItemType);
                    }
                    return false;
                }

                needCheckReagents = true;
                break;
            }
            case SPELL_EFFECT_LEARN_SPELL:
            {
                SpellInfo const* spellInfo2 = sSpellMgr->GetSpellInfo(spellInfo->Effects[i].TriggerSpell);
                if (!IsSpellValid(spellInfo2, player, msg))
                {
                    if (msg)
                    {
                        if (player)
                            ChatHandler(player->GetSession()).PSendSysMessage("Spell %u learn to broken spell %u, and then...", spellInfo->Id, spellInfo->Effects[i].TriggerSpell);
                        else
                            TC_LOG_ERROR("sql.sql", "Spell %u learn to invalid spell %u, and then...", spellInfo->Id, spellInfo->Effects[i].TriggerSpell);
                    }
                    return false;
                }
                break;
            }
        }
    }

    if (needCheckReagents)
    {
        for (uint8 j = 0; j < MAX_SPELL_REAGENTS; ++j)
        {
            if (spellInfo->Reagent[j] > 0 && !sObjectMgr->GetItemTemplate(spellInfo->Reagent[j]))
            {
                if (msg)
                {
                    if (player)
                        ChatHandler(player->GetSession()).PSendSysMessage("Craft spell %u have not-exist reagent in DB item (Entry: %u) and then...", spellInfo->Id, spellInfo->Reagent[j]);
                    else
                        TC_LOG_ERROR("sql.sql", "Craft spell %u have not-exist reagent in DB item (Entry: %u) and then...", spellInfo->Id, spellInfo->Reagent[j]);
                }
                return false;
            }
        }
    }

    return true;
}

uint32 SpellMgr::GetSpellDifficultyId(uint32 spellId) const
{
    SpellDifficultySearcherMap::const_iterator i = mSpellDifficultySearcherMap.find(spellId);
    return i == mSpellDifficultySearcherMap.end() ? 0 : i->second;
}

void SpellMgr::SetSpellDifficultyId(uint32 spellId, uint32 id)
{
    if (uint32 i = GetSpellDifficultyId(spellId))
        TC_LOG_ERROR("spells", "SpellMgr::SetSpellDifficultyId: Spell %u has already spellDifficultyId %u. Will override with spellDifficultyId %u.", spellId, i, id);
    mSpellDifficultySearcherMap[spellId] = id;
}

uint32 SpellMgr::GetSpellIdForDifficulty(uint32 spellId, Difficulty difficulty) const
{
    if (!GetSpellInfo(spellId))
        return spellId;

    uint32 difficultyInt = static_cast<uint32>(difficulty);

    if (difficultyInt >= MAX_DIFFICULTY)
    {
        TC_LOG_ERROR("spells", "SpellMgr::GetSpellIdForDifficulty: Incorrect Difficulty for spell %u.", spellId);
        return spellId; //return source spell
    }

    uint32 difficultyId = GetSpellDifficultyId(spellId);
    if (!difficultyId)
        return spellId; //return source spell, it has only REGULAR_DIFFICULTY

    SpellDifficultyEntry const* difficultyEntry = sSpellDifficultyStore.LookupEntry(difficultyId);
    if (!difficultyEntry)
    {
        TC_LOG_ERROR("spells", "SpellMgr::GetSpellIdForDifficulty: SpellDifficultyEntry not found for spell %u. This should never happen.", spellId);
        return spellId; //return source spell
    }

    if (difficultyEntry->SpellID[difficultyInt] <= 0 && difficultyInt > DUNGEON_DIFFICULTY_HEROIC)
    {
        TC_LOG_DEBUG("spells", "SpellMgr::GetSpellIdForDifficulty: spell %u mode %u spell is NULL, using mode %u", spellId, difficultyInt, difficultyInt - 2);
        difficultyInt -= 2;
    }

    if (difficultyEntry->SpellID[difficultyInt] <= 0)
    {
        TC_LOG_ERROR("sql.sql", "SpellMgr::GetSpellIdForDifficulty: spell %u mode %u spell is 0. Check spelldifficulty_dbc!", spellId, difficultyInt);
        return spellId;
    }

    TC_LOG_DEBUG("spells", "SpellMgr::GetSpellIdForDifficulty: spellid for spell %u in mode %u is %d", spellId, difficultyInt, difficultyEntry->SpellID[difficultyInt]);
    return uint32(difficultyEntry->SpellID[difficultyInt]);
}

uint32 SpellMgr::GetSpellIdForDifficulty(uint32 spellId, Unit const* caster) const
{
    if (!caster || !caster->GetMap() || !caster->GetMap()->IsDungeon())
        return spellId;

    return GetSpellIdForDifficulty(spellId, static_cast<Difficulty>(caster->GetMap()->GetSpawnMode()));
}

SpellInfo const* SpellMgr::GetSpellForDifficultyFromSpell(SpellInfo const* spell, Unit const* caster) const
{
    if (!spell)
        return NULL;

    uint32 newSpellId = GetSpellIdForDifficulty(spell->Id, caster);
    SpellInfo const* newSpell = GetSpellInfo(newSpellId);
    if (!newSpell)
    {
        TC_LOG_DEBUG("spells", "SpellMgr::GetSpellForDifficultyFromSpell: spell %u not found. Check spelldifficulty_dbc!", newSpellId);
        return spell;
    }

    TC_LOG_DEBUG("spells", "SpellMgr::GetSpellForDifficultyFromSpell: Spell id for instance mode is %u (original %u)", newSpell->Id, spell->Id);
    return newSpell;
}

SpellChainNode const* SpellMgr::GetSpellChainNode(uint32 spell_id) const
{
    SpellChainMap::const_iterator itr = mSpellChains.find(spell_id);
    if (itr == mSpellChains.end())
        return NULL;

    return &itr->second;
}

uint32 SpellMgr::GetFirstSpellInChain(uint32 spell_id) const
{
    if (SpellChainNode const* node = GetSpellChainNode(spell_id))
        return node->first->Id;

    return spell_id;
}

uint32 SpellMgr::GetLastSpellInChain(uint32 spell_id) const
{
    if (SpellChainNode const* node = GetSpellChainNode(spell_id))
        return node->last->Id;

    return spell_id;
}

uint32 SpellMgr::GetNextSpellInChain(uint32 spell_id) const
{
    if (SpellChainNode const* node = GetSpellChainNode(spell_id))
        if (node->next)
            return node->next->Id;

    return 0;
}

uint32 SpellMgr::GetPrevSpellInChain(uint32 spell_id) const
{
    if (SpellChainNode const* node = GetSpellChainNode(spell_id))
        if (node->prev)
            return node->prev->Id;

    return 0;
}

uint8 SpellMgr::GetSpellRank(uint32 spell_id) const
{
    if (SpellChainNode const* node = GetSpellChainNode(spell_id))
        return node->rank;

    return 0;
}

uint32 SpellMgr::GetSpellWithRank(uint32 spell_id, uint32 rank, bool strict) const
{
    if (SpellChainNode const* node = GetSpellChainNode(spell_id))
    {
        if (rank != node->rank)
            return GetSpellWithRank(node->rank < rank ? node->next->Id : node->prev->Id, rank, strict);
    }
    else if (strict && rank > 1)
        return 0;
    return spell_id;
}

SpellRequiredMapBounds SpellMgr::GetSpellsRequiredForSpellBounds(uint32 spell_id) const
{
    return mSpellReq.equal_range(spell_id);
}

SpellsRequiringSpellMapBounds SpellMgr::GetSpellsRequiringSpellBounds(uint32 spell_id) const
{
    return mSpellsReqSpell.equal_range(spell_id);
}

bool SpellMgr::IsSpellRequiringSpell(uint32 spellid, uint32 req_spellid) const
{
    SpellsRequiringSpellMapBounds spellsRequiringSpell = GetSpellsRequiringSpellBounds(req_spellid);
    for (SpellsRequiringSpellMap::const_iterator itr = spellsRequiringSpell.first; itr != spellsRequiringSpell.second; ++itr)
    {
        if (itr->second == spellid)
            return true;
    }
    return false;
}

SpellLearnSkillNode const* SpellMgr::GetSpellLearnSkill(uint32 spell_id) const
{
    SpellLearnSkillMap::const_iterator itr = mSpellLearnSkills.find(spell_id);
    if (itr != mSpellLearnSkills.end())
        return &itr->second;
    else
        return NULL;
}

SpellLearnSpellMapBounds SpellMgr::GetSpellLearnSpellMapBounds(uint32 spell_id) const
{
    return mSpellLearnSpells.equal_range(spell_id);
}

bool SpellMgr::IsSpellLearnSpell(uint32 spell_id) const
{
    return mSpellLearnSpells.find(spell_id) != mSpellLearnSpells.end();
}

bool SpellMgr::IsSpellLearnToSpell(uint32 spell_id1, uint32 spell_id2) const
{
    SpellLearnSpellMapBounds bounds = GetSpellLearnSpellMapBounds(spell_id1);
    for (SpellLearnSpellMap::const_iterator i = bounds.first; i != bounds.second; ++i)
        if (i->second.spell == spell_id2)
            return true;
    return false;
}

SpellTargetPosition const* SpellMgr::GetSpellTargetPosition(uint32 spell_id, SpellEffIndex effIndex) const
{
    SpellTargetPositionMap::const_iterator itr = mSpellTargetPositions.find(std::make_pair(spell_id, effIndex));
    if (itr != mSpellTargetPositions.end())
        return &itr->second;
    return NULL;
}

SpellSpellGroupMapBounds SpellMgr::GetSpellSpellGroupMapBounds(uint32 spell_id) const
{
    spell_id = GetFirstSpellInChain(spell_id);
    return mSpellSpellGroup.equal_range(spell_id);
}

bool SpellMgr::IsSpellMemberOfSpellGroup(uint32 spellid, SpellGroup groupid) const
{
    SpellSpellGroupMapBounds spellGroup = GetSpellSpellGroupMapBounds(spellid);
    for (SpellSpellGroupMap::const_iterator itr = spellGroup.first; itr != spellGroup.second; ++itr)
    {
        if (itr->second == groupid)
            return true;
    }
    return false;
}

SpellGroupSpellMapBounds SpellMgr::GetSpellGroupSpellMapBounds(SpellGroup group_id) const
{
    return mSpellGroupSpell.equal_range(group_id);
}

void SpellMgr::GetSetOfSpellsInSpellGroup(SpellGroup group_id, std::set<uint32>& foundSpells) const
{
    std::set<SpellGroup> usedGroups;
    GetSetOfSpellsInSpellGroup(group_id, foundSpells, usedGroups);
}

void SpellMgr::GetSetOfSpellsInSpellGroup(SpellGroup group_id, std::set<uint32>& foundSpells, std::set<SpellGroup>& usedGroups) const
{
    if (usedGroups.find(group_id) != usedGroups.end())
        return;
    usedGroups.insert(group_id);

    SpellGroupSpellMapBounds groupSpell = GetSpellGroupSpellMapBounds(group_id);
    for (SpellGroupSpellMap::const_iterator itr = groupSpell.first; itr != groupSpell.second; ++itr)
    {
        if (itr->second < 0)
        {
            SpellGroup currGroup = (SpellGroup)abs(itr->second);
            GetSetOfSpellsInSpellGroup(currGroup, foundSpells, usedGroups);
        }
        else
        {
            foundSpells.insert(itr->second);
        }
    }
}

bool SpellMgr::AddSameEffectStackRuleSpellGroups(SpellInfo const* spellInfo, int32 amount, std::map<SpellGroup, int32>& groups) const
{
    uint32 spellId = spellInfo->GetFirstRankSpell()->Id;
    SpellSpellGroupMapBounds spellGroup = GetSpellSpellGroupMapBounds(spellId);
    // Find group with SPELL_GROUP_STACK_RULE_EXCLUSIVE_SAME_EFFECT if it belongs to one
    for (SpellSpellGroupMap::const_iterator itr = spellGroup.first; itr != spellGroup.second; ++itr)
    {
        SpellGroup group = itr->second;
        SpellGroupStackMap::const_iterator found = mSpellGroupStack.find(group);
        if (found != mSpellGroupStack.end())
        {
            if (found->second == SPELL_GROUP_STACK_RULE_EXCLUSIVE_SAME_EFFECT)
            {
                //TC_LOG_DEBUG(LOG_FILTER_VEHICLES, "Spellgroup same effect is %u", group);
                // Put the highest amount in the map
                if (groups.find(group) == groups.end())
                    groups[group] = amount;
                else
                {
                    int32 curr_amount = groups[group];
                    // Take absolute value because this also counts for the highest negative aura
                    if (abs(curr_amount) < abs(amount))
                        groups[group] = amount;
                }
                // return because a spell should be in only one SPELL_GROUP_STACK_RULE_EXCLUSIVE_SAME_EFFECT group
                return true;
            }
        }
    }
    // Not in a SPELL_GROUP_STACK_RULE_EXCLUSIVE_SAME_EFFECT group, so return false
    return false;
}

SpellGroupStackRule SpellMgr::CheckSpellGroupStackRules(SpellInfo const* spellInfo1, SpellInfo const* spellInfo2) const
{
    uint32 spellid_1 = spellInfo1->GetFirstRankSpell()->Id;
    uint32 spellid_2 = spellInfo2->GetFirstRankSpell()->Id;
    if (spellid_1 == spellid_2)
        return SPELL_GROUP_STACK_RULE_DEFAULT;
    // find SpellGroups which are common for both spells
    SpellSpellGroupMapBounds spellGroup1 = GetSpellSpellGroupMapBounds(spellid_1);
    std::set<SpellGroup> groups;
    for (SpellSpellGroupMap::const_iterator itr = spellGroup1.first; itr != spellGroup1.second; ++itr)
    {
        if (IsSpellMemberOfSpellGroup(spellid_2, itr->second))
        {
            bool add = true;
            SpellGroupSpellMapBounds groupSpell = GetSpellGroupSpellMapBounds(itr->second);
            for (SpellGroupSpellMap::const_iterator itr2 = groupSpell.first; itr2 != groupSpell.second; ++itr2)
            {
                if (itr2->second < 0)
                {
                    SpellGroup currGroup = (SpellGroup)abs(itr2->second);
                    if (IsSpellMemberOfSpellGroup(spellid_1, currGroup) && IsSpellMemberOfSpellGroup(spellid_2, currGroup))
                    {
                        add = false;
                        break;
                    }
                }
            }
            if (add)
                groups.insert(itr->second);
        }
    }

    SpellGroupStackRule rule = SPELL_GROUP_STACK_RULE_DEFAULT;

    for (std::set<SpellGroup>::iterator itr = groups.begin(); itr!= groups.end(); ++itr)
    {
        SpellGroupStackMap::const_iterator found = mSpellGroupStack.find(*itr);
        if (found != mSpellGroupStack.end())
            rule = found->second;
        if (rule)
            break;
    }
    return rule;
}

SpellGroupStackRule SpellMgr::GetSpellGroupStackRule(SpellGroup group) const
{
    SpellGroupStackMap::const_iterator itr = mSpellGroupStack.find(group);
    if (itr != mSpellGroupStack.end())
        return itr->second;

    return SPELL_GROUP_STACK_RULE_DEFAULT;
}

SpellProcEventEntry const* SpellMgr::GetSpellProcEvent(uint32 spellId) const
{
    SpellProcEventMap::const_iterator itr = mSpellProcEventMap.find(spellId);
    if (itr != mSpellProcEventMap.end())
        return &itr->second;
    return NULL;
}

bool SpellMgr::IsSpellProcEventCanTriggeredBy(SpellProcEventEntry const* spellProcEvent, uint32 EventProcFlag, SpellInfo const* procSpell, uint32 procFlags, uint32 procExtra, bool active) const
{
    // No extra req need
    uint32 procEvent_procEx = PROC_EX_NONE;

    // check prockFlags for condition
    if ((procFlags & EventProcFlag) == 0)
        return false;

    bool hasFamilyMask = false;

    /* Check Periodic Auras

    *Dots can trigger if spell has no PROC_FLAG_SUCCESSFUL_NEGATIVE_MAGIC_SPELL
        nor PROC_FLAG_TAKEN_POSITIVE_MAGIC_SPELL

    *Only Hots can trigger if spell has PROC_FLAG_TAKEN_POSITIVE_MAGIC_SPELL

    *Only dots can trigger if spell has both positivity flags or PROC_FLAG_SUCCESSFUL_NEGATIVE_MAGIC_SPELL

    *Aura has to have PROC_FLAG_TAKEN_POSITIVE_MAGIC_SPELL or spellfamily specified to trigger from Hot

    */

    if (procFlags & PROC_FLAG_DONE_PERIODIC)
    {
        if (EventProcFlag & PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG)
        {
            if (!(procExtra & PROC_EX_INTERNAL_DOT))
                return false;
        }
        else if (procExtra & PROC_EX_INTERNAL_HOT)
        {
            procExtra |= PROC_EX_INTERNAL_REQ_FAMILY;
            //! HACK: make an exeption for trigger spells with hots.
            //        hot can trigger a spell when PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS is set
            if (EventProcFlag & PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS)
                return true;
        }
        else if (EventProcFlag & PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS)
            return false;
    }

    if (procFlags & PROC_FLAG_TAKEN_PERIODIC)
    {
        if (EventProcFlag & PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_POS)
        {
            if (!(procExtra & PROC_EX_INTERNAL_HOT))
                return false;
        }
        else if (procExtra & PROC_EX_INTERNAL_HOT)
            procExtra |= PROC_EX_INTERNAL_REQ_FAMILY;
        else if (EventProcFlag & PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_POS)
            return false;
    }
    // Trap casts are active by default
    if (procFlags & PROC_FLAG_DONE_TRAP_ACTIVATION)
        active = true;

    // Always trigger for this
    if (procFlags & (PROC_FLAG_KILLED | PROC_FLAG_KILL | PROC_FLAG_DEATH))
        return true;

    if (spellProcEvent)     // Exist event data
    {
        // Store extra req
        procEvent_procEx = spellProcEvent->procEx;

        // For melee triggers
        if (procSpell == NULL)
        {
            // Check (if set) for school (melee attack have Normal school)
            if (spellProcEvent->schoolMask && (spellProcEvent->schoolMask & SPELL_SCHOOL_MASK_NORMAL) == 0)
                return false;
        }
        else // For spells need check school/spell family/family mask
        {
            // Check (if set) for school
            if (spellProcEvent->schoolMask && (spellProcEvent->schoolMask & procSpell->SchoolMask) == 0)
                return false;

            // Check (if set) for spellFamilyName
            if (spellProcEvent->spellFamilyName && (spellProcEvent->spellFamilyName != procSpell->SpellFamilyName))
                return false;

            // spellFamilyName is Ok need check for spellFamilyMask if present
            if (spellProcEvent->spellFamilyMask)
            {
                if (!(spellProcEvent->spellFamilyMask & procSpell->SpellFamilyFlags))
                    return false;
                hasFamilyMask = true;
                // Some spells are not considered as active even with have spellfamilyflags
                if (!(procEvent_procEx & PROC_EX_ONLY_ACTIVE_SPELL))
                    active = true;
            }
        }
    }

    if (procExtra & (PROC_EX_INTERNAL_REQ_FAMILY))
    {
        if (!hasFamilyMask)
            return false;
    }

    // Check for extra req (if none) and hit/crit
    if (procEvent_procEx == PROC_EX_NONE)
    {
        // No extra req, so can trigger only for hit/crit - spell has to be active
        if ((procExtra & (PROC_EX_NORMAL_HIT|PROC_EX_CRITICAL_HIT)) && active)
            return true;
    }
    else // Passive spells hits here only if resist/reflect/immune/evade
    {
        if (procExtra & AURA_SPELL_PROC_EX_MASK)
        {
            // if spell marked as procing only from not active spells
            if (active && procEvent_procEx & PROC_EX_NOT_ACTIVE_SPELL)
                return false;
            // if spell marked as procing only from active spells
            if (!active && procEvent_procEx & PROC_EX_ONLY_ACTIVE_SPELL)
                return false;
            // Exist req for PROC_EX_EX_TRIGGER_ALWAYS
            if (procEvent_procEx & PROC_EX_EX_TRIGGER_ALWAYS)
                return true;
            // PROC_EX_NOT_ACTIVE_SPELL and PROC_EX_ONLY_ACTIVE_SPELL flags handle: if passed checks before
            if ((procExtra & (PROC_EX_NORMAL_HIT|PROC_EX_CRITICAL_HIT)) && ((procEvent_procEx & (AURA_SPELL_PROC_EX_MASK)) == 0))
                return true;
        }
        // Check Extra Requirement like (hit/crit/miss/resist/parry/dodge/block/immune/reflect/absorb and other)
        if (procEvent_procEx & procExtra)
            return true;
    }
    return false;
}

SpellProcEntry const* SpellMgr::GetSpellProcEntry(uint32 spellId) const
{
    SpellProcMap::const_iterator itr = mSpellProcMap.find(spellId);
    if (itr != mSpellProcMap.end())
        return &itr->second;
    return NULL;
}

bool SpellMgr::CanSpellTriggerProcOnEvent(SpellProcEntry const& procEntry, ProcEventInfo& eventInfo) const
{
    // proc type doesn't match
    if (!(eventInfo.GetTypeMask() & procEntry.typeMask))
        return false;

    // check XP or honor target requirement
    if (procEntry.attributesMask & PROC_ATTR_REQ_EXP_OR_HONOR)
        if (Player* actor = eventInfo.GetActor()->ToPlayer())
            if (eventInfo.GetActionTarget() && !actor->isHonorOrXPTarget(eventInfo.GetActionTarget()))
                return false;

    // always trigger for these types
    if (eventInfo.GetTypeMask() & (PROC_FLAG_KILLED | PROC_FLAG_KILL | PROC_FLAG_DEATH))
        return true;

    // check school mask (if set) for other trigger types
    if (procEntry.schoolMask && !(eventInfo.GetSchoolMask() & procEntry.schoolMask))
        return false;

    // check spell family name/flags (if set) for spells
    if (eventInfo.GetTypeMask() & (PERIODIC_PROC_FLAG_MASK | SPELL_PROC_FLAG_MASK | PROC_FLAG_DONE_TRAP_ACTIVATION))
    {
        SpellInfo const* eventSpellInfo = eventInfo.GetSpellInfo();

        if (procEntry.spellFamilyName && eventSpellInfo && (procEntry.spellFamilyName != eventSpellInfo->SpellFamilyName))
            return false;

        if (procEntry.spellFamilyMask && eventSpellInfo && !(procEntry.spellFamilyMask & eventSpellInfo->SpellFamilyFlags))
            return false;
    }

    // check spell type mask (if set)
    if (eventInfo.GetTypeMask() & (SPELL_PROC_FLAG_MASK | PERIODIC_PROC_FLAG_MASK))
    {
        if (procEntry.spellTypeMask && !(eventInfo.GetSpellTypeMask() & procEntry.spellTypeMask))
            return false;
    }

    // check spell phase mask
    if (eventInfo.GetTypeMask() & REQ_SPELL_PHASE_PROC_FLAG_MASK)
    {
        if (!(eventInfo.GetSpellPhaseMask() & procEntry.spellPhaseMask))
            return false;
    }

    // check hit mask (on taken hit or on done hit, but not on spell cast phase)
    if ((eventInfo.GetTypeMask() & TAKEN_HIT_PROC_FLAG_MASK) || ((eventInfo.GetTypeMask() & DONE_HIT_PROC_FLAG_MASK) && !(eventInfo.GetSpellPhaseMask() & PROC_SPELL_PHASE_CAST)))
    {
        uint32 hitMask = procEntry.hitMask;
        // get default values if hit mask not set
        if (!hitMask)
        {
            // for taken procs allow normal + critical hits by default
            if (eventInfo.GetTypeMask() & TAKEN_HIT_PROC_FLAG_MASK)
                hitMask |= PROC_HIT_NORMAL | PROC_HIT_CRITICAL;
            // for done procs allow normal + critical + absorbs by default
            else
                hitMask |= PROC_HIT_NORMAL | PROC_HIT_CRITICAL | PROC_HIT_ABSORB;
        }
        if (!(eventInfo.GetHitMask() & hitMask))
            return false;
    }

    return true;
}

SpellBonusEntry const* SpellMgr::GetSpellBonusData(uint32 spellId) const
{
    // Lookup data
    SpellBonusMap::const_iterator itr = mSpellBonusMap.find(spellId);
    if (itr != mSpellBonusMap.end())
        return &itr->second;
    // Not found, try lookup for 1 spell rank if exist
    if (uint32 rank_1 = GetFirstSpellInChain(spellId))
    {
        SpellBonusMap::const_iterator itr2 = mSpellBonusMap.find(rank_1);
        if (itr2 != mSpellBonusMap.end())
            return &itr2->second;
    }
    return NULL;
}

SpellThreatEntry const* SpellMgr::GetSpellThreatEntry(uint32 spellID) const
{
    SpellThreatMap::const_iterator itr = mSpellThreatMap.find(spellID);
    if (itr != mSpellThreatMap.end())
        return &itr->second;
    else
    {
        uint32 firstSpell = GetFirstSpellInChain(spellID);
        itr = mSpellThreatMap.find(firstSpell);
        if (itr != mSpellThreatMap.end())
            return &itr->second;
    }
    return NULL;
}

SkillLineAbilityMapBounds SpellMgr::GetSkillLineAbilityMapBounds(uint32 spell_id) const
{
    return mSkillLineAbilityMap.equal_range(spell_id);
}

PetAura const* SpellMgr::GetPetAura(uint32 spell_id, uint8 eff) const
{
    SpellPetAuraMap::const_iterator itr = mSpellPetAuraMap.find((spell_id<<8) + eff);
    if (itr != mSpellPetAuraMap.end())
        return &itr->second;
    else
        return NULL;
}

std::optional<uint32> SpellMgr::IsPetAura(uint32 auraId) const
{
    auto itr = this->mSpellPetAuraStore.find(auraId);
    if (itr != mSpellPetAuraStore.end())
        return itr->second;
    else
        return std::nullopt;
}

SpellEnchantProcEntry const* SpellMgr::GetSpellEnchantProcEvent(uint32 enchId) const
{
    SpellEnchantProcEventMap::const_iterator itr = mSpellEnchantProcEventMap.find(enchId);
    if (itr != mSpellEnchantProcEventMap.end())
        return &itr->second;
    return NULL;
}

bool SpellMgr::IsArenaAllowedEnchancment(uint32 ench_id) const
{
    return mEnchantCustomAttr[ench_id];
}

const std::vector<int32>* SpellMgr::GetSpellLinked(int32 spell_id) const
{
    SpellLinkedMap::const_iterator itr = mSpellLinkedMap.find(spell_id);
    return itr != mSpellLinkedMap.end() ? &(itr->second) : NULL;
}

PetLevelupSpellSet const* SpellMgr::GetPetLevelupSpellList(uint32 petFamily) const
{
    PetLevelupSpellMap::const_iterator itr = mPetLevelupSpellMap.find(petFamily);
    if (itr != mPetLevelupSpellMap.end())
        return &itr->second;
    else
        return NULL;
}

PetDefaultSpellsEntry const* SpellMgr::GetPetDefaultSpellsEntry(int32 id) const
{
    PetDefaultSpellsMap::const_iterator itr = mPetDefaultSpellsMap.find(id);
    if (itr != mPetDefaultSpellsMap.end())
        return &itr->second;
    return NULL;
}

SpellAreaMapBounds SpellMgr::GetSpellAreaMapBounds(uint32 spell_id) const
{
    return mSpellAreaMap.equal_range(spell_id);
}

SpellAreaForQuestMapBounds SpellMgr::GetSpellAreaForQuestMapBounds(uint32 quest_id) const
{
    return mSpellAreaForQuestMap.equal_range(quest_id);
}

SpellAreaForQuestMapBounds SpellMgr::GetSpellAreaForQuestEndMapBounds(uint32 quest_id) const
{
    return mSpellAreaForQuestEndMap.equal_range(quest_id);
}

SpellAreaForAuraMapBounds SpellMgr::GetSpellAreaForAuraMapBounds(uint32 spell_id) const
{
    return mSpellAreaForAuraMap.equal_range(spell_id);
}

SpellAreaForAreaMapBounds SpellMgr::GetSpellAreaForAreaMapBounds(uint32 area_id) const
{
    return mSpellAreaForAreaMap.equal_range(area_id);
}

bool SpellArea::IsFitToRequirements(Player const* player, uint32 newZone, uint32 newArea) const
{
    if (gender != GENDER_NONE)                   // not in expected gender
        if (!player || gender != player->getGender())
            return false;

    if (raceMask)                                // not in expected race
        if (!player || !(raceMask & player->getRaceMask()))
            return false;

    if (areaId)                                  // not in expected zone
        if (newZone != areaId && newArea != areaId)
            return false;

    if (questStart)                              // not in expected required quest state
        if (!player || (((1 << player->GetQuestStatus(questStart)) & questStartStatus) == 0))
            return false;

    if (questEnd)                                // not in expected forbidden quest state
        if (!player || (((1 << player->GetQuestStatus(questEnd)) & questEndStatus) == 0))
            return false;

    if (auraSpell)                               // not have expected aura
        if (!player || (auraSpell > 0 && !player->HasAura(auraSpell)) || (auraSpell < 0 && player->HasAura(-auraSpell)))
            return false;

    if (player)
    {
        if (Battleground* bg = player->GetBattleground())
            return bg->IsSpellAllowed(spellId, player);
    }

    // Extra conditions
    switch (spellId)
    {
        case 58600: // No fly Zone - Dalaran
        {
            if (!player)
                return false;

            AreaTableEntry const* pArea = sAreaTableStore.LookupEntry(player->GetAreaId());
            if (!(pArea && pArea->flags & AREA_FLAG_NO_FLY_ZONE))
                return false;
            if (!player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) && !player->HasAuraType(SPELL_AURA_FLY))
                return false;
            break;
        }
        case 58730: // No fly Zone - Wintergrasp
        {
            if (!player)
                return false;

            Battlefield* Bf = sBattlefieldMgr->GetBattlefieldToZoneId(player->GetZoneId());
            if (!Bf || Bf->CanFlyIn() || player->isDead() || (!player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) && !player->HasAuraType(SPELL_AURA_FLY)))
                return false;
            break;
        }
        case 56618: // Horde Controls Factory Phase Shift
        case 56617: // Alliance Controls Factory Phase Shift
        {
            if (!player)
                return false;

            Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(player->GetZoneId());

            if (!bf || bf->GetTypeId() != BATTLEFIELD_WG)
                return false;

            // team that controls the workshop in the specified area
            uint32 team = bf->GetData(newArea);

            if (team == TEAM_HORDE)
                return spellId == 56618;
            else if (team == TEAM_ALLIANCE)
                return spellId == 56617;
            break;
        }
        case 57940: // Essence of Wintergrasp - Northrend
        case 58045: // Essence of Wintergrasp - Wintergrasp
        {
            if (!player)
                return false;

            if (Battlefield* battlefieldWG = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG))
                return battlefieldWG->IsEnabled() && (player->GetTeamId() == battlefieldWG->GetDefenderTeam()) && !battlefieldWG->IsWarTime();
            break;
        }
        case 74411: // Battleground - Dampening
        {
            if (!player)
                return false;

            if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(player->GetZoneId()))
                return bf->IsWarTime();
            break;
        }
    }

    return true;
}

void SpellMgr::UnloadSpellInfoChains()
{
    for (SpellChainMap::iterator itr = mSpellChains.begin(); itr != mSpellChains.end(); ++itr)
        mSpellInfoMap[itr->first]->ChainEntry = NULL;

    mSpellChains.clear();
}

void SpellMgr::LoadSpellTalentRanks()
{
    // cleanup core data before reload - remove reference to ChainNode from SpellInfo
    UnloadSpellInfoChains();

    for (uint32 i = 0; i < sTalentStore.GetNumRows(); ++i)
    {
        TalentEntry const* talentInfo = sTalentStore.LookupEntry(i);
        if (!talentInfo)
            continue;

        SpellInfo const* lastSpell = NULL;
        for (uint8 rank = MAX_TALENT_RANK - 1; rank > 0; --rank)
        {
            if (talentInfo->RankID[rank])
            {
                lastSpell = GetSpellInfo(talentInfo->RankID[rank]);
                break;
            }
        }

        if (!lastSpell)
            continue;

        SpellInfo const* firstSpell = GetSpellInfo(talentInfo->RankID[0]);
        if (!firstSpell)
        {
            TC_LOG_ERROR("spells", "SpellMgr::LoadSpellTalentRanks: First Rank Spell %u for TalentEntry %u does not exist.", talentInfo->RankID[0], i);
            continue;
        }

        SpellInfo const* prevSpell = NULL;
        for (uint8 rank = 0; rank < MAX_TALENT_RANK; ++rank)
        {
            uint32 spellId = talentInfo->RankID[rank];
            if (!spellId)
                break;

            SpellInfo const* currentSpell = GetSpellInfo(spellId);
            if (!currentSpell)
            {
                TC_LOG_ERROR("spells", "SpellMgr::LoadSpellTalentRanks: Spell %u (Rank: %u) for TalentEntry %u does not exist.", spellId, rank + 1, i);
                break;
            }

            SpellChainNode node;
            node.first = firstSpell;
            node.last  = lastSpell;
            node.rank  = rank + 1;

            node.prev = prevSpell;
            node.next = node.rank < MAX_TALENT_RANK ? GetSpellInfo(talentInfo->RankID[node.rank]) : NULL;

            mSpellChains[spellId] = node;
            mSpellInfoMap[spellId]->ChainEntry = &mSpellChains[spellId];

            prevSpell = currentSpell;
        }
    }
}

void SpellMgr::LoadSpellRanks()
{
    // cleanup data and load spell ranks for talents from dbc
    LoadSpellTalentRanks();

    uint32 oldMSTime = getMSTime();

    //                                                     0             1      2
    QueryResult result = WorldDatabase.Query("SELECT first_spell_id, spell_id, rank from spell_ranks ORDER BY first_spell_id, rank");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 spell rank records. DB table `spell_ranks` is empty.");
        return;
    }

    uint32 count = 0;
    bool finished = false;

    do
    {
                        // spellid, rank
        std::list < std::pair < int32, int32 > > rankChain;
        int32 currentSpell = -1;
        int32 lastSpell = -1;

        // fill one chain
        while (currentSpell == lastSpell && !finished)
        {
            Field* fields = result->Fetch();

            currentSpell = fields[0].GetUInt32();
            if (lastSpell == -1)
                lastSpell = currentSpell;
            uint32 spell_id = fields[1].GetUInt32();
            uint32 rank = fields[2].GetUInt8();

            // don't drop the row if we're moving to the next rank
            if (currentSpell == lastSpell)
            {
                rankChain.push_back(std::make_pair(spell_id, rank));
                if (!result->NextRow())
                    finished = true;
            }
            else
                break;
        }
        // check if chain is made with valid first spell
        SpellInfo const* first = GetSpellInfo(lastSpell);
        if (!first)
        {
            TC_LOG_ERROR("sql.sql", "Spell rank identifier(first_spell_id) %u listed in `spell_ranks` does not exist!", lastSpell);
            continue;
        }
        // check if chain is long enough
        if (rankChain.size() < 2)
        {
            TC_LOG_ERROR("sql.sql", "There is only 1 spell rank for identifier(first_spell_id) %u in `spell_ranks`, entry is not needed!", lastSpell);
            continue;
        }
        int32 curRank = 0;
        bool valid = true;
        // check spells in chain
        for (std::list<std::pair<int32, int32> >::iterator itr = rankChain.begin(); itr!= rankChain.end(); ++itr)
        {
            SpellInfo const* spell = GetSpellInfo(itr->first);
            if (!spell)
            {
                TC_LOG_ERROR("sql.sql", "Spell %u (rank %u) listed in `spell_ranks` for chain %u does not exist!", itr->first, itr->second, lastSpell);
                valid = false;
                break;
            }
            ++curRank;
            if (itr->second != curRank)
            {
                TC_LOG_ERROR("sql.sql", "Spell %u (rank %u) listed in `spell_ranks` for chain %u does not have proper rank value(should be %u)!", itr->first, itr->second, lastSpell, curRank);
                valid = false;
                break;
            }
        }
        if (!valid)
            continue;
        int32 prevRank = 0;
        // insert the chain
        std::list<std::pair<int32, int32> >::iterator itr = rankChain.begin();
        do
        {
            ++count;
            int32 addedSpell = itr->first;

            if (mSpellInfoMap[addedSpell]->ChainEntry)
                TC_LOG_ERROR("sql.sql", "Spell %u (rank: %u, first: %u) listed in `spell_ranks` has already ChainEntry from dbc.", addedSpell, itr->second, lastSpell);

            mSpellChains[addedSpell].first = GetSpellInfo(lastSpell);
            mSpellChains[addedSpell].last = GetSpellInfo(rankChain.back().first);
            mSpellChains[addedSpell].rank = itr->second;
            mSpellChains[addedSpell].prev = GetSpellInfo(prevRank);
            mSpellInfoMap[addedSpell]->ChainEntry = &mSpellChains[addedSpell];
            prevRank = addedSpell;
            ++itr;

            if (itr == rankChain.end())
            {
                mSpellChains[addedSpell].next = NULL;
                break;
            }
            else
                mSpellChains[addedSpell].next = GetSpellInfo(itr->first);
        }
        while (true);
    }
    while (!finished);

    TC_LOG_INFO("server.loading", ">> Loaded %u spell rank records in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellRequired()
{
    uint32 oldMSTime = getMSTime();

    mSpellsReqSpell.clear();                                   // need for reload case
    mSpellReq.clear();                                         // need for reload case

    //                                                   0        1
    QueryResult result = WorldDatabase.Query("SELECT spell_id, req_spell from spell_required");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 spell required records. DB table `spell_required` is empty.");

        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 spell_id = fields[0].GetUInt32();
        uint32 spell_req = fields[1].GetUInt32();

        // check if chain is made with valid first spell
        SpellInfo const* spell = GetSpellInfo(spell_id);
        if (!spell)
        {
            TC_LOG_ERROR("sql.sql", "spell_id %u in `spell_required` table is not found in dbcs, skipped", spell_id);
            continue;
        }

        SpellInfo const* reqSpell = GetSpellInfo(spell_req);
        if (!reqSpell)
        {
            TC_LOG_ERROR("sql.sql", "req_spell %u in `spell_required` table is not found in dbcs, skipped", spell_req);
            continue;
        }

        if (spell->IsRankOf(reqSpell))
        {
            TC_LOG_ERROR("sql.sql", "req_spell %u and spell_id %u in `spell_required` table are ranks of the same spell, entry not needed, skipped", spell_req, spell_id);
            continue;
        }

        if (IsSpellRequiringSpell(spell_id, spell_req))
        {
            TC_LOG_ERROR("sql.sql", "duplicated entry of req_spell %u and spell_id %u in `spell_required`, skipped", spell_req, spell_id);
            continue;
        }

        mSpellReq.insert (std::pair<uint32, uint32>(spell_id, spell_req));
        mSpellsReqSpell.insert (std::pair<uint32, uint32>(spell_req, spell_id));
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u spell required records in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

}

void SpellMgr::LoadSpellLearnSkills()
{
    uint32 oldMSTime = getMSTime();

    mSpellLearnSkills.clear();                              // need for reload case

    // search auto-learned skills and add its to map also for use in unlearn spells/talents
    uint32 dbc_count = 0;
    for (uint32 spell = 0; spell < GetSpellInfoStoreSize(); ++spell)
    {
        SpellInfo const* entry = GetSpellInfo(spell);

        if (!entry)
            continue;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (entry->Effects[i].Effect == SPELL_EFFECT_SKILL)
            {
                SpellLearnSkillNode dbc_node;
                dbc_node.skill = entry->Effects[i].MiscValue;
                dbc_node.step  = entry->Effects[i].CalcValue();
                if (dbc_node.skill != SKILL_RIDING)
                    dbc_node.value = 1;
                else
                    dbc_node.value = dbc_node.step * 75;
                dbc_node.maxvalue = dbc_node.step * 75;
                mSpellLearnSkills[spell] = dbc_node;
                ++dbc_count;
                break;
            }
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u Spell Learn Skills from DBC in %u ms", dbc_count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellLearnSpells()
{
    uint32 oldMSTime = getMSTime();

    mSpellLearnSpells.clear();                              // need for reload case

    //                                                  0      1        2
    QueryResult result = WorldDatabase.Query("SELECT entry, SpellID, Active FROM spell_learn_spell");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 spell learn spells. DB table `spell_learn_spell` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 spell_id = fields[0].GetUInt16();

        SpellLearnSpellNode node;
        node.spell       = fields[1].GetUInt16();
        node.active      = fields[2].GetBool();
        node.autoLearned = false;

        if (!GetSpellInfo(spell_id))
        {
            TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_learn_spell` does not exist", spell_id);
            continue;
        }

        if (!GetSpellInfo(node.spell))
        {
            TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_learn_spell` learning not existed spell %u", spell_id, node.spell);
            continue;
        }

        if (GetTalentSpellCost(node.spell))
        {
            TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_learn_spell` attempt learning talent spell %u, skipped", spell_id, node.spell);
            continue;
        }

        mSpellLearnSpells.insert(SpellLearnSpellMap::value_type(spell_id, node));

        ++count;
    } while (result->NextRow());

    // search auto-learned spells and add its to map also for use in unlearn spells/talents
    uint32 dbc_count = 0;
    for (uint32 spell = 0; spell < GetSpellInfoStoreSize(); ++spell)
    {
        SpellInfo const* entry = GetSpellInfo(spell);

        if (!entry)
            continue;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (entry->Effects[i].Effect == SPELL_EFFECT_LEARN_SPELL)
            {
                SpellLearnSpellNode dbc_node;
                dbc_node.spell = entry->Effects[i].TriggerSpell;
                dbc_node.active = true;                     // all dbc based learned spells is active (show in spell book or hide by client itself)

                // ignore learning not existed spells (broken/outdated/or generic learnig spell 483
                if (!GetSpellInfo(dbc_node.spell))
                    continue;

                // talent or passive spells or skill-step spells auto-cast and not need dependent learning,
                // pet teaching spells must not be dependent learning (cast)
                // other required explicit dependent learning
                dbc_node.autoLearned = entry->Effects[i].TargetA.GetTarget() == TARGET_UNIT_PET || GetTalentSpellCost(spell) > 0 || entry->IsPassive() || entry->HasEffect(SPELL_EFFECT_SKILL_STEP);

                SpellLearnSpellMapBounds db_node_bounds = GetSpellLearnSpellMapBounds(spell);

                bool found = false;
                for (SpellLearnSpellMap::const_iterator itr = db_node_bounds.first; itr != db_node_bounds.second; ++itr)
                {
                    if (itr->second.spell == dbc_node.spell)
                    {
                        TC_LOG_ERROR("sql.sql", "Spell %u auto-learn spell %u in spell.dbc then the record in `spell_learn_spell` is redundant, please fix DB.",
                            spell, dbc_node.spell);
                        found = true;
                        break;
                    }
                }

                if (!found)                                  // add new spell-spell pair if not found
                {
                    mSpellLearnSpells.insert(SpellLearnSpellMap::value_type(spell, dbc_node));
                    ++dbc_count;
                }
            }
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u spell learn spells + %u found in DBC in %u ms", count, dbc_count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellTargetPositions()
{
    uint32 oldMSTime = getMSTime();

    mSpellTargetPositions.clear();                                // need for reload case

    //                                                0      1          2             3                  4                  5                   6
    QueryResult result = WorldDatabase.Query("SELECT id, effIndex, target_map, target_position_x, target_position_y, target_position_z, target_orientation FROM spell_target_position");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 spell target coordinates. DB table `spell_target_position` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 Spell_ID = fields[0].GetUInt32();
        SpellEffIndex effIndex = SpellEffIndex(fields[1].GetUInt8());

        SpellTargetPosition st;

        st.target_mapId       = fields[2].GetUInt16();
        st.target_X           = fields[3].GetFloat();
        st.target_Y           = fields[4].GetFloat();
        st.target_Z           = fields[5].GetFloat();
        st.target_Orientation = fields[6].GetFloat();

        MapEntry const* mapEntry = sMapStore.LookupEntry(st.target_mapId);
        if (!mapEntry)
        {
            TC_LOG_ERROR("sql.sql", "Spell (Id: %u, effIndex: %u) target map (ID: %u) does not exist in `Map.dbc`.", Spell_ID, effIndex, st.target_mapId);
            continue;
        }

        if (st.target_X==0 && st.target_Y==0 && st.target_Z==0)
        {
            TC_LOG_ERROR("sql.sql", "Spell (Id: %u, effIndex: %u) target coordinates not provided.", Spell_ID, effIndex);
            continue;
        }

        SpellInfo const* spellInfo = GetSpellInfo(Spell_ID);
        if (!spellInfo)
        {
            TC_LOG_ERROR("sql.sql", "Spell (Id: %u) listed in `spell_target_position` does not exist.", Spell_ID);
            continue;
        }

        if (spellInfo->Effects[effIndex].TargetA.GetTarget() == TARGET_DEST_DB || spellInfo->Effects[effIndex].TargetB.GetTarget() == TARGET_DEST_DB)
        {
            std::pair<uint32, SpellEffIndex> key = std::make_pair(Spell_ID, effIndex);
            mSpellTargetPositions[key] = st;
            ++count;
        }
        else
        {
            TC_LOG_ERROR("sql.sql", "Spell (Id: %u, effIndex: %u) listed in `spell_target_position` does not have target TARGET_DEST_DB (17).", Spell_ID, effIndex);
            continue;
        }

    } while (result->NextRow());

    /*
    // Check all spells
    for (uint32 i = 1; i < GetSpellInfoStoreSize; ++i)
    {
        SpellInfo const* spellInfo = GetSpellInfo(i);
        if (!spellInfo)
            continue;

        bool found = false;
        for (int j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            switch (spellInfo->Effects[j].TargetA)
            {
                case TARGET_DEST_DB:
                    found = true;
                    break;
            }
            if (found)
                break;
            switch (spellInfo->Effects[j].TargetB)
            {
                case TARGET_DEST_DB:
                    found = true;
                    break;
            }
            if (found)
                break;
        }
        if (found)
        {
            if (!sSpellMgr->GetSpellTargetPosition(i))
                TC_LOG_DEBUG("spells", "Spell (ID: %u) does not have record in `spell_target_position`", i);
        }
    }*/

    TC_LOG_INFO("server.loading", ">> Loaded %u spell teleport coordinates in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellGroups()
{
    uint32 oldMSTime = getMSTime();

    mSpellSpellGroup.clear();                                  // need for reload case
    mSpellGroupSpell.clear();

    //                                                0     1
    QueryResult result = WorldDatabase.Query("SELECT id, spell_id FROM spell_group");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 spell group definitions. DB table `spell_group` is empty.");
        return;
    }

    std::set<uint32> groups;
    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 group_id = fields[0].GetUInt32();
        if (group_id <= SPELL_GROUP_DB_RANGE_MIN && group_id >= SPELL_GROUP_CORE_RANGE_MAX)
        {
            TC_LOG_ERROR("sql.sql", "SpellGroup id %u listed in `spell_group` is in core range, but is not defined in core!", group_id);
            continue;
        }
        int32 spell_id = fields[1].GetInt32();

        groups.insert(std::set<uint32>::value_type(group_id));
        mSpellGroupSpell.insert(SpellGroupSpellMap::value_type((SpellGroup)group_id, spell_id));

    } while (result->NextRow());

    for (SpellGroupSpellMap::iterator itr = mSpellGroupSpell.begin(); itr!= mSpellGroupSpell.end();)
    {
        if (itr->second < 0)
        {
            if (groups.find(abs(itr->second)) == groups.end())
            {
                TC_LOG_ERROR("sql.sql", "SpellGroup id %u listed in `spell_group` does not exist", abs(itr->second));
                mSpellGroupSpell.erase(itr++);
            }
            else
                ++itr;
        }
        else
        {
            SpellInfo const* spellInfo = GetSpellInfo(itr->second);

            if (!spellInfo)
            {
                TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_group` does not exist", itr->second);
                mSpellGroupSpell.erase(itr++);
            }
            else if (spellInfo->GetRank() > 1)
            {
                TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_group` is not first rank of spell", itr->second);
                mSpellGroupSpell.erase(itr++);
            }
            else
                ++itr;
        }
    }

    for (std::set<uint32>::iterator groupItr = groups.begin(); groupItr != groups.end(); ++groupItr)
    {
        std::set<uint32> spells;
        GetSetOfSpellsInSpellGroup(SpellGroup(*groupItr), spells);

        for (std::set<uint32>::iterator spellItr = spells.begin(); spellItr != spells.end(); ++spellItr)
        {
            ++count;
            mSpellSpellGroup.insert(SpellSpellGroupMap::value_type(*spellItr, SpellGroup(*groupItr)));
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u spell group definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellGroupStackRules()
{
    uint32 oldMSTime = getMSTime();

    mSpellGroupStack.clear();                                  // need for reload case

    //                                                       0         1
    QueryResult result = WorldDatabase.Query("SELECT group_id, stack_rule FROM spell_group_stack_rules");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 spell group stack rules. DB table `spell_group_stack_rules` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 group_id = fields[0].GetUInt32();
        uint8 stack_rule = fields[1].GetInt8();
        if (stack_rule >= SPELL_GROUP_STACK_RULE_MAX)
        {
            TC_LOG_ERROR("sql.sql", "SpellGroupStackRule %u listed in `spell_group_stack_rules` does not exist", stack_rule);
            continue;
        }

        SpellGroupSpellMapBounds spellGroup = GetSpellGroupSpellMapBounds((SpellGroup)group_id);

        if (spellGroup.first == spellGroup.second)
        {
            TC_LOG_ERROR("sql.sql", "SpellGroup id %u listed in `spell_group_stack_rules` does not exist", group_id);
            continue;
        }

        mSpellGroupStack[(SpellGroup)group_id] = (SpellGroupStackRule)stack_rule;

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u spell group stack rules in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellProcEvents()
{
    uint32 oldMSTime = getMSTime();

    mSpellProcEventMap.clear();                             // need for reload case

    //                                                0      1           2                3                 4                 5                 6          7       8        9             10
    QueryResult result = WorldDatabase.Query("SELECT entry, SchoolMask, SpellFamilyName, SpellFamilyMask0, SpellFamilyMask1, SpellFamilyMask2, procFlags, procEx, ppmRate, CustomChance, Cooldown FROM spell_proc_event");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 spell proc event conditions. DB table `spell_proc_event` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        int32 spellId = fields[0].GetInt32();

        bool allRanks = false;
        if (spellId < 0)
        {
            allRanks = true;
            spellId = -spellId;
        }

        SpellInfo const* spellInfo = GetSpellInfo(spellId);
        if (!spellInfo)
        {
            TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_proc_event` does not exist", spellId);
            continue;
        }

        if (allRanks)
        {
            if (!spellInfo->IsRanked())
                TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_proc_event` with all ranks, but spell has no ranks.", spellId);

            if (spellInfo->GetFirstRankSpell()->Id != uint32(spellId))
            {
                TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_proc_event` is not first rank of spell.", spellId);
                continue;
            }
        }

        SpellProcEventEntry spellProcEvent;

        spellProcEvent.schoolMask         = fields[1].GetInt8();
        spellProcEvent.spellFamilyName    = fields[2].GetUInt16();
        spellProcEvent.spellFamilyMask[0] = fields[3].GetUInt32();
        spellProcEvent.spellFamilyMask[1] = fields[4].GetUInt32();
        spellProcEvent.spellFamilyMask[2] = fields[5].GetUInt32();
        spellProcEvent.procFlags          = fields[6].GetUInt32();
        spellProcEvent.procEx             = fields[7].GetUInt32();
        spellProcEvent.ppmRate            = fields[8].GetFloat();
        spellProcEvent.customChance       = fields[9].GetFloat();
        spellProcEvent.cooldown           = fields[10].GetUInt32();

        while (spellInfo)
        {
            if (mSpellProcEventMap.find(spellInfo->Id) != mSpellProcEventMap.end())
            {
                TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_proc_event` already has its first rank in table.", spellInfo->Id);
                break;
            }

            if (!spellInfo->ProcFlags && !spellProcEvent.procFlags)
                TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_proc_event` probably not triggered spell", spellInfo->Id);

            mSpellProcEventMap[spellInfo->Id] = spellProcEvent;

            if (allRanks)
                spellInfo = spellInfo->GetNextRankSpell();
            else
                break;
        }

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u extra spell proc event conditions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellProcs()
{
    uint32 oldMSTime = getMSTime();

    mSpellProcMap.clear();                             // need for reload case

    //                                                 0        1           2                3                 4                 5                 6         7              8               9        10              11             12      13        14
    QueryResult result = WorldDatabase.Query("SELECT spellId, schoolMask, spellFamilyName, spellFamilyMask0, spellFamilyMask1, spellFamilyMask2, typeMask, spellTypeMask, spellPhaseMask, hitMask, attributesMask, ratePerMinute, chance, cooldown, charges FROM spell_proc");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 spell proc conditions and data. DB table `spell_proc` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        int32 spellId = fields[0].GetInt32();

        bool allRanks = false;
        if (spellId < 0)
        {
            allRanks = true;
            spellId = -spellId;
        }

        SpellInfo const* spellInfo = GetSpellInfo(spellId);
        if (!spellInfo)
        {
            TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_proc` does not exist", spellId);
            continue;
        }

        if (allRanks)
        {
            if (!spellInfo->IsRanked())
                TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_proc` with all ranks, but spell has no ranks.", spellId);

            if (spellInfo->GetFirstRankSpell()->Id != uint32(spellId))
            {
                TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_proc` is not first rank of spell.", spellId);
                continue;
            }
        }

        SpellProcEntry baseProcEntry;

        baseProcEntry.schoolMask      = fields[1].GetInt8();
        baseProcEntry.spellFamilyName = fields[2].GetUInt16();
        baseProcEntry.spellFamilyMask[0] = fields[3].GetUInt32();
        baseProcEntry.spellFamilyMask[1] = fields[4].GetUInt32();
        baseProcEntry.spellFamilyMask[2] = fields[5].GetUInt32();
        baseProcEntry.typeMask        = fields[6].GetUInt32();
        baseProcEntry.spellTypeMask   = fields[7].GetUInt32();
        baseProcEntry.spellPhaseMask  = fields[8].GetUInt32();
        baseProcEntry.hitMask         = fields[9].GetUInt32();
        baseProcEntry.attributesMask  = fields[10].GetUInt32();
        baseProcEntry.ratePerMinute   = fields[11].GetFloat();
        baseProcEntry.chance          = fields[12].GetFloat();
        float cooldown                = fields[13].GetFloat();
        baseProcEntry.cooldown        = uint32(cooldown);
        baseProcEntry.charges         = fields[14].GetUInt32();

        while (spellInfo)
        {
            if (mSpellProcMap.find(spellInfo->Id) != mSpellProcMap.end())
            {
                TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_proc` already has its first rank in table.", spellInfo->Id);
                break;
            }

            SpellProcEntry procEntry = SpellProcEntry(baseProcEntry);

            // take defaults from dbcs
            if (!procEntry.typeMask)
                procEntry.typeMask = spellInfo->ProcFlags;
            if (!procEntry.charges)
                procEntry.charges = spellInfo->ProcCharges;
            if (!procEntry.chance && !procEntry.ratePerMinute)
                procEntry.chance = float(spellInfo->ProcChance);

            // validate data
            if (procEntry.schoolMask & ~SPELL_SCHOOL_MASK_ALL)
                TC_LOG_ERROR("sql.sql", "`spell_proc` table entry for spellId %u has wrong `schoolMask` set: %u", spellInfo->Id, procEntry.schoolMask);
            if (procEntry.spellFamilyName && (procEntry.spellFamilyName < 3 || procEntry.spellFamilyName > 17 || procEntry.spellFamilyName == 14 || procEntry.spellFamilyName == 16))
                TC_LOG_ERROR("sql.sql", "`spell_proc` table entry for spellId %u has wrong `spellFamilyName` set: %u", spellInfo->Id, procEntry.spellFamilyName);
            if (procEntry.chance < 0)
            {
                TC_LOG_ERROR("sql.sql", "`spell_proc` table entry for spellId %u has negative value in `chance` field", spellInfo->Id);
                procEntry.chance = 0;
            }
            if (procEntry.ratePerMinute < 0)
            {
                TC_LOG_ERROR("sql.sql", "`spell_proc` table entry for spellId %u has negative value in `ratePerMinute` field", spellInfo->Id);
                procEntry.ratePerMinute = 0;
            }
            if (cooldown < 0)
            {
                TC_LOG_ERROR("sql.sql", "`spell_proc` table entry for spellId %u has negative value in `cooldown` field", spellInfo->Id);
                procEntry.cooldown = 0;
            }
            if (procEntry.chance == 0 && procEntry.ratePerMinute == 0)
                TC_LOG_ERROR("sql.sql", "`spell_proc` table entry for spellId %u doesn't have `chance` and `ratePerMinute` values defined, proc will not be triggered", spellInfo->Id);
            if (procEntry.charges > 99)
            {
                TC_LOG_ERROR("sql.sql", "`spell_proc` table entry for spellId %u has too big value in `charges` field", spellInfo->Id);
                procEntry.charges = 99;
            }
            if (!procEntry.typeMask)
                TC_LOG_ERROR("sql.sql", "`spell_proc` table entry for spellId %u doesn't have `typeMask` value defined, proc will not be triggered", spellInfo->Id);
            if (procEntry.spellTypeMask & ~PROC_SPELL_TYPE_MASK_ALL)
                TC_LOG_ERROR("sql.sql", "`spell_proc` table entry for spellId %u has wrong `spellTypeMask` set: %u", spellInfo->Id, procEntry.spellTypeMask);
            if (procEntry.spellTypeMask && !(procEntry.typeMask & (SPELL_PROC_FLAG_MASK | PERIODIC_PROC_FLAG_MASK)))
                TC_LOG_ERROR("sql.sql", "`spell_proc` table entry for spellId %u has `spellTypeMask` value defined, but it won't be used for defined `typeMask` value", spellInfo->Id);
            if (!procEntry.spellPhaseMask && procEntry.typeMask & REQ_SPELL_PHASE_PROC_FLAG_MASK)
                TC_LOG_ERROR("sql.sql", "`spell_proc` table entry for spellId %u doesn't have `spellPhaseMask` value defined, but it's required for defined `typeMask` value, proc will not be triggered", spellInfo->Id);
            if (procEntry.spellPhaseMask & ~PROC_SPELL_PHASE_MASK_ALL)
                TC_LOG_ERROR("sql.sql", "`spell_proc` table entry for spellId %u has wrong `spellPhaseMask` set: %u", spellInfo->Id, procEntry.spellPhaseMask);
            if (procEntry.spellPhaseMask && !(procEntry.typeMask & REQ_SPELL_PHASE_PROC_FLAG_MASK))
                TC_LOG_ERROR("sql.sql", "`spell_proc` table entry for spellId %u has `spellPhaseMask` value defined, but it won't be used for defined `typeMask` value", spellInfo->Id);
            if (procEntry.hitMask & ~PROC_HIT_MASK_ALL)
                TC_LOG_ERROR("sql.sql", "`spell_proc` table entry for spellId %u has wrong `hitMask` set: %u", spellInfo->Id, procEntry.hitMask);
            if (procEntry.hitMask && !(procEntry.typeMask & TAKEN_HIT_PROC_FLAG_MASK || (procEntry.typeMask & DONE_HIT_PROC_FLAG_MASK && (!procEntry.spellPhaseMask || procEntry.spellPhaseMask & (PROC_SPELL_PHASE_HIT | PROC_SPELL_PHASE_FINISH)))))
                TC_LOG_ERROR("sql.sql", "`spell_proc` table entry for spellId %u has `hitMask` value defined, but it won't be used for defined `typeMask` and `spellPhaseMask` values", spellInfo->Id);

            mSpellProcMap[spellInfo->Id] = procEntry;

            if (allRanks)
                spellInfo = spellInfo->GetNextRankSpell();
            else
                break;
        }
        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u spell proc conditions and data in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellBonusess()
{
    uint32 oldMSTime = getMSTime();

    mSpellBonusMap.clear();                             // need for reload case

    //                                                0      1             2          3         4
    QueryResult result = WorldDatabase.Query("SELECT entry, direct_bonus, dot_bonus, ap_bonus, ap_dot_bonus FROM spell_bonus_data");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 spell bonus data. DB table `spell_bonus_data` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 entry = fields[0].GetUInt32();

        SpellInfo const* spell = GetSpellInfo(entry);
        if (!spell)
        {
            TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_bonus_data` does not exist", entry);
            continue;
        }

        SpellBonusEntry& sbe = mSpellBonusMap[entry];
        sbe.direct_damage = fields[1].GetFloat();
        sbe.dot_damage    = fields[2].GetFloat();
        sbe.ap_bonus      = fields[3].GetFloat();
        sbe.ap_dot_bonus   = fields[4].GetFloat();

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u extra spell bonus data in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellThreats()
{
    uint32 oldMSTime = getMSTime();

    mSpellThreatMap.clear();                                // need for reload case

    //                                                0      1        2       3
    QueryResult result = WorldDatabase.Query("SELECT entry, flatMod, pctMod, apPctMod FROM spell_threat");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 aggro generating spells. DB table `spell_threat` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        if (!GetSpellInfo(entry))
        {
            TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_threat` does not exist", entry);
            continue;
        }

        SpellThreatEntry ste;
        ste.flatMod  = fields[1].GetInt32();
        ste.pctMod   = fields[2].GetFloat();
        ste.apPctMod = fields[3].GetFloat();

        mSpellThreatMap[entry] = ste;
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u SpellThreatEntries in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSkillLineAbilityMap()
{
    uint32 oldMSTime = getMSTime();

    mSkillLineAbilityMap.clear();

    uint32 count = 0;

    for (uint32 i = 0; i < sSkillLineAbilityStore.GetNumRows(); ++i)
    {
        SkillLineAbilityEntry const* SkillInfo = sSkillLineAbilityStore.LookupEntry(i);
        if (!SkillInfo)
            continue;

        mSkillLineAbilityMap.insert(SkillLineAbilityMap::value_type(SkillInfo->spellId, SkillInfo));
        ++count;
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u SkillLineAbility MultiMap Data in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellPetAuras()
{
    uint32 oldMSTime = getMSTime();

    mSpellPetAuraMap.clear();                                  // need for reload case

    //                                                  0       1       2    3
    QueryResult result = WorldDatabase.Query("SELECT spell, effectId, pet, aura FROM spell_pet_auras");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 spell pet auras. DB table `spell_pet_auras` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 spell = fields[0].GetUInt32();
        uint8 eff = fields[1].GetUInt8();
        uint32 pet = fields[2].GetUInt32();
        uint32 aura = fields[3].GetUInt32();

        SpellPetAuraMap::iterator itr = mSpellPetAuraMap.find((spell<<8) + eff);
        if (itr != mSpellPetAuraMap.end())
            itr->second.AddAura(pet, aura);
        else
        {
            SpellInfo const* spellInfo = GetSpellInfo(spell);
            if (!spellInfo)
            {
                TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_pet_auras` does not exist", spell);
                continue;
            }
            if (spellInfo->Effects[eff].Effect != SPELL_EFFECT_DUMMY &&
               (spellInfo->Effects[eff].Effect != SPELL_EFFECT_APPLY_AURA ||
                spellInfo->Effects[eff].ApplyAuraName != SPELL_AURA_DUMMY))
            {
                TC_LOG_ERROR("spells", "Spell %u listed in `spell_pet_auras` does not have dummy aura or dummy effect", spell);
                continue;
            }

            SpellInfo const* spellInfo2 = GetSpellInfo(aura);
            if (!spellInfo2)
            {
                TC_LOG_ERROR("sql.sql", "Aura %u listed in `spell_pet_auras` does not exist", aura);
                continue;
            }

            PetAura pa(pet, aura, spellInfo->Effects[eff].TargetA.GetTarget() == TARGET_UNIT_PET, spellInfo->Effects[eff].CalcValue());
            mSpellPetAuraMap[(spell<<8) + eff] = pa;

            mSpellPetAuraStore.emplace(aura, spell);
        }

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u spell pet auras in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

// Fill custom data about enchancments
void SpellMgr::LoadEnchantCustomAttr()
{
    uint32 oldMSTime = getMSTime();

    uint32 size = sSpellItemEnchantmentStore.GetNumRows();
    mEnchantCustomAttr.resize(size);

    for (uint32 i = 0; i < size; ++i)
       mEnchantCustomAttr[i] = 0;

    uint32 count = 0;
    for (uint32 i = 0; i < GetSpellInfoStoreSize(); ++i)
    {
        SpellInfo const* spellInfo = GetSpellInfo(i);
        if (!spellInfo)
            continue;

        /// @todo find a better check
        if (!spellInfo->HasAttribute(SPELL_ATTR2_PRESERVE_ENCHANT_IN_ARENA) || !spellInfo->HasAttribute(SPELL_ATTR0_NOT_SHAPESHIFT))
            continue;

        for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            if (spellInfo->Effects[j].Effect == SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY)
            {
                uint32 enchId = spellInfo->Effects[j].MiscValue;
                SpellItemEnchantmentEntry const* ench = sSpellItemEnchantmentStore.LookupEntry(enchId);
                if (!ench)
                    continue;
                mEnchantCustomAttr[enchId] = true;
                break;
            }
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u custom enchant attributes in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellEnchantProcData()
{
    uint32 oldMSTime = getMSTime();

    mSpellEnchantProcEventMap.clear();                             // need for reload case

    //                                                  0         1           2         3
    QueryResult result = WorldDatabase.Query("SELECT entry, customChance, PPMChance, procEx FROM spell_enchant_proc_data");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 spell enchant proc event conditions. DB table `spell_enchant_proc_data` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 enchantId = fields[0].GetUInt32();

        SpellItemEnchantmentEntry const* ench = sSpellItemEnchantmentStore.LookupEntry(enchantId);
        if (!ench)
        {
            TC_LOG_ERROR("sql.sql", "Enchancment %u listed in `spell_enchant_proc_data` does not exist", enchantId);
            continue;
        }

        SpellEnchantProcEntry spe;

        spe.customChance = fields[1].GetUInt32();
        spe.PPMChance = fields[2].GetFloat();
        spe.procEx = fields[3].GetUInt32();

        mSpellEnchantProcEventMap[enchantId] = spe;

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u enchant proc data definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellLinked()
{
    uint32 oldMSTime = getMSTime();

    mSpellLinkedMap.clear();    // need for reload case

    //                                                0              1             2
    QueryResult result = WorldDatabase.Query("SELECT spell_trigger, spell_effect, type FROM spell_linked_spell");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 linked spells. DB table `spell_linked_spell` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        int32 trigger = fields[0].GetInt32();
        int32 effect = fields[1].GetInt32();
        int32 type = fields[2].GetUInt8();

        SpellInfo const* spellInfo = GetSpellInfo(abs(trigger));
        if (!spellInfo)
        {
            TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_linked_spell` does not exist", abs(trigger));
            continue;
        }

        if (effect >= 0)
            for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
            {
                if (spellInfo->Effects[j].CalcValue() == abs(effect))
                    TC_LOG_ERROR("sql.sql", "Spell %u Effect: %u listed in `spell_linked_spell` has same bp%u like effect (possible hack)", abs(trigger), abs(effect), j);
            }

        spellInfo = GetSpellInfo(abs(effect));
        if (!spellInfo)
        {
            TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_linked_spell` does not exist", abs(effect));
            continue;
        }

        if (type) //we will find a better way when more types are needed
        {
            if (trigger > 0)
                trigger += SPELL_LINKED_MAX_SPELLS * type;
            else
                trigger -= SPELL_LINKED_MAX_SPELLS * type;
        }
        mSpellLinkedMap[trigger].push_back(effect);

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u linked spells in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadPetLevelupSpellMap()
{
    uint32 oldMSTime = getMSTime();

    mPetLevelupSpellMap.clear();                                   // need for reload case

    uint32 count = 0;
    uint32 family_count = 0;

    for (uint32 i = 0; i < sCreatureFamilyStore.GetNumRows(); ++i)
    {
        CreatureFamilyEntry const* creatureFamily = sCreatureFamilyStore.LookupEntry(i);
        if (!creatureFamily)                                     // not exist
            continue;

        for (uint8 j = 0; j < 2; ++j)
        {
            if (!creatureFamily->skillLine[j])
                continue;

            for (uint32 k = 0; k < sSkillLineAbilityStore.GetNumRows(); ++k)
            {
                SkillLineAbilityEntry const* skillLine = sSkillLineAbilityStore.LookupEntry(k);
                if (!skillLine)
                    continue;

                //if (skillLine->skillId != creatureFamily->skillLine[0] &&
                //    (!creatureFamily->skillLine[1] || skillLine->skillId != creatureFamily->skillLine[1]))
                //    continue;

                if (skillLine->skillId != creatureFamily->skillLine[j])
                    continue;

                if (skillLine->AutolearnType != SKILL_LINE_ABILITY_LEARNED_ON_SKILL_LEARN)
                    continue;

                SpellInfo const* spell = GetSpellInfo(skillLine->spellId);
                if (!spell) // not exist or triggered or talent
                    continue;

                if (!spell->SpellLevel)
                    continue;

                PetLevelupSpellSet& spellSet = mPetLevelupSpellMap[creatureFamily->ID];
                if (spellSet.empty())
                    ++family_count;

                spellSet.insert(PetLevelupSpellSet::value_type(spell->SpellLevel, spell->Id));
                ++count;
            }
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u pet levelup and default spells for %u families in %u ms", count, family_count, GetMSTimeDiffToNow(oldMSTime));
}

bool LoadPetDefaultSpells_helper(CreatureTemplate const* cInfo, PetDefaultSpellsEntry& petDefSpells)
{
    // skip empty list;
    bool have_spell = false;
    for (uint8 j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
    {
        if (petDefSpells.spellid[j])
        {
            have_spell = true;
            break;
        }
    }
    if (!have_spell)
        return false;

    // remove duplicates with levelupSpells if any
    if (PetLevelupSpellSet const* levelupSpells = cInfo->family ? sSpellMgr->GetPetLevelupSpellList(cInfo->family) : NULL)
    {
        for (uint8 j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
        {
            if (!petDefSpells.spellid[j])
                continue;

            for (PetLevelupSpellSet::const_iterator itr = levelupSpells->begin(); itr != levelupSpells->end(); ++itr)
            {
                if (itr->second == petDefSpells.spellid[j])
                {
                    petDefSpells.spellid[j] = 0;
                    break;
                }
            }
        }
    }

    // skip empty list;
    have_spell = false;
    for (uint8 j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
    {
        if (petDefSpells.spellid[j])
        {
            have_spell = true;
            break;
        }
    }

    return have_spell;
}

void SpellMgr::LoadPetDefaultSpells()
{
    uint32 oldMSTime = getMSTime();

    mPetDefaultSpellsMap.clear();

    uint32 countCreature = 0;
    uint32 countData = 0;

    CreatureTemplateContainer const* ctc = sObjectMgr->GetCreatureTemplates();
    for (CreatureTemplateContainer::const_iterator itr = ctc->begin(); itr != ctc->end(); ++itr)
    {

        if (!itr->second.PetSpellDataId)
            continue;

        // for creature with PetSpellDataId get default pet spells from dbc
        CreatureSpellDataEntry const* spellDataEntry = sCreatureSpellDataStore.LookupEntry(itr->second.PetSpellDataId);
        if (!spellDataEntry)
            continue;

        int32 petSpellsId = -int32(itr->second.PetSpellDataId);
        PetDefaultSpellsEntry petDefSpells;
        for (uint8 j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
            petDefSpells.spellid[j] = spellDataEntry->spellId[j];

        if (LoadPetDefaultSpells_helper(&itr->second, petDefSpells))
        {
            mPetDefaultSpellsMap[petSpellsId] = petDefSpells;
            ++countData;
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded addition spells for %u pet spell data entries in %u ms", countData, GetMSTimeDiffToNow(oldMSTime));

    TC_LOG_INFO("server.loading", "Loading summonable creature templates...");
    oldMSTime = getMSTime();

    // different summon spells
    for (uint32 i = 0; i < GetSpellInfoStoreSize(); ++i)
    {
        SpellInfo const* spellEntry = GetSpellInfo(i);
        if (!spellEntry)
            continue;

        for (uint8 k = 0; k < MAX_SPELL_EFFECTS; ++k)
        {
            if (spellEntry->Effects[k].Effect == SPELL_EFFECT_SUMMON || spellEntry->Effects[k].Effect == SPELL_EFFECT_SUMMON_PET)
            {
                uint32 creature_id = spellEntry->Effects[k].MiscValue;
                CreatureTemplate const* cInfo = sObjectMgr->GetCreatureTemplate(creature_id);
                if (!cInfo)
                    continue;

                // already loaded
                if (cInfo->PetSpellDataId)
                    continue;

                // for creature without PetSpellDataId get default pet spells from creature_template
                int32 petSpellsId = cInfo->Entry;
                if (mPetDefaultSpellsMap.find(cInfo->Entry) != mPetDefaultSpellsMap.end())
                    continue;

                PetDefaultSpellsEntry petDefSpells;
                for (uint8 j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
                    petDefSpells.spellid[j] = cInfo->spells[j];

                if (LoadPetDefaultSpells_helper(cInfo, petDefSpells))
                {
                    mPetDefaultSpellsMap[petSpellsId] = petDefSpells;
                    ++countCreature;
                }
            }
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u summonable creature templates in %u ms", countCreature, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellAreas()
{
    uint32 oldMSTime = getMSTime();

    mSpellAreaMap.clear();                                  // need for reload case
    mSpellAreaForQuestMap.clear();
    mSpellAreaForQuestEndMap.clear();
    mSpellAreaForAuraMap.clear();

    //                                                  0     1         2              3               4                 5          6          7       8         9
    QueryResult result = WorldDatabase.Query("SELECT spell, area, quest_start, quest_start_status, quest_end_status, quest_end, aura_spell, racemask, gender, autocast FROM spell_area");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 spell area requirements. DB table `spell_area` is empty.");

        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 spell = fields[0].GetUInt32();
        SpellArea spellArea;
        spellArea.spellId             = spell;
        spellArea.areaId              = fields[1].GetUInt32();
        spellArea.questStart          = fields[2].GetUInt32();
        spellArea.questStartStatus    = fields[3].GetUInt32();
        spellArea.questEndStatus      = fields[4].GetUInt32();
        spellArea.questEnd            = fields[5].GetUInt32();
        spellArea.auraSpell           = fields[6].GetInt32();
        spellArea.raceMask            = fields[7].GetUInt32();
        spellArea.gender              = Gender(fields[8].GetUInt8());
        spellArea.autocast            = fields[9].GetBool();

        if (SpellInfo const* spellInfo = GetSpellInfo(spell))
        {
            if (spellArea.autocast)
                const_cast<SpellInfo*>(spellInfo)->Attributes |= SPELL_ATTR0_CANT_CANCEL;
        }
        else
        {
            TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_area` does not exist", spell);
            continue;
        }

        {
            bool ok = true;
            SpellAreaMapBounds sa_bounds = GetSpellAreaMapBounds(spellArea.spellId);
            for (SpellAreaMap::const_iterator itr = sa_bounds.first; itr != sa_bounds.second; ++itr)
            {
                if (spellArea.spellId != itr->second.spellId)
                    continue;
                if (spellArea.areaId != itr->second.areaId)
                    continue;
                if (spellArea.questStart != itr->second.questStart)
                    continue;
                if (spellArea.auraSpell != itr->second.auraSpell)
                    continue;
                if ((spellArea.raceMask & itr->second.raceMask) == 0)
                    continue;
                if (spellArea.gender != itr->second.gender)
                    continue;

                // duplicate by requirements
                ok = false;
                break;
            }

            if (!ok)
            {
                TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_area` already listed with similar requirements.", spell);
                continue;
            }
        }

        if (spellArea.areaId && !sAreaTableStore.LookupEntry(spellArea.areaId))
        {
            TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_area` have wrong area (%u) requirement", spell, spellArea.areaId);
            continue;
        }

        if (spellArea.questStart && !sObjectMgr->GetQuestTemplate(spellArea.questStart))
        {
            TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_area` have wrong start quest (%u) requirement", spell, spellArea.questStart);
            continue;
        }

        if (spellArea.questEnd)
        {
            if (!sObjectMgr->GetQuestTemplate(spellArea.questEnd))
            {
                TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_area` have wrong end quest (%u) requirement", spell, spellArea.questEnd);
                continue;
            }
        }

        if (spellArea.auraSpell)
        {
            SpellInfo const* spellInfo = GetSpellInfo(abs(spellArea.auraSpell));
            if (!spellInfo)
            {
                TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_area` have wrong aura spell (%u) requirement", spell, abs(spellArea.auraSpell));
                continue;
            }

            if (uint32(abs(spellArea.auraSpell)) == spellArea.spellId)
            {
                TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_area` have aura spell (%u) requirement for itself", spell, abs(spellArea.auraSpell));
                continue;
            }

            // not allow autocast chains by auraSpell field (but allow use as alternative if not present)
            if (spellArea.autocast && spellArea.auraSpell > 0)
            {
                bool chain = false;
                SpellAreaForAuraMapBounds saBound = GetSpellAreaForAuraMapBounds(spellArea.spellId);
                for (SpellAreaForAuraMap::const_iterator itr = saBound.first; itr != saBound.second; ++itr)
                {
                    if (itr->second->autocast && itr->second->auraSpell > 0)
                    {
                        chain = true;
                        break;
                    }
                }

                if (chain)
                {
                    TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_area` have aura spell (%u) requirement that itself autocast from aura", spell, spellArea.auraSpell);
                    continue;
                }

                SpellAreaMapBounds saBound2 = GetSpellAreaMapBounds(spellArea.auraSpell);
                for (SpellAreaMap::const_iterator itr2 = saBound2.first; itr2 != saBound2.second; ++itr2)
                {
                    if (itr2->second.autocast && itr2->second.auraSpell > 0)
                    {
                        chain = true;
                        break;
                    }
                }

                if (chain)
                {
                    TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_area` have aura spell (%u) requirement that itself autocast from aura", spell, spellArea.auraSpell);
                    continue;
                }
            }
        }

        if (spellArea.raceMask && (spellArea.raceMask & RACEMASK_ALL_PLAYABLE) == 0)
        {
            TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_area` have wrong race mask (%u) requirement", spell, spellArea.raceMask);
            continue;
        }

        if (spellArea.gender != GENDER_NONE && spellArea.gender != GENDER_FEMALE && spellArea.gender != GENDER_MALE)
        {
            TC_LOG_ERROR("sql.sql", "Spell %u listed in `spell_area` have wrong gender (%u) requirement", spell, spellArea.gender);
            continue;
        }

        SpellArea const* sa = &mSpellAreaMap.insert(SpellAreaMap::value_type(spell, spellArea))->second;

        // for search by current zone/subzone at zone/subzone change
        if (spellArea.areaId)
            mSpellAreaForAreaMap.insert(SpellAreaForAreaMap::value_type(spellArea.areaId, sa));

        // for search at quest start/reward
        if (spellArea.questStart)
            mSpellAreaForQuestMap.insert(SpellAreaForQuestMap::value_type(spellArea.questStart, sa));

        // for search at quest start/reward
        if (spellArea.questEnd)
            mSpellAreaForQuestEndMap.insert(SpellAreaForQuestMap::value_type(spellArea.questEnd, sa));

        // for search at aura apply
        if (spellArea.auraSpell)
            mSpellAreaForAuraMap.insert(SpellAreaForAuraMap::value_type(abs(spellArea.auraSpell), sa));

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u spell area requirements in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellInfoStore()
{
    uint32 oldMSTime = getMSTime();

    UnloadSpellInfoStore();
    mSpellInfoMap.resize(sSpellStore.GetNumRows(), NULL);

    for (uint32 i = 0; i < sSpellStore.GetNumRows(); ++i)
        if (SpellEntry const* spellEntry = sSpellStore.LookupEntry(i))
            mSpellInfoMap[i] = new SpellInfo(spellEntry);

    TC_LOG_INFO("server.loading", ">> Loaded SpellInfo store in %u ms", GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::UnloadSpellInfoStore()
{
    for (uint32 i = 0; i < GetSpellInfoStoreSize(); ++i)
        delete mSpellInfoMap[i];

    mSpellInfoMap.clear();
}

void SpellMgr::UnloadSpellInfoImplicitTargetConditionLists()
{
    for (uint32 i = 0; i < GetSpellInfoStoreSize(); ++i)
        if (mSpellInfoMap[i])
            mSpellInfoMap[i]->_UnloadImplicitTargetConditionLists();
}

void SpellMgr::LoadSpellInfoCustomAttributes()
{
    uint32 oldMSTime = getMSTime();
    uint32 oldMSTime2 = oldMSTime;
    SpellInfo* spellInfo = NULL;

    QueryResult result = WorldDatabase.Query("SELECT entry, attributes FROM spell_custom_attr");

    if (!result)
        TC_LOG_INFO("server.loading", ">> Loaded 0 spell custom attributes from DB. DB table `spell_custom_attr` is empty.");
    else
    {
        uint32 count = 0;
        do
        {
            Field* fields = result->Fetch();

            uint32 spellId = fields[0].GetUInt32();
            uint32 attributes = fields[1].GetUInt32();

            spellInfo = _GetSpellInfo(spellId);
            if (!spellInfo)
            {
                TC_LOG_ERROR("sql.sql", "Table `spell_custom_attr` has wrong spell (entry: %u), ignored.", spellId);
                continue;
            }

            // TODO: validate attributes

            spellInfo->AttributesCu |= attributes;
            ++count;
        } while (result->NextRow());

        TC_LOG_INFO("server.loading", ">> Loaded %u spell custom attributes from DB in %u ms", count, GetMSTimeDiffToNow(oldMSTime2));
    }

    for (uint32 i = 0; i < GetSpellInfoStoreSize(); ++i)
    {
        spellInfo = mSpellInfoMap[i];
        if (!spellInfo)
            continue;

        for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            switch (spellInfo->Effects[j].ApplyAuraName)
            {
                case SPELL_AURA_PERIODIC_HEAL:
                case SPELL_AURA_PERIODIC_DAMAGE:
                case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
                case SPELL_AURA_PERIODIC_LEECH:
                case SPELL_AURA_PERIODIC_MANA_LEECH:
                case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
                case SPELL_AURA_PERIODIC_ENERGIZE:
                case SPELL_AURA_OBS_MOD_HEALTH:
                case SPELL_AURA_OBS_MOD_POWER:
                case SPELL_AURA_POWER_BURN:
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_NO_INITIAL_THREAT;
                    break;
            }

            switch (spellInfo->Effects[j].Effect)
            {
                case SPELL_EFFECT_SCHOOL_DAMAGE:
                case SPELL_EFFECT_WEAPON_DAMAGE:
                case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                case SPELL_EFFECT_HEAL:
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_DIRECT_DAMAGE;
                    break;
                case SPELL_EFFECT_POWER_DRAIN:
                case SPELL_EFFECT_POWER_BURN:
                case SPELL_EFFECT_HEAL_MAX_HEALTH:
                case SPELL_EFFECT_HEALTH_LEECH:
                case SPELL_EFFECT_HEAL_PCT:
                case SPELL_EFFECT_ENERGIZE_PCT:
                case SPELL_EFFECT_ENERGIZE:
                case SPELL_EFFECT_HEAL_MECHANICAL:
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_NO_INITIAL_THREAT;
                    break;
                case SPELL_EFFECT_CHARGE:
                case SPELL_EFFECT_CHARGE_DEST:
                case SPELL_EFFECT_JUMP:
                case SPELL_EFFECT_JUMP_DEST:
                case SPELL_EFFECT_LEAP_BACK:
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_CHARGE;
                    break;
                case SPELL_EFFECT_PICKPOCKET:
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_PICKPOCKET;
                    break;
                case SPELL_EFFECT_ENCHANT_ITEM:
                case SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY:
                case SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC:
                case SPELL_EFFECT_ENCHANT_HELD_ITEM:
                {
                    // only enchanting profession enchantments procs can stack
                    if (IsPartOfSkillLine(SKILL_ENCHANTING, i))
                    {
                        uint32 enchantId = spellInfo->Effects[j].MiscValue;
                        SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
                        if (!enchant)
                            break;

                        for (uint8 s = 0; s < MAX_ITEM_ENCHANTMENT_EFFECTS; ++s)
                        {
                            if (enchant->type[s] != ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL)
                                continue;

                            SpellInfo* procInfo = _GetSpellInfo(enchant->spellid[s]);
                            if (!procInfo)
                                continue;

                            // if proced directly from enchantment, not via proc aura
                            // NOTE: Enchant Weapon - Blade Ward also has proc aura spell and is proced directly
                            // however its not expected to stack so this check is good
                            if (procInfo->HasAura(SPELL_AURA_PROC_TRIGGER_SPELL))
                                continue;

                            procInfo->AttributesCu |= SPELL_ATTR0_CU_ENCHANT_PROC;
                        }
                    }
                    break;
                }
            }
        }

        //! Binary Spells
        //! All binary spells must have damage_class_magic and musn't be a poison.
        if (spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MAGIC && spellInfo->Dispel != DISPEL_POISON)
        {
            //! At the begin all spells are binary
            bool isBinarySpell = true;

            bool exitLoop = false;
            for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
            {
                switch (spellInfo->Effects[j].Effect)
                {
                        //! All spells with a damage-effect are non binary, if they have no additional effect.
                    case SPELL_EFFECT_SCHOOL_DAMAGE:
                    case SPELL_EFFECT_WEAPON_DAMAGE:
                    case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                    case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                    case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                    case SPELL_EFFECT_HEAL:
                        isBinarySpell = false;
                        break;
                    case SPELL_EFFECT_ATTACK_ME:
                        isBinarySpell = false;
                        exitLoop = true;
                        break;
                        //! spells which have a periodic-damage-aura are non binary.
                        //! All of them don't care about additional effects.
                    case SPELL_EFFECT_APPLY_AURA:
                        if (spellInfo->Effects[j].ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE ||
                            spellInfo->Effects[j].ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE_PERCENT ||
                            spellInfo->Effects[j].ApplyAuraName == SPELL_AURA_PERIODIC_LEECH ||
                            spellInfo->Effects[j].ApplyAuraName == SPELL_AURA_MOD_TAUNT)
                        {
                            isBinarySpell = false;
                            exitLoop = true;
                        }
                        //! spells with apply aura effect which don't have a periodic-damage-aura are binary.
                        else
                            isBinarySpell = true;
                        break;
                    default:
                        //! At this point spell is an only-damage-spell, that means binary.
                        //! No need to check effect_3 if effect_2 is 0
                        if (spellInfo->Effects[j].Effect == 0)
                            exitLoop = true;
                        //! All spells which have a non-damage-effect and no periodic-damage-aura are binary.
                        else
                        {
                            isBinarySpell = true;
                            exitLoop = true;
                        }
                        break;
                }
                if (exitLoop)
                    break;
            }

            //! some expection for the upper rules:
            //! mage firebolt
            if (spellInfo->SpellFamilyName == SPELLFAMILY_MAGE && spellInfo->SpellIconID == 185)
                isBinarySpell = false;
            //! mage - frostbolt
            if (spellInfo->SpellFamilyName == SPELLFAMILY_MAGE && spellInfo->SpellIconID == 188)
                isBinarySpell = false;
            //! warlock - chaos bolt
            if (spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK && spellInfo->SpellIconID == 3178)
                isBinarySpell = false;
            //! Mage Arcane Blast
            if (spellInfo->SpellFamilyName == SPELLFAMILY_MAGE && spellInfo->SpellIconID == 2294)
                isBinarySpell = false;
            //! DK Icy Touch
            if (spellInfo->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT && spellInfo->SpellIconID == 2721)
                isBinarySpell = false;
            //! DK howling blast
            if (spellInfo->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT && spellInfo->SpellIconID == 2131)
                isBinarySpell = false;
            //! Warlock Haunt
            if (spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK && spellInfo->SpellIconID == 3172)
                isBinarySpell = false;
            //! Druid Starfall
            if (spellInfo->SpellFamilyName == SPELLFAMILY_DRUID && spellInfo->SpellIconID == 2854)
                isBinarySpell = false;

            //! If spell is binary set the custom attribute, to identify it later
            if (isBinarySpell)
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_BINARY;
        }

        if (!spellInfo->_IsPositiveEffect(EFFECT_0, false))
            spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE_EFF0;

        if (!spellInfo->_IsPositiveEffect(EFFECT_1, false))
            spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE_EFF1;

        if (!spellInfo->_IsPositiveEffect(EFFECT_2, false))
            spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE_EFF2;

        if (spellInfo->SpellVisual[0] == 3879)
            spellInfo->AttributesCu |= SPELL_ATTR0_CU_CONE_BACK;

        switch (spellInfo->SpellFamilyName)
        {
            case SPELLFAMILY_HUNTER:
                // Monstrous Bite
                // seems we incorrectly handle spells with Targets (0, 0) (NO_TARGET, NO_TARGET)
                if (spellInfo->SpellIconID == 599)
                    spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_CASTER;
                break;
            default:
                break;
        }

        spellInfo->_InitializeExplicitTargetMask();
    }

    for (uint32 i = 0; i < GetSpellInfoStoreSize(); ++i)
    {
        spellInfo = mSpellInfoMap[i];
        if (!spellInfo)
            continue;

        // remove several trigegr/passive spells. These spells are learned but never show up in spellbook
        if (spellInfo->HasAttribute(SPELL_ATTR0_PASSIVE))
            continue;
        if (spellInfo->HasAttribute(SPELL_ATTR0_HIDDEN_CLIENTSIDE))
            continue;
        if (spellInfo->HasAttribute(SPELL_ATTR3_DEATH_PERSISTENT))
            continue;
        if (spellInfo->HasAttribute(SPELL_ATTR1_FARSIGHT))
            continue;

        // player ai only is active in combat... self explaining
        if (spellInfo->HasAttribute(SPELL_ATTR0_CANT_USED_IN_COMBAT))
            continue;

        // no potion spells or pet spells
        if (spellInfo->SpellFamilyName == SPELLFAMILY_POTION || spellInfo->SpellFamilyName == SPELLFAMILY_PET)
            continue;

        // player ai does not use spells which cost something
        bool costsReagent = false;
        for (uint8 i = 0; i < MAX_SPELL_REAGENTS && !costsReagent; ++i)
            costsReagent = spellInfo->Reagent[i] != 0;
        if (costsReagent)
            continue;

        // duration infinity. some of these spells dont work properly
        if (spellInfo->DurationEntry && spellInfo->DurationEntry->ID == 21)
            continue;

        bool isPlayerAISpell = true;
        for (auto& effect : spellInfo->Effects)
        {
            switch (effect.Effect)
            {
                case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                case SPELL_EFFECT_CREATE_ITEM:
                case SPELL_EFFECT_CREATE_ITEM_2:
                case SPELL_EFFECT_CREATE_MANA_GEM:
                case SPELL_EFFECT_CREATE_RANDOM_ITEM:
                case SPELL_EFFECT_CREATE_TAMED_PET:
                case SPELL_EFFECT_SANCTUARY:
                case SPELL_EFFECT_LEAP:
                case SPELL_EFFECT_LEAP_BACK:
                case SPELL_EFFECT_TRADE_SKILL:

                case SPELL_EFFECT_SUMMON: // temporary disabled due to internal implementation of pets (causes to many issues with pets)
                    isPlayerAISpell = false;
                default:
                    break;
            }

            if (isPlayerAISpell && effect.IsEffect())
            {
                switch (effect.TargetA.GetTarget())
                {
                    case TARGET_UNIT_TARGET_ENEMY:
                    case TARGET_UNIT_DEST_AREA_ENEMY:
                    case TARGET_DEST_DYNOBJ_ENEMY:

                    case TARGET_UNIT_CASTER:
                    case TARGET_SRC_CASTER:
                    case TARGET_DEST_CASTER:
                    case TARGET_DEST_TARGET_ENEMY:
                    case TARGET_DEST_DYNOBJ_ALLY:
                    case TARGET_DEST_TARGET_ANY:
                    case TARGET_DEST_TARGET_FRONT:
                    case TARGET_DEST_TARGET_BACK:
                    case TARGET_DEST_TARGET_RIGHT:
                    case TARGET_DEST_TARGET_LEFT:
                    case TARGET_UNIT_CASTER_AREA_RAID:
                    case TARGET_DEST_CASTER_FRONT_RIGHT:
                    case TARGET_DEST_CASTER_BACK_RIGHT:
                    case TARGET_DEST_CASTER_BACK_LEFT:
                    case TARGET_DEST_CASTER_FRONT_LEFT:
                    case TARGET_DEST_CASTER_FRONT:
                    case TARGET_DEST_CASTER_BACK:
                    case TARGET_DEST_CASTER_RIGHT:
                    case TARGET_DEST_CASTER_LEFT:
                    case TARGET_UNIT_TARGET_ALLY:
                    case TARGET_UNIT_TARGET_RAID:
                        break;
                    default:
                        isPlayerAISpell = false;
                        break;
                }

                // no homestone
                if (effect.TargetB.GetTarget() == TARGET_DEST_HOME)
                    isPlayerAISpell = false;

                // no posessing while charmed. never tried what happens - but could be weird
                if (effect.ApplyAuraName == SPELL_AURA_MOD_POSSESS)
                    isPlayerAISpell = false;
            }
        }

        // algogithm doesn't work here. so special selection
        switch (spellInfo->Id)
        {
            case 42650:
            case 46584:
            case 47788:
            case 48020:
            case 49016:
            case 61999:
                isPlayerAISpell = false;
                break;
            case 740:
            case 45524:
                isPlayerAISpell = true;
                break;
            default:
                break;
        }
        if (spellInfo->SpellIconID == 100)
            isPlayerAISpell = true;

        // general selection is done. rest is done by player ai
        if (isPlayerAISpell)
            spellInfo->AttributesCu |= SPELL_ATTR0_CU_USABLE_FOR_PLAYER_AI;
    }

    TC_LOG_INFO("server.loading", ">> Loaded SpellInfo custom attributes in %u ms", GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellInfoCorrections()
{
    uint32 oldMSTime = getMSTime();

    SpellInfo* spellInfo = NULL;
    for (uint32 i = 0; i < GetSpellInfoStoreSize(); ++i)
    {
        spellInfo = mSpellInfoMap[i];
        if (!spellInfo)
            continue;

        for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            switch (spellInfo->Effects[j].Effect)
            {
                case SPELL_EFFECT_CHARGE:
                case SPELL_EFFECT_CHARGE_DEST:
                case SPELL_EFFECT_JUMP:
                case SPELL_EFFECT_JUMP_DEST:
                case SPELL_EFFECT_LEAP_BACK:
                    if (!spellInfo->Speed && !spellInfo->SpellFamilyName)
                        spellInfo->Speed = SPEED_CHARGE;
                    break;
            }
        }

        if (spellInfo->ActiveIconID == 2158)  // flight
            spellInfo->Attributes |= SPELL_ATTR0_PASSIVE;

        switch (spellInfo->Id)
        {
            case 7855:
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_DEST_CASTER_LEFT);
                break;
            case 54559: // Energy Surge
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 5321:  // Ishamuhale's Rage (Rank 1)
            case 5325:  // Strength of Isha Awak (Rank 1)
            case 5322:  // Lakota'mani's Thunder (Rank 1)
                spellInfo->Effects[0].MiscValue = UNIT_MOD_STAT_STAMINA;
                break;
            case 39010: // Challenge from the Horde
                spellInfo->Effects[1].Effect = 0;
                break;
            case 46443: // Weakness to Lightning: Kill Credit Direct to Player
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ANY);
                break;
            case 68987: // Pursuit
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 11389:
                spellInfo->PowerType = 0;
                spellInfo->ManaCost = 0;
                spellInfo->ManaPerSecond = 0;
                break;
            case 11758: // Dowsing
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(3); // 60secs, was 5 Minutes
                break;
            case 64844: // Divine Hymn
                spellInfo->DmgClass = SPELL_DAMAGE_CLASS_MAGIC;
            break;
            case 54433: // Hostile Airspace
            case 42380: // Conflagration
                spellInfo->Attributes |= SPELL_ATTR0_NEGATIVE_1;
                spellInfo->Attributes |= SPELL_ATTR0_CANT_CANCEL;
                break;
             case 7328:  // Redemption
             case 7329:  // Redemption
             case 10322: // Redemption
             case 10324: // Redemption
             case 20772: // Redemption
             case 20773: // Redemption
             case 48949: // Redemption
             case 48950: // Redemption
                 spellInfo->SpellFamilyName = SPELLFAMILY_PALADIN;
                 break; 
            case 53096: // Quetz'lun's Judgment
            case 70743: // AoD Special
            case 70614: // AoD Special - Vegard
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 42730:
                spellInfo->Effects[EFFECT_1].TriggerSpell = 42739;
                break;
            case 42436: // Drink! (Brewfest)
            case 33388: // Apprentice Riding
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ANY);
                break;
            case 43730: // Stormchops
                spellInfo->Effects[EFFECT_1].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
                spellInfo->Effects[EFFECT_1].TargetB = SpellImplicitTargetInfo();
                break;
            case 49214: // New Summon Test, summons angry murloc for quest: Red Snapper - Very Tasty!
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(3); // 60secs, was 15 which is to low to loot them
                break;
            case 70132: // Empowered Blizzard
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ENEMY);
                break;
            case 674: // Shaman Dual Wield has skill
                spellInfo->Effects[1].Effect = SPELL_EFFECT_SKILL;
                spellInfo->Effects[1].BasePoints = 1;
                spellInfo->Effects[1].MiscValue = 118; // Dual Wield skill
                break;
            case 13812: // Explosive Trap Effect
            case 14314: // Explosive Trap Effect
            case 14315: // Explosive Trap Effect
            case 27026: // Explosive Trap Effect
            case 43446: // Explosive Trap Effect
            case 49064: // Explosive Trap Effect
            case 49065: // Explosive Trap Effect
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_DEST_AREA_ENTRY;
                spellInfo->AttributesEx6 |= SPELL_ATTR6_CAN_TARGET_INVISIBLE;
                break;
            case 19408: // Panic - Magmadar
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_30_YARDS);
                break;
            case 32307: // Plant Warmaul Ogre Banner
                spellInfo->Effects[EFFECT_1].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ENEMY);
                break;
            case 59735:
                spellInfo->Effects[EFFECT_1].TriggerSpell = 59736;
                break;
            case 52611: // Summon Skeletons
            case 52612: // Summon Skeletons
            case 53334: // Animate Bones
            case 53520: // Carrion Beetles
            case 53521: // Carrion Beetles
            case 44879: // Living Flare
            case 44999: // Summon Converted Sentry
            case 4100:  // Goblin Land Mine
                spellInfo->Effects[EFFECT_0].MiscValueB = 64;
                break;
            case 61544: // Summon Budd PET
                spellInfo->Effects[EFFECT_0].MiscValueB = 67;
                break;
            case 39793: // Call Cenarion Sparrowhawk
                spellInfo->Effects[EFFECT_0].MiscValueB = 61;
                break;
            case 40244: // Simon Game Visual
            case 40245: // Simon Game Visual
            case 40246: // Simon Game Visual
            case 40247: // Simon Game Visual
            case 42835: // Spout, remove damage effect, only anim is needed
            case 46085: // Place Fake Fur
            case 42793: // Burn Body - summon gameobjecteffect not needed
            case 40900: // Corlok's Skull Barrage Knockdown
            case 41064: // Sky Shatter
            case 40929: // Ichman's Blazing Fireball Knockdown
            case 40931: // Mulverick's Great Balls of Lightning
            case 40905: // Oldie's Rotten Pumpkin Knockdown
                spellInfo->Effects[EFFECT_0].Effect = 0;
                break;
            case 30657: // Quake
                spellInfo->Effects[EFFECT_0].TriggerSpell = 30571;
                break;
            case 50051: // Ethereal Pet Aura
                spellInfo->Effects[EFFECT_0].TriggerSpell = 50050;
                break;
            case 52922: // Dk - Startarea, The Inquisitor's Penance
            case 52923: // Dk - Startarea, The Inquisitor's Penance
            case 26476: // Digestive Acid
            case 40241: // Speed of the Falcon
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
            case 30541: // Blaze (needs conditions entry)
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ENEMY);
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo();
                break;
            case 37433: // Spout
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ENEMY);
                spellInfo->Effects[EFFECT_1].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ENEMY);
                break;
            case 63665: // Charge (Argent Tournament emote on riders)
            case 31298: // Sleep (needs target selection script)
            case 51904: // Summon Ghouls On Scarlet Crusade (this should use conditions table, script for this spell needs to be fixed)
            case 2895:  // Wrath of Air Totem rank 1 (Aura)
            case 68933: // Wrath of Air Totem rank 2 (Aura)
            case 29200: // Purify Helboar Meat
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo();
                break;
            case 55028: // Summon Freed Proto-Drake
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
                break;
            case 13982: // Bael'Gar's Fiery Essence
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo();
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_DUMMY;
                spellInfo->Effects[EFFECT_1].BasePoints = 1;
                spellInfo->Effects[EFFECT_1].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ANY);
                spellInfo->Effects[EFFECT_1].TargetB = SpellImplicitTargetInfo();
                break;
            case 38202: // Heat Mold
            case 39703: // Curse of Mending
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ANY);
                break;
            case 58600: // Restricted Flight Area (Dalaran)
                spellInfo->Effects[EFFECT_1].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
                break;
            case 68778: // Chilling Wave
            case 70333: // Chilling Wave
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_DEST_TARGET_ENEMY);
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo();
                break;
            case 26022: // Pursuit of Justice Rank 1
                spellInfo->Effects[EFFECT_1].Effect = 6;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_MOD_MOUNTED_AND_FLIGHT_SPEED_NOT_STACK;
                spellInfo->Effects[EFFECT_1].BasePoints = 7;
                break;
            case 26023: // Pursuit of Justice Rank 2
                spellInfo->Effects[EFFECT_1].Effect = 6;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_MOD_MOUNTED_AND_FLIGHT_SPEED_NOT_STACK;
                spellInfo->Effects[EFFECT_1].BasePoints = 14;
                break;
            case 31344: // Howl of Azgalor
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_100_YARDS); // 100yards instead of 50000?!
                break;
            case 42818: // Headless Horseman - Wisp Flight Port
            case 42821: // Headless Horseman - Wisp Flight Missile
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(6); // 100 yards
                break;
            case 36350: // They Must Burn Bomb Aura (self)
                spellInfo->Effects[EFFECT_0].TriggerSpell = 36325; // They Must Burn Bomb Drop (DND)
                break;
            case 3600:  // Shaman Earthbin Totem - should not generate threat
            case 8187:  // Magma Totem Spells -> should not generate any threat
            case 10579:
            case 10580:
            case 10581:
            case 25550:
            case 58732:
            case 58735:
            case 57560: // Obsi: Cyclone Aura
            case 57562: // Obsi: Burning Winds
            case 61138: // Wild Charge
            case 61132: // Wild Charge
            case 49376: // Wild Charge
            case 50259: // Wild Charge
            case 63944: // Renewed Hope
            case 68066: // Renewed Hope
                spellInfo->AttributesEx |= SPELL_ATTR1_NO_THREAT;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                break;
            case 41637: // Prayer of Mending
            case 48110: // Prayer of Mending
            case 48111: // Prayer of Mending
            case 47755: // Rapture
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                break;
            case 41635: // Prayer of Mending
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                break;
            case 52610: // Savage Roar
                spellInfo->AttributesEx |= SPELL_ATTR1_NO_THREAT;
                spellInfo->AttributesEx |= SPELL_ATTR1_NOT_BREAK_STEALTH;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                break;
            case 62758: // Wild Hunt 
            case 62762:
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 45437: // Stamp Out Bonfire
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_DUMMY;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_NEARBY_ENTRY;
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;                
            case 29831: // Light Bonfire (DND)
            case 38134: // Tainted Core
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 42490: // Energized!
            case 42492: // Cast Energized
                spellInfo->AttributesEx |= SPELL_ATTR1_NO_THREAT;
                break;
            case 18662: // Curse of Doom Effect
            case 54994: // Summon Stormforged Pursuer
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(21); // Summoned Unit should not despawn
                break;
            case 49838: // Stop Time
            case 34919: // Vampiric Touch (Energize)
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                break;
            case 61407: // Energize Cores
            case 62136: // Energize Cores
            case 54069: // Energize Cores
            case 56251: // Energize Cores
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_SRC_AREA_ENTRY);
                break;
            case 50785: // Energize Cores
            case 59372: // Energize Cores
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_SRC_AREA_ENEMY);
                break;
            case 20335: // Heart of the Crusader
            case 20336:
            case 20337:
            case 53228: // Rapid Killing (Rank 1)
            case 53232: // Rapid Killing (Rank 2)
                // Entries were not updated after spell effect change, we have to do that manually :/
                spellInfo->AttributesEx3 |= SPELL_ATTR3_CAN_PROC_WITH_TRIGGERED;
                break;
            case 5308:  // Execute (Rank 1)
            case 20658: // Execute (Rank 2)
            case 20660: // Execute (Rank 3)
            case 20661: // Execute (Rank 4)
            case 20662: // Execute (Rank 5)
            case 25234: // Execute (Rank 6)
            case 25236: // Execute (Rank 7)
            case 47470: // Execute (Rank 8)
            case 47471: // Execute (Rank 9)
                spellInfo->AttributesEx3 |= SPELL_ATTR3_CANT_TRIGGER_PROC;
                break;
            case 59725: // Improved Spell Reflection - aoe aura
                // Target entry seems to be wrong for this spell :/
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER_AREA_PARTY);
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_20_YARDS);
                break;
            case 19506: // Trueshot Aura
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_100_YARDS);
                break;
            case 61455: // death knight - runic focus
                // Because runic focus spell is taken into account multiplicative 
                // unlike other SPELLMOD_CRIT_DAMAGE_BONUS
                spellInfo->Effects[0].SpellClassMask = SPELL_ATTR3_NO_DONE_BONUS;
                break;                
            case 19185: // Entrapment trigger
            case 64803: // Entrapment trigger
            case 64804: // Entrapment trigger
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_DEST_TARGET_ENEMY);
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo(TARGET_UNIT_DEST_AREA_ENEMY);
                spellInfo->AttributesEx5 |= SPELL_ATTR5_UNK26;
                break;
            case 1463: // Mana Shield
            case 8494:
            case 8495:
            case 10191:
            case 10192:
            case 10193:
            case 27131:
            case 43019:
            case 43020:
                spellInfo->ProcChance = 100;
                break;
            case 44978: // Wild Magic
            case 45001:
            case 45002:
            case 45004:
            case 45006:
            case 45010:
            case 31347: // Doom
            case 44869: // Spectral Blast
            case 45027: // Revitalize
            case 45976: // Muru Portal Channel
            case 39365: // Thundering Storm
            case 41071: // Raise Dead (HACK)
            case 52124: // Sky Darkener Assault
            case 42442: // Vengeance Landing Cannonfire
            case 45863: // Cosmetic - Incinerate to Random Target
            case 25425: // Shoot
            case 45761: // Shoot
            case 42611: // Shoot
            case 61588: // Blazing Harpoon
            case 52479: // Gift of the Harvester
            case 48246: // Ball of Flame
            case 36327: // Shoot Arcane Explosion Arrow
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 36384: // Skartax Purple Beam
                spellInfo->MaxAffectedTargets = 2;
                break;
            case 41376: // Spite
            case 39992: // Needle Spine
            case 29576: // Multi-Shot
            case 40816: // Saber Lash
            case 37790: // Spread Shot
            case 46771: // Flame Sear
            case 45248: // Shadow Blades
            case 41303: // Soul Drain
            case 54172: // Divine Storm (heal)
            case 29213: // Curse of the Plaguebringer - Noth
            case 28542: // Life Drain - Sapphiron
            case 66588: // Flaming Spear
            case 54171: // Divine Storm
                spellInfo->MaxAffectedTargets = 3;
                break;
            case 38310: // Multi-Shot
            case 53385: // Divine Storm (Damage)
                spellInfo->MaxAffectedTargets = 4;
                break;
            case 42005: // Bloodboil
            case 38296: // Spitfire Totem
            case 37676: // Insidious Whisper
            case 46008: // Negative Energy
            case 45641: // Fire Bloom
            case 55665: // Life Drain - Sapphiron (H)
            case 28796: // Poison Bolt Volly - Faerlina
                spellInfo->MaxAffectedTargets = 5;
                break;
            case 40827: // Sinful Beam
            case 40859: // Sinister Beam
            case 40860: // Vile Beam
            case 40861: // Wicked Beam
            case 54835: // Curse of the Plaguebringer - Noth (H)
            case 54098: // Poison Bolt Volly - Faerlina (H)
                spellInfo->MaxAffectedTargets = 10;
                break;
            case 50312: // Unholy Frenzy
                spellInfo->MaxAffectedTargets = 15;
                break;
            case 33711: // Murmur's Touch
            case 38794:
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->Effects[EFFECT_0].TriggerSpell = 33760;
                break;
            case 17941: // Shadow Trance
            case 22008: // Netherwind Focus
            case 31834: // Light's Grace
            case 34754: // Clearcasting
            case 34936: // Backlash
            case 48108: // Hot Streak
            case 51124: // Killing Machine
            case 54741: // Firestarter
            case 57761: // Fireball!
            case 39805: // Lightning Overload
            case 64823: // Item - Druid T8 Balance 4P Bonus
            case 34477: // Misdirection
            case 44401: // Missile Barrage
            case 18820: // Insight 
                spellInfo->ProcCharges = 1;
                break;
            case 44544: // Fingers of Frost : Added 32 in last part to allow deep freeze damage to profit from increaed crit on (pseudo) frozen targets
                spellInfo->Effects[EFFECT_0].SpellClassMask = flag96(685904631, 1151048, 32);
                break;
            case 74396: // Fingers of Frost visual buff
                spellInfo->ProcCharges = 2;
                spellInfo->StackAmount = 0;
                break;
            case 54314: // Drain Power
            case 59354: // Drain Power
                spellInfo->StackAmount = 5;
                break;
            case 28200: // Ascendance (Talisman of Ascendance trinket)
                spellInfo->ProcCharges = 6;
                break;
            case 49224: // Magic Suppression - DK
            case 49610: // Magic Suppression - DK
            case 49611: // Magic Suppression - DK
            case 55166: // Tidal Force
                spellInfo->ProcCharges = 0;
                break;
            case 37408: // Oscillation Field
            case 48504: // Living Seed
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 47198: // Death's Embrace
            case 47199:
            case 47200:
                spellInfo->Effects[EFFECT_1].SpellClassMask[0] |= 0x00004000; // Drain Soul
                break;
            case 47201: // Everlasting Affliction
            case 47202:
            case 47203:
            case 47204:
            case 47205:
                // add corruption to affected spells
                spellInfo->Effects[EFFECT_1].SpellClassMask[0] |= 2;
                break;
            case 57470: // Renewed Hope (Rank 1)
            case 57472: // Renewed Hope (Rank 2)
                // should also affect Flash Heal
                spellInfo->AttributesEx |= SPELL_ATTR1_NO_THREAT;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                spellInfo->Effects[EFFECT_0].SpellClassMask[0] |= 0x800;
                break;
            case 51852: // The Eye of Acherus (no spawn in phase 2 in db)
                spellInfo->Effects[EFFECT_0].MiscValue |= 1;
                break;
            case 51912: // Crafty's Ultra-Advanced Proto-Typical Shortening Blaster Trigger
                spellInfo->Effects[EFFECT_0].Amplitude = 3000;
                break;
            case 29809: // Desecration Arm - 36 instead of 37 - typo? :/
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_7_YARDS);
                break;
            // Master Shapeshifter: missing stance data for forms other than bear - bear version has correct data
            // To prevent aura staying on target after talent unlearned
            case 48420:
                spellInfo->Stances = 1 << (FORM_CAT - 1);
                break;
            case 48421:
                spellInfo->Stances = 1 << (FORM_MOONKIN - 1);
                break;
            case 48422:
                spellInfo->Stances = 1 << (FORM_TREE - 1);
                break;
            case 48391: // Owlkin Frenzy
                spellInfo->Stances = 0;
                break;
            case 51466: // Elemental Oath (Rank 1)
            case 51470: // Elemental Oath (Rank 2)
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_ADD_FLAT_MODIFIER;
                spellInfo->Effects[EFFECT_1].MiscValue = SPELLMOD_EFFECT2;
                spellInfo->Effects[EFFECT_1].SpellClassMask = flag96(0x00000000, 0x00004000, 0x00000000);
                break;
            case 47569: // Improved Shadowform (Rank 1)
                // with this spell atrribute aura can be stacked several times
                spellInfo->Attributes &= ~SPELL_ATTR0_NOT_SHAPESHIFT;
                break;
            case 64904: // Hymn of Hope
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_MOD_INCREASE_ENERGY_PERCENT;
                break;
            case 19465: // Improved Stings (Rank 2)
                spellInfo->Effects[EFFECT_2].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
                break;
            case 49546: // Eagle Eyes
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_NEARBY_ENTRY;
                break;
            case 30421: // Nether Portal - Perseverence
                spellInfo->Effects[EFFECT_2].BasePoints += 30000;
                break;
            case 16834: // Natural shapeshifter
            case 16835:
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(21);
                break;
            case 51735: // Ebon Plague
            case 51734:
            case 51726:
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                spellInfo->SpellFamilyFlags[2] = 0x10;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN;
                break;
            case 50508: // Cryph Feaver
            case 50509: // Cryph Feaver
            case 50510: // Cryph Feaver
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 55968: // Bloodthirst
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ENEMY);
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo();
                break;
            case 41913: // Parasitic Shadowfiend Passive
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY; // proc debuff, and summon infinite fiends
                break;
            case 27892: // To Anchor 1
            case 27928: // To Anchor 1
            case 27935: // To Anchor 1
            case 27915: // Anchor to Skulls
            case 27931: // Anchor to Skulls
            case 27937: // Anchor to Skulls
            case 16177: // Ancestral Fortitude (Rank 1)
            case 16236: // Ancestral Fortitude (Rank 2)
            case 16237: // Ancestral Fortitude (Rank 3)
            case 47930: // Grace
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(13);
                break;
            // target allys instead of enemies, target A is src_caster, spells with effect like that have ally target
            // this is the only known exception, probably just wrong data
            case 29214: // Wrath of the Plaguebringer
            case 54836: // Wrath of the Plaguebringer
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo(TARGET_UNIT_SRC_AREA_ALLY);
                spellInfo->Effects[EFFECT_1].TargetB = SpellImplicitTargetInfo(TARGET_UNIT_SRC_AREA_ALLY);
                break;
            case 57994: // Wind Shear - improper data for EFFECT_1 in 3.3.5 DBC, but is correct in 4.x
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_MODIFY_THREAT_PERCENT;
                spellInfo->Effects[EFFECT_1].BasePoints = -6; // -5%
                spellInfo->AttributesEx |= SPELL_ATTR1_NO_THREAT;
                break;
            case 67188: // Item - Paladin T9 Retribution 2P Bonus (Righteous Vengeance)
            case 67118: // Item - Death Knight T9 Melee 4P Bonus
            case 67150: // Item - Hunter T9 2P Bonus
                spellInfo->Effects[EFFECT_1].BasePoints = 0;
                break;
            case 53652: // Paladin - Beacon of Light: Heal Effect
            case 53653:
            case 53654:
                spellInfo->AttributesEx6 &= ~SPELL_ATTR6_NO_HEALING_BONUS;
                break;
            case 19485: // Hunter - Mortal Shots
            case 19487: // Hunter - Mortal Shots
            case 19488: // Hunter - Mortal Shots
            case 19489: // Hunter - Mortal Shots
            case 19490: // Hunter - Mortal Shots
                spellInfo->Effects[EFFECT_1].BasePoints *= 0.50;
                break;
            case 70908: // Mage - Summon Water Elemental (Permanent)
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_SUMMON_PET;
                break;
            case 63675: // Improved Devouring Plague
            case 52752: // Shaman - Ancestral Awakening
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                break;
            case 12721: // Deep Wounds shouldnt ignore resillience or damage taken auras because its damage is not based off a spell.
                spellInfo->AttributesEx4 = 0;
                break;
            case 10909: // Mind Vision
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_50000_YARDS); 
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 47632: // Deathcoil Damage
            case 47633: // Deathcoil Heal
            case 50536: // Unholy Blight
            case 44461: // Living Bomb
            case 55361: // Living Bomb
            case 55362: // Living Bomb
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 33110: // Prayer of Mending triggers Inspiration
                spellInfo->AttributesEx2 |= SPELL_ATTR2_TRIGGERED_CAN_TRIGGER_PROC;
                // causes handling als delayed spells, so OnHit effect gets applyed after dealing the damage/effect
                spellInfo->AttributesEx4 |= SPELL_ATTR4_UNK4;
                break;
            case 6474: // Earthbind Totem (instant pulse)
            case 8188:  // Magma Totem - Rank 1 (instant pulse)
            case 10582: // Magma Totem - Rank 2 (instant pulse)
            case 10583: // Magma Totem - Rank 3 (instant pulse)
            case 10584: // Magma Totem - Rank 4 (instant pulse)
            case 25551: // Magma Totem - Rank 5 (instant pulse)
            case 58733: // Magma Totem - Rank 6 (instant pulse)
            case 58736: // Magma Totem - Rank 7 (instant pulse)
                spellInfo->AttributesEx5 |= SPELL_ATTR5_START_PERIODIC_AT_APPLY;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                spellInfo->AttributesEx |= SPELL_ATTR1_NO_THREAT;
                break;
            case 8145: // Tremor Totem (instant pulse)
                spellInfo->AttributesEx5 |= SPELL_ATTR5_START_PERIODIC_AT_APPLY;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                spellInfo->AttributesEx |= SPELL_ATTR1_NO_THREAT;
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 34650: // shadowfiend - Mana Leech ignore LOS
            case 8146:  // Tremor Totem effect ignore LOS
            case 39610: // Mana tide Totem ignore LOS
            case 39609: // Mana tide Totem ignore LOS
            case 46747: // Midsummer Fling torch  LOS
            case 52042: // Healing stream Totem - ignore LOS
            case 52041: // Healing stream Totem - ignore LOS (Rank 1)
            case 52046: // Healing stream Totem - ignore LOS (Rank 2)
            case 52047: // Healing stream Totem - ignore LOS (Rank 3)
            case 52048: // Healing stream Totem - ignore LOS (Rank 4)
            case 52049: // Healing stream Totem - ignore LOS (Rank 5)
            case 52050: // Healing stream Totem - ignore LOS (Rank 6)
            case 58759: // Healing stream Totem - ignore LOS (Rank 7)
            case 58760: // Healing stream Totem - ignore LOS (Rank 8)
            case 58761: // Healing stream Totem - ignore LOS (Rank 9)
            case 65993: // Healing stream Totem - ignore LOS (Rank 10)
            case 68883: // Healing stream Totem - ignore LOS (Rank 11)
            case 48543: // Revitalize
            case 48540: // Revitalize
            case 48541: // Revitalize
            case 48542: // Revitalize
            case 63853: // Rapture
            case 57669: // Replenishment
            case 27681: // Prayer of Spirit
            case 32999:
            case 48074:
            case 21562: // Prayer of Fortitude
            case 21564:
            case 25392:
            case 48162:
            case 27683: // Prayer of Shadow Protection
            case 39236:
            case 39374:
            case 48170:
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 530: // Charm (Possess)
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                spellInfo->ChannelInterruptFlags = 0;
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_50000_YARDS);
                // spellInfo->Attributes |= SPELL_ATTR0_HIDDEN_CLIENTSIDE;
                break;
            case 16191: // Mana tide Totem ignore LOS
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                spellInfo->Effects[EFFECT_0].Amplitude = 2700;
                break;
            case 46054: // Midsummer Torch
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_2_YARDS);
                break;
            case 51560: // Shaman - Improved Earth Shield R1
            case 51561: // Shaman - Improved Earth Shield R2
                //! HACK: special heal bonus calculation so we can't use normal effect -> so we do all the calculation in a spellscript
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 8178: // Shaman - Grounding Totem ignore LOS
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 379: // Shaman - Earthshield heal trigger
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                // causes handling als delayed spells, so OnHit effect gets applyed after dealing the damage/effect
                spellInfo->AttributesEx4 |= SPELL_ATTR4_UNK4;
                break;
            case 57840: // Killing Spree
            case 57841: // Killing Spree
            case 61851: // Killing Spree
            case 51696: // Killing Spree
            case 51690: // Killing Spree
                spellInfo->AttributesEx2 |= SPELL_ATTR2_NOT_RESET_AUTO_ACTIONS;
                break;
            case 50516: // Typhoon trigger r1
            case 53223: // Typhoon trigger r2
            case 53225: // Typhoon trigger r3
            case 53226: // Typhoon trigger r4
            case 61384: // Typhoon trigger r5
            case 1543:  // Hunter - Flare
            case 42671: // Hunter - Silent Shot
            case 34490: // Hunter - Silent Shot 
            case 71289: // Dominate Mind
                spellInfo->Speed = 0.0f; // This spell is instant
                break;
            case 42243: // Hunter - Volley damage r1
            case 42244: // Hunter - Volley damage r2
            case 42245: // Hunter - Volley damage r3
            case 42234: // Hunter - Volley damage r4
            case 58432: // Hunter - Volley damage r5
            case 58433: // Hunter - Volley damage r6
                spellInfo->Attributes &= ~SPELL_ATTR0_REQ_AMMO;
                break;
            case 61391: // Typhoon r1
            case 61390: // Typhoon r2 
            case 61388: // Typhoon r3
            case 61387: // Typhoon r4
            case 53227: // Typhoon r5
                spellInfo->Speed = 40.0f;
                break;
            case 58873: // Dalaran arena - Water Spout
                spellInfo->Effects[0].MiscValue = 300;
                spellInfo->Effects[EFFECT_0].BasePoints = 40;
                break;
            case 51421: // Fire Cannon - Wintergrasp Cannon
            case 67452: // Rocket Blast
            case 66541: // Incendiary Rocket
            case 68170: // Rocket Blast
            case 68169: // Incendiary Rocket
            case 49872: // Rocket Blast
            case 50896: // Hurl Boulder
            case 57606: // Plague Barrel
            case 57609: // Fire Cannon
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(174); // 1000 Yards
                break;
            case 38065: // Death Coil
            case 49097: // Quest - Out of Body Experience
            case 34709: // Shadow Sight -> Arena buff
            case 6277:  // Bind Sight (PT)
            case 26102: // AQ40/Ouro - Sandblast
            case 65607: // Synthebrew Goggles
            case 62972: // On Guard
                spellInfo->Attributes |= SPELL_ATTR0_CANT_CANCEL;
                break;
            case 52109: // Flametongue Totem rank 1 (Aura)
            case 52110: // Flametongue Totem rank 2 (Aura)
            case 52111: // Flametongue Totem rank 3 (Aura)
            case 52112: // Flametongue Totem rank 4 (Aura)
            case 52113: // Flametongue Totem rank 5 (Aura)
            case 58651: // Flametongue Totem rank 6 (Aura)
            case 58654: // Flametongue Totem rank 7 (Aura)
            case 58655: // Flametongue Totem rank 8 (Aura)
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
                spellInfo->Effects[EFFECT_1].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo();
                spellInfo->Effects[EFFECT_1].TargetB = SpellImplicitTargetInfo();
                break;
            case 53241: // Marked for Death (Rank 1)
            case 53243: // Marked for Death (Rank 2)
            case 53244: // Marked for Death (Rank 3)
            case 53245: // Marked for Death (Rank 4)
            case 53246: // Marked for Death (Rank 5)
                spellInfo->Effects[EFFECT_0].SpellClassMask = flag96(0x00067801, 0x10820001, 0x00000801);
                break;
            case 70728: // Exploit Weakness (needs target selection script)
            case 70840: // Devious Minds (needs target selection script)
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo(TARGET_UNIT_PET);
                break;
                // spelltarget selection script fails, problem in PetAI.cpp.
                // if spell is autocasted targets are selected from dbc values not from script
            case 53434: // Call of the Wild
            case 70893: // Culling The Herd
            case 24604: // Furious Howl - Rank 1
            case 64491: // Furious Howl - Rank 2
            case 64492: // Furious Howl - Rank 3
            case 64493: // Furious Howl - Rank 4
            case 64494: // Furious Howl - Rank 5
            case 64495: // Furious Howl - Rank 6
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo(TARGET_UNIT_MASTER);
                spellInfo->Effects[EFFECT_1].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
                spellInfo->Effects[EFFECT_1].TargetB = SpellImplicitTargetInfo(TARGET_UNIT_MASTER);
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 54800: // Sigil of the Frozen Conscience - change class mask to custom extended flags of Icy Touch
                        // this is done because another spell also uses the same SpellFamilyFlags as Icy Touch
                        // SpellFamilyFlags[0] & 0x00000040 in SPELLFAMILY_DEATHKNIGHT is currently unused (3.3.5a)
                        // this needs research on modifier applying rules, does not seem to be in Attributes fields
                spellInfo->Effects[EFFECT_0].SpellClassMask = flag96(0x00000040, 0x00000000, 0x00000000);
                break;
            case 64949: // Idol of the Flourishing Life
                spellInfo->Effects[EFFECT_0].SpellClassMask = flag96(0x00000000, 0x02000000, 0x00000000);
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_ADD_FLAT_MODIFIER;
                break;
            case 34231: // Libram of the Lightbringer
            case 60792: // Libram of Tolerance
            case 64956: // Libram of the Resolute
                spellInfo->Effects[EFFECT_0].SpellClassMask = flag96(0x80000000, 0x00000000, 0x00000000);
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_ADD_FLAT_MODIFIER;
                break;
            case 28851: // Libram of Light
            case 28853: // Libram of Divinity
            case 32403: // Blessed Book of Nagrand
                spellInfo->Effects[EFFECT_0].SpellClassMask = flag96(0x40000000, 0x00000000, 0x00000000);
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_ADD_FLAT_MODIFIER;
                break;
            case 45602: // Ride Carpet
                spellInfo->Effects[EFFECT_0].BasePoints = 0; // force seat 0, vehicle doesn't have the required seat flags for "no seat specified (-1)"
                break;
            case 45627: // Temple A Credit
            case 45628: // Temple B Credit
            case 45629: // Temple C Credit
            case 3355:  // Freezing Trap Effect
            case 14308: // Freezing Trap Effect
            case 14309: // Freezing Trap Effect
            case 31932: // Freezing Trap Effect
            case 43415: // Freezing Trap Effect
            case 43448: // Freezing Trap Effect
            case 55041: // Freezing Trap Effect
            case 60210: // Freezing Arrow Effect
            case 45145: // Snape Trap Effect
            case 43485: // Snape Trap Effect
            case 63487: // Frost Trap Effect
            case 41086: // Frost Trap Effect
            case 71647: // Frost Trap Effect
            case 13797: // Immolation Trap Effect
            case 14298: // Immolation Trap Effect
            case 14299: // Immolation Trap Effect
            case 14300: // Immolation Trap Effect
            case 14301: // Immolation Trap Effect
            case 27024: // Immolation Trap Effect
            case 49053: // Immolation Trap Effect
            case 49054: // Immolation Trap Effect
            case 51740: // Immolation Trap Effect
                spellInfo->AttributesEx6 |= SPELL_ATTR6_CAN_TARGET_INVISIBLE;
                break;
            case 64745: // Item - Death Knight T8 Tank 4P Bonus
            case 64936: // Item - Warrior T8 Protection 4P Bonus
                spellInfo->Effects[EFFECT_0].BasePoints = 100; // 100% chance of procc'ing, not -10% (chance calculated in PrepareTriggersExecutedOnHit)
                break;
            case 59653: // Warrior - Damage Shield
                // allow damage shield to procc triggers (like deep wounds etc)
                spellInfo->AttributesEx3 &= ~SPELL_ATTR3_CANT_TRIGGER_PROC;
                // fallthrough intended
            case 64442: // Enchant - Blade Warding
                // enable critting (damage class none can't crit)
                spellInfo->DmgClass = SPELL_DAMAGE_CLASS_MELEE;
                break;
            case 12834: // Warrior - Deep Wounds (Rank 1 - 3)
            case 12849:
            case 12867: // Enable proccing on ranged attacks
                spellInfo->ProcFlags |= (PROC_FLAG_DONE_RANGED_AUTO_ATTACK | PROC_FLAG_DONE_SPELL_RANGED_DMG_CLASS);
                spellInfo->EquippedItemSubClassMask |= ITEM_SUBCLASS_MASK_WEAPON_RANGED;
                break;
            case 19970: // Entangling Roots (Rank 6) -- Nature's Grasp Proc
            case 19971: // Entangling Roots (Rank 5) -- Nature's Grasp Proc
            case 19972: // Entangling Roots (Rank 4) -- Nature's Grasp Proc
            case 19973: // Entangling Roots (Rank 3) -- Nature's Grasp Proc
            case 19974: // Entangling Roots (Rank 2) -- Nature's Grasp Proc
            case 19975: // Entangling Roots (Rank 1) -- Nature's Grasp Proc
            case 27010: // Entangling Roots (Rank 7) -- Nature's Grasp Proc
            case 53313: // Entangling Roots (Rank 8) -- Nature's Grasp Proc
                spellInfo->CastTimeEntry = sSpellCastTimesStore.LookupEntry(1);
                break;
            case 18754: // Improved succubus - problems with apply if target is pet
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_ADD_FLAT_MODIFIER; // it's affects duration of seduction, let's minimize affection
                spellInfo->Effects[0].BasePoints = -1.5*IN_MILLISECONDS*0.22; // reduce cast time of seduction by 22%
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 18755:
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_ADD_FLAT_MODIFIER;
                spellInfo->Effects[0].BasePoints = -1.5*IN_MILLISECONDS*0.44; // reduce cast time of seduction by 44%
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 18756:
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_ADD_FLAT_MODIFIER;
                spellInfo->Effects[0].BasePoints = -1.5*IN_MILLISECONDS*0.66; // reduce cast time of seduction by 66%
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 53517: // Roar of Recovery
            case 136:   // Mend Pet (Rank 1)
            case 3111:  // Mend Pet (Rank 2)
            case 3661:  // Mend Pet (Rank 3)
            case 3662:  // Mend Pet (Rank 4)
            case 13542: // Mend Pet (Rank 5)
            case 13543: // Mend Pet (Rank 6)
            case 13544: // Mend Pet (Rank 7)
            case 27046: // Mend Pet (Rank 8)
            case 48989: // Mend Pet (Rank 9)
            case 48990: // Mend Pet (Rank 10)
            case 19574: // Bestial Wrath
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 16689: // Nature's Grasp (Rank 1)
            case 16810: // Nature's Grasp (Rank 2)
            case 16811: // Nature's Grasp (Rank 3)
            case 16812: // Nature's Grasp (Rank 4)
            case 16813: // Nature's Grasp (Rank 5)
            case 17329: // Nature's Grasp (Rank 6)
            case 27009: // Nature's Grasp (Rank 7)
            case 53312: // Nature's Grasp (Rank 8)
            case 35944: // Power Word: Shield (creature ability)
            case 17139: // Power Word: Shield (creature ability)
                spellInfo->StartRecoveryTime = 0; // disable GCD
                break;
            case 10713: // Albino Snake
                spellInfo->StartRecoveryCategory = 133;
                spellInfo->StartRecoveryTime = 1500; // 1.5 seconds CD
                break;
            case 40098: // Summon Raven God
                spellInfo->CategoryRecoveryTime = 72000000; // 20 hours CD
                break;
            case 19872: // BWL - Razorgore: Calm Dragonkin
                spellInfo->RecoveryTime = 0;
                spellInfo->CategoryRecoveryTime = 5 * IN_MILLISECONDS;
                spellInfo->StartRecoveryCategory = 133;
                spellInfo->StartRecoveryTime = 5 * IN_MILLISECONDS;
                break;
            case 23364: // BWL - Nefarian: Tail Smash
                spellInfo->Effects[EFFECT_2].TargetA = SpellImplicitTargetInfo(TARGET_DEST_DEST_BACK);
                spellInfo->Effects[EFFECT_2].TargetB = SpellImplicitTargetInfo(TARGET_UNIT_DEST_AREA_ENEMY);
                spellInfo->Effects[EFFECT_2].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_20_YARDS);
            case 15847: // BWL - Vaelastraz: Tail Smash
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_DEST_DEST_BACK);
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo(TARGET_UNIT_DEST_AREA_ENEMY);
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_20_YARDS);
                spellInfo->Effects[EFFECT_1].TargetA = SpellImplicitTargetInfo(TARGET_DEST_DEST_BACK);
                spellInfo->Effects[EFFECT_1].TargetB = SpellImplicitTargetInfo(TARGET_UNIT_DEST_AREA_ENEMY);
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_20_YARDS);
                break;
            case 23603: // BWL - Nefarian: Mage ClassCall / Wild Polymorph
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ANY);
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_1].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ANY);
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                spellInfo->Effects[EFFECT_2].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ANY);
                spellInfo->Effects[EFFECT_2].TargetB = 0;
                break;
            case 23419: // BWL - Nefarian: Shaman ClassCall / Corrupted Totem
                spellInfo->Effects[EFFECT_0].MiscValueB = 83;
                break;
            case 23420: // BWL - Nefarian: Shaman ClassCall / Corrupted Totem
            case 23422:
            case 23423:
                spellInfo->Effects[EFFECT_1].MiscValueB = 83;
                break;
            case 22247: // Blackwing Lair Suppression device
            case 2825:  // Bloodlust
            case 32182: // Heroism
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 2641: // Hunter - Dismiss pet
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 49575: // deathknight - deathgrip
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                spellInfo->Effects[EFFECT_0].MiscValue = 20;
                break;
            case 47482: // deathknight/ghul - leap
                spellInfo->Effects[1].MiscValue = 40;
                break;
            case 45529: // blood tap - swap effect order for correct conversion w/o hackfixes :)
                std::swap(spellInfo->Effects[0], spellInfo->Effects[1]);
                break;
            case 55959: // Embrace of the Vampyr
            case 59513: // Embrace of the Vampyr
            case 59414: // Pulsing Shockwave Aura (Loken)
            case 50245: // Hunter - Pin
            case 53544: // Hunter - Pin
            case 53545: // Hunter - Pin
            case 53546: // Hunter - Pin
            case 53547: // Hunter - Pin
            case 53548: // Hunter - Pin
                // this flag breaks movement, remove it
                spellInfo->AttributesEx &= ~SPELL_ATTR1_CHANNELED_1;
                break;
            case 70809: // Shaman T 10 Bonus
            case 70691: // Druid T 10 Bonus
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                spellInfo->AttributesEx6 &= ~SPELL_ATTR6_NO_HEALING_BONUS;
                break;
            case 55430: // Gymer's Buddy
                spellInfo->Attributes &= ~SPELL_ATTR0_CANT_CANCEL;
                break;
            case 70890: // DK - Scourge Strike
                spellInfo->AttributesEx6 &= ~SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS;
                break;
            case 42723: // Dark Smash (Ingvar the Plunderer)
            case 59709: // Dark Smash (Ingvar the Plunderer)
                spellInfo->AttributesEx2 &= ~SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 45406: // Holiday - Midsummer, Ribbon Pole Periodic Visual - Remove on Move
                spellInfo->AuraInterruptFlags |= AURA_INTERRUPT_FLAG_MOVE;
                break;
            case 61719: // Easter Lay Noblegarden Egg Aura - Interrupt flags copied from aura which this aura is linked with
                spellInfo->AuraInterruptFlags = AURA_INTERRUPT_FLAG_HITBYSPELL | AURA_INTERRUPT_FLAG_TAKE_DAMAGE;
                break;
            case 3714: // Hunters mark
            case 60068: // Hunters mark
                spellInfo->AuraInterruptFlags = AURA_INTERRUPT_FLAG_TAKE_DAMAGE;
                break;
            case 47424: // Korkron Wing Commander
                spellInfo->AuraInterruptFlags = 0;
                break;
            case 62225: // Risen Ally - effect doesn't work correctly
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 36030: // Protecting Our Own
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 56191: // Dark Jade Beam
            case 55346: // Dark Jade Beam
            case 56190: // Dark Jade Beam
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ANY);
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo();
                spellInfo->Attributes |= SPELL_ATTR0_HIDDEN_CLIENTSIDE | SPELL_ATTR0_HIDE_IN_COMBAT_LOG;
                spellInfo->Speed = 0.0f;
                break;
            case 29832: // Attumen Shadow Cleave
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_SCHOOL_DAMAGE;
                spellInfo->Effects[EFFECT_0].BasePoints = 5000;
                break;
            case 58428: // Overkill
            case 31666: // Master of Subtlety triggers
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_SCRIPT_EFFECT;
                break;
            case 26084: // Whirlwind - Battleguard Sartura
            case 44533: // Wretched Stab - Wretched Skulker
            case 44534: // Wretched Strike - Wretched Bruiser
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_SCHOOL_DAMAGE;
                spellInfo->Effects[EFFECT_0].BasePoints = 999;
                break; 
            case 50526: // DK - Wandering Plague
                spellInfo->AttributesEx6 |= SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                break;
            case 15290: // Vampiric embrace
            case 33619: // Reflective Shield
            case 14156: // Ruthlessness
            case 14160:
            case 14161:
            case 1725:  // Distract
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                break;
            case 32645: // Envenom
            case 32684:
            case 57992:
            case 57993:
                spellInfo->Dispel = DISPEL_NONE;
                break;
            case 53480: // Roar of Sacrifice Split damage
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_SPLIT_DAMAGE_PCT;
                spellInfo->Effects[EFFECT_1].MiscValue = SPELL_SCHOOL_MASK_ALL;
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                spellInfo->Effects[EFFECT_1].BasePoints = 20;
                break;
            case 30174: // Riding Turtle
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_1].BasePoints = 60;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_MOD_INCREASE_SWIM_SPEED;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_CASTER;
                break;
            case 28478: // Kel'thuzad's Frostbolt 10N (Naxxramas)
            case 55802: // Kel'thuzad's Frostbolt 25N
            case 12051: // Evocation
            case 42650: // Army of the Dead
                // there has to be another *spell can be interrupted* flag =/
                spellInfo->InterruptFlags |= SPELL_INTERRUPT_FLAG_INTERRUPT;
                break;
            case 31258: // Death & Decay - Hyjal
                spellInfo->InterruptFlags = 0;
                spellInfo->ChannelInterruptFlags = 0;
                break;
            case 29317: // Naxxramas Gothik - Shadow Bolt (10man)
            case 56405: // Naxxramas Gothik - Shadow Bolt (25man)
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 42132: // Headless Horseman - Start Fire
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(13);
                break;
            case 62012: // Turkey Caller
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(36);
                break;
            case 38055: // Destroy Deathforged Infernal
            case 32009: // Cutdown
                spellInfo->RecoveryTime = 0;
                spellInfo->CategoryRecoveryTime = 8 * IN_MILLISECONDS;
                spellInfo->StartRecoveryCategory = 133;
                spellInfo->StartRecoveryTime = 1.5 * IN_MILLISECONDS;
                break;
            case 37920: // Turbo Boost
                spellInfo->RecoveryTime = 0;
                spellInfo->CategoryRecoveryTime = 30 * IN_MILLISECONDS;
                spellInfo->StartRecoveryCategory = 133;
                spellInfo->StartRecoveryTime = 30 * IN_MILLISECONDS;
                break;
            case 38054: // Random Rocket Missile
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_20_YARDS);
                break;
            case 64869: //  Item - Mage T8 4P Bonus
                spellInfo->Effects[EFFECT_0].BasePoints = 20;
                break;
            case 53290: // Hunter - Hunting Party (rank1) 
            case 53291: // Hunter - Hunting Party (rank2) 
            case 53292: // Hunter - Hunting Party (rank3) 
                spellInfo->ProcFlags = PROC_FLAG_DONE_RANGED_AUTO_ATTACK|PROC_FLAG_DONE_SPELL_RANGED_DMG_CLASS|
                    PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG|PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG;
                break;
            case 52910: // Turn the Tables (Rank 3)
            case 52914: // Turn the Tables (Rank 1)
            case 52915: // Turn the Tables (Rank 2)
            case 52916: // Honor Among Thieves
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 66:    // Mage Invisibility
            case 35009: // Trigger Spell from Invisibility
            case 32578: // Gor'drek's Ointment
            case 69563: // Cologne Spritz
            case 69445: // Perfume Spritz
            case 69438: // Sample Satisfaction
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                break;
            case 71838: // Drain Life - Bryntroll Normal
            case 71839: // Drain Life - Bryntroll Heroic
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
                break;
            case 38373: // The Beast Within
            case 34471: // The Beast Within
                spellInfo->Attributes |= SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY;
                spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_CONFUSED | SPELL_ATTR5_USABLE_WHILE_FEARED | SPELL_ATTR5_USABLE_WHILE_STUNNED;
                spellInfo->Effects[EFFECT_2].BasePoints = 1;
                spellInfo->Effects[EFFECT_2].MiscValue = 11;
                break;
            case 56606: // Ride Jokkum
            case 61791: // Ride Vehicle (Yogg-Saron)
                /// @todo: remove this when basepoints of all Ride Vehicle auras are calculated correctly
                spellInfo->Effects[EFFECT_0].BasePoints = 1;
                break;
            case 61556: // Nexus - Crystalline Tangler - Tangle
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(174);
                break;
            case 69510: // Charm Collector
                spellInfo->AttributesEx3 |= SPELL_ATTR3_DEATH_PERSISTENT;
                break;
            case 59630: // Black Magic
                spellInfo->Attributes |= SPELL_ATTR0_PASSIVE;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_DEATH_PERSISTENT;
                break;
            case 17619: // Alchemist's Stone
            case 72968: // Precious's Ribbon
            case 46273: // Multiphase Goggles
                spellInfo->AttributesEx3 |= SPELL_ATTR3_DEATH_PERSISTENT;
                break;
            case 31984: // hyjal/archimonde: finger of death
                spellInfo->Attributes &= ~ SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY;
                break;
            case 49152: // Titan Grip
            case 7376:  // Defensive Stance Passive
            case 57339: // Tank Class Passive Threat
            case 38770: // Mortal Wound (used for Arena Heal Reduce)
                spellInfo->Attributes |= SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY;
                break;
            case 62626: //Shield Breaker
                 spellInfo->FacingCasterFlags |= SPELL_FACING_FLAG_INFRONT;
                 break;
            case 59565: // Summon Reanimated Abomination
                spellInfo->Effects[0].MiscValueB = 67;
                break;
            case 1122: // Summon Infernal
                spellInfo->Effects[0].MiscValueB = 67;
                break;
            case 7979: // Compact Harvest Reaper
                spellInfo->Effects[0].MiscValueB = 61;
                break;
            case 52811: // Bloated Abomination - Burst at the Seams
                spellInfo->Attributes &= ~SPELL_ATTR0_CASTABLE_WHILE_DEAD; // spells with this attribute are not added to action bar in CharmInfo::InitPossessCreateSpells()
                break;
            case 100004: //create item 36765 for quest 'Slim Pickings' in spell_dbc
                spellInfo->Effects[EFFECT_0].ItemType = 36765;
                break;
            case 49028: //Dancing Rune Weapon
                spellInfo->Effects[EFFECT_1].TriggerSpell = 0;
                break;
            case 64056: // Eating
                spellInfo->Effects[EFFECT_1].TriggerSpell = 1137;
                break;
            case 38736: //Rod of Purification
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_TRIGGER_SPELL;
                spellInfo->AttributesEx &= ~ SPELL_ATTR1_CHANNELED_1;
                spellInfo->AttributesEx &= ~ SPELL_ATTR1_CHANNEL_DISPLAY_SPELL_NAME;
                spellInfo->CastTimeEntry = sSpellCastTimesStore.LookupEntry(7);
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(585);
                spellInfo->Attributes |= SPELL_ATTR0_NOT_SHAPESHIFT;
                break;
            case 44017: // Northsea Force Reaction
                spellInfo->AttributesEx |= SPELL_ATTR1_DONT_DISPLAY_IN_AURA_BAR;
                spellInfo->Attributes |= SPELL_ATTR0_PASSIVE;
                break;
            case 61375: //Water Bolt
                spellInfo->Effects[EFFECT_0].BasePoints = 2300;
                spellInfo->Effects[EFFECT_0].DieSides = 300;
                break;
            case 47189: // Spiritual Insight
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(572);
                break;
            case 61374: //Lightning Bolt
                spellInfo->Effects[EFFECT_0].BasePoints = 900;
                spellInfo->Effects[EFFECT_0].DieSides = 100;
                break;
            case 46221: // Animal Blood - Hateful towards Faction
                spellInfo->Effects[EFFECT_1].BasePoints = 0;
                break;
            case 74410: // Arena - Dampening
            case 74411: // Battleground - Dampening
                spellInfo->Effects[EFFECT_0].BasePoints = -5;
                // damage debuff
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].BasePoints = -5;
                spellInfo->Effects[EFFECT_1].MiscValue = 127;
                break;
            case 68786: // Permafrost - NH
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break; 
            case 70336: // Permafrost - HC
                spellInfo->Effects[EFFECT_0].BasePoints = 520;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 29917: // Feed Captured Animal 
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 56746: // Punsh - nergeld
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(13);
                break;
            case 4307: // Safirdrang's Chill
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(18);
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(18);
                spellInfo->Effects[EFFECT_2].RadiusEntry = sSpellRadiusStore.LookupEntry(18);
                break;
            case 45853: // Survey Sinkholes
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(174); // 1000 Yards
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(174);
                break;
            case 64681: // Loaned Gryphon
            case 64761: // Loaned WWind Rider
                spellInfo->AreaGroupId = 883;
                break;
            case 51209: // DK - Hungering Cold
                spellInfo->AuraInterruptFlags |= AURA_INTERRUPT_FLAG_DIRECT_DAMAGE;
                spellInfo->SpellFamilyFlags |= flag96(0x00, 67108864, 0x00);
                break;
            case 47481:
                spellInfo->StartRecoveryTime = 0;
                break;
            case 52338: // bg/sa - hurl boulder
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(174); // 1000 Yards
                break;
            case 52418: // bg/sa - Carrying Seaforium
                spellInfo->Attributes |= SPELL_ATTR0_NEGATIVE_1 | SPELL_ATTR0_CANT_CANCEL;
                break;
            case 60549: // Deadly Gladiator's Libram of Fortitude second Effect to Range
                 spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_MOD_RANGED_ATTACK_POWER;
                 break;
            case 60551: // Furious Gladiator's Libram of Fortitude second Effect to Range
                 spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_MOD_RANGED_ATTACK_POWER;
                 break;
            case 60553: 
                // Relentless Gladiator's Libram of Fortitude second Effect to Range
                // Relentless Gladiator's Idol of Resolve second Effect to Range
                 spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_MOD_RANGED_ATTACK_POWER;
                 break;
            case 61834: // Dalaran/Minievent - Manabonked!
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 49163: // Perpetual Instability
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN;
                spellInfo->Effects[EFFECT_0].MiscValue |= SPELL_SCHOOL_MASK_ALL;
                break;
            case 63504: // Improved Flash Heal
            case 63505: // Improved Flash Heal
            case 63506: // Improved Flash Heal
                spellInfo->Effects[EFFECT_0].SpellClassMask = spellInfo->Effects[EFFECT_1].SpellClassMask;
                break;
            case 52408: // BG SA - Seaforium Blast
                // 2750 is 25% of the max health from a gate
                spellInfo->Effects[EFFECT_1].BasePoints = 2750;
                break;
            case 56580: // Ahn'Kahet - Deep Crawler - Glutinous Poisen - normal 
            case 59108: // Ahn'Kahet - Deep Crawler - Glutinous Poisen - heroic
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 67745: // ToC5 - Black Knight - Death Respite (5man)
            case 68306: // ToC5 - Black Knight - Death Respite (5man heroic)
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 66904: // ToC5 - Eadric - Hammer of Righteous (player aura)
                // remove create player vehicle aura, this causes crashes when other player try to enter the vehicle...
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 66863: // ToC5 - Eadric - Hammer of Justice
            case 66940:
            case 66941:
            case 32409: // SW: Death (Priest)
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 63652: // Priest - Rapture - energy trigger spell
            case 63653: // Priest - Rapture - energy trigger spell
            case 63654: // Priest - Rapture - energy trigger spell
            case 63655: // Priest - Rapture - energy trigger spell
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                break;
            case 51673: // BG SA - antiperson cannon
            case 52339: // BG SA - demolisher - bolder
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
                break;
            case 24869: // Well Fed - Winterbufffood
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_1].BasePoints = 0;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_PERIODIC_TRIGGER_SPELL;
                spellInfo->Effects[EFFECT_1].Amplitude = 10 * IN_MILLISECONDS;
                spellInfo->Effects[EFFECT_1].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
                spellInfo->Effects[EFFECT_1].TargetB = SpellImplicitTargetInfo();
                break;
            case 61874: // Food - Noble Garden Bufffood
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_1].BasePoints = 0;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_PERIODIC_TRIGGER_SPELL;
                spellInfo->Effects[EFFECT_1].Amplitude = 10 * IN_MILLISECONDS;
                spellInfo->Effects[EFFECT_1].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
                spellInfo->Effects[EFFECT_1].TargetB = SpellImplicitTargetInfo();
                break;
            case 31961:
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                spellInfo->AttributesEx4 |= SPELL_ATTR4_IGNORE_RESISTANCES;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 47476: // Deathknight - Strangulate
            case 15487: // Priest - Silence
            case 5211:  // Druid - Bash  - R1
            case 6798:  // Druid - Bash  - R2
            case 8983:  // Druid - Bash  - R3
                spellInfo->AttributesEx7 |= SPELL_ATTR7_INTERRUPT_ONLY_NONPLAYER;
                break;
            case 47669: // Awaken Subboss
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ANY);
                break;
            case 48136: // Acid Splatter
            case 59272: // Acid Splatter
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(-1); // unlimited, was 2 sec
                break;
            case 53768: // Item - Haunted Memento
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(9); // 30 seconds
                break;
            case 52212: // DK: Death 'n Decay damage spell
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                spellInfo->AttributesEx5 |= SPELL_ATTR5_REDUCED_BY_AOE_DAMAGE_AVOIDANCE;
                spellInfo->AttributesEx6 |= SPELL_ATTR6_CAN_TARGET_INVISIBLE;
                break;
            case 59782: // Summoning Stone Effect
            case 61994: // Summoning Ritual Effect
                spellInfo->CastTimeEntry = sSpellCastTimesStore.LookupEntry(1);
                break;
            case 15787:
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(165); // 7s - Otherwise Periodic Spell will not trigger
                break;
// ########## ISLE OF CONQUEST SPELLS ##########
            case 66550: // Isle of conquest - teleport
            case 66551: // Isle of conquest - teleport
                spellInfo->Effects[EFFECT_0].Effect = 0;
                break;
            case 66186: // Isle of conquest - Napalm
            case 68832: // Isle of conquest - Napalm
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(152); // 150 Yards
                break;
            case 66655: // Isle of conquest - Airship Cannon Effect
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_20_YARDS);
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_45_YARDS);
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(174); // 1000 Yards
                break;
            case 68825: // Isle of conquest - Airship Cannon
            case 66518:
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(174); // 1000 Yards
                break;
            case 67462: // Steam Blast
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 66218: // catapult - launch
                spellInfo->Effects[1].Effect = 0;
                break;
            case 40604: // Fel Rage - Bt
            case 66656: // parachute
            case 45232: // Fireblast
            case 24425: // Spirit of Zandalar
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
            case 47340: // Dark Brewmaiden's Stun
            case 47310: // Direbrew's Disarm
                spellInfo->Attributes |= SPELL_ATTR0_CANT_CANCEL;
                break;

// ########## ENDOF ISLE OF CONQUEST SPELLS ##########


// ########## NAXXRAMAS SPELLS ##########
            case 54097: // Widow's Embrace (Naxx, Grandwidow Faerlina, Worshipper)
                spellInfo->Attributes &= ~SPELL_ATTR0_CASTABLE_WHILE_DEAD;
                // spells with this attribute are not added to action bar in CharmInfo::InitPossessCreateSpells()
                break;
            case 28522: // Icebold
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
// ########## ENDOF NAXXRAMAS SPELLS ##########


// ########## OBSIDIAN SANCTUM SPELLS ##########
            case 57753: // Conjure Flame Orb
                spellInfo->Effects[0].Effect = SPELL_EFFECT_SUMMON;
                spellInfo->Effects[0].MiscValue = 30702;
                spellInfo->Effects[0].MiscValueB = 64;
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_DEST_CASTER);
                break;
            case 57750: // Flame Orb Periodic
            case 58937: // Flame Orb Periodic
                spellInfo->Effects[EFFECT_0].Amplitude = 1000;
                break;
            case 57740: // Devotion Aura
            case 58944: // Devotion Aura
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_APPLY_AREA_AURA_FRIEND;
                break;
            case 57935: // Twilight Torment
                spellInfo->ProcCharges = 0;
                break;
            case 61187: // Leave Twilight portal
            case 61190:
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
            case 58766: // Gift of Twilight (Satharion)
                spellInfo->AttributesEx &= ~SPELL_ATTR1_CHANNELED_1;
                break;
            case 57591: // Lava-Strike (Satharion)
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
// ########## ENDOF OBSIDIAN SANCTUM SPELLS ##########


// ########## MALYGOS SPELLS ##########
            case 60474: // Exit Portal
                spellInfo->Attributes |= SPELL_ATTR0_INDOORS_ONLY;
                break;
            case 56272: // Malygos Deep breath
            case 60072:
                spellInfo->AttributesEx2 |= SPELL_ATTR1_CANT_BE_REFLECTED;
                break;
            case 56431:
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
            case 61693: // Arcane Storm - 10er
                spellInfo->MaxAffectedTargets = 4;
                break;
            case 61694: // Arcane Storm - 25er
                spellInfo->MaxAffectedTargets = 10;
                break;
            case 56397: // Arcane Barrage
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 55853: // Vortex (4)
            case 56263: // Vortex (5)
                // remove me, when the last tick isn't dropped any longer
                spellInfo->AttributesEx5 |= SPELL_ATTR5_START_PERIODIC_AT_APPLY;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 57143: // Life Burst
                spellInfo->Effects[EFFECT_1].BasePoints = 2500;
                spellInfo->Effects[EFFECT_1].PointsPerComboPoint = 2500;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_VEHICLE;
                break;
            case 57431: // Summon Static Field
            case 42867: // Magic Wings
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(18); // 20secs, was -1 (unlimited)
                break;
            case 63934: // Arcane Barrage
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT; // casted by player on self
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 55849: // Power Spark
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
// ########## ENDOF MALYGOS SPELLS ##########


// ########## ULDUAR SPELLS ##########
            case 62355:// Leviathan-Fight / vehicle: charge
            case 62306: // Leviathan-Fight / vehicle: boulder
            case 62357: // Leviathan-Fight / vehicle: cannon
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_20_YARDS);
                break;
            case 62490: // Leviathan-Fight / vehicle: pyrit
            case 62308: // Leviathan-Fight / vehicle: ram
            case 62345: // Leviathan-Fight / vehicle: ram
                spellInfo->Effects[EFFECT_2].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_20_YARDS);
                break;
            case 62363: // Leviathan - Anti Air Rocket
                spellInfo->Effects[EFFECT_0].Effect = 0; // deals enemy damage but it shouldn't
                break;
            case 62907:     // Leviathan -  Freyas Ward
            case 62947:
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(0);
                break;
            case 62576: // Blizzard (Thorim)
            case 62602:
                // @workaround: looks like TARGET_DEST_CASTER in effect 0 overrides
                // TARGET_DEST_CASTER_LEFT in effect 1, this aura needs to spawn 13 yds to the left of the caster
                // without this was spawning at caster's pos.
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_DEST_CASTER_LEFT);
                break;
            case 63527:     // Summon Charged Sphere
                spellInfo->Effects[1].Effect = 0;
                break;
            case 62713:     // Ironbranch's Essence
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(39);
                break;
            case 61934: // Ulduar Leap
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(174); // 1000 Yards
                break;
            case 61924: // Ulduar Lava Burst
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(34); // 25 yards
                break;
            case 65210: // Keeper Mimiron Destabilization Matrix - Ignore LoS (because Mimiron stands in a Tube and is out of LoS)
            case 62042: // Thorim - Stormhammer
            case 62521: // Freya - Attuned to Nature 25 Dose Reduction
            case 62524: // Freya - Attuned to Nature 2 Dose Reduction
            case 62525: // Freya - Attuned to Nature 10 Dose Reduction
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 62488: // Ignis Activate Construct
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 63342: // Focused Eyebeam Summon Trigger (Kologarn)
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
            case 62716: // Growth of Nature (Freya)
            case 65584: // Growth of Nature (Freya)
            case 64381: // Strength of the Pack (Auriaya)
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 63018: // Searing Light (XT-002)
            case 65121: // Searing Light (25m) (XT-002)
            case 63024: // Gravity Bomb (XT-002)
            case 64234: // Gravity Bomb (25m) (XT-002)
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 64600: // Freya - Nature Bomb (GO Visual)
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(38);
                break;
            case 62056: // Kologarn - some Stone Grip related Spells that have SPELL_ATTR1_IGNORE_IMMUNITY (NYI?)
            case 63985:
            case 64224:
            case 64225:
            case 62287: // Tar Passive
                spellInfo->Attributes |= SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY;
                break;
            case 62711: // Ignis - Grab
                spellInfo->Attributes |= SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY;
                spellInfo->AttributesEx2 |= SPELL_ATTR1_CANT_BE_REFLECTED;
                break;
            case 62834: // Boom (XT-002)
            // This hack is here because we suspect our implementation of spell effect execution on targets
            // is done in the wrong order. We suspect that EFFECT_0 needs to be applied on all targets,
            // then EFFECT_1, etc - instead of applying each effect on target1, then target2, etc.
            // The above situation causes the visual for this spell to be bugged, so we remove the instakill
            // effect and implement a script hack for that.
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 64386: // Terrifying Screech (Auriaya)
                spellInfo->ChannelInterruptFlags &= ~AURA_INTERRUPT_FLAG_MOVE;
                break;
            case 64389: // Sentinel Blast (Auriaya)
            case 64678: // Sentinel Blast (Auriaya)
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(28); // 5 seconds, wrong DBC data?
                spellInfo->ChannelInterruptFlags = 0;
                break;
            case 64321: // Potent Pheromones (Freya)
                // spell should dispel area aura, but doesn't have the attribute
                // may be db data bug, or blizz may keep reapplying area auras every update with checking immunity
                // that will be clear if we get more spells with problem like this
                spellInfo->AttributesEx |= SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY;
                break;
            case 64468: // Empowering Shadows (Yogg-Saron)
            case 64486: // Empowering Shadows (Yogg-Saron)
                spellInfo->MaxAffectedTargets = 3;  // same for both modes?
                break;
            case 36238: // fel fire blast hack
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(13);
                break;
            case 63277: // Shadow Crash
                spellInfo->Effects[EFFECT_0].MiscValue = SPELL_SCHOOL_MASK_MAGIC; // include shadow damage
                spellInfo->Effects[EFFECT_2].Effect = 0;
                break;
            case 63716: // Stone Shout (10man)
            case 64005: // (25man)
                spellInfo->Effects[EFFECT_0].TargetA = 1;
                break;
            case 62661: // Searing Flames
                spellInfo->InterruptFlags |= SPELL_INTERRUPT_FLAG_INTERRUPT;
                break;
            case 62030: // Petrifying Breath (10man)
            case 63980: // Petrifying Breath 25man
                spellInfo->AttributesEx5 &= ~SPELL_ATTR5_START_PERIODIC_AT_APPLY;
                break;
            case 61915: // Lightning Whirl (10man)
            case 63483: // 25man
                spellInfo->InterruptFlags |= SPELL_INTERRUPT_FLAG_INTERRUPT;
                break;
            case 61889: // Meltdown
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 62659: // Shadowcrash - Vezax
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_8_YARDS);
                spellInfo->Effects[1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_8_YARDS);
                break;
            case 61911: // Static Disruption (10man)
            case 63495: // (25man)
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_TRIGGER_SPELL;
                break;
            case 62912: // Thorim's Hammer
                spellInfo->Effects[EFFECT_0].DieSides = 3001;
                spellInfo->Effects[EFFECT_0].BasePoints = 13499;
                break;
            case 62343: // Heat
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(17); // 13yrd up from 10yrd(id:13)
                break;
            case 64145: // Diminish Power
                spellInfo->StackAmount = 4;
                break;
            case 63689: // Plasma Ball (10man)
            case 64535: // (25man)
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
                break;
            case 62283: // Freya - Iron roots
                // Hackfix to avoid dispelling by hand of freedom etc.
                spellInfo->Mechanic = MECHANIC_NONE; // was: MECHANIC_ROOT
                break;
            case 64599: // Arcane Barrage (10man)
            case 64607: // Arcane Barrage (25man)
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 62039: // Hodir - Biting Cold - Remove on Move
                spellInfo->AuraInterruptFlags |= AURA_INTERRUPT_FLAG_MOVE;
                break;
            case 62476: // Hodir - Flash Freeze Icicle Summon (10man)
                spellInfo->MaxAffectedTargets = 2;
                break;
            case 62477: // Hodir - Flash Freeze Icicle Summon (25man)
                spellInfo->MaxAffectedTargets = 3;
                break;
            case 61968: // Hodir - Flash Freeze
                spellInfo->Attributes |= SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY;
                break;
            case 62807: // Hodir - Starlight
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_1_YARD);
                spellInfo->StackAmount = 1;
                break;
            case 62797: // Hodir - Stormcloud (Selector)
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 62457: // Hodir - Ice Shards
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_2_YARDS);
                spellInfo->Effects[1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_2_YARDS);
                break;
            case 65370: // Hodir - Ice Shards
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_5_YARDS);
                spellInfo->Effects[1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_5_YARDS);
                break;
            case 62017: // Thorim - Lightning Shock
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 62320: // Thorim - Aura of Celerity
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                spellInfo->StackAmount = 2;
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_APPLY_AREA_AURA_FRIEND;
                break;
            case 62016: // Thorim - Charge Orb
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 62862: // Freya - Iron Roots Summon (10man)
            case 62450: // Freya - Unstable Sunbeam Summon (10man)
            case 62275: // Freya - Elder Ironbranch - Iron Roots Summon (10man)
            case 62929: // Freya - Elder Ironbranch - Iron Roots Summon (25man)
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 62868: // Freya - Unstable Sunbeam Summon (25man)
            case 62439: // Freya - Iron Roots Summon (25man)
                spellInfo->MaxAffectedTargets = 3;
                break;
            case 62207: // Freya - Elder Brightlead - Unstable Sunbeam Summon
                spellInfo->Effects[EFFECT_1].Effect = 0;
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 62584: // Freya - Lifebinder's Gift (10man)
            case 64185: // Freya - Lifebinder's Gift (25man)
                spellInfo->Effects[EFFECT_2].Effect = 0;
                break;
            case 62590: // Freya - Ancient Conservator - Nature's Fury (10man)
            case 63570: // Freya - Ancient Conservator - Nature's Fury (25man)
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
                break;
            //case 65761: // Freya - Brightleaf - Brightleafs Essence
            case 62968: // Freya - Brightleaf - Brightleafs Essence
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(39);
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 63036: // Mimiron - VX-001 - Rocket Strike
                spellInfo->Attributes |= SPELL_ATTR0_CASTABLE_WHILE_MOUNTED;
                spellInfo->AttributesEx6 |= SPELL_ATTR6_CASTABLE_WHILE_ON_VEHICLE;
                break;
            case 65229: // Mimiron - MK II - Clear Fires
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(39); // 2sec
                break;
            case 64567: // Mimiron - Summon Flames (Trigger)
                spellInfo->MaxAffectedTargets = 3;
                spellInfo->Attributes |= SPELL_ATTR0_CASTABLE_WHILE_MOUNTED;
                spellInfo->AttributesEx6 |= SPELL_ATTR6_CASTABLE_WHILE_ON_VEHICLE;
                break;
            case 64564: // Mimiron - Summon Flames (Spread)
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_CASTER;
                break;
            case 63830: // Malady of Mind
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_2].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_2].TargetB = TARGET_UNIT_TARGET_ANY;
                spellInfo->Attributes |= SPELL_ATTR0_CANT_CANCEL;
                break;
            case 63802: // Brain Link
                spellInfo->MaxAffectedTargets = 2;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_TARGET_ANY;
                spellInfo->Attributes |= SPELL_ATTR0_CANT_CANCEL;
                break;
            case 64465: // Yogg Saron Shadow Beacon
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_TARGET_ANY;
                break;
            case 64163: //Lunatic Gaze (Yogg Saron)
                spellInfo->Effects[EFFECT_1].Effect = 0;    // Disable instant tick
                break;
            case 65301: // Sara Psychosis (25man)
            case 63795: // Sara Psychosis (10man)
                spellInfo->Effects[EFFECT_1].Effect = 0;
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                break;
            case 63747: // Yogg Saron - Sara - Saras Fervor P1
            case 63745: // Yogg Saron - Sara - Saras Blessing P1
            case 63744: // Yogg Saron - Sara - Saras Anger P1
            case 62978: // Yogg Saron - Sara - Summon Guardian P1
            case 64132: // Yogg Saron - Yogg - Constrictor Tentacle P2
            case 64172: // Yogg Saron - Thorim - Titanic Storm
            case 65206: // Yogg Saron - Mimiron - Destabilization Matrix
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 64144: // Yogg Saron - Tentacle - Erupt P2
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(14); // 8yrd
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(14); // 8yrd
                break;
            case 64152: // Yogg Saron - Corruptor Tentacle - draining poison
            case 64153: // Yogg Saron - Corruptor Tentacle - black plague
            case 64156: // Yogg Saron - Corruptor Tentacle - apathy
            case 64157: // Yogg Saron - Corruptor Tentacle - curse of doom
            case 64159: // Yogg Saron - Immortal Guardian - Lifedrain
            case 64160: // Yogg Saron - Immortal Guardian - Lifedrain
                spellInfo->InterruptFlags |= SPELL_INTERRUPT_FLAG_INTERRUPT;
                break;
            case 62301: // Algalon - Cosmic Smash
               spellInfo->MaxAffectedTargets = 1;
               break;
            case 64598: // Algalon - Cosmic Smash
                spellInfo->MaxAffectedTargets = 3;
                break;
            case 62293: // Algalon - Cosmic Smash
                break;
            case 62311: // Algalon - Cosmic Smash
            case 64596: // Algalon - Cosmic Smash
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(6);  // 100yd
                break;
            case 64443: // Algalon - Big Bang
            case 64584: // Algalon - Big Bang
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 65088: // Scrapbot Sawblade
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(21);
                break;
            case 65015: // SPELL_DEFORESTATION_CREDIT
            case 65074: // SPELL_KNOCK_ON_WOOD_CREDIT
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_DEAD;
                break;
            case 64014: //SPELL_EXPEDITION_BASE_CAMP_TELEPORT
            case 64024: //SPELL_CONSERVATORY_TELEPORT
            case 64025: //SPELL_HALLS_OF_INVENTIONS_TELEPORT
            case 64026: //SPELL_HALLS_OF_WINTER_TELEPORT
            case 64027: //SPELL_DESCEND_INTO_MADNESS_TELEPORT
            case 64028: //SPELL_COLOSSAL_FORGE_TELEPORT
            case 64029: //SPELL_SHATTERED_WALKWAY_TELEPORT
            case 64030: //SPELL_ANTECHAMBER_TELEPORT
            case 64031: //SPELL_SCRAPYARD_TELEPORT
            case 64032: //SPELL_FORMATIONS_GROUNDS_TELEPORT
            case 65042: //SPELL_PRISON_OF_YOGG_SARON_TELEPORT
            case 65061: //SPELL_ULDUAR_TRAM_TELEPORT
                spellInfo->Effects[EFFECT_1].TargetB = TARGET_DEST_DB;
                break;
// ########## ENDOF ULDUAR SPELLS ##########


// ########## TRIAL OF THE CRUSADER SPELLS ##########
            case 66331: // ToC - Gormock - Impale
            case 67477: // ToC - Gormock - Impale
            case 67478: // ToC - Gormock - Impale
            case 67479: // ToC - Gormock - Impale -- Add bleeding mechanic for dot to ignore armor
                 spellInfo->Effects[EFFECT_1].Mechanic = MECHANIC_BLEED;
            break;
            case 66258: // Infernal Eruption (10N)
            case 67901: // Infernal Eruption (25N)
                // increase duration from 15 to 18 seconds because caster is already
                // unsummoned when spell missile hits the ground so nothing happen in result
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(85);
                break;
            case 65546: // Dispel Magic (10N)
            case 68624: // Dispel Magic (10H)
            case 68625: // Dispel Magic (25N)
            case 68626: // Dispel Magic (25H)
                // may hit friendly and hostile targets, selection done via scipt
            case 42337: // Raptor Capture Credit
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 66132: // Twin Empathy (dark)
            case 66133: // Twin Empathy (light)
                spellInfo->Attributes |= SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY;
                spellInfo->AttributesEx |= SPELL_ATTR1_NO_THREAT;
                break;
            case 67721: // Expose Weakness 10man
            case 67847: // Expose Weakness 25man
                spellInfo->StackAmount = 9;
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_1].BasePoints = 39;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT;
                break;
            case 59892: // Cyclone Fall
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_APPLY_AREA_AURA_FRIEND;
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_10_YARDS);
                spellInfo->AttributesEx &= ~SPELL_ATTR0_CANT_CANCEL;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
            case 66129: // Spider Frenzy
            case 65775: // Acid-Drenched Mandibles
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 65983: // Faction Champions - Heroism
            case 65980: // Faction Champions - Bloodlust
                spellInfo->DmgClass = SPELL_DAMAGE_CLASS_MAGIC;
                break;
            case 66879: // Burning Bite
            case 67624:
            case 67625:
            case 67626:
            case 66824: // Paralytic Bite
            case 67612:
            case 67613:
            case 67614:
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 66263: // Nether Portal
            case 67103:
            case 67104:
            case 67105:
                spellInfo->Effects[EFFECT_0].Amplitude = 5*IN_MILLISECONDS;
                break;
            case 66870: // Burning Bile
            case 67621:
            case 67622:
            case 67623:
            case 66240: // Leeching Swarm
            case 69490: // Aura of Darkness
            case 72629: // Aura of Darkness
                spellInfo->AttributesEx7 |= SPELL_ATTR7_NO_PUSHBACK_ON_DAMAGE;
                break;
            case 65950: // Touch of Light
            case 67296:
            case 67297:
            case 67298:
            case 66001: // Touch of Dark
            case 67281:
            case 67282:
            case 67283:
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->Effects[EFFECT_0].TargetA= TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
                // Increase base points in several modes
                // NORTHEND BEATS
            case 66818: // Acidic Spew - 8 ticks
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(39);
                break;
                // LORD JARAXXUS
                // TWINS
            case 65684: // 10 man normal (dark essence)
            case 67176: // 25 man normal (dark essence)
            case 67177: // 10 man heroic (dark essence)
            case 67178: // 25 man heroic (dark essence)
            case 65686: // 10 man normal (light essence)
            case 67222: // 25 man normal (light essence)
            case 67223: // 10 man heroic (light essence)
            case 67224: // 25 man heroic (light essence)
                spellInfo->Effects[EFFECT_0].BasePoints = 100000000; // default * 100 
                break;
// ########## ENDOF TRIAL OF THE CRUSADER SPELLS ##########


// ########## ONYXIAS LAIR SPELLS ##########
            case 18500: // Wing Buffet (10man)
            case 69293: // Wing Buffet (25man)
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 68958: // Blast Nova
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_18_YARDS); // should be 15, but with 15 the radius is too small
                break;
// ########## ENDOF ONYXIAS LAIR SPELLS


// ########## ICECROWN CITADEL SPELLS ##########
            // THESE SPELLS ARE WORKING CORRECTLY EVEN WITHOUT THIS HACK
            // THE ONLY REASON ITS HERE IS THAT CURRENT GRID SYSTEM
            // DOES NOT ALLOW FAR OBJECT SELECTION (dist > 333)
            case 70781: // Light's Hammer Teleport
            case 70856: // Oratory of the Damned Teleport
            case 70857: // Rampart of Skulls Teleport
            case 70858: // Deathbringer's Rise Teleport
            case 70859: // Upper Spire Teleport
            case 70860: // Frozen Throne Teleport
            case 70861: // Sindragosa's Lair Teleport
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_DEST_DB);
                break;
            case 71464: // Divine Surge
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_30_YARDS);
                break;
            case 69492:
                spellInfo->Effects[EFFECT_0].BasePoints = 150; // Spellinfo
                break;
            case 69055: // Bone Slice (Lord Marrowgar)
            case 70814: // Bone Slice (Lord Marrowgar)
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_5_YARDS); // 5yd
                break;
            case 69138: // Coldflame
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                spellInfo->Speed = 0.0f;
                break;
            case 69075: // Bone Storm (Lord Marrowgar)
            case 70834: // Bone Storm (Lord Marrowgar)
            case 70835: // Bone Storm (Lord Marrowgar)
            case 70836: // Bone Storm (Lord Marrowgar)
            case 71160: // Plague Stench (Stinky)
            case 71161: // Plague Stench (Stinky)
            case 71123: // Decimate (Stinky & Precious)
                spellInfo->StackAmount = 99;
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_100_YARDS); // 100yd
                break;
            case 72864: // Death Plague (Rotting Frost Giant)
				spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_8_YARDS); // 8yd
                break;
            case 71906: // Severed Essence (Val'kyr Herald)
            case 71942: // Severed Essence (Val'kyr Herald)
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ENEMY);
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 17364: // Stormstrike
            case 71169: // Shadow's Fate
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 53651: // Light's Beacon
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 72347: // Lock Players and Tap Chest
                spellInfo->AttributesEx3 &= ~SPELL_ATTR3_NO_INITIAL_AGGRO;
                break;
            case 73843: // Award Reputation - Boss Kill
            case 73844: // Award Reputation - Boss Kill
            case 73845: // Award Reputation - Boss Kill
            case 73846: // Award Reputation - Boss Kill
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_50000_YARDS); // 50000yd
                break;
            case 72378: // Blood Nova (Deathbringer Saurfang)
            case 73058: // Blood Nova (Deathbringer Saurfang)
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_200_YARDS);
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_200_YARDS);
                break;
            case 72769: // Scent of Blood (Deathbringer Saurfang)
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_200_YARDS);
                // no break
            case 72771: // Scent of Blood (Deathbringer Saurfang)
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_200_YARDS);
                break;
            case 72723: // Resistant Skin (Deathbringer Saurfang adds)
                // this spell initially granted Shadow damage immunity, however it was removed but the data was left in client
                spellInfo->Effects[EFFECT_2].Effect = 0;
                break;
            case 72255: // Mark of the fallen Champion damage (Deathbringer Saurfang)
            case 72444: // Mark of the fallen Champion damage (Deathbringer Saurfang)
            case 72445: // Mark of the fallen Champion damage (Deathbringer Saurfang)
            case 72446: // Mark of the fallen Champion damage (Deathbringer Saurfang)
                spellInfo->AttributesEx3 |=  SPELL_ATTR3_NO_DONE_BONUS;
                break;
            case 70460: // Coldflame Jets (Traps after Saurfang)
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(1); // 10 seconds
                break;
            case 71412: // Green Ooze Summon (Professor Putricide)
            case 71415: // Orange Ooze Summon (Professor Putricide)
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ANY);
                break;
            case 71159: // Awaken Plagued Zombies
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(21);
                break;
            case 70530: // Volatile Ooze Beam Protection (Professor Putricide)
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_APPLY_AURA; // for an unknown reason this was SPELL_EFFECT_APPLY_AREA_AURA_RAID
                break;
            // THIS IS HERE BECAUSE COOLDOWN ON CREATURE PROCS IS NOT IMPLEMENTED
            case 71604: // Mutated Strength (Professor Putricide)
            case 72673: // Mutated Strength (Professor Putricide)
            case 72674: // Mutated Strength (Professor Putricide)
            case 72675: // Mutated Strength (Professor Putricide)
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 72454: // Mutated Plague (Professor Putricide)
            case 72464: // Mutated Plague (Professor Putricide)
            case 72506: // Mutated Plague (Professor Putricide)
            case 72507: // Mutated Plague (Professor Putricide)
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_50000_YARDS); // 50000yd
                spellInfo->AttributesEx4 |= SPELL_ATTR4_IGNORE_RESISTANCES;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 70911: // Unbound Plague (Professor Putricide) (needs target selection script)
            case 72854: // Unbound Plague (Professor Putricide) (needs target selection script)
            case 72855: // Unbound Plague (Professor Putricide) (needs target selection script)
            case 72856: // Unbound Plague (Professor Putricide) (needs target selection script)
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ENEMY);
                spellInfo->AttributesEx4 |= SPELL_ATTR4_IGNORE_RESISTANCES;
                break;
            case 70917: //Unbound Plague Searcher (Professor Putricide) (needs target selection script)
                // Should be able to be cast on friendly units, so we can refind
                // former plague owners
                spellInfo->Targets |= TARGET_FLAG_UNIT_MASK;
                spellInfo->Effects[EFFECT_0].Amplitude = 400;
                break;
            case 71518: // Unholy Infusion Quest Credit (Professor Putricide)
            case 72934: // Blood Infusion Quest Credit (Blood-Queen Lana'thel)
            case 72289: // Frost Infusion Quest Credit (Sindragosa)
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_200_YARDS); // another missing radius
                break;
            case 71708: // Empowered Flare (Blood Prince Council)
            case 72785: // Empowered Flare (Blood Prince Council)
            case 72786: // Empowered Flare (Blood Prince Council)
            case 72787: // Empowered Flare (Blood Prince Council)
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_15_YARDS);
                break;
            case 72039: // Empowered Shock Vortex (Blood Prince Council)
            case 73037: // Empowered Shock Vortex (Blood Prince Council)
            case 73038: // Empowered Shock Vortex (Blood Prince Council)
            case 73039: // Empowered Shock Vortex (Blood Prince Council)
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_50000_YARDS);
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_50000_YARDS);
                break;
            case 71266: // Swarming Shadows
            case 72890: // Swarming Shadows
                spellInfo->AreaGroupId = 0; // originally, these require area 4522, which is... outside of Icecrown Citadel
                break;
         case 12579: // Winter's Chill Rank 1-3
            case 70602: // Corruption
            case 48278: // Paralyze
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 70715: // Column of Frost (visual marker)
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(32); // 6 seconds (missing)
                break;
            case 71085: // Mana Void (periodic aura)
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(9); // 30 seconds (missing)
                break;
            case 72015: // Frostbolt Volley (only heroic)
            case 72016: // Frostbolt Volley (only heroic)
                spellInfo->Effects[EFFECT_2].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_40_YARDS);
                break;
            case 72037: // Shock Vortex
                spellInfo->Effects[EFFECT_2].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_0_YARDS);
                break;
            case 72092: // Frozen Orb
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_0_YARDS);
                break;
            case 72091: // Frozen Orb
            case 72095: // Frozen Orb
                spellInfo->Effects[EFFECT_0].TriggerSpell = 0;
                break;
            case 70936: // Summon Suppressor (needs target selection script)
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ANY);
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo();
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(13);
                break;
            case 72706: // Achievement Check (Valithria Dreamwalker)
            case 71357: // Order Whelp
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_200_YARDS);   // 200yd
                break;
            case 70598: // Sindragosa's Fury
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_DEST_DEST);
                break;
            case 69846: // Frost Bomb
            case 69489: // Chocolate Sample
                spellInfo->Speed = 0.0f;    // This spell's summon happens instantly
                break;
            case 71614: // Ice Lock
                spellInfo->Mechanic = MECHANIC_STUN;
                break;
            case 72762: // Defile
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(559); // 53 seconds
                break;
            case 72743: // Defile
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(22); // 45 seconds
                break;
            case 72754: // Defile
            case 73708: // Defile
            case 73709: // Defile
            case 73710: // Defile
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_200_YARDS); // 200yd
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_200_YARDS); // 200yd
                break;
            case 70541: // Infest - 10nm
            case 73779: // Infest - 25nm
                spellInfo->Effects[EFFECT_0].BasePoints = 1150;
                break;
            case 73780: // Infest - 10hc
                spellInfo->Effects[EFFECT_0].BasePoints = 1750;
                break;
            case 73781:// Infest - 25hc
                spellInfo->Effects[EFFECT_0].BasePoints = 2500;
                break;
            case 69030: // Val'kyr Target Search
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_200_YARDS); // 200yd
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_200_YARDS); // 200yd
                break;
            case 69198: // Raging Spirit Visual
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(13); // 50000yd
                break;
            case 73654: // Harvest Souls
            case 74295: // Harvest Souls
            case 74296: // Harvest Souls
            case 74297: // Harvest Souls
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_50000_YARDS); // 50000yd
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_50000_YARDS); // 50000yd
                spellInfo->Effects[EFFECT_2].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_50000_YARDS); // 50000yd
                break;
            case 73655: // Harvest Soul
            case 71841: // Smite
            case 71842: // Smite
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                spellInfo->AttributesEx6 |= SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS;
                break;
            case 20267: // Judgement of Light
                spellInfo->AttributesEx6 |= SPELL_ATTR6_NO_HEALING_BONUS;
                break;
            case 73540: // Summon Shadow Trap
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(23); // 90 seconds
                break;
            case 73530: // Shadow Trap (visual)
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(28); // 5 seconds
                break;
            case 73529: // Shadow Trap
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_10_YARDS); // 10yd
                break;
            case 74282: // Shadow Trap (searcher)
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_3_YARDS); // 3yd
                break;
            case 72595: // Restore Soul
            case 73650: // Restore Soul
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_200_YARDS); // 200yd
                break;
            case 74086: // Destroy Soul
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_200_YARDS); // 200yd
                break;
            case 74302: // Summon Spirit Bomb
            case 74342: // Summon Spirit Bomb
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_200_YARDS); // 200yd
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 74341: // Summon Spirit Bomb
            case 74343: // Summon Spirit Bomb
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_200_YARDS); // 200yd
                spellInfo->MaxAffectedTargets = 3;
                break;
            case 73579: // Summon Spirit Bomb
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_25_YARDS); // 25yd
                break;
            case 72350: // Fury of Frostmourne
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_50000_YARDS); // 50000yd
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_50000_YARDS); // 50000yd
                break;
            case 75127: // Kill Frostmourne Players
            case 72351: // Fury of Frostmourne
            case 72431: // Jump (removes Fury of Frostmourne debuff)
            case 72429: // Mass Resurrection
            case 73159: // Play Movie
            case 73582: // Trigger Vile Spirit (Inside, Heroic)
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_50000_YARDS); // 50000yd
                break;
            case 72376: // Raise Dead
                spellInfo->MaxAffectedTargets = 3;
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_50000_YARDS); // 50000yd
                break;
            case 71809: // Jump
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(3); // 20yd
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_25_YARDS); // 25yd
                break;
            case 72405: // Broken Frostmourne
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_200_YARDS); // 200yd
                break;
            case 69065: // Impaled
                spellInfo->Mechanic = MECHANIC_STUN;
                spellInfo->DmgClass = SPELL_DAMAGE_CLASS_MELEE;
                break;
            case 69200: // ICC - LK - Raging Spirit
                spellInfo->Attributes |= SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY;
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 70232: // ICC - empowered blood triggered
                spellInfo->Effects[EFFECT_0].MiscValue = 127;
                spellInfo->Effects[EFFECT_0].MiscValueB = 127;
                break;
            case 69146: // Coldflame
            case 70823: // Coldflame
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(32); // 6 seconds instead of 3
                break;
            case 70824: // Coldflame
            case 70825: // Coldflame
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(1); // 10 seconds instead of 3 in Hc
                break;
            case 70475: // Giant Insect Swarm (putricide trap)
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(9); // 30 seconds
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_80_YARDS);
                break;
            case 70484: // Summon Plagued Insects
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(22); // 45 seconds - guessed value
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_TRAJ;
                break;
            case 71475: // Essence of the Blood Queen 60sek duration - nh
                spellInfo->Effects[EFFECT_1].TriggerSpell = 71473;
                break;
            case 71477: // Essence of the Blood Queen 60sek duration -hc
                spellInfo->Effects[EFFECT_1].TriggerSpell = 71533;
                break; 
            case 71837: // Vampiric Bite
                spellInfo->AttributesEx &= ~SPELL_ATTR0_HIDE_IN_COMBAT_LOG;
                break;
            case 69628: // Icy Blast
            case 69238: // Icy Blast
            case 71380: // Icy Blast
            case 71381: // Icy Blast
                spellInfo->Mechanic = MECHANIC_SNARE;
                break;
            case 70897: // Dark Martyrdom
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_DEAD;
                break;
            case 69188: // Rocket Pack
            case 73077: // Rocket Pack
                spellInfo->Attributes |= SPELL_ATTR0_CANT_CANCEL;
                break;
            case 69783: // Ooze flood (Rotface)
            case 69797: // Ooze flood (Rotface)
            case 69799: // Ooze flood (Rotface)
            case 69802: // Ooze flood (Rotface)
                // Those spells are cast on creatures with same entry as caster while they have TARGET_UNIT_NEARBY_ENTRY.
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_TARGET_SELF;
                break;
            // HoR Spells
            case 69780: // Remorseless Winter
            case 69781: // Remorseless Winter
            case 74115: // Pain and Suffering
            case 74116: // Pain and Suffering
            case 74117: // Pain and Suffering
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 69705: // Below Zero - Gunship
                spellInfo->InterruptFlags = 0;
                spellInfo->ChannelInterruptFlags = 0;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                spellInfo->AttributesEx &= ~SPELL_ATTR1_CHANNELED_1;
                break;   
            case 71088: // Blight Bomb
                spellInfo->InterruptFlags = 0;
                break;
            case 70173: // Canon Blast - Gunship
            case 69400: // Canon Blast - Gunship
                spellInfo->Effects[EFFECT_0].BasePoints = 1499;
                spellInfo->Effects[EFFECT_1].BasePoints = 1499;
                break;
            case 69399: // Cannon Blast - Gunship
            case 70172: // Cannon Blast - Gunship
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(174); // 1000 Yards
                spellInfo->Speed = 0.0f;
                break;
            case 69401: // Incinerating Blast - Gunship
            case 70174: // Incinerating Blast - Gunship
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(174); // 1000 Yards
                spellInfo->Speed = 0.0f;
                break;
            case 69660: // Burning Pitch - Gunship
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 72053: // Kinetic Bomb (Blood Prince Council)
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_10_YARDS);
                break;
            case 71341: // Pakt of the Darkfallen (Blood Queen Lana'thel)
                spellInfo->AttributesEx4 |= SPELL_ATTR4_IGNORE_RESISTANCES;
                break;
            case 71815: // Empowered Shadow Lance (Blood Prince Council)
            case 72809: // Empowered Shadow Lance (Blood Prince Council)
            case 72810: // Empowered Shadow Lance (Blood Prince Council)
            case 72811: // Empowered Shadow Lance (Blood Prince Council)
                spellInfo->AttributesEx4 |= SPELL_ATTR4_IGNORE_RESISTANCES;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 69409: // Soul Reaper (Lichking)
            case 73797: // Soul Reaper (Lichking)
            case 73798: // Soul Reaper (Lichking)
            case 73799: // Soul Reaper (Lichking)
                spellInfo->AttributesEx4 |= SPELL_ATTR4_IGNORE_RESISTANCES;
                break;
            case 70157: // Icetomb (Sindragosa)
                spellInfo->ExcludeTargetAuraSpell = 0;
                break;
            case 70126: // Frost Beacon (Sindragosa)
                spellInfo->ExcludeTargetAuraSpell = 69700;
                break;
            case 69099: // Ice Pulse (Lichking)
            case 73776: // Ice Pulse (Lichking)
            case 73777: // Ice Pulse (Lichking)
            case 73778: // Ice Pulse (Lichking)
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT; 
                break;
            case 71617: // Tear Gas (Putricide)
                spellInfo->Speed = 0.0f; // This spell is instant
                break;
            case 71280: // Putricide - Choking Gas Bomb Periodic Explosion Trigger
                spellInfo->Effects[EFFECT_0].Amplitude = 20000;
                break;
            case 71273: // Putricide - Choking Gas Bomb Summon
            case 71275:
            case 71276:
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(18); // 20 seconds
                break;
            case 70521: // IceBomb (PoS)
                spellInfo->Effects[1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_50000_YARDS);
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 69508: // Slime Spray (ICC)
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->InterruptFlags = 0;
                spellInfo->AuraInterruptFlags = 0;
                spellInfo->ChannelInterruptFlags = 0;
                break;
            case 73576: // Spirit Bomb (Lichking)
            case 73803: // Spirit Bomb (Lichking)
            case 73804: // Spirit Bomb (Lichking)
            case 73805: // Spirit Bomb (Lichking)
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_5_YARDS);
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 72830: // Achievement Check (HoR)
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_50000_YARDS); // 50000yd
                break;
            case 72900: // Start Halls of Reflection Quest AE (HoR)
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_200_YARDS); // 200yd
                break;
            case 71709: // Ball of Flames Periodic (Blood Prince Council)
                spellInfo->Effects[EFFECT_0].Amplitude = 1000;
                break;
            case 71822: // Shadowresonance (Blood Prince Council)
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(6);  // 100yd
                spellInfo->AttributesEx4 |= SPELL_ATTR4_FIXED_DAMAGE;
                break;
            case 71393: // Flames (Blood Prince Council)
            case 72789: // Flames (Blood Prince Council)
            case 72790: // Flames (Blood Prince Council)
            case 72791: // Flames (Blood Prince Council)
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
            case 55891: // Flame Sphere Spawn Effect (Blood Prince Council)
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(27); // 3 seconds
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 71302: // Summon Ymirjar
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(21);  // No Despawn
                spellInfo->Effects[EFFECT_0].BasePoints = 5;
            case 72292: // Frost Infusion
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                spellInfo->AttributesEx6 |= SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS;
                break;
            case 69649: // Frost Breath (Sindragosa)
            case 71056: // Frost Breath (Sindragosa)
            case 71057: // Frost Breath (Sindragosa)
            case 71058: // Frost Breath (Sindragosa)
            case 73061: // Frost Breath (Sindragosa)
            case 73062: // Frost Breath (Sindragosa)
            case 73063: // Frost Breath (Sindragosa)
            case 73064: // Frost Breath (Sindragosa)
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_30_YARDS);
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_30_YARDS);
                spellInfo->Effects[EFFECT_2].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_30_YARDS);
                break;
            case 72296: // Malleable Goo
                spellInfo->AttributesEx &= ~SPELL_ATTR0_HIDE_IN_COMBAT_LOG;
                break;
            case 70852: // Mallable Goo (Putricide)
                spellInfo->AttributesEx &= ~SPELL_ATTR0_HIDE_IN_COMBAT_LOG;
                spellInfo->Speed = 5.5f;
                break;
            case 70402: // Mutated Transformation
            case 72511: // Mutated Transformation
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
                break;
            case 72512: // Mutated Transformation
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
                spellInfo->Effects[EFFECT_0].BasePoints = 3000;
                break;
            case 72513: // Mutated Transformation
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
                spellInfo->Effects[EFFECT_0].BasePoints = 3400;
                break;
            case 72038: // Empowered Shock Vortex (Blood Prince Council)
            case 72815: // Empowered Shock Vortex (Blood Prince Council)
            case 72816: // Empowered Shock Vortex (Blood Prince Council)
            case 72817: // Empowered Shock Vortex (Blood Prince Council)
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_10_YARDS); 
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_11_YARDS); 
                break;
            case 71279: // Choking Gas Explosion (Professor Putricide)
            case 72459: // Choking Gas Explosion (Professor Putricide)
            case 72621: // Choking Gas Explosion (Professor Putricide)
            case 72622: // Choking Gas Explosion (Professor Putricide)
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_8_YARDS);
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_8_YARDS);
                spellInfo->Effects[EFFECT_2].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_8_YARDS);
                break;
            case 72508: // Mutated Transformation (25 NH)
                spellInfo->Effects[EFFECT_0].TriggerSpell = 72511;
                break;
            case 72509: // Mutated Transformation (10 HC)
                spellInfo->Effects[EFFECT_0].TriggerSpell = 72512;
                break;
            case 72510: // Mutated Transformation (25 HC)
                spellInfo->Effects[EFFECT_0].TriggerSpell = 72513;
                break;
            case 70337: // necrotic plague
            case 73912:
            case 73913:
            case 73914:
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
// ########## ENDOF ICECROWN CITADEL SPELLS ##########
                // HALLS OF REFLECTION SPELLS
                //
            case 72435: // Defiling Horror
            case 72452: // Defiling Horror
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_60_YARDS); // 60yd
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_60_YARDS); // 60yd
                break;
                // ENDOF HALLS OF REFLECTION SPELLS

// ########## RUBY SANCTUM SPELLS ##########
            case 74637: // Meteor Strike
                spellInfo->Speed = 0.f; // will be handled via spell delay
                break;
            case 74641: // Meteor Strike
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(589);
                break;
            case 74799: // Soul Consumption
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_12_YARDS);
                break;
            case 74769: // Twilight Cutter
            case 77844: // Twilight Cutter
            case 77845: // Twilight Cutter
            case 77846: // Twilight Cutter
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_100_YARDS); // 100yd
                break;
            case 75509: // Twilight Mending
                spellInfo->AttributesEx6 |= SPELL_ATTR6_CAN_TARGET_INVISIBLE;
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                spellInfo->Effects[EFFECT_0].BasePoints = 20;
                spellInfo->Effects[EFFECT_1].BasePoints = 20;
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_100_YARDS);
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_100_YARDS);
                break;
            case 75888: // Awaken Flames
            case 75889: // Awaken Flames
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_0].MiscValue = 127;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_2].TargetA = TARGET_UNIT_CASTER;
                break;
            case 74384: // Intimidating Roar
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_100_YARDS);
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_100_YARDS);
                spellInfo->AuraInterruptFlags = 0;
                break;
            case 75884: // Combustion (25HC)
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_6_YARDS);
                // no break
            case 75883: // Combustion (10HC)
            case 75876: // Consumption (25HC)
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_6_YARDS);
            case 74630: // Combustion (10NM)
            case 75882: // Combustion (25NM)
            case 74507: // Baltharus - Siphoned Might
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 75875: // Consumption (10HC)
                spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_6_YARDS);
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_6_YARDS);
                spellInfo->Effects[EFFECT_0].Mechanic = MECHANIC_NONE;
                spellInfo->Effects[EFFECT_1].Mechanic = MECHANIC_SNARE;
                break;
            case 75886: // Blazing Aura
            case 75887: // Blazing Aura
                spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_DEST_AREA_ENTRY;
                break;
// ########## ENDOF RUBY SANCTUM SPELLS ##########


            // OCULUS SPELLS
            // The spells below are here, because their effect 1 is giving warning, because the triggered spell is not found in dbc and is missing from encounter sniff.
            case 49462: // Call Ruby Drake
            case 49461: // Call Amber Drake
            case 49345: // Call Emerald Drake
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 40055: // Introspection
            case 40165: // Introspection
            case 40166: // Introspection
            case 40167: // Introspection
                spellInfo->Attributes |= SPELL_ATTR0_NEGATIVE_1;
                break;
            case 45524: // Chains of Ice
                spellInfo->Effects[EFFECT_2].TargetA = SpellImplicitTargetInfo();
                break;
            case 2378: // Minor Fortitude
                spellInfo->ManaCost = 0;
                spellInfo->ManaPerSecond = 0;
                break;
            case 65783: // Ogre Pinata
                spellInfo->MaxLevel = 1;
                break;
            case 49352: // Crashin' Thrashin' Racer Controller
            case 75111: // Blue Crashin' Thrashin' Racer Controller
                spellInfo->CastTimeEntry = sSpellCastTimesStore.LookupEntry(1);
                break;
            case 74797: // Paint bomb only on players not npcs
            case 24178: // Will of Hakkar
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
            case 62973: //Foamsword
            case 62991:
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_TARGET_SELF;
                spellInfo->AttributesEx6 |=SPELL_ATTR6_CAN_TARGET_POSSESSED_FRIENDS;
                break;
            case 60488: // Extract of Necromatic Power
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_TARGET_SELF;
                break;
            case 74890: // TCG - Instant Statue
                spellInfo->Attributes |= SPELL_ATTR0_OUTDOORS_ONLY;
                break;
            case 42760: // Goblin Gumbo Hackfix
                spellInfo->Effects[EFFECT_0].ApplyAuraName = 23; // Orignal aura effect is not implemented correctly
                spellInfo->Effects[EFFECT_0].Amplitude = 6000;   // Trigger spell proc every 6sec, handle via SpellScript
                break;
            case 46363: // Midsummer - Beam Attack against Ahune
                spellInfo->Effects[0].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_TARGET_ANY;
                break;
            case 45732: // Midsummer - Torch Toss
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(6); // 100 yards
                break;
            case 61460: // Ankahet - Plaguewalker - Aura of lost hope
            case 56710: // Ankahet - Plaguewalker - Aura of lost hope
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_40_YARDS);
                spellInfo->Effects[1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_40_YARDS);
                spellInfo->AttributesEx |= SPELL_ATTR1_NO_THREAT;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                break;
            case 61459: // Ankahet - Plaguewalker - Aura of lost hope - triggerspell
            case 56709: // Ankahet - Plaguewalker - Aura of lost hope - triggerspell
                spellInfo->AttributesEx |= SPELL_ATTR1_NO_THREAT;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                break;
            case 56151: // Ahnkahet - Elder Nadox aura duration
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(550);
                break;
            case 55926:
            case 59508: // Ahnkahet - Taldaram Flamesphere damage
                spellInfo->Effects[0].Amplitude = 300;
                break;
            case 55964: // Ahnkahet - Vanish
                spellInfo->Effects[EFFECT_1].Effect = 0;
                spellInfo->Effects[EFFECT_2].Effect = 0;
                break;
            case 59034: // Halls of Stone - Chiseling Ray
                spellInfo->ChannelInterruptFlags = 0;
                break;
            case 59015: // Blood Siphon
                spellInfo->Effects[0].ValueMultiplier = 1;
                break;
            case 48894: // Chainheal - Draktharon - Now is 350 basepoint + 250*level
                spellInfo->Effects[0].RealPointsPerLevel = 250;
                break;
            case 55095: // Frost Fever
                spellInfo->Effects[0].RealPointsPerLevel = 0.368f;
                break;
            case 55078: // Blood Plague
                spellInfo->Effects[0].RealPointsPerLevel = 0.447f;
                break;
            case 50833: // Crash ground - no interupt possible
                spellInfo->InterruptFlags = 0;
                break;
            case 50645: // Overload
                spellInfo->Effects[EFFECT_1].MiscValue = SPELL_SCHOOL_MASK_NORMAL;
                break;
            case 55098:
                spellInfo->Effects[EFFECT_2].BasePoints = 74;
                spellInfo->InterruptFlags |= SPELL_INTERRUPT_FLAG_INTERRUPT;
                break;
            case 59452:
            case 59847:
                spellInfo->MaxAffectedTargets = 2;
                break;
            case 66492: // Stormforged Mole Machine
                //unknown GO_id
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 59578: // The Art of War
                // adding second effect, to stop swing timer to reset even if exorcism is an instant cast through this proc
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_IGNORE_MELEE_RESET;
                spellInfo->Effects[EFFECT_1].SpellClassMask = spellInfo->Effects[EFFECT_0].SpellClassMask;
                break;
            case 33745: // Lacerate (Rank 1)
            case 48567: // Lacerate (Rank 2)
            case 48568: // Lacerate (Rank 3)
                spellInfo->Effects[EFFECT_1].Mechanic = MECHANIC_BLEED;
                break;
            case 54878: // Gundrak - Colossus - Merge
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 45907: // Midsummer - Torch Target Picker
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 45671: // Midsummer - Juggle Torch (Catch, Quest)
                spellInfo->AttributesEx3 &= ~SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
            case 65418: // Pilgrims Bounty - Food
            case 65419:
            case 65420:
            case 65421:
            case 65422:
            case 66478:
                spellInfo->Effects[EFFECT_0].BasePoints = 3;
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_OBS_MOD_HEALTH;
                spellInfo->Effects[EFFECT_0].Amplitude = 1000;
                break;
            case 66041: // Pilgrims Bounty - Drink
                spellInfo->Effects[EFFECT_0].BasePoints = 3;
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_OBS_MOD_POWER;
                spellInfo->Effects[EFFECT_0].Amplitude = 1000;
                break;
            case 16261: // Improved Shields (Rank 1)
            case 16290: // Improved Shields (Rank 2)
            case 51881: // Improved Shields (Rank 3)
                // bonus for earth shield is applied in script
                spellInfo->Effects[EFFECT_1].SpellClassMask &= ~flag96(0x0, 0x00000400, 0x0);
                break;
            case 48503: // Living Seed
            case 61607: // Mark of Blood
            case 52759: // Ancestral Awakening
                // causes handling als delayed spells, so OnHit effect gets applyed after dealing the damage/effect
                spellInfo->AttributesEx4 |= SPELL_ATTR4_UNK4;
                break;
            case 14157: // Ruthlessness
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                spellInfo->AttributesEx4 |= SPELL_ATTR4_UNK4;
                break;
            case 59407: // Shadows in the Dark
                spellInfo->AttributesEx4 |= SPELL_ATTR4_NOT_STEALABLE;
                break;
            case 50978: // Colossal Strike
                spellInfo->Effects[EFFECT_2].BasePoints *= 2.5;
                break;
            case 38358: // SSC - Tidal Surge
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_FORCE_CAST;
                spellInfo->Effects[EFFECT_1].TriggerSpell = 38357;
                break;
            case 37713: // SSC - Leotheras Demonic alligment (Event Buff)
                spellInfo->Effects[EFFECT_1].BasePoints = 150;
                spellInfo->Effects[EFFECT_2].BasePoints = 150;
                break;
            case 29293: // Poison Bolt Volley (Event Buff)
                spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_25_YARDS);
                spellInfo->Effects[EFFECT_2].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_25_YARDS);
                break;
            case 23310: // Timelapse (Event Buff)
                spellInfo->Effects[EFFECT_1].BasePoints = -12;
                break;
            default:
                break;
        }

        /// ICC Hardmode buff
        switch (spellInfo->Id)
        {
            // Lord Marrowgar
        case 70835: // Bone Storm
        case 70836:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.25;
            break;
        case 70825: // Iceflame (25 hm)
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.18;
            break;
            // Lady Deathwisper
        case 72501: // Frostbolt
        case 72502:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.15;
            break;
        case 72907: // Frostboltvolley
        case 72908:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.20;
            break;
        case 72493: // Shadowcleave
        case 72494:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.20;
            break;
            // Gunship Battle
            // Deathbringer Saurfang
            // Festergut
        case 70136: // Geaseous Blight
        case 70137:
        case 70139:
        case 70140:
        case 70469:
        case 70470:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.20;
            break;
        case 73031: // Purgend Blight
        case 73032:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.33;
            break;
            // Rotface
        case 73189: // Slime Spray (10 man)
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.23;
            break;
        case 73190: // Slime Spray (10 man)
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.12;
            break;
        case 73029: // Unstable OOze Explosion
        case 73030:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.33;
            break;
        case 73027: // Radiating Ooze
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.23;
            break;
        case 73025: // Weak Radiating Ooze
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.20;
            break;
            // Professor Putricide
        case 72868: // Slime Puddle
        case 72869:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.25;
            break;
        case 72621: // Choking Gasbomb Explosion
        case 72622:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.25;
            break;
        case 72873: // Malleble Goo
        case 72874:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.25;
            break;
            // Blood Prince Council
        case 72805: // Shadow Lance
        case 72806:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.20;
            break;
        case 72810: // Empowered Shadow Lance
        case 72811:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.15;
            break;
        case 72786: // Empowered Flame Periodic
        case 72787:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.10;
            break;
        case 72797: // Glittering Sparks
        case 72798:
            spellInfo->Effects[EFFECT_1].BasePoints *= 1.20;
            break;
        case 72801: // Kinetic Bomb
        case 72802:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.20;
            break;
        case 73038: // Empowered Shock Vortex
        case 73039:
        case 72816:
        case 72817:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.20;
            break;
        case 72813: // Shock Vortex
        case 72814:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.20;
            break;
        case 72999: // Shadow Prison (Damage)
            // not possible, single id/script
            break;
            // Bloodqueen Lana'Thel
        case 71700: // Shroud of Sorrow
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.11;
            break;
        case 72636: // Swarming Shadows
        case 72637:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.20;
            break;
        case 71479: // Bloodbolt Whirl
        case 71480:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.10;
            break;
            // Valithria
        case 71940: // Twisted Nightmare
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.1;
            spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
            spellInfo->AttributesEx6 |= SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS;
            break;
            // Sindragosa
        case 70106: // Penetrating Chill
            // not possible, single id
            break;
        case 71048: // Blistering Cold
        case 71049:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.20;
            break;
        case 71054: // Frost Bomb
        case 71055:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.20;
            break;
            // Lich King
        case 73789: // Pain and Suffering
        case 73790:
            spellInfo->Effects[EFFECT_1].BasePoints *= 1.18;
            spellInfo->Effects[EFFECT_2].BasePoints *= 1.25;
            break;
        case 73655: // Harvest Souls
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.25;
            break;
        case 73792: // Remorseless Winter
        case 73793:
            spellInfo->Effects[EFFECT_0].BasePoints *= 1.10;
            break;
        }
        /// ICC Hardmode buff
        /// Ruby HC Buff
        switch (spellInfo->Id)
        {
            case 75951:
            case 75952:
                spellInfo->Effects[EFFECT_0].BasePoints *= 1.10;
                break;
        }
        /// Ruby HC Buff

        switch (spellInfo->SpellFamilyName)
        {
            case SPELLFAMILY_PALADIN:
                // Seals of the Pure should affect Seal of Righteousness
                if (spellInfo->SpellIconID == 25 && spellInfo->HasAttribute(SPELL_ATTR0_PASSIVE))
                    spellInfo->Effects[EFFECT_0].SpellClassMask[1] |= 0x20000000;
                break;
            case SPELLFAMILY_DEATHKNIGHT:
                // Icy Touch - extend FamilyFlags (unused value) for Sigil of the Frozen Conscience to use
                if (spellInfo->SpellIconID == 2721 && spellInfo->SpellFamilyFlags[0] & 0x2)
                    spellInfo->SpellFamilyFlags[0] |= 0x40;
                break;
            case SPELLFAMILY_PRIEST:
                // Mind Flay
                if (spellInfo->SpellFamilyFlags[0] & 0x00800000)
                    spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
        }
    }

    if (SummonPropertiesEntry* properties = const_cast<SummonPropertiesEntry*>(sSummonPropertiesStore.LookupEntry(121)))
        properties->Type = SUMMON_TYPE_TOTEM;
    if (SummonPropertiesEntry* properties = const_cast<SummonPropertiesEntry*>(sSummonPropertiesStore.LookupEntry(647))) // 52893
        properties->Type = SUMMON_TYPE_TOTEM;

    TC_LOG_INFO("server.loading", ">> Loaded SpellInfo corrections in %u ms", GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellDelays()
{
    uint32 oldMSTime = getMSTime();

    // reload case
    SpellInfo* spellInfo = NULL;
    for (uint32 i = 0; i < GetSpellInfoStoreSize(); ++i)
    {
        spellInfo = mSpellInfoMap[i];
        if (!spellInfo)
            continue;
        spellInfo->Delay = 0;
    }

    //                                                0      1      2
    QueryResult result = WorldDatabase.Query("SELECT type, value, delay FROM spell_delay");

    if (!result)
    {
        TC_LOG_ERROR("sql.sql", ">> Loaded 0 spell delays. DB table `spell_delay` is empty.");
        return;
    }

    std::map<uint8, std::list<SpellDelayInfo> > delayInfos;

    uint32 count = 0;
    do
    {
        Field* field = result->Fetch();

        SpellDelayInfo info;
        uint8 type = field[0].GetUInt8();    // 0: spellId, 1: Effect, 2: Aura, 3: MechanicMask
        info.value = field[1].GetInt32();
        info.delay = field[2].GetUInt32();


        if (type > SPELL_DELAY_MAX_TYPES)
        {
            TC_LOG_ERROR("sql.sql", "Spell delay type %u is invalid (value=%i, delay=%u)", type, info.value, info.delay);
            continue;
        }

        if (type == SPELL_DELAY_TYPE_SPELLID && !mSpellInfoMap[info.value])
        {
            TC_LOG_ERROR("sql.sql", "Spell %i doesn't exists (type=%u, delay=%u)", info.value, type, info.delay);
            continue;
        }

        if (type == SPELL_DELAY_TYPE_AURA_NAME && (info.value <= SPELL_AURA_NONE || info.value >= TOTAL_AURAS))
        {
            TC_LOG_ERROR("sql.sql", "There is no Aura with name %i (type=%u, delay=%u)", info.value, type, info.delay);
            continue;
        }

        if (type == SPELL_DELAY_TYPE_MECHANIC && info.value == MECHANIC_NONE)
        {
            TC_LOG_ERROR("sql.sql", "Spell Delay type %u must have a mechanic valid mechanic mask", type);
            continue;
        }

        if (!info.value)
        {
            TC_LOG_ERROR("sql.sql", "Null values aren't allowed (type=%u)", type);
            continue;
        }


        delayInfos[type].push_back(info);
        ++count;
    } while (result->NextRow());

    typedef std::list<SpellDelayInfo>::const_iterator DelayItr;

    uint32 affecting = 0;

    std::list<SpellDelayInfo>& infos = delayInfos[SPELL_DELAY_TYPE_MECHANIC];
    for (uint32 i = 0; i < GetSpellInfoStoreSize(); ++i)
    {
        spellInfo = mSpellInfoMap[i];
        if (!spellInfo)
            continue;

        for (DelayItr itr = infos.begin(); itr != infos.end(); ++itr)
            if (spellInfo->Mechanic == static_cast<uint32>(itr->value))
            {
                spellInfo->Delay = itr->delay;
                ++affecting;
            }
    }

    infos = delayInfos[SPELL_DELAY_TYPE_AURA_NAME];
    for (uint32 i = 0; i < GetSpellInfoStoreSize(); ++i)
    {
        spellInfo = mSpellInfoMap[i];
        if (!spellInfo)
            continue;

        for (DelayItr itr = infos.begin(); itr != infos.end(); ++itr)
        {
            for (uint8 eff = EFFECT_0; eff <= EFFECT_2; ++eff)
                if (spellInfo->Effects[eff].ApplyAuraName == static_cast<uint32>(itr->value))
                {
                    spellInfo->Delay = itr->delay;
                    ++affecting;
                }
        }
    }

    infos = delayInfos[SPELL_DELAY_TYPE_SPELLID];
    for (DelayItr itr = infos.begin(); itr != infos.end(); ++itr)
    {
        if ((spellInfo = mSpellInfoMap[itr->value]))
        {
            spellInfo->Delay = itr->delay;
            ++affecting;
        }
    }


    TC_LOG_INFO("server.loading", ">> Loaded %u spell delays affecting %u spells in %u ms", count, affecting, GetMSTimeDiffToNow(oldMSTime));
}
