#pragma once

#include <vector>
#include <memory>

#include "LogModule.hpp"
#include "modules/ItemLogModule.hpp"
#include "modules/MoneyLogModule.hpp"
#include "modules/LevelLogModule.hpp"
#include "modules/EncounterLogModule.hpp"
#include "modules/ServerStatisticsModule.hpp"
#include "modules/GMLogModule.hpp"
#include "modules/TradeLogModule.hpp"
#include "modules/PremiumLogModule.hpp"

namespace RG
{
    namespace Logging
    {
        class GAME_API Manager
        {
        public:
            static void Shutdown();
            static void Initialize();

            static void LoadConfig();

            template<typename Type, typename... Args>
            static auto Log(Args&&... args) -> 
                decltype(std::declval<Type>().Log(std::forward<Args>(args)...))
            {
                uint8 idx = static_cast<uint8>(GetType<Type>());

                return static_cast<Type*>(modules_[idx].get())->Log(std::forward<Args>(args)...);
            }

            template<typename Type>
            static Type* GetModule()
            {
                uint8 idx = static_cast<uint8>(GetType<Type>());

                return static_cast<Type*>(modules_[idx].get());
            }

            static bool IsEnabled() { return enabled_; }

        private:
            template<typename T>
            static void AddModule()
            {
                uint8 idx = static_cast<uint8>(GetType<T>());
                ASSERT(modules_[idx].get() == nullptr);
                modules_[idx] = std::unique_ptr<T>(new T());
            }

            template<typename T>
            static void ForEach(const T& func)
            {
                for (std::unique_ptr<LogModule>& module : modules_)
                    func(module);
            }

            template<typename T>
            static inline ModuleType GetType();

        private:
            static std::vector<std::unique_ptr<LogModule>> modules_;
            static bool enabled_;
            static bool initDone_;
        };

#define MAP_MODULE_TYPE(CppType, EnumType) \
    template<> inline ModuleType Manager::GetType<CppType>() { return ModuleType::EnumType; }

        MAP_MODULE_TYPE(ItemLogModule, ITEM)
        MAP_MODULE_TYPE(MoneyLogModule, MONEY)
        MAP_MODULE_TYPE(LevelLogModule, LEVEL)
        MAP_MODULE_TYPE(EncounterLogModule, ENCOUNTER)
        MAP_MODULE_TYPE(ServerStatisticsModule, SERVER_STATISTICS)
        MAP_MODULE_TYPE(GMLogModule, GM_ACTIONS)
        MAP_MODULE_TYPE(TradeLogModule, TRADE)
        MAP_MODULE_TYPE(PremiumLogModule, PREMIUM)

#undef MAP_MODULE_TYPE
    }
}

template<typename T, typename... Args>
inline auto RG_LOG(Args&&... args) -> decltype(RG::Logging::Manager::Log<T>(std::forward<Args>(args)...))
{
    return RG::Logging::Manager::Log<T>(std::forward<Args>(args)...);
}
