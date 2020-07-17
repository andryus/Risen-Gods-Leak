#pragma once

#include "Define.h"
#include <string>

namespace RG
{
    namespace Logging
    {
        namespace detail
        {
            class TableCleanupTask;
        }

        enum class ModuleType : uint8
        {
            ITEM,
            MONEY,
            LEVEL,
            ENCOUNTER,
            SERVER_STATISTICS,
            GM_ACTIONS,
            TRADE,
            PREMIUM,
            __MAX_TYPES
        };

        constexpr uint8 MODULE_COUNT = static_cast<uint8>(ModuleType::__MAX_TYPES);

        class LogModule
        {
        public:
            LogModule(const std::string& moduleName);
            virtual ~LogModule() {}

            virtual void Initialize() {}
            virtual void LoadConfig();

            bool IsEnabled() const { return enabled_; }

        protected:
            std::string GetOption(const std::string& option) const;

        private:
            std::string prefix_;
            bool enabled_;
        };
    }
}
