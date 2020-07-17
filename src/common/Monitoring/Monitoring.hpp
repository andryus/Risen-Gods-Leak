#pragma once

#include <memory>

namespace prometheus { class Collectable; }

namespace RG
{
namespace Monitoring
{
void Initialize();
void LoadConfig();
void AddRegistry(const std::weak_ptr<prometheus::Collectable>& collectable);

template<typename T>
void RegisterModule()
{
    AddRegistry(T::Instance().GetRegistry());

};
}
}
