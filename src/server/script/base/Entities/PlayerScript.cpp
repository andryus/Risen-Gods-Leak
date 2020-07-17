#include "PlayerScript.h"
#include "Creature.h"
#include "Player.h"
#include "ObjectMgr.h"

SCRIPT_ACTION_IMPL(GiveNpcCreditTo)
{
    me.KilledMonster(npc.GetCreatureTemplate(), npc.GetGUID()); // todo: use npc directly
}

SCRIPT_ACTION_IMPL(GiveDummyCreditTo)
{
    me.KilledMonsterCredit(entry.entry);
}

SCRIPT_FILTER_IMPL(IsPlayer)
{
    return me.IsOfType<Player>();
}

constexpr QuestStatus mapQuestState(QuestState status)
{
    switch(status)
    {
    case QuestState::NOT_ACCEPTED:
        return QUEST_STATUS_NONE;
    case QuestState::ACCEPTED:
        return QUEST_STATUS_INCOMPLETE;
    case QuestState::COMPLETE:
        return QUEST_STATUS_COMPLETE;
    case QuestState::FAILED:
        return QUEST_STATUS_FAILED;
    case QuestState::REWARDED:
        return QUEST_STATUS_REWARDED;
    }

    return QUEST_STATUS_NONE;
}

constexpr QuestState mapQuestStatus(QuestStatus status)
{
    switch (status)
    {
    case QUEST_STATUS_NONE:
        return QuestState::NOT_ACCEPTED;
    case QUEST_STATUS_INCOMPLETE:
        return QuestState::ACCEPTED;
    case QUEST_STATUS_COMPLETE:
        return QuestState::COMPLETE;
    case QUEST_STATUS_FAILED:
        return QuestState::FAILED;
    case QUEST_STATUS_REWARDED:
        return QuestState::REWARDED;
    }

    return QuestState::NOT_ACCEPTED;
}

QuestState getQuestState(const Player& me, uint32 id)
{
    return mapQuestStatus(me.GetQuestStatus(id));
}

SCRIPT_FILTER_IMPL(QuestAtLeast)
{
    return (uint8)getQuestState(me, data.questId.id) >= (uint8)data.state;
}

SCRIPT_FILTER_IMPL(QuestAtMost)
{
    return (uint8)getQuestState(me, data.questId.id) <= (uint8)data.state;
}

SCRIPT_FILTER_IMPL(QuestIs)
{
    return getQuestState(me, data.questId.id) == data.state;
}

SCRIPT_FILTER_IMPL(DoingQuest) 
{
    return me <<= If::QuestIs({ questId, QuestState::ACCEPTED });
}

SCRIPT_FILTER_IMPL(HasQuest)
{
    return me <<= 
        (If::QuestAtLeast({ questId, QuestState::ACCEPTED }) &&
            If::QuestAtMost({questId, QuestState::COMPLETE}));
}

SCRIPT_FILTER_IMPL(QuestAvailableFor)
{ 
    if (const Quest* quest = sObjectMgr->GetQuestTemplate(questId.id))
        if (!me.CanTakeQuest(quest, false))
            return false;

    return me <<= If::QuestAtMost({ questId, QuestState::NOT_ACCEPTED });
}
