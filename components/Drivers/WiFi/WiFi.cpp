#include "Drivers/WiFi.h"
#include "WiFiSoftAp.h"
#include "WiFiStation.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"

WiFi::WiFi() {
    InitEventLoop();
}

WiFi::~WiFi() {
    spdlog::info("{} Destroying WiFi", LOG_TAG);
    m_Station.reset();
    m_SoftAP.reset();
    DeinitEventLoop();
}

bool WiFi::Connect(Mode mode, std::string_view ssid, std::string_view password, int retryMax) {
    if (!m_EventLoopInitialized && !InitEventLoop()) {
        return false;
    }

    switch (mode) {
        case Mode::Station:
            if (!m_Station) {
                m_Station = std::make_unique<WiFiStation>();
            }
            if (!m_Station->Connect(ssid, password, retryMax)) {
                return false;
            }
            if (m_SNTPInitialized && !m_SNTPStarted) {
                spdlog::info("[WiFi] Starting SNTP");
                esp_netif_sntp_start();
                esp_sntp_setservername(1, m_SNTPServer.c_str());
                m_SNTPStarted = true;
            }
            return true;
        case Mode::SoftAP:
            if (!m_SoftAP) {
                m_SoftAP = std::make_unique<WiFiSoftAP>();
            }
            return m_SoftAP->Start(ssid, password);
    }

    return false;
}

bool WiFi::Disconnect(Mode mode) {
    switch (mode) {
        case Mode::Station:
            if (m_Station) {
                m_Station.reset();
            }
            break;

        case Mode::SoftAP:
            if (m_SoftAP) {
                m_SoftAP.reset();
            }
            break;
    }

    if (!m_Station && !m_SoftAP) {
        DeinitEventLoop();
    }

    return true;
}

ip4_addr_t WiFi::GetIP(Mode mode) {
    switch (mode) {
        case Mode::Station:
            if (m_Station) {
                return m_Station->GetIP();
            }
            break;

        case Mode::SoftAP:
            if (m_SoftAP) {
                return m_SoftAP->GetIP();
            }
            break;
    }

    return {};
}

bool WiFi::InitEventLoop() {
    if (m_EventLoopInitialized) {
        return true;
    }

    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to initialize netif: {}", LOG_TAG, esp_err_to_name(ret));
        return false;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to create event loop: {}", LOG_TAG, esp_err_to_name(ret));
        esp_netif_deinit();
        return false;
    }

    m_EventLoopInitialized = true;
    return true;
}

void WiFi::DeinitEventLoop() {
    if (m_EventLoopInitialized) {
        esp_event_loop_delete_default();
        esp_netif_deinit();
        m_EventLoopInitialized = false;
    }
}
