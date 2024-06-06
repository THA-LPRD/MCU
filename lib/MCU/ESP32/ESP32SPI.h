#ifdef MCU_ESP32
#ifndef MCU_ESP32SPI_H
#define MCU_ESP32SPI_H

#include "SPI.h"
#include "driver/spi_master.h"

namespace MCU
{
    class ESP32SPI : public SPI
    {
    public:
        ESP32SPI(int8_t mosi, int8_t miso, int8_t sck, int8_t cs);
        ESP32SPI(int8_t mosi, int8_t miso, int8_t sck, int8_t cs, spi_bus_config_t buscfg,
                 spi_device_interface_config_t devcfg);
        ~ESP32SPI();
        void Write(uint8_t data) override;
        uint8_t Read() override;
    private:
//        void Initialize();
        spi_device_handle_t m_SPI = nullptr;

    };
} // namespace MCU

#endif //MCU_ESP32SPI_H
#endif // MCU_ESP32


