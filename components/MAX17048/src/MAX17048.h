#ifndef LPRD_MCU_MAX17048_H
#define LPRD_MCU_MAX17048_H

#include <cstdint>
#include "Drivers/I2C.h"

class MAX17048 {
public:
    static constexpr uint8_t DefaultAddress = 0x36;

    enum AlertFlag : uint8_t {
        AlertSocChange = 0x20,
        AlertSocLow = 0x10,
        AlertVoltageReset = 0x08,
        AlertVoltageLow = 0x04,
        AlertVoltageHigh = 0x02,
        AlertResetIndicator = 0x01,
    };

    struct AlertVoltages {
        float minVoltage = 0.0f;
        float maxVoltage = 0.0f;
    };

    struct Snapshot {
        uint16_t rawVersion = 0;
        uint16_t rawCellVoltage = 0;
        uint16_t rawCellPercent = 0;
        uint16_t rawChargeRate = 0;
        float cellVoltage = 0.0f;
        float cellPercent = 0.0f;
        float chargeRate = 0.0f;
        bool ready = false;
    };

    explicit MAX17048(I2C::Device device);

    [[nodiscard]] I2C::Result<bool> IsReady() const;
    [[nodiscard]] I2C::Result<uint16_t> GetVersion() const;
    [[nodiscard]] I2C::Result<uint8_t> GetChipId() const;

    [[nodiscard]] I2C::Result<float> GetCellVoltage() const;
    [[nodiscard]] I2C::Result<float> GetCellPercent() const;
    [[nodiscard]] I2C::Result<float> GetChargeRate() const;
    [[nodiscard]] I2C::Result<Snapshot> ReadSnapshot() const;

    [[nodiscard]] I2C::Result<void> Reset();
    [[nodiscard]] I2C::Result<void> QuickStart();

    [[nodiscard]] I2C::Result<void> ClearAlertFlags(uint8_t flags);
    [[nodiscard]] I2C::Result<uint8_t> GetAlertStatus() const;
    [[nodiscard]] I2C::Result<bool> IsAlertActive() const;

    [[nodiscard]] I2C::Result<void> SetResetVoltage(float voltage);
    [[nodiscard]] I2C::Result<float> GetResetVoltage() const;
    [[nodiscard]] I2C::Result<void> SetAlertVoltages(float minVoltage, float maxVoltage);
    [[nodiscard]] I2C::Result<AlertVoltages> GetAlertVoltages() const;

    [[nodiscard]] I2C::Result<void> SetActivityThreshold(float voltage);
    [[nodiscard]] I2C::Result<float> GetActivityThreshold() const;
    [[nodiscard]] I2C::Result<void> SetHibernationThreshold(float percentPerHour);
    [[nodiscard]] I2C::Result<float> GetHibernationThreshold() const;

    [[nodiscard]] I2C::Result<void> Hibernate();
    [[nodiscard]] I2C::Result<void> Wake();
    [[nodiscard]] I2C::Result<bool> IsHibernating() const;
    [[nodiscard]] I2C::Result<void> SetSleepEnabled(bool enabled);
    [[nodiscard]] I2C::Result<void> SetSleep(bool sleep);

private:
    static constexpr uint8_t VCellReg = 0x02;
    static constexpr uint8_t SocReg = 0x04;
    static constexpr uint8_t ModeReg = 0x06;
    static constexpr uint8_t VersionReg = 0x08;
    static constexpr uint8_t HibernationReg = 0x0A;
    static constexpr uint8_t ConfigReg = 0x0C;
    static constexpr uint8_t AlertVoltageReg = 0x14;
    static constexpr uint8_t ChargeRateReg = 0x16;
    static constexpr uint8_t ResetVoltageReg = 0x18;
    static constexpr uint8_t ChipIdReg = 0x19;
    static constexpr uint8_t StatusReg = 0x1A;
    static constexpr uint8_t CommandReg = 0xFE;

    [[nodiscard]] I2C::Result<uint8_t> ReadU8(uint8_t reg) const;
    [[nodiscard]] I2C::Result<uint16_t> ReadU16(uint8_t reg) const;
    [[nodiscard]] I2C::Result<void> WriteU8(uint8_t reg, uint8_t value);
    [[nodiscard]] I2C::Result<void> WriteU16(uint8_t reg, uint16_t value);
    [[nodiscard]] I2C::Result<void> UpdateU8(uint8_t reg, uint8_t mask, uint8_t value);
    [[nodiscard]] I2C::Result<void> UpdateU16(uint8_t reg, uint16_t mask, uint16_t value);

    I2C::Device m_Device;
};

#endif // LPRD_MCU_MAX17048_H
