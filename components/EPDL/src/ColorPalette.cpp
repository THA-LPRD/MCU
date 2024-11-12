#include "ColorPalette.h"

uint8_t ColorPalette::GetClosestColor(const RGB &color) {
    if (cache.find(color) != cache.end()) {
        return cache[color];
    }

    uint64_t minDistance = UINT64_MAX;
    uint8_t closestColorIndex = 0;

    for (size_t i = 0; i < palette.size(); ++i) {
        uint64_t distance = GetSquaredEuclideanDistance(color, palette[i]);
        if (distance < minDistance) {
            minDistance = distance;
            closestColorIndex = static_cast<uint8_t>(i);
        }
    }

    cache[color] = closestColorIndex;
    return closestColorIndex;
}

uint64_t ColorPalette::GetSquaredEuclideanDistance(const RGB &p, const RGB &q) {
    return (p.r - q.r) * (p.r - q.r) +
           (p.g - q.g) * (p.g - q.g) +
           (p.b - q.b) * (p.b - q.b);
}
