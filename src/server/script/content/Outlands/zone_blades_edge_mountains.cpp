#include "ScriptInclude.h"

/*******************************
* Razaani Light Orb
* NPC   #20635
* Quest #10674 / #10859
*******************************/

constexpr CreatureEntry NPC_TRAPPING_THE_LIGHT_DUMMY_CREDIT = 21929;
constexpr GameObjectEntry GO_MULTI_SPECTRUM_LIGHT_TRAP = 185011;

constexpr SpellId SPELL_GATHER_THE_ORB_DUMMY = 38872;
constexpr SpellId SPELL_TRAPPING_THE_LIGHT_DUMMY = 37902;
constexpr SpellId SPELL_ARCANE_EXPLOSION_VISUAL = 35426;

SCRIPT_EVENT_PARAM(ReachedTotemOrTrapBunny, Creature, Unit*, target)

CREATURE_SCRIPT(razaani_light_orb)
{
    me <<= (On::HitBySpell(SPELL_TRAPPING_THE_LIGHT_DUMMY), On::HitBySpell(SPELL_GATHER_THE_ORB_DUMMY))
        |= *Get::SpellCaster
        |= Exec::StateEvent(On::Respawn)
        |= Exec::MoveNear({ 0, 0 }) |= BeginBlock
            += Do::StopMovement
            += Exec::After(3s) |= BeginBlock
                += Do::Cast(SPELL_ARCANE_EXPLOSION_VISUAL)
                += ~Fire::ReachedTotemOrTrapBunny
                += Exec::After(1s) |= Do::DespawnCreature
                EndBlock
        EndBlock;
}

/*******************************
* Multi-Spectrum Light Trap Bunny
* NPC   #21926
* Quest #10674
*******************************/

SCRIPT_QUERY(LightTrap, Creature, GameObject*) { return nullptr; }

CREATURE_SCRIPT(multi_spectrum_trap_bunny)
{
    me <<= On::InitWorld
        |= Get::Location
            |= Do::SummonGO(GO_MULTI_SPECTRUM_LIGHT_TRAP).Swapped()
                |= *Respond::LightTrap |= Get::Me;

    me <<= On::SummonGO |= BeginBlock
        += *Do::MakeNonInteractible
        += Exec::After(1s) |= ~Do::UseGO
        += Exec::After(2s) |= Do::Cast(SPELL_TRAPPING_THE_LIGHT_DUMMY)
        EndBlock;

    me <<= On::ReachedTotemOrTrapBunny |= BeginBlock
        += Exec::After(2s)
            |= Query::LightTrap |= *Do::MakeGOReady
        += Get::Me |= *Select::Owner
            |= Do::GiveDummyCreditTo(NPC_TRAPPING_THE_LIGHT_DUMMY_CREDIT)
        EndBlock;
}

/*******************************
* Orb-Collecting Totem
* Totem #22333
* Quest #10859
*******************************/

TOTEM_SCRIPT(orb_collecting_totem)
{
    me <<= On::ReachedTotemOrTrapBunny |= BeginBlock
        += Get::Me |= *Select::Owner
            |= Do::GiveDummyCreditTo(NPC_TRAPPING_THE_LIGHT_DUMMY_CREDIT)
        += Exec::After(1s)
            |= Do::DespawnCreature
        EndBlock;
}



SCRIPT_FILE(zone_blades_edge_mountains)