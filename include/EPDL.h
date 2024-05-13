#ifndef EPDL_H_
#define EPDL_H_

#include <vector>
#include <string_view>
#include <memory>
#include <cstdint>

namespace EPDL
{
    typedef int ImageHandle;
    typedef uint8_t Byte;
    struct ImageData {
        std::string Filename;
        u_int16_t Width;
        u_int16_t Height;
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
} // namespace EPDL


#endif /*EPDL_H_*/
