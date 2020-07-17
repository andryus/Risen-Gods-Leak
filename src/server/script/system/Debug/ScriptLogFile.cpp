#include "ScriptLogFile.h"
#include <time.h>
#include <fstream>
#include <boost/filesystem.hpp>
#include "ScriptLogger.h"
#include "Config.h"

#include "Log.h"
#include "Utilities/Util.h"

namespace script
{

    std::string getTimeStr()
    {
        const auto now = std::chrono::system_clock::now();
        time_t time = std::chrono::system_clock::to_time_t(now);
        tm aTm;
        localtime_r(&time, &aTm);
        char buf[20];
        snprintf(buf, 20, "%04d-%02d-%02d_%02d-%02d-%02d", aTm.tm_year + 1900, aTm.tm_mon + 1, aTm.tm_mday, aTm.tm_hour, aTm.tm_min, aTm.tm_sec);
        return std::string(buf);
    }

    std::string getTimeStamp(std::chrono::system_clock::time_point now)
    {
        const time_t time = std::chrono::system_clock::to_time_t(now);

        const auto duration = now.time_since_epoch();
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        const int milliseconds = millis % 1000;

        tm aTm;
        localtime_r(&time, &aTm);
        char buf[32];
        snprintf(buf, 32, "/// %02d:%02d:%02d:%03d\n", aTm.tm_hour, aTm.tm_min, aTm.tm_sec, milliseconds);

        return std::string(buf);
    }

    ScriptLogFile::ScriptLogFile(std::string&& scriptName) :
        scriptName(std::move(scriptName)), logType(LogType::NONE) { }

    void ScriptLogFile::SetOwnerName(std::string_view name, std::string_view fullName)
    {
        std::string time = getTimeStr();

        if (sharedLogDir.empty()) // todo: manual initialisation?
        {
            sharedLogDir = sConfigMgr->GetStringDefault("Script.LogsDir", "Scripts") + '/';

            if (!boost::filesystem::exists(sharedLogDir))
                boost::filesystem::create_directory(sharedLogDir);

            const size_t pos = time.rfind('-');
            ASSERT(pos != std::string::npos);
            time.erase(pos, std::string::npos);

            sharedLogDir += time + '/';

            if (!boost::filesystem::exists(sharedLogDir))
                boost::filesystem::create_directory(sharedLogDir);
        }

        fileName = sharedLogDir;
        fileName += scriptName + '_';
        fileName += name;
        fileName += '_' + time;
        fileName += ".slog";

        thisName = fullName;

        CheckCreateFile();
    }

    void ScriptLogFile::ChangeLogging(LogType type)
    {
        if (logType == type)
            return;

        logType = type;

        CheckCreateFile();
    }

    bool ScriptLogFile::CanLog() const
    {
        return logType != LogType::NONE;
    }

    ScriptLogFile::Block ScriptLogFile::RequestFileBlock(ScriptLogFile* file)
    {
        if(file)
            return file->RequestLogger();
        else
            return {};
    }

    ScriptLogFile::Block ScriptLogFile::RequestLogger()
    {
        const auto now = std::chrono::system_clock::now();
        ScriptLogger* logger = loggers.emplace_back(Entry{std::make_unique<ScriptLogger>(thisName), true, numLoggers, now}).logger.get();

        numLoggers++;

        return Block(this, logger);
    }

    ScriptLogger* ScriptLogFile::LastLogger() const
    {
        if (!loggers.empty())
            return loggers.back().logger.get();
        else
            return nullptr;
    }

    void ScriptLogFile::PopLogger(bool success)
    {
        ASSERT(numLoggers >= 1);
        numLoggers--;

        ASSERT(numLoggers < loggers.size());
        loggers[numLoggers].success = success;

        if (!numLoggers)
        {
            ASSERT(!loggers.empty());

            const auto canLogEntry = [logType = logType](const auto& entry)
            {
                return (logType != LogType::NONE && (entry.success || logType == LogType::VERBOSE)) || entry.logger->HadFailure();
            };

            bool canLogEntries = false;
            for (const auto& entry : loggers)
                canLogEntries |= canLogEntry(entry);

            if (canLogEntries)
            {
                std::ofstream file(fileName, std::ios_base::ate | std::ios_base::app);
                const std::string stamp = getTimeStamp(loggers.front().time);

                file.write(stamp.data(), stamp.size());
                for (auto& entry : loggers)
                {
                    if (canLogEntry(entry))
                        Append(entry, file);
                }
            }

            loggers.clear();
        }
    }

    void ScriptLogFile::Append(const Entry& entry, std::ostream& stream)
    {
        const std::string content = entry.logger->String(entry.indent);

        if(entry.logger->HadFailure())
        {
            const std::string_view failure = "!!! ERROR (NULL-ACCESS) !!!\n";

            stream.write(failure.data(), failure.size());
        }

        stream.write(content.data(), content.size());

        if (entry.success && !entry.logger->HadFailure())
            TC_LOG_INFO("server.script", "\n\n%s", content.data());
    }

    void ScriptLogFile::CheckCreateFile()
    {
        if (logType != LogType::NONE)
        {
            if (!fileName.empty() && !boost::filesystem::exists(fileName))
            {
                std::ofstream file(fileName, std::ios_base::trunc);
                file << getTimeStamp(std::chrono::system_clock::now());
                file << "#THIS: " << thisName << "\n\n";
            }
        }
    }

    std::string ScriptLogFile::sharedLogDir;

    ScriptLogger* requestCurrentLogger(ScriptLogFile* file)
    {
        if (file)
            return file->LastLogger();
        else
            return nullptr;
    }

}
