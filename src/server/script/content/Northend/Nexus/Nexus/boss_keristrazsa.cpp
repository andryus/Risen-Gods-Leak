#include "nexus.h"

constexpr TextLine SAY_ENRAGE = 2;
constexpr TextLine SAY_CRYSTAL_NOVA = 3;
constexpr TextLine SAY_FRENZY = 5;

constexpr EncounterTexts ENCOUNTER_TEXT_KERISTRAZSA = { 0, 1, 3};

constexpr SpellAbility ABILITY_FIRE_BREATH = { 48096, 14s, 10s };
constexpr SpellAbility ABILITY_TAIL_SWEEP = { 50155, 5s};
constexpr SpellAbility ABILITY_CRYSTAL_CHAINS = { 50997, 10s, {25s, 30s} };
constexpr SpellAbility ABILITY_CRYSTALIZE = { 48179, 10s, { 25s, 30s }, SAY_CRYSTAL_NOVA };
constexpr SpellAbility ABILITY_ENRAGE = {8599, SAY_ENRAGE };

constexpr SpellId SPELL_FROZEN_PRISON = 47854;
constexpr SpellId SPELL_INTENSIVE_COLD = 48094;

/*******************************
* Intense Cold
* Achievemen #2036 / 7315
*******************************/

constexpr AchievementCriteriaId ACHIEVEMENT_INTENSE_COLD = 7315;

SCRIPT_EVENT(TooManyColdStacks, Encounter)

ACHIEVEMENT_SCRIPT(Intense_Cold)
{
    me <<= On::Init |= On::TooManyColdStacks
        |= Do::CriteriaFailed;
}

/*******************************
* Keristrasza
* Boss #26723
*******************************/

ENCOUNTER_SCRIPT(Keristraza)
{
    me <<= On::EncounterStarted
        |= Do::StartAchievement(ACHIEVEMENT_INTENSE_COLD);
}

BOSS_SCRIPT(Keristrasza)
{
    me <<= On::Init |= BeginBlock
        += Do::MakeDefensive
        += Do::AddEncounterTexts(ENCOUNTER_TEXT_KERISTRAZSA)
        += Do::AddAbility::Multi(
            ABILITY_FIRE_BREATH,
            ABILITY_TAIL_SWEEP)
        += If::InHeroicMode.ToParam()
            |= Get::EigtherOr<SpellAbility>({ 
                ABILITY_CRYSTALIZE,
                ABILITY_CRYSTAL_CHAINS
                 })
            .PutData<Do::AddAbility>()
    EndBlock;

    me <<= On::HealthReducedBelow(25) |= BeginBlock
        += Do::ExecuteAbility(ABILITY_ENRAGE)
        += Do::Talk(SAY_FRENZY)
    EndBlock;

    me <<= On::EnterCombat
        |= Do::Cast(SPELL_INTENSIVE_COLD);

    me <<= On::InitWorld |= On::UnfreezeKeristrasza |= BeginBlock
        += Do::MakeAttackable
        += Do::RemoveAuraBySpellId(SPELL_FROZEN_PRISON)
        += Exec::After(2s)
            |= Do::MakeAggressive
    EndBlock;
}

/*******************************
* Intensive Cold
* Aura #48095
*******************************/

AURA_SCRIPT(Intensive_Cold)
{
    me <<= On::GainedStack
        |= If::GreaterEqual(3)
            |= Fire::TooManyColdStacks;
}

SCRIPT_FILE(boss_keristrazsa)
