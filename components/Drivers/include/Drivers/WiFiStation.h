#ifndef LPRD_MCU_WIFISTATION_H
#define LPRD_MCU_WIFISTATION_H

#include <string_view>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <lwip/inet.h>

struct StationContext {
    const char* LOG_TAG{};
    int RetryN = 0;
    int RetryMax = 5;
    EventGroupHandle_t EventGroup{};
};

class WiFiStation {
public:
    WiFiStation();
    ~WiFiStation();
    bool Connect(std::string_view ssid, std::string_view password, int retryMax = 5);
    bool Disconnect();
    ip4_addr_t GetIP();

private:
    bool InitializeNetIf();
    bool InitializeWiFi();
    bool RegisterEventHandlers(StationContext* context);
    bool ConfigureSettings(std::string_view ssid, std::string_view password);

    static constexpr const char* LOG_TAG = "[WiFi Station] -";
    bool m_Active = false;
    esp_event_handler_instance_t m_InstanceAnyId = nullptr;
    esp_event_handler_instance_t m_InstanceGotIp = nullptr;
    StationContext m_Context;
    esp_netif_t* m_NetIf = nullptr;
};

#endif //LPRD_MCU_WIFISTATION_H