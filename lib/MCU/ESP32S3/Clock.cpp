#ifdef MCU_ESP32S3

#include "Clock.h"
#include <sys/time.h>
#include <Log.h>

namespace MCU { namespace Clock
{
    void SetTime(int year, int month, int day, int hour, int minute, int second, int DST) {
        Log::Debug("[Clock] Setting time to %d-%d-%d %d:%d:%d", year, month, day, hour, minute, second);
        struct tm timeinfo = {0};
        timeinfo.tm_year = year - 1900;
        timeinfo.tm_mon = month - 1;
        timeinfo.tm_mday = day;
        timeinfo.tm_hour = hour;
        timeinfo.tm_min = minute;
        timeinfo.tm_sec = second;
        timeinfo.tm_isdst = DST;

        timeval tv = {0};
        tv.tv_sec = mktime(&timeinfo);

        settimeofday(&tv, NULL);
    }

    time_t GetTime() {
        timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec;
    }
}} // namespace MCU::Clock

#endif //MCU_ESP32S3