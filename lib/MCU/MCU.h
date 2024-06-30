#ifndef MCU_MCU_H
#define MCU_MCU_H

#include <cstdint>

namespace MCU
{
    void Sleep(uint32_t ms);
    void SetSleepTime(uint32_t ms);
    void LightSleep();
    void DeepSleep();
    void Restart();
} // namespace MCU

#endif //MCU_MCU_H
