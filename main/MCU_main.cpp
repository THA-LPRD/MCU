#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <esp_pm.h>
#include <LittleFS.h>
#include "esp_system.h"
#include "LoggingUtils.h"
#include "Application/Application.h"
#include "WiFi.h"
#include "HTTPServer/HTTPServer.h"

void MountLittleFS(std::shared_ptr<spdlog::logger> logger) {
    if (!LittleFS.begin(true, "/storage", 10, "storage")) {
        logger->critical("Failed to mount LittleFS");
        for (int i = 0; i < 5; i++) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            logger->critical("Restarting in {} seconds", 5 - i);
        }
        esp_restart();
    }
    logger->info("Mounted LittleFS");
}

void LogRAM(const std::shared_ptr<spdlog::logger> &logger = nullptr) {
    size_t SRAMTotal = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    size_t SRAMFree = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t PSRAMTotal = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t PSRAMFree = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    if (!logger) {
        ESP_LOGV("Main", "SRAM: %d/%d, (%d%%) - PSRAM: %d/%d, (%d%%)", SRAMTotal - SRAMFree, SRAMTotal,
                 (SRAMTotal - SRAMFree) * 100 / SRAMTotal, PSRAMTotal - PSRAMFree, PSRAMTotal,
                 (PSRAMTotal - PSRAMFree) * 100 / PSRAMTotal);
    }
    else {
        logger->trace("SRAM: {}/{}, ({}%) - PSRAM: {}/{}, ({}%)", SRAMTotal - SRAMFree, SRAMTotal,
                      (SRAMTotal - SRAMFree) * 100 / SRAMTotal, PSRAMTotal - PSRAMFree, PSRAMTotal,
                      (PSRAMTotal - PSRAMFree) * 100 / PSRAMTotal);
    }
}

template<typename T, typename F, typename... Args>
T LogRAMWrapper(F&& fn, const std::shared_ptr<spdlog::logger>& logger, Args&&... args) {
    LogRAM(logger);
    T result = std::forward<F>(fn)(std::forward<Args>(args)...);
    LogRAM(logger);
    return result;
}

template<typename F, typename... Args>
void LogRAMWrapper(F&& fn, const std::shared_ptr<spdlog::logger>& logger, Args&&... args) {
    LogRAM(logger);
    std::forward<F>(fn)(std::forward<Args>(args)...);
    LogRAM(logger);
}

extern "C" void app_main(void) {
    {
        esp_pm_config_t pm_config = {
                .max_freq_mhz = 160,
                .min_freq_mhz = 10, // XTAL(40MHz) / 4 = 10MHz
                .light_sleep_enable = false
        };
        ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
    }

    spdlog::set_pattern("%^[%C-%m-%d %H:%M:%S.%e] [%=11n] %=8l - %v%$");
    spdlog::set_level(spdlog::level::trace);
    std::shared_ptr<spdlog::logger> Logger_Main = spdlog::stdout_color_mt("Main", spdlog::color_mode::always);
    std::shared_ptr<spdlog::logger> Logger_Application = spdlog::stdout_color_mt("Application",
                                                                                spdlog::color_mode::always);
    Logger_Main->debug("Initialized Main logger");

    LogRAM(Logger_Main);

    LogRAMWrapper(std::function<void()>(initArduino), Logger_Main);

    LogRAMWrapper(std::function<void(std::shared_ptr<spdlog::logger>)>(MountLittleFS), Logger_Main, Logger_Main);

    Application *app = Application::Create(Application::Mode::Network, Logger_Application);
    if (!app->Init()) {
        Logger_Main->critical("Failed to initialize application");
        esp_restart();
    }
    LogRAM(Logger_Main);

    while (true) {
        vTaskDelay(portMAX_DELAY);
//        vTaskDelay(5000 / portTICK_PERIOD_MS);
//        LogRAM(Logger_Main);
    }
}
