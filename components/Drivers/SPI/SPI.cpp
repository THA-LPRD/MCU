#include <spdlog/spdlog.h>
#include "Drivers/SPI.h"
#include <cstring>

SPI::SPI(int mosi, int miso, int sck, int cs) {
    if (mosi == -1 && miso == -1 && sck == -1 && cs == -1) return;
    SetPins(mosi, miso, sck, cs);
}

SPI::~SPI() {
    if (!m_Initialized) return;
    if (ESP_OK != spi_bus_remove_device(m_SPI)) {
        spdlog::error("{} Failed to remove SPI device {}", LOG_TAG, static_cast<int>(m_SPIDevice));
        return;
    }
    if (ESP_OK != spi_bus_free(static_cast<spi_host_device_t>(m_SPIDevice))) {
        spdlog::error("{} Failed to free SPI bus {}", LOG_TAG, static_cast<int>(m_SPIDevice));
    }
}

void SPI::SetPins(int mosi, int miso, int sck, int cs) {
    bool wasInitialized = m_Initialized;
    m_Initialized = false;

    if (wasInitialized) {
        if (ESP_OK != spi_bus_remove_device(m_SPI)) {
            spdlog::error("{} Failed to remove SPI device while reconfiguring pins", LOG_TAG);
            m_Initialized = true;
            return;
        }
        if (ESP_OK != spi_bus_free(static_cast<spi_host_device_t>(m_SPIDevice))) {
            spdlog::error("{} Failed to free SPI bus while reconfiguring pins", LOG_TAG);
            return;
        }
    }

    m_MOSI = mosi;
    m_MISO = miso;
    m_SCK = sck;
    m_CS = cs;

    if (m_MOSI == -1 && m_MISO == -1) {
        spdlog::warn("{} SPI configured without MOSI and MISO - limited functionality", LOG_TAG);
    }
    if (m_SCK == -1) {
        spdlog::error("{}Invalid SPI configuration: SCK cannot be -1", LOG_TAG);
        return;
    }

    // Update bus configuration
    m_BusCfg = {};
    m_BusCfg.mosi_io_num = m_MOSI;
    m_BusCfg.miso_io_num = m_MISO;
    m_BusCfg.sclk_io_num = m_SCK;
    m_BusCfg.quadwp_io_num = -1;
    m_BusCfg.quadhd_io_num = -1;
    m_BusCfg.max_transfer_sz = 4094;

    m_DevCfg = {};
    m_DevCfg.mode = 0;
    m_DevCfg.clock_speed_hz = static_cast<int>(m_Frequency);
    m_DevCfg.spics_io_num = m_CS;
    m_DevCfg.queue_size = 7;

    if (wasInitialized) {
        Start(m_SPIDevice);
    }
}

void SPI::SetFrequency(SPIFreq freq) {
    bool wasInitialized = m_Initialized;
    m_Initialized = false;

    if (wasInitialized) {
        if (ESP_OK != spi_bus_remove_device(m_SPI)) {
            spdlog::error("{} Failed to remove SPI device while reconfiguring pins", LOG_TAG);
            m_Initialized = true;
            return;
        }
        if (ESP_OK != spi_bus_free(static_cast<spi_host_device_t>(m_SPIDevice))) {
            spdlog::error("{} Failed to free SPI bus while reconfiguring pins", LOG_TAG);
            return;
        }
    }

    m_Frequency = freq;
    m_DevCfg.clock_speed_hz = static_cast<int>(m_Frequency);

    if (wasInitialized) {
        Start(m_SPIDevice);
    }
}

bool SPI::Start(SPIDevice spiDevice) {
    if (m_SCK == -1) {
        spdlog::error("{} Cannot start SPI: SCK pin not configured", LOG_TAG);
        m_Initialized = false;
        return false;
    }

    if (m_MOSI == -1 && m_MISO == -1) {
        spdlog::error("{} Cannot start SPI: At least one of MOSI or MISO must be configured", LOG_TAG);
        m_Initialized = false;
        return false;
    }

    m_SPIDevice = spiDevice;

    if (ESP_OK != spi_bus_initialize(static_cast<spi_host_device_t>(spiDevice), &m_BusCfg, SPI_DMA_CH_AUTO)) {
        spdlog::error("{} Failed to initialize SPI bus", LOG_TAG);
        m_Initialized = false;
        return false;
    }

    if (ESP_OK != spi_bus_add_device(static_cast<spi_host_device_t>(spiDevice), &m_DevCfg, &m_SPI)) {
        spdlog::error("{} Failed to add SPI device", LOG_TAG);
        spi_bus_free(static_cast<spi_host_device_t>(spiDevice));
        m_Initialized = false;
        return false;
    }

    m_Initialized = true;
    return true;
}

void SPI::Write(uint8_t data) {
    if (!m_Initialized) {
        spdlog::error("{} SPI not initialized", LOG_TAG);
        return;
    }
    if (m_MOSI == -1) {
        spdlog::error("{} Write operation attempted without MOSI line configured", LOG_TAG);
        return;
    }
//    spdlog::trace("{} SPI Write. Data: 0x{:02X}", LOG_TAG, data); // Too verbose
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.rxlength = 8;
    t.tx_buffer = &data;
    esp_err_t ret = spi_device_polling_transmit(m_SPI, &t);
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to Write to SPI", LOG_TAG);
    }
//    spdlog::trace("{} SPI Write complete", LOG_TAG); // Too verbose
}

void SPI::Write(const uint8_t* data, size_t length) {
    if (!m_Initialized) {
        spdlog::error("{} SPI not initialized", LOG_TAG);
        return;
    }
    if (m_MOSI == -1) {
        spdlog::error("{} Write operation attempted without MOSI line configured", LOG_TAG);
        return;
    }

//    spdlog::trace("{} SPI Write. Data: [{:02x}]", LOG_TAG, fmt::join(data, data + length, " ")); // too verbose

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = length * 8;
    t.rxlength = length * 8;
    t.tx_buffer = data;
    esp_err_t ret = spi_device_polling_transmit(m_SPI, &t);
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to Write to SPI", LOG_TAG);
    }
//    spdlog::trace("{} SPI Write complete", LOG_TAG); // too verbose
}

uint8_t SPI::Read() {
    if (!m_Initialized) {
        spdlog::error("{} SPI not initialized", LOG_TAG);
        return 0;
    }
    if (m_MISO == -1) {
        spdlog::error("{} Read operation attempted without MISO line configured", LOG_TAG);
        return 0;
    }
//    spdlog::trace("{} SPI Read.", LOG_TAG); // too verbose
    uint8_t rx_data = 0;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.rxlength = 8;
    t.rx_buffer = &rx_data;
    esp_err_t ret = spi_device_polling_transmit(m_SPI, &t);
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to Read from SPI", LOG_TAG);
        return 0;
    }
//    spdlog::trace("{} SPI Read complete. Data: 0x{:02X}", LOG_TAG, rx_data);
    return rx_data;
}

std::vector<u_int8_t> SPI::Read(size_t length) {
    if (!m_Initialized) {
        spdlog::error("{} SPI not initialized", LOG_TAG);
        return {};
    }
    if (m_MISO == -1) {
        spdlog::error("{} Read operation attempted without MISO line configured", LOG_TAG);
        return {};
    }
//    spdlog::trace("{} SPI Read.", LOG_TAG); // too verbose
    std::vector<u_int8_t> rx_data;
    rx_data.reserve(length);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8 * length;
    t.rxlength = 8 * length;
    t.rx_buffer = rx_data.data();
    esp_err_t ret = spi_device_polling_transmit(m_SPI, &t);
    if (ret != ESP_OK) {
        spdlog::error("{} Failed to Read from SPI", LOG_TAG);
        return {};
    }

//    spdlog::trace("{} SPI Read complete. Data: [{:02x}]", LOG_TAG, fmt::join(rx_data.begin(), rx_data.end(), " ")); // too verbose
    return rx_data;
}