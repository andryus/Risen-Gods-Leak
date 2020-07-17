#include "ScriptInclude.h"

/*******************************
* Magister Aledis
* NPC   #20159
* Quest #10286
*******************************/

constexpr TextLine SAY_COMBAT = 0;
constexpr TextLine SAY_END = 1;

constexpr SpellAbility ABILITY_PYROBLAST = { 33975, {18s, 21s}};
constexpr SpellAbility ABILITY_FROST_NOVA = { 11831,{12s, 16s}};
constexpr SpellAbility ABILITY_FIREBALL = { 20823, 1s, { 3s, 4s}};

constexpr FactionType ALEDIS_COMBAT_FACTION = 634;

constexpr QuestId QUEST_ARELIONS_SECRET = 10286;
constexpr QuestId QUEST_MISTRESS_REVEALED = 10287;

SCRIPT_EVENT_PARAM(StartAledisCombat, Creature, Player*, player)
SCRIPT_EVENT(EndAledisCombat, Creature)

CREATURE_SCRIPT(Magister_Aledis)
{
    me <<= On::Init |= BeginBlock
        += Do::RemoveQuestGiver
        += Do::AddAbility::Multi(
            ABILITY_PYROBLAST,
            ABILITY_FROST_NOVA,
            ABILITY_FIREBALL)
    EndBlock;

    me <<= On::Respawn |= BeginBlock // @todo: hack
        += Fire::ReachedHome
        += Do::RemoveQuestGiver
    EndBlock;

    me <<= On::GossipOption(0)
        |= Exec::StateEvent(On::ReachedHome) |= BeginBlock
            += Do::DisableGossip
            += Do::StopActiveMovement
            += Do::Dismount
            += Do::FaceTo
            += Exec::After(2s)
                |= Do::TalkTo(SAY_COMBAT)
            += Exec::After(6s)
                |= Fire::StartAledisCombat
    EndBlock;

    me <<= On::StartAledisCombat 
        |= Exec::StateEvent(On::LeaveCombat) |= BeginBlock
            += Do::ChangeFaction(ALEDIS_COMBAT_FACTION)
            += Do::MakeUnkillable
            += Do::EngageWith
            += On::HealthReducedBelow(30) |= Exec::UntilSuccess |= !If::Casting |= BeginBlock 
                += Do::RemoveFromFight
                += Fire::EndAledisCombat
            EndBlock
        EndBlock;

    me <<= On::EndAledisCombat |= BeginBlock
        += Do::Talk(SAY_END)
        += Exec::After(2s)
            |= Do::MakeQuestGiver
        += Exec::After(1min) |= BeginBlock
            += Do::DisappearAndDie(2min)
            += Do::RemoveQuestGiver
        EndBlock
    EndBlock;

    me <<= Respond::CanSeeGossipOption(0)
        |= (*If::HasQuest(QUEST_ARELIONS_SECRET)
            || *If::QuestAvailableFor(QUEST_MISTRESS_REVEALED)).ToParam();
}

SCRIPT_FILE(zone_hellfire_peninsula)
