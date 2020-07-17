#pragma once

#include "API/Provider/Provider.hpp"

#include "Entities/Object/ObjectGuid.h"
#include "Support/Ticket.hpp"

namespace api::providers
{

    class GAME_API Account : public Provider<AccountId>
    {
    public:
        using Provider<AccountId>::Provider;

        FutureResultType Execute(AccountId id, std::shared_ptr<const json> requestedValues, std::shared_ptr<const Request> request) const override;
    };

    class GAME_API Character : public Provider<ObjectGuid>
    {
    public:
        using Provider<ObjectGuid>::Provider;

        using Provider<ObjectGuid>::operator();
        FutureResultType operator()(ObjectGuid guid) const
        {
            return operator()(guid.IsEmpty() ? boost::none : boost::optional<ObjectGuid>(guid));
        }

        FutureResultType Execute(ObjectGuid id, std::shared_ptr<const json> requestedValues, std::shared_ptr<const Request> request) const override;
    };

    class GAME_API Guild : public Provider<uint32>
    {
    public:
        using Provider<uint32>::Provider;

        FutureResultType Execute(uint32 id, std::shared_ptr<const json> requestedValues, std::shared_ptr<const Request> request) const override;
    };

    class GAME_API Ticket : public Provider<support::Ticket::Id>
    {
    public:
        using Provider<support::Ticket::Id>::Provider;

        FutureResultType Execute(support::Ticket::Id id, std::shared_ptr<const json> requestedValues, std::shared_ptr<const Request> request) const override;
    };

}
