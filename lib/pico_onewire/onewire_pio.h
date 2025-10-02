#pragma once

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "onewire.pio.h"
#include <stdint.h>

class OneWirePIO {
public:
    OneWirePIO(uint8_t pin);
    ~OneWirePIO();
    
    // Reset the bus and check for presence
    bool reset();
    
    // Write a bit to the bus
    void write_bit(uint8_t bit);
    
    // Read a bit from the bus
    uint8_t read_bit();
    
    // Write a byte to the bus
    void write_byte(uint8_t byte);
    
    // Read a byte from the bus
    uint8_t read_byte();
    
    // Write multiple bytes
    void write_bytes(const uint8_t* bytes, uint16_t count);
    
    // Read multiple bytes
    void read_bytes(uint8_t* bytes, uint16_t count);
    
    // Select a specific device by ROM
    void select(const uint8_t rom[8]);
    
    // Skip ROM (use when only one device on bus)
    void skip_rom();
    
    // Search for devices on the bus
    bool search(uint8_t* addr);
    
    // Reset search state
    void reset_search();
    
    // Get the current pin
    uint8_t get_pin() const { return pin_; }
    
    // Parasitic power control
    void power_on();
    void power_off();
    
    // Static CRC calculation
    static uint8_t crc8(const uint8_t* data, uint8_t len);

private:
    uint8_t pin_;
    PIO pio_;
    uint sm_;
    uint offset_;
    
    // Search state
    uint8_t last_discrepancy_;
    uint8_t last_family_discrepancy_;
    bool last_device_flag_;
    uint8_t rom_[8];
    
    // Initialize PIO
    void init_pio();
};
