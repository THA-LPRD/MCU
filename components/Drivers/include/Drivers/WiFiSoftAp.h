#ifndef LPRD_MCU_WIFISOFTAP_H
#define LPRD_MCU_WIFISOFTAP_H

#include <string_view>
#include "esp_event.h"
#include "esp_netif.h"
#include <lwip/inet.h>

class WiFiSoftAP {
public:
    WiFiSoftAP() = default;
    ~WiFiSoftAP();
    bool Start(std::string_view ssid, std::string_view password);
    bool Stop();
    ip4_addr_t GetIP();
private:
    bool InitializeNetIf();
    bool InitializeWiFi();
    bool RegisterEventHandlers();
    bool ConfigureSettings(std::string_view ssid, std::string_view password);
private:
    static constexpr const char* LOG_TAG = "[WiFi SoftAP] -";
    bool m_Active = false;
    esp_event_handler_instance_t m_Instance = nullptr;
    esp_netif_t* m_NetIf = nullptr;
};

#endif //LPRD_MCU_WIFISOFTAP_H
