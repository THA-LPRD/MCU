#include "Drivers/WiFi.h"
#include "Drivers/WiFiSoftAp.h"
#include "freertos/FreeRTOS.h"

static void SoftAPEventHandler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    const char* LOG_TAG = static_cast<const char*>(arg);

    switch (event_id) {
        case WIFI_EVENT_AP_STACONNECTED: {
            auto* event = (wifi_event_ap_staconnected_t*) event_data;
            spdlog::info("{} Station joined, MAC: {:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
                         LOG_TAG, MAC2STR(event->mac));
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED: {
            auto* event = (wifi_event_ap_stadisconnected_t*) event_data;
            spdlog::info("{} Station left, MAC: {:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
                         LOG_TAG, MAC2STR(event->mac));
            break;
        }
        case IP_EVENT_AP_STAIPASSIGNED: {
            auto* event = (ip_event_ap_staipassigned_t*) event_data;
            const uint8_t* mac = event->mac;
            const auto* ip = reinterpret_cast<const ip4_addr_t*>(&event->ip);
            spdlog::info("{} Station assigned IP: {:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x} -> {}",
                         LOG_TAG, MAC2STR(mac), ip4addr_ntoa(ip));
            break;
        }
        default:
            break;
    }
}

WiFiSoftAP::~WiFiSoftAP() {
    Stop();
}

bool WiFiSoftAP::Start(std::string_view ssid, std::string_view password) {
    if (m_Active) {
        spdlog::warn("{} Already active", LOG_TAG);
        return false;
    }

    spdlog::debug("{} Starting SoftAP", LOG_TAG);

    if (!InitializeNetIf() || !InitializeWiFi() ||
        !RegisterEventHandlers() || !ConfigureSettings(ssid, password)) {
        return false;
    }

    m_Active = true;
    ip4_addr_t ip = GetIP();
    spdlog::info("{} Access Point '{}' is up and running at {}", LOG_TAG, ssid, ip4addr_ntoa(&ip));
    return true;
}

bool WiFiSoftAP::Stop() {
    if (!m_Active) return true;

    spdlog::debug("{} Stopping SoftAP", LOG_TAG);

    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, m_Instance);

    esp_err_t ret = esp_wifi_stop();
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to stop WiFi: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_deinit();
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to deinit WiFi: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    m_Active = false;
    spdlog::info("{} SoftAP stopped successfully", LOG_TAG);
    return true;
}

ip4_addr_t WiFiSoftAP::GetIP() {
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(m_NetIf, &ip_info);
    return {ip_info.ip.addr};
}

bool WiFiSoftAP::InitializeNetIf() {
    m_NetIf = esp_netif_create_default_wifi_ap();
    if (m_NetIf == nullptr) {
        spdlog::error("{} Failed to create AP interface", LOG_TAG);
        return false;
    }
    return true;
}

bool WiFiSoftAP::InitializeWiFi() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // IDF claims to have lower energy consumption with their settings but at a cost of ~100KB of RAM
    cfg.static_rx_buf_num = 4; // 10 IDF, 4 Arduino
    cfg.dynamic_rx_buf_num = 32; // 32 IDF, 32 Arduino
    cfg.tx_buf_type = 1; // 0 IDF, 1 Arduino
    cfg.static_tx_buf_num = 0; // 16 IDF, 0 Arduino
    cfg.dynamic_tx_buf_num = 32; // 0 IDF, 32 Arduino
    cfg.cache_tx_buf_num = 4; // 32 IDF, 4 Arduino
    ;

    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to init WiFi: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool WiFiSoftAP::RegisterEventHandlers() {
    esp_err_t ret = esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            SoftAPEventHandler,
            const_cast<char*>(LOG_TAG),
            &m_Instance
    );

    if (ret != ESP_OK) {
        spdlog::error("{} Failed to register event handler: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool WiFiSoftAP::ConfigureSettings(std::string_view ssid, std::string_view password) {
    wifi_config_t wifi_config = {};

    // Configure SSID
    if (ssid.length() > sizeof(wifi_config.ap.ssid) - 1) {
        spdlog::error("{} SSID too long (max {} chars)", LOG_TAG, sizeof(wifi_config.ap.ssid) - 1);
        return false;
    }
    strncpy((char*) wifi_config.ap.ssid, ssid.data(), sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid_len = ssid.length();

    // Configure security settings
    if (password.empty()) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    else {
        if (password.length() < 8 || password.length() > sizeof(wifi_config.ap.password) - 1) {
            spdlog::error("{} Password must be between 8 and {} chars", LOG_TAG,
                          sizeof(wifi_config.ap.password) - 1);
            return false;
        }
        strncpy((char*) wifi_config.ap.password, password.data(), sizeof(wifi_config.ap.password) - 1);
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_WPA3_PSK;
    }

    // Additional AP settings
    srand(time(nullptr));
    wifi_config.ap.channel = static_cast<uint8_t>((rand() % 11) + 1); // Random channel 1-13
    wifi_config.ap.max_connection = 1;
    wifi_config.ap.pmf_cfg.required = true;
    wifi_config.ap.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    wifi_config.ap.pmf_cfg.required = true;

    // Apply configuration
    esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to set mode: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to set config: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to start: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    return true;
}
