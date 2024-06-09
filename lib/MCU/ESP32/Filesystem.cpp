#ifdef MCU_ESP32

#include "Filesystem.h"
#include <Log.h>
#include <sys/stat.h>
#include <dirent.h>
#include "MCU.h"
#include <LittleFS.h>

namespace MCU { namespace Filesystem
{
    namespace
    {
        constexpr const char BasePath[] = "/filesystem";
    }

    void Init() {
        Log::Debug("[Filesystem] Initializing Filesystem");
        if (!LittleFS.begin(true, "/filesystem", 10, "storage")) {
            Log::Fatal("[Filesystem] Failed to mount or format filesystem");
            MCU::Restart();
        }
        Log::Debug("[Filesystem] Filesystem initialized");
    }

    std::string GetPath(const char* path) {
        return std::string(BasePath) + path;
    }

    std::string GetPath(std::string_view path) {
        return std::string(BasePath) + path.data();
    }

    size_t Total() {
        return LittleFS.totalBytes();
    }

    size_t Used() {
        return LittleFS.usedBytes();
    }

    size_t Free() {
        return Total() - Used();
    }

    bool exists(std::string_view path) {
        std::string nativePath = GetPath(path.data());
        struct stat st;
        if (stat(nativePath.c_str(), &st) != 0) {
            Log::Trace("[Filesystem] exists: File does not exist: %s", nativePath.c_str());
            return false;
        }
        return true;
    }

    bool rm(std::string_view path, bool recursive) {
        bool status = false;
        std::string nativePath = GetPath(path.data());
        struct stat st;
        if (stat(nativePath.c_str(), &st) != 0) {
            Log::Error("[Filesystem] rm: File does not exist: %s", path.data());
            return false;
        }

        if (S_ISDIR(st.st_mode)) {
            if (!recursive) {
                Log::Error("[Filesystem] rm: Cannot remove %s: is a directory", nativePath.c_str());
                return false;
            }
            status = LittleFS.rmdir(path.data());
        }
        else {
            status = LittleFS.remove(path.data());
        }

        if (status) { Log::Trace("[Filesystem] rm: Removed file: %s", nativePath.c_str()); }
        else { Log::Error("[Filesystem] rm: Failed to remove file: %s", nativePath.c_str()); }
        return status;
    }

    bool mv(std::string_view from, std::string_view to) {
        // TODO test when mv is used as a rename
        // TODO test when mv is used for different levels of directories
        // TODO handle edge case dir to file
        // TODO handle edge case file to dir
        if (from == to) {
            Log::Error("[Filesystem] mv: Source and destination are the same: %s", from.data());
            return false;
        }
        bool status = false;
        std::string nativeFromPath = GetPath(from.data());
        std::string nativeToPath = GetPath(to.data());
        struct stat st;
        if (stat(nativeFromPath.c_str(), &st) != 0) {
            Log::Error("[Filesystem] mv: File does not exist: %s", from.data());
            return false;
        }
        if (stat(nativeToPath.c_str(), &st) == 0) {
            Log::Trace("[Filesystem] mv: Destination file already exists removing it");
            if (!rm(to, true)) { return false; }
            status = LittleFS.rename(from.data(), to.data());
        }
        else {
            status = LittleFS.rename(from.data(), to.data());
        }

        if (status) {
            Log::Trace("[Filesystem] mv: Moved file: %s to %s",
                       nativeFromPath.c_str(),
                       nativeToPath.c_str());
        }
        else {
            Log::Error("[Filesystem] mv: Failed to move file: %s to %s",
                       nativeFromPath.c_str(),
                       nativeToPath.c_str());
        }
        return status;
    }

    std::string ls(std::string_view path) {
        std::string pathStr = GetPath(path.data());

        struct stat st;
        if (stat(pathStr.c_str(), &st) != 0) {
            Log::Error("[Filesystem] ls: Cannot access %s: No such file or directory", pathStr.c_str());
            return "";
        }

        if (S_ISDIR(st.st_mode)) {
            DIR* dir = opendir(pathStr.c_str());
            if (dir == nullptr) {
                Log::Error("[Filesystem] ls: Failed to open directory: %s", pathStr.c_str());
                return "";
            }

            std::string result;
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    result += entry->d_name;
                    result += '\n';
                }
            }
            closedir(dir);
            // Remove the last newline character
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }

            Log::Trace("[Filesystem] ls: Directory: %s", pathStr.c_str());
            return result;
        }
        else if (S_ISREG(st.st_mode)) {
            Log::Trace("[Filesystem] ls: File: %s", pathStr.c_str());
            return path.data();
        }
        else {
            Log::Error("[Filesystem] ls: Unsupported file type: %s", pathStr.c_str());
            return "";
        }
    }

    bool mkdir(std::string_view path) {
        std::string pathStr = GetPath(path.data());
        if (LittleFS.mkdir(path.data())) {
            Log::Trace("[Filesystem] mkdir: Created directory: %s", pathStr.c_str());
            return true;
        }
        Log::Error("[Filesystem] mkdir: Failed to create directory: %s", pathStr.c_str());
        return false;
    }
}} // namespace MCU::Filesystem

#endif //MCU_ESP32
