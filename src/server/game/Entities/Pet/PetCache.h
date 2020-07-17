#pragma once

#include "DBCStructure.h"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>

enum PetLoadQueryIndex
{
    PET_LOAD_QUERY_LOAD_FROM                    = 0,
    PET_LOAD_QUERY_LOAD_AURAS                   = 1,
    PET_LOAD_QUERY_LOAD_SPELLS                  = 2,
    PET_LOAD_QUERY_LOAD_SPELL_COOLDOWNS         = 3,
    MAX_PET_LOAD_QUERY
};

class Pet;
class WorldPacket;

class GAME_API PetCache
{
public:
    struct AuraEntry
    {
        ObjectGuid caster_guid;
        uint32 spell;
        uint8 effect_mask;
        uint8 recalculate_mask;
        uint8 stackcount;
        int32 amount[MAX_SPELL_EFFECTS];
        int32 base_amount[MAX_SPELL_EFFECTS];
        int32 maxduration;
        int32 remaintime;
        uint8 remaincharges;
    };

    struct SpellEntry
    {
        uint32 spell;
        uint8 active;
    };

    struct SpellCooldownEntry
    {
        uint32 spell;
        time_t time;
    };

    struct Entry
    {
        uint32 petnumber;           // const
        uint32 entry;               // const
        ObjectGuid owner;           // const
        uint32 modelid;             // const
        uint32 createdBySpell; 
        uint8 petType;              // const
        uint32 level;
        uint32 exp;
        uint8 reactState;
        std::string name;
        bool renamed;
        uint8 slot;
        uint32 curhealth;
        uint32 curmana;
        uint32 curhappiness;
        uint32 savetime;
        std::string abdata;
        std::vector<AuraEntry> auras;
        std::vector<SpellEntry> spells;
        std::vector<SpellCooldownEntry> cooldown;
    };

private:
    struct petnumber_tag {};
    struct entry_tag {};
    struct slot_tag {};
    
    using Store = boost::multi_index::multi_index_container<
            Entry,
            boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<
                            boost::multi_index::tag<petnumber_tag>,
                            boost::multi_index::member<Entry, uint32, &Entry::petnumber>
                    >,
                    boost::multi_index::hashed_non_unique<
                            boost::multi_index::tag<slot_tag>,
                            boost::multi_index::member<Entry, uint8, &Entry::slot>
                    >,
                    boost::multi_index::hashed_non_unique<
                            boost::multi_index::tag<entry_tag>,
                            boost::multi_index::member<Entry, uint32, &Entry::entry>
                    >
            >
    >;

public:
    PetCache();
    const Entry* GetByPetNumber(uint32 petnumber) const;
    const Entry* GetBySlot(uint8 slot) const;
    const Entry* GetByEntry(uint32 slot) const;
    static SQLQueryHolder* BuildPetLoadQuery(uint32 owner, uint32 petnumber);

    uint8 FillStableData(WorldPacket& data);
    uint8 GetFirstFreeStableSlot();
    bool SwapStableSlots(uint8 slotA, uint8 slotB);
    bool HasDeadHunterPet() const;

    void Update(Pet& pet, uint8 slot);
    void Remove(uint32 petnumber);
    
    bool LoadFromDB(SQLQueryHolder *holder);
private:
    PetCache(const PetCache&) = delete;
    void WriteToDB(const Entry& petData);
    Store cache_;
};

