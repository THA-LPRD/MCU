#ifndef MCU_IMAGEDATA_H
#define MCU_IMAGEDATA_H

#include <vector>
#include <string>
#include <string_view>
#include <cstdint>
#include <PNGdec.h>
#include "FrameBuffer.h"

namespace EPDL
{
    class ImageData {
    public:
        ImageData(std::string_view filePath);
        ~ImageData() = default;
        inline int GetWidth() const { return m_Width; }
        inline int GetHeight() const { return m_Height; }
        void DrawImage(EPDL::FrameBuffer* frameBuffer, size_t offset_x, size_t offset_y);
    private:
        std::string m_FilePath;
        int m_Width;
        int m_Height;
        PNG m_PNG;
    };
} // namespace EPDL
#endif //MCU_IMAGEDATA_H
