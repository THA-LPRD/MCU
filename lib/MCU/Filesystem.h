#ifndef MCU_FILESYSTEM_H
#define MCU_FILESYSTEM_H

#include <string>

namespace MCU { namespace Filesystem
{
    void Init();
    std::string GetPath(const char* path);
    size_t Total();
    size_t Used();
    size_t Free();
    bool rm(std::string_view path, bool recursive = false);
    bool mv(std::string_view from, std::string_view to);
    std::string ls(std::string_view path);
}} // namespace MCU::Filesystem

#endif //MCU_FILESYSTEM_H
