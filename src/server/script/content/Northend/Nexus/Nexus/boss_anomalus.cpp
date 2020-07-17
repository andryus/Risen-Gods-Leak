#include "nexus.h"


constexpr TextLine SAY_AGGRO = 0;
constexpr TextLine SAY_DEATH = 1;
constexpr TextLine SAY_RIFT = 2;
constexpr TextLine SAY_SHIELD = 3;
constexpr TextLine SAY_SUMMON_RIFT = 4;
constexpr TextLine SAY_PROTECTED = 5;
constexpr TextLine SAY_KILL = 6;

constexpr SpellAbility ABILITY_SPARK = { 47751, 5s,{ 7s, 12s }, AbilityTarget::RANDOM };
constexpr SpellAbility ABILITY_ARCANE_ATTRACTION = { 57063, 15s, {12s, 15s}};
constexpr SpellAbility ABILITY_CREATE_RIFT = { 47743, 15s, 20s, SAY_RIFT };
constexpr SpellAbility ABILITY_RIFT_SHIELD = { 47748, SAY_SHIELD };
constexpr SpellAbility ABILITY_CHARGE_RIFT = { 47747, SAY_PROTECTED };

// for ending the rift-shield phase, we make use of query stacking
// as long as one rift is alive, LastRift will return "true"
SCRIPT_EVENT(RiftDestroyed, Encounter)
SCRIPT_QUERY(HasRift, Encounter, bool) { return false; }


/*******************************
* Chaos Theory
* Achievement #2037 / 7316
*******************************/

constexpr AchievementCriteriaId ACHIEVEMENT_CHAOS_THEORY = 7316;

ACHIEVEMENT_SCRIPT(Chaos_Theory)
{
    me <<= On::Init |= On::RiftDestroyed
        |= Do::CriteriaFailed;
}

/*******************************
* Anomalus
* Encounter
*******************************/

ENCOUNTER_SCRIPT(Anomalus)
{
    me <<= On::EncounterFinished
        |= Fire::ActivateOrb(OrbType::Anomalus);

    me <<= On::EncounterStarted
        |= Do::StartAchievement(ACHIEVEMENT_CHAOS_THEORY);
}

/*******************************
* Chaotic Rift (Anomalus)
* NPC #26918
*******************************/

constexpr SpellId SPELL_CHAOTIC_RIFT_AURA = 47733;
constexpr SpellId SPELL_CHAOTIC_RIFT_AURA_BOSS = 47742;

constexpr CreatureEntry NPC_CHAOTIC_RIFT = 26918;

CREATURE_SCRIPT(Anomalus_Rift)
{
    me <<= On::Death
        |= Fire::RiftDestroyed;

    me <<= On::InitWorld
        |= Exec::StateEvent(On::Death) 
            |= Respond::HasRift
                |= Get::Constant(true);

    me <<= On::ReceivedAura(ABILITY_CHARGE_RIFT)
        |= Do::Cast::Multi(
            SPELL_CHAOTIC_RIFT_AURA,
            SPELL_CHAOTIC_RIFT_AURA_BOSS);
}

/*******************************
* Anomalus
* Boss #26763
*******************************/

BOSS_SCRIPT(Anomalus)
{
    me <<= On::Init |= BeginBlock
        += Do::AddEncounterTexts({SAY_AGGRO, SAY_KILL, SAY_DEATH})
        += Do::AddAbility::Multi(
            ABILITY_SPARK,
            ABILITY_CREATE_RIFT)
        += If::InHeroicMode 
            |= Do::AddAbility(ABILITY_ARCANE_ATTRACTION)
    EndBlock;

    me <<= On::SummonCreature(NPC_CHAOTIC_RIFT) |= BeginBlock
        += Do::Talk(SAY_SUMMON_RIFT)
        += *Do::UseScript(Creatures::Anomalus_Rift)
    EndBlock;

    // RIFT SHIELD - PHASE2

    me <<= On::HealthReducedBelow(50)
        |= Do::PushStateEvent(On::LostAura(ABILITY_RIFT_SHIELD)) |= BeginBlock
            += Do::DisableAbilities
            += Do::DisableAutoAttack
            += Do::ExecuteAbility(ABILITY_CREATE_RIFT)
            += On::SummonCreature(NPC_CHAOTIC_RIFT)
                |= Do::ExecuteAbility(ABILITY_RIFT_SHIELD)
            += On::RiftDestroyed
               |= Exec::After(250ms)
                   |= Query::HasRift 
                   |= !If::BoolTrue
                       |= Do::RemoveAuraBySpellId(ABILITY_RIFT_SHIELD)
    EndBlock;

    me <<= On::ReceivedAura(ABILITY_RIFT_SHIELD)
        |= Exec::After(250ms) 
            |= Do::ExecuteAbility(ABILITY_CHARGE_RIFT);
    me <<= On::LostAura(ABILITY_RIFT_SHIELD)
        |= Do::Interrupt;
}

/*******************************
* Charge rift
* Spell #47747
*******************************/

SPELL_SCRIPT(Charge_Rift)
{
    me <<= Respond::ValidTargetForSpell
        |= *Get::Owner |= If::ParamValid.ToParam();
}

SCRIPT_FILE(boss_anomalus)
