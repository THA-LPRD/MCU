#include "ImageData.h"
#include <fstream>
#include <RLEData.h>
#include "Log.h"
#include <LittleFS.h>
#include <cstdio>
#include "FrameBuffer.h"
#include <cmath>
#include <cstdint>
#include "png.inl"


namespace EPDL
{
    uint8_t ColorPalette::GetClosestColor(uint16_t pixel) {
        if (cache.find(pixel) != cache.end()) {
            return cache[pixel];
        }

        RGBPixel p = ExtractRGB(pixel);

        uint64_t minDistance = UINT64_MAX;
        uint8_t closestColorIndex = 0;

        for (size_t i = 0; i < palette.size(); ++i) {
            uint64_t distance = GetSquaredEuclideanDistance(p, palette[i]);
            if (distance < minDistance) {
                minDistance = distance;
                closestColorIndex = static_cast<uint8_t>(i);
            }
        }

        cache[pixel] = closestColorIndex;
        return closestColorIndex;
    }

    uint64_t ColorPalette::GetSquaredEuclideanDistance(const RGBPixel& p, const RGBPixel& q) {
        return (p.r - q.r) * (p.r - q.r) +
            (p.g - q.g) * (p.g - q.g) +
            (p.b - q.b) * (p.b - q.b);
    }

    RGBPixel ColorPalette::ExtractRGB(uint16_t pixel) {
        uint8_t r = (pixel >> 11) & 0x1F; // 5 bits for red
        uint8_t g = (pixel >> 5) & 0x3F;  // 6 bits for green
        uint8_t b = pixel & 0x1F;         // 5 bits for blue

        // Scale them to 8 bits
        r = (r * 255) / 31;
        g = (g * 255) / 63;
        b = (b * 255) / 31;

        return { r, g, b };
    }

    ImageData::ImageData(std::string_view filePath) :
        m_FilePath(filePath),
        m_PNG{
                .iWidth = 0,
                .iHeight = 0,
                .ucBpp = 0,
                .ucPixelType = 0,
                .ucMemType = 0,
                .pImage = nullptr,
                .iPitch = 0,
                .iHasAlpha = 0,
                .iInterlaced = 0,
                .iTransparent = 0,
                .iError = 0,
                .pfnRead = Read,
                .pfnSeek = Seek,
                .pfnOpen = Open,
                .pfnDraw = Draw,
                .pfnClose = Close,
                .PNGFile = {0, 0, nullptr, nullptr},
                .ucZLIB = {0},
                .ucPalette = {0},
                .ucPixels = {0},
                .ucFileBuf = {0} } {
        Log::Trace("[ImageData] Creating ImageData object for file %s", filePath.data());

        m_PNG.PNGFile.fHandle = (*Open)(filePath.data(), &m_PNG.PNGFile.iSize);
        if (m_PNG.PNGFile.fHandle == NULL) {
            Log::Error("[ImageData] Could not open file %s", filePath.data());
            return;
        }

        m_PNG.iError = PNGParseInfo(&m_PNG);
        if (m_PNG.iError != PNG_SUCCESS) {
            LogError();
            return;
        }
        Log::Debug("[ImageData] Successfully created ImageData object for file %s", filePath.data());
    }

    ImageData::~ImageData() {
        Log::Trace("[ImageData] Destroying ImageData object");
        Close(m_PNG.PNGFile.fHandle);
    }

    void ImageData::LogError() {
        switch (m_PNG.iError) {
        case PNG_INVALID_PARAMETER:
            Log::Error("[ImageData] Invalid parameter");
            break;
        case PNG_DECODE_ERROR:
            Log::Error("[ImageData] Decode error");
            break;
        case PNG_MEM_ERROR:
            Log::Error("[ImageData] Memory error");
            break;
        case PNG_NO_BUFFER:
            Log::Error("[ImageData] No buffer");
            break;
        case PNG_UNSUPPORTED_FEATURE:
            Log::Error("[ImageData] Unsupported feature");
            break;
        case PNG_INVALID_FILE:
            Log::Error("[ImageData] Invalid file");
            break;
        case PNG_TOO_BIG:
            Log::Error("[ImageData] Image too big");
            break;
        default:
            Log::Error("[ImageData] Unknown error");
            break;
        }
    }

    void ImageData::DrawImage(DecodeInfo& decodeInfo) {
        if (m_PNG.iError != 0) return;
        m_PNG.iError = DecodePNG(&m_PNG, static_cast<void*>(&decodeInfo), 0);
        if (m_PNG.iError != PNG_SUCCESS) {
            LogError();
        }
    }

    void ImageData::Draw(PNGDRAW* pDraw) {
        DecodeInfo* decodeInfo = static_cast<struct DecodeInfo*>(pDraw->pUser);
        FrameBuffer* pFramebuffer = decodeInfo->frameBuffer;
        ColorPalette* pColorPalette = decodeInfo->colorPalette;
        uint16_t offset_x = decodeInfo->offset_x;
        uint16_t offset_y = decodeInfo->offset_y;

        uint16_t width = pDraw->iWidth;
        uint16_t usPixels[width];
        PNGRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff, pDraw->iHasAlpha);
        Log::Debug("[ImageData] Get Line: %d", pDraw->y);
        for (int i = 0; i < width; i++) {
            uint8_t color = pColorPalette->GetClosestColor(usPixels[i]);
            pFramebuffer->SetPixel(i + offset_x, pDraw->y + offset_y, color);
        }
    }

    void* ImageData::Open(const char* filename, int32_t* size) {
        std::ifstream* file = new std::ifstream(filename, std::ios::in | std::ios::binary);
        if (!file->is_open()) {
            Log::Error("[ImageData] Could not open file %s", filename);
            return nullptr;
        }
        file->seekg(0, std::ios::end);
        *size = file->tellg();
        file->seekg(0, std::ios::beg);
        return file;
    }

    void ImageData::Close(void* handle) {
        std::ifstream* file = static_cast<std::ifstream*>(handle);
        if (file && file->is_open()) file->close();
    }

    int32_t ImageData::Read(PNGFILE* handle, uint8_t* buffer, int32_t length) {
        std::ifstream* file = static_cast<std::ifstream*>(handle->fHandle);
        if (!file->is_open()) {
            Log::Error("[ImageData] File not open");
            return 0;
        }
        file->read(reinterpret_cast<char*>(buffer), length);
        return file->gcount();
    }

    int32_t ImageData::Seek(PNGFILE* handle, int32_t position) {
        std::ifstream* file = static_cast<std::ifstream*>(handle->fHandle);
        if (!file->is_open()) {
            Log::Error("[ImageData] File not open");
            return 1;
        }
        file->seekg(position);
        return 0;
    }
} // namespace EPDL