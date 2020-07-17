#include "ScriptInclude.h"

/*************************************
* Warlord Tartek
* NPC #28121
* Quest #12575
**************************************/

constexpr SpellAbility SPELL_HEROIC_STRIKE = { 29426, {5s, 8s}, {5s, 8s} };
constexpr SpellAbility SPELL_SWEEPING_STRIKES = { 35429, {7s, 15s}, {5s, 10s} };
constexpr SpellAbility SPELL_THUNDER_ARMOR = { 15572, {6s, 15s}, {6s, 15s} };

constexpr SpellId SPELL_RIDE_VEHICLE_HARDCODED = 46598;

constexpr CreatureEntry NPC_ZEPTEK_THE_DESTROYER = 28399;
constexpr CreatureEntry NPC_JALOOT = 28121;

constexpr TextLine SAY_SPAWN = 0;
constexpr TextLine SAY_ATTACK = 1;
constexpr TextLine SAY_JALOOT_TARTEK_DEATH = 5;

constexpr Position POS_DISMOUNT = { 6712.461f, 5136.462f, -19.3981f };
constexpr Position POS_FINAL = { 6707.71f, 5129.31f, -19.337f, 5.50f }; // Todo::SniffThisPosition

SCRIPT_EVENT(DismountEvent, Creature);
SCRIPT_QUERY(Jaloot, Creature, Creature*) { return nullptr; }

CREATURE_SCRIPT(warlord_tartek)
{
    me <<= On::Init |= BeginBlock
        += Do::AddAbility::Multi(SPELL_HEROIC_STRIKE, SPELL_SWEEPING_STRIKES, SPELL_THUNDER_ARMOR)
        += Do::MakeUnattackable
        EndBlock;

    me <<= On::InitWorld |= BeginBlock
        += Do::Talk(SAY_SPAWN)
        += Get::Location |= Do::SummonCreature(NPC_ZEPTEK_THE_DESTROYER) |= Do::CastOn(SPELL_RIDE_VEHICLE_HARDCODED)
        EndBlock;

    me <<= On::DismountEvent |= BeginBlock
        += Do::MovePos(POS_FINAL)
        += Exec::After(3s) |= BeginBlock
            += Do::MakeAttackable
            += Do::Talk(SAY_ATTACK)
            EndBlock
        EndBlock;

    me <<= On::Noticed |= ~(If::IsCreature(NPC_JALOOT)
        |= *Respond::Jaloot 
            |= &Util::ReturnParam);

    me <<= On::Death
        |= Query::Jaloot
            |= *Do::Talk(SAY_JALOOT_TARTEK_DEATH);
 
}

/*************************************
* Zeptek The Destroyer
* NPC #28399
* Quest #12575
**************************************/

CREATURE_SCRIPT(zeptek_the_destroyer) 
{
    me <<= On::Init
        |= Do::MakeUnattackable;

    me <<= On::InitWorld
        |= Exec::After(5s)
            |= Exec::MovePos(POS_DISMOUNT) |= BeginBlock
                += Do::RemoveAuraBySpellId(SPELL_RIDE_VEHICLE_HARDCODED)
                += Exec::After(3s) |= BeginBlock
                    += Do::MakeAttackable
                    += Get::Owner |= *Fire::DismountEvent
                EndBlock
            EndBlock;

    me <<= On::HitBySpell(SPELL_RIDE_VEHICLE_HARDCODED)
        |= Do::ChangeFaction(2061);
}

/*************************************
* Chicken Escapee
* NPC #28161
* Quest #12532 / #12702
**************************************/

constexpr SpellId SPELL_CAPTURE_CHICKEN_ESCAPEE = 51037;
constexpr SpellId SPELL_CAPTURED_CHICKEN_COVER = 51961;
constexpr SpellId SPELL_CHICKEN_LOCATION_PING = 51843;
constexpr SpellId SPELL_SCARED_CHICKEN = 51846;

SCRIPT_EVENT(ChickenDespawn, Creature);

CREATURE_SCRIPT(chicken_escapee) 
{
    me <<= On::HitBySpell(SPELL_CHICKEN_LOCATION_PING) 
        |= Do::Cast(SPELL_SCARED_CHICKEN);

    me <<= On::SpellClick |= BeginBlock
        += Do::RemoveAuraBySpellId(SPELL_SCARED_CHICKEN)
        += Do::StopActiveMovement
        += Do::CastOn(SPELL_CAPTURE_CHICKEN_ESCAPEE)
        EndBlock;

    me <<= On::ChickenDespawn
        |= Do::DisappearAndDie(120s);
      
}

SPELL_SCRIPT(capture_chicken_escapee)
{
    me <<= On::HitTarget
        |= Select::SpellCaster
            |= Fire::ChickenDespawn;
}


SCRIPT_FILE(zone_sholazar_basin);