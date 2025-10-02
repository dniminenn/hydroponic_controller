#include "ds18b20.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

DS18B20::DS18B20(OneWirePIO* oneWire, bool parasiticPower) 
    : oneWire_(oneWire), device_count_(0), resolution_(RES_12_BIT), 
      last_request_time_(0), conversion_pending_(false), parasitic_power_(parasiticPower) {
    memset(device_addresses_, 0, sizeof(device_addresses_));
}

bool DS18B20::begin() {
    // Search for devices on the bus
    searchForDevices();
    
    if (device_count_ == 0) {
        printf("DS18B20: No devices found\n");
        return false;
    }
    
    printf("DS18B20: Found %d device(s)\n", device_count_);
    
    // Set resolution for all devices
    for (uint8_t i = 0; i < device_count_; i++) {
        setResolution(resolution_);
    }
    
    return true;
}

bool DS18B20::requestTemperatures() {
    if (device_count_ == 0) {
        return false;
    }
    
    // Reset the bus
    if (!oneWire_->reset()) {
        return false;
    }
    
    // Skip ROM to address all devices
    oneWire_->skip_rom();
    
    // Send convert temperature command
    oneWire_->write_byte(CMD_CONVERT_T);
    
    // For parasitic power, hold DQ line HIGH during conversion
    if (parasitic_power_) {
        oneWire_->power_on();
    }
    
    last_request_time_ = to_ms_since_boot(get_absolute_time());
    conversion_pending_ = true;
    
    return true;
}

float DS18B20::getTempC() {
    if (device_count_ == 0) {
        return DEVICE_DISCONNECTED_C;
    }
    
    // If conversion is pending, wait for it to complete
    if (conversion_pending_) {
        blockTillConversionComplete();
        
        // Turn off parasitic power after conversion
        if (parasitic_power_) {
            oneWire_->power_off();
        }
    }
    
    // Read temperature from first device
    uint8_t scratchPad[9];
    if (!readScratchPad(device_addresses_[0], scratchPad)) {
        return DEVICE_DISCONNECTED_C;
    }
    
    return calculateTemperature(device_addresses_[0], scratchPad);
}

float DS18B20::getTempF() {
    return getTempC() * 9.0f / 5.0f + 32.0f;
}

bool DS18B20::isConversionComplete() {
    if (!conversion_pending_) {
        return true;
    }
    
    uint32_t now = to_ms_since_boot(get_absolute_time());
    uint16_t conversion_time = getConversionTime();
    
    if (now - last_request_time_ >= conversion_time) {
        conversion_pending_ = false;
        return true;
    }
    
    return false;
}

void DS18B20::blockTillConversionComplete() {
    while (!isConversionComplete()) {
        sleep_ms(10);
    }
}

bool DS18B20::getAddress(uint8_t* deviceAddress, uint8_t index) {
    if (index >= device_count_) {
        return false;
    }
    
    memcpy(deviceAddress, device_addresses_[index], 8);
    return true;
}

void DS18B20::setResolution(uint8_t newResolution) {
    resolution_ = newResolution;
    
    // Set resolution for all devices
    for (uint8_t i = 0; i < device_count_; i++) {
        uint8_t scratchPad[9];
        
        // Read current scratchpad
        if (!readScratchPad(device_addresses_[i], scratchPad)) {
            continue;
        }
        
        // Update resolution
        scratchPad[4] = resolution_;
        
        // Write back to scratchpad
        writeScratchPad(device_addresses_[i], scratchPad);
        
        // Copy to EEPROM
        copyScratchPad(device_addresses_[i]);
    }
}

bool DS18B20::isConnected(const uint8_t* deviceAddress) {
    uint8_t scratchPad[9];
    return readScratchPad(deviceAddress, scratchPad);
}

bool DS18B20::readScratchPad(const uint8_t* deviceAddress, uint8_t* scratchPad) {
    if (!oneWire_->reset()) {
        return false;
    }
    
    if (deviceAddress == nullptr) {
        oneWire_->skip_rom();
    } else {
        oneWire_->select(deviceAddress);
    }
    
    oneWire_->write_byte(CMD_READ_SCRATCHPAD);
    oneWire_->read_bytes(scratchPad, 9);
    
    // Verify CRC
    if (OneWirePIO::crc8(scratchPad, 8) != scratchPad[8]) {
        return false;
    }
    
    return true;
}

bool DS18B20::writeScratchPad(const uint8_t* deviceAddress, const uint8_t* scratchPad) {
    if (!oneWire_->reset()) {
        return false;
    }
    
    if (deviceAddress == nullptr) {
        oneWire_->skip_rom();
    } else {
        oneWire_->select(deviceAddress);
    }
    
    oneWire_->write_byte(CMD_WRITE_SCRATCHPAD);
    oneWire_->write_bytes(scratchPad, 3);  // Only write first 3 bytes
    
    return true;
}

bool DS18B20::copyScratchPad(const uint8_t* deviceAddress) {
    if (!oneWire_->reset()) {
        return false;
    }
    
    if (deviceAddress == nullptr) {
        oneWire_->skip_rom();
    } else {
        oneWire_->select(deviceAddress);
    }
    
    oneWire_->write_byte(CMD_COPY_SCRATCHPAD);
    
    // Wait for copy to complete (up to 10ms)
    sleep_ms(10);
    
    return true;
}

bool DS18B20::recallScratchPad(const uint8_t* deviceAddress) {
    if (!oneWire_->reset()) {
        return false;
    }
    
    if (deviceAddress == nullptr) {
        oneWire_->skip_rom();
    } else {
        oneWire_->select(deviceAddress);
    }
    
    oneWire_->write_byte(CMD_RECALL_EE);
    
    // Wait for recall to complete (up to 10ms)
    sleep_ms(10);
    
    return true;
}

bool DS18B20::readPowerSupply(const uint8_t* deviceAddress) {
    if (!oneWire_->reset()) {
        return false;
    }
    
    if (deviceAddress == nullptr) {
        oneWire_->skip_rom();
    } else {
        oneWire_->select(deviceAddress);
    }
    
    oneWire_->write_byte(CMD_READ_POWER_SUPPLY);
    
    return oneWire_->read_bit();
}

float DS18B20::calculateTemperature(const uint8_t* deviceAddress, const uint8_t* scratchPad) {
    int16_t raw = (scratchPad[1] << 8) | scratchPad[0];
    
    // Check for invalid reading
    if (raw == 0x0550) {  // 85°C is power-on reset value
        return DEVICE_DISCONNECTED_C;
    }
    
    // Check resolution
    uint8_t cfg = scratchPad[4];
    uint8_t resolution = 9 + ((cfg >> 5) & 0x03);
    
    // Mask out unused bits based on resolution
    switch (resolution) {
        case 9:
            raw &= ~7;  // Clear bits 0-2
            break;
        case 10:
            raw &= ~3;  // Clear bits 0-1
            break;
        case 11:
            raw &= ~1;  // Clear bit 0
            break;
        case 12:
        default:
            // No masking needed
            break;
    }
    
    return (float)raw / 16.0f;
}

uint16_t DS18B20::getConversionTime() {
    switch (resolution_) {
        case RES_9_BIT:
            return CONV_TIME_9_BIT;
        case RES_10_BIT:
            return CONV_TIME_10_BIT;
        case RES_11_BIT:
            return CONV_TIME_11_BIT;
        case RES_12_BIT:
        default:
            return CONV_TIME_12_BIT;
    }
}

void DS18B20::searchForDevices() {
    device_count_ = 0;
    oneWire_->reset_search();
    
    uint8_t addr[8];
    while (oneWire_->search(addr) && device_count_ < 8) {
        // Check if it's a DS18B20 (family code 0x28)
        if (addr[0] == 0x28) {
            memcpy(device_addresses_[device_count_], addr, 8);
            device_count_++;
        }
    }
}

