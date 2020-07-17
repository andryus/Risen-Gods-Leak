#include "Monitoring/Module.hpp"

#include "Utilities/StringFormat.h"
#include <prometheus/exposer.h>

using namespace RG::Monitoring;

Module::Module(std::string module_name) :
        module_name_(std::move(module_name)),
        registry_(std::make_shared<prometheus::Registry>())
{}

const std::shared_ptr<prometheus::Registry>& Module::GetRegistry() const
{
    return registry_;
}

prometheus::Family<prometheus::Counter>& Module::AddCounter(const std::string& name,
                                                            const std::string& help,
                                                            const std::map<std::string, std::string>& labels)
{
    return prometheus::BuildCounter()
            .Help(help)
            .Name(Trinity::StringFormat("trinity_%s_%s", module_name_, name))
            .Labels(labels)
            .Register(*registry_);
}

prometheus::Family<prometheus::Gauge>& Module::AddGauge(const std::string& name,
                                                        const std::string& help,
                                                        const std::map<std::string, std::string>& labels)
{
    return prometheus::BuildGauge()
            .Help(help)
            .Name(Trinity::StringFormat("trinity_%s_%s", module_name_, name))
            .Labels(labels)
            .Register(*registry_);
}

prometheus::Family<prometheus::Histogram>& Module::AddHistogram(const std::string& name,
                                                                const std::string& help,
                                                                const std::map<std::string, std::string>& labels)
{
    return prometheus::BuildHistogram()
            .Help(help)
            .Name(Trinity::StringFormat("trinity_%s_%s", module_name_, name))
            .Labels(labels)
            .Register(*registry_);
}

prometheus::Family<prometheus::Summary>& Module::AddSummary(const std::string& name,
                                                            const std::string& help,
                                                            const std::map<std::string, std::string>& labels)
{
    return prometheus::BuildSummary()
            .Help(help)
            .Name(Trinity::StringFormat("trinity_%s_%s", module_name_, name))
            .Labels(labels)
            .Register(*registry_);
}
