#include "RLEData.h"
#include <fstream>
#include <Log.h>

namespace RLE
{
    namespace
    {
        std::vector<uint8_t> VLQEncode(uint64_t count) {
            std::vector<uint8_t> encodedCount;
            do {
                uint8_t byte = count & 0x7F;
                count >>= 7;
                if (count != 0) {
                    byte |= 0x80;
                }
                encodedCount.push_back(byte);
            } while (count != 0);
            return encodedCount;
        }

        uint64_t VLQDecode(std::vector<uint8_t> &data) {
            uint64_t count = 0;
            for (size_t i = 0; i < data.size(); ++i) {
                count = (count << 7) | (data[data.size() - 1 - i] & 0x7F);
            }
            return count;
        }

        void WriteBytes(const std::string_view file_path, const std::vector<uint8_t> &data) {
            std::ofstream file(file_path.data(), std::ios::binary);
            if (!file) {
                Log::Error("Could not open file for writing.");
                return;
            }

            file.write(reinterpret_cast<const char*>(data.data()), data.size());
        }

        std::vector<uint8_t> read_bytes(const std::string_view file_path) {
            std::ifstream file(file_path.data(), std::ios::binary);
            if (!file) {
                Log::Error("Could not open file for reading.");
                return {};
            }

            std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return data;
        }
    } // namespace

    void Encode(std::string_view filepath, const std::vector<uint8_t> &data) {
        if (data.empty()) {
            return;
        }

        std::vector<uint8_t> encodedData;
        uint64_t count = 1;
        uint8_t prevByte = data[0];

        for (size_t i = 1; i < data.size(); ++i) {
            uint8_t byte = data[i];
            if (byte == prevByte && count < 0x7FFFFFFFFFFFFFFF) {
                ++count;
            }
            else {
                std::vector<uint8_t> encoded_count = VLQEncode(count);
                encodedData.insert(encodedData.end(), encoded_count.begin(), encoded_count.end());
                encodedData.push_back(prevByte);
                prevByte = byte;
                count = 1;
            }
        }

        std::vector<uint8_t> encodedCount = VLQEncode(count);
        encodedData.insert(encodedData.end(), encodedCount.begin(), encodedCount.end());
        encodedData.push_back(prevByte);
        WriteBytes(std::string(filepath), encodedData);
    }

    std::vector<std::vector<uint8_t>> Decode(std::string_view filepath, size_t width, size_t yStart, size_t yEnd) {
        yEnd++;
        if (width == 0 || yStart > yEnd) {
            return {};
        }

        std::ifstream file(filepath.data(), std::ios::binary);
        if (!file.is_open()) return {};

        // Initialize variables for decoding
        uint64_t count = 0;
        std::vector<std::vector<uint8_t>> data(yEnd - yStart, std::vector<uint8_t>(width));
        uint8_t currentValue = 0;
        std::vector<uint8_t> countBytes;
        size_t indexStart = yStart * width;
        size_t indexEnd = yEnd * width;
        size_t index = 0;

        // Decode the file data
        while (!file.eof()) {
            uint8_t byte;
            file.read(reinterpret_cast<char*>(&byte), sizeof(byte));
            if (file.gcount() == 0) {
                break;
            }

            countBytes.push_back(byte);
            if (!(byte & 0x80)) { // End of count sequence
                count = VLQDecode(countBytes);

                file.read(reinterpret_cast<char*>(&currentValue), sizeof(currentValue));
                if (file.gcount() == 0) {
                    break;
                }

                // Fill the vector with the decoded data
                while (count > 0) {
                    if (index < indexStart) {
                        uint64_t skip = std::min(count, static_cast<uint64_t>(indexStart - index));
                        index += skip;
                        count -= skip;
                    }
                    else {
                        size_t internalIndex = index - indexStart;
                        data[internalIndex / width][internalIndex % width] = currentValue;
                        count--;
                        index++;
                    }
                    if (index >= indexEnd) return data;
                }
                countBytes.clear();
            }
        }

        return data;
    }
} // namespace RLE
