#ifndef EPDL_H_
#define EPDL_H_

#include <spdlog/spdlog.h>
#include <memory>
#include <unordered_map>
#include <string>
#include "PNGDecoder.h"
#include "Displays/Driver.h"

class EPDL {
public:
    enum class Display {
        WS_7IN3G, WS_9IN7, GD_7IN5
    };
    EPDL(int busyPin, int resetPin, int dcPin, int csPin, int sckPin, int mosiPin);
    ~EPDL() = default;
    void Start(Display display);
    void Terminate();
    int CreateImage(std::string_view filename);
    void DeleteImage(int handle);
    void DrawImage(int handle, uint16_t x, uint16_t y);
    void BeginFrame();
    void EndFrame();
    void SwapBuffers();
    DriverInfo GetDriverInfo();
private:
    int BusyPin;
    int ResetPin;
    int DCPin;
    int CSPin;
    int SCKPin;
    int MOSIPin;
    Driver* m_Driver = nullptr;
};


#endif /*EPDL_H_*/
