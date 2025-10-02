#include "time_utils.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

uint32_t TimeUtils::getSecondsFromMidnight() {
    time_t now = time(nullptr);
    if (now < 1600000000) return 0;  // No valid time available
    
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    return (uint32_t)timeinfo.tm_hour * 3600U + 
           (uint32_t)timeinfo.tm_min * 60U + 
           (uint32_t)timeinfo.tm_sec;
}

uint32_t TimeUtils::parseTimeToSeconds(const char* timeStr) {
    if (!timeStr) return 0;
    
    int hours, minutes;
    if (sscanf(timeStr, "%d:%d", &hours, &minutes) != 2) return 0;
    
    if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59) return 0;
    
    return hours * 3600U + minutes * 60U;
}

void TimeUtils::secondsToTimeString(uint32_t seconds, char* buffer, size_t bufferSize) {
    uint32_t hours = seconds / 3600;
    uint32_t minutes = (seconds % 3600) / 60;
    snprintf(buffer, bufferSize, "%02lu:%02lu", hours, minutes);
}

bool TimeUtils::isValidTimeString(const char* timeStr) {
    if (!timeStr) return false;
    
    int hours, minutes;
    if (sscanf(timeStr, "%d:%d", &hours, &minutes) != 2) return false;
    
    return (hours >= 0 && hours <= 23 && minutes >= 0 && minutes <= 59);
}
