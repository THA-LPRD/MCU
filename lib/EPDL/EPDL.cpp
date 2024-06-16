#include <memory>
#include "EPDL.h"
#include "Drivers/Driver.h"
#include "Drivers/WS_7IN3G.h"
#include "Drivers/WS_9IN7.h"
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
        MCU::GPIO::SetMode(EPDL::Pin::BUSY, MCU::GPIO::Mode::Input); // Also HRDY Pin
        MCU::GPIO::SetMode(EPDL::Pin::RST, MCU::GPIO::Mode::Output);

        MCU::GPIO::SetMode(EPDL::Pin::SCK, MCU::GPIO::Mode::Output);
        MCU::GPIO::SetMode(EPDL::Pin::MOSI, MCU::GPIO::Mode::Output);
        MCU::GPIO::SetMode(EPDL::Pin::CS, MCU::GPIO::Mode::Output);

        // Set SPI Devices inactive
        MCU::GPIO::Write(EPDL::Pin::CS, 1);
        MCU::GPIO::Write(EPDL::Pin::SCK, 0);
        return 0;
    }

    void Terminate() {
        // Pointer löschen / Deinit
        // Vcc ausschalten
        return;
    }

    void LoadDriver(std::string_view type) {
        if (type == "WS_7IN3G") {
            MCU::GPIO::SetMode(EPDL::Pin::DC, MCU::GPIO::Mode::Output);
            m_Driver = std::make_unique<WS_7IN3G>();
        }
        else if (type == "WS_9IN7") {
            MCU::GPIO::SetMode(EPDL::Pin::DC, MCU::GPIO::Mode::Input);
            m_Driver = std::make_unique<WS_9IN7>();
        } else {
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

    uint16_t GetHeight() {
        if (m_Driver == nullptr) {
            return 0;
        }
        return m_Driver->GetHeight();
    }
} // namespace EPDL