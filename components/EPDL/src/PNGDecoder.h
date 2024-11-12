#ifndef LPRD_MCU_PNGDECODER_H
#define LPRD_MCU_PNGDECODER_H

#include <vector>
#include <string>
#include <string_view>
#include <cstdint>
#include <functional>
#include <spng.h>

struct RGB {
    uint8_t r, g, b;
    bool operator==(const RGB &other) const {
        return r == other.r && g == other.g && b == other.b;
    }
};

class PNGDecoder {
public:
    struct FileOps {
        std::function<bool(std::string_view, void**)> Open;   // filename, handle
        int (* Read)(spng_ctx*, void*, void*, size_t);        // context, handle, buffer, size
        std::function<bool(void*)> Close;                     // handle
    };

    enum class Error {
        None,
        FileOpenFailed,
        AllocationFailed,
        HeaderParseFailed,
        DecodeFailed,
        InvalidContext,
    };

    PNGDecoder() = default;
    ~PNGDecoder() = default;
    static FileOps StdCFileOps();
    static FileOps LittleFSFileOps();
    static FileOps SDFileOps();
    static FileOps SDMMCFileOps();
    Error Decode(const std::string &filename,
                 const std::function<void(uint32_t x, int y, const RGB &color)> &pixelCallback,
                 const FileOps &fileOps,
                 bool usePSRAM = true);
private:
    Error ProcessHeader(spng_ctx* context, spng_ihdr &headerInfo);
    Error ProcessImageData(spng_ctx* context, const spng_ihdr &headerInfo, uint8_t* rowBuffer,
                           const std::function<void(uint32_t x, int y, const RGB &color)> &pixelCallback);
private:
    struct DecoderContext {
        void* FileHandle = nullptr;
        spng_ctx* SPNGContext = nullptr;
        uint8_t* RowBuffer = nullptr;
        FileOps FileOperations;
        struct spng_alloc Allocator = {};
        ~DecoderContext();
    };

    static constexpr size_t MAX_CHUNK_SIZE = 262144; // 256KB maximum for row data
    static constexpr const char* LOG_TAG = "[PNGDecoder] -";
};

#endif //LPRD_MCU_PNGDECODER_H
