#ifndef LPRD_MCU_IT8951E_H
#define LPRD_MCU_IT8951E_H

#include "Driver.h"

class IT8951E : public Driver {
public:
    IT8951E(int busyPin, int resetPin, int dcPin, int csPin, int sckPin, int mosiPin);
    ~IT8951E() override = default;
    void Initialize() override;
    void DrawImage(int handle, int x_offset, int y_offset) override = 0;
    void BeginFrame() override = 0;
    void EndFrame() override = 0;
    void SwapBuffers() override = 0;
protected:
    void SendData(const uint16_t* data = nullptr, uint16_t length = 1);
    void StartTransmission();
    void EndTransmission();
    void WaitUntilDisplayReady();
    void Sleep();
protected:
    struct DeviceInfo {
        uint16_t PanelWidth;
        uint16_t PanelHeight;
        uint16_t ImgBufAddrL;
        uint16_t ImgBufAddrH;
        uint16_t FWVersion[8];   //16 Bytes String
        uint16_t LUTVersion[8];  //16 Bytes String
    } m_DeviceInfo = {};
    static constexpr uint8_t COLOR_DEPTH = 4;
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


#endif //LPRD_MCU_IT8951E_H
