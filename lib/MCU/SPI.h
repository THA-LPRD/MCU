#ifndef MCU_SPI_H
#define MCU_SPI_H

#include <vector>
#include <string>
#include <string_view>
#include <cstdint>

namespace MCU
{

    enum SPIDevice {
        SPI0 = 0,
        SPI1 = 1,
        SPI2 = 2,
        SPI3 = 3,
        MAX = 4
    };

    class SPI {
    public:
        SPI(SPIDevice spiDevice, uint8_t mosi, uint8_t miso, uint8_t sck, uint8_t cs) :
                m_SPIDevice(spiDevice), m_MOSI(mosi), m_MISO(miso), m_SCK(sck), m_CS(cs) {}
        virtual ~SPI() = default;
        static SPI* Create(SPIDevice spiDevice, int8_t mosi, int8_t miso, int8_t sck, int8_t cs, bool swspi = false);
        virtual void Write(uint8_t data) = 0;
        virtual void Write(uint8_t* data, size_t length) = 0;
        virtual uint8_t Read() = 0;
        virtual std::vector<u_int8_t> Read(size_t length) = 0;
    protected:
        bool m_Initialized = false;
        SPIDevice m_SPIDevice;
        uint8_t m_MOSI;
        uint8_t m_MISO;
        uint8_t m_SCK;
        uint8_t m_CS;
    private:
        static SPI* CreateHardwareSPI(SPIDevice spiDevice, int8_t mosi, int8_t miso, int8_t sck, int8_t cs);
    };

    class SWSPI : public SPI {
    public:
        SWSPI(SPIDevice spiDevice, int8_t mosi, int8_t miso, int8_t sck, int8_t cs);
        ~SWSPI() override = default;
        void Write(uint8_t data) override;
        void Write(uint8_t* data, size_t length) override;
        uint8_t Read() override;
        std::vector<u_int8_t> Read(size_t length) override;
    };
} // namespace MCU

#endif //MCU_SPI_H
