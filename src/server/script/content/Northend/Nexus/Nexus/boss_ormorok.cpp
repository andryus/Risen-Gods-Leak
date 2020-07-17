#include "nexus.h"
#include "Utilities/Span.h"

constexpr TextLine SayReflect = 3;
constexpr TextLine SayCrystalSpikes = 4;
constexpr TextLine SayFrenzy = 6;

constexpr EncounterTexts ORMOROK_TEXTS = { 1, 5, 2};

constexpr SpellAbility SpellOrmorokFrenzy = { 48017, SayFrenzy };
constexpr SpellAbility SpellOrmorokTrample = { 48016, 10s };
constexpr SpellAbility SpellOrmorokReflection = { 47981, 30s, SayReflect };
constexpr SpellAbility SpellOrmorokCrystalSpikes = { 47958, 12s, SayCrystalSpikes };
constexpr SpellAbility SpellOrmorokSummonCrystalizeTangler = { 61564, 17s, AbilityTarget::RANDOM };

/*************************************
* Ormorok
* ENCOUNTER
**************************************/

ENCOUNTER_SCRIPT(Nexus_Ormorok)
{
    me <<= On::EncounterFinished |= BeginBlock
        += Fire::KillPlants 
        += Fire::ActivateOrb(OrbType::Ormorok)
    EndBlock;
}

/*************************************
* Ormorok
* BOSS #26794
**************************************/

BOSS_SCRIPT(Ormorok)
{
    me <<= On::Init |= BeginBlock
        += Do::AddEncounterTexts(ORMOROK_TEXTS)
        += Do::AddAbility(SpellOrmorokTrample)
        += Do::AddAbility(SpellOrmorokReflection)
        += Do::AddAbility(SpellOrmorokCrystalSpikes)
        += If::InHeroicMode |=
               Do::AddAbility(SpellOrmorokSummonCrystalizeTangler)
    EndBlock;

    me <<= On::HealthReducedBelow(25)
        |= Do::ExecuteAbility(SpellOrmorokFrenzy);
}

/*************************************
* Crystaline Tangler
* NPC #32665 - HEROIC
**************************************/

CREATURE_SCRIPT(Crystalline_Tangler)
{
    me <<= On::Init
        |= Do::DisableMovement;
}

/************************************
*
* CRYSTAL SPIKES
*
************************************/

constexpr SpellId SpellCrystalSpikesDummy = 57083;

constexpr SpellId SpellCrystalSpikePrevisual = 50442;
constexpr SpellId SpellCrystalSpikeDamage = 47944;

constexpr SpellId summonSpikesAxis[] =
{
    47954, 47955, // front, left
    47956, 47957  // right, back
};

constexpr SpellId summonSpikesDiagonal[] =
{             
    57077, 57078, // frontL, frontR 
    57080, 57081  // backL, backR
};

/*************************************
* Crystal spikes
* SPELL #47958
**************************************/

SCRIPT_GETTER_INLINE(CrystalSpikeDirections, Unit, script::List<SpellId>)
{
    constexpr std::array<span<SpellId>, 2> axisToUse = { summonSpikesAxis, summonSpikesDiagonal };

    const uint8 whichAxis = urand(0, 1);
    const uint8 positionToDrop = urand(0, 3);

    script::List<SpellId> primaryAxisVector(axisToUse[whichAxis].begin(), axisToUse[whichAxis].end());
    script::List<SpellId> wildcards(axisToUse[1 - whichAxis].begin(), axisToUse[1 - whichAxis].end());

    auto itr = primaryAxisVector.begin() + positionToDrop;
    wildcards.push_back(*itr);
    primaryAxisVector.erase(itr);

    const uint8 wildcardToSelect = urand(0, 4);
    primaryAxisVector.push_back(wildcards[wildcardToSelect]);

    return primaryAxisVector;
};

SPELL_SCRIPT(Crystal_Spike)
{
    me <<= On::CastSuccess 
        |= Select::SpellCaster |= BeginBlock
            += Get::CrystalSpikeDirections.PutData<Do::Cast>()
            += Do::Cast(SpellCrystalSpikesDummy)
    EndBlock;
}

/*************************************
* Crystal Spike
* AURA #47941
**************************************/

SCRIPT_QUERY(SpikeCount, Unit, uint8) { return 8; }

constexpr SpellId SpikeSummonSpells[] =
{
    47936, // trigger back
    47942, // trigger back-left
    47943  // trigger back-right
};

const OneOfPair<span<SpellId>> SpikeSummonPair =
{
    make_span(SpikeSummonSpells, 1),
    make_span(SpikeSummonSpells)
};

AURA_SCRIPT(Crystal_Spike)
{
    me <<= On::EffectPeriodic(0)
        |= Select::EffectTarget
        |= Query::SpikeCount 
        |= If::LessEqual(7)
            |= If::LessEqual(0).ToParam()
            |= Do::Cast::OneOfFrom(Get::EigtherOr(SpikeSummonPair));
};

/****************************************
* Crystal Spike Initial Trigger
* NPC #27101
*****************************************/

constexpr SpellId SpellCrystalSpikeAura = 47941;

CREATURE_SCRIPT(crystal_spike_initial_trigger)
{
    me <<= On::SummonedBy |= BeginBlock
        += Do::FaceTo
        += Do::Cast(SpellCrystalSpikeAura)
        += Exec::After(15s) 
            |= Do::DespawnCreature
    EndBlock;

    me <<= Respond::SpikeCount
        |= Get::IncrementalInt;
}

/*************************************
* Crystal Spike Trigger
* NPC #27079
**************************************/

CREATURE_SCRIPT(crystal_spike_trigger)
{
    me <<= On::InitWorld |= BeginBlock
        += Do::Cast(SpellCrystalSpikeAura)
        += Exec::TempState(2200ms) // cancel aura early (due to visual bug; blizzlike confirmed)
            |= Do::Cast(SpellCrystalSpikePrevisual)
    EndBlock;

    me <<= On::SummonedBy
        |= Util::ForwardTo
            % Respond::SpikeCount;
};

/*************************************
* Crystal Spike
* NPC #27099
**************************************/

CREATURE_SCRIPT(crystal_spike)
{
    me <<= On::Init |= BeginBlock
        += Do::MakePassive
        += Do::DisableAutoAttack
    EndBlock;

    me <<= On::SummonedBy
        |= Do::Cast(SpellCrystalSpikeDamage);
}

/*************************************
* Crystal Spike
* GO #188537
**************************************/

GO_SCRIPT(Crystal_Spike)
{
    me <<= On::SummonedBy |= BeginBlock
        += Exec::After(2s) |= Do::UseGO
        += Exec::After(5s) |= Do::DespawnGO
    EndBlock;
}

SCRIPT_FILE(boss_ormorok)
