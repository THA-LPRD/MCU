#ifdef MCU_ESP32

#include "MCU.h"
#include <Log.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace MCU
{
    void Sleep(uint32_t ms) {
        Log::LoTrace("[MCU] Sleep start. Length: %d ms", ms);
        vTaskDelay(ms / portTICK_PERIOD_MS);
        Log::LoTrace("[MCU] Sleep end");
    }

    void Restart() {
        Log::Info("[MCU] Restart device");
        esp_restart();
    }
} // namespace MCU

#endif // MCU_ESP32
