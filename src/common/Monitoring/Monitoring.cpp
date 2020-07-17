#include "Monitoring/Monitoring.hpp"

#include <Config.h>
#include <Utilities/StringFormat.h>
#include <prometheus/exposer.h>

namespace RG { namespace Monitoring {
namespace
{
struct
{
    bool enabled = true;

    struct
    {
        std::string host = "127.0.0.1";
        uint16_t port = 9110;
    } exposer;

} config_;

std::unique_ptr<prometheus::Exposer> exposer_;

}

void AddRegistry(const std::weak_ptr<prometheus::Collectable>& collectable)
{
    exposer_->RegisterCollectable(collectable);
};

void Initialize()
{
    LoadConfig();

    if (!config_.enabled)
        return;

    exposer_ = std::make_unique<prometheus::Exposer>(Trinity::StringFormat("%s:%u", config_.exposer.host, config_.exposer.port));
}

void LoadConfig()
{
    config_.enabled = sConfigMgr->GetBoolDefault("RG.Monitoring.Enabled", true);
    config_.exposer.host = sConfigMgr->GetStringDefault("RG.Monitoring.Exposer.Host", "127.0.0.1");
    config_.exposer.port = static_cast<uint16_t>(sConfigMgr->GetIntDefault("RG.Monitoring.Exposer.Port", 9110));
}

}}
