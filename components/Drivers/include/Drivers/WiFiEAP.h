#ifndef LPRD_MCU_WIFIEAP_H
#define LPRD_MCU_WIFIEAP_H

#include <string_view>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <lwip/inet.h>

// Event group bits
static const int WIFI_CONNECTED_BIT = BIT0;
static const int WIFI_FAIL_BIT = BIT1;

struct EAPContext {
    const char* LOG_TAG{};
    int RetryN = 0;
    int RetryMax = 5;
    EventGroupHandle_t EventGroup{};
};

class WiFiEAP {
public:
    WiFiEAP();
    virtual ~WiFiEAP();
    bool Connect(std::string_view ssid, std::string_view password, std::string_view anonymous_identity, 
                std::string_view username,
                std::string_view ca_cert_path,
                int retryMax = 5);

    // Hide WiFiStation::Connect
    // bool Connect(std::string_view ssid, std::string_view password, int retryMax = 5) {
    //     return false; // Nicht erlaubt für Eduroam
    // }
    bool Disconnect();
    ip4_addr_t GetIP();

protected:
    bool InitializeNetIf();
    bool InitializeWiFi();
    bool RegisterEventHandlers(EAPContext* context);
    virtual bool ConfigureSettings(std::string_view ssid, std::string_view password, std::string_view anonymous_identity, std::string_view username);

    static constexpr const char* LOG_TAG = "[WiFi EAP] -";
    bool m_Active = false;
    esp_event_handler_instance_t m_InstanceAnyId = nullptr;
    esp_event_handler_instance_t m_InstanceGotIp = nullptr;
    EAPContext m_Context;
    esp_netif_t* m_NetIf = nullptr;
};

#endif //LPRD_MCU_WIFIEAP_H