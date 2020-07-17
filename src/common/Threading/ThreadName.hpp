#pragma once

#include <string>
#include "API.h"

namespace util
{
namespace thread
{
namespace nameing
{

COMMON_API void SetName(const std::string& name);
COMMON_API std::string GetName();

}
}
}