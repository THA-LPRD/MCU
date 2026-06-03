#ifndef LPRD_MCU_I2C_H
#define LPRD_MCU_I2C_H

#include <cstdint>
#include <expected>
#include <span>
#include <vector>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"

namespace I2C {

enum class Error {
    InvalidArgument,
    InvalidState,
    NoMemory,
    Timeout,
    NotFound,
    Fail,
    Unknown,
};

template<typename T>
using Result = std::expected<T, Error>;

[[nodiscard]] const char* ToString(Error error);

class Transaction {
public:
    Transaction& Write(std::span<const uint8_t> bytes);
    Transaction& Read(std::span<uint8_t> bytes);

private:
    friend class Device;

    enum class Direction {
        Write,
        Read,
    };

    struct Phase {
        Direction direction;
        std::span<const uint8_t> write;
        std::span<uint8_t> read;
    };

    std::vector<Phase> m_Phases;
};

class Bus {
public:
    struct Config {
        i2c_port_t port = I2C_NUM_0;
        gpio_num_t sda = GPIO_NUM_NC;
        gpio_num_t scl = GPIO_NUM_NC;
        uint32_t frequency = 100000;
        bool enablePullups = true;
        TickType_t timeout = pdMS_TO_TICKS(1000);
    };

    Bus() = default;
    explicit Bus(const Config& config);
    ~Bus();

    Bus(const Bus&) = delete;
    Bus& operator=(const Bus&) = delete;
    Bus(Bus&& other) noexcept;
    Bus& operator=(Bus&& other) noexcept;

    [[nodiscard]] Result<void> Init(const Config& config);
    void Deinit();

    [[nodiscard]] bool IsInitialized() const;

private:
    friend class Device;

    [[nodiscard]] i2c_port_t Port() const;
    [[nodiscard]] TickType_t Timeout() const;

    Config m_Config = {};
    bool m_Initialized = false;
    bool m_OwnsDriver = false;
};

class Device {
public:
    Device(Bus& bus, uint8_t address);

    [[nodiscard]] Result<void> Execute(const Transaction& transaction) const;
    [[nodiscard]] uint8_t Address() const;

private:
    Bus* m_Bus = nullptr;
    uint8_t m_Address = 0;
};

} // namespace I2C

#endif // LPRD_MCU_I2C_H
