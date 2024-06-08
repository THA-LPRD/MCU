#include "WS_9IN7.h"
#include "Log.h"
#include <MCU.h>
#include <GPIO.h>
#include <SPI.h>

namespace EPDL
{
    WS_9IN7::WS_9IN7() : m_FrameBuffer(m_Width, m_Height, m_PixelSize) {
        Log::Debug("[EPDL] Initializing WS_9IN7 display driver");
        // MISO Pin is labeled DC on EPD
        m_SPIController = MCU::SPI::Create(static_cast<MCU::SPIDevice>(EPDL::SPI::SPIDevice),
                                           EPDL::Pin::MOSI,
                                           EPDL::Pin::DC,
                                           EPDL::Pin::SCK,
                                           EPDL::Pin::CS,
                                           false);
        // Lesezeichen
        // Software reset
        Reset();

        // GetDisplayInfo();
        

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

        BeginFrame();
        SwapBuffers();
        EndFrame();

        Log::Debug("[EPDL] WS_9IN7 display driver initialized");
    }

    WS_9IN7::~WS_9IN7() {
        Log::Debug("[EPDL] Deinitializing WS_9IN7 display driver");
        BeginFrame();
        SwapBuffers();
        EndFrame();
        Log::Debug("[EPDL] WS_9IN7 display driver deinitialized");
    }

    void WS_9IN7::DrawImage(ImageHandle handle, int x, int y) {
        Log::Debug("[EPDL] Drawing image %d at (%d, %d)", handle, x, y);
        ImageData* imageData = m_ImageData[handle].get();
        for (uint16_t j = 0; j < imageData->GetHeight(); j++) {
            for (uint16_t i = 0; i < imageData->GetWidth(); i++) {
                if (i + x >= m_Width || j + y >= m_Height) {
                    continue;
                }
                m_FrameBuffer.SetPixel(i + x, j + y, imageData->GetPixel(i, j));
            }
        }
    }

    void WS_9IN7::BeginFrame() {
        Log::Debug("[EPDL] Begin frame");
        m_FrameBuffer.ClearColor(Color::White);
        PowerOn();
    }

    void WS_9IN7::EndFrame() {
        Log::Debug("[EPDL] End frame");
        Refresh();
        PowerOff();
        MCU::Sleep(1000);
    }

    void WS_9IN7::SwapBuffers() {
        Log::Debug("[EPDL] Swap buffers");
        uint16_t scale = 8 / m_PixelSize;
        StartDataTransmission();
        for (uint16_t j = 0; j < m_Height; j++) {
            for (uint16_t i = 0; i < m_Width / scale; i++) {
                uint8_t data = 0;
                for (uint8_t k = 0; k < 4; k++) {
                    data |= m_FrameBuffer.GetPixel(i * 4 + k, j) << (6 - k * 2);
                }
                SendData(data);
            }
        }
    }

    void WS_9IN7::SendCommand(uint8_t command) {
        MCU::GPIO::Write(EPDL::Pin::DC, 0);
        m_SPIController->Write(command);
    }

    void WS_9IN7::SendData(uint8_t data) {
        MCU::GPIO::Write(EPDL::Pin::DC, 1);
        m_SPIController->Write(data);
    }

    void WS_9IN7::WaitUntilReady() {
        //LOW: idle, HIGH: busy
        Log::Debug("[EPDL] Waiting for display");


        while (MCU::GPIO::Read(EPDL::Pin::BUSY) == 0) {
            MCU::Sleep(5);
        }
        Log::Debug("[EPDL] Display ready");
    }

    void WS_9IN7::Reset() {
        MCU::GPIO::Write(EPDL::Pin::RST, 0);
        MCU::Sleep(1000);
        MCU::GPIO::Write(EPDL::Pin::RST, 1);
        MCU::Sleep(20);
    }

    void WS_9IN7::PowerOff() {
        WaitUntilReady();
        SendCommand(0x02);
        SendData(0x00);
    }

    void WS_9IN7::PowerOn() {
        SendCommand(0x04);
    }

    void WS_9IN7::Sleep() {
        WaitUntilReady();
        SendCommand(0x07);
        SendData(0xA5);
    }

    void WS_9IN7::StartDataTransmission() {
        WaitUntilReady();
        SendCommand(0x10);
    }

    void WS_9IN7::Refresh() {
        SendCommand(0x12);
        SendData(0x00);
    }
} // namespace EPDL
