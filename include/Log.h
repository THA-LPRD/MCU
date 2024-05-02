#ifndef LOG_H_
#define LOG_H_

#include <cstdarg>
#include <functional>

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 256
#endif

class Log {
public:
    enum class Level {
        FATAL,
        ERROR,
        WARNING,
        INFO,
        DEBUG
    };

    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;
    inline static Log& GetInstance() {
        static Log m_Instance;
        return m_Instance;
    }
    inline static void SetLogLevel(Level level) {
        GetInstance().m_LogLevel = level;
    }
    inline static void SetLogFunction(std::function<void(char* msg)> logFunction) {
        GetInstance().m_LogFunction = logFunction;
    }
    static void Debug(const char* format, ...);
    static void Info(const char* format, ...);
    static void Warning(const char* format, ...);
    static void Error(const char* format, ...);
    static void Fatal(const char* format, ...);
private:
    Log();
    static void LogMessage(const char* level, const char* format, va_list args);
private:
    Level m_LogLevel;
    std::function<void(char* msg)> m_LogFunction;
};

#endif /*LOG_H_*/
