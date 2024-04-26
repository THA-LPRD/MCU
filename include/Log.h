#include <Arduino.h>
#include <cstdarg>
#include <functional>

#ifndef DEFAULT_BUFFER_SIZE
#define DEFAULT_BUFFER_SIZE 256
#endif

class Log {
public:
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;

    static Log& GetInstance() {
        static Log m_Instance;
        return m_Instance;
    }

    static void Debug(const char* format, ...) {
        va_list args;
        va_start(args, format);
        LogMessage("DEBUG - ", format, args);
        va_end(args);
    }

    static void Info(const char* format, ...) {
        va_list args;
        va_start(args, format);
        LogMessage("INFO - ", format, args);
        va_end(args);
    }

    static void Warn(const char* format, ...) {
        va_list args;
        va_start(args, format);
        LogMessage("WARN - ", format, args);
        va_end(args);
    }

    static void Error(const char* format, ...) {
        va_list args;
        va_start(args, format);
        LogMessage("ERROR - ", format, args);
        va_end(args);
    }

    static void Fatal(const char* format, ...) {
        va_list args;
        va_start(args, format);
        LogMessage("FATAL - ", format, args);
        va_end(args);
    }

    static void SetLogFunction(std::function<void(char* msg)> logFunction) {
        GetInstance().m_LogFunction = logFunction;
    }

private:
    Log() {
        m_LogFunction = [](char* msg) {
            Serial.println(msg);
        };
    }

    static void LogMessage(const char* level, const char* format, va_list args) {
        char buffer[DEFAULT_BUFFER_SIZE];
        snprintf(buffer, DEFAULT_BUFFER_SIZE, "%s", level);
        vsnprintf(buffer + strlen(level), DEFAULT_BUFFER_SIZE - strlen(level), format, args);
        GetInstance().m_LogFunction(buffer);
    }

    std::function<void(char* msg)> m_LogFunction;
};
