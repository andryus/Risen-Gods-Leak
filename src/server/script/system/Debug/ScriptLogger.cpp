#include "ScriptLogger.h"
#include <stack>
#include <algorithm>
#include <cmath>
#include <cstring>

#include "Log.h"
#include "Errors.h"
#include "IterateHelper.h"


namespace script
{

    std::string_view TypePrinter<std::string>::operator()(std::string_view data) const
    {
        return data;
    }

    std::string TypePrinter<bool>::operator()(bool data) const
    {
        return data ? "true" : "false";
    }

    ScriptLogger::ScriptLogger(std::string_view thisName) :
        thisName(thisName) { }

    void ScriptLogger::AddParam(std::string_view name, std::string_view value)
    {
        const bool valueIsThis = value == thisName;

        const size_t size = 6 + name.size() + 2 + (valueIsThis ? 4 : value.size());
        std::string outName = GetIndentation(false, size);
        outName += "|    #";

        const size_t pos = outName.size();

        if (!name.empty())
        {
            outName += name;
            outName[pos] = toupper(outName[pos]);
        }

        outName += + ": ";
        outName += (value == thisName ? "THIS" : value);

        AddLine(std::move(outName));
    }

    void ScriptLogger::AddModule(std::string_view name)
    {
        const size_t size = 5 + name.size();

        std::string line = GetIndentation(connected, size);
        connected = false;

        line += "|=>> ";
        line += name;

        AddLine(std::move(line));
    }

    void ScriptLogger::Start()
    {
        indentation++;

        connected = true;
    }

    void ScriptLogger::AddComposite(std::string_view nameSpace, std::string_view name)
    {
        if (!canLog)
            return;

        const size_t size = 3 + nameSpace.size() + 2 + name.size();

        std::string line = GetIndentation(connected, size);
        connected = false;
        
        line += "|= ";
        line += nameSpace;
        line += "::";
        line += name;

        AddLine(std::move(line));
    }

    void ScriptLogger::AddFailure(bool hard, std::string_view message)
    {
        for (std::string& line : reverse(lines))
        {
            const size_t pos = line.rfind('='); 
            if (pos != std::string::npos)
            {
                line[pos + 1] = hard ? '?' : '/';
                break;
            }
        }

        if (hard)
        {
            if (logLevel)
                TC_LOG_INFO("server.script", "Encountered an error while executing script '%s': %s\nCallstack:\n\n%s", thisName.data(), message.data(), String(0).c_str());
            else
                TC_LOG_ERROR("server.script", "Encountered an error while executing script '%s': %s\nCallstack:\n\n%s", thisName.data(), message.data(), String(0).c_str());
        }

        failed |= hard;
    }

    void ScriptLogger::AddError(std::string_view description)
    {
        std::string message = "!!! ";
        message += description;
        message += " !!!";

        AddParam("ERROR", message);
    }

    void ScriptLogger::AddData(std::string_view data)
    {
        ASSERT(!lines.empty());

        if (!data.empty())
        {
            auto& line = lines.back();
            line.reserve(line.size() + 1 + data.size() + 1);
            line += '(';
            line += data;
            line += ')';
        }
             
    }

    void ScriptLogger::SetLogLevel(bool infoOnly)
    {
        logLevel = infoOnly;
    }

    void ScriptLogger::End()
    {
        ASSERT(indentation >= 1);
        indentation--;

        connected = true;
    }

    void removeTrace(uint8 pos, uint32 begin, uint32 end, std::vector<std::string>& lines)
    {
        for (uint32 i = begin + 1; i < end; i++)
        {
            std::string& lineToReplace = lines[i];
            if (lineToReplace.size() > pos && lineToReplace[pos] == '|')
                lineToReplace[pos] = ' ';
        }
    }

    constexpr auto CALL_TRACE = "|   ";

    std::string ScriptLogger::String(uint8 addIndent) const
    {
        // fixup trailing call-traces '|'

        std::string& lastLine = lines.back();

        std::stack<uint8> toReplace;

        // collect size
        size_t size = 0;
        size_t maxSize = 0;
        for (const auto& line : lines)
        {
            const size_t lineSize = line.size();
            maxSize = std::max(maxSize, lineSize);
            size += lineSize;
        }

        const uint8 numTabsToCheck = maxSize / strlen(CALL_TRACE);

        for(uint8 i = 0; i < numTabsToCheck; i++)
        {
            const uint8 pos = i * strlen(CALL_TRACE);

            uint32 lastFound = 0;
            char foundOp = 0;

            for (const auto [line, index] : iterateIndex(lines))
            {
                if (pos + 1 >= line.size())
                    continue;

                const char currentOp = line[pos + 1];
                if (currentOp != ' ')
                {
                    const bool remove = 
                        (foundOp == '-' && currentOp != '-') ||
                        (foundOp == '=' && currentOp == '=' && (pos != 0 && line.at(pos - 1) == '-')) ||
                        (currentOp != '-' && currentOp != '=');

                    if (remove)
                        removeTrace(pos, lastFound, index, lines);

                    foundOp = currentOp;
                    lastFound = index;
                }
            }

            removeTrace(pos, lastFound, lines.size(), lines);
        }

        const std::string indent(addIndent * 4, ' ');
        const size_t indentSize = lines.size() * (addIndent * 4);

        // print content
        std::string output;
        output.reserve(size + 1);
        for (const auto& line : lines)
        {
            output += indent + line + '\n';
        }

        output += '\n';

        return output;
    }

    bool ScriptLogger::HadFailure() const
    {
        return failed;
    }

    bool ScriptLogger::LogLevel() const
    {
        return logLevel;
    }

    ScriptLogger::Block ScriptLogger::StoreLogger(ScriptLogger* logger)
    {
        ScriptLogger* _logger = currentLogger;
        currentLogger = logger;

        return {logger, logger};
    }

    ScriptLogger* ScriptLogger::GetGlobalLogger()
    {
        return currentLogger;
    }


    std::string ScriptLogger::GetIndentation(bool connected, size_t size) const
    {
        std::string string;
        string.reserve(indentation * sizeof(CALL_TRACE) + size);
        for (size_t i = 0; i < indentation; i++)
            string += CALL_TRACE;

        if (connected && !string.empty())
            string.replace(string.size() - 3, string.size(), "---");

        return string;
    }

    void ScriptLogger::AddLine(std::string&& line)
    {
        if (!canLog)
            return;

        lines.emplace_back(std::move(line));
    }

    LoggerBlock startLoggerBlock(ScriptLogger* logger)
    {
        if (logger)
            logger->Start();

        return LoggerBlock(logger);
    }

    LogLevelBlock applyLogLevel(ScriptLogger* logger, bool infoOnly)
    {
        bool oldLogLevel = false;
        if (logger)
        {
            oldLogLevel = logger->LogLevel();
            logger->SetLogLevel(infoOnly);
        }

        return LogLevelBlock(logger, oldLogLevel);
    }

    void reportLoggerFailure(ScriptLogger* logger, bool hard, std::string_view name)
    {
        if (logger)
            logger->AddFailure(hard, name);
    }

    ScriptLogger* ScriptLogger::currentLogger = nullptr;

}
