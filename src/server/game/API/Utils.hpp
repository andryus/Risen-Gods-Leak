#pragma once

#include <memory>
#include <sstream>

#include <boost/optional.hpp>

#include "json/json.hpp"

#include "API/Define.hpp"

namespace api::util
{

    inline std::shared_ptr<const json> ExtractRequestedValues(const std::shared_ptr<const Request>& request)
    {
        const json& params = request->parameters;

        const auto it = params.find("request");
        if (it == params.end())
            throw APIException("no values requested");

        if (!it->is_object())
            throw APIException("parameter <request> must be an object");

        return std::shared_ptr<const json>(request, &(*it));
    }

    inline void AssertType(const json& params, const std::string& parameter, json::value_t type)
    {
        const auto it = params.find(parameter);
        if (it == params.end())
        {
            std::stringstream message;
            message << "parameter <" << parameter << "> missing";
            throw api::InvalidArgumentException(message.str());
        }

        if (it->type() != type)
        {
            std::stringstream message;
            message << "parameter <" << parameter << "> must be of type \"" << json::type_name(type) << "\" but was \"" << it->type_name() << "\" instead";
            throw api::InvalidArgumentException(message.str());
        }
    }

    inline void AssertTypeOptional(const json& params, const std::string& parameter, json::value_t type)
    {
        const auto it = params.find(parameter);
        if (it == params.end())
            return; // missing is allowed

        if (it->type() != type)
        {
            std::stringstream message;
            message << "parameter <" << parameter << "> must be of type \"" << json::type_name(type) << "\" but was \"" << it->type_name() << "\" instead";
            throw api::InvalidArgumentException(message.str());
        }
    }

    template<typename T>
    T Get(const json& params, const std::string& parameter)
    {
        const auto it = params.find(parameter);
        if (it == params.end())
        {
            std::stringstream message;
            message << "parameter <" << parameter << "> missing";
            throw api::InvalidArgumentException(message.str());
        }

        return it->get<T>();
    }

    template<typename T>
    boost::optional<T> GetOptional(const json& params, const std::string& parameter)
    {
        const auto it = params.find(parameter);
        if (it == params.end())
            return boost::none; // missing so none

        return it->get<T>();
    }

    template<typename T>
    T GetOptional(const json& params, const std::string& parameter, T def)
    {
        const auto it = params.find(parameter);
        if (it == params.end())
            return def; // missing so return default

        return it->get<T>();
    }

}
