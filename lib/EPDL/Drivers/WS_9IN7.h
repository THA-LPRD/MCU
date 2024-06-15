#ifndef WS9IN7NEW_H
#define WS9IN7NEW_H

#include "IT8951E.h"
#include "FrameBuffer.h"

namespace EPDL
{
    class WS_9IN7 : public EPDL::IT8951E {
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

        WS_9IN7();
        ~WS_9IN7() = default;
        void DrawImage(ImageHandle handle, int x, int y) override;
        void BeginFrame() override;
        void EndFrame() override;
        void SwapBuffers() override;
    private:
        uint16_t m_Width = 1200;
        uint16_t m_Height = 825;
        FrameBuffer m_FrameBuffer;
    };
} // EPDL

#endif //WS9IN7NEW_H
