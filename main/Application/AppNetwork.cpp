#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "AppNetwork.h"
#include <cstring>
#include "esp_system.h"
#include <map>

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

AppNetwork::AppNetwork(std::shared_ptr<spdlog::logger> logger) {
    m_Logger = logger;
    m_Context.logger = m_Logger;
    m_Context.EventGroup = xEventGroupCreate();

    Application::CoreInit();

    std::shared_ptr<spdlog::logger> Logger_HTTPServer = spdlog::stdout_color_mt("HTTP Server",
                                                                                spdlog::color_mode::always);
    m_Server = std::make_unique<HTTPServer>(Logger_HTTPServer, "/api/v1");
}

AppNetwork::~AppNetwork() {
    m_Logger->info("Destroying network application");
    if (!Disconnect()) m_Logger->error("Failed to disconnect from network");
}

bool AppNetwork::Init() {
    m_Logger->info("Initializing network application");
    if (!Connect()) return false;
    if (!InitServer()) return false;
    m_Logger->info("Network application initialized");
    return true;
}

void AppNetwork::Run() {
    m_Logger->info("Running network application");
}

bool CheckWiFiCError(esp_err_t ret, spdlog::logger* logger) {
    if (ret != ESP_OK) {
        logger->error("Failed to initialize WiFi: {}", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool AppNetwork::Connect() {
    m_Logger->debug("Configuring WiFi");
    if (!CheckWiFiCError(esp_netif_init(), m_Context.logger.get())) return false;
    if (!CheckWiFiCError(esp_event_loop_create_default(), m_Context.logger.get())) return false;

    if (esp_netif_create_default_wifi_sta() == nullptr) {
        m_Logger->error("Failed to create default WiFi station interface");
        return false;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // IDF claims to have lower energy consumption with their settings but at a cost of ~100KB of RAM
    cfg.static_rx_buf_num = 4; // 10 IDF, 4 Arduino
    cfg.dynamic_rx_buf_num = 32; // 32 IDF, 32 Arduino
    cfg.tx_buf_type = 1; // 0 IDF, 1 Arduino
    cfg.static_tx_buf_num = 0; // 16 IDF, 0 Arduino
    cfg.dynamic_tx_buf_num = 32; // 0 IDF, 32 Arduino
    cfg.cache_tx_buf_num = 4; // 32 IDF, 4 Arduino
    if (!CheckWiFiCError(esp_wifi_init(&cfg), m_Context.logger.get())) return false;

    if (!CheckWiFiCError(esp_event_handler_instance_register(WIFI_EVENT,
                                                             ESP_EVENT_ANY_ID,
                                                             &event_handler,
                                                             &m_Context,
                                                             &m_Context.InstanceAnyId), m_Context.logger.get())) {
        return false;
    }
    if (!CheckWiFiCError(esp_event_handler_instance_register(IP_EVENT,
                                                             IP_EVENT_STA_GOT_IP,
                                                             &event_handler,
                                                             &m_Context,
                                                             &m_Context.InstanceGotIp), m_Context.logger.get())) {
        return false;
    }

    wifi_config_t WiFiConfig = {};

    WiFiConfig.sta.listen_interval = 3;
    WiFiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    WiFiConfig.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    strncpy((char*) WiFiConfig.sta.ssid,
            m_ConfigManagerNetwork->GetString(AppNetworkConfig::WiFiSSID).c_str(),
            sizeof(WiFiConfig.sta.ssid) - 1);
    WiFiConfig.sta.ssid[sizeof(WiFiConfig.sta.ssid) - 1] = '\0';
    strncpy((char*) WiFiConfig.sta.password,
            m_ConfigManagerNetwork->GetString(AppNetworkConfig::WiFiPassword).c_str(),
            sizeof(WiFiConfig.sta.password) - 1);
    WiFiConfig.sta.password[sizeof(WiFiConfig.sta.password) - 1] = '\0';

    if (!CheckWiFiCError(esp_wifi_set_mode(WIFI_MODE_STA), m_Context.logger.get())) return false;
    if (!CheckWiFiCError(esp_wifi_set_config(WIFI_IF_STA, &WiFiConfig), m_Context.logger.get())) return false;
    if (!CheckWiFiCError(esp_wifi_start(), m_Context.logger.get())) return false;
    if (!CheckWiFiCError(esp_wifi_set_inactive_time(WIFI_IF_STA, 6), m_Context.logger.get())) return false;

    m_Logger->debug("Configuration complete, waiting for connection");

    EventBits_t bits = xEventGroupWaitBits(m_Context.EventGroup,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        m_Logger->info("Connected to WiFi: {}", m_ConfigManagerNetwork->GetString(AppNetworkConfig::WiFiSSID));
    }
    else if (bits & WIFI_FAIL_BIT) {
        m_Logger->error("Failed to connect to WiFi");
        return false;
    }
    else {
        m_Logger->error("Unexpected event");
        return false;
    }

    return true;
}

bool CheckWiFiDError(esp_err_t ret, spdlog::logger* logger) {
    if (ret != ESP_OK) {
        logger->error("Failed to deinitialize WiFi: {}", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool AppNetwork::Disconnect() {
    m_Logger->debug("Disconnecting from network");

    if (!CheckWiFiDError(esp_wifi_disconnect(), m_Context.logger.get())) return false;
    if (!CheckWiFiDError(esp_wifi_stop(), m_Context.logger.get())) return false;

    if (!CheckWiFiDError(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, m_Context.InstanceAnyId),
                         m_Context.logger.get())) {
        return false;
    }
    if (!CheckWiFiDError(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, m_Context.InstanceGotIp),
                         m_Context.logger.get())) {
        return false;
    }

    if (!CheckWiFiDError(esp_wifi_deinit(), m_Context.logger.get())) return false;
    if (!CheckWiFiDError(esp_event_loop_delete_default(), m_Context.logger.get())) return false;

    return true;
}

void AppNetwork::event_handler(void* ctx, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    auto* context = static_cast<WiFiContext*>(ctx);

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (context->RetryN < context->RetryMax) {
            esp_wifi_connect();
            context->RetryN++;
            context->logger->warn("WiFi connection lost, retrying. Attempt: {}/{}", context->RetryN, context->RetryMax);
        }
        else {
            context->logger->error("Failed to connect to WiFi");
            xEventGroupSetBits(context->EventGroup, WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto* event = (ip_event_got_ip_t*) event_data;
        context->logger->info("Connected to WiFi. IP: {}.{}.{}.{}, Gateway: {}.{}.{}.{}",
                              IP2STR(&event->ip_info.ip),
                              IP2STR(&event->ip_info.gw));
        context->RetryN = 0;
        xEventGroupSetBits(context->EventGroup, WIFI_CONNECTED_BIT);
    }
}

bool AppNetwork::InitServer() {
    if (!m_Server->Init()) return false;

    std::map<std::string, std::string> filesToServe = {
            {"/index.html",               "/www/index.html"},
            {"/settings.html",            "/www/settings.html"},
            {"/style.css",                "/www/style.css"},
            {"/LPRD-Logo.webp",           "/www/LPRD-Logo.webp"},
            {"/icons8-settings-25-w.png", "/www/icons8-settings-25-w.png"},
            {"/html2canvas.min.js",       "/www/html2canvas.min.js"},
            {"/utils.js",                 "/www/utils.js"},
            {"/script.js",                "/www/script.js"},
            {"/layouts",                  "/www/layouts"},
    };

    m_Server->SetFilesToServe(filesToServe);

    InitServerCore();
    InitServerStandalone();
    InitServerNetwork();
    InitServerServer();
    InitServerHTTP();

    m_Server->AddUploadEndpoint(
            "/api/v1/UploadImg",
            [](std::string_view filename) {
                return "/" + std::string("img.png");
            },
            [](std::string_view filename) {}
    );

    m_Server->AddEndpoint(
            "/api/v1/restart",
            http_method::HTTP_POST,
            [this](PsychicRequest* request) -> esp_err_t {
                m_Logger->info("Received {} request from client {}", request->uri().c_str(),
                               request->client()->remoteIP().toString().c_str());
                m_Logger->trace("Body: {}", request->body().c_str());
                request->reply(200, "text/plain", "OK");
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                // TODO: Make sure all saved
                esp_restart();
            }
    );

    return true;
}

void AppNetwork::InitServerCore() {
    m_Server->CreateVariable(
            [this]() { return m_ConfigManagerCore->GetString(Config::OperatingMode); },
            [this](std::string_view value) {
                return m_ConfigManagerCore->Set(Config::OperatingMode, value);
            },
            "OpMode"
    );

    m_Server->CreateVariable(
            [this]() { return m_ConfigManagerCore->GetString(Config::DisplayDriver); },
            [this](std::string_view value) {
                return m_ConfigManagerCore->Set(Config::DisplayDriver, value);
            },
            "displayModule"
    );

    m_Server->CreateVariable(
            [this]() { return m_ConfigManagerCore->GetString(Config::LogLevel); },
            [this](std::string_view value) {
                return m_ConfigManagerCore->Set(Config::LogLevel, value);
            },
            "LogLevel"
    );
}

void AppNetwork::InitServerStandalone() {
    m_Server->CreateVariable(
            [this]() { return m_ConfigManagerStandalone->GetString(AppStandaloneConfig::WiFiSSID); },
            [this](std::string_view value) {
                return m_ConfigManagerStandalone->Set(AppStandaloneConfig::WiFiSSID, value);
            },
            "SSID",
            false, true
    );

    m_Server->CreateVariable(
            [this]() { return m_ConfigManagerStandalone->GetString(AppStandaloneConfig::WiFiPassword); },
            [this](std::string_view value) {
                return m_ConfigManagerStandalone->Set(AppStandaloneConfig::WiFiPassword, value);
            },
            "Password",
            false, true
    );
}

void AppNetwork::InitServerNetwork() {
    m_Server->CreateVariable(
            [this]() { return m_ConfigManagerNetwork->GetString(AppNetworkConfig::WiFiSSID); },
            [this](std::string_view value) { return m_ConfigManagerNetwork->Set(AppNetworkConfig::WiFiSSID, value); },
            "SSID",
            false, true
    );

    m_Server->CreateVariable(
            [this]() { return m_ConfigManagerNetwork->GetString(AppNetworkConfig::WiFiPassword); },
            [this](std::string_view value) {
                return m_ConfigManagerNetwork->Set(AppNetworkConfig::WiFiPassword,
                                                   value);
            },
            "Password",
            false, true
    );
}

void AppNetwork::InitServerServer() {
    m_Server->CreateVariable(
            [this]() { return m_ConfigManagerServer->GetString(AppServerConfig::WiFiSSID); },
            [this](std::string_view value) {
                return m_ConfigManagerServer->Set(AppServerConfig::WiFiSSID, value);
            },
            "SSID",
            false, true
    );

    m_Server->CreateVariable(
            [this]() { return m_ConfigManagerServer->GetString(AppServerConfig::WiFiPassword); },
            [this](std::string_view value) {
                return m_ConfigManagerServer->Set(AppServerConfig::WiFiPassword, value);
            },
            "Password",
            false, true
    );

    m_Server->CreateVariable(
            [this]() { return m_ConfigManagerServer->GetString(AppServerConfig::ServerURL); },
            [this](std::string_view value) {
                return m_ConfigManagerServer->Set(AppServerConfig::ServerURL, value);
            },
            "ServerURL"
    );
}

void AppNetwork::InitServerHTTP() {
    auto* server = m_Server.get();
    m_Server->CreateVariable(
            [server]() { return server->Get(HTTPServer::Config::HTTPPort); },
            [server](std::string_view value) { return server->Set(HTTPServer::Config::HTTPPort, value); },
            "HttpPort"
    );

    m_Server->CreateVariable(
            [server]() { return server->Get(HTTPServer::Config::HTTPSPort); },
            [server](std::string_view value) {
                return server->Set(HTTPServer::Config::HTTPSPort, value);
            },
            "HttpsPort"
    );

    m_Server->CreateVariable(
            [server]() { return server->Get(HTTPServer::Config::HTTPS); },
            [server](std::string_view value) {
                return server->Set(HTTPServer::Config::HTTPS, value);
            },
            "Https"
    );

    m_Server->CreateVariable(
            [server]() { return server->Get(HTTPServer::Config::HTTPAuth); },
            [server](std::string_view value) {
                return server->Set(HTTPServer::Config::HTTPAuth, value);
            },
            "HttpAuth"
    );

    m_Server->CreateVariable(
            [server]() { return server->Get(HTTPServer::Config::HTTPAuthUser); },
            [server](std::string_view value) {
                return server->Set(HTTPServer::Config::HTTPAuthUser, value);
            },
            "HttpAuthUser"
    );

    m_Server->CreateVariable(
            [server]() { return server->Get(HTTPServer::Config::HTTPAuthPass); },
            [server](std::string_view value) {
                return server->Set(HTTPServer::Config::HTTPAuthPass, value);
            },
            "HttpAuthPass"
    );
}