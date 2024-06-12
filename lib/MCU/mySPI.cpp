#include "mySPI.h"
#include "GPIO.h"
#include <Log.h>
#include <cstring>

namespace MCU
{
    namespace
    {
        bool SPI0_Initialized = false;
        bool SPI1_Initialized = false;
        bool SPI2_Initialized = false;
        bool SPI3_Initialized = false;
    }

    SPI* SPI::Create(SPIDevice spiDevice, int8_t mosi, int8_t miso, int8_t sck, int8_t cs, bool swspi) {
        if (spiDevice < SPIDevice::SPI0 || spiDevice >= SPIDevice::MAX) {
            Log::Error("[MCU] Invalid SPI device");
            return nullptr;
        }

        switch (spiDevice) {
            case SPIDevice::SPI0:
                if (SPI0_Initialized) {
                    Log::Error("[MCU] SPI0 already initialized");
                    return nullptr;
                }
                break;
            case SPIDevice::SPI1:
                if (SPI1_Initialized) {
                    Log::Error("[MCU] SPI1 already initialized");
                    return nullptr;
                }
                break;
            case SPIDevice::SPI2:
                if (SPI2_Initialized) {
                    Log::Error("[MCU] SPI2 already initialized");
                    return nullptr;
                }
                break;
            case SPIDevice::SPI3:
                if (SPI3_Initialized) {
                    Log::Error("[MCU] SPI3 already initialized");
                    return nullptr;
                }
                break;
        }

        if (mosi == -1 && miso == -1) {
            Log::Error("[MCU] Invalid SPI configuration. MOSI and MISO cannot both be -1");
            return nullptr;
        }
        if (sck == -1 || cs == -1) {
            Log::Error("[MCU] Invalid SPI configuration. SCK and CS must be configured");
            return nullptr;
        }

        if (swspi) { return new SWSPI(spiDevice, mosi, miso, sck, cs); }
        else { return CreateHardwareSPI(spiDevice, mosi, miso, sck, cs); }
    }

    SWSPI::SWSPI(SPIDevice spiDevice, int8_t mosi, int8_t miso, int8_t sck, int8_t cs) :
            SPI(spiDevice, mosi, miso, sck, cs) {
        GPIO::SetMode(sck, MCU::GPIO::Mode::Output);
        GPIO::SetMode(mosi, MCU::GPIO::Mode::Output);
        GPIO::SetMode(cs, MCU::GPIO::Mode::Output);

        GPIO::Write(cs, 1);
        GPIO::Write(sck, 0);

        switch (spiDevice) {
            case SPIDevice::SPI0:
                SPI0_Initialized = true;
                break;
            case SPIDevice::SPI1:
                SPI1_Initialized = true;
                break;
            case SPIDevice::SPI2:
                SPI2_Initialized = true;
                break;
            case SPIDevice::SPI3:
                SPI3_Initialized = true;
                break;
        }

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