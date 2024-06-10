#ifndef FRAMEBUFFER_H_
#define FRAMEBUFFER_H_

#include <vector>
#include <cstdint>

namespace EPDL
{
    class FrameBuffer {
    public:
        FrameBuffer(uint16_t width, uint16_t height, uint8_t pixelSize);
        ~FrameBuffer() = default;
        bool SetPixel(int x, int y, uint8_t color);
        uint8_t GetPixel(int x, int y) const;
        void ClearColor(const uint8_t color);
        inline uint8_t GetPixelSize() const { return m_PixelSize; }
        inline const std::vector<std::vector<uint8_t>> &GetData() const { return m_Data; }
    private:
        std::vector<std::vector<uint8_t>> m_Data;
        uint16_t m_Width;
        uint16_t m_WidthBuffer;
        uint16_t m_Height;
        uint8_t m_PixelSize = 8;
        uint8_t m_Scale;
        uint8_t m_Mask;
    };
} // namespace EPDL

#endif /*FRAMEBUFFER_H_*/