#pragma once

#include "Chat/Command/ExecutionContext.hpp"
#include "Chat/Command/Commands/Command.hpp"
#include "Chat/Command/Util/Argument.hpp"

namespace chat {
    namespace command {

        class GAME_API SimpleCommand : public Command
        {
        public:
            enum class ExecutionResult
            {
                SUCCESS,
                FAILURE, // use this if no other option fits
                FAILURE_ERROR_MESSAGE_ALREADY_SENT,
                FAILURE_NOT_ALLOWED,
                FAILURE_MISSING_ARGUMENT,
                FAILURE_INVALID_ARGUMENT,
            };

        public:
            explicit SimpleCommand(CommandDefinition definition, bool availableOnConsole);

            bool IsAvailable(const ExecutionContext& context) const override;
            void Handle(const CommandString& command, ExecutionContext& context) override;

            virtual ExecutionResult Execute(const CommandString& command, ExecutionContext& context);
            virtual ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) { return ExecutionResult::SUCCESS; };

        protected:
            template<typename... Args>
            ExecutionResult SendError(ExecutionContext& context, Args&&... args) const
            {
                context.SendSystemMessage(std::forward<Args>(args)...);
                return ExecutionResult::FAILURE_ERROR_MESSAGE_ALREADY_SENT;
            }

        private:
            bool availableOnConsole;
        };

    }
}
