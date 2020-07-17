#include "ScriptInclude.h"

/**************************
* Warlord Zol'Maz Stronghold
* AREA #4372
**************************/

constexpr GameObjectEntry GO_ZOLMAZ_GATE = 190784;

SCRIPT_QUERY(ZolMazGate, Area, GameObject*) { return nullptr; }

AREA_SCRIPT(zolmaz_stronghold) 
{
    me <<= On::AddToArea
        |= *(If::IsGO(GO_ZOLMAZ_GATE)
            |= Respond::ZolMazGate |= Get::Me);
}

/*************************************
* Warlord Zol'Maz
* NPC #28902
* Quest 12712
**************************************/

constexpr CreatureEntry NPC_WARLORD_ZOLMAZ = 28902;

constexpr SpellAbility SPELL_DECAPITATE = { 54670 , {10s, 12s}, {18s, 20s} };
constexpr SpellAbility SPELL_CHARGE = { 32323, {10s, 15s} };
constexpr SpellId SPELL_ENRAGE = 8599;
constexpr SpellId SPELL_RETALIATION = 40546;

constexpr TextLine SAY_ENRAGE = 0;
constexpr TextLine SAY_EVENTSTART = 1;
constexpr TextLine SAY_ATTACK = 2;

SCRIPT_QUERY(Warlord, Area, Creature*) { return nullptr; }

SCRIPT_EVENT_PARAM(StartZolMazEvent, Creature, Unit*, tikiowner)

CREATURE_SCRIPT(warlord_zol_maz)
{
    me <<= On::Init |= BeginBlock
        += Do::AddAbility(SPELL_DECAPITATE)
        += Do::AddAbility(SPELL_CHARGE)
        EndBlock;

    me <<= On::InitWorld
        |= Respond::Warlord |= Get::Me;

    me <<= On::StartZolMazEvent 
        |= Exec::StateEvent((On::Respawn, On::Evade)) |= BeginBlock
        += Exec::After(2s)
            |= Do::TalkTo(SAY_EVENTSTART)
        += Exec::After(5s) |= BeginBlock
            += Query::ZolMazGate |= ~Do::ActivateGO
            += Exec::After(2s) |= BeginBlock
                += Do::MakeAttackable
                += Do::AttackTarget
                EndBlock
            EndBlock
        EndBlock;

    me <<= On::HealthReducedBelow(35)
        |= Do::Cast(SPELL_RETALIATION);

    me <<= On::HealthReducedBelow(20) |= BeginBlock
            += Get::Victim |= Do::TalkTo(SAY_ENRAGE)
            += Do::Cast(SPELL_ENRAGE)
        EndBlock;
               
}

/*************************************
* Enchanted Tiki Dervish
* NPC #28927
* Quest 12712
**************************************/

CREATURE_SCRIPT(enchanted_tiki_dervish)
{
    me <<= On::InitWorld |= BeginBlock
        += Do::MorphCreatureToModel(25749)
        += Get::Owner |= Do::MoveFollow(1.0f)
        += Query::Warlord 
            |= Select::Owner |= ~Fire::StartZolMazEvent
        EndBlock;

    me <<= On::LeaveCombat
        |= Get::Owner |= Do::MoveFollow(1.0f);
}


SCRIPT_FILE(zone_zuldrak)