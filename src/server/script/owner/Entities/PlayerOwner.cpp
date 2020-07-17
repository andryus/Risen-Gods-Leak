#include "PlayerOwner.h"
#include "Player.h"
#include "ObjectMgr.h"

SCRIPT_PRINTER_IMPL(QuestId)
{
    std::string out = "Quest: ";
    out += script::print(value.id) + ", ";
    
    if (const Quest* quest = sObjectMgr->GetQuestTemplate(value.id))
        out += quest->GetTitle();
    else
        out += "<INVALID>";

    return out;
}

SCRIPT_OWNER_IMPL(Player, Unit)
{
    return base.To<Player>();
}
