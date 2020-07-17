#pragma once

#include <boost/optional.hpp>
#include <string>

#include "Chat/Command/CommandDefines.hpp"

namespace chat {
    namespace command {

        class ExecutionContext;
        class MetaCommand;

        class GAME_API Command
        {
            friend class MetaCommand;

        // Attributes
        private:
            const uint32 id;
            const int32 sortingKey;
            const std::string name;
            std::string fullName;
            const locale::LocalizedString syntax;
            const locale::LocalizedString description;

            const uint32 permission;
            const CommandMask mask;

        // Methods
        public:
            explicit Command(CommandDefinition definition);
            virtual ~Command() = default;
            virtual void Initialize(std::string fullName);

            uint32 GetId() const { return id; };
            const std::string& GetName() const { return name; };
            const std::string& GetFullName() const { return fullName; };
            const locale::LocalizedString& GetSyntax() const { return syntax; };
            const locale::LocalizedString& GetDescription() const { return description; };
            uint32 GetPermission() const { return permission; };
            CommandMask GetMask() const { return mask; };

            virtual boost::optional<MetaCommand&> ToMetaCommand() { return boost::none; };
            virtual boost::optional<const MetaCommand&> ToMetaCommand() const { return boost::none; };

            virtual bool IsAvailable(const ExecutionContext& context) const;
            virtual void Handle(const CommandString& command, ExecutionContext& context) = 0; // note: implementation needs to make sure that the command is available!
            virtual void HandleHelp(const CommandString& command, ExecutionContext& context) const;
            virtual void PrintHelp(ExecutionContext& context) const;

            void Log(const CommandString& args, const ExecutionContext& context) const;
        };

    }
}
