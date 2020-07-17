#pragma once

#include "RG/Logging/LogModule.hpp"
#include <atomic>
#include <unordered_map>

class Unit;
class Map;


namespace RG
{
namespace Logging
{
    class GAME_API EncounterLogModule : public LogModule
    {
    private:
        enum class Mode : uint8
        {
            DEFAULT = 1,
            INSTANCE_NORMAL = 2,
            INSTANCE_HEROIC = 3,
            INSTANCE_EPIC = 4,
            RAID_10MAN = 5,
            RAID_25MAN = 6,
            RAID_10MAN_HEROIC = 7,
            RAID_25MAN_HEROIC = 8,
        };
    public:
        enum class Type : uint8
        {
            UNDEFINED    = 0,
            PULL        = 1,
            WIPE        = 2,
            KILL        = 3,
            SPECIAL        = 4,
        };

    public:
        EncounterLogModule();

        void Initialize() override;

        void Log(Unit* boss) { Log(boss, Type::KILL); }
        void Log(Unit* boss, Type type, bool force = false);
    private:
        static Mode GetMode(Map* map);

    private:
        std::atomic<uint32> logNumber_;
        std::unordered_map<uint32, uint8> extendedLoggingMaps_;
    };
}
}

using RG::Logging::EncounterLogModule;
