#pragma once

#include "Appender.h"

class AppenderGelf : public Appender
{
public:
    typedef std::integral_constant<AppenderType, APPENDER_GELF>::type TypeIndex;

public:
    AppenderGelf(uint8 id, std::string const& name, LogLevel level, AppenderFlags flags, ExtraAppenderArgs extraArgs);
    ~AppenderGelf();

    AppenderType getType() const override { return TypeIndex::value; }

private:
    void _write(LogMessage const* message) override;

    static double GetMsTimestamp(LogMessage::TimePoint const& time);
};
