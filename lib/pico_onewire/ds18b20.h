#pragma once

#include "onewire_pio.h"
#include <stdint.h>

// Constants
#define DEVICE_DISCONNECTED_C -127.0f

class DS18B20 {
public:
    DS18B20(OneWirePIO* oneWire, bool parasiticPower = false);
    
    // Initialize the sensor
    bool begin();
    
    // Request temperature conversion
    bool requestTemperatures();
    
    // Get temperature in Celsius
    float getTempC();
    
    // Get temperature in Fahrenheit
    float getTempF();
    
    // Check if conversion is complete
    bool isConversionComplete();
    
    // Wait for conversion to complete
    void blockTillConversionComplete();
    
    // Get device count
    uint8_t getDeviceCount() const { return device_count_; }
    
    // Get device address by index
    bool getAddress(uint8_t* deviceAddress, uint8_t index);
    
    // Set resolution (9, 10, 11, or 12 bits)
    void setResolution(uint8_t newResolution);
    
    // Get resolution
    uint8_t getResolution() const { return resolution_; }

private:
    OneWirePIO* oneWire_;
    uint8_t device_count_;
    uint8_t device_addresses_[8][8];  // Support up to 8 devices
    uint8_t resolution_;
    uint32_t last_request_time_;
    bool conversion_pending_;
    bool parasitic_power_;
    
    // DS18B20 commands
    static const uint8_t CMD_CONVERT_T = 0x44;
    static const uint8_t CMD_READ_SCRATCHPAD = 0xBE;
    static const uint8_t CMD_WRITE_SCRATCHPAD = 0x4E;
    static const uint8_t CMD_COPY_SCRATCHPAD = 0x48;
    static const uint8_t CMD_RECALL_EE = 0xB8;
    static const uint8_t CMD_READ_POWER_SUPPLY = 0xB4;
    
    // Resolution settings
    static const uint8_t RES_9_BIT = 0x1F;   // 0.5°C, 93.75ms
    static const uint8_t RES_10_BIT = 0x3F;  // 0.25°C, 187.5ms
    static const uint8_t RES_11_BIT = 0x5F;  // 0.125°C, 375ms
    static const uint8_t RES_12_BIT = 0x7F;  // 0.0625°C, 750ms
    
    // Conversion times (in milliseconds)
    static const uint16_t CONV_TIME_9_BIT = 94;
    static const uint16_t CONV_TIME_10_BIT = 188;
    static const uint16_t CONV_TIME_11_BIT = 375;
    static const uint16_t CONV_TIME_12_BIT = 750;
    
    // Helper functions
    bool isConnected(const uint8_t* deviceAddress);
    bool readScratchPad(const uint8_t* deviceAddress, uint8_t* scratchPad);
    bool writeScratchPad(const uint8_t* deviceAddress, const uint8_t* scratchPad);
    bool copyScratchPad(const uint8_t* deviceAddress);
    bool recallScratchPad(const uint8_t* deviceAddress);
    bool readPowerSupply(const uint8_t* deviceAddress);
    float calculateTemperature(const uint8_t* deviceAddress, const uint8_t* scratchPad);
    uint16_t getConversionTime();
    void searchForDevices();
};
