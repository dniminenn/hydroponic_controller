#pragma once

#include <stdint.h>
#include <time.h>

class TimeUtils {
public:
    static uint32_t getSecondsFromMidnight();
    static uint32_t parseTimeToSeconds(const char* timeStr);
    static void secondsToTimeString(uint32_t seconds, char* buffer, size_t bufferSize);
    static bool isValidTimeString(const char* timeStr);
};
