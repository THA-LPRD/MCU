#ifdef MCU_ESP32

#include "ESP32SPI.h"
#include <cstring>
#include <Log.h>
#include "GPIO.h"

namespace MCU
{
    namespace
    {
        uint8_t s_SPIBusCount = 0;
    }
    SPI* SPI::CreateHardwareSPI(SPIDevice spiDevice, int8_t mosi, int8_t miso, int8_t sck, int8_t cs) {
        return new ESP32SPI(spiDevice, mosi, miso, sck, cs);
    }

    ESP32SPI::ESP32SPI(SPIDevice spiDevice, int8_t mosi, int8_t miso, int8_t sck, int8_t cs) :
        ESP32SPI(spiDevice, mosi, miso, sck, cs,
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
                    .clock_speed_hz = 4 * 1000 * 1000,  // Clock out at 4 MHz
                    .spics_io_num = cs,                 // CS pin
                    .queue_size = 7                     // We want to be able to queue 7 transactions at a time
            }
        ) {
    }
    ESP32SPI::ESP32SPI(SPIDevice spiDevice,
        int8_t mosi,
        int8_t miso,
        int8_t sck,
        int8_t cs,
        spi_bus_config_t buscfg,
        spi_device_interface_config_t devcfg) : SPI(spiDevice, mosi, miso, sck, cs) {
        esp_err_t ret;
        switch (spiDevice) {
        case SPI0:
            ret = spi_bus_initialize(HSPI_HOST, &buscfg, 1);
            if (ret != ESP_OK) {
                Log::Error("[MCU] Failed to Create SPI Controller");
                return;
            }

            ret = spi_bus_add_device(HSPI_HOST, &devcfg, &m_SPI);
            if (ret != ESP_OK) {
                spi_bus_free(HSPI_HOST);
                Log::Error("[MCU] Failed to Create SPI Controller");
                return;
            }
            break;
        case SPI1:
            ret = spi_bus_initialize(VSPI_HOST, &buscfg, 1);
            if (ret != ESP_OK) {
                Log::Error("[MCU] Failed to Create SPI Controller");
                return;
            }

            ret = spi_bus_add_device(VSPI_HOST, &devcfg, &m_SPI);
            if (ret != ESP_OK) {
                spi_bus_free(VSPI_HOST);
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

    ESP32SPI::~ESP32SPI() {
        spi_bus_remove_device(m_SPI);
        switch (m_SPIDevice) {
        case SPI0:
            spi_bus_free(HSPI_HOST);
            break;
        case SPI1:
            spi_bus_free(VSPI_HOST);
            break;
        }
    }

    void ESP32SPI::Write(uint8_t data) {
        if (!m_Initialized) {
            Log::Error("[MCU] SPI not initialized");
            return;
        }
        if (m_MOSI == -1) {
            Log::Error("[MCU] Write operation attempted without MOSI line configured");
            return;
        }
        Log::Trace("[MCU] SPI Write. Data: 0x%02X", data);
        GPIO::Write(m_CS, 0);
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = 8;
        t.tx_buffer = &data;
        esp_err_t ret = spi_device_polling_transmit(m_SPI, &t);
        if (ret != ESP_OK) {
            Log::Error("[MCU] Failed to Write to SPI");
        }
        GPIO::Write(m_CS, 1);
        Log::Trace("[MCU] SPI Write complete");
    }

    uint8_t ESP32SPI::Read() {
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
        GPIO::Write(m_CS, 0);
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
        GPIO::Write(m_CS, 1);
        Log::Trace("[MCU] SPI Read complete. Data: 0x%02X", rx_data);
        return rx_data;
    }
} // namespace MCU

#endif // MCU_ESP32
