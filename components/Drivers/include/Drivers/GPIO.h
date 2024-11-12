#ifndef LPRD_MCU_GPIO_H
#define LPRD_MCU_GPIO_H

#include <cstdint>

namespace GPIO
{
    enum class Mode : uint8_t {
        Input = 0,
        InputPullup = 1,
        InputPulldown = 2,
        Output = 3
    };
    void SetMode(uint8_t pin, uint8_t mode);
    void SetMode(uint8_t pin, GPIO::Mode mode);
    void Write(uint8_t pin, uint8_t value);
    uint8_t Read(uint8_t pin);
} // namespace GPIO

#endif //LPRD_MCU_GPIO_H
