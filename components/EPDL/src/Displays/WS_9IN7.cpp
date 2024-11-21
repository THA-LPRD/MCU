#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "WS_9IN7.h"
#include <Drivers/GPIO.h>

WS_9IN7::WS_9IN7(int busyPin, int resetPin, int dcPin, int csPin, int sckPin, int mosiPin) :
        IT8951E(busyPin, resetPin, dcPin, csPin, sckPin, mosiPin),
        m_FrameBuffer(WIDTH, HEIGHT, COLOR_DEPTH),
        m_ColorPalette({
                               {0,   0,   0},   // Black (0b0000)
                               {17,  17,  17},  // DarkGray1 (0b0001)
                               {34,  34,  34},  // DarkGray2 (0b0010)
                               {51,  51,  51},  // DarkGray3 (0b0011)
                               {68,  68,  68},  // Gray1 (0b0100)
                               {85,  85,  85},  // Gray2 (0b0101)
                               {102, 102, 102}, // Gray3 (0b0110)
                               {119, 119, 119}, // Gray4 (0b0111)
                               {136, 136, 136}, // LightGray1 (0b1000)
                               {153, 153, 153}, // LightGray2 (0b1001)
                               {170, 170, 170}, // LightGray3 (0b1010)
                               {187, 187, 187}, // LightGray4 (0b1011)
                               {204, 204, 204}, // LightGray5 (0b1100)
                               {221, 221, 221}, // LightGray6 (0b1101)
                               {238, 238, 238}, // LightGray7 (0b1110)
                               {255, 255, 255}  // White (0b1111)
                       }) {
    m_DriverInfo = {
            .Width = WIDTH,
            .Height = HEIGHT,
            .ColorDepth = COLOR_DEPTH,
            .PartialRefresh = false,
            .DriverName = "WaveShare 9.7 inch Grayscale"
    };

}

void WS_9IN7::Initialize() {
    spdlog::debug("{} Initializing WS_9IN7 display driver", LOG_TAG);
    IT8951E::Initialize();
    if (m_DeviceInfo.PanelHeight != HEIGHT || m_DeviceInfo.PanelWidth != WIDTH) {
        spdlog::error("Panel size mismatch:", LOG_TAG);
        spdlog::error("{} Expected: {} x {}", LOG_TAG, WIDTH, HEIGHT);
        spdlog::error("{} Actual: {} x {}", LOG_TAG, m_DeviceInfo.PanelWidth, m_DeviceInfo.PanelHeight);
        return;
    }
    spdlog::debug("{} WS_9IN7 display driver initialized", LOG_TAG);
}

void WS_9IN7::DrawImage(int handle, int x_offset, int y_offset) {
    spdlog::debug("{} Drawing image {} at ({}, {})", LOG_TAG, handle, x_offset, y_offset);
    if (PNGs.find(handle) == PNGs.end()) {
        spdlog::error("{} Image handle {} not found", LOG_TAG, handle);
        return;
    }
    auto filename = *PNGs[handle];
    PNGDecoder::FileOps fileOps = PNGDecoder::LittleFSFileOps();
    m_PNGDecoder.Decode(filename, [this, x_offset, y_offset](int x, int y, const RGB &color) {
        uint8_t _color = this->m_ColorPalette.GetClosestColor(color);
        this->m_FrameBuffer.SetPixel(x + x_offset, y + y_offset, _color);
    }, fileOps, true);

}

void WS_9IN7::BeginFrame() {
    spdlog::debug("{} Begin frame", LOG_TAG);
    m_FrameBuffer.ClearColor(Color::White);
    Initialize();
    StartTransmission();
}

void WS_9IN7::EndFrame() {
    spdlog::debug("{} End frame", LOG_TAG);
    EndTransmission();
    Sleep();
}

void WS_9IN7::SwapBuffers() {
    spdlog::debug("{} Swap buffers", LOG_TAG);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x += 4) {
            uint16_t pixel = 0;
            pixel = m_FrameBuffer.GetPixel(x, y);
            pixel |= m_FrameBuffer.GetPixel(x + 1, y) << 4;
            pixel |= m_FrameBuffer.GetPixel(x + 2, y) << 8;
            pixel |= m_FrameBuffer.GetPixel(x + 3, y) << 12;
            SendData(&pixel);
        }
        // Yield Watchdog every 10 rows
        if (y % 10 == 0) vTaskDelay(2 / portTICK_PERIOD_MS);
    }
}