#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

class GpioUtils {
public:
    static void initializeGpioOutputs();
    static void setRelay(uint8_t pin, bool on);
    static void setAllRelaysOff();
};
