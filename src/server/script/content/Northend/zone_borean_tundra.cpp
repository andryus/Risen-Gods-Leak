#include "ScriptInclude.h"

/*************************************
* Glrglrglr
* NPC #28375
**************************************/
constexpr TextLine SAY_GLRGLRGLR_0 = 0;

constexpr Position GO_CAGE_POS = {4004.89f, 6389.44f, 29.6338f, -1.37881f};
constexpr GameObjectEntry GO_CAGE_ENTRY = {190567, 0.0f, 0.0f, -0.636078f, 0.771625f };

constexpr uint32 QUEST_KEYMASTER_URMGRGL = 11569;

SCRIPT_EVENT_PARAM(OpenCage, Unit, Unit*, unit)

CREATURE_SCRIPT(glrglrglr) 
{
    me <<= (On::InitWorld, On::Respawn)
        |= Do::SummonGO(GO_CAGE_ENTRY).Bind(GO_CAGE_POS)
           |= ~(*On::OpenCage |= Do::UseGO);

    me <<= On::QuestReward(QUEST_KEYMASTER_URMGRGL)
        |= Exec::StateEvent(On::PathEnded) |= BeginBlock
            += Do::MakeAlwaysActive
            += Do::DisablePlayerInteraction
            += Exec::After(2s) 
                |= &Fire::OpenCage
            += Exec::After(4s) 
                |= Exec::MovePath(false)
                    |= Do::DisappearAndDie(5s)
            += Exec::After(6s) 
                |= Do::Talk(SAY_GLRGLRGLR_0)
    EndBlock;
}


/*************************************
* General Purpose Bunny - Magmoth
* NPC #24021
**************************************/

constexpr CreatureEntry NPC_MAGMOTH_TOTEM = 25444;
constexpr CreatureEntry NPC_GENERAL_BUNNY = 24021;

SCRIPT_EVENT_PARAM(CastCosmeticFire, Creature, Creature*, trigger)

CREATURE_SCRIPT(Magmoth_Totem_Trigger)
{
    me <<= On::Init
        |= Do::OverrideSightRange(15.0f);

    me <<= On::Noticed
        |= ~(If::IsCreature(NPC_MAGMOTH_TOTEM) 
            |= Fire::CastCosmeticFire);
}

 
/*************************************
* Magmoth
* AREA #4321
**************************************/

AREA_SCRIPT(Magmoth)
{
    me <<= On::AddToArea
        |= *If::IsCreature(NPC_GENERAL_BUNNY)
            |= *Do::UseScript(Creatures::Magmoth_Totem_Trigger);
}

/*************************************
* Magmoth Fire Totem
* Totem #25444 
**************************************/

constexpr SpellId SPELL_COSMETIC_FIRE_BEAM = 45576;

TOTEM_SCRIPT(Magmoth_Fire_Totem)
{
    me <<= On::CastCosmeticFire
        |= Exec::Once 
            |= Do::CastOn(SPELL_COSMETIC_FIRE_BEAM);

    me <<= On::EnterCombat
        |= Do::Interrupt;
}

/*************************************
* Steam Rager #24601
* Windsoul Totem #25987
* Quest #11893
**************************************/

constexpr SpellAbility SPELL_STEAM_BLAST =
    { 50375, 8s }; // Todo: MinMax


constexpr SpellId WINDSOUL_TOTEM_AURA = 46374;
constexpr SpellId AIRY_SOUL = 46376;
constexpr SpellId WINDSOUL_CREDIT = 46378;

CREATURE_SCRIPT(steam_rager) 
{
    me <<= On::Init
        |= Do::AddAbility(SPELL_STEAM_BLAST);

    me <<= On::Death
        |= If::HasAura(WINDSOUL_TOTEM_AURA) 
            |= Do::Cast(AIRY_SOUL);
}

TOTEM_SCRIPT(windsoul_totem) 
{
    me <<= On::HitBySpell(AIRY_SOUL)
        |= Do::Cast(WINDSOUL_CREDIT);
}


/*************************************
* Winterfin Tadpole
* NPC #25201
* Quest #11560
**************************************/

constexpr SpellId SPELL_WINTERFIN_TADPOLE_CAGE_OPENED_PLAYER_CAST = 45279;

constexpr uint32 QUEST_OH_NOES_THE_TADPOLES = 11560;
constexpr CreatureEntry NPC_WINTERFIN_TADPOLE = 25201;

constexpr TextLine SAY_PLAYER_NOT_ON_QUEST = { 0, 30 };
constexpr TextLine SAY_PLAYER_ON_QUEST = { 1, 30 };

SCRIPT_ACTION1_INLINE(TadpoleFollow, Creature, Unit&, unit)
{
    me <<= Do::MoveFollow({ 1.0f, frand(1.0f, 5.0f) }).Bind(unit);
}

SCRIPT_EVENT(ResetTadpole, Creature)

CREATURE_SCRIPT(winterfin_tadpole)
{
    me <<= On::HitBySpell(SPELL_WINTERFIN_TADPOLE_CAGE_OPENED_PLAYER_CAST)
        |= *Get::SpellCaster
            |= Exec::StateEvent((On::Respawn, On::ResetTadpole)) |= Exec::Once |= BeginBlock
            += *If::DoingQuest(QUEST_OH_NOES_THE_TADPOLES) |= BeginBlock
                += Do::TalkTo(SAY_PLAYER_ON_QUEST)
                += Do::TadpoleFollow
                += ~Do::GiveNpcCreditTo
                += Exec::After(60s) // 60s
                    |= Do::DisappearAndDie(60s) //60s
            EndBlock
            += *!If::DoingQuest(QUEST_OH_NOES_THE_TADPOLES) |= BeginBlock
                += Do::TalkTo(SAY_PLAYER_NOT_ON_QUEST)
                += Exec::After(100ms) |= Fire::ResetTadpole
            EndBlock
        EndBlock;
}

/*************************************
* Cage
* GAMEOBJECT #187373
* Quest #11560
**************************************/

GO_SCRIPT(tadpole_cage)
{
    me <<= On::UsedByPlayer
        |= *!If::DoingQuest(QUEST_OH_NOES_THE_TADPOLES)
             |= Exec::After(2s) |= Do::MakeGOReady;
}


SCRIPT_FILE(zone_borean_tundra);
