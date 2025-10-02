#pragma once

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdint.h>

class DHT22 {
public:
    DHT22(uint8_t pin);
    
    bool begin();
    bool readTemperatureAndHumidity(float* temperature, float* humidity);
    float readTemperature();
    float readHumidity();
    uint8_t getLastError() const { return last_error_; }
    
private:
    uint8_t pin_;
    uint8_t last_error_;
    uint32_t last_read_time_;
    
    static const uint32_t MIN_READ_INTERVAL_MS = 2000;
    static const uint32_t DHT_TIMEOUT_US = 1000;
    static const uint32_t DHT_MAX_CYCLES = 10000;
    
    bool readData(uint8_t* data);
    uint32_t waitForPulse(bool level);
};

