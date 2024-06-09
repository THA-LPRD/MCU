#ifndef MCU_IMAGEDATA_H
#define MCU_IMAGEDATA_H

#include <vector>
#include <string>
#include <string_view>
#include <cstdint>

namespace EPDL
{
    class ImageData {
    public:
        ImageData(std::string_view filePath, uint16_t width, uint16_t height, size_t bufferSize);
        ~ImageData() = default;
        inline uint16_t GetWidth() const { return m_Width; }
        inline uint16_t GetHeight() const { return m_Height; }
        uint8_t GetPixel(uint16_t x, uint16_t y);
        void LoadData(size_t startLine, size_t endLine);
        inline void ResetBuffer() { m_BufferOffset = -1; }
    private:
        std::string m_FilePath;
        uint16_t m_Width;
        uint16_t m_Height;
        size_t m_BufferSize;
        int64_t m_BufferOffset;
        std::vector<std::vector<uint8_t>> m_Buffer;
    };
} // namespace EPDL
#endif //MCU_IMAGEDATA_H
