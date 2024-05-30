#include <unity.h>
#include <Framebuffer.h>
#include "ImageData.h"

EPDL::FrameBuffer* fb;
uint16_t width = 800;
uint16_t height = 480;
uint8_t pixelSize = 2;
uint8_t backgroundColor = 0x1;

void setUp(void) {
    fb = new EPDL::FrameBuffer(width, height, pixelSize, backgroundColor);
}

void tearDown(void) {
    delete fb;
}

void test_FrameBufferOutOfBounds(void) {
    // Test out of bounds pixel setting
    TEST_ASSERT_FALSE(fb->SetPixel(-1, 0, 0));
    TEST_ASSERT_FALSE(fb->SetPixel(0, -1, 0));
    TEST_ASSERT_FALSE(fb->SetPixel(width, 0, 0));
    TEST_ASSERT_FALSE(fb->SetPixel(0, height, 0));
    // Test out of bounds pixel getting
    TEST_ASSERT_EQUAL_UINT8(backgroundColor, fb->GetPixel(-1, 0));
    TEST_ASSERT_EQUAL_UINT8(backgroundColor, fb->GetPixel(0, -1));
    TEST_ASSERT_EQUAL_UINT8(backgroundColor, fb->GetPixel(width, 0));
    TEST_ASSERT_EQUAL_UINT8(backgroundColor, fb->GetPixel(0, height));
}

void test_FrameBufferSetPixels(void) {
    // Set pixels in framebuffer based on image data
    for (uint16_t y = 0; y < height; ++y) {
        for (uint16_t x = 0; x < width; x += 4) {
            uint8_t byte = gImage_7in3g[(y * width + x) / 4];

            // Extract each 2-bit pixel and set it in the framebuffer
            for (int i = 0; i < 4; ++i) {
                uint8_t pixel = (byte >> (6 - 2 * i)) & 0x03; // Get 2 bits
                fb->SetPixel(x + i, y, pixel);
            }
        }
    }
    std::vector<std::vector<uint8_t>> data = fb->GetData();
    for (uint16_t y = 0; y < height; ++y) {
        for (uint16_t x = 0; x < width; x += 4) {
            uint8_t byte = gImage_7in3g[(y * width + x) / 4];
            // Check if the reconstructed byte matches the original byte
            TEST_ASSERT_EQUAL_UINT8(byte, data[y][x / 4]);
        }
    }
}

// This test assumes that the SetPixel method is working correctly
void test_FrameBufferGetPixels(void){
    // Set pixels in framebuffer based on image data
    for (uint16_t y = 0; y < height; ++y) {
        for (uint16_t x = 0; x < width; x += 4) {
            uint8_t byte = gImage_7in3g[(y * width + x) / 4];

            // Extract each 2-bit pixel and set it in the framebuffer
            for (int i = 0; i < 4; ++i) {
                uint8_t pixel = (byte >> (6 - 2 * i)) & 0x03; // Get 2 bits
                fb->SetPixel(x + i, y, pixel);
            }
        }
    }
    // Verify the framebuffer data matches the image data
    for (uint16_t y = 0; y < height; ++y) {
        for (uint16_t x = 0; x < width; x += 4) {
            uint8_t byte = gImage_7in3g[(y * width + x) / 4];
            uint8_t reconstructedByte = 0;

            // Reconstruct byte from framebuffer
            for (int i = 0; i < 4; ++i) {
                uint8_t pixel = fb->GetPixel(x + i, y);
                reconstructedByte |= (pixel << (6 - 2 * i));
            }

            // Check if the reconstructed byte matches the original byte
            TEST_ASSERT_EQUAL_UINT8(byte, reconstructedByte);
        }
    }
}