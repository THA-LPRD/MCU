#ifndef LPRD_MCU_TIME_H
#define LPRD_MCU_TIME_H

#include <ctime>
#include <string_view>

namespace Time
{
    /**
     * @brief Sets the system time (UTC)
     * @param timestamp Unix timestamp to set
     */
    void Set(time_t timestamp);

    /**
     * @brief Gets the current system time (UTC)
     * @return Current Unix timestamp
     */
    time_t Get();
} // namespace Time

#endif //LPRD_MCU_TIME_H
