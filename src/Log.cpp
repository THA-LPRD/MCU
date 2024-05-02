#include <Arduino.h>
#include "Log.h"

void Log::Debug(const char* format, ...) {
    if (GetInstance().m_LogLevel >= Level::DEBUG) {
        va_list args;
        va_start(args, format);
        LogMessage("DEBUG - ", format, args);
        va_end(args);
    }
}

void Log::Info(const char* format, ...) {
    if (GetInstance().m_LogLevel >= Level::INFO) {
        va_list args;
        va_start(args, format);
        LogMessage("INFO - ", format, args);
        va_end(args);
    }
}

void Log::Warning(const char* format, ...) {
    if (GetInstance().m_LogLevel >= Level::WARNING) {
        va_list args;
        va_start(args, format);
        LogMessage("WARNING - ", format, args);
        va_end(args);
    }
}

void Log::Error(const char* format, ...) {
    if (GetInstance().m_LogLevel >= Level::ERROR) {
        va_list args;
        va_start(args, format);
        LogMessage("ERROR - ", format, args);
        va_end(args);
    }
}

void Log::Fatal(const char* format, ...) {
    if (GetInstance().m_LogLevel >= Level::FATAL) {
        va_list args;
        va_start(args, format);
        LogMessage("FATAL - ", format, args);
        va_end(args);
    }
}

Log::Log() : m_LogLevel(Level::INFO) {
    m_LogFunction = [](char* msg) {
        Serial.println(msg);
    };
}

void Log::LogMessage(const char* level, const char* format, va_list args) {
    char buffer[LOG_BUFFER_SIZE];
    snprintf(buffer, LOG_BUFFER_SIZE, "%s", level);
    vsnprintf(buffer + strlen(level), LOG_BUFFER_SIZE - strlen(level), format, args);
    GetInstance().m_LogFunction(buffer);
}
