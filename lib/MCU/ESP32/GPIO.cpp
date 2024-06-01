#ifdef MCU_ESP32

#include "GPIO.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "Log.h"

namespace MCU { namespace GPIO
{
    namespace
    {
        gpio_mode_t ModeToEspMode(uint8_t mode) {
            switch (mode) {
                case Mode::Input:
                    return GPIO_MODE_INPUT;
                case Mode::InputPullup:
                    return GPIO_MODE_INPUT; // Mode remains input, pull-up configuration is separate
                case Mode::InputPulldown:
                    return GPIO_MODE_INPUT; // Mode remains input, pull-down configuration is separate
                case Mode::Output:
                    return GPIO_MODE_OUTPUT;
                default:
                    return GPIO_MODE_DISABLE;
            }
        }
    }

// SetMode function
    void SetMode(uint8_t pin, uint8_t mode) {
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask = (1ULL << pin);
        io_conf.mode = ModeToEspMode(mode);

        // Set pull-up or pull-down based on the mode
        if (mode == Mode::InputPullup) {
            io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        }
        else if (mode == Mode::InputPulldown) {
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
        }
        else {
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        }

        io_conf.intr_type = GPIO_INTR_DISABLE;
        esp_err_t err = gpio_config(&io_conf);
        if (err != ESP_OK) {
            Log::Error("[GPIO] Failed to set pin mode: %d", pin);
        }
    }

    void Write(uint8_t pin, uint8_t value) {
        esp_err_t err = gpio_set_level((gpio_num_t) pin, value);
        if (err != ESP_OK) {
            Log::Error("[GPIO] Failed to write to pin: %d", pin);
        }
    }

    uint8_t Read(uint8_t pin) {
        int value = gpio_get_level((gpio_num_t) pin);
        return value;
    }
}} // namespace MCU::GPIO

#endif // MCU_ESP32
