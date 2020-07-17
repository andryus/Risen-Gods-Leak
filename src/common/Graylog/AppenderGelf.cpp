#include "AppenderGelf.h"

#include "Config.h"
#include "StringFormat.h"
#include "Graylog/Graylog.hpp"

AppenderGelf::AppenderGelf(uint8 id, std::string const& name, LogLevel level, AppenderFlags flags, ExtraAppenderArgs extraArgs) :
    Appender(id, name, level, flags)
{
    if (!sConfigMgr->GetBoolDefault("RG.Logging.Graylog.Enabled", false))
        throw InvalidAppenderArgsException(Trinity::StringFormat("Log::CreateAppenderFromConfig: Skipped gelf appender '%s', gelf is disabled", name.c_str()));
}

AppenderGelf::~AppenderGelf()
{

}

double AppenderGelf::GetMsTimestamp(LogMessage::TimePoint const& time)
{
    typedef std::chrono::duration<double> FloatingSeconds;

    return std::chrono::duration_cast<FloatingSeconds>(time.time_since_epoch()).count();
}


void AppenderGelf::_write(LogMessage const* message)
{
    gelfcpp::GelfMessage msg;
    msg.SetField("timestamp", GetMsTimestamp(message->mtime));
    msg.SetField("log_level", Appender::getLogLevelString(message->level));
    msg.SetField("log_type", message->type);
    msg.SetMessage(message->text);

    sGraylog->Send(msg);
}
