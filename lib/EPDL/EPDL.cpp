#include <memory>
#include "EPDL.h"
#include "Drivers/Driver.h"
#include "Drivers/WS_7IN3G.h"
#include <Log.h>
#include <SPI.h>
#include <GPIO.h>

namespace EPDL
{
    namespace
    { // Private members
        static std::unique_ptr<Driver> m_Driver = nullptr;
    } // namespace

    int Init() {
        MCU::GPIO::SetMode(EPDL::Pin::BUSY, MCU::GPIO::Mode::Input);
        MCU::GPIO::SetMode(EPDL::Pin::RST, MCU::GPIO::Mode::Output);
        MCU::GPIO::SetMode(EPDL::Pin::DC, MCU::GPIO::Mode::Output);

        MCU::GPIO::SetMode(EPDL::Pin::SCK, MCU::GPIO::Mode::Output);
        MCU::GPIO::SetMode(EPDL::Pin::MOSI, MCU::GPIO::Mode::Output);
        MCU::GPIO::SetMode(EPDL::Pin::CS, MCU::GPIO::Mode::Output);

        MCU::GPIO::Write(EPDL::Pin::CS, 1);
        MCU::GPIO::Write(EPDL::Pin::SCK, 0);
//        DEV_Module_Init();
        return 0;
    }

    void Terminate() {
        return;
    }

    void LoadDriver(std::string_view type) {
        if (type == "WS_7IN3F") {
            m_Driver = std::make_unique<WS_7IN3G>();
        }
        else {
            Log::Warning("[EPDL] Unknown display driver: %s", type.data());
            m_Driver = nullptr;
        }
    }

    ImageHandle CreateImage(std::unique_ptr<ImageData> data) {
        if (m_Driver == nullptr) {
            return -1;
        }
        return m_Driver->CreateImage(std::move(data));
    }

    void DeleteImage(ImageHandle handle) {
        if (m_Driver == nullptr) {
            return;
        }
        m_Driver->DeleteImage(handle);
    }

    void DrawImage(ImageHandle handle, uint16_t x, uint16_t y) {
        if (m_Driver == nullptr) {
            return;
        }
        m_Driver->DrawImage(handle, x, y);
    }

    void BeginFrame() {
        if (m_Driver == nullptr) {
            return;
        }
        m_Driver->BeginFrame();
    }

    void EndFrame() {
        if (m_Driver == nullptr) {
            return;
        }
        m_Driver->EndFrame();
    }

    void SwapBuffers() {
        if (m_Driver == nullptr) {
            return;
        }
        m_Driver->SwapBuffers();
    }

    uint16_t GetWidth() {
        if (m_Driver == nullptr) {
            return 0;
        }
        return m_Driver->GetWidth();
    }
} // namespace EPDL