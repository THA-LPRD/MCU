#ifndef LPRD_MCU_APPLICATION_H
#define LPRD_MCU_APPLICATION_H

#include <string_view>
#include <spdlog/spdlog.h>
#include <memory>
#include "../ConfigManager.h"
#include "../HTTPServer/HTTPServer.h"
#include "EPDL.h"
#include <Drivers/WiFi.h>

class Application {
public:
    Application();
    virtual ~Application();
    static Application* Create(std::string_view mode);
    bool Init();
    virtual uint64_t Run() = 0;
protected:
    virtual bool InitImpl() = 0;
private:
    bool MountLittleFS();
    bool MountSDMMC();
protected:
    static constexpr const char* LOG_TAG = "[Application] -";
    std::string m_DeviceID;
    ConfigManager m_ConfigApplication = ConfigManager("application");
    ConfigManager m_ConfigPeripherals = ConfigManager("peripherals");
    std::unique_ptr<EPDL> m_Display;
    WiFi m_WiFi;
    bool m_Running = true;
    ip4_addr_t m_IP = {};
};

#endif //LPRD_MCU_APPLICATION_H
