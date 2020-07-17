#pragma once
#include "API/ScriptComposites.h"
#include "PlayerOwner.h"
#include "CreatureOwner.h"

/************************************
* ACTIONS
************************************/

SCRIPT_ACTION1(GiveNpcCreditTo, Player, Creature&, npc)
SCRIPT_ACTION_EX(GiveDummyCreditTo, Player, CreatureEntry, entry)

/************************************
* FILTER
************************************/

SCRIPT_FILTER(IsPlayer, WorldObject)

enum class QuestState : uint8
{
    NOT_ACCEPTED,
    ACCEPTED,
    COMPLETE,
    FAILED,
    REWARDED
};

struct QuestStateData
{
    QuestId questId;
    QuestState state;
};

SCRIPT_FILTER_EX(QuestAtLeast, Player, QuestStateData, data)
SCRIPT_FILTER_EX(QuestAtMost, Player, QuestStateData, data)
SCRIPT_FILTER_EX(QuestIs, Player, QuestStateData, data)

SCRIPT_FILTER_EX(DoingQuest, Player, QuestId, questId)
SCRIPT_FILTER_EX(HasQuest, Player, QuestId, questId)
SCRIPT_FILTER_EX(QuestAvailableFor, Player, QuestId, questId)

/************************************
* MODIFIER
************************************/