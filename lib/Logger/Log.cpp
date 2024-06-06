#include <cstdarg>
#include <functional>
#include <Arduino.h>
#include "Log.h"

namespace Log
{
    namespace
    { // Private members
        static Level m_LogLevel = Level::INFO;
        static std::function<void(char* msg)> m_LogFunction = [](char* msg) {
            Serial.println(msg);
        };

        void LogMessage(const char* level, const char* format, va_list args) {
            char buffer[LOG_BUFFER_SIZE] = {0};
            snprintf(buffer, LOG_BUFFER_SIZE, "%s", level);
            vsnprintf(buffer + strlen(level), LOG_BUFFER_SIZE - strlen(level), format, args);
            m_LogFunction(buffer);
        }
    } // namespace

    void SetLogLevel(Level level) {
        m_LogLevel = level;
    }
    void SetLogFunction(std::function<void(char* msg)> logFunction) {
        m_LogFunction = logFunction;
    }

    void Trace(const char* format, ...) {
        if (m_LogLevel >= Level::TRACE) {
            va_list args;
            va_start(args, format);
            LogMessage("TRACE - ", format, args);
            va_end(args);
        }
    }

    void Debug(const char* format, ...) {
        if (m_LogLevel >= Level::DEBUG) {
            va_list args;
            va_start(args, format);
            LogMessage("DEBUG - ", format, args);
            va_end(args);
        }
    }

    void Info(const char* format, ...) {
        if (m_LogLevel >= Level::INFO) {
            va_list args;
            va_start(args, format);
            LogMessage("INFO - ", format, args);
            va_end(args);
        }
    }

    void Warning(const char* format, ...) {
        if (m_LogLevel >= Level::WARNING) {
            va_list args;
            va_start(args, format);
            LogMessage("WARNING - ", format, args);
            va_end(args);
        }
    }

    void Error(const char* format, ...) {
        if (m_LogLevel >= Level::ERROR) {
            va_list args;
            va_start(args, format);
            LogMessage("ERROR - ", format, args);
            va_end(args);
        }
    }

    void Fatal(const char* format, ...) {
        if (m_LogLevel >= Level::FATAL) {
            va_list args;
            va_start(args, format);
            LogMessage("FATAL - ", format, args);
            va_end(args);
        }
    }
} // namespace Log
