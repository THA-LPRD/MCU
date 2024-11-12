#ifndef MCU_SPI_H
#define MCU_SPI_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include "driver/spi_master.h"

enum SPIDevice {
    SPI1,
    SPI2,
    SPI3,
    SPI4
};

enum SPIFreq : int {
    MHZ_8 = SPI_MASTER_FREQ_8M,
    MHZ_9 = SPI_MASTER_FREQ_9M,
    MHZ_10 = SPI_MASTER_FREQ_10M,
    MHZ_11 = SPI_MASTER_FREQ_11M,
    MHZ_13 = SPI_MASTER_FREQ_13M,
    MHZ_16 = SPI_MASTER_FREQ_16M,
    MHZ_20 = SPI_MASTER_FREQ_20M,
    MHZ_26 = SPI_MASTER_FREQ_26M,
    MHZ_40 = SPI_MASTER_FREQ_40M,
    MHZ_80 = SPI_MASTER_FREQ_80M
};

class SPI {
public:
    explicit SPI(int mosi = -1, int miso = -1, int sck = -1, int cs = -1);
    ~SPI();
    void SetPins(int mosi, int miso, int sck, int cs = -1);
    void SetFrequency(SPIFreq freq);
    bool Start(SPIDevice spiDevice);
    void Write(uint8_t data);
    void Write(const uint8_t* data, size_t length);
    uint8_t Read();
    std::vector<u_int8_t> Read(size_t length);
private:
    static constexpr const char* LOG_TAG = "[SPIManager] -";
    spi_device_handle_t m_SPI = nullptr;
    SPIDevice m_SPIDevice;
    int m_MOSI = -1;
    int m_MISO = -1;
    int m_SCK = -1;
    int m_CS = -1;
    SPIFreq m_Frequency = MHZ_8;
    spi_bus_config_t m_BusCfg;
    spi_device_interface_config_t m_DevCfg;
    bool m_Initialized = false;
};


#endif //MCU_SPI_H
