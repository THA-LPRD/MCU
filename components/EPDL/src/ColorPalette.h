#ifndef LPRD_MCU_COLORPALETTE_H
#define LPRD_MCU_COLORPALETTE_H

#include <vector>
#include <unordered_map>
#include <cstdint>
#include "PNGDecoder.h"

struct RGBHash {
    std::size_t operator()(const RGB &rgb) const {
        return (static_cast<size_t>(rgb.r) << 16) |
               (static_cast<size_t>(rgb.g) << 8) |
               static_cast<size_t>(rgb.b);
    }
};

class ColorPalette {
public:
    explicit ColorPalette(const std::vector<RGB> &colors) : palette(colors) {}
    uint8_t GetClosestColor(const RGB &color);
private:
    static uint64_t GetSquaredEuclideanDistance(const RGB &p, const RGB &q);
private:
    std::vector<RGB> palette;
    std::unordered_map<RGB, uint8_t, RGBHash> cache;
};

#endif //LPRD_MCU_COLORPALETTE_H
