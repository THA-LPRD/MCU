#ifndef LPRD_MCU_WIFI_H
#define LPRD_MCU_WIFI_H

#include <memory>
#include <string_view>
#include <spdlog/spdlog.h>
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "lwip/ip4_addr.h"

class WiFiStation;

class WiFiSoftAP;

class WiFi {
public:
    enum class Mode {
        Station,
        SoftAP
    };

    WiFi();
    ~WiFi();
    bool Connect(Mode mode, std::string_view ssid, std::string_view password, int retryMax = 5);
    bool Disconnect(Mode mode);
    static std::string GetMAC() {
        uint8_t mac[6];
        char macStr[18];
        esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return {macStr};
    }
    ip4_addr_t GetIP(Mode mode);
    bool ConfigureSNTP(std::string_view server = "");
    void StopSNTP();
private:
    bool InitEventLoop();
    void DeinitEventLoop();
private:
    static constexpr const char* LOG_TAG = "[WiFi] -";
    std::unique_ptr<WiFiStation> m_Station;
    std::unique_ptr<WiFiSoftAP> m_SoftAP;
    bool m_EventLoopInitialized = false;
    bool m_SNTPInitialized = false;
    bool m_SNTPStarted = false;
    std::string m_SNTPServer = "pool.ntp.org";
};

#endif //LPRD_MCU_WIFI_H
