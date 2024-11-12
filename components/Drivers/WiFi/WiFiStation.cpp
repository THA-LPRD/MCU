#include "Drivers/WiFi.h"
#include "WiFiStation.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static void StationEventHandler(void* ctx, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    auto* context = static_cast<StationContext*>(ctx);
    if (!context) return;

    spdlog::debug("{} Event: {}", context->LOG_TAG, event_id);

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                if (context->RetryN < context->RetryMax) {
                    esp_wifi_connect();
                    context->RetryN++;
                    spdlog::warn("{} Retrying connection ({}/{})",
                                 context->LOG_TAG,
                                 context->RetryN,
                                 context->RetryMax);
                }
                else {
                    spdlog::error("{} Failed to connect to WiFi", context->LOG_TAG);
                    xEventGroupSetBits(context->EventGroup, WIFI_FAIL_BIT);
                }
                break;
            default:
                break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto* event = (ip_event_got_ip_t*) event_data;
        spdlog::info("{} Connected to WiFi, IP: {}.{}.{}.{}",
                     context->LOG_TAG,
                     IP2STR(&event->ip_info.ip));
        context->RetryN = 0;
        xEventGroupSetBits(context->EventGroup, WIFI_CONNECTED_BIT);
    }

    spdlog::debug("{} Event handled", context->LOG_TAG);
}

WiFiStation::WiFiStation() = default;

WiFiStation::~WiFiStation() {
    Disconnect();
}

// TODO: Clean up when failing
bool WiFiStation::Connect(std::string_view ssid, std::string_view password, int retryMax) {
    if (m_Active) {
        spdlog::warn("{} Already connected", LOG_TAG);
        return false;
    }

    spdlog::debug("{} Connecting to WiFi: {}", LOG_TAG, ssid);

    m_Context = {
            .LOG_TAG = LOG_TAG,
            .RetryN = 0,
            .RetryMax = retryMax,
            .EventGroup = xEventGroupCreate()
    };

    if (!InitializeNetIf() || !InitializeWiFi() ||
        !RegisterEventHandlers(&m_Context) || !ConfigureSettings(ssid, password)) {
        return false;
    }


    EventBits_t bits = xEventGroupWaitBits(m_Context.EventGroup,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    bool success = (bits & WIFI_CONNECTED_BIT) != 0;
    m_Active = success;

    return success;
}

bool WiFiStation::Disconnect() {
    if (!m_Active) return true;

    spdlog::debug("{} Disconnecting from WiFi", LOG_TAG);

    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, m_InstanceAnyId);
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, m_InstanceGotIp);

    vEventGroupDelete(m_Context.EventGroup);

    esp_err_t ret = esp_wifi_disconnect();
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to disconnect: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_stop();
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to stop WiFi: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_deinit();
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to deinit WiFi: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    spdlog::info("{} Disconnected from WiFi", LOG_TAG);

    m_Active = false;
    return true;
}

ip4_addr_t WiFiStation::GetIP() {
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(m_NetIf, &ip_info);
    return {ip_info.ip.addr};
}

bool WiFiStation::InitializeNetIf() {
    m_NetIf = esp_netif_create_default_wifi_sta();
    if (m_NetIf == nullptr) {
        spdlog::error("{} Failed to create station interface", LOG_TAG);
        return false;
    }
    return true;
}

bool WiFiStation::InitializeWiFi() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // IDF claims to have lower energy consumption with their settings but at a cost of ~100KB of RAM
    cfg.static_rx_buf_num = 4; // 10 IDF, 4 Arduino
    cfg.dynamic_rx_buf_num = 32; // 32 IDF, 32 Arduino
    cfg.tx_buf_type = 1; // 0 IDF, 1 Arduino
    cfg.static_tx_buf_num = 0; // 16 IDF, 0 Arduino
    cfg.dynamic_tx_buf_num = 32; // 0 IDF, 32 Arduino
    cfg.cache_tx_buf_num = 4; // 32 IDF, 4 Arduino


    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to init WiFi: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool WiFiStation::RegisterEventHandlers(StationContext* context) {
    esp_err_t ret = esp_event_handler_instance_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID,
            StationEventHandler, context, &m_InstanceAnyId
    );
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to register WIFI handler: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    ret = esp_event_handler_instance_register(
            IP_EVENT, IP_EVENT_STA_GOT_IP,
            StationEventHandler, context, &m_InstanceGotIp
    );
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to register IP handler: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    return true;
}

bool WiFiStation::ConfigureSettings(std::string_view ssid, std::string_view password) {
    wifi_config_t wifi_config = {};

    wifi_config.sta.listen_interval = 3;
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    strncpy((char*) wifi_config.sta.ssid, ssid.data(), sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*) wifi_config.sta.password, password.data(), sizeof(wifi_config.sta.password) - 1);

    esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to set mode: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to set config: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to start: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_set_inactive_time(WIFI_IF_STA, 6);
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to set inactive time: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    return true;
}
