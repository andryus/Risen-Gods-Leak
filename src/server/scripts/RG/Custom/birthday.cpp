#include "ScriptMgr.h"
#include "Player.h"
#include "AchievementMgr.h"
#include "Mail.h"
#include "GameEventMgr.h"
#include "WorldSession.h"
#include "Language.h"
#include "DBCStores.h"

namespace Scripts { namespace RG { namespace Custom
{

class BirthdayReward : public PlayerScript
{
    enum
    {
        WORLDEVENT_9TH_BIRTHDAY = 114,
        ACHIEV_4TH_BIRTHDAY     = 2398,
        ITEM_RG_MARK            = 66819,
    };

public:
    BirthdayReward() : PlayerScript("rg_custom_birthdayreward") {}

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        if (!sGameEventMgr->IsActiveEvent(WORLDEVENT_9TH_BIRTHDAY))
            return;

        // only once
        if (!player->HasAchieved(ACHIEV_4TH_BIRTHDAY))
        {
            // complete achievement
            if (auto entry = sAchievementStore.LookupEntry(ACHIEV_4TH_BIRTHDAY))
                player->CompletedAchievement(entry);

            // send RG-Mark
            {
                SQLTransaction trans = CharacterDatabase.BeginTransaction();

                MailSender sender(MAIL_NORMAL, player->GetGUID().GetCounter(), MAIL_STATIONERY_GM);
                MailReceiver receiver(player, player->GetGUID());
                MailDraft draft(player->GetSession()->GetTrinityString(LANG_RG_BIRTHDAY_MAIL_TITLE),
                                player->GetSession()->GetTrinityString(LANG_RG_BIRTHDAY_MAIL_TEXT));

                if (Item* item = Item::CreateItem(ITEM_RG_MARK, 1, player))
                {
                    item->SaveToDB(trans);
                    draft.AddItem(item);
                }

                draft.SendMailTo(trans, receiver, sender);
                CharacterDatabase.CommitTransaction(trans);
            }
        }
    }
};

}}}

void AddSC_custom_birthday()
{
    new Scripts::RG::Custom::BirthdayReward();
}
