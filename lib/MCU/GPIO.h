#ifndef MCU_GPIO_H
#define MCU_GPIO_H

#include <cstdint>

namespace MCU { namespace GPIO
{
    enum Mode : uint8_t {
        Input = 0,
        Output = 1
    };
    void SetMode(uint8_t pin, uint8_t mode);
    void Write(uint8_t pin, uint8_t value);
    uint8_t Read(uint8_t pin);
}} // namespace MCU::GPIO

#endif //MCU_GPIO_H
