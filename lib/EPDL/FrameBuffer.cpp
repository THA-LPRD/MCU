#include "FrameBuffer.h"
#include "Log.h"

namespace EPDL
{
    FrameBuffer::FrameBuffer(uint16_t width, uint16_t height, uint8_t pixelSize) :
            m_Width(width), m_Height(height), m_PixelSize(pixelSize), m_Scale(8 / pixelSize),
            m_Mask(0xFF >> (8 - pixelSize)) {
        m_WidthBuffer = m_Width % m_Scale == 0 ? m_Width / m_Scale : m_Width / m_Scale + 1;
        m_Data.resize(m_Height);
        for (uint16_t i = 0; i < m_Height; i++) {
            m_Data[i].resize(m_WidthBuffer);
        }
    }

    bool FrameBuffer::SetPixel(int x, int y, uint8_t color) {
        if (x < 0 || x >= m_Width || y < 0 || y >= m_Height) {
            return false;
        }

        uint8_t pixel = m_Data[y][x / m_Scale];
        uint8_t shift = m_PixelSize * (3 - (x % m_Scale));
        pixel &= ~(m_Mask << shift);
        pixel |= (color & m_Mask) << shift;
        m_Data[y][x / m_Scale] = pixel;
        return true;
    }

    uint8_t FrameBuffer::GetPixel(int x, int y) const {
        if (x < 0 || x >= m_Width || y < 0 || y >= m_Height) {
            return 0x0;
        }
        uint8_t pixel = m_Data[y][x / m_Scale];
        uint8_t shift = m_PixelSize * (3 - (x % m_Scale));
        return (pixel >> shift) & m_Mask;
    }

    void FrameBuffer::ClearColor(const uint8_t color) {
        uint8_t scaledColor = 0x0;
        for (uint8_t i = 0; i < m_Scale; i++) {
            scaledColor |= (color & m_Mask) << m_PixelSize * i;
        }
        for (auto &row: m_Data) {
            std::fill(row.begin(), row.end(), scaledColor);
        }
    }
} // namespace EPDL