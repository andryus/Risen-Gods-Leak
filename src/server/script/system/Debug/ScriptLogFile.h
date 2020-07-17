#pragma once
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include "Define.h"

namespace script
{

    class ScriptLogger;

    enum class LogType : uint8
    {
        NONE,
        NORMAL,
        VERBOSE
    };

    class SCRIPT_API ScriptLogFile
    {
        struct Block
        {
            Block() = default;
            Block(ScriptLogFile* file, ScriptLogger* logger) : 
                file(file), logger(std::move(logger)) {}

            Block(Block&& block) = default;

            void SetSuccess(bool success) { this->success = success; }

            ~Block() { if(logger) file->PopLogger(success); }

            ScriptLogger* Logger() const { return logger; }
        private:
            ScriptLogFile* file = nullptr;
            ScriptLogger* logger;
            bool success = true;
        };

    public:
        ScriptLogFile(std::string&& scriptName);
        ScriptLogFile(const ScriptLogFile&) = delete;
        void operator=(const ScriptLogFile&) = delete;

        void SetOwnerName(std::string_view name, std::string_view fullName);
        Block RequestLogger();
        ScriptLogger* LastLogger() const;

        void ChangeLogging(LogType type);
        bool CanLog() const;

        static Block RequestFileBlock(ScriptLogFile* file);

    private:

        struct Entry
        {
            std::unique_ptr<ScriptLogger> logger;
            bool success;
            uint8 indent;
            std::chrono::system_clock::time_point time;
        };

        void PopLogger(bool success);
        void Append(const Entry& entry, std::ostream& stream);

        void CheckCreateFile();

        std::vector<Entry> loggers;
        uint8 numLoggers = 0;

        LogType logType;
        std::string scriptName, fileName, thisName;

        static std::string sharedLogDir;
    };

    SCRIPT_API ScriptLogger* requestCurrentLogger(ScriptLogFile* file);

}