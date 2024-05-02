#ifndef CLOCK_H_
#define CLOCK_H_

#include <sys/time.h>

namespace Clock
{
    void SetTime(int year, int month, int day, int hour, int minute, int second, int DST = -1);
    inline time_t GetTime() {
        timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec;
    }
}

#endif /*CLOCK_H_*/
