#include <Arduino.h>
#include <sys/time.h>

namespace Clock
{
    void SetTime(int year, int month, int day, int hour, int minute, int second, int DST = -1) {
        struct tm t = { 0 };
        t.tm_year = year - 1900;
        t.tm_mon = month - 1;
        t.tm_mday = day;
        t.tm_hour = hour;
        t.tm_min = minute;
        t.tm_sec = second;
        t.tm_isdst = DST;

        timeval tv = { 0 };
        tv.tv_sec = mktime(&t);

        settimeofday(&tv, NULL);
    }

    time_t GetTime() {
        timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec;
    }

}