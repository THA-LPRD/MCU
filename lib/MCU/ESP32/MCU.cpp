#ifdef MCU_ESP32

#include "MCU.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace MCU
{
    void Sleep(uint32_t ms)
    {
        vTaskDelay(ms / portTICK_PERIOD_MS);
    }
} // namespace MCU

#endif // MCU_ESP32
