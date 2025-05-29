#include "Drivers/WiFi.h"
#include "Drivers/WiFiEAP.h"
#include "esp_wifi.h"
#include "esp_eap_client.h"
#include "esp_netif.h"


/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */

/* CA cert, taken from ca.pem
   Client cert, taken from client.crt
   Client key, taken from client.key

   The PEM, CRT and KEY file were provided by the person or organization
   who configured the AP with wifi enterprise.

   To embed it in the app binary, the PEM, CRT and KEY file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/


// esp_eap_ttls_phase2_types TTLS_PHASE2_METHOD = ESP_EAP_TTLS_PHASE2_PAP;
/* CONFIG_EXAMPLE_EAP_METHOD_TTLS */


#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static void EAPEventHandler(void* ctx, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    auto* context = static_cast<EAPContext*>(ctx);
    if (!context) return;

    spdlog::debug("{} Event: {}", context->LOG_TAG, event_id);

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                spdlog::debug("{} STA Disconnected: {}", context->LOG_TAG, event_data);
                // spdlog::debug("{} STA Disconnected: {}", context->LOG_TAG, context);
                if (context->RetryN < context->RetryMax) {
                    esp_wifi_connect();
                    xEventGroupClearBits(context->EventGroup, WIFI_CONNECTED_BIT);
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

WiFiEAP::WiFiEAP() = default;

WiFiEAP::~WiFiEAP() {
    Disconnect();
}

bool WiFiEAP::Connect(std::string_view ssid, std::string_view password, std::string_view anonymous_identity, std::string_view username, std::string_view ca_cert_path, int retryMax) {
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
        !RegisterEventHandlers(&m_Context) || !ConfigureSettings(ssid, password, anonymous_identity, username)) {
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

bool WiFiEAP::Disconnect() {
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

ip4_addr_t WiFiEAP::GetIP() {
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(m_NetIf, &ip_info);
    return {ip_info.ip.addr};
}

bool WiFiEAP::InitializeNetIf() {
    // esp_netif_init(); // Bereits in WiFi.InitEventLoop();
    // esp_event_loop_create_default(); // Bereits in WiFi.InitEventLoop();
    m_NetIf = esp_netif_create_default_wifi_sta();
    // assert(m_NetIf);
    if (m_NetIf == nullptr) {
        spdlog::error("{} Failed to create station interface", LOG_TAG);
        return false;
    }
    return true;
}

bool WiFiEAP::InitializeWiFi() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // IDF claims to have lower energy consumption with their settings but at a cost of ~100KB of RAM
    /*
    cfg.static_rx_buf_num = 4; // 10 IDF, 4 Arduino
    cfg.dynamic_rx_buf_num = 32; // 32 IDF, 32 Arduino
    cfg.tx_buf_type = 1; // 0 IDF, 1 Arduino
    cfg.static_tx_buf_num = 0; // 16 IDF, 0 Arduino
    cfg.dynamic_tx_buf_num = 32; // 0 IDF, 32 Arduino
    cfg.cache_tx_buf_num = 4; // 32 IDF, 4 Arduino
    */

    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to init WiFi: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool WiFiEAP::RegisterEventHandlers(EAPContext* context) {
    esp_err_t ret = esp_event_handler_instance_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID,
            EAPEventHandler, context, &m_InstanceAnyId
    );
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to register WIFI handler: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    ret = esp_event_handler_instance_register(
            IP_EVENT, IP_EVENT_STA_GOT_IP,
            EAPEventHandler, context, &m_InstanceGotIp
    );
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to register IP handler: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    return true;
}

bool WiFiEAP::ConfigureSettings(std::string_view ssid, std::string_view password, std::string_view anonymous_identity, std::string_view username) {
    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    wifi_config_t wifi_config = {};

    // Auch im EAP?
    // wifi_config.sta.listen_interval = 3;
    // wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    // wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

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

    ret = esp_eap_client_set_identity((uint8_t *)anonymous_identity.data(), strlen(anonymous_identity.data()));
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to set identity: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    ret = esp_eap_client_set_username((uint8_t *)username.data(), strlen(username.data()));
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to set username: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    ret = esp_eap_client_set_password((uint8_t *)password.data(), strlen(password.data()));
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to set password: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    // esp_eap_ttls_phase2_types TTLS_PHASE2_METHOD = ESP_EAP_TTLS_PHASE2_PAP;

    ret = esp_eap_client_set_ttls_phase2_method(ESP_EAP_TTLS_PHASE2_PAP);
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to set TTLS Phase 2 Method: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    // ret = esp_eap_client_use_default_cert_bundle(true);
    // if (ret != ESP_OK) {
    //     spdlog::error("{} Failed to set default cert bundle: {}", LOG_TAG, esp_err_to_name(ret));
    //     return false;
    // }

    ret = esp_wifi_sta_enterprise_enable();
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to enable enterprise: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to start: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    // In EAP notwendig?
    ret = esp_wifi_set_inactive_time(WIFI_IF_STA, 6);
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to set inactive time: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    return true;
}
