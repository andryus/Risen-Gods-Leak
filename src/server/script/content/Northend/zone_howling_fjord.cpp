#include "ScriptInclude.h"

/*******************************
* Chill Nymph
* NPC #23678
*******************************/

constexpr SpellAbility ABILITY_WRATH = { 9739, 2s, 5s };

constexpr SpellId SPELL_SUMMON_SCARLET_BUD = 42388;
constexpr SpellId SPELL_LURIELLES_PENDANT = 43340;
constexpr SpellId SPELL_FJORD_SPRITE_SPELL_REDEMTION = 43341;

constexpr CreatureEntry NPC_SCARLET_IVY = 23763;

constexpr TextLine SAY_WEAKEND = 0;
constexpr TextLine SAY_FREED = 1;

SCRIPT_EVENT(RespawnIvy, Creature)

CREATURE_SCRIPT(Chill_Nymph)
{
    me <<= On::Init |= BeginBlock
        += Do::MakeMoveIdleRandom(20.0f)
        += Do::AddAbility(ABILITY_WRATH)
    EndBlock;

    me <<= On::InitWorld
        |= Exec::After({2s, 5s}) |= &Fire::RespawnIvy;

    me <<= On::SummonCreature(NPC_SCARLET_IVY) |= BeginBlock
        += *On::Death 
            |= Exec::After(5min) |= Fire::RespawnIvy
        += Util::ForwardTo % On::RespawnIvy
    EndBlock;

    me <<= On::RespawnIvy
        |= Exec::UntilSuccess
        |= !If::InCombat && If::Alive
            |= Do::Cast(SPELL_SUMMON_SCARLET_BUD);

    me <<= On::HealthReducedBelow(30) 
        |= Do::Talk(SAY_WEAKEND);

    me <<= On::HitBySpell(SPELL_LURIELLES_PENDANT)
        |= !If::IsOfFaction(Faction::FRIENDLY_AH) |= Util::ReplaceParam(Select::SpellCaster) |=  BeginBlock
            += Do::CastOn(SPELL_FJORD_SPRITE_SPELL_REDEMTION)
            += Do::RemoveFromFight
            += Exec::StateEvent(On::Respawn) |= BeginBlock
                += Do::ChangeFaction(Faction::FRIENDLY_AH)
                += Exec::After(2s) |= BeginBlock
                    += Do::TalkTo(SAY_FREED)
                    += Exec::After(2s) |= BeginBlock
                        += Do::MoveRandomOffset(50.0f)
                        += Exec::After(6s)
                            |= Do::DisappearAndDie(0s)
                    EndBlock
                EndBlock
            EndBlock
        EndBlock;
}

CREATURE_SCRIPT(Scarlet_Ivy)
{
    me <<= On::SummonedBy
        |= Do::MoveFollow(5.0f);

    me <<= On::UnInitWorld 
        |= If::Alive |= Fire::RespawnIvy;
};

/*******************************
* Lurielles Pendant
* Spell #49624
*******************************/

SPELL_SCRIPT(Lurielles_Pendant)
{
    me <<= Respond::ValidTargetForSpell.Demoted()
        |= (!If::IsOfFaction(Faction::FRIENDLY_AH) && If::HealthBelowPct(30)).ToParam();
}

/*******************************
* Proto-Drake
* NPC #23689
* Quest #11280
*******************************/

constexpr SpellAbility FLAME_BREATH = { 51219, 8s, {10s, 20s}};
constexpr SpellAbility WING_BUFFET = { 41572, 6s, {20s, 30s}};

constexpr SpellId FLAMES_OF_BIRTH = 42360;
constexpr SpellId SPELL_DRACONIS_GASTRITIS_KILL_CREDIT = 43174;
constexpr SpellId SPELL_UPSET_STOMACH = 43176;
constexpr SpellId SPELL_PROTO_DRAKE_VOMIT_EFFECT = 43177;

constexpr GameObjectEntry GO_TILLINHASTS_PLAGUED_MEAT = 186598;
// This is a linked trap - it should spawn automatically when GO_TILLINHASTS_PLAGUED_MEAT is spawned - needs fix in gameobject class
constexpr GameObjectEntry GO_AURA_TRAP_GREEN_SHORT = 186326;

constexpr CreatureEntry NPC_DRACONIS_GASTRITIS_BUNNY = 24170;
constexpr CreatureEntry NPC_PROTO_DRAKE = 23689;
constexpr CreatureEntry NPC_PROTO_WHELP_HATCHLING = 23750;

SCRIPT_QUERY(HatchlingTarget, Creature, Creature*) { return nullptr; }

CREATURE_SCRIPT(Proto_Drake) 
{
    me <<= On::Init |= BeginBlock
        += Do::AddAbility(FLAME_BREATH)
        += Do::AddAbility(WING_BUFFET)
    EndBlock;

    // Proto Drakes should cast 'FLAMES_OF_BIRTH' 
    //me <<= On::Noticed |= ~(If::IsCreature(NPC_PROTO_WHELP_HATCHLING)
    //    |= *Respond::HatchlingTarget 
    //        |= &Util::ReturnParam);

    //me <<= On::InitWorld
    //    |= Exec::Every(10s) |= If::IsAttackable
    //        |= Query::HatchlingTarget |= *If::WithinRange(100.0f) |= BeginBlock
    //            += Do::StopMovement
    //            += Do::CastOn(FLAMES_OF_BIRTH)
    //            += Exec::After(30s)
    //                |= Do::MoveHome
    //        EndBlock;
}

/*******************************
* Draconis-Gastritis-Bunny
* NPC #24170
* Quest #11280
*******************************/

SCRIPT_EVENT(DrakeReset, Creature)

CREATURE_SCRIPT(Draconis_Gastritis_Bunny) 
{
    me <<= On::Init
        |= Do::OverrideSightRange(200.0f);
    
    me <<= On::InitWorld
        |= Get::Location
        |= Do::SummonGO::Multi(GO_TILLINHASTS_PLAGUED_MEAT, GO_AURA_TRAP_GREEN_SHORT)
            |= *Do::MakeNonInteractible;

    me <<= On::Noticed
        |= *If::IsCreature(NPC_PROTO_DRAKE) && *!If::HasAura(SPELL_UPSET_STOMACH)
            |= Exec::Once
            |= ~(Exec::TempState(2min) |= BeginBlock
                += Do::MakeUnattackable
                += Exec::MoveFollow(0) 
                    |= Exec::After(3s) |= BeginBlock
                        += Do::Cast(SPELL_PROTO_DRAKE_VOMIT_EFFECT)
                        += Do::Cast(SPELL_UPSET_STOMACH)
                        += *Select::Owner 
                            |= Do::Cast(SPELL_DRACONIS_GASTRITIS_KILL_CREDIT)
                        += Exec::After(2s) |= BeginBlock
                            += Do::MakeAttackable
                            += *Do::DespawnCreature
                        EndBlock
                    EndBlock
            EndBlock);
}

SCRIPT_FILE(zone_howling_fjord)

/*******************************
* "Mad" Jonah Sterling 
* NPC #24742
* Quest #11471
*******************************/

constexpr SpellAbility WILDLY_FLAILING =
    { 50188, {15s, 30s} };

constexpr Position JUMP_START_POS = { -35.7038f, -3425.64f, 5.223f };
constexpr JumpPosition JUMP_TARGET_POS = { { -40.9998f, -3417.85f, -14.9212f }, 20.0f, 20.0f };

constexpr CreatureEntry NPC_HOZZER = 24547;
constexpr Position HOZZER_SUMMON_POS = { -46.2467f, -3392.34f, -9.33044f, 3.47581f };
constexpr SpellId SPELL_HOZZER_FEEDS = 44458;

constexpr TextLine SAY_0 = 0;
constexpr TextLine SAY_1 = 1;

SCRIPT_EVENT_PARAM(HozzerStart, Creature, Creature*, stirling)
SCRIPT_QUERY(HozzerAlive, Area, Creature*) { return nullptr; }
SCRIPT_EVENT(ResetHozzer, Creature)

CREATURE_SCRIPT(jonah_sterling)
{
    me <<= On::Init |= BeginBlock
        += Do::MakeUnkillable
        += Do::AddAbility(WILDLY_FLAILING)
        += Do::MakeMoveIdleRandom(5.0f)
    EndBlock;

    me <<= On::InitWorld
        |= Do::SummonCreature(NPC_HOZZER).Bind(HOZZER_SUMMON_POS)
            |= Util::ForwardTo |= On::HozzerStart;

    me <<= On::Respawn
        |= Exec::After(1ms) |= Query::HozzerAlive |= BeginBlock
            += If::ParamValid |= *Fire::ResetHozzer
            += !If::ParamValid |= Fire::InitWorld
    EndBlock;

    me <<= On::HealthReducedBelow(30) |= BeginBlock
        += Do::RemoveFromFight
        += Exec::StateEvent(On::Respawn) |= BeginBlock
            += Do::MakePassive
            += Do::MakeUnattackable
            += Do::DisableHealthReg
            += Exec::After(1s) |= BeginBlock
                += Do::Talk(SAY_0)
                += Exec::MovePos(JUMP_START_POS) |= BeginBlock
                    += &Fire::HozzerStart
                    += Exec::MoveJump(JUMP_TARGET_POS)
                        |= Do::StopMovement
                EndBlock
            EndBlock
        EndBlock
    EndBlock;

    me <<= On::HitBySpell(SPELL_HOZZER_FEEDS) |= BeginBlock
        += Do::Talk(SAY_1)
        += Exec::After(2s) |= Do::DisappearAndDie(2s)
    EndBlock;
         
}

/*******************************
* Hozzer
* NPC #24547
* Quest #11471
*******************************/

CREATURE_SCRIPT(hozzer) 
{
    me <<= On::InitWorld
        |= Respond::HozzerAlive |= If::Alive |= Get::Me;

    me <<= On::SummonedBy
        |= *On::Respawn |= If::Dead |= Do::DespawnCreature;

    me <<= On::ResetHozzer
        |= Exec::MovePos(HOZZER_SUMMON_POS)
            |= Do::PopState;

    me <<= On::HozzerStart |= BeginBlock
        += Do::PushState
        += Do::StandUp
        += Exec::MovePath(false) |= BeginBlock
            += Do::MakePosToHome
            += Do::FaceTo
            += Do::CastOn(SPELL_HOZZER_FEEDS)
            += Do::Talk(SAY_0)
            += Exec::After(2s)
                |= Exec::StateEvent(On::ResetHozzer) |= Do::MakeAttackable
        EndBlock
    EndBlock;
}
