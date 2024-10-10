#include "SPI.h"
#include "ESP32S3SPI.h"
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
        uint8_t s_SPIBusCount = 0;
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
        if (sck == -1) {
            Log::Error("[MCU] Invalid SPI configuration. SCK cannot be -1");
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

    void SWSPI::Write(uint8_t* data, size_t length) {
        if (!m_Initialized) {
            Log::Error("[MCU] SPI not initialized");
            return;
        }
        if (m_MOSI == -1) {
            Log::Error("[MCU] Write operation attempted without MOSI line configured");
        }
        for (size_t i = 0; i < length; i++) {
            Write(data[i]);
        }
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

    std::vector<u_int8_t> SWSPI::Read(size_t length) {
        if (!m_Initialized) {
            Log::Error("[MCU] SPI not initialized");
            return {};
        }
        if (m_MISO != -1) {
            Log::Error("[MCU] Read operation attempted without MISO line configured");
            return {};
        }
        std::vector<u_int8_t> data;
        data.reserve(length);
        for (size_t i = 0; i < length; i++) {
            data.push_back(Read());
        }
        return data;
    }
    SPI* SPI::CreateHardwareSPI(SPIDevice spiDevice, int8_t mosi, int8_t miso, int8_t sck, int8_t cs) {
        return new ESP32S3SPI(spiDevice, mosi, miso, sck, cs);
    }

    ESP32S3SPI::ESP32S3SPI(SPIDevice spiDevice, int8_t mosi, int8_t miso, int8_t sck, int8_t cs) :
            ESP32S3SPI(spiDevice, mosi, miso, sck, cs,
                       spi_bus_config_t{
                               .mosi_io_num = mosi,
                               .miso_io_num = miso,
                               .sclk_io_num = sck,
                               .quadwp_io_num = -1,
                               .quadhd_io_num = -1,
                               .max_transfer_sz = 4094
                       },
                       spi_device_interface_config_t{
                               .mode = 0,                          // SPI mode 0
                               .clock_speed_hz = SPI_MASTER_FREQ_10M,  // Clock out at 4 MHz
                               .spics_io_num = cs,                 // CS pin
                               .queue_size = 7                     // We want to be able to queue 7 transactions at a time
                       }
            ) {}
    ESP32S3SPI::ESP32S3SPI(SPIDevice spiDevice,
                           int8_t mosi,
                           int8_t miso,
                           int8_t sck,
                           int8_t cs,
                           spi_bus_config_t buscfg,
                           spi_device_interface_config_t devcfg) : SPI(spiDevice, mosi, miso, sck, cs) {
        esp_err_t ret;
        switch (spiDevice) {
            case SPI0:
                ret = spi_bus_initialize(SPI2_HOST, &buscfg, 3);
                if (ret != ESP_OK) {
                    Log::Error("[MCU] Failed to Create SPI Controller");
                    return;
                }

                ret = spi_bus_add_device(SPI2_HOST, &devcfg, &m_SPI);
                if (ret != ESP_OK) {
                    spi_bus_free(SPI2_HOST);
                    Log::Error("[MCU] Failed to Create SPI Controller");
                    return;
                }
                break;
            case SPI1:
                ret = spi_bus_initialize(SPI3_HOST, &buscfg, 3);
                if (ret != ESP_OK) {
                    Log::Error("[MCU] Failed to Create SPI Controller");
                    return;
                }

                ret = spi_bus_add_device(SPI3_HOST, &devcfg, &m_SPI);
                if (ret != ESP_OK) {
                    spi_bus_free(SPI3_HOST);
                    Log::Error("[MCU] Failed to Create SPI Controller");
                    return;
                }
                break;
            default:
                Log::Error("[MCU] ESP32 only supports SPI0 and SPI1");
                return;
        }

        m_Initialized = true;
    }

    ESP32S3SPI::~ESP32S3SPI() {
        spi_bus_remove_device(m_SPI);
        switch (m_SPIDevice) {
            case SPI0:
                spi_bus_free(SPI2_HOST);
                break;
            case SPI1:
                spi_bus_free(SPI3_HOST);
                break;
        }
    }

    void ESP32S3SPI::Write(uint8_t data) {
        if (!m_Initialized) {
            Log::Error("[MCU] SPI not initialized");
            return;
        }
        if (m_MOSI == -1) {
            Log::Error("[MCU] Write operation attempted without MOSI line configured");
            return;
        }
        Log::Trace("[MCU] SPI Write. Data: 0x%02X", data);
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = 8;
        t.rxlength = 8;
        t.tx_buffer = &data;
        esp_err_t ret = spi_device_polling_transmit(m_SPI, &t);
        if (ret != ESP_OK) {
            Log::Error("[MCU] Failed to Write to SPI");
        }
        Log::Trace("[MCU] SPI Write complete");
    }

    void ESP32S3SPI::Write(uint8_t* data, size_t length) {
        if (!m_Initialized) {
            Log::Error("[MCU] SPI not initialized");
            return;
        }
        if (m_MOSI == -1) {
            Log::Error("[MCU] Write operation attempted without MOSI line configured");
            return;
        }

        // Unnecessary overhead, ideally the logger should directly support arrays
//         std::string datastr;
//        for (int i = 0; i < length; i++) {
//            char buffer[3];
//            sprintf(buffer, "%02X", data[i]);
//            datastr += buffer;
//        }
//        Log::Trace("[MCU] SPI Write. Data: 0x%s", datastr.c_str());

        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = length * 8;
        t.rxlength = length * 8;
        t.tx_buffer = data;
        esp_err_t ret = spi_device_polling_transmit(m_SPI, &t);
        if (ret != ESP_OK) {
            Log::Error("[MCU] Failed to Write to SPI");
        }
        Log::Trace("[MCU] SPI Write complete");
    }

    uint8_t ESP32S3SPI::Read() {
        if (!m_Initialized) {
            Log::Error("[MCU] SPI not initialized");
            return 0;
        }
        if (m_MISO == -1) {
            Log::Error("[MCU] Read operation attempted without MISO line configured");
            return 0;
        }
        Log::Trace("[MCU] SPI Read.");
        uint8_t rx_data = 0;
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = 8;
        t.rxlength = 8;
        t.rx_buffer = &rx_data;
        esp_err_t ret = spi_device_polling_transmit(m_SPI, &t);
        if (ret != ESP_OK) {
            Log::Error("[MCU] Failed to Read from SPI");
            return 0;
        }
        Log::Trace("[MCU] SPI Read complete. Data: 0x%02X", rx_data);
        return rx_data;
    }

    std::vector<u_int8_t> ESP32S3SPI::Read(size_t length) {
        if (!m_Initialized) {
            Log::Error("[MCU] SPI not initialized");
            return {};
        }
        if (m_MISO == -1) {
            Log::Error("[MCU] Read operation attempted without MISO line configured");
            return {};
        }
        Log::Trace("[MCU] SPI Read.");
        std::vector<u_int8_t> rx_data;
        rx_data.reserve(length);

        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = 8 * length;
        t.rxlength = 8 * length;
        t.rx_buffer = rx_data.data();
        esp_err_t ret = spi_device_polling_transmit(m_SPI, &t);
        if (ret != ESP_OK) {
            Log::Error("[MCU] Failed to Read from SPI");
            return {};
        }

        // Unnecessary overhead, ideally the logger should directly support arrays
//        std::string datastr;
//        for (int i = 0; i < length; i++) {
//            char buffer[3];
//            sprintf(buffer, "%02X", rx_data[i]);
//            datastr += buffer;
//        }
//        Log::Trace("[MCU] SPI Read complete. Data: 0x%s", datastr.c_str());
        return rx_data;
    }
}