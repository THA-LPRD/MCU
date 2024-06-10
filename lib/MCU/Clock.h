#ifndef MCU_CLOCK_H
#define MCU_CLOCK_H

#include <time.h>

namespace MCU { namespace Clock
{
    void SetTime(int year, int month, int day, int hour, int minute, int second, int DST = -1);
    time_t GetTime();
}}

#endif //MCU_CLOCK_H
