#ifndef MCU_GPIO_H
#define MCU_GPIO_H

#include <cstdint>

namespace MCU { namespace GPIO
{
    enum Mode : uint8_t {
        Input = 0,
        InputPullup = 1,
        InputPulldown = 2,
        Output = 3
    };

    #ifdef MCU_ESP32
    // ESP32 Devmodule
    enum Pin : uint8_t {
        BTN1 = 2,
        VCC = 43,
    };
    #endif

    #ifdef MCU_ESP32S3
    enum Pin : uint8_t {
        BTN1 = 44,
        VCC = 43,
        };
    #endif

    void SetMode(uint8_t pin, uint8_t mode);
    void Write(uint8_t pin, uint8_t value);
    uint8_t Read(uint8_t pin);
}} // namespace MCU::GPIO

#endif //MCU_GPIO_H
