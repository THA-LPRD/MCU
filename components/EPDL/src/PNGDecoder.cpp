#include <spdlog/spdlog.h>
#include "PNGDecoder.h"
#include <esp_heap_caps.h>
#include "spng.h"

static struct spng_alloc CreateAllocator(bool usePSRAM) {
    struct spng_alloc alloc = {};
    if (usePSRAM) {
        alloc.malloc_fn = [](size_t size) -> void* {
            return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
        };
        alloc.realloc_fn = [](void* ptr, size_t size) -> void* {
            return heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM);
        };
        alloc.calloc_fn = [](size_t num, size_t size) -> void* {
            return heap_caps_calloc(num, size, MALLOC_CAP_SPIRAM);
        };
        alloc.free_fn = [](void* ptr) {
            heap_caps_free(ptr);
        };
    }
    else {
        alloc.malloc_fn = [](size_t size) -> void* {
            return heap_caps_malloc(size, MALLOC_CAP_INTERNAL);
        };
        alloc.realloc_fn = [](void* ptr, size_t size) -> void* {
            return heap_caps_realloc(ptr, size, MALLOC_CAP_INTERNAL);
        };
        alloc.calloc_fn = [](size_t num, size_t size) -> void* {
            return heap_caps_calloc(num, size, MALLOC_CAP_INTERNAL);
        };
        alloc.free_fn = [](void* ptr) {
            heap_caps_free(ptr);
        };
    }
    return alloc;
}

PNGDecoder::DecoderContext::~DecoderContext() {
    if (FileHandle) {
        FileOperations.Close(FileHandle);
        FileHandle = nullptr;
    }
    if (SPNGContext) {
        spng_ctx_free(SPNGContext);
        SPNGContext = nullptr;
    }
    if (RowBuffer) {
        Allocator.free_fn(RowBuffer);
        RowBuffer = nullptr;
    }
}

PNGDecoder::Error PNGDecoder::Decode(const std::string &filename,
                                     const std::function<void(uint32_t x, int y, const RGB &color)> &pixelCallback,
                                     const FileOps &fileOps,
                                     bool usePSRAM) {
    spdlog::info("{} Starting decode of file: {}", LOG_TAG, filename);

    DecoderContext context;
    context.FileOperations = fileOps;
    context.Allocator = CreateAllocator(usePSRAM);
    struct spng_ihdr headerInfo = {};

    if (!context.FileOperations.Open(filename, &context.FileHandle)) {
        spdlog::error("{} Failed to open file: {}", LOG_TAG, filename);
        return Error::FileOpenFailed;
    }

    context.SPNGContext = spng_ctx_new(0);
    if (!context.SPNGContext) {
        spdlog::error("{} Failed to create PNG context", LOG_TAG);
        return Error::InvalidContext;
    }

    spng_set_crc_action(context.SPNGContext, SPNG_CRC_USE, SPNG_CRC_USE);
    spng_set_chunk_limits(context.SPNGContext, MAX_CHUNK_SIZE, MAX_CHUNK_SIZE);
    spng_set_png_stream(context.SPNGContext, context.FileOperations.Read, context.FileHandle);

    Error error = ProcessHeader(context.SPNGContext, headerInfo);
    if (error != Error::None) {
        return error;
    }

    if (headerInfo.width == 0 || headerInfo.height == 0) {
        spdlog::error("{} Invalid image dimensions: {}x{}", LOG_TAG, headerInfo.width, headerInfo.height);
        return Error::HeaderParseFailed;
    }

    int ret = spng_decode_image(context.SPNGContext, nullptr, 0, SPNG_FMT_RGB8, SPNG_DECODE_PROGRESSIVE);
    if (ret) {
        spdlog::error("{} Progressive decode initialization failed: {}", LOG_TAG, spng_strerror(ret));
        return Error::DecodeFailed;
    }

    context.RowBuffer = reinterpret_cast<uint8_t*>(context.Allocator.malloc_fn(headerInfo.width * 3));
    if (!context.RowBuffer) {
        spdlog::error("{} Failed to allocate row buffer", LOG_TAG);
        return Error::AllocationFailed;
    }

    error = ProcessImageData(context.SPNGContext, headerInfo, context.RowBuffer, pixelCallback);

    return error;
}


PNGDecoder::Error PNGDecoder::ProcessHeader(spng_ctx* context, spng_ihdr &headerInfo) {
    spdlog::debug("{} Processing PNG header", LOG_TAG);

    if (spng_get_ihdr(context, &headerInfo) != 0) {
        spdlog::error("{} Failed to get PNG header info", LOG_TAG);
        return Error::HeaderParseFailed;
    }

    spdlog::info("{} PNG dimensions: {}x{}", LOG_TAG, headerInfo.width, headerInfo.height);
    spdlog::trace("{} Bit depth: {}", LOG_TAG, headerInfo.bit_depth);
    spdlog::trace("{} Color type: {}", LOG_TAG, headerInfo.color_type);
    spdlog::trace("{} Compression method: {}", LOG_TAG, headerInfo.compression_method);
    spdlog::trace("{} Filter method: {}", LOG_TAG, headerInfo.filter_method);
    spdlog::trace("{} Interlace method: {}", LOG_TAG, headerInfo.interlace_method);

    return Error::None;
}

PNGDecoder::Error PNGDecoder::ProcessImageData(spng_ctx* context, const spng_ihdr &headerInfo, uint8_t* rowBuffer,
                                               const std::function<void(uint32_t x, int y,
                                                                        const RGB &color)> &pixelCallback) {
    Error result = Error::None;
    int rowsProcessed = 0;

    while (rowsProcessed < headerInfo.height) {
        int ret = spng_decode_row(context, rowBuffer, headerInfo.width * 3);
        if (ret) {
            if (ret == SPNG_EOI && rowsProcessed >= headerInfo.height - 1) {
                spdlog::info("{} Successfully decoded {} rows", LOG_TAG, rowsProcessed + 1);
                return Error::None;
            }
            spdlog::error("{} Failed to decode row {} / {} : {}",
                          LOG_TAG,
                          rowsProcessed,
                          headerInfo.height,
                          spng_strerror(ret));
            return Error::DecodeFailed;
        }

        for (uint32_t i = 0; i < headerInfo.width; i++)
            pixelCallback(i, rowsProcessed, {rowBuffer[i * 3], rowBuffer[i * 3 + 1], rowBuffer[i * 3 + 2]});

        rowsProcessed++;

    }
    return result;
}
