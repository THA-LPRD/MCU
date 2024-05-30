#ifndef EPDL_H_
#define EPDL_H_

#include <vector>
#include <string_view>
#include <memory>
#include <cstdint>
#include "ImageData.h"

namespace EPDL
{
    typedef int ImageHandle;

    enum Pin : uint8_t {
        BUSY = 27,
        RST = 26,
        DC = 25,
        CS = 15,
        SCK = 14,
        MOSI = 13
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
