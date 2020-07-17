#include "LogModule.hpp"

#include <sstream>
#include "LogManager.hpp"

#include "Config.h"

RG::Logging::LogModule::LogModule(const std::string& moduleName) :
    prefix_(moduleName)
{
}

void RG::Logging::LogModule::LoadConfig()
{
    enabled_ = Manager::IsEnabled() && sConfigMgr->GetBoolDefault(GetOption("Enabled").c_str(), false);
}

std::string RG::Logging::LogModule::GetOption(const std::string & option) const
{
    std::ostringstream os;
    os << "RG.Logging." << prefix_ << '.' << option;
    return os.str();
}

