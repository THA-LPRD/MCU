#include "ImageData.h"
#include <fstream>
#include <RLEData.h>
#include "Log.h"

namespace EPDL
{
    ImageData::ImageData(std::string_view filePath, uint16_t width, uint16_t height, size_t bufferSize)
            : m_FilePath(filePath), m_Width(width), m_Height(height), m_BufferSize(bufferSize), m_BufferOffset(-1) {
        if (m_BufferSize < 1) m_BufferSize = 1;
        m_Buffer.resize(m_BufferSize);
    }

    uint8_t ImageData::GetPixel(uint16_t x, uint16_t y) {
        if (x >= m_Width || y >= m_Height) {
            Log::Error("[ImageData] Pixel coordinates are out of range %d %d", x, y);
            return 0;
        }

        if (m_BufferOffset == -1 || y < m_BufferOffset || y >= m_BufferOffset + m_Buffer.size()) {
            // Buffer does not contain the required line, load new data
            size_t startLine = y;
            size_t endLine = std::min(startLine + m_BufferSize - 1, static_cast<size_t>(m_Height));

            LoadData(startLine, endLine);
        }
        if (y-m_BufferOffset >= m_Buffer.size() || x >= m_Buffer[0].size()) {
            Log::Error("[ImageData] Pixel coordinates are out of range %d %d", x, y);
            Log::Debug("[ImageData] Buffer size: %d %d", m_Buffer.size(), m_Buffer[0].size());
            Log::Debug("[ImageData] Buffer offset: %d", m_BufferOffset);
            return 0;
        }
        return m_Buffer[y - m_BufferOffset][x];
    }

    void ImageData::LoadData(size_t startLine, size_t endLine) {
        if (endLine - startLine + 1 > m_BufferSize) {
            Log::Error("[ImageData] Requested lines exceed buffer size");
            m_BufferOffset = -1;
            return;
        }

        m_Buffer = RLE::Decode(m_FilePath, m_Width, startLine, endLine);
        m_BufferOffset = startLine;
    }
} // namespace EPDL