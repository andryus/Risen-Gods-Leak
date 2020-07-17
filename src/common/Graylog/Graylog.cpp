#include "Graylog.hpp"

#ifndef TRINITY_WITHOUT_GELFCPP

#include "Configuration/Config.h"
#include "Logging/Log.h"

void util::log::Graylog::Initialize()
{
    output_.reset();

    config_.enabled     = sConfigMgr->GetBoolDefault("RG.Logging.Graylog.Enabled", false);
    config_.hostname    = sConfigMgr->GetStringDefault("RG.Logging.Graylog.Server.Host", "");
    config_.port        = sConfigMgr->GetIntDefault("RG.Logging.Graylog.Server.Port", 0);
    config_.source      = sConfigMgr->GetStringDefault("RG.Logging.Graylog.Fields.source", "");
    config_.application = sConfigMgr->GetStringDefault("RG.Logging.Graylog.Fields.application_name", "");

    if (!config_.enabled)
    {
        TC_LOG_INFO("util.log.graylog", "Init: Graylog logging disabled");
        return;
    }

    if (config_.hostname.empty())
    {
        TC_LOG_ERROR("util.log.graylog", "Init: Graylog Input host not set");
        return;
    }

    if (config_.port <= 0 || config_.port > 65535)
    {
        TC_LOG_ERROR("util.log.graylog", "Init: Graylog Input port %u out of range [1, 65535]", config_.port);
        return;
    }

    if (config_.source.empty())
    {
        TC_LOG_ERROR("util.log.graylog", "Init: 'source' field not set");
        return;
    }

    if (config_.application.empty())
    {
        TC_LOG_ERROR("util.log.graylog", "Init: 'application_name' field not set");
        return;
    }

    output_ = std::make_unique<gelfcpp::output::GelfUDPOutput>(config_.hostname, config_.port);
}

double util::log::Graylog::GetCurrentTimestamp()
{
    using UnixTimeWithMS = std::chrono::duration<double>;
    return std::chrono::duration_cast<UnixTimeWithMS>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void util::log::Graylog::AddStaticFields(gelfcpp::GelfMessage& message)
{
    message.SetField("host", config_.source);
    message.SetField("source", config_.source);
    message.SetField("application_name", config_.application);
}

void util::log::Graylog::operator()(gelfcpp::GelfMessage& message)
{
    AddStaticFields(message);
    message.SetField("timestamp", GetCurrentTimestamp());
}

void util::log::Graylog::Send(gelfcpp::GelfMessage& message)
{
    AddStaticFields(message);

    if (output_)
        output_->Write(message);
}

#endif
