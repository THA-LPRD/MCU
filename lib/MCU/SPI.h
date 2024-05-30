#ifndef MCU_SPI_H
#define MCU_SPI_H

#include <cstdint>

namespace MCU
{
    class SPI {
    public:
        SPI(uint8_t mosi, uint8_t miso, uint8_t sck, uint8_t cs) : m_MOSI(mosi), m_MISO(miso), m_SCK(sck), m_CS(cs) {}
        ~SPI() = default;
        static SPI* Create(int8_t mosi, int8_t miso, int8_t sck, int8_t cs, bool swspi = false);
        virtual void Write(uint8_t data) = 0;
        virtual uint8_t Read() = 0;
    protected:
        bool m_Initialized = false;
        uint8_t m_MOSI;
        uint8_t m_MISO;
        uint8_t m_SCK;
        uint8_t m_CS;
    private:
        static SPI* CreateHardwareSPI(int8_t mosi, int8_t miso, int8_t sck, int8_t cs);
    };

    class SWSPI : public SPI {
    public:
        SWSPI(int8_t mosi, int8_t miso, int8_t sck, int8_t cs);
        ~SWSPI() = default;
        void Write(uint8_t data) override;
        uint8_t Read() override;
    };
} // namespace MCU

#endif //MCU_SPI_H
