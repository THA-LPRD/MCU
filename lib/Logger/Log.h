#ifndef LOGGER_LOG_H_
#define LOGGER_LOG_H_

#include <functional>

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 256
#endif

namespace Log
{
    enum class Level {
        FATAL,
        ERROR,
        WARNING,
        INFO,
        DEBUG
    };

    void SetLogLevel(Level level);
    void SetLogFunction(std::function<void(char* msg)> logFunction);
    void Debug(const char* format, ...);
    void Info(const char* format, ...);
    void Warning(const char* format, ...);
    void Error(const char* format, ...);
    void Fatal(const char* format, ...);
} // namespace Log


#endif /*LOGGER_LOG_H_*/
