#ifndef LPRD_MCU_FRAMEBUFFER_H
#define LPRD_MCU_FRAMEBUFFER_H

#include <vector>
#include <cstdint>

class FrameBuffer {
public:
    FrameBuffer(uint16_t width, uint16_t height, uint8_t pixelSize, bool psram = true);
    ~FrameBuffer() = default;
    bool SetPixel(int x, int y, uint8_t color);
    [[nodiscard]] uint8_t GetPixel(int x, int y) const;
    void ClearColor(uint8_t color);
    [[nodiscard]] inline uint8_t GetPixelSize() const { return m_PixelSize; }
    [[nodiscard]] inline const uint8_t* GetData() const { return m_Data; }
private:
    uint8_t* m_Data;
    bool m_Allocated = false;
    uint16_t m_Width;
    uint16_t m_WidthBuffer;
    uint16_t m_Height;
    uint8_t m_PixelSize = 8;
    uint8_t m_Scale;
    uint8_t m_Mask;
};

#endif //LPRD_MCU_FRAMEBUFFER_H
