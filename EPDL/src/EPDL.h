#ifndef EPDL_H_
#define EPDL_H_

#include <vector>
#include <string_view>
#include <memory>
#include <cstdint>
#include "ImageData.h"
#include "SPI.h"

namespace EPDL
{
    typedef int ImageHandle;

    enum SPI {
        SPIDevice = MCU::SPIDevice::SPI1,
    };

    enum Pin : uint8_t {
        BUSY = 1,
        RST = 44,
        DC = 3,
        CS = 4,
        SCK = 7,
        MOSI = 9
    };

    int Init();
    void Terminate();
    void LoadDriver(std::string_view type);
    ImageHandle CreateImage(std::unique_ptr<ImageData> data);
    void DeleteImage(ImageHandle handle);
    void DrawImage(ImageHandle handle, uint16_t x, uint16_t y);
    void BeginFrame();
    void EndFrame();
    void SwapBuffers();
    uint16_t GetWidth();
    uint16_t GetHeight();
} // namespace EPDL


#endif /*EPDL_H_*/
