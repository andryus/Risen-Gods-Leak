#pragma once

#ifndef TRINITY_WITHOUT_GELFCPP

#include "Define.h"
#include <gelfcpp/output/GelfUDPOutput.hpp>
#include <gelfcpp/GelfMessageStream.hpp>
#include <chrono>
#include <memory>

namespace util
{
namespace log
{

class Graylog
{
public:
    static Graylog* Instance()
    {
        static Graylog instance;
        return &instance;
    }

    void Initialize();

    void Send(gelfcpp::GelfMessage& message);

    gelfcpp::output::GelfUDPOutput* GetOutput() { return output_.get(); }
    Graylog& GetDecorator() { return *this; }
    void operator()(gelfcpp::GelfMessage& message);

private:
    void AddStaticFields(gelfcpp::GelfMessage& message);
    double GetCurrentTimestamp();

private:
    std::unique_ptr<gelfcpp::output::GelfUDPOutput> output_;
    struct
    {
        bool enabled;
        std::string hostname;
        uint32 port;

        std::string source;
        std::string application;
    } config_;
};

}
}

#define sGraylog ::util::log::Graylog::Instance()
#define GRAYLOG_STRUCTURED() GELF_MESSAGE(sGraylog->GetOutput())(sGraylog->GetDecorator())("is_structured", true)

#else
namespace util
{
namespace log
{

class Graylog
{
    struct DummyOutput
    {
        template<typename T> void Write(const T&) {};
    };
public:
    static Graylog* Instance()
    {
        static Graylog instance;
        return &instance;
    }

    void Initialize() {}
    template<typename T> void Send(T&) {};

    DummyOutput* GetOutput() { return nullptr; }
    Graylog& GetDecorator() { return *this; }
};

struct Absorber
{
    Absorber operator()(...)
    {
        return {};
    }
};

}
}

#define sGraylog ::util::log::Graylog::Instance()
#define GRAYLOG_STRUCTURED() for (;false;) ::util::log::Absorber()
#endif
