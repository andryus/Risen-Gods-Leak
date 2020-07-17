#pragma once

#include <exception>

namespace api
{

    class APIException : public std::runtime_error
    {
    public:
        using runtime_error::runtime_error;
    };

    class InvalidArgumentException : public APIException
    {
    public:
        explicit InvalidArgumentException(const std::string& message) : APIException(message) {}
    };

    class AccessControlException : public APIException
    {
    public:
        explicit AccessControlException(const std::string& message = "access denied") : APIException(message) {}
    };

}
