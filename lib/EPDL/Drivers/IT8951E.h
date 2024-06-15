#ifndef MCU_IT8951E_H
#define MCU_IT8951E_H

#include "Driver.h"

namespace EPDL
{
    class IT8951E : public EPDL::Driver {
    public:
        IT8951E();
        ~IT8951E();
        void DrawImage(ImageHandle handle, int x, int y) override = 0;
        void BeginFrame() override = 0;
        void EndFrame() override = 0;
        void SwapBuffers() override = 0;
        uint16_t GetWidth() const override { return m_DeviceInfo.PanelWidth; }
        uint16_t GetHeight() const override { return m_DeviceInfo.PanelHeight; }
    protected:
        void SendData(uint16_t* data = nullptr, uint16_t length = 1);
        void StartTransmission();
        void EndTransmission();
        void WaitUntilDisplayReady();
    protected:
        struct DeviceInfo {
            uint16_t PanelWidth;
            uint16_t PanelHeight;
            uint16_t ImgBufAddrL;
            uint16_t ImgBufAddrH;
            uint16_t FWVersion[8];   //16 Bytes String
            uint16_t LUTVersion[8];  //16 Bytes String
        } m_DeviceInfo;
    private:
        void SendCommand(uint16_t cmd, uint16_t* args = nullptr, uint16_t argCount = 0);
        std::vector<uint16_t> Read(size_t length = 1);
        void WaitUntilReady();
        void EnableI80PackedMode();
        void Reset();
        DeviceInfo GetDeviceInfo();
    private:
        enum Endian {
            Little = 0,
            Big = 1,
        };
        enum BitsPerPixel {
            BPP2 = 0,
            BPP3 = 1,
            BPP4 = 2,
            BPP8 = 3,
        };
        enum Rotate {
            Deg0 = 0,
            Deg90 = 1,
            Deg180 = 2,
            Deg270 = 3,
        };
        struct LoadImageInfo {
            uint16_t Endianness = Endian::Little;
            uint16_t BitsPerPixel = BitsPerPixel::BPP4;
            uint16_t Rotate = Rotate::Deg0;
        } m_LoadImageInfo;
    };
} // namespace EPDL

#endif //MCU_IT8951E_H
