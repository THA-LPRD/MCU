#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Application.h"
#include "AppStandalone.h"
#include "AppNetwork.h"
#include "AppServer.h"
#include <magic_enum.hpp>
#include "esp_log.h"
#include "SD_MMC.h"
#include "SD.h"
#include <driver/rtc_io.h>
#include "spdlog/sinks/rotating_file_sink.h"
#include <filesystem>
#include "Drivers/GPIO.h"

Application::Application() :
        m_DeviceID("LPRD-" + WiFi::GetMAC())
{
    m_Display = std::make_unique<EPDL>(
            m_ConfigPeripherals.GetNested<int>("Display.Pins.Busy", 8),
            m_ConfigPeripherals.GetNested<int>("Display.Pins.Reset", 39),
            m_ConfigPeripherals.GetNested<int>("Display.Pins.DC", 38),
            m_ConfigPeripherals.GetNested<int>("Display.Pins.CS", 37),
            m_ConfigPeripherals.GetNested<int>("Display.Pins.CLK", 36),
            m_ConfigPeripherals.GetNested<int>("Display.Pins.MOSI", 35)
    );
    spdlog::info("{} Starting application", LOG_TAG);

    int PinPowerEnable = m_ConfigPeripherals.Get("PowerEnable", 18);
    GPIO::SetMode(PinPowerEnable, GPIO::Mode::Output);
    GPIO::Write(PinPowerEnable, 1);
}

Application::~Application() {
    spdlog::debug("{} Destroying application", LOG_TAG);

    // Remove and close the SD log sink before touching the SD card or SPI bus.
    // The sink destructor closes the file, which flushes the FatFS sector cache.
    if (m_SDLogSink) {
        auto& sinks = spdlog::get("Global")->sinks();
        sinks.erase(std::remove(sinks.begin(), sinks.end(), m_SDLogSink), sinks.end());
        m_SDLogSink.reset();
    }

    m_Display->Terminate();

    SD.end();
    int PinPowerEnable = m_ConfigPeripherals.Get("PowerEnable", 18);
    GPIO::SetMode(PinPowerEnable, GPIO::Mode::Output);
    GPIO::Write(PinPowerEnable, 0);

    uint64_t wakeMask = 0;

    for (int i = 0; i < 4; i++) {
        // int pin = m_ConfigPeripherals.Get(("Button" + std::to_string(i)).c_str(), 17);
        // HARDCODED!
        int pin = 14 + i;
        if (pin == -1) continue;

        if (pin >= 22) {
            spdlog::error("{} Button {} is not a valid RTC GPIO and cannot be used as a wakeup source", LOG_TAG, i);
            continue;
        }

        spdlog::debug("{} Setting Button {} as wakeup source", LOG_TAG, i);

        GPIO::SetMode(pin, GPIO::Mode::Input);

        esp_err_t err = rtc_gpio_pulldown_dis(static_cast<gpio_num_t>(pin));
        if (err == ESP_OK) { err = rtc_gpio_pullup_dis(static_cast<gpio_num_t>(pin)); }
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
        if (err != ESP_OK) { spdlog::error("{} Failed to configure wakeup sources", LOG_TAG); }
        else {
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
       app = new AppServer();
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

bool Application::MountSDSPI() {
    int PinCD = m_ConfigPeripherals.GetNested("SD.Pins.CD", 5);
    

    GPIO::SetMode(PinCD, GPIO::Mode::Input);
    if (GPIO::Read(PinCD) == 1) {
        spdlog::error("{} SD Card not inserted", LOG_TAG);
        return false;
    }

    auto spi = new SPIClass(HSPI);
    spi->begin(
            m_ConfigPeripherals.GetNested("SD.Pins.CLK", 12),
            m_ConfigPeripherals.GetNested("SD.Pins.D0", 11),
            m_ConfigPeripherals.GetNested("SD.Pins.CMD", 13),
            m_ConfigPeripherals.GetNested("SD.Pins.D3", 10)
    );

    int status = SD.begin(m_ConfigPeripherals.GetNested("SD.Pins.D3", 10), *spi, 4000000, "/sd", 32, false);

    if (!status) {
        spdlog::error("{} Failed to mount SD Card, please make sure that the mode switch is set correctly", LOG_TAG);
        return false;
    }

    return true;
}

bool Application::InitFuelGauge() {
    bool enabled = m_ConfigPeripherals.GetNested("peripherals.FuelGauge.Enabled", true);
    if (!enabled) {
        spdlog::info("{} Fuel gauge disabled by config", LOG_TAG);
        return true;
    }

    int powerEnablePin = m_ConfigPeripherals.GetNested("peripherals.FuelGauge.Pins.PowerEnable", 7);
    if (powerEnablePin != -1) {
        GPIO::SetMode(powerEnablePin, GPIO::Mode::Output);
        GPIO::Write(powerEnablePin, 1);
        spdlog::info("{} Fuel gauge power enable GPIO{} set high", LOG_TAG, powerEnablePin);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    I2C::Bus::Config config = {
            .port = I2C_NUM_0,
            .sda = static_cast<gpio_num_t>(m_ConfigPeripherals.GetNested("peripherals.FuelGauge.Pins.SDA", 3)),
            .scl = static_cast<gpio_num_t>(m_ConfigPeripherals.GetNested("peripherals.FuelGauge.Pins.SCL", 4)),
            .frequency = static_cast<uint32_t>(m_ConfigPeripherals.GetNested("peripherals.FuelGauge.I2C.Frequency", 100000)),
            .enablePullups = m_ConfigPeripherals.GetNested("peripherals.FuelGauge.I2C.EnablePullups", true),
            .timeout = pdMS_TO_TICKS(m_ConfigPeripherals.GetNested("peripherals.FuelGauge.I2C.TimeoutMs", 1000)),
    };

    auto init = m_I2CBus.Init(config);
    if (!init) {
        spdlog::error("{} Failed to initialize fuel gauge I2C: {}", LOG_TAG, I2C::ToString(init.error()));
        return false;
    }

    m_FuelGauge = std::make_unique<MAX17048>(I2C::Device(m_I2CBus, MAX17048::DefaultAddress));

    auto snapshot = m_FuelGauge->ReadSnapshot();
    if (!snapshot) {
        spdlog::error("{} Failed to read MAX17048: {}", LOG_TAG, I2C::ToString(snapshot.error()));
        return false;
    }

    spdlog::info("{} MAX17048 initialized: {:.3f} V, {:.1f} %, {:+.2f} %/hr, ready={}",
                 LOG_TAG,
                 snapshot->cellVoltage,
                 snapshot->cellPercent,
                 snapshot->chargeRate,
                 snapshot->ready ? "yes" : "no");

    return true;
}

bool Application::Init() {
    if (!MountLittleFS()) return false;
    // if (!MountSDMMC()) return false; // SD with SD Protocol
    if (!MountSDSPI()) return false; // SD with SPI Protocol
    // std::filesystem::create_directory("/sd/logs");
    mkdir("/sd/logs", 0777);
    m_SDLogSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "/sd/logs/logs", 1024 * 1024 * 1, 5, false);
        m_SDLogSink->set_level(spdlog::level::debug);
        spdlog::get("Global")->sinks().push_back(m_SDLogSink);
        spdlog::info("{} Storage initialized", LOG_TAG);
    if (!InitFuelGauge()) return false;

    std::string displaystr = m_ConfigPeripherals.GetNested<std::string>("Display.Driver");
    EPDL::Display display = magic_enum::enum_cast<EPDL::Display>(displaystr).value_or(EPDL::Display::GD_7IN5);
    m_Display->Start(display);

    if (!InitImpl()) return false;
    return true;
}
