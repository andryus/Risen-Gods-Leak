#include "TradeLogModule.hpp"

#include "Config.h"
#include "Player.h"
#include "Mail.h"

RG::Logging::TradeLogModule::TradeLogModule() :
    LogModule("Trade"),
    tradeLogNumber_(1),
    mailLogNumber_(1)
{ }

void RG::Logging::TradeLogModule::Initialize()
{
    if (QueryResult result = RGDatabase.Query("SELECT MAX(id) FROM log_trade"))
        tradeLogNumber_.store((*result)[0].GetUInt32() + 1);

    if (QueryResult result = RGDatabase.Query("SELECT MAX(id) FROM log_mail"))
        mailLogNumber_.store((*result)[0].GetUInt32() + 1);
}

void RG::Logging::TradeLogModule::LoadConfig()
{
    LogModule::LoadConfig();

    config.trade.enabled    = sConfigMgr->GetBoolDefault(GetOption("Trade.Enabled").c_str(), false);

    config.mail.enabled     = sConfigMgr->GetBoolDefault(GetOption("Mail.Enabled").c_str(), false);
    config.mail.player_only = sConfigMgr->GetBoolDefault(GetOption("Mail.PlayerOnly").c_str(), true);
}

void RG::Logging::TradeLogModule::Log(const Player* player, const TradeData* playerTrade, const Player* partner, const TradeData* partnerTrade)
{
    if (!IsEnabled() || !config.trade.enabled)
        return;

    SQLTransaction trans = RGDatabase.BeginTransaction();
    PreparedStatement* stmt;

    uint32 logId = tradeLogNumber_++;

    stmt = RGDatabase.GetPreparedStatement(RG_INS_TRADE);
    stmt->setUInt32(0, logId);
    stmt->setUInt32(1, player->GetGUID().GetCounter());
    stmt->setUInt32(2, partner->GetGUID().GetCounter());
    stmt->setUInt32(3, playerTrade->GetMoney());
    stmt->setUInt32(4, partnerTrade->GetMoney());

    trans->Append(stmt);

    for (uint8 i = 0; i < 2; ++i)
    {
        const Direction direction = i == 0 ? Direction::GIVE : Direction::GET;

        const TradeData* myData = direction == Direction::GIVE ? playerTrade : partnerTrade;
        const TradeData* hisData = direction == Direction::GIVE ? partnerTrade : playerTrade;

        if (uint32 spell = myData->GetSpell())
        {
            stmt = RGDatabase.GetPreparedStatement(RG_INS_TRADE_SPELL);
            stmt->setUInt32(0, logId);
            stmt->setUInt8(1, static_cast<uint8>(direction));
            stmt->setUInt32(2, spell);

            const Item* item = hisData->GetItem(TRADE_SLOT_NONTRADED);
            stmt->setUInt32(3, item->GetGUID().GetCounter());
            stmt->setUInt32(4, item->GetEntry());

            trans->Append(stmt);
        }

        for (uint8 slot = 0; slot < TRADE_SLOT_COUNT; ++slot)
        {
            if (Item* item = myData->GetItem(static_cast<TradeSlots>(slot)))
            {
                stmt = RGDatabase.GetPreparedStatement(RG_INS_TRADE_ITEM);
                stmt->setUInt32(0, logId);
                stmt->setUInt8(1, static_cast<uint8>(direction));
                stmt->setUInt8(2, slot);
                stmt->setUInt32(3, item->GetGUID().GetCounter());
                stmt->setUInt32(4, item->GetEntry());
                stmt->setUInt32(5, item->GetCount());

                trans->Append(stmt);
            }
        }
    }

    RGDatabase.CommitTransaction(trans);
}

void RG::Logging::TradeLogModule::Log(const MailDraft& draft, const MailReceiver& receiver, const MailSender& sender, const MailDraft::MailItemMap& items)
{
    if (!IsEnabled() || !config.mail.enabled)
        return;

    if (config.mail.player_only && sender.GetMailMessageType() != MAIL_NORMAL)
        return;

    SQLTransaction trans = RGDatabase.BeginTransaction();
    PreparedStatement* stmt;

    uint32 logId = mailLogNumber_++;

    stmt = RGDatabase.GetPreparedStatement(RG_INS_MAIL);
    stmt->setUInt32(0, logId);
    stmt->setUInt32(1, sender.GetSenderId());
    stmt->setUInt32(2, receiver.GetPlayerGUID().GetCounter());
    stmt->setString(3, draft.GetSubject());
    stmt->setUInt32(4, draft.GetMoney());
    stmt->setUInt32(5, draft.GetCOD());
    trans->Append(stmt);

    uint8 slot = 0;
    for (const auto& itemEntry : items)
    {
        const Item* item = itemEntry.second;

        stmt = RGDatabase.GetPreparedStatement(RG_INS_MAIL_ITEM);
        stmt->setUInt32(0, logId);
        stmt->setUInt8(1, slot++);
        stmt->setUInt32(2, item->GetGUID().GetCounter());
        stmt->setUInt32(3, item->GetEntry());
        stmt->setUInt32(4, item->GetCount());
        trans->Append(stmt);
    }

    RGDatabase.CommitTransaction(trans);
}
