#ifndef LPRD_MCU_GD_7IN5_H
#define LPRD_MCU_GD_7IN5_H

#include "Driver.h"
#include "../FrameBuffer.h"
#include "../ColorPalette.h"
#include "../PNGDecoder.h"

class GD_7IN5 : public Driver {
public:
    enum Color : uint8_t {
        Black = 0x0,
        White = 0x1,
        Red = 0x2
    };
    GD_7IN5(int busyPin, int resetPin, int dcPin, int csPin, int sckPin, int mosiPin);
    ~GD_7IN5() override = default;
    void Initialize() override;
    void DrawImage(int handle, int x_offset, int y_offset) override;
    void BeginFrame() override;
    void EndFrame() override;
    void SwapBuffers() override;
private:
    void SendCommand(uint8_t command);
    void SendData(uint8_t data);
    void WaitUntilReady();
    void Reset();
    void PowerOff();
    void PowerOn();
    void Sleep();
    void StartDataTransmissionBW();
    void StartDataTransmissionRW();
    void Refresh();
private:
    static constexpr const uint16_t WIDTH = 800;
    static constexpr const uint16_t HEIGHT = 480;
    static constexpr const uint8_t COLOR_DEPTH = 2;
    FrameBuffer m_FrameBuffer;
    ColorPalette m_ColorPalette;
};

#endif //LPRD_MCU_GD_7IN5_H
