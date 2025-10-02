#include "sht30.h"
#include <stdio.h>
#include <string.h>

SHT30::SHT30(i2c_inst_t* i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t address)
    : i2c_(i2c), sda_pin_(sda_pin), scl_pin_(scl_pin), address_(address), last_error_(0) {
}

bool SHT30::begin() {
    // Initialize I2C
    i2c_init(i2c_, 100000);  // 100kHz
    
    // Set GPIO functions
    gpio_set_function(sda_pin_, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin_, GPIO_FUNC_I2C);
    
    // Enable pull-ups
    gpio_pull_up(sda_pin_);
    gpio_pull_up(scl_pin_);
    
    // Wait for sensor to stabilize
    sleep_ms(100);
    
    // Test communication
    if (!isConnected()) {
        printf("SHT30: Sensor not found at address 0x%02X\n", address_);
        return false;
    }
    
    printf("SHT30: Sensor found at address 0x%02X\n", address_);
    
    // Soft reset
    if (!reset()) {
        printf("SHT30: Reset failed\n");
        return false;
    }
    
    // Set default repeatability
    setRepeatability(REPEATABILITY_HIGH);
    
    return true;
}

bool SHT30::readTemperatureAndHumidity(float* temperature, float* humidity) {
    if (!temperature || !humidity) {
        last_error_ = 1;
        return false;
    }
    
    // Send measurement command
    if (!writeCommand(getMeasurementCommand())) {
        last_error_ = 2;
        return false;
    }
    
    // Wait for measurement to complete
    sleep_ms(getMeasurementDelay());
    
    // Read 6 bytes of data
    uint8_t data[6];
    if (!readData(data, 6)) {
        last_error_ = 3;
        return false;
    }
    
    // Verify CRC for temperature data
    if (!checkCRC(data, 2)) {
        last_error_ = 4;
        return false;
    }
    
    // Verify CRC for humidity data
    if (!checkCRC(data + 3, 2)) {
        last_error_ = 5;
        return false;
    }
    
    // Convert temperature
    uint16_t temp_raw = (data[0] << 8) | data[1];
    *temperature = -45.0f + 175.0f * (float)temp_raw / 65535.0f;
    
    // Convert humidity
    uint16_t hum_raw = (data[3] << 8) | data[4];
    *humidity = 100.0f * (float)hum_raw / 65535.0f;
    
    last_error_ = 0;
    return true;
}

float SHT30::readTemperature() {
    float temperature, humidity;
    if (readTemperatureAndHumidity(&temperature, &humidity)) {
        return temperature;
    }
    return -999.0f;  // Error value
}

float SHT30::readHumidity() {
    float temperature, humidity;
    if (readTemperatureAndHumidity(&temperature, &humidity)) {
        return humidity;
    }
    return -999.0f;  // Error value
}

void SHT30::setRepeatability(uint8_t repeatability) {
    // This is handled in getMeasurementCommand()
    // Store the setting for future use
    last_error_ = 0;
}

bool SHT30::reset() {
    return writeCommand(CMD_SOFT_RESET);
}

bool SHT30::isConnected() {
    // Try to read status register
    if (!writeCommand(CMD_STATUS)) {
        return false;
    }
    
    uint8_t data[3];
    return readData(data, 3);
}

bool SHT30::writeCommand(uint16_t command) {
    uint8_t cmd[2];
    cmd[0] = (command >> 8) & 0xFF;
    cmd[1] = command & 0xFF;
    
    int result = i2c_write_blocking(i2c_, address_, cmd, 2, false);
    return result == 2;
}

bool SHT30::readData(uint8_t* data, uint8_t length) {
    int result = i2c_read_blocking(i2c_, address_, data, length, false);
    return result == length;
}

bool SHT30::checkCRC(uint8_t* data, uint8_t length) {
    uint8_t crc = calculateCRC(data, length);
    return crc == data[length];
}

uint8_t SHT30::calculateCRC(uint8_t* data, uint8_t length) {
    uint8_t crc = 0xFF;
    
    for (uint8_t i = 0; i < length; i++) {
        crc ^= data[i];
        
        for (uint8_t bit = 8; bit > 0; bit--) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = (crc << 1);
            }
        }
    }
    
    return crc;
}

uint16_t SHT30::getMeasurementDelay() {
    // For now, always use high repeatability
    return DELAY_HIGH;
}

uint16_t SHT30::getMeasurementCommand() {
    // For now, always use high repeatability
    return CMD_MEASURE_HPM;
}
