#include "WS_9IN7.h"
#include <Log.h>
#include "pic.h"

namespace EPDL
{
    WS_9IN7::WS_9IN7() : IT8951E(), m_FrameBuffer(m_Width, m_Height, 4) {
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
        for (int i = 0; i < 80000; i++) {
            uint8_t byte = pic[i];
            uint8_t extracted = byte & 0xf0;
            extracted = extracted >> 4;
            m_FrameBuffer.SetPixel((i * 2) % 400, i / 200, extracted);
            extracted = (byte & 0x0f);
            m_FrameBuffer.SetPixel((i * 2 + 1) % 400, i / 200, extracted);
        }
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
                for (int i = 3; i >= 0; i--) {
                    uint16_t temp = m_FrameBuffer.GetPixel(x + 3 - i, y);
                    pixel |= temp << (i * 4);
                    if (y < 1 && (x + 3 - i) < 20) {
                    }
                }
                uint16_t temp1 = (pixel & 0x00ff) << 8;
                uint16_t temp2 = (pixel & 0xff00) >> 8;
                pixel = 0;
                pixel |= temp1;
                pixel |= temp2;

                SendData(&pixel);
            }
        }
    }
} // EPDL
