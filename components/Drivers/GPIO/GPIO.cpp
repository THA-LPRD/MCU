#include "Drivers/GPIO.h"
#include <spdlog/spdlog.h>
#include "driver/gpio.h"
#include "esp_err.h"

namespace GPIO
{
    static gpio_mode_t ModeToEspMode(GPIO::Mode mode) {
        switch (mode) {
            case GPIO::Mode::Input:
            case GPIO::Mode::InputPullup:
            case GPIO::Mode::InputPulldown:
                return GPIO_MODE_INPUT;
            case GPIO::Mode::Output:
                return GPIO_MODE_OUTPUT;
            default:
                return GPIO_MODE_DISABLE;
        }
    }

    void SetMode(uint8_t pin, uint8_t mode) {
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask = (1ULL << pin);
        io_conf.mode = ModeToEspMode(static_cast<GPIO::Mode>(mode));

        if (mode == static_cast<uint8_t>(GPIO::Mode::InputPullup)) {
            io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        }
        else if (mode == static_cast<uint8_t>(GPIO::Mode::InputPulldown)) {
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
            spdlog::error("[GPIO] - Failed to set Pin: %d to Mode: %d", pin, mode);
        }
        spdlog::trace("[GPIO] - Set Pin: {} to Mode: {}", pin, mode);
    }

    void SetMode(uint8_t pin, GPIO::Mode mode) {
        SetMode(pin, static_cast<uint8_t>(mode));
    }

    void Write(uint8_t pin, uint8_t value) {
        esp_err_t err = gpio_set_level((gpio_num_t) pin, value);
        if (err != ESP_OK) {
            spdlog::error("[GPIO] - Failed to write to Pin: {} with Value: {}", pin, value);
        }
//        spdlog::trace("[GPIO] Write to Pin: {} with Value: {}", pin, value);
    }

    uint8_t Read(uint8_t pin) {
        int value = gpio_get_level((gpio_num_t) pin);
//        spdlog::trace(" [GPIO] Read from Pin: {} with Value: {}", pin, value);
        return value;
    }
} // namespace GPIO
