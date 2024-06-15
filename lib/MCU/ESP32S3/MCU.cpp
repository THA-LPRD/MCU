#ifdef MCU_ESP32S3

#include "MCU.h"
#include <Log.h>
#include "esp_system.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace MCU
{
    void Sleep(uint32_t ms) {
        Log::Trace("[MCU] Sleep start. Length: %d ms", ms);
        vTaskDelay(ms / portTICK_PERIOD_MS);
        Log::Trace("[MCU] Sleep end");
    }

    void SetSleepTime(uint32_t ms) {
        Log::Trace("[MCU] Set sleep time: %d ms", ms);
        esp_sleep_enable_timer_wakeup(ms * 1000);
    }

    void LightSleep() {
        Log::Trace("[MCU] Light sleep start");
        esp_light_sleep_start();
        Log::Trace("[MCU] Light sleep end");
    }

    void DeepSleep() {
        Log::Info("[MCU] Deep sleep");
        esp_deep_sleep_start();
    }

    void Restart() {
        Log::Info("[MCU] Restart device");
        esp_restart();
    }
} // namespace MCU

#endif // MCU_ESP32S3
