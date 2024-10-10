#ifndef LPRD_MCU_APPLICATION_H
#define LPRD_MCU_APPLICATION_H

#include <string_view>
#include <spdlog/spdlog.h>
#include <memory>
#include <ConfigManager.h>
#include "../HTTPServer/HTTPServer.h"

class Application {
public:
    enum class Mode {
        Default,
        Standalone,
        Network,
        Server
    };
    enum class LogLevel {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical
    };
    enum class Config {
        OperatingMode,
        DisplayDriver,
        LogLevel,
    };
    enum class AppStandaloneConfig {
        WiFiSSID,
        WiFiPassword,
    };
    enum class AppNetworkConfig {
        WiFiSSID,
        WiFiPassword,
    };
    enum class AppServerConfig {
        WiFiSSID,
        WiFiPassword,
        ServerURL,
    };
    Application() = default;
    virtual ~Application() = default;
    virtual bool Init() = 0;
    virtual void Run() = 0;
    static Application* Create(Mode mode, std::shared_ptr<spdlog::logger> logger);
    static Application* Create(std::string_view mode, std::shared_ptr<spdlog::logger> logger);
protected:
    void CoreInit();
private:
    void SetupConfigStandalone();
    void SetupConfigNetwork();
    void SetupConfigServer();
protected:
    std::shared_ptr<spdlog::logger> m_Logger;
    std::unique_ptr<ConfigManager<Config>> m_ConfigManagerCore;
    std::unique_ptr<ConfigManager<AppStandaloneConfig>> m_ConfigManagerStandalone;
    std::unique_ptr<ConfigManager<AppNetworkConfig>> m_ConfigManagerNetwork;
    std::unique_ptr<ConfigManager<AppServerConfig>> m_ConfigManagerServer;
};

#endif //LPRD_MCU_APPLICATION_H
