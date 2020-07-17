#pragma once

#include "Mail.h"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <unordered_map>
#include <shared_mutex>
#include <SynchronizedPtr.hpp>

class GAME_API MailMgr
{
    using Mutex = std::shared_timed_mutex;
    using ReadLock = std::shared_lock<Mutex>;
    using WriteLock = std::unique_lock<Mutex>;

    public:
        struct id_tag {};
        struct reciever_tag {};

        using Store = boost::multi_index::multi_index_container<
            Mail,
            boost::multi_index::indexed_by<
                boost::multi_index::hashed_unique<
                    boost::multi_index::tag<id_tag>,
                    boost::multi_index::member<Mail, uint32, &Mail::messageID>
                >,
                boost::multi_index::hashed_non_unique<
                    boost::multi_index::tag<reciever_tag>,
                    boost::multi_index::member<Mail, ObjectGuid, &Mail::receiver>
                >
            >
        >;
        
        class PlayerMailIterator
        {
            using iterator = Store::index<reciever_tag>::type::iterator;

            iterator _itr;
            iterator _end;
            ReadLock _lock;

            public:
                PlayerMailIterator(iterator end) : _itr(end), _end(end), _lock() {}
                PlayerMailIterator(iterator itr, iterator end, ReadLock&& lock) : _itr(itr), _end(end), _lock(std::move(lock)) {}

                PlayerMailIterator& operator++()
                {
                    ObjectGuid receiver = _itr->receiver;
                    _itr++;
                    if (_itr != _end && receiver != _itr->receiver)
                        _itr = _end;
                    return *this;
                }

                bool operator==(const PlayerMailIterator& right) { return _itr == right._itr; }
                bool operator!=(const PlayerMailIterator& right) { return _itr != right._itr; }
                const Mail* operator*() { return &(*_itr); }
        };

        using PlayerMailEntry = util::synchronized_ptr<const Mail, ReadLock>;

        static PlayerMailEntry Get(uint32 mailId);
        static uint8 GetMailCount(ObjectGuid player);

        static void Init();

        static void Insert(Mail mail);

        static bool MarkAsRead(uint32 mailId, ObjectGuid playerGuid);
        static bool MarkAsCopied(uint32 mailId, ObjectGuid playerGuid);
        static bool ReturnToSender(uint32 mailId, ObjectGuid playerGuid, uint32 accountId);
        static bool TakeItem(uint32 mailId, ObjectGuid itemId, Player& player);
        static bool TakeMoney(uint32 mailId, Player& player);
        static bool Delete(uint32 mailId);

        static void GetPlayerMailData(ObjectGuid playerGuid, uint8& unreadMailCount, time_t& mailDeliveryTime);

        static void ReturnOrDeleteOldMails();

    private:

        static MailMgr::Store::iterator ReturnToSender(MailMgr::Store::iterator itr, ObjectGuid playerGuid, uint32 accountId, std::unique_ptr<WriteLock> optionalLock = nullptr);

        static bool Update(uint32 mailId, std::function<void(Mail&)> updater);

        template<typename iterator>
        static iterator Delete(iterator mailItr, SQLTransaction trans = nullptr);

        static Store storage;

        static Mutex mutex;
    public:
        static PlayerMailIterator GetPlayerMailBegin(ObjectGuid playerGuid) { return PlayerMailIterator(storage.get<reciever_tag>().find(playerGuid), storage.get<reciever_tag>().end(), ReadLock(mutex)); }
        static PlayerMailIterator GetPlayerMailEnd() { return PlayerMailIterator(storage.get<reciever_tag>().end()); }
};
