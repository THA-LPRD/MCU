#include "AppNetwork.h"
#include "PNGDecoder.h"
#include "EPDL.h"

AppNetwork::~AppNetwork() {
    spdlog::info("{} Destroyed network application", LOG_TAG);
}

bool AppNetwork::InitImpl() {
    spdlog::info("{} Initializing network application", LOG_TAG);
    m_WiFi.ConfigureSNTP();
    if (!m_WiFi.Connect(WiFi::Mode::Station,
                        m_ConfigApplication.GetNested<std::string_view>("AppNetwork.WiFi.SSID"),
                        m_ConfigApplication.GetNested<std::string_view>("AppNetwork.WiFi.Password"))) {
        return false;
    }
    m_IP = m_WiFi.GetIP(WiFi::Mode::Station);
    if (!InitServer()) return false;

    spdlog::info("{} Network application initialized", LOG_TAG);
    return true;
}

uint64_t AppNetwork::Run() {
    spdlog::info("{} Running network application", LOG_TAG);
    while (m_Running) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    return m_SleepTime;
}
