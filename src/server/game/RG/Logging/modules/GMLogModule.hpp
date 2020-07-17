#pragma once

#include "RG/Logging/LogModule.hpp"
#include <set>

class Player;
class Unit;
class Spell;
class SpellCastTargets;

namespace RG
{
    namespace Logging
    {
        class GMLogModule : public LogModule
        {
        public:
            GMLogModule();

            void LoadConfig() override;

            void Log(Player* player, Spell* spell, Unit* caster, const SpellCastTargets& targets);          //! Cast
            void Log(uint32 accountId, Player* player, const std::string& cmd, const std::string& params);  //! Command
            void Log(uint32 accountId, const std::string& cmd, const std::string& params);                  //! cli commands

        private:
            static uint32 GetTargetType(Player* player);
            static uint64 GetGuidOrEntry(Player* player);

        private:
            struct
            {
                uint32 gm_level;

                bool spell_logging_enabled;
                bool command_logging_enabled;
            } config;
        };
    }
}

using RG::Logging::GMLogModule;
