/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITY_MAIL_H
#define TRINITY_MAIL_H

#include "Common.h"
#include "Transaction.h"
#include <map>
#include <vector>

#include "Item.h"
#include "ObjectGuid.h"

struct AuctionEntry;
struct CalendarEvent;
class Item;
class Object;
class Player;

#define MAIL_BODY_ITEM_TEMPLATE 8383                        // - plain letter, A Dusty Unsent Letter: 889
#define MAX_MAIL_ITEMS 12

enum MailMessageType
{
    MAIL_NORMAL         = 0,
    MAIL_AUCTION        = 2,
    MAIL_CREATURE       = 3,                                // client send CMSG_CREATURE_QUERY on this mailmessagetype
    MAIL_GAMEOBJECT     = 4,                                // client send CMSG_GAMEOBJECT_QUERY on this mailmessagetype
    MAIL_CALENDAR       = 5
};

enum MailCheckMask
{
    MAIL_CHECK_MASK_NONE        = 0x00,
    MAIL_CHECK_MASK_READ        = 0x01,
    MAIL_CHECK_MASK_RETURNED    = 0x02,                     /// This mail was returned. Do not allow returning mail back again.
    MAIL_CHECK_MASK_COPIED      = 0x04,                     /// This mail was copied. Do not allow making a copy of items in mail.
    MAIL_CHECK_MASK_COD_PAYMENT = 0x08,
    MAIL_CHECK_MASK_HAS_BODY    = 0x10                      /// This mail has body text.
};

// gathered from Stationery.dbc
enum MailStationery
{
    MAIL_STATIONERY_TEST    = 1,
    MAIL_STATIONERY_DEFAULT = 41,
    MAIL_STATIONERY_GM      = 61,
    MAIL_STATIONERY_AUCTION = 62,
    MAIL_STATIONERY_VAL     = 64,                           // Valentine
    MAIL_STATIONERY_CHR     = 65,                           // Christmas
    MAIL_STATIONERY_ORP     = 67                            // Orphan
};

enum MailShowFlags
{
    MAIL_SHOW_UNK0    = 0x0001,
    MAIL_SHOW_DELETE  = 0x0002,                             // forced show delete button instead return button
    MAIL_SHOW_AUCTION = 0x0004,                             // from old comment
    MAIL_SHOW_UNK2    = 0x0008,                             // unknown, COD will be shown even without that flag
    MAIL_SHOW_RETURN  = 0x0010
};

enum MailLevelRewardExtendedFlags
{
    MAIL_REWARD_SEND_AS_GM  = 0x0001                        // Send the mail as a GM
};

class MailSender
{
    public:                                                 // Constructors
        MailSender(MailMessageType messageType, uint32 sender_lowguid_or_entry, MailStationery stationery = MAIL_STATIONERY_DEFAULT)
            : m_messageType(messageType), m_senderId(sender_lowguid_or_entry), m_stationery(stationery)
        {
        }
        MailSender(Object* sender, MailStationery stationery = MAIL_STATIONERY_DEFAULT);
        MailSender(CalendarEvent* sender);
        MailSender(AuctionEntry* sender);
        MailSender(Player* sender);
    public:                                                 // Accessors
        MailMessageType GetMailMessageType() const { return m_messageType; }
        uint32 GetSenderId() const { return m_senderId; }
        MailStationery GetStationery() const { return m_stationery; }
    private:
        MailMessageType m_messageType;
        uint32 m_senderId;                                  // player low guid or other object entry
        MailStationery m_stationery;
};

class GAME_API MailReceiver
{
    public:                                                 // Constructors
        explicit MailReceiver(ObjectGuid receiver_guid) : m_receiver(NULL), m_receiver_guid(receiver_guid) { }
        MailReceiver(Player* receiver);
        MailReceiver(Player* receiver, ObjectGuid receiver_guid);
    public:                                                 // Accessors
        Player* GetPlayer() const { return m_receiver; }
        ObjectGuid  GetPlayerGUID() const { return m_receiver_guid; }
    private:
        Player* m_receiver;
        ObjectGuid  m_receiver_guid;
};

class GAME_API MailDraft
{
public:
    typedef std::map<uint32, Item*> MailItemMap;

    public:                                                 // Constructors
        explicit MailDraft(uint16 mailTemplateId, bool need_items = true)
            : m_mailTemplateId(mailTemplateId), m_mailTemplateItemsNeed(need_items), m_money(0), m_COD(0)
        { }
        MailDraft(std::string const& subject, std::string const& body)
            : m_mailTemplateId(0), m_mailTemplateItemsNeed(false), m_subject(subject), m_body(body), m_money(0), m_COD(0) { }
    public:                                                 // Accessors
        uint16 GetMailTemplateId() const { return m_mailTemplateId; }
        std::string const& GetSubject() const { return m_subject; }
        uint32 GetMoney() const { return m_money; }
        uint32 GetCOD() const { return m_COD; }
        std::string const& GetBody() const { return m_body; }

    public:                                                 // modifiers
        MailDraft& AddItem(Item* item);
        MailDraft& AddMoney(uint32 money) { m_money = money; return *this; }
        MailDraft& AddCOD(uint32 COD) { m_COD = COD; return *this; }

    public:                                                 // finishers
        void SendReturnToSender(uint32 sender_acc, ObjectGuid sender_guid, ObjectGuid receiver_guid, SQLTransaction& trans);
        void SendMailTo(SQLTransaction& trans, MailReceiver const& receiver, MailSender const& sender, MailCheckMask checked = MAIL_CHECK_MASK_NONE, uint32 deliver_delay = 0);

    private:
        void deleteIncludedItems(SQLTransaction& trans, bool inDB = false);
        void prepareItems(Player* receiver, SQLTransaction& trans);                // called from SendMailTo for generate mailTemplateBase items

        uint16      m_mailTemplateId;
        bool        m_mailTemplateItemsNeed;
        std::string m_subject;
        std::string m_body;

        MailItemMap m_items;                                // Keep the items in a map to avoid duplicate guids (which can happen), store only low part of guid

        uint32 m_money;
        uint32 m_COD;
};

struct Mail
{
    uint32 messageID;
    uint8 messageType;
    uint8 stationery;
    uint16 mailTemplateId;
    uint32 sender;
    ObjectGuid receiver;
    std::string subject;
    std::string body;
    std::vector<Item*> items;
    time_t expire_time;
    time_t deliver_time;
    uint32 money;
    uint32 COD;
    uint32 checked;

    void AddItem(Item* it)
    {
        items.push_back(it);
    }

    Item* RemoveItem(ObjectGuid item_guid)
    {
        for (auto itr = items.begin(); itr != items.end(); ++itr)
        {
            if ((*itr)->GetGUID() == item_guid)
            {
				Item* result = (*itr);
            	items.erase(itr);
                return result;
            }
        }
        return nullptr;
    }

    bool HasItems() const { return !items.empty(); }
};

#endif
