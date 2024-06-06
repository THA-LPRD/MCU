#include "SPI.h"
#include "GPIO.h"
#include <Log.h>
#include <cstring>

namespace MCU
{
    SPI* SPI::Create(int8_t mosi, int8_t miso, int8_t sck, int8_t cs, bool swspi) {
        if (swspi) { return new SWSPI(mosi, miso, sck, cs); }
        else { return CreateHardwareSPI(mosi, miso, sck, cs); }
    }

    SWSPI::SWSPI(int8_t mosi, int8_t miso, int8_t sck, int8_t cs) : SPI(mosi, miso, sck, cs) {
        GPIO::SetMode(sck, MCU::GPIO::Mode::Output);
        GPIO::SetMode(mosi, MCU::GPIO::Mode::Output);
        GPIO::SetMode(cs, MCU::GPIO::Mode::Output);

        GPIO::Write(cs, 1);
        GPIO::Write(sck, 0);

        m_Initialized = true;
    }

    void SWSPI::Write(uint8_t data) {
        if (!m_Initialized) {
            Log::Error("[MCU] SPI not initialized");
            return;
        }
        if (m_MOSI == -1) {
            Log::Error("[MCU] Write operation attempted without MOSI line configured");
        }
        Log::Trace("[MCU] SPI Write. Data: 0x%02X", data);
        GPIO::Write(m_CS, 0);
        for (int i = 0; i < 8; i++) {
            GPIO::Write(m_MOSI, data & 0x80);
            GPIO::Write(m_SCK, 1);
            GPIO::Write(m_SCK, 0);
            data <<= 1;
        }
        GPIO::Write(m_CS, 1);
        Log::Trace("[MCU] SPI Write complete");
    }

    uint8_t SWSPI::Read() {
        if (!m_Initialized) {
            Log::Error("[MCU] SPI not initialized");
            return 0;
        }
        if (m_MISO != -1) {
            Log::Error("[MCU] Read operation attempted without MISO line configured");
            return 0;
        }
        Log::Trace("[MCU] SPI Read.");
        uint8_t data = 0;
        GPIO::Write(m_CS, 0);
        for (int i = 0; i < 8; i++) {
            GPIO::Write(m_SCK, 1);
            data <<= 1;
            data |= GPIO::Read(m_MISO);
            GPIO::Write(m_SCK, 0);
        }
        GPIO::Write(m_CS, 1);
        Log::Trace("[MCU] SPI Read complete. Data: 0x%02X", data);
        return data;
    }
}