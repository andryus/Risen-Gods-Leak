#pragma once
#include <iosfwd>
#include <memory>
#include <vector>
#include <string_view>
#include <string>
#include "Define.h"

namespace script
{

    /************************************
    * Printer
    *************************************/

    template<class Data, typename = void>
    struct TypePrinter
    {
        constexpr std::string_view operator()(const Data&) const
        {
            return "<UNK>";
        }
    };

    template<class Data>
    auto print(Data&& data)
    {
        return TypePrinter<std::decay_t<Data>>()(data);
    }

    template<>
    struct SCRIPT_API TypePrinter<std::string>
    {
        std::string_view operator()(std::string_view data) const;
    };

    template<>
    struct SCRIPT_API TypePrinter<bool>
    {
        std::string operator()(bool data) const;
    };

    template<class Data>
    struct TypePrinter<Data*>
    {
        auto operator()(Data* data)
        {
            if (data)
                return print(*data);
            else
                return decltype(print(*data))("NULL");
        }
    };

    template<class Data>
    struct TypePrinter<Data, std::enable_if_t<std::is_integral_v<Data>>>
    {
        std::string operator()(Data data) const
        {
            return std::to_string(data);
        }
    };

    template<class Data>
    struct TypePrinter<Data, std::enable_if_t<std::is_floating_point_v<Data>>>
    {
        std::string operator()(Data data) const
        {
            return std::to_string(data);
        }
    };

#define SCRIPT_PRINTER(TYPE)                             \
    template<>                                           \
    struct script::TypePrinter<TYPE>                     \
    {                                                    \
        std::string operator()(const TYPE& value) const; \
    };                                                   \

#define SCRIPT_PRINTER_IMPL(TYPE)                                              \
    std::string script::TypePrinter<TYPE>::operator()(const TYPE& value) const  \

    /************************************
    * Logger
    *************************************/

    class SCRIPT_API ScriptLogger
    {
        struct Indentation
        {
            Indentation(std::string&& string) : 
                string(std::move(string)) {}

            std::string string;
        };

        struct Block
        {
            Block(ScriptLogger* logger, ScriptLogger* oldLogger) :
                logger(logger),  oldLogger(oldLogger) {}

            Block(Block&& block) :
                logger(block.logger), oldLogger(block.oldLogger) 
            {
                block.logger = nullptr;
                block.oldLogger = nullptr;

                block.apply = false;
            }

            Block(const Block& block) = delete;

            ~Block()
            {
                if (apply)
                    ScriptLogger::currentLogger = oldLogger;
            }

        private:

            ScriptLogger* logger;
            ScriptLogger* oldLogger;
            bool apply = true;
        };

    public:

        ScriptLogger(std::string_view thisName);

        void AddParam(std::string_view name, std::string_view value);
        void AddModule(std::string_view name);
        void AddData(std::string_view data);
        void SetLogLevel(bool infoOnly);

        void Start();
        void AddComposite(std::string_view nameSpace, std::string_view name);
        void AddFailure(bool hard, std::string_view name);
        void AddError(std::string_view description);
        void End();

        std::string String(uint8 addIndent) const;

        static Block StoreLogger(ScriptLogger* logger);
        static ScriptLogger* GetGlobalLogger();

        bool HadFailure() const;
        bool LogLevel() const;

    private:

        std::string GetIndentation(bool connected, size_t size) const;
        void AddLine(std::string&& line);

        bool canLog = true;
        bool logLevel = false;
        mutable std::vector<std::string> lines;
        uint8 indentation = 0;
        bool connected = false;
        bool failed = false;
        std::string_view thisName;

        static ScriptLogger* currentLogger;
    };

    struct LoggerBlock
    {
        LoggerBlock(ScriptLogger* logger) :
            logger(logger) { }

        ~LoggerBlock() { if(logger) logger->End(); }

    private:

        ScriptLogger* logger;
    };

    struct LogLevelBlock
    {
        LogLevelBlock(ScriptLogger* logger, bool oldLogLevel) :
            oldLogLevel(oldLogLevel), logger(logger) { }

        ~LogLevelBlock() { if (logger) logger->SetLogLevel(oldLogLevel); }

    private:

        bool oldLogLevel;
        ScriptLogger* logger;
    };

    SCRIPT_API LoggerBlock startLoggerBlock(ScriptLogger* logger);
    SCRIPT_API LogLevelBlock applyLogLevel(ScriptLogger* logger, bool infoOnly);
    SCRIPT_API void reportLoggerFailure(ScriptLogger* logger, bool hard, std::string_view name);

}
