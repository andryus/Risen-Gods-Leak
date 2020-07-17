#pragma once

#include <boost/optional.hpp>
#include <boost/range/any_range.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <memory>

#include "Utilities/boost_string_view/compare.hpp"

#include "Chat/Command/Commands/Command.hpp"

namespace chat {
    namespace command {

        class MetaCommand : public Command
        {
        public:
            using CommandRange = boost::any_range<Command, boost::forward_traversal_tag>;
            using ConstCommandRange = boost::any_range<const Command, boost::forward_traversal_tag>;

        private:
            struct name_tag {};
            struct order_tag {};
            using SubCommandContainer = boost::multi_index::multi_index_container<
                std::unique_ptr<Command>,
                boost::multi_index::indexed_by<
                    boost::multi_index::ordered_unique<
                        boost::multi_index::tag<order_tag>,
                        boost::multi_index::composite_key<
                            std::unique_ptr<Command>,
                            boost::multi_index::member<Command, const int32, &Command::sortingKey>,
                            boost::multi_index::const_mem_fun<Command, const std::string&, &Command::GetName>
                        >,
                        boost::multi_index::composite_key_compare<
                            std::less<int32>,
                            Trinity::string_view::is_iless
                        >
                    >,
                    boost::multi_index::hashed_unique<
                        boost::multi_index::tag<name_tag>,
                        boost::multi_index::const_mem_fun<Command, const std::string&, &Command::GetName>,
                        Trinity::string_view::ihash,
                        Trinity::string_view::is_iequal
                    >
                >
            >;

        // Attributes
        private:
            SubCommandContainer subCommands;

        // Methods
        public:
            explicit MetaCommand(CommandDefinition definition);
            void Initialize(std::string fullName) override;

            bool HasSubCommand(const CommandString& subCommandName) const;
            boost::optional<Command&> GetSubCommand(const CommandString& subCommandName);
            boost::optional<const Command&> GetSubCommand(const CommandString& subCommandName) const;
            void AddSubCommand(std::unique_ptr<Command> subCommand);

            bool HasSubCommands() const;
            bool HasAvailableSubCommands(const ExecutionContext& context) const;

            CommandRange GetSubCommands();
            ConstCommandRange GetSubCommands() const;
            CommandRange GetAvailableSubCommands(const ExecutionContext& context);
            ConstCommandRange GetAvailableSubCommands(const ExecutionContext& context) const;
            CommandRange GetAvailableSubCommandsByAbbreviation(const CommandString& abbreviation, const ExecutionContext& context);
            ConstCommandRange GetAvailableSubCommandsByAbbreviation(const CommandString& abbreviation, const ExecutionContext& context) const;

            boost::optional<MetaCommand&> ToMetaCommand() override { return boost::optional<MetaCommand&>{ *this }; };
            boost::optional<const MetaCommand&> ToMetaCommand() const override { return boost::optional<const MetaCommand&>{ *this }; };

            bool IsAvailable(const ExecutionContext& context) const override;

            void Handle(const CommandString& command, ExecutionContext& context) override;
            virtual void Handle_OnNoCommandNameFound(const CommandString& command, ExecutionContext& context);
            virtual void Handle_OnNoSubCommandFound(const CommandString& fullCommand, const CommandString& commandName, ExecutionContext& context);

            void HandleHelp(const CommandString& command, ExecutionContext& context) const override;
            void PrintHelp(ExecutionContext& context) const override;
        };

    }
}
