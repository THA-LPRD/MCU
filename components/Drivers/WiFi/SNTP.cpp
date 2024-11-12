#include "Drivers/WiFi.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"

static void SNTPNotificationCallback(struct timeval* tv) {
    spdlog::info("[SNTP] Time synchronized");
}

bool WiFi::ConfigureSNTP(std::string_view server) {
    if (!m_EventLoopInitialized && !InitEventLoop()) {
        return false;
    }
    if (m_SNTPInitialized) {
        spdlog::warn("[SNTP] SNTP already initialized");
        return false;
    }

    spdlog::debug("[SNTP] Initializing SNTP");
    esp_sntp_config_t config;

    if (server.empty()) {
        spdlog::warn("[SNTP] No SNTP server provided, using default SNTP server: pool.ntp.org");
        config = ESP_NETIF_SNTP_DEFAULT_CONFIG(m_SNTPServer.c_str());
    }
    else {
        m_SNTPServer = server;
        spdlog::info("[SNTP] Using custom SNTP server: {} along with dhcp recommended server", m_SNTPServer);
        config = ESP_NETIF_SNTP_DEFAULT_CONFIG(m_SNTPServer.c_str());
    }

    config.sync_cb = SNTPNotificationCallback;

    if (m_Station) {
        spdlog::warn("[SNTP] WiFi station is connected, won't be able to use DHCP recommended server");
        m_SNTPStarted = true;
    }
    else {
        config.start = false;
        config.server_from_dhcp = true;
        config.renew_servers_after_new_IP = true;
        config.index_of_first_server = 1;
        config.ip_event_to_renew = IP_EVENT_STA_GOT_IP;

    }

    esp_err_t ret = esp_netif_sntp_init(&config);
    if (ret != ESP_OK) {
        spdlog::error("[SNTP] Failed to initialize SNTP: {}", esp_err_to_name(ret));
        m_SNTPStarted = false;
        return false;
    }

    m_SNTPInitialized = true;
    setenv("TZ", "UTC0", 1);
    tzset();
    return true;
}

void WiFi::StopSNTP() {
    if (!m_SNTPStarted) {
        spdlog::warn("[SNTP] SNTP not started");
        return;
    }

    spdlog::debug("[SNTP] Stopping SNTP");

    esp_netif_sntp_deinit();

    m_SNTPStarted = false;
    spdlog::info("[SNTP] SNTP stopped");
}