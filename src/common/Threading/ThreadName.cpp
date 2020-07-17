#include "ThreadName.hpp"

#include "CompilerDefs.h"
#if PLATFORM == PLATFORM_UNIX
    #include <sys/prctl.h>
#endif

namespace util
{
namespace thread
{
namespace nameing
{

#if PLATFORM == PLATFORM_UNIX
void SetName(const std::string& name)
{
    prctl(PR_SET_NAME, name.c_str(), 0, 0, 0);
}

std::string GetName()
{
    char buffer[16];
    prctl(PR_GET_NAME, buffer, 0, 0, 0);
    return std::string(buffer);
}

#else
void SetName(const std::string& name) {}
std::string GetName() { return std::string("NYI"); }
#endif

}
}
}