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
    struct RGBPixel {
        uint8_t r, g, b;
    };

    class ColorPalette {
    public:
        ColorPalette(const std::vector<RGBPixel> &colors) : palette(colors) {}
        uint8_t GetClosestColor(uint16_t pixel);
    private:
        static RGBPixel ExtractRGB(uint16_t pixel);
        static uint64_t GetSquaredEuclideanDistance(const RGBPixel &p, const RGBPixel &q);
    private:
        std::vector<RGBPixel> palette;
        std::unordered_map<uint16_t, uint8_t> cache;
    };

    class ImageData {
    public:
        struct DecodeInfo {
            FrameBuffer* frameBuffer;
            ColorPalette* colorPalette;
            size_t offset_x;
            size_t offset_y;
        };
        ImageData(std::string_view filePath);
        ~ImageData();
        inline int GetWidth() const { return m_PNG.iWidth; }
        inline int GetHeight() const { return m_PNG.iHeight; }
        void DrawImage(DecodeInfo& decodeInfo);
    private:
        void LogError();
        static void Draw(PNGDRAW* pDraw);
        static void* Open(const char* filename, int32_t *size);
        static void Close(void* handle);
        static int32_t Read(PNGFILE* handle, uint8_t* buffer, int32_t length);
        static int32_t Seek(PNGFILE* handle, int32_t position);
    private:
        std::string m_FilePath;
        int m_Width;
        int m_Height;
        PNGIMAGE m_PNG;
    };
} // namespace EPDL
#endif //MCU_IMAGEDATA_H
