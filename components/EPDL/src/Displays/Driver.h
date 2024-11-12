#ifndef LPRD_MCU_DRIVER_H
#define LPRD_MCU_DRIVER_H

#include <spdlog/spdlog.h>
#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>
#include <unordered_map>
#include <memory>
#include <Drivers/SPI.h>
#include <Drivers/GPIO.h>
#include "../PNGDecoder.h"

typedef struct {
    uint16_t  Width;            // Display width in pixels
    uint16_t Height;            // Display height in pixels
    uint8_t ColorDepth;        // Number of supported color bits
    bool PartialRefresh;       // Partial refresh support flag
    std::string DriverName;    // Display driver name/identifier
} DriverInfo;

class Driver {
public:
    Driver(int busyPin, int resetPin, int dcPin, int csPin, int sckPin, int mosiPin) :
            m_BusyPin(busyPin),
            m_ResetPin(resetPin),
            m_DCPin(dcPin),
            m_CSPin(csPin),
            m_SCKPin(sckPin),
            m_MOSIPin(mosiPin) {
        GPIO::SetMode(busyPin, GPIO::Mode::Input);
        GPIO::SetMode(resetPin, GPIO::Mode::Output);
        GPIO::SetMode(csPin, GPIO::Mode::Output);

        // Set SPI Devices inactive
        GPIO::Write(csPin, 1);
    }
    virtual ~Driver() = default;
    virtual void Initialize() = 0;
    virtual int CreateImage(std::string_view filename) {
        int handle = index++;
        PNGs[handle] = std::make_unique<std::string>(filename);
        return handle;
    }

    inline virtual void DeleteImage(int handle) { PNGs.erase(handle); }
    virtual void DrawImage(int handle, int x_offset, int y_offset) = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void SwapBuffers() = 0;
    inline DriverInfo GetDriverInfo() { return m_DriverInfo; }
protected:
    static constexpr const char* LOG_TAG = "[EPDL] -";
    int m_BusyPin;
    int m_ResetPin;
    int m_DCPin;
    int m_CSPin;
    int m_SCKPin;
    int m_MOSIPin;
    SPI m_SPIController;
    DriverInfo m_DriverInfo = {};
    std::unordered_map<int, std::unique_ptr<std::string>> PNGs;
    int index = 0;
    PNGDecoder m_PNGDecoder;
};

#endif //LPRD_MCU_DRIVER_H
