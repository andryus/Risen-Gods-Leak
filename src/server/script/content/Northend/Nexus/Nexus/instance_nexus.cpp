#include "ScriptInclude.h"
#include "nexus.h"

SCRIPT_EVENT(CheckOrbs, Instance)
SCRIPT_QUERY(HasActiveOrbs, Instance, bool) { return false; }

constexpr CreatureEntry NPC_ALLY_COMMANDER = 26796;
constexpr CreatureEntry NPC_ALLY_BERSERKER = 26800;


SCRIPT_GETTER_INLINE(CreatureFactionEntry, InstanceMap, CreatureEntry)
{
    constexpr CreatureEntry factionCreatures[2][3] =
    {
        { 26799, 26801, 26803 },
        { 26800, 26802, 26805 }
    };

    const uint8 activeFactionId = me <<= Get::ActiveFactionId;

    return factionCreatures[activeFactionId][urand(0, 2)];
}

SCRIPT_GETTER_INLINE(CommanderEntry, InstanceMap, CreatureEntry)
{
    constexpr CreatureEntry NPC_ENEMY_COMMANDER[2][2] =
    {
        { 27947, 26798 },
        { 27949, 26796 }
    };

    const bool isHeroic = me <<= If::InHeroicMode;
    const uint8 activeFactionId = me <<= Get::ActiveFactionId;

    return NPC_ENEMY_COMMANDER[activeFactionId][isHeroic];
}

INSTANCE_SCRIPT(Nexus)
{
    me <<= On::CheckOrbs
        |= Query::HasActiveOrbs |= !If::BoolTrue
           |= Fire::UnfreezeKeristrasza;

    me <<= On::AddToInstance.Demoted()
        |= If::IsCreature(NPC_ALLY_BERSERKER)
            |= Get::CreatureFactionEntry.PutData<Do::MorphCreatureEntry>();

    me <<= On::AddToInstance.Demoted()
        |= If::IsCreature(NPC_ALLY_COMMANDER) 
            |= Get::CommanderEntry.PutData<Do::MorphCreatureEntry>();
}


/***************************
* Keristraza Containment Spheres
* GOs #188526 - #188528
****************************/

constexpr CreatureEntry NPC_BREATH_TRIGGER = 27048;
constexpr SpellId SPELL_FROST_BREATH = 47842; // needs to be cast manually. aura from template won't look correctly

constexpr Position BREATH_POSITIONS[] =
{
    { 285.878f, -23.0428f, -0.959156f, 0.645296f },
    { 319.416f, 13.6349f, -0.959156f, 4.01426f },
    { 285.126f, 13.1338f, -0.959156f, 5.37038f },
};

SCRIPT_MACRO_DATA(Util, MakeSelectableAfterEncounter, ME, OrbType, orb)
{    
    ME <<= On::Init
        |= Do::SpawnCreature(NPC_BREATH_TRIGGER).Bind(BREATH_POSITIONS[(uint8)orb]).Swapped()
            |= Do::Cast(SPELL_FROST_BREATH)
            |= *On::OrbDeactived(orb)
                |= Do::DespawnCreature;

    ME <<= On::InitWorld |= BeginBlock
        += On::ActivateOrb(orb) 
            |= Exec::StateEvent(On::OrbDeactived(orb))
                |= Do::MakeInteractible
        += Exec::StateEvent(On::OrbDeactived(orb))
            |= Respond::HasActiveOrbs
                |= Get::Constant(true)
    EndBlock;

    ME <<= On::UsedByPlayer |= BeginBlock
        += Fire::OrbDeactived(orb)
        += Fire::CheckOrbs
    EndBlock;
}

// #188526
GO_SCRIPT(Telestra_Sphere)
{
    Util::MakeSelectableAfterEncounter(OrbType::Telestra) 
        |= me;
}

// #188527
GO_SCRIPT(Anomalus_Sphere)
{
    Util::MakeSelectableAfterEncounter(OrbType::Anomalus)
        |= me;
}

// #188528
GO_SCRIPT(Ormorok_Sphere)
{
    Util::MakeSelectableAfterEncounter(OrbType::Ormorok) 
        |= me;
}

/***************************
* Seed Pod
* GO #191016
****************************/

GO_SCRIPT(SeedPod)
{
   me <<= On::InitWorld
        |= On::KillPlants 
            |= Exec::UntilSuccess |= Do::ActivateGO;
}


SCRIPT_FILE(instance_nexus)
