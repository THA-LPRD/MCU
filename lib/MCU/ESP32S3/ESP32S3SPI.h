#ifdef MCU_ESP32S3
#ifndef MCU_ESP32S3SPI_H
#define MCU_ESP32S3SPI_H

#include "mySPI.h"
#include "driver/spi_master.h"

namespace MCU
{
    class ESP32S3SPI : public SPI {
    public:
        ESP32S3SPI(SPIDevice spiDevice, int8_t mosi, int8_t miso, int8_t sck, int8_t cs);
        ~ESP32S3SPI();
        void Write(uint8_t data) override;
        void Write(uint8_t* data, size_t length);
        uint8_t Read() override;
        std::vector<u_int8_t> Read(size_t length) override;
    private:
        ESP32S3SPI(SPIDevice spiDevice, int8_t mosi, int8_t miso, int8_t sck, int8_t cs, spi_bus_config_t buscfg,
                 spi_device_interface_config_t devcfg);
        spi_device_handle_t m_SPI = nullptr;
    };
} // namespace MCU

#endif //MCU_ESP32S3SPI_H
#endif // MCU_ESP32S3



