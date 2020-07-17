#pragma once

#include "Define.h"
#include <string>
#include <vector>

class ChatCommand;

namespace RG
{
namespace Cleanup
{
namespace Account
{
    void Run();

    void UpdateList();
    void UpdateState();
    void ProcessList();

    void LoadFromConfig(bool reload = false);

    uint32 ProcessAccount(uint32 accountId, const std::string& username, const std::string& reason, uint32& removed);
    void DumpAccountInfo(uint32 accountId, const std::string& username);
}
}
}
