#pragma once

#ifdef CONFIG_WITH_PROMETHEUS_MONITORING
#include "Define.h"
#include <memory>
#include <prometheus/registry.h>

namespace RG { namespace Monitoring {

class Module
{
public:
    explicit Module(std::string module_name);

    const std::shared_ptr<prometheus::Registry>& GetRegistry() const;

    prometheus::Family<prometheus::Counter>& AddCounter(const std::string& name,
                                                        const std::string& help,
                                                        const std::map<std::string, std::string>& labels = {});
    prometheus::Family<prometheus::Gauge>& AddGauge(const std::string& name,
                                                    const std::string& help,
                                                    const std::map<std::string, std::string>& labels = {});
    prometheus::Family<prometheus::Histogram>& AddHistogram(const std::string& name,
                                                            const std::string& help,
                                                            const std::map<std::string, std::string>& labels = {});
    prometheus::Family<prometheus::Summary>& AddSummary(const std::string& name,
                                                        const std::string& help,
                                                        const std::map<std::string, std::string>& labels = {});

protected:
    const std::string module_name_;
    std::shared_ptr<prometheus::Registry> registry_;
};

template<typename T> struct ModuleAccessor;

}}
#endif
