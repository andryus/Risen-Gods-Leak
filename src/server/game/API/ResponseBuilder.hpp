#pragma once

#include <memory>
#include <string_view>

#include <boost/optional.hpp>

#include "json/json.hpp"

#include "Accounts/RBAC.h"

#include "API/Request.hpp"

namespace api
{

    class ResponseBuilder
    {
    private:
        json response = json::object();
        const std::shared_ptr<const json> values{};
        const std::shared_ptr<const Request> request{};

    public:
        ResponseBuilder(const std::shared_ptr<const json> values, const std::shared_ptr<const Request> request) :
            values(std::move(values)),
            request(std::move(request))
        {}


        template<typename T>
        void set(const std::string& key, const T& value)
        {
            if (!values->has(key))
                return;

            response[key] = value;
        }

        template<typename T>
        void set(const std::string& key, T&& value)
        {
            if (!values->has(key))
                return;

            response[key] = std::move(value);
        }

        template<typename T>
        void set(const std::string& key, T&& value, rbac::RBACPermissions permission)
        {
            if (!request->HasPermission(permission))
                return;

            set(key, std::forward<T>(value));
        }

        template<typename T, typename ...Args>
        void setOptional(const std::string& key, boost::optional<T> opt, Args&&... args)
        {
            if (!opt)
                return;

#if COMPILER == COMPILER_MICROSOFT
            if (std::is_reference_v<T>) // otherwise: fatal error C1001: An internal error has occurred in the compiler.
#else
            if constexpr(std::is_reference_v<T>)
#endif
                set(key, *opt, std::forward<Args>(args)...);
            else
                set(key, std::move(*opt), std::forward<Args>(args)...); // move if the optional contains the value directly
        }

        template<typename FuncT>
        void setWith(const std::string& key, FuncT&& f)
        {
            if (values->has(key))
                response[key] = f();
        }

        template<typename FuncT>
        void setWith(const std::string& key, FuncT&& f, rbac::RBACPermissions permission)
        {
            if (!request->HasPermission(permission))
                return;

            if (values->has(key))
                response[key] = f();
        }

        json build()
        {
            return std::move(response);
        }
    };

}