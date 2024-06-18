#include "FrameBuffer.h"
#include "Log.h"
#include <esp_heap_caps.h>

namespace EPDL
{
    FrameBuffer::FrameBuffer(uint16_t width, uint16_t height, uint8_t pixelSize, bool psram) :
            m_Width(width), m_Height(height), m_PixelSize(pixelSize), m_Scale(8 / pixelSize),
            m_Mask(0xFF >> (8 - pixelSize)) {
        m_WidthBuffer = m_Width % m_Scale == 0 ? m_Width / m_Scale : m_Width / m_Scale + 1;
        if (psram) {
            m_Data = (uint8_t*) heap_caps_malloc(m_Height * m_WidthBuffer, MALLOC_CAP_SPIRAM);
        }
        else {
            m_Data = new uint8_t[m_Height * m_WidthBuffer];
        }
        if (m_Data == nullptr) {
            Log::Error("[EPDL] Failed to allocate memory for FrameBuffer");
        }

        m_Allocated = true;
    }

    bool FrameBuffer::SetPixel(int x, int y, uint8_t color) {
        if (!m_Allocated) return false;
        if (x < 0 || x >= m_Width || y < 0 || y >= m_Height) {
            return false;
        }
        uint8_t pixel = m_Data[y * m_WidthBuffer + x / m_Scale];
        uint8_t shift = m_PixelSize * (m_Scale - 1 - (x % m_Scale));
        pixel &= ~(m_Mask << shift);
        pixel |= (color & m_Mask) << shift;
        m_Data[y * m_WidthBuffer + x / m_Scale] = pixel;
        return true;
    }

    uint8_t FrameBuffer::GetPixel(int x, int y) const {
        if (!m_Allocated) return 0x0;
        if (x < 0 || x >= m_Width || y < 0 || y >= m_Height) {
            return 0x0;
        }
        uint8_t pixel = m_Data[y * m_WidthBuffer + x / m_Scale];
        uint8_t shift = m_PixelSize * (m_Scale - 1 - (x % m_Scale));
        return (pixel >> shift) & m_Mask;
    }

    void FrameBuffer::ClearColor(const uint8_t color) {
        if (!m_Allocated) return;
        uint8_t scaledColor = 0x0;
        for (uint8_t i = 0; i < m_Scale; i++) {
            scaledColor |= (color & m_Mask) << m_PixelSize * i;
        }
        for (uint16_t i = 0; i < m_Height; i++) {
            for (uint16_t j = 0; j < m_WidthBuffer; j++) {
                m_Data[i * m_WidthBuffer + j] = scaledColor;
            }
        }
    }
} // namespace EPDL