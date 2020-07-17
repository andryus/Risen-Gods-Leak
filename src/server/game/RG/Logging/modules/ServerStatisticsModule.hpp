#pragma once

#include "RG/Logging/LogModule.hpp"
#include "Timer.h"

namespace RG
{
    namespace Logging
    {
        class ServerStatisticsModule : public LogModule
        {
        public:
            ServerStatisticsModule();

            void LoadConfig() override;

            void Update(uint32 diff);

        private:
            void CollectData();

        private:
            struct
            {
                uint32 interval;
            } config;

            IntervalTimer timer_;
            struct
            {
                uint64 sum;
                uint32 counter;
                uint32 peak;
            } updatediff;
        };
    }
}

using RG::Logging::ServerStatisticsModule;
