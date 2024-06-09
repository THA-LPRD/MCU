#ifndef WS_7IN3G_H_
#define WS_7IN3G_H_

#include "Driver.h"
#include "FrameBuffer.h"

namespace EPDL
{
    class WS_7IN3G : public EPDL::Driver {
    public:
        enum Color : uint8_t {
            Black = 0x0,
            White = 0x1,
            Yellow = 0x2,
            Red = 0x3
        };
        WS_7IN3G();
        ~WS_7IN3G();
        void DrawImage(ImageHandle handle, int x, int y) override;
        void BeginFrame() override;
        void EndFrame() override;
        void SwapBuffers() override;
        uint16_t GetWidth() const override { return m_Width; }
        uint16_t GetHeight() const override { return m_Height; }
    private:
        void SendCommand(uint8_t command);
        void SendData(uint8_t data);
        void WaitUntilReady();
        void Reset();
        void PowerOff();
        void PowerOn();
        void Sleep();
        void StartDataTransmission();
        void Refresh();
    private:
        uint16_t m_Width = 800;
        uint16_t m_Height = 480;
        uint8_t m_PixelSize = 2;
        FrameBuffer m_FrameBuffer;
    };
} // namespace EPDL

#endif /*WS_7IN3G_H_*/
