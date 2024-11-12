#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "WS_7IN3G.h"
#include <Drivers/GPIO.h>
#include <Drivers/SPI.h>

WS_7IN3G::WS_7IN3G(int busyPin, int resetPin, int dcPin, int csPin, int sckPin, int mosiPin) :
        Driver(busyPin, resetPin, dcPin, csPin, sckPin, mosiPin),
        m_FrameBuffer(WIDTH, HEIGHT, COLOR_DEPTH),
        m_ColorPalette({
                               {0,   0,   0},   // Black (0b00)
                               {255, 255, 255}, // White (0b01)
                               {255, 255, 0},   // Yellow (0b10)
                               {255, 0,   0}    // Red (0b11)
                       }) {
    GPIO::SetMode(busyPin, GPIO::Mode::Input);
    GPIO::SetMode(resetPin, GPIO::Mode::Output);
    GPIO::SetMode(dcPin, GPIO::Mode::Output);
    GPIO::SetMode(csPin, GPIO::Mode::Output);
    GPIO::SetMode(sckPin, GPIO::Mode::Output);
    GPIO::SetMode(mosiPin, GPIO::Mode::Input);
    m_SPIController.SetFrequency(SPIFreq::MHZ_20);
    m_SPIController.SetPins(mosiPin, -1, sckPin, csPin);
    m_DriverInfo = {
            .Width = WIDTH,
            .Height = HEIGHT,
            .ColorDepth = COLOR_DEPTH,
            .PartialRefresh = false,
            .DriverName = "WaveShare 7.3 inch 4-Color"
    };
}

void WS_7IN3G::Initialize() {
    spdlog::debug("{} Initializing WS_7IN3G Display Driver", LOG_TAG);

    m_SPIController.Start(SPIDevice::SPI2);

    // Software reset
    Reset();

    // Initialize Display Register
    SendCommand(0xAA);
    SendData(0x49);
    SendData(0x55);
    SendData(0x20);
    SendData(0x08);
    SendData(0x09);
    SendData(0x18);

    SendCommand(0x01);
    SendData(0x3F);

    SendCommand(0x00);
    SendData(0x4F);
    SendData(0x69);

    SendCommand(0x05);
    SendData(0x40);
    SendData(0x1F);
    SendData(0x1F);
    SendData(0x2C);

    SendCommand(0x08);
    SendData(0x6F);
    SendData(0x1F);
    SendData(0x1F);
    SendData(0x22);

    SendCommand(0x06);
    SendData(0x6F);
    SendData(0x1F);
    SendData(0x14);
    SendData(0x14);

    SendCommand(0x03);
    SendData(0x00);
    SendData(0x54);
    SendData(0x00);
    SendData(0x44);

    SendCommand(0x60);
    SendData(0x02);
    SendData(0x00);

    SendCommand(0x30);
    SendData(0x08);

    SendCommand(0x50);
    SendData(0x3F);

    SendCommand(0x61);
    SendData(0x03);
    SendData(0x20);
    SendData(0x01);
    SendData(0xE0);

    SendCommand(0xE3);
    SendData(0x2F);

    SendCommand(0x84);
    SendData(0x01);

    spdlog::info("{} Initialized WS_7IN3G Display Driver", LOG_TAG);
}

void WS_7IN3G::DrawImage(int handle, int x_offset, int y_offset) {
    spdlog::debug("{} Drawing image {} at ({}, {})", LOG_TAG, handle, x_offset, y_offset);
    if (PNGs.find(handle) == PNGs.end()) {
        spdlog::error("{} Image handle {} not found", LOG_TAG, handle);
        return;
    }
    auto filename = *PNGs[handle];
    PNGDecoder::FileOps fileOps = PNGDecoder::SDFileOps();
    m_PNGDecoder.Decode(filename, [this, x_offset, y_offset](int x, int y, const RGB &color) {
        uint8_t _color = this->m_ColorPalette.GetClosestColor(color);
        this->m_FrameBuffer.SetPixel(x + x_offset, y + y_offset, _color);
    }, fileOps, true);
}

void WS_7IN3G::BeginFrame() {
    spdlog::debug("{} Begin frame", LOG_TAG);
    m_FrameBuffer.ClearColor(Color::White);
    Initialize();
    PowerOn();
}

void WS_7IN3G::EndFrame() {
    spdlog::debug("{} End frame", LOG_TAG);
    Refresh();
    PowerOff();
    Sleep();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void WS_7IN3G::SwapBuffers() {
    spdlog::debug("{} Swap buffers", LOG_TAG);
    uint16_t scale = 8 / COLOR_DEPTH;
    StartDataTransmission();
    for (uint16_t j = 0; j < HEIGHT; j++) {
        for (uint16_t i = 0; i < WIDTH / scale; i++) {
            uint8_t data = 0;
            for (uint8_t k = 0; k < 4; k++) {
                data |= m_FrameBuffer.GetPixel(i * 4 + k, j) << (6 - k * 2);
            }
            SendData(data);
        }
    }
}

void WS_7IN3G::SendCommand(uint8_t command) {
    GPIO::Write(m_DCPin, 0);
    m_SPIController.Write(command);
}

void WS_7IN3G::SendData(uint8_t data) {
    GPIO::Write(m_DCPin, 1);
    m_SPIController.Write(data);
}

void WS_7IN3G::WaitUntilReady() {
    //LOW: idle, HIGH: busy
    spdlog::debug("{} Waiting for display", LOG_TAG);
    while (GPIO::Read(m_BusyPin) == 0) {
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    spdlog::debug("{} Display ready", LOG_TAG);
}

void WS_7IN3G::Reset() {
    GPIO::Write(m_ResetPin, 1);
    vTaskDelay(20 / portTICK_PERIOD_MS);
    GPIO::Write(m_ResetPin, 0);
    vTaskDelay(2 / portTICK_PERIOD_MS);
    GPIO::Write(m_ResetPin, 1);
    vTaskDelay(20 / portTICK_PERIOD_MS);
    WaitUntilReady();
    vTaskDelay(100 / portTICK_PERIOD_MS);
}

void WS_7IN3G::PowerOff() {
    WaitUntilReady();
    SendCommand(0x02);
    SendData(0x00);
}

void WS_7IN3G::PowerOn() {
    SendCommand(0x04);
}

void WS_7IN3G::Sleep() {
    WaitUntilReady();
    SendCommand(0x07);
    SendData(0xA5);
}

void WS_7IN3G::StartDataTransmission() {
    WaitUntilReady();
    SendCommand(0x10);
}

void WS_7IN3G::Refresh() {
    SendCommand(0x12);
    SendData(0x00);
}
