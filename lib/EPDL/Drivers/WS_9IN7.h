#ifndef WS_9IN7_H_
#define WS_9IN7_H_

#include "Driver.h"
#include "FrameBuffer.h"

namespace EPDL
{
    class WS_9IN7 : public EPDL::Driver {
    public:
        enum Color : uint8_t {
            Black = 0x0,
            White = 0x1,
            Yellow = 0x2,
            Red = 0x3
        };
        WS_9IN7();
        ~WS_9IN7();
        void DrawImage(ImageHandle handle, int x, int y) override;
        void BeginFrame() override;
        void EndFrame() override;
        void SwapBuffers() override;
        uint16_t GetWidth() const override { return m_Width; }
        uint16_t GetHeight() const override { return m_Height; }
    private:
        typedef struct
        {
            uint16_t usPanelW;
            uint16_t usPanelH;
            uint16_t usImgBufAddrL;
            uint16_t usImgBufAddrH;
            uint16_t usFWVersion[8];   //16 Bytes String
            uint16_t usLUTVersion[8];  //16 Bytes String
        } IT8951DevInfo;
        void SendCommand(uint8_t command);
        void SendData(uint8_t data);
        uint8_t ReadData(uint8_t data);
        void WaitUntilReady();
        void Reset();
        void PowerOff();
        void PowerOn();
        void Sleep();
        void StartDataTransmission();
        void Refresh();
        uint16_t m_Width = 1200;
        uint16_t m_Height = 825;
        uint8_t m_PixelSize = 4;
        FrameBuffer m_FrameBuffer;
    };
} // namespace EPDL

#endif //WS_9IN7_H_
