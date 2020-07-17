#include "nexus.h"

constexpr SpellAbility SPELL_ENSNARE = { 48053, 5s };

constexpr SpellId SPELL_SUMMON_SEED_POD = 52796;
constexpr SpellId SPELL_SEED_POD = 48082;
constexpr SpellId SPELL_AURA_OF_REGENERATION = 52067;
constexpr SpellId SPELL_CRYSTAL_BLOOM = 48058;

constexpr CreatureEntry NPC_WILTED_FRAYER = 29911;


/*************************
* Crystalline Frayer
* NPC #26793
**************************/

CREATURE_SCRIPT(Crystalline_Frayer)
{
    me <<= On::Init |= BeginBlock
        += Do::MakeUnkillable
        += Do::AddAbility(SPELL_ENSNARE)
    EndBlock;

    me <<= On::EnterCombat
        |= Do::Cast(SPELL_CRYSTAL_BLOOM);

    me <<= On::FatalDamage |= BeginBlock
        += Exec::After(0.25s) |= Do::StopActiveMovement
        += Exec::After(1s) |= BeginBlock
            += Do::RemoveFromFight
            += Do::MakeEvade
            += Do::PushState
            += Do::MakeUnselectable
            += Do::MakePassive
            += Do::StopMovement
            += Exec::UntilSuccess
                |= !If::IsMoving
                    |= Do::Cast::Multi(
                        SPELL_SUMMON_SEED_POD,
                        SPELL_SEED_POD,
                        SPELL_AURA_OF_REGENERATION)
            += Exec::After(90s) |= BeginBlock
                += Do::PopState
                += Do::Talk(0)
                += Fire::Evade
            EndBlock
        EndBlock
    EndBlock;

    me <<= On::InitWorld
        |= On::KillPlants |= BeginBlock
            += Do::Die 
            += Do::MorphCreatureEntry(NPC_WILTED_FRAYER)
            += Do::OverrideCorpseDecay(24h)
        EndBlock;
}

/*************************
* Mageslayer 
* NPC #26730
**************************/

constexpr SpellAbility ABILITY_DRAW_MAGIC = {50131, {9s, 12s}, {11s, 20s}};
constexpr SpellAbility ABILITY_SPELL_LOCK = {30849, {2s, 8s}, {20s, 30s} };

CREATURE_SCRIPT(Mage_Slayer)
{
    me <<= On::Init
        |= Do::AddAbility::Multi(
            ABILITY_DRAW_MAGIC,
            ABILITY_SPELL_LOCK
        );
}


SPELL_SCRIPT(spell_lock)
{
    //me <<= On::Init // @todo: implement
    //    |= Do::MakeDefaultTargetRandom;
    
    me <<= Respond::ValidTargetForSpell
        |= *If::Casting.ToParam();
}

/*************************
* Mage Hunter Ascendant
* NPC #26727
**************************/

constexpr SpellAbility ABILITY_ARCANE_HASTE = { 50182, 2s, {65s, 80s}};

constexpr SpellId MAGE_HUNTER_SPELLS[3][3] =
{
    // frost bolt, freezing trap, cone of cold
    { 12737, 55040, 15244 },
    // fireball, immolation trap, rain of fire
    { 12466, 47784, 36808 },
    // arcane explosion, arcane trap, polymorph
    { 34933, 47789, 13323 }
};

constexpr SpellId SPELL_LEVITATE = 50195;

CREATURE_SCRIPT(Mage_Hunter_Ascendant)
{
    const auto spells = MAGE_HUNTER_SPELLS[urand(0, 2)];

    const SpellAbility ABILITY_NORMAL = { spells[0], {3s, 4s}, {3s, 5s}};
    const SpellAbility ABILITY_TRAP = { spells[1], {15s, 30s}, {16s, 20s}};
    const SpellAbility ABILITY_AOE = { spells[2], { 12s, 20s },{ 10s, 20s } };

    me <<= On::Init
        |= Do::AddAbility::Multi(
            ABILITY_ARCANE_HASTE,
            ABILITY_NORMAL,
            ABILITY_TRAP,
            ABILITY_AOE);

    me <<= On::SpellCast(ABILITY_AOE).Swapped()
        |= Respond::ValidTargetForSpell
            |= Select::SpellCaster 
            |= ~If::PrimaryTargetOf.Not().ToParam();

    me <<= On::EnterCombat
        |= Do::RemoveAuraBySpellId(SPELL_LEVITATE);
}


/*************************
* Crazed Mana Wyrm
* NPC #26761
**************************/

constexpr CreatureEntry NPC_CRAZED_MANA_WYRM = 26761;

CREATURE_SCRIPT(Crazed_Mana_Wyrm)
{
    me <<= On::Init
        |= Do::MakePassive;
}

/*************************
* Azur Skyrazor
* NPC #26736
**************************/

constexpr SpellId SPELL_ARCANE_BOLT = 47959;

SCRIPT_QUERY(TargetedWyrm, Creature, Creature*) { return nullptr; }

CREATURE_SCRIPT(Azure_Skyrazor)
{
    me <<= On::Noticed
        |= *(If::IsCreature(NPC_CRAZED_MANA_WYRM) && If::Alive)
            |= Exec::Once
                |= Respond::TargetedWyrm |= Get::Me;

    me <<= On::InitWorld
        |= Exec::Every(3s) 
            |= Query::TargetedWyrm 
                |= Do::CastOn(SPELL_ARCANE_BOLT);
}

/*************************
* Crazed Mana Wraith
* NPC #26746
**************************/

constexpr SpellAbility ABILITY_ARCANE_MISSLES = { 33833, {3s, 5s} };

CREATURE_SCRIPT(Crazed_Mana_Wraith)
{
    me <<= On::Init
        |= Do::AddAbility(ABILITY_ARCANE_MISSLES);

    me <<= On::SummonedBy |= BeginBlock
        += Do::MakeMoveIdleRandom(15.0f)
        += Respond::DamageTakenFromMultiplier
            |= *If::IsNPC.ToParam()
                |= Get::EigtherOr<float>({0.25f, 1.0f})
        += Respond::DamageDealtToMultiplier
            |= *If::IsNPC.ToParam()
                |= Get::EigtherOr<float>({0.1f, 1.0f})
    EndBlock;
}

SCRIPT_FILE(nexus_trash)
