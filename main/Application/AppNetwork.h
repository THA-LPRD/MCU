#ifndef LPRD_MCU_APPNETWORK_H
#define LPRD_MCU_APPNETWORK_H

#include "Application.h"
#include <memory>
#include <spdlog/spdlog.h>
#include "freertos/event_groups.h"
#include <ConfigManager.h>
#include "../HTTPServer/HTTPServer.h"

class AppNetwork : public Application {
public:
    AppNetwork(std::shared_ptr<spdlog::logger> logger);
    ~AppNetwork() override;
    bool Init() override;
    void Run() override;
private:
    struct WiFiContext {
        std::shared_ptr<spdlog::logger> logger;
        int RetryN = 0;
        int RetryMax = 5;
        EventGroupHandle_t EventGroup;
        esp_event_handler_instance_t InstanceAnyId;
        esp_event_handler_instance_t InstanceGotIp;
    };
    bool Connect();
    bool Disconnect();
    static void event_handler(void* ctx, esp_event_base_t event_base, int32_t event_id, void* event_data);
    bool InitServer();
private:
    void InitServerCore();
    void InitServerStandalone();
    void InitServerNetwork();
    void InitServerServer();
    void InitServerHTTP();
private:
    WiFiContext m_Context;
    std::unique_ptr<ConfigManager<Config>> m_ConfigManager;
    std::unique_ptr<HTTPServer> m_Server;
};

#endif //LPRD_MCU_APPNETWORK_H
