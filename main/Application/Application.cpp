#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Application.h"
#include "AppStandalone.h"
#include "AppNetwork.h"
//#include "AppServer.h"
#include <magic_enum.hpp>
#include "esp_log.h"
#include "SD_MMC.h"
#include <driver/rtc_io.h>

Application::Application() :
        m_DeviceID("LPRD-" + WiFi::GetMAC())
{
    m_Display = std::make_unique<EPDL>(
            m_ConfigPeripherals.GetNested<int>("Display.Pins.Busy"),
            m_ConfigPeripherals.GetNested<int>("Display.Pins.Reset"),
            m_ConfigPeripherals.GetNested<int>("Display.Pins.DC"),
            m_ConfigPeripherals.GetNested<int>("Display.Pins.CS"),
            m_ConfigPeripherals.GetNested<int>("Display.Pins.CLK"),
            m_ConfigPeripherals.GetNested<int>("Display.Pins.MOSI")
    );
    spdlog::info("{} Starting application", LOG_TAG);

    int PinPowerEnable = m_ConfigPeripherals.Get("PowerEnable", -1);
    GPIO::SetMode(PinPowerEnable, GPIO::Mode::Output);
    GPIO::Write(PinPowerEnable, 1);
}

Application::~Application() {
    spdlog::debug("{} Destroying application", LOG_TAG);

    m_Display->Terminate();

    int PinPowerEnable = m_ConfigPeripherals.Get("PowerEnable", -1);
    GPIO::SetMode(PinPowerEnable, GPIO::Mode::Output);
    GPIO::Write(PinPowerEnable, 0);

    uint64_t wakeMask = 0;

for (int i = 0; i < 4; i++) {
    int pin = m_ConfigPeripherals.Get(("Button" + std::to_string(i)).c_str(), -1);
    if (pin == -1) continue;

    if (pin >= 22) {
        spdlog::error("{} Button {} is not a valid RTC GPIO and cannot be used as a wakeup source", LOG_TAG, i);
        continue;
    }

    spdlog::debug("{} Setting Button {} as wakeup source", LOG_TAG, i);

    GPIO::SetMode(pin, GPIO::Mode::Input);

    esp_err_t err = rtc_gpio_pulldown_dis(static_cast<gpio_num_t>(pin));
    if (err == ESP_OK) {
        err = rtc_gpio_pullup_dis(static_cast<gpio_num_t>(pin));
    }
    
    if (err != ESP_OK) {
        spdlog::error("{} Failed to configure Button {} GPIO settings", LOG_TAG, i);
        continue;
    }

    // Füge den Pin zur Wakeup-Maske hinzu
    wakeMask |= (1ULL << pin);
    spdlog::info("{} Button {} prepared for wakeup configuration", LOG_TAG, i);
}

// Konfiguriere alle gesammelten Pins als Wakeup-Quellen
if (wakeMask != 0) {
    // 14 = 16384
    // 15 = 32768 
    // 16 = 65536 
    // 17 = 131072
    esp_err_t err = esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_LOW);
    if (err != ESP_OK) {
        spdlog::error("{} Failed to configure wakeup sources", LOG_TAG);
    } else {
        spdlog::info("{} Successfully configured all wakeup sources", LOG_TAG);
        spdlog::info("{} WakeupMask {}", LOG_TAG, wakeMask);
    }
}
    spdlog::info("{} Application destroyed", LOG_TAG);
}

Application* Application::Create(std::string_view mode) {
    Application* app = nullptr;
    if (mode == "Standalone") {
        app = new AppStandalone();
    }
    else if (mode == "Network") {
        app = new AppNetwork();
    }
    else if (mode == "Server") {
//        app = new AppServer(std::move(configApplication));
    }
    else {
        spdlog::error("Unknown application mode: {}", mode);
        app = new AppStandalone();
    }

    return app;
}

bool Application::MountLittleFS() {
    if (!LittleFS.begin(true, "/storage", 10, "storage")) {
        ESP_LOGE("Application", "Failed to mount LittleFS");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        return false;
    }
    ESP_LOGI("Application", "Mounted LittleFS");
    return true;
}

bool Application::MountSDMMC() {
    int PinCD = m_ConfigPeripherals.GetNested("SD.Pins.CD", -1);

    GPIO::SetMode(PinCD, GPIO::Mode::Input);
    if (GPIO::Read(PinCD) == 0) {
        spdlog::error("{} SD Card not inserted", LOG_TAG);
        return false;
    }

    int SDMode = m_ConfigPeripherals.GetNested("SD.Mode", 0);

    bool status = false;
    switch (SDMode) {
        case 1:
            SD_MMC.setPins(
                    m_ConfigPeripherals.GetNested("SD.Pins.SCLK", -1),
                    m_ConfigPeripherals.GetNested("SD.Pins.CMD", -1),
                    m_ConfigPeripherals.GetNested("SD.Pins.D0", -1),
                    -1,
                    -1,
                    m_ConfigPeripherals.GetNested("SD.Pins.D3", -1)
            );
            status = SD_MMC.begin("/sdcard", true, true, BOARD_MAX_SDMMC_FREQ, 10);
            break;
        case 4:
            SD_MMC.setPins(
                    m_ConfigPeripherals.GetNested("SD.Pins.CLK", -1),
                    m_ConfigPeripherals.GetNested("SD.Pins.CMD", -1),
                    m_ConfigPeripherals.GetNested("SD.Pins.D0", -1),
                    m_ConfigPeripherals.GetNested("SD.Pins.D1", -1),
                    m_ConfigPeripherals.GetNested("SD.Pins.D2", -1),
                    m_ConfigPeripherals.GetNested("SD.Pins.D3", -1)
            );
            status = SD_MMC.begin("/sdcard", false, true, BOARD_MAX_SDMMC_FREQ, 10);
            break;
        default:
            spdlog::error("{} Invalid SD Lane mode: {}", LOG_TAG, SDMode);
            return false;
    }

    if (!status) {
        spdlog::error("{} Failed to mount SD Card, please make sure that the mode switch is set correctly", LOG_TAG);
        return false;
    }

    return true;
}

bool Application::Init() {
    if (!MountLittleFS()) return false;
//    if (!MountSDMMC()) return false;

    std::string displaystr = m_ConfigPeripherals.GetNested<std::string>("Display.Driver");
    EPDL::Display display = magic_enum::enum_cast<EPDL::Display>(displaystr).value_or(EPDL::Display::WS_7IN3G);
    m_Display->Start(display);

    if (!InitImpl()) return false;
    return true;
}
