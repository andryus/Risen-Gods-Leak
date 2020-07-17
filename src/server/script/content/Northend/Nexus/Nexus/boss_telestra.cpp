#include "nexus.h"

constexpr EncounterTexts TELESTRA_TEXTS = { 0, 1, 2};

constexpr TextLine SAY_MERGE = 3;
constexpr TextLine SAY_SPLIT = 4;

constexpr SpellAbility ABILITY_FIRE_BOMB = { 47773, 1.0s, 2.5s };
constexpr SpellAbility ABILITY_GRAVITY_WELL = { 47756, 15s };
constexpr SpellAbility ABILITY_ICE_NOVA = { 47772, 45s };

constexpr SpellAbility ABILITY_TELESTRA_SUMMON = { 47710, 0s, SAY_SPLIT };
constexpr SpellAbility ABILITY_TELESTRA_BACK = { 47714, 0s, SAY_MERGE };
constexpr SpellId SPELL_TELEPORT_TELESTRA = 47754;

constexpr SpellId SPELLS_SUMMON_MAGI[] = { 47707, 47708, 47709 }; // fire  arcane  frost

/**********************************
* Split Personality
* Achievement #2150 / 7577
************************************/

constexpr AchievementCriteriaId ACHIEVEMENT_SPLIT_PERSONALITY = 7577;

SCRIPT_EVENT(SplitPersonalityFailed, Encounter)
SCRIPT_EVENT(CloneDied, Encounter)
SCRIPT_EVENT(ClonesDeafeated, Encounter)

ACHIEVEMENT_SCRIPT(Split_Personality)
{
    me <<= On::Init |= On::SplitPersonalityFailed
        |= Do::CriteriaFailed;
}

/**********************************
* Grandmagus Telestra
* BOSS #26731
************************************/

ENCOUNTER_SCRIPT(Telestra)
{
    me <<= On::EncounterStarted
        |= Do::StartAchievement(ACHIEVEMENT_SPLIT_PERSONALITY);
    me <<= On::EncounterFinished
        |= Fire::ActivateOrb(OrbType::Telestra);

    me <<= On::CloneDied 
        |= Exec::StateEvent(On::ClonesDeafeated) 
            |= Exec::After(5s)
               |= Fire::SplitPersonalityFailed;
}

SCRIPT_EVENT(Reappear, Creature)
SCRIPT_EVENT(BeginSplit, Creature)
SCRIPT_EVENT(CheckEndSplit, Creature)

BOSS_SCRIPT(Magus_Telestra)
{
    me <<= On::Init |= BeginBlock
        += Do::AddEncounterTexts(TELESTRA_TEXTS)
        += Do::AddAbility::Multi(
            ABILITY_FIRE_BOMB,
            ABILITY_GRAVITY_WELL,
            ABILITY_ICE_NOVA)
    EndBlock;

    // Split phase - 50% / 25% (in Heroic)
    me <<= On::HealthReducedBelow(50)
        |= Fire::BeginSplit;

    me <<= On::BeginSplit |= BeginBlock
        += Do::ChangeEquip(0)
        += Do::PushState
        += Do::DisableMovement
        += Do::DisableAbilities
        += Do::DisableAutoAttack
        += Do::PushState
        += Do::ExecuteAbility(ABILITY_TELESTRA_SUMMON)
    EndBlock;

    me <<= On::CheckEndSplit
        |= Exec::WhenCalled(3) |= BeginBlock
            += Fire::ClonesDeafeated
            += Exec::After(6.0s)
                |= Do::ExecuteAbility(ABILITY_TELESTRA_BACK)
    EndBlock;
}

/**********************************
* Telestra Spawns Back in
* SPELL #47714
************************************/

SPELL_SCRIPT(Telestra_Spawns_Back_In)
{
    me <<= On::CastSuccess |= Select::SpellTarget |= BeginBlock
        += Do::PopState
        += Exec::After(2s) |= BeginBlock
           += Do::PopState
           += Exec::Once
               |= If::InHeroicMode
               |= On::HealthReducedBelow(25)
                   |= Fire::BeginSplit
        EndBlock
    EndBlock;
}

/**********************************
* Summon Telestra Clones
* SPELL #47710
************************************/

SPELL_SCRIPT(Summon_Telestra_Clones)
{
    me <<= On::CastSuccess |= Select::SpellCaster |= BeginBlock
        += Do::Cast::Multi(SPELLS_SUMMON_MAGI)
        += Do::MakeUnselectable
        += Exec::After(2s) 
            |= Do::Cast(SPELL_TELEPORT_TELESTRA)
    EndBlock;
}

/**********************************
* Telestra Clones
* NPC #26928 #26929 #26930
************************************/

void initTelestraClone(script::ConstructionCtx<Creature> me, SpellAbility ability1, SpellAbility ability2, SpellId deathSpell, SpellId visual)
{
    me <<= On::Init |= BeginBlock
        += Do::MakeUnkillable
        += Do::AddAbility::Multi(ability1, ability2)
    EndBlock;

    me <<= On::AuraApplied(deathSpell)
        |= ~(*On::UnInit |= Do::DeleteAura);

    me <<= On::FatalDamage |= Get::Boss |= BeginBlock
        += Do::RemoveFromFight
        += Do::HideModel
        += Do::MakeUnselectable
        += Do::MakePassive
        += Do::MakeEvade
        += Do::RemoveAuraBySpellId(visual)
        += Do::CastOn(deathSpell)
        += Fire::CloneDied
        += *Fire::CheckEndSplit
    EndBlock;
}

// FIRE

constexpr SpellAbility ABILITY_SCORTCH = { 47723, 9s, {9.5s, 11.5s}};
constexpr SpellAbility ABILITY_FIRE_BLAST = { 47721, 3s,{ 8s, 9s } };
constexpr SpellId SPELL_VISUAL_FIRE = 47705;
constexpr SpellId SPELL_DEATH_FIRE = 47711;

CREATURE_SCRIPT(Telestra_Clone_Fire)
{
    initTelestraClone(me, ABILITY_SCORTCH, ABILITY_FIRE_BLAST, SPELL_DEATH_FIRE, SPELL_VISUAL_FIRE);
}

// ARCANE

constexpr SpellAbility ABILITY_TIME_STOP = {47736, {15s, 16s}};
constexpr SpellAbility ABILITY_CRITTER = {47731, {6s, 8s}, {10s, 12s}};
constexpr SpellId SPELL_VISUAL_ARCANE = 47704;
constexpr SpellId SPELL_DEATH_ARCANE = 47712;

CREATURE_SCRIPT(Telestra_Clone_Arcane)
{
    initTelestraClone(me, ABILITY_TIME_STOP, ABILITY_CRITTER, SPELL_DEATH_ARCANE, SPELL_VISUAL_ARCANE);
}

// FROST

constexpr SpellAbility ABILITY_ICE_BARB = { 47729, 3s, { 8s, 9s } };
constexpr SpellAbility ABILITY_BLIZZARD = { 47727, 9s,{ 15s, 16s } };
constexpr SpellId SPELL_VISUAL_FROST = 47706;
constexpr SpellId SPELL_DEATH_FROST = 47713;

CREATURE_SCRIPT(Telestra_Clone_Frost)
{
    initTelestraClone(me, ABILITY_ICE_BARB, ABILITY_BLIZZARD, SPELL_DEATH_FROST, SPELL_VISUAL_FROST);
}

/**********************************
* Gravity Well Effect
* SPELL #47764
************************************/

// for tracking movement phase
constexpr SpellId SPELL_GRAVITY_AURA_CUSTOM = 47763;

SCRIPT_GETTER1_INLINE(GravityTarget, Unit, Position, Position, target)
{
    const uint8 count = me <<= Get::NumAuraStacks(SPELL_GRAVITY_AURA_CUSTOM);
    const uint8 phase = count % 4;

    Position dir;
    if (phase == 1 || phase == 3)
    {
        dir = me <<= Get::ForwardDirection;
        if(phase == 3)
        {
            dir.m_positionX *= -1;
            dir.m_positionY *= -1;
        }
    }

    target.m_positionX -= dir.m_positionX * 10.0f;
    target.m_positionY -= dir.m_positionY * 10.0f;

    if (count)
        target.m_positionZ += 3.0f;

    return target;
}

SPELL_SCRIPT(Gravity_Well_Effect)
{
    me <<= On::HitTarget
        |= *Do::AddAura(SPELL_GRAVITY_AURA_CUSTOM);

    me <<= Respond::EffectPullTowardsDest(0)
       |= Select::SpellTarget |= Get::GravityTarget;
}

SCRIPT_FILE(boss_telestra)
