#ifndef LPRD_MCU_WS9IN7_H
#define LPRD_MCU_WS9IN7_H

#include "IT8951E.h"
#include "../FrameBuffer.h"
#include "../ColorPalette.h"
#include "../PNGDecoder.h"

class WS_9IN7 : public IT8951E {
public:
    enum Color : uint8_t {
        Black = 0x0,
        DarkGray1 = 0x1,
        DarkGray2 = 0x2,
        DarkGray3 = 0x3,
        Gray1 = 0x4,
        Gray2 = 0x5,
        Gray3 = 0x6,
        Gray4 = 0x7,
        LightGray1 = 0x8,
        LightGray2 = 0x9,
        LightGray3 = 0xA,
        LightGray4 = 0xB,
        LightGray5 = 0xC,
        LightGray6 = 0xD,
        LightGray7 = 0xE,
        White = 0xF
    };

    WS_9IN7(int busyPin, int resetPin, int dcPin, int csPin, int sckPin, int mosiPin);
    ~WS_9IN7() override = default;
    void Initialize() override;
    void DrawImage(int handle, int x_offset, int y_offset) override;
    void BeginFrame() override;
    void EndFrame() override;
    void SwapBuffers() override;
private:
    static constexpr const uint16_t WIDTH = 1200;
    static constexpr const uint16_t HEIGHT = 825;
    FrameBuffer m_FrameBuffer;
    ColorPalette m_ColorPalette;
};

#endif //LPRD_MCU_WS9IN7_H
