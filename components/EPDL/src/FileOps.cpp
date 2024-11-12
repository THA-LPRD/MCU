#include "PNGDecoder.h"
#include <cstdio>
#include <SD.h>
#include <LittleFS.h>
#include <SD_MMC.h>


PNGDecoder::FileOps PNGDecoder::StdCFileOps() {
    FileOps ops;

    ops.Open = [](std::string_view filename, void** handle) -> bool {
        FILE* file = fopen(filename.data(), "rb");
        *handle = file;
        return file != nullptr;
    };

    ops.Read = [](spng_ctx* context, void* handle, void* buffer, size_t size) -> int {
        FILE* file = static_cast<FILE*>(handle);

        if (fread(buffer, size, 1, file) != 1) {
            if (feof(file)) { return SPNG_IO_EOF; }
            else { return SPNG_IO_ERROR; }
        }
        return 0;

    };

    ops.Close = [](void* handle) -> bool {
        return fclose(static_cast<FILE*>(handle)) == 0;
    };

    return ops;
}

PNGDecoder::FileOps PNGDecoder::LittleFSFileOps() {
    FileOps ops;

    ops.Open = [](std::string_view filename, void** handle) -> bool {
        File* file = new File();
        *file = LittleFS.open(filename.data(), FILE_READ);
        *handle = file;
        return *file;
    };

    ops.Read = [](spng_ctx* context, void* handle, void* buffer, size_t size) -> int {
        auto file = static_cast<File*>(handle);

        if (!file->available()) {
            return SPNG_IO_EOF;
        }

        size_t bytesRead = file->readBytes((char*) buffer, size);

        if (bytesRead != size) {
            if (file->available()) {
                return SPNG_IO_ERROR;
            }
            return SPNG_IO_ERROR;
        }

        return 0;

    };

    ops.Close = [](void* handle) -> bool {
        auto file = static_cast<File*>(handle);
        file->close();
        delete file;
        return true;
    };

    return ops;
}

PNGDecoder::FileOps PNGDecoder::SDFileOps() {
    FileOps ops;

    ops.Open = [](std::string_view filename, void** handle) -> bool {
        File* file = new File();
        *file = SD.open(filename.data(), FILE_READ);
        *handle = file;
        return *file;
    };

    ops.Read = [](spng_ctx* context, void* handle, void* buffer, size_t size) -> int {
        auto file = static_cast<File*>(handle);

        if (!file->available()) {
            return SPNG_IO_EOF;
        }

        size_t bytesRead = file->readBytes((char*) buffer, size);

        if (bytesRead != size) {
            if (file->available()) {
                return SPNG_IO_ERROR;
            }
            return SPNG_IO_ERROR;
        }

        return 0;
    };

    ops.Close = [](void* handle) -> bool {
        auto file = static_cast<File*>(handle);
        file->close();
        delete file;
        return true;
    };

    return ops;
}

PNGDecoder::FileOps PNGDecoder::SDMMCFileOps() {
    FileOps ops;

    ops.Open = [](std::string_view filename, void** handle) -> bool {
        File* file = new File();
        *file = SD_MMC.open(filename.data(), FILE_READ);
        *handle = file;
        return *file;
    };

    ops.Read = [](spng_ctx* context, void* handle, void* buffer, size_t size) -> int {
        auto file = static_cast<File*>(handle);

        if (!file->available()) {
            return SPNG_IO_EOF;
        }

        size_t bytesRead = file->readBytes((char*) buffer, size);

        if (bytesRead != size) {
            if (file->available()) {
                return SPNG_IO_ERROR;
            }
            return SPNG_IO_ERROR;
        }

        return 0;

    };

    ops.Close = [](void* handle) -> bool {
        auto file = static_cast<File*>(handle);
        file->close();
        delete file;
        return true;
    };

    return ops;
}
