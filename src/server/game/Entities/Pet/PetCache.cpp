#include "Pet.h"
#include "PetCache.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "WorldPacket.h"
#include "util/meta/TypeChecks.hpp"
#include <chrono>

typedef std::chrono::system_clock Clock;

class PetLoadQueryHolder : public SQLQueryHolder
{
private:
    uint32 ownerId;
    uint32 petNumber;
public:
    PetLoadQueryHolder(uint32 ownerId, uint32 petNumber) : ownerId(ownerId), petNumber(petNumber) { }
    uint64 GetGuid() const { return petNumber; }
    bool Initialize();
};

bool PetLoadQueryHolder::Initialize()
{
    SetSize(MAX_PET_LOAD_QUERY);

    bool res = true;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_PET_BY_ENTRY);
    stmt->setUInt32(0, ownerId);
    stmt->setUInt32(1, petNumber);
    res &= SetPreparedQuery(PET_LOAD_QUERY_LOAD_FROM, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PET_AURA);
    stmt->setUInt32(0, petNumber);
    res &= SetPreparedQuery(PET_LOAD_QUERY_LOAD_AURAS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PET_SPELL);
    stmt->setUInt32(0, petNumber);
    res &= SetPreparedQuery(PET_LOAD_QUERY_LOAD_SPELLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PET_SPELL_COOLDOWN);
    stmt->setUInt32(0, petNumber);
    res &= SetPreparedQuery(PET_LOAD_QUERY_LOAD_SPELL_COOLDOWNS, stmt);

    return res;
}

SQLQueryHolder * PetCache::BuildPetLoadQuery(uint32 owner, uint32 petnumber)
{
    PetLoadQueryHolder *holder = new PetLoadQueryHolder(owner, petnumber);
    if (!holder->Initialize())
    {
        delete holder;
        return nullptr;
    }

    return static_cast<SQLQueryHolder*>(holder);
}

void PetCache::Update(/*const*/ Pet& pet, uint8 slot)
{
    time_t curTime = time(NULL);

    const auto assign = [&pet, &slot, &curTime](Entry& entry)
    {
        entry.petnumber = pet.GetCharmInfo()->GetPetNumber();
        entry.entry = pet.GetEntry();
        entry.owner = pet.GetOwnerGUID();
        entry.modelid = pet.GetNativeDisplayId();
        entry.createdBySpell = pet.GetUInt32Value(UNIT_CREATED_BY_SPELL);
        entry.petType = pet.getPetType();
        entry.level = pet.getLevel();
        entry.exp = pet.GetUInt32Value(UNIT_FIELD_PETEXPERIENCE);
        entry.reactState = uint8(pet.GetReactState());
        entry.slot = slot;
        entry.curhealth = pet.GetHealth();
        entry.curmana = pet.GetPower(POWER_MANA);
        entry.curhappiness = pet.GetPower(POWER_HAPPINESS);
        entry.renamed = !pet.HasByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED);
        entry.savetime = curTime;
        entry.name = pet.GetName();
        std::ostringstream ss;
        for (uint32 i = ACTION_BAR_INDEX_START; i < ACTION_BAR_INDEX_END; ++i)
        {
            ss << uint32(pet.GetCharmInfo()->GetActionBarEntry(i)->GetType()) << ' '
                << uint32(pet.GetCharmInfo()->GetActionBarEntry(i)->GetAction()) << ' ';
        };
        entry.abdata = ss.str();

        entry.auras.clear();
        entry.spells.clear();
        entry.cooldown.clear();

        for (const auto [spellId, aura] : pet.m_ownedAuras)
        {
            util::meta::Unreferenced(spellId);

            // check if the aura has to be saved
            if (!aura->CanBeSaved())
                continue;

            if(!pet.CanHaveAura(spellId))
                continue;

            int32 damage[MAX_SPELL_EFFECTS];
            int32 baseDamage[MAX_SPELL_EFFECTS];
            uint8 effMask = 0;
            uint8 recalculateMask = 0;
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                if (aura->GetEffect(i))
                {
                    baseDamage[i] = aura->GetEffect(i)->GetBaseAmount();
                    damage[i] = aura->GetEffect(i)->GetAmount();
                    effMask |= (1 << i);
                    if (aura->GetEffect(i)->CanBeRecalculated())
                        recalculateMask |= (1 << i);
                }
                else
                {
                    baseDamage[i] = 0;
                    damage[i] = 0;
                }
            }

            // don't save guid of caster in case we are caster of the spell - guid for pet is generated every pet load, so it won't match saved guid anyways
            ObjectGuid casterGUID = (aura->GetCasterGUID() == pet.GetGUID()) ? ObjectGuid::Empty : aura->GetCasterGUID();

            uint8 index = 0;

            AuraEntry auraEntry;

            auraEntry.caster_guid = casterGUID;
            auraEntry.spell = aura->GetId();
            auraEntry.effect_mask = effMask;
            auraEntry.recalculate_mask = recalculateMask;
            auraEntry.stackcount = aura->GetStackAmount();

            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                auraEntry.amount[i] = damage[i];
                auraEntry.base_amount[i] = baseDamage[i];
            }
            auraEntry.maxduration = aura->GetMaxDuration();
            auraEntry.remaintime = aura->GetDuration();
            auraEntry.remaincharges = aura->GetCharges();

            entry.auras.push_back(auraEntry);
        }

        for (PetSpellMap::iterator itr = pet.m_spells.begin(), next = pet.m_spells.begin(); itr != pet.m_spells.end(); itr = next)
        {
            ++next;

            if (itr->second.type == PETSPELL_FAMILY)
                continue;

            SpellEntry spellEntry;
            spellEntry.spell = itr->first;
            spellEntry.active = itr->second.active;

            entry.spells.push_back(spellEntry);
        }

        pet.GetSpellHistory()->ForAll([&](SpellHistory::CooldownStorageType::const_iterator itr) {
            if (itr->second.CooldownEnd > Clock::from_time_t(curTime))
            {
                SpellCooldownEntry cooldownEntry;
                cooldownEntry.spell = itr->first;
                cooldownEntry.time = Clock::to_time_t(itr->second.CooldownEnd);

                entry.cooldown.push_back(cooldownEntry);

                ++itr;
            }
        });
    };

    auto& index = cache_.get<petnumber_tag>();
    auto itr = index.find(pet.GetCharmInfo()->GetPetNumber());
    if (itr != index.end())
    {
        index.modify(itr, assign);
        WriteToDB(*itr);
    }
    else
    {
        Entry entry;
        assign(entry);
        index.insert(std::move(entry));
        WriteToDB(entry);
    }
}

void PetCache::Remove(uint32 petnumber)
{
    auto& index = cache_.get<petnumber_tag>();
    auto itr = index.find(petnumber);
    if (itr != index.end())
        index.erase(itr);
}

bool PetCache::LoadFromDB(SQLQueryHolder * holder)
{
    Entry entry;

    PreparedQueryResult result = holder->GetPreparedResult(PET_LOAD_QUERY_LOAD_FROM);
    if (!result)
        return false;

    Field* fields = result->Fetch();

    entry.petnumber = fields[0].GetUInt32();
    entry.entry = fields[1].GetUInt32();
    entry.owner = ObjectGuid(HighGuid::Player, fields[2].GetUInt32());
    entry.modelid = fields[3].GetUInt32();
    entry.createdBySpell = fields[15].GetUInt32();
    entry.petType = fields[16].GetUInt8();
    entry.level = fields[4].GetUInt32();
    entry.exp = fields[5].GetUInt32();
    entry.reactState = fields[6].GetUInt8();
    entry.name = fields[8].GetString();
    entry.renamed = fields[9].GetBool();
    entry.slot = fields[7].GetUInt8();
    entry.curhealth = fields[10].GetUInt32();
    entry.curmana = fields[11].GetUInt32();
    entry.curhappiness = fields[12].GetUInt32();
    entry.savetime = fields[14].GetUInt32();
    entry.abdata = fields[13].GetString();

    result = holder->GetPreparedResult(PET_LOAD_QUERY_LOAD_AURAS);
    if (result)
    {
        do
        {
            AuraEntry aura;
            fields = result->Fetch();
            aura.caster_guid = ObjectGuid(fields[0].GetUInt64());
            aura.spell = fields[1].GetUInt32();
            aura.effect_mask = fields[2].GetUInt8();
            aura.recalculate_mask = fields[3].GetUInt8();
            aura.stackcount = fields[4].GetUInt8();
            for (uint8 effIdx = 0; effIdx < MAX_SPELL_EFFECTS; effIdx++)
            {
                aura.amount[effIdx] = fields[5 + effIdx].GetInt32();
                aura.base_amount[effIdx] = fields[8 + effIdx].GetInt32();
            }
            aura.maxduration = fields[11].GetInt32();
            aura.remaincharges = fields[12].GetInt32();
            aura.remaincharges = fields[13].GetUInt8();
            entry.auras.push_back(aura);
        } while (result->NextRow());
    }

    result = holder->GetPreparedResult(PET_LOAD_QUERY_LOAD_SPELLS);
    if (result)
    {
        do
        {
            SpellEntry spell;
            fields = result->Fetch();
            spell.spell = fields[0].GetUInt32();
            spell.active = fields[1].GetUInt8();
            entry.spells.push_back(spell);
        } while (result->NextRow());
    }

    result = holder->GetPreparedResult(PET_LOAD_QUERY_LOAD_SPELL_COOLDOWNS);
    if (result)
    {
        do
        {
            SpellCooldownEntry cooldown;
            fields = result->Fetch();
            cooldown.spell = fields[0].GetUInt32();
            cooldown.time = time_t(fields[1].GetUInt32());
            entry.cooldown.push_back(cooldown);
        } while (result->NextRow());
    }

    cache_.insert(std::move(entry));
    return true;
}

void PetCache::WriteToDB(const Entry & petData)
{
    uint32 ownerLowGUID = petData.owner.GetCounter();
    std::string name = petData.name;
    CharacterDatabase.EscapeString(name);
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    // remove current data

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_PET_BY_ID);
    stmt->setUInt32(0, petData.petnumber);
    trans->Append(stmt);

    // save pet
    std::ostringstream ss;
    ss << "INSERT INTO character_pet (id, entry,  owner, modelid, level, exp, Reactstate, slot, name, renamed, curhealth, curmana, curhappiness, abdata, savetime, CreatedBySpell, PetType) "
        << "VALUES ("
        << petData.petnumber << ','
        << petData.entry << ','
        << ownerLowGUID << ','
        << petData.modelid << ','
        << uint32(petData.level) << ','
        << petData.exp << ','
        << uint32(petData.reactState) << ','
        << uint32(petData.slot) << ", '"
        << name.c_str() << "', "
        << uint32(petData.renamed ? 1 : 0) << ','
        << petData.curhealth << ','
        << petData.curmana << ','
        << petData.curhappiness << ", '"
        << petData.abdata  << "', "
        << petData.savetime << ','
        << petData.createdBySpell << ','
        << uint32(petData.petType) << ')';

    trans->Append(ss.str().c_str());
    CharacterDatabase.CommitTransaction(trans);
}

void AddStablePet(WorldPacket& data, const PetCache::Entry * petData)
{
    data << uint32(petData->petnumber);             // petnumber
    data << uint32(petData->entry);                 // creature entry
    data << uint32(petData->level);                 // level
    data << petData->name;                          // name
    // 1 = current, 2/3 = in stable (any from 4, 5, ... create problems with proper show)
    data << uint8((petData->slot == PET_SAVE_NOT_IN_SLOT || petData->slot == PET_SAVE_AS_CURRENT) ? 1 : 2);
}

uint8 PetCache::FillStableData(WorldPacket & data)
{
    uint8 cnt = 0;
    const PetCache::Entry* petData;
    petData = GetBySlot(PET_SAVE_AS_CURRENT);
    if (!petData)
        petData = GetBySlot(PET_SAVE_NOT_IN_SLOT);

    if (petData)
    {
        AddStablePet(data, petData);
        cnt++;
    }

    for (uint8 slot = PET_SAVE_FIRST_STABLE_SLOT; slot <= PET_SAVE_LAST_STABLE_SLOT; slot++)
    {
        petData = GetBySlot(slot);
        if (petData)
        {
            AddStablePet(data, petData);
            cnt++;
        }
    }

    return cnt;
}

uint8 PetCache::GetFirstFreeStableSlot()
{
    for (uint8 slot = PET_SAVE_FIRST_STABLE_SLOT; slot <= PET_SAVE_LAST_STABLE_SLOT; slot++)
        if (!GetBySlot(slot))
            return slot;
    return uint8(0);
}

void WriteStableSlotToDB(uint32 slot, uint32 petnumber, uint32 owner, SQLTransaction& trans)
{
    auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_PET_SLOT_BY_ID);
    stmt->setUInt8(0, uint8(slot));
    stmt->setUInt32(1, owner);
    stmt->setUInt32(2, petnumber);
    trans->Append(stmt);
}

bool PetCache::SwapStableSlots(uint8 slotA, uint8 slotB)
{
    const auto slotUpdate = [&slotA, &slotB](Entry& entry)
    {
        entry.slot = (entry.slot == slotA ? slotB : slotA);
    };

    auto& index = cache_.get<slot_tag>();
    auto itrA = index.find(slotA);
    auto itrB = index.find(slotB);

    if (itrA != index.end() || itrB != index.end())
    {
        if (itrA == index.end() && itrB != index.end())
        {
            itrA = itrB;
            itrB = index.end();
        }

        if (itrA->petType != HUNTER_PET)
            return false;

        if (itrB != index.end())
        {
            if (itrB->petType != HUNTER_PET)
                return false;
            index.modify(itrA, slotUpdate);
            index.modify(itrB, slotUpdate);

            SQLTransaction trans = CharacterDatabase.BeginTransaction();
            WriteStableSlotToDB(itrA->slot, itrA->petnumber, itrA->owner.GetCounter(), trans);
            WriteStableSlotToDB(itrB->slot, itrB->petnumber, itrB->owner.GetCounter(), trans);
            CharacterDatabase.CommitTransaction(trans);
        }
        else
        {
            index.modify(itrA, slotUpdate);

            SQLTransaction trans = CharacterDatabase.BeginTransaction();
            WriteStableSlotToDB(itrA->slot, itrA->petnumber, itrA->owner.GetCounter(), trans);
            CharacterDatabase.CommitTransaction(trans);
        }

        return true;
    }
    else
        return false;
}

bool PetCache::HasDeadHunterPet() const
{
    if (auto curpet = GetBySlot(PET_SAVE_NOT_IN_SLOT))
        return curpet->curhealth == 0;
    return false;
}

PetCache::PetCache() { }

const PetCache::Entry * PetCache::GetByPetNumber(uint32 petnumber) const
{
    const auto& index = cache_.get<petnumber_tag>();
    auto itr = index.find(petnumber);
    if (itr != index.end())
        return &(*itr);
    else
        return nullptr;
}

const PetCache::Entry * PetCache::GetBySlot(uint8 slot) const
{
    const auto& index = cache_.get<slot_tag>();
    auto itr = index.find(slot);
    if (itr != index.end())
        return &(*itr);
    else
        return nullptr;
}

const PetCache::Entry * PetCache::GetByEntry(uint32 entry) const
{
    const auto& index = cache_.get<entry_tag>();
    auto itr = index.find(entry);
    if (itr != index.end())
        return &(*itr);
    else
        return nullptr;
}
