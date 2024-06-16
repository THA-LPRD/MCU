#include "WS_9IN7.h"
#include <Log.h>

namespace EPDL
{
    WS_9IN7::WS_9IN7() :
            IT8951E(), m_FrameBuffer(m_Width, m_Height, m_PixelSize),
            m_ColorPalette({
                                   {0,   0,   0}, // Black (0b0000)
                                   {17,  17,  17}, // DarkGray1 (0b0001)
                                   {34, 34, 34}, // DarkGray2 (0b0010)
                                   {51, 51, 51}, // DarkGray3 (0b0011)
                                   {68, 68, 68}, // Gray1 (0b0100)
                                   {85, 85, 85}, // Gray2 (0b0101)
                                   {102, 102, 102}, // Gray3 (0b0110)
                                   {119, 119, 119}, // Gray4 (0b0111)
                                   {136, 136, 136}, // LightGray1 (0b1000)
                                   {153, 153, 153}, // LightGray2 (0b1001)
                                   {170, 170, 170}, // LightGray3 (0b1010)
                                   {187, 187, 187}, // LightGray4 (0b1011)
                                   {204, 204, 204}, // LightGray5 (0b1100)
                                   {221, 221, 221}, // LightGray6 (0b1101)
                                   {238, 238, 238}, // LightGray7 (0b1110)
                                   {255, 255, 255} // White (0b1111)
                           }) {
        Log::Debug("[EPDL] Initializing WS_9IN7 display driver");
        if (m_DeviceInfo.PanelWidth != m_Width || m_DeviceInfo.PanelHeight != m_Height) {
            Log::Error("[EPDL] Panel size mismatch:");
            Log::Error("[EPDL] Expected: %d x %d", m_Width, m_Height);
            Log::Error("[EPDL] Actual: %d x %d", m_DeviceInfo.PanelWidth, m_DeviceInfo.PanelHeight);
        }
        Log::Debug("[EPDL] WS_9IN7 display driver initialized");
    }

    void WS_9IN7::DrawImage(ImageHandle handle, int x, int y) {
        Log::Debug("[EPDL] Drawing image %d at (%d, %d)", handle, x, y);
        ImageData* imageData = m_ImageData[handle].get();
        ImageData::DecodeInfo decodeInfo{
                &m_FrameBuffer,
                &m_ColorPalette,
                static_cast<size_t>(x),
                static_cast<size_t>(y)
        };
        imageData->DrawImage(decodeInfo);
    }

    void WS_9IN7::BeginFrame() {
        Log::Debug("[EPDL] Begin frame");
        m_FrameBuffer.ClearColor(Color::White);
        StartTransmission();
    }

    void WS_9IN7::EndFrame() {
        Log::Debug("[EPDL] End frame");
        EndTransmission();
    }

    void WS_9IN7::SwapBuffers() {
        Log::Debug("[EPDL] Swap buffers");
        for (int y = 0; y < m_Height; y++) {
            for (int x = 0; x < m_Width; x += 4) {
                uint16_t pixel = 0;
                pixel = m_FrameBuffer.GetPixel(x, y);
                pixel |= m_FrameBuffer.GetPixel(x + 1, y) << 4;
                pixel |= m_FrameBuffer.GetPixel(x + 2, y) << 8;
                pixel |= m_FrameBuffer.GetPixel(x + 3, y) << 12;
                SendData(&pixel);
            }
        }
    }
} // EPDL
