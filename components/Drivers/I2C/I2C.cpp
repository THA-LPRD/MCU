#include "Drivers/I2C.h"
#include <optional>
#include <utility>

namespace {

I2C::Error FromEspError(esp_err_t err) {
    switch (err) {
        case ESP_OK:
            return I2C::Error::Fail;
        case ESP_ERR_INVALID_ARG:
            return I2C::Error::InvalidArgument;
        case ESP_ERR_INVALID_STATE:
            return I2C::Error::InvalidState;
        case ESP_ERR_NO_MEM:
            return I2C::Error::NoMemory;
        case ESP_ERR_TIMEOUT:
            return I2C::Error::Timeout;
        case ESP_ERR_NOT_FOUND:
            return I2C::Error::NotFound;
        case ESP_FAIL:
            return I2C::Error::Fail;
        default:
            return I2C::Error::Unknown;
    }
}

template<typename T>
I2C::Result<T> Unexpected(esp_err_t err) {
    return std::unexpected(FromEspError(err));
}

} // namespace

namespace I2C {

const char* ToString(Error error) {
    switch (error) {
        case Error::InvalidArgument:
            return "InvalidArgument";
        case Error::InvalidState:
            return "InvalidState";
        case Error::NoMemory:
            return "NoMemory";
        case Error::Timeout:
            return "Timeout";
        case Error::NotFound:
            return "NotFound";
        case Error::Fail:
            return "Fail";
        case Error::Unknown:
            return "Unknown";
        default:
            return "Unknown";
    }
}

Transaction& Transaction::Write(std::span<const uint8_t> bytes) {
    m_Phases.push_back(Phase{
            .direction = Direction::Write,
            .write = bytes,
            .read = {},
    });
    return *this;
}

Transaction& Transaction::Read(std::span<uint8_t> bytes) {
    m_Phases.push_back(Phase{
            .direction = Direction::Read,
            .write = {},
            .read = bytes,
    });
    return *this;
}

Bus::Bus(const Config& config) {
    (void) Init(config);
}

Bus::~Bus() {
    Deinit();
}

Bus::Bus(Bus&& other) noexcept {
    *this = std::move(other);
}

Bus& Bus::operator=(Bus&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    Deinit();
    m_Config = other.m_Config;
    m_Initialized = other.m_Initialized;
    m_OwnsDriver = other.m_OwnsDriver;

    other.m_Config = {};
    other.m_Initialized = false;
    other.m_OwnsDriver = false;
    return *this;
}

Result<void> Bus::Init(const Config& config) {
    Deinit();

    if (config.sda == GPIO_NUM_NC || config.scl == GPIO_NUM_NC) {
        return std::unexpected(Error::InvalidArgument);
    }

    i2c_config_t i2cConfig = {};
    i2cConfig.mode = I2C_MODE_MASTER;
    i2cConfig.sda_io_num = config.sda;
    i2cConfig.scl_io_num = config.scl;
    i2cConfig.sda_pullup_en = config.enablePullups ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    i2cConfig.scl_pullup_en = config.enablePullups ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    i2cConfig.master.clk_speed = config.frequency;

    esp_err_t err = i2c_param_config(config.port, &i2cConfig);
    if (err != ESP_OK) {
        return Unexpected<void>(err);
    }

    err = i2c_driver_install(config.port, I2C_MODE_MASTER, 0, 0, 0);
    if (err == ESP_ERR_INVALID_STATE) {
        m_Config = config;
        m_Initialized = true;
        m_OwnsDriver = false;
        return {};
    }

    if (err != ESP_OK) {
        return Unexpected<void>(err);
    }

    m_Config = config;
    m_Initialized = true;
    m_OwnsDriver = true;
    return {};
}

void Bus::Deinit() {
    if (!m_Initialized) {
        return;
    }

    if (m_OwnsDriver) {
        i2c_driver_delete(m_Config.port);
    }

    m_Config = {};
    m_Initialized = false;
    m_OwnsDriver = false;
}

bool Bus::IsInitialized() const {
    return m_Initialized;
}

i2c_port_t Bus::Port() const {
    return m_Config.port;
}

TickType_t Bus::Timeout() const {
    return m_Config.timeout;
}

Device::Device(Bus& bus, uint8_t address) : m_Bus(&bus), m_Address(address) {}

Result<void> Device::Execute(const Transaction& transaction) const {
    if (m_Bus == nullptr || !m_Bus->IsInitialized()) {
        return std::unexpected(Error::InvalidState);
    }

    if (transaction.m_Phases.empty()) {
        return std::unexpected(Error::InvalidArgument);
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == nullptr) {
        return std::unexpected(Error::NoMemory);
    }

    std::optional<Transaction::Direction> activeDirection;

    for (size_t phaseIndex = 0; phaseIndex < transaction.m_Phases.size(); phaseIndex++) {
        const auto& phase = transaction.m_Phases[phaseIndex];

        if (!activeDirection.has_value() || *activeDirection != phase.direction) {
            activeDirection = phase.direction;
            i2c_master_start(cmd);
            i2c_master_write_byte(
                    cmd,
                    static_cast<uint8_t>((m_Address << 1) |
                                         (phase.direction == Transaction::Direction::Read ? I2C_MASTER_READ : I2C_MASTER_WRITE)),
                    true);
        }

        if (phase.direction == Transaction::Direction::Write) {
            if (!phase.write.empty()) {
                i2c_master_write(cmd, phase.write.data(), phase.write.size(), true);
            }
            continue;
        }

        for (size_t byteIndex = 0; byteIndex < phase.read.size(); byteIndex++) {
            bool lastReadByte = byteIndex == phase.read.size() - 1;

            for (size_t nextPhase = phaseIndex + 1; nextPhase < transaction.m_Phases.size(); nextPhase++) {
                const auto& candidate = transaction.m_Phases[nextPhase];
                if (candidate.direction != Transaction::Direction::Read) {
                    break;
                }
                if (!candidate.read.empty()) {
                    lastReadByte = false;
                    break;
                }
            }

            i2c_master_read_byte(
                    cmd,
                    &phase.read[byteIndex],
                    lastReadByte ? I2C_MASTER_NACK : I2C_MASTER_ACK);
        }
    }

    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(m_Bus->Port(), cmd, m_Bus->Timeout());
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK) {
        return Unexpected<void>(err);
    }

    return {};
}

uint8_t Device::Address() const {
    return m_Address;
}

} // namespace I2C
