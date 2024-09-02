#include <cstdarg>
#include <functional>
#include <Arduino.h>
#include "Log.h"

namespace Log
{
    namespace
    { // Private members
        static Level m_LogLevel = Level::INFO;
        char m_Buffer[LOG_BUFFER_SIZE] = {0};
        static std::function<void(char* msg)> m_LogFunction = [](char* msg) {
            Serial.println(msg);
        };

        // Note: This function is not thread safe
        void LogMessage(const char* level, const char* format, va_list args) {
            const char* color_code = "";

            // Determine color based on log level
            if (strcmp(level, "TRACE - ") == 0) {
                color_code = "\033[37m"; // White
            } else if (strcmp(level, "DEBUG - ") == 0) {
                color_code = "\033[36m"; // Cyan
            } else if (strcmp(level, "INFO - ") == 0) {
                color_code = "\033[32m"; // Green
            } else if (strcmp(level, "WARNING - ") == 0) {
                color_code = "\033[33m"; // Yellow
            } else if (strcmp(level, "ERROR - ") == 0) {
                color_code = "\033[31m"; // Red
            } else if (strcmp(level, "FATAL - ") == 0) {
                color_code = "\033[41m"; // Red background
            }

            int offset = snprintf(m_Buffer, LOG_BUFFER_SIZE, "%s%s", color_code, level);
            if (offset < 0) { return; }
            vsnprintf(m_Buffer + offset, LOG_BUFFER_SIZE - offset, format, args);
            m_Buffer[LOG_BUFFER_SIZE - 1] = '\0';

            strncat(m_Buffer, "\033[0m", LOG_BUFFER_SIZE - strlen(m_Buffer) - 1);

            m_LogFunction(m_Buffer);

        }
    } // namespace

    void SetLogLevel(Level level) {
        m_LogLevel = level;
    }
    bool SetLogLevel(std::string_view level) {
        if (level == "Trace") {
            m_LogLevel = Level::TRACE;
        }
        else if (level == "Debug") {
            m_LogLevel = Level::DEBUG;
        }
        else if (level == "Info") {
            m_LogLevel = Level::INFO;
        }
        else if (level == "Warning") {
            m_LogLevel = Level::WARNING;
        }
        else if (level == "Error") {
            m_LogLevel = Level::ERROR;
        }
        else if (level == "Fatal") {
            m_LogLevel = Level::FATAL;
        }
        else {
            return false;
        }
        return true;
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
