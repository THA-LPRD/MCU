#include "EPDL.h"
#include "Displays/WS_7IN3G.h"
#include "Displays/WS_9IN7.h"
#include <spdlog/sinks/stdout_color_sinks.h>

EPDL::EPDL(int busyPin, int resetPin, int dcPin, int csPin, int sckPin, int mosiPin) :
        BusyPin(busyPin),
        ResetPin(resetPin),
        DCPin(dcPin),
        CSPin(csPin),
        SCKPin(sckPin),
        MOSIPin(mosiPin) {}

void EPDL::Start(Display display) {
    switch (display) {
        case Display::WS_7IN3G:
            spdlog::info("Starting 7.3in display");
            m_Driver = new WS_7IN3G(
                    BusyPin,
                    ResetPin,
                    DCPin,
                    CSPin,
                    SCKPin,
                    MOSIPin
            );
            break;
        case Display::WS_9IN7:
            spdlog::info("Starting 9.7in display");
            m_Driver = new WS_9IN7(
                    BusyPin,
                    ResetPin,
                    DCPin,
                    CSPin,
                    SCKPin,
                    MOSIPin
            );
            break;
        case Display::GD_7IN5:
            spdlog::info("Starting 7.5in display");
            break;
    }
}


void EPDL::Terminate() {
    // Pointer löschen / Deinit
    // Vcc ausschalten
}

int EPDL::CreateImage(std::string_view filename) {
    return m_Driver->CreateImage(filename);
}

void EPDL::DeleteImage(int handle) {
    m_Driver->DeleteImage(handle);
}

void EPDL::DrawImage(int handle, uint16_t x, uint16_t y) {
    m_Driver->DrawImage(handle, x, y);
}

void EPDL::BeginFrame() {
    m_Driver->BeginFrame();
}

void EPDL::EndFrame() {
    m_Driver->EndFrame();
}

void EPDL::SwapBuffers() {
    m_Driver->SwapBuffers();
}

DriverInfo EPDL::GetDriverInfo() {
    return m_Driver->GetDriverInfo();
}