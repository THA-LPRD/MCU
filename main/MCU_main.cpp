#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <esp_pm.h>
#include "esp_system.h"
#include "Application/Application.h"
#include "Application/AppServer.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "driver/timer.h"
#include "nvs.h"
#include "Drivers/GPIO.h"
#include <LittleFS.h>

const uint8_t MAX_BOOT_ATTEMPTS = 5;

RTC_NOINIT_ATTR static struct {
    uint8_t bootCount;
    bool wasSuccess;
} rtcData = {0, false};

void ResetBootCount() {
    rtcData.bootCount = 0;
    rtcData.wasSuccess = true;
}

void CheckBootCount() {
    esp_reset_reason_t reason = esp_reset_reason();
    spdlog::info("Boot reason: {}", magic_enum::enum_name(reason));

    switch (reason) {
        case ESP_RST_POWERON:
            ResetBootCount();
            spdlog::info("Power-on or Reset button pressed - resetting boot count");
            break;
        case ESP_RST_PANIC:
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT:
        case ESP_RST_BROWNOUT:
        case ESP_RST_CPU_LOCKUP:
            if (!rtcData.wasSuccess) {
                rtcData.bootCount++;
                spdlog::warn("Crash recovery boot attempt #{}", rtcData.bootCount);

                if (rtcData.bootCount >= MAX_BOOT_ATTEMPTS) {
                    spdlog::critical("Maximum boot attempts reached. Entering infinite sleep to prevent battery drain.");
                    spdlog::critical("Press the RESET button to try again.");
                    esp_deep_sleep_start();
                }
            }
            break;
        default:
            break;
    }

    rtcData.wasSuccess = false;
}

void InitLogging() {
    spdlog::set_pattern("%^[%C-%m-%d %H:%M:%S.%e] %=8l: [%=11n] - %v%$");
    std::shared_ptr<spdlog::logger> globalLogger = spdlog::stdout_color_mt("Global", spdlog::color_mode::always);
    globalLogger->set_pattern("%^[%C-%m-%d %H:%M:%S.%e] %=8l: %v%$");
    spdlog::set_default_logger(globalLogger);
    spdlog::set_level(spdlog::level::trace);
}


void InitESP() {
    esp_pm_config_t pm_config = {
            .max_freq_mhz = 160,
            .min_freq_mhz = 10, // XTAL(40MHz) / 4 = 10MHz
            .light_sleep_enable = false
    };
    esp_err_t err = esp_pm_configure(&pm_config);
    if (err != ESP_OK) {
        spdlog::error("Failed to configure power management: {}", esp_err_to_name(err));
    }
}

bool CheckConfigReset() {
    static constexpr const char* LOG_TAG = "[Config Reset] -";
    static constexpr uint32_t RESET_HOLD_TIME_MS = 5000;
    static constexpr const char* NVSNAMESPACE = "ConfigManager";

    // Get Button0 pin from config
    ConfigManager configPeripherals("peripherals");
    int buttonPin = configPeripherals.Get("Button0", 17);

    if (buttonPin == -1) {
        spdlog::error("{} Button0 pin not configured", LOG_TAG);
        return false;
    }

    GPIO::SetMode(buttonPin, GPIO::Mode::Input);

    if (GPIO::Read(buttonPin) == 0) {
        spdlog::info("{} Button pressed, waiting for {} ms to confirm reset", LOG_TAG, RESET_HOLD_TIME_MS);

        uint64_t startTime = esp_timer_get_time() / 1000;
        uint64_t elapsedTime = 0;
        int lastLoggedSecond = -1;

        while (GPIO::Read(buttonPin) == 0) {
            elapsedTime = (esp_timer_get_time() / 1000) - startTime;
            int currentSecond = static_cast<int>(floor(elapsedTime / 1000.0));

            if (currentSecond != lastLoggedSecond) {
                float remainingSeconds = ceil((RESET_HOLD_TIME_MS - elapsedTime) / 1000.0);
                spdlog::info("{} Hold for {:.0f} more seconds to reset config...",
                             LOG_TAG,
                             remainingSeconds);
                lastLoggedSecond = currentSecond;
            }

            if (elapsedTime >= RESET_HOLD_TIME_MS) {
                spdlog::warn("{} Erasing all configuration data...", LOG_TAG);

                nvs_handle_t nvs_handle;
                esp_err_t err = nvs_open(NVSNAMESPACE, NVS_READWRITE, &nvs_handle);
                if (err != ESP_OK) {
                    spdlog::error("{} Failed to open NVS handle: {}", LOG_TAG, esp_err_to_name(err));
                    return false;
                }

                err = nvs_erase_all(nvs_handle);
                if (err != ESP_OK) {
                    spdlog::error("{} Failed to erase namespace {}: {}", LOG_TAG, NVSNAMESPACE, esp_err_to_name(err));
                    nvs_close(nvs_handle);
                    return false;
                }

                err = nvs_commit(nvs_handle);
                if (err != ESP_OK) {
                    spdlog::error("{} Failed to commit NVS changes: {}", LOG_TAG, esp_err_to_name(err));
                    nvs_close(nvs_handle);
                    return false;
                }
                nvs_close(nvs_handle);

                spdlog::info("{} Configuration reset successful", LOG_TAG);
                return true;
            }

            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    return false;
}


extern "C" void app_main(void) {
    uint64_t time = 0;
    Application* app = nullptr;

    // Init sequence
    InitLogging();
    InitESP();
    initArduino();
    CheckBootCount();
    CheckConfigReset();

    ConfigManager configApplication("application");
    std::string loglevel = configApplication.Get("LogLevel", "info");
    spdlog::default_logger()->sinks()[0]->set_level(spdlog::level::from_str(loglevel.data()));
    std::string mode = configApplication.Get("OperatingMode", "Standalone");

    // Application creation and init
    app = Application::Create(mode);
    if (!app || !app->Init()) {
        spdlog::critical("Failed to start application");
        spdlog::critical("Rebooting in 5 seconds");

        // Increment boot count for initialization failure
        rtcData.bootCount++;
        spdlog::warn("Init failure boot attempt #{}", rtcData.bootCount);

        if (rtcData.bootCount >= MAX_BOOT_ATTEMPTS) {
            spdlog::critical("Maximum boot attempts reached. Entering infinite sleep to prevent battery drain.");
            spdlog::critical("Press the RESET button to try again.");
            esp_deep_sleep_start();
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    else {
        time = app->Run();
        spdlog::debug("This Asset is valid for {} seconds", time / 1000 / 1000);
        ResetBootCount();
    }

    delete app;

    if (time == 0) {
        spdlog::info("Rebooting");
        esp_restart();
    }
    else {
        spdlog::info("Entering deep sleep for {} seconds", time / 1000 / 1000);
        esp_sleep_enable_timer_wakeup(time);
        spdlog::info("Entering deep sleep");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_deep_sleep_start();
    }
}
