#pragma once

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include <stdint.h>

class SHT30 {
public:
    SHT30(i2c_inst_t* i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t address = 0x44);
    
    // Initialize the sensor
    bool begin();
    
    // Read temperature and humidity
    bool readTemperatureAndHumidity(float* temperature, float* humidity);
    
    // Read temperature only
    float readTemperature();
    
    // Read humidity only
    float readHumidity();
    
    // Set measurement repeatability
    void setRepeatability(uint8_t repeatability);
    
    // Soft reset the sensor
    bool reset();
    
    // Check if sensor is connected
    bool isConnected();
    
    // Get last error
    uint8_t getLastError() const { return last_error_; }

private:
    i2c_inst_t* i2c_;
    uint8_t sda_pin_;
    uint8_t scl_pin_;
    uint8_t address_;
    uint8_t last_error_;
    
    // SHT30 commands
    static const uint16_t CMD_MEASURE_HPM = 0x2400;  // High repeatability
    static const uint16_t CMD_MEASURE_MPM = 0x240B;  // Medium repeatability
    static const uint16_t CMD_MEASURE_LPM = 0x2416;  // Low repeatability
    static const uint16_t CMD_SOFT_RESET = 0x30A2;
    static const uint16_t CMD_HEATER_ENABLE = 0x306D;
    static const uint16_t CMD_HEATER_DISABLE = 0x3066;
    static const uint16_t CMD_STATUS = 0xF32D;
    static const uint16_t CMD_CLEAR_STATUS = 0x3041;
    
    // Repeatability settings
    static const uint8_t REPEATABILITY_HIGH = 0;
    static const uint8_t REPEATABILITY_MEDIUM = 1;
    static const uint8_t REPEATABILITY_LOW = 2;
    
    // Measurement delays (in milliseconds)
    static const uint16_t DELAY_HIGH = 15;
    static const uint16_t DELAY_MEDIUM = 6;
    static const uint16_t DELAY_LOW = 4;
    
    // Helper functions
    bool writeCommand(uint16_t command);
    bool readData(uint8_t* data, uint8_t length);
    bool checkCRC(uint8_t* data, uint8_t length);
    uint8_t calculateCRC(uint8_t* data, uint8_t length);
    uint16_t getMeasurementDelay();
    uint16_t getMeasurementCommand();
};
