#include "WS_7IN3G.h"
#include "Log.h"
#include <MCU.h>
#include <GPIO.h>
#include <SPI.h>

namespace EPDL
{
    WS_7IN3G::WS_7IN3G() : m_FrameBuffer(m_Width, m_Height, m_PixelSize) {
        Log::Debug("[EPDL] Initializing WS_7IN3G display driver");

        m_SPIController = MCU::SPI::Create(EPDL::Pin::MOSI, -1, EPDL::Pin::SCK, EPDL::Pin::CS, false);

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

        BeginFrame();
        SwapBuffers();
        EndFrame();

        Log::Debug("[EPDL] WS_7IN3G display driver initialized");
    }

    WS_7IN3G::~WS_7IN3G() {
        Log::Debug("[EPDL] Deinitializing WS_7IN3G display driver");
        BeginFrame();
        SwapBuffers();
        EndFrame();
        Log::Debug("[EPDL] WS_7IN3G display driver deinitialized");
    }

    void WS_7IN3G::DrawImage(ImageHandle handle, int x, int y) {
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

    void WS_7IN3G::BeginFrame() {
        Log::Debug("[EPDL] Begin frame");
        m_FrameBuffer.ClearColor(Color::White);
        PowerOn();
    }

    void WS_7IN3G::EndFrame() {
        Log::Debug("[EPDL] End frame");
        Refresh();
        PowerOff();
        MCU::Sleep(1000);
    }

    void WS_7IN3G::SwapBuffers() {
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

    void WS_7IN3G::SendCommand(uint8_t command) {
        MCU::GPIO::Write(EPDL::Pin::DC, 0);
        m_SPIController->Write(command);
    }

    void WS_7IN3G::SendData(uint8_t data) {
        MCU::GPIO::Write(EPDL::Pin::DC, 1);
        m_SPIController->Write(data);
    }

    void WS_7IN3G::WaitUntilReady() {
        //LOW: idle, HIGH: busy
        Log::Debug("[EPDL] Waiting for display");
        while (MCU::GPIO::Read(EPDL::Pin::BUSY) == 0) {
            MCU::Sleep(5);
        }
        Log::Debug("[EPDL] Display ready");
    }

    void WS_7IN3G::Reset(){
        MCU::GPIO::Write(EPDL::Pin::RST, 1);
        MCU::Sleep(20);
        MCU::GPIO::Write(EPDL::Pin::RST, 0);
        MCU::Sleep(2);
        MCU::GPIO::Write(EPDL::Pin::RST, 1);
        MCU::Sleep(20);
        WaitUntilReady();
        MCU::Sleep(100);
    }

    void WS_7IN3G::PowerOff(){
        WaitUntilReady();
        SendCommand(0x02);
        SendData(0x00);
    }

    void WS_7IN3G::PowerOn(){
        SendCommand(0x04);
    }

    void WS_7IN3G::Sleep(){
        WaitUntilReady();
        SendCommand(0x07);
        SendData(0xA5);
    }

    void WS_7IN3G::StartDataTransmission(){
        WaitUntilReady();
        SendCommand(0x10);
    }

    void WS_7IN3G::Refresh(){
        SendCommand(0x12);
        SendData(0x00);
    }
} // namespace EPDL
