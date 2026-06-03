#include "MAX17048.h"
#include <algorithm>
#include <array>

namespace {

uint8_t ClampToByte(float value) {
    return static_cast<uint8_t>(std::clamp(static_cast<int>(value), 0, 255));
}

uint8_t ClampTo7Bit(float value) {
    return static_cast<uint8_t>(std::clamp(static_cast<int>(value), 0, 127));
}

float CellVoltageFromRaw(uint16_t raw) {
    return static_cast<float>(raw) * 78.125f / 1000000.0f;
}

float CellPercentFromRaw(uint16_t raw) {
    return static_cast<float>(raw) / 256.0f;
}

float ChargeRateFromRaw(uint16_t raw) {
    return static_cast<float>(static_cast<int16_t>(raw)) * 0.208f;
}

bool ReadyFromVersion(uint16_t version) {
    return (version & 0xFFF0) == 0x0010;
}

} // namespace

MAX17048::MAX17048(I2C::Device device) : m_Device(device) {}

I2C::Result<bool> MAX17048::IsReady() const {
    auto version = GetVersion();
    if (!version) {
        return std::unexpected(version.error());
    }

    return ReadyFromVersion(*version);
}

I2C::Result<uint16_t> MAX17048::GetVersion() const {
    return ReadU16(VersionReg);
}

I2C::Result<uint8_t> MAX17048::GetChipId() const {
    return ReadU8(ChipIdReg);
}

I2C::Result<float> MAX17048::GetCellVoltage() const {
    auto raw = ReadU16(VCellReg);
    if (!raw) {
        return std::unexpected(raw.error());
    }

    return CellVoltageFromRaw(*raw);
}

I2C::Result<float> MAX17048::GetCellPercent() const {
    auto raw = ReadU16(SocReg);
    if (!raw) {
        return std::unexpected(raw.error());
    }

    return CellPercentFromRaw(*raw);
}

I2C::Result<float> MAX17048::GetChargeRate() const {
    auto raw = ReadU16(ChargeRateReg);
    if (!raw) {
        return std::unexpected(raw.error());
    }

    return ChargeRateFromRaw(*raw);
}

I2C::Result<MAX17048::Snapshot> MAX17048::ReadSnapshot() const {
    Snapshot snapshot = {};

    auto version = ReadU16(VersionReg);
    if (!version) {
        return std::unexpected(version.error());
    }

    auto voltage = ReadU16(VCellReg);
    if (!voltage) {
        return std::unexpected(voltage.error());
    }

    auto percent = ReadU16(SocReg);
    if (!percent) {
        return std::unexpected(percent.error());
    }

    auto chargeRate = ReadU16(ChargeRateReg);
    if (!chargeRate) {
        return std::unexpected(chargeRate.error());
    }

    snapshot.rawVersion = *version;
    snapshot.rawCellVoltage = *voltage;
    snapshot.rawCellPercent = *percent;
    snapshot.rawChargeRate = *chargeRate;
    snapshot.cellVoltage = CellVoltageFromRaw(*voltage);
    snapshot.cellPercent = CellPercentFromRaw(*percent);
    snapshot.chargeRate = ChargeRateFromRaw(*chargeRate);
    snapshot.ready = ReadyFromVersion(*version);
    return snapshot;
}

I2C::Result<void> MAX17048::Reset() {
    // The reset command can NACK because the chip resets before acknowledging.
    auto reset = WriteU16(CommandReg, 0x5400);
    (void) reset;

    for (uint8_t retry = 0; retry < 3; retry++) {
        auto cleared = ClearAlertFlags(AlertResetIndicator);
        if (cleared) {
            return {};
        }
    }

    return std::unexpected(I2C::Error::Fail);
}

I2C::Result<void> MAX17048::QuickStart() {
    return UpdateU8(ModeReg, 0x40, 0x40);
}

I2C::Result<void> MAX17048::ClearAlertFlags(uint8_t flags) {
    auto status = ReadU8(StatusReg);
    if (!status) {
        return std::unexpected(status.error());
    }

    return WriteU8(StatusReg, *status & ~flags);
}

I2C::Result<uint8_t> MAX17048::GetAlertStatus() const {
    auto status = ReadU8(StatusReg);
    if (!status) {
        return std::unexpected(status.error());
    }

    return static_cast<uint8_t>(*status & 0x7F);
}

I2C::Result<bool> MAX17048::IsAlertActive() const {
    auto config = ReadU16(ConfigReg);
    if (!config) {
        return std::unexpected(config.error());
    }

    return (*config & 0x0020) != 0;
}

I2C::Result<void> MAX17048::SetResetVoltage(float voltage) {
    return UpdateU8(ResetVoltageReg, 0x7F, ClampTo7Bit(voltage / 0.04f));
}

I2C::Result<float> MAX17048::GetResetVoltage() const {
    auto raw = ReadU8(ResetVoltageReg);
    if (!raw) {
        return std::unexpected(raw.error());
    }

    return static_cast<float>(*raw & 0x7F) * 0.04f;
}

I2C::Result<void> MAX17048::SetAlertVoltages(float minVoltage, float maxVoltage) {
    auto minResult = WriteU8(AlertVoltageReg, ClampToByte(minVoltage / 0.02f));
    if (!minResult) {
        return minResult;
    }

    return WriteU8(AlertVoltageReg + 1, ClampToByte(maxVoltage / 0.02f));
}

I2C::Result<MAX17048::AlertVoltages> MAX17048::GetAlertVoltages() const {
    auto minRaw = ReadU8(AlertVoltageReg);
    if (!minRaw) {
        return std::unexpected(minRaw.error());
    }

    auto maxRaw = ReadU8(AlertVoltageReg + 1);
    if (!maxRaw) {
        return std::unexpected(maxRaw.error());
    }

    return AlertVoltages{
            .minVoltage = static_cast<float>(*minRaw) * 0.02f,
            .maxVoltage = static_cast<float>(*maxRaw) * 0.02f,
    };
}

I2C::Result<void> MAX17048::SetActivityThreshold(float voltage) {
    return WriteU8(HibernationReg + 1, ClampToByte(voltage / 0.00125f));
}

I2C::Result<float> MAX17048::GetActivityThreshold() const {
    auto raw = ReadU8(HibernationReg + 1);
    if (!raw) {
        return std::unexpected(raw.error());
    }

    return static_cast<float>(*raw) * 0.00125f;
}

I2C::Result<void> MAX17048::SetHibernationThreshold(float percentPerHour) {
    return WriteU8(HibernationReg, ClampToByte(percentPerHour / 0.208f));
}

I2C::Result<float> MAX17048::GetHibernationThreshold() const {
    auto raw = ReadU8(HibernationReg);
    if (!raw) {
        return std::unexpected(raw.error());
    }

    return static_cast<float>(*raw) * 0.208f;
}

I2C::Result<void> MAX17048::Hibernate() {
    auto activity = WriteU8(HibernationReg + 1, 0xFF);
    if (!activity) {
        return activity;
    }

    return WriteU8(HibernationReg, 0xFF);
}

I2C::Result<void> MAX17048::Wake() {
    auto activity = WriteU8(HibernationReg + 1, 0x00);
    if (!activity) {
        return activity;
    }

    return WriteU8(HibernationReg, 0x00);
}

I2C::Result<bool> MAX17048::IsHibernating() const {
    auto mode = ReadU8(ModeReg);
    if (!mode) {
        return std::unexpected(mode.error());
    }

    return (*mode & 0x10) != 0;
}

I2C::Result<void> MAX17048::SetSleepEnabled(bool enabled) {
    return UpdateU8(ModeReg, 0x20, enabled ? 0x20 : 0x00);
}

I2C::Result<void> MAX17048::SetSleep(bool sleep) {
    return UpdateU16(ConfigReg, 0x0080, sleep ? 0x0080 : 0x0000);
}

I2C::Result<uint8_t> MAX17048::ReadU8(uint8_t reg) const {
    std::array<uint8_t, 1> regData = {reg};
    std::array<uint8_t, 1> readData = {};

    auto result = m_Device.Execute(I2C::Transaction{}
            .Write(regData)
            .Read(readData));
    if (!result) {
        return std::unexpected(result.error());
    }

    return readData[0];
}

I2C::Result<uint16_t> MAX17048::ReadU16(uint8_t reg) const {
    std::array<uint8_t, 1> regData = {reg};
    std::array<uint8_t, 2> readData = {};

    auto result = m_Device.Execute(I2C::Transaction{}
            .Write(regData)
            .Read(readData));
    if (!result) {
        return std::unexpected(result.error());
    }

    return (static_cast<uint16_t>(readData[0]) << 8) | readData[1];
}

I2C::Result<void> MAX17048::WriteU8(uint8_t reg, uint8_t value) {
    std::array<uint8_t, 2> data = {reg, value};
    return m_Device.Execute(I2C::Transaction{}.Write(data));
}

I2C::Result<void> MAX17048::WriteU16(uint8_t reg, uint16_t value) {
    std::array<uint8_t, 3> data = {
            reg,
            static_cast<uint8_t>(value >> 8),
            static_cast<uint8_t>(value & 0xFF),
    };

    return m_Device.Execute(I2C::Transaction{}.Write(data));
}

I2C::Result<void> MAX17048::UpdateU8(uint8_t reg, uint8_t mask, uint8_t value) {
    auto current = ReadU8(reg);
    if (!current) {
        return std::unexpected(current.error());
    }

    return WriteU8(reg, (*current & ~mask) | (value & mask));
}

I2C::Result<void> MAX17048::UpdateU16(uint8_t reg, uint16_t mask, uint16_t value) {
    auto current = ReadU16(reg);
    if (!current) {
        return std::unexpected(current.error());
    }

    return WriteU16(reg, (*current & ~mask) | (value & mask));
}
