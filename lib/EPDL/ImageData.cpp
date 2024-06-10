#include "ImageData.h"
#include <fstream>
#include <RLEData.h>
#include "Log.h"
#include <LittleFS.h>
#include <cstdio>
#include "FrameBuffer.h"
#include <cmath>
#include <cstdint>

namespace EPDL
{
    File myfile;
// Function to extract red, green, and blue components from RGB565
    void extractRGB(uint16_t pixel, uint8_t &red, uint8_t &green, uint8_t &blue) {
        red = (pixel >> 11) & 0x1F;  // 5 bits for red
        green = (pixel >> 5) & 0x3F; // 6 bits for green
        blue = pixel & 0x1F;         // 5 bits for blue

        // Scale them to 8 bits
        red = (red * 255) / 31;
        green = (green * 255) / 63;
        blue = (blue * 255) / 31;
    }

// Function to calculate the squared Euclidean distance between two RGB colors
    uint32_t euclideanDistanceSquared(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2) {
        return (r1 - r2) * (r1 - r2) + (g1 - g2) * (g1 - g2) + (b1 - b2) * (b1 - b2);
    }

    uint8_t mapTo2Bits(uint16_t pixel) {
        // Define RGB values for the 4 colors
        const uint8_t colors[4][3] = {
                {0, 0, 0},       // Black (0b00)
                {255, 0, 0},     // Red (0b11)
                {255, 255, 255}, // White (0b01)
                {255, 255, 0}    // Yellow (0b10)
        };

        // Extract RGB components from the RGB565 pixel
        uint8_t red, green, blue;
        extractRGB(pixel, red, green, blue);

        // Find the closest color
        uint32_t minDistance = UINT32_MAX;
        uint8_t closestColor = 0;

        for (uint8_t i = 0; i < 4; ++i) {
            uint32_t distance = euclideanDistanceSquared(red, green, blue, colors[i][0], colors[i][1], colors[i][2]);
            if (distance < minDistance) {
                minDistance = distance;
                closestColor = i;
            }
        }

        // Return the mapped 2-bit color code
        switch (closestColor) {
            case 0: return 0b00; // Black
            case 1: return 0b11; // Red
            case 2: return 0b01; // White
            case 3: return 0b10; // Yellow
            default: return 0b00; // Fallback to Black
        }
    }
    struct decodeInfo {
        FrameBuffer* frameBuffer;
        size_t offset_x;
        size_t offset_y;
        PNG* pPNG;
    };

    void PNGDraw(PNGDRAW* pDraw) {
        decodeInfo* decodeInfo = static_cast<struct decodeInfo*>(pDraw->pUser);
        FrameBuffer* pFramebuffer = decodeInfo->frameBuffer;
        uint16_t offset_x = decodeInfo->offset_x;
        uint16_t offset_y = decodeInfo->offset_y;
        PNG* pPNG = decodeInfo->pPNG;

        Log::Debug("[ImageData] Drawing PNG");
        uint16_t width = pDraw->iWidth;
        uint16_t usPixels[width];
        pPNG->getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);
        Log::Debug("[ImageData] Get Line: %d", pDraw->y);
        for (int i = 0; i < width; i++) {
            uint8_t color = mapTo2Bits(usPixels[i]);
            pFramebuffer->SetPixel(i + offset_x, pDraw->y + offset_y, mapTo2Bits(usPixels[i]));
        }
    }

    std::string sanitizePath(std::string_view path) {
        std::string ltfspath = path.data();
        size_t pos = ltfspath.rfind('/'); // Find the last '/' in the string
        if (pos != std::string::npos) {
            // Find the second to last '/' starting from the last '/'
            pos = ltfspath.rfind('/', pos - 1);
            if (pos != std::string::npos) {
                ltfspath = ltfspath.substr(pos);
            }
        }
        return ltfspath;
    }

    void *myOpen(const char *filename, int32_t *size) {
        Log::Debug("[EPDL] Attempting to open %s", filename);
        myfile = LittleFS.open(filename);
        *size = myfile.size();
        return &myfile;
    }
    void myClose(void *handle) {
        if (myfile) myfile.close();
    }
    int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
        if (!myfile) {
            Log::Error("[ImageData] File not open");
            return 0;
        }
        return myfile.read(buffer, length);
    }
    int32_t mySeek(PNGFILE *handle, int32_t position) {
        if (!myfile) {
            Log::Error("[ImageData] File not open");
            return 0;
        }
        return myfile.seek(position);
    }


    ImageData::ImageData(std::string_view filePath)
            : m_FilePath(filePath) {
        Log::Debug("[ImageData] Loading image %s", filePath.data());
        m_PNG.open(filePath.data(), myOpen, myClose, myRead, mySeek, PNGDraw);
        m_Width = m_PNG.getWidth();
        m_Height = m_PNG.getHeight();
        Log::Debug("[ImageData] Image size: %dx%d", GetWidth(), GetHeight());
    }


    void ImageData::DrawImage(EPDL::FrameBuffer* frameBuffer, size_t offset_x, size_t offset_y) {
        struct decodeInfo decodeInfo{frameBuffer, offset_x, offset_y, &m_PNG};
        m_PNG.decode(&decodeInfo, 0);
    }

} // namespace EPDL