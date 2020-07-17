#pragma once

#include <memory>
#include <string_view>

#include <boost/optional.hpp>

#include <folly/futures/Future.h>

#include <json/json.hpp>

#include "API/Request.hpp"

namespace api::providers
{

    template<typename ParameterT>
    class Provider
    {
    public:
        using ResultType = boost::optional<json>;
        using FutureResultType = folly::Future<ResultType>;

    private:
        const std::shared_ptr<const json> requestedValues{};
        const std::shared_ptr<const Request> request{};

    public:
        Provider(std::shared_ptr<const json> requestedValues, std::shared_ptr<const Request> request) :
            requestedValues(std::move(requestedValues)),
            request(std::move(request))
        {}
        virtual ~Provider() = default;

        FutureResultType operator()(boost::optional<ParameterT> arg) const
        {
            if (!requestedValues->is_object())
                return boost::none;

            if (!arg)
                return ResultType(json(nullptr));

            return Execute(*arg, requestedValues, request);
        }

        virtual FutureResultType Execute(ParameterT arg, std::shared_ptr<const json> requestedValues, std::shared_ptr<const Request> request) const = 0;
    };

    template<typename ProviderT>
    ProviderT MakeProvider(const std::shared_ptr<const json>& values, const std::string& key, std::shared_ptr<const Request> request)
    {
        const auto it = values->find(key);
        return ProviderT(
            (it == values->end()) ? std::make_shared<const json>(nullptr) : std::shared_ptr<const json>(values, &(*it)),
            std::move(request)
        );
    }

}
