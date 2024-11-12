#include "AppStandalone.h"

AppStandalone::~AppStandalone() {
    spdlog::info("{} Destroyed Standalone Application", LOG_TAG);
}

bool AppStandalone::InitImpl() {
    spdlog::info("{} Initializing Standalone Application", LOG_TAG);
    if (!m_WiFi.Connect(WiFi::Mode::SoftAP,
                        m_ConfigApplication.GetNested("AppStandalone.WiFi.SSID", m_DeviceID),
                        m_ConfigApplication.GetNested("AppStandalone.WiFi.Password", "password"))) {
        return false;
    }
    m_IP = m_WiFi.GetIP(WiFi::Mode::SoftAP);
    if (!InitServer()) return false;

    spdlog::info("{} Standalone Application initialized", LOG_TAG);
    return true;
}

uint64_t AppStandalone::Run() {
    spdlog::info("Running Standalone Application", LOG_TAG);
    while (m_Running) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    return m_SleepTime;

}
