#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Application.h"
#include "AppDefault.h"
#include "AppStandalone.h"
#include "AppNetwork.h"
//#include "AppServer.h"
#include <magic_enum.hpp>
#include <esp_system.h>

Application* Application::Create(Application::Mode mode, std::shared_ptr<spdlog::logger> logger) {
    Application* app = nullptr;

    switch (mode) {
        case Application::Mode::Default:
            app = new AppDefault(logger);
            break;
        case Application::Mode::Standalone:
            app = new AppStandalone(logger);
            break;
        case Application::Mode::Network:
            app = new AppNetwork(logger);
            break;
        case Application::Mode::Server:
//            app = new AppServer();
            break;
    }

    if (app == nullptr) {
        logger->critical("Failed to create application in mode {}", magic_enum::enum_name(mode));
        for (int i = 0; i < 5; i++) {
            logger->critical("Restarting in {} seconds", 5 - i);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        esp_restart();
    }
    return app;
}


Application* Application::Create(std::string_view mode, std::shared_ptr<spdlog::logger> logger) {
    return Create(magic_enum::enum_cast<Application::Mode>(mode.data()).value(), logger);
}

void Application::CoreInit() {
    std::unordered_map<Application::Config, std::string> defaultConfig = {
            {Application::Config::OperatingMode, "Default"},
            {Application::Config::DisplayDriver, "WS_7IN3G"},
            {Application::Config::LogLevel,      "Info"},
    };
    m_ConfigManagerCore = std::make_unique<ConfigManager<Config>>(m_Logger, defaultConfig);
    m_ConfigManagerCore->SetValidator(Config::OperatingMode, [this](std::string_view value) {
        return magic_enum::enum_cast<Mode>(value.data()).has_value();
    });
//    m_ConfigManagers[Core]->SetValidator(Application::Config::DisplayDriver, [this](std::string_view value) {
//        return magic_enum::enum_cast<DisplayDriver>(value.data()).has_value();
//    });
    m_ConfigManagerCore->SetValidator(Config::LogLevel, [this](std::string_view value) {
        return magic_enum::enum_cast<LogLevel>(value.data()).has_value();
    });

    SetupConfigStandalone();
    SetupConfigNetwork();
    SetupConfigServer();
}

void Application::SetupConfigStandalone() {
    std::unordered_map<AppStandaloneConfig, std::string> defaultConfig = {
            {AppStandaloneConfig::WiFiSSID,     "SSID"},
            {AppStandaloneConfig::WiFiPassword, "Password"},
    };
    m_ConfigManagerStandalone = std::make_unique<ConfigManager<AppStandaloneConfig>>(m_Logger, defaultConfig);
    m_ConfigManagerStandalone->SetValidator(AppStandaloneConfig::WiFiSSID, [](std::string_view value) {
        return !value.empty();
    });
    m_ConfigManagerStandalone->SetValidator(AppStandaloneConfig::WiFiPassword, [](std::string_view value) {
        return !value.empty();
    });
}

void Application::SetupConfigNetwork() {
    std::unordered_map<AppNetworkConfig, std::string> defaultConfig = {
            {AppNetworkConfig::WiFiSSID,     "Testing"},
            {AppNetworkConfig::WiFiPassword, "hGk9xqv_x*2!DMNbRRr*_.st_w*u4@Lq"}
    };

    m_ConfigManagerNetwork = std::make_unique<ConfigManager<AppNetworkConfig>> (m_Logger, defaultConfig);
    m_ConfigManagerNetwork->SetValidator(AppNetworkConfig::WiFiSSID, [](std::string_view value) {
        return !value.empty();
    });
    m_ConfigManagerNetwork->SetValidator(AppNetworkConfig::WiFiPassword, [](std::string_view value) {
        return value.size() > 8;
    });
}

void Application::SetupConfigServer() {
    std::unordered_map<AppServerConfig, std::string> defaultConfig = {
            {AppServerConfig::WiFiSSID,     "Testing"},
            {AppServerConfig::WiFiPassword, "hGk9xqv_x*2!DMNbRRr*_.st_w*u4@Lq"},
            {AppServerConfig::ServerURL,    "http://example.com"}
    };

    m_ConfigManagerServer = std::make_unique<ConfigManager<AppServerConfig>> (m_Logger, defaultConfig);
    m_ConfigManagerServer->SetValidator(AppServerConfig::WiFiSSID, [](std::string_view value) {
        return !value.empty();
    });
    m_ConfigManagerServer->SetValidator(AppServerConfig::WiFiPassword, [](std::string_view value) {
        return value.size() > 8;
    });
    m_ConfigManagerServer->SetValidator(AppServerConfig::ServerURL, [](std::string_view value) {
        return !value.empty();
    });
}
