#ifndef RLEDATA_H
#define RLEDATA_H

#include <vector>
#include <string_view>
#include <cstdint>

namespace RLE
{
    void Encode(std::string_view filepath, std::vector<uint8_t> data);
    std::vector<std::vector<uint8_t>>
    Decode(std::string_view filepath, size_t width, size_t start_height, size_t end_height);
}

#endif //RLEDATA_H
