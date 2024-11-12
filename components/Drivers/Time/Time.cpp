#include "Drivers/Time.h"
#include <spdlog/spdlog.h>
#include <sys/time.h>

namespace Time
{
    void Set(time_t timestamp) {
        spdlog::debug("[Time] Setting time to: {}", timestamp);
        struct timeval tv = {
                .tv_sec = timestamp,
                .tv_usec = 0
        };

        if (settimeofday(&tv, nullptr) != 0) {
            spdlog::error("[Time] Failed to set time");
            return;
        }

        spdlog::info("[Time] Time set to: {}", timestamp);
    }

    time_t Get() {
        spdlog::debug("[Time] Getting time");
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return tv.tv_sec;
    }
} // namespace Time