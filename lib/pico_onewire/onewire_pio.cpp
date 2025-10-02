#include "onewire_pio.h"
#include <string.h>

OneWirePIO::OneWirePIO(uint8_t pin) 
    : pin_(pin), last_discrepancy_(0), last_family_discrepancy_(0), last_device_flag_(false) {
    memset(rom_, 0, sizeof(rom_));
    init_pio();
}

OneWirePIO::~OneWirePIO() {
    pio_sm_set_enabled(pio_, sm_, false);
    pio_remove_program(pio_, &onewire_program, offset_);
}

void OneWirePIO::init_pio() {
    // Get a free PIO and state machine
    pio_ = pio0;
    sm_ = pio_claim_unused_sm(pio_, true);
    
    // Load the program
    offset_ = pio_add_program(pio_, &onewire_program);
    
    // Initialize the program
    onewire_program_init(pio_, sm_, offset_, pin_);
}

bool OneWirePIO::reset() {
    return onewire_reset(pio_, sm_);
}

void OneWirePIO::write_bit(uint8_t bit) {
    onewire_write_bit(pio_, sm_, bit);
}

uint8_t OneWirePIO::read_bit() {
    return onewire_read_bit(pio_, sm_) ? 1 : 0;
}

void OneWirePIO::write_byte(uint8_t byte) {
    onewire_write_byte(pio_, sm_, byte);
}

uint8_t OneWirePIO::read_byte() {
    return onewire_read_byte(pio_, sm_);
}

void OneWirePIO::write_bytes(const uint8_t* bytes, uint16_t count) {
    for (uint16_t i = 0; i < count; i++) {
        write_byte(bytes[i]);
    }
}

void OneWirePIO::read_bytes(uint8_t* bytes, uint16_t count) {
    for (uint16_t i = 0; i < count; i++) {
        bytes[i] = read_byte();
    }
}

void OneWirePIO::select(const uint8_t rom[8]) {
    write_byte(0x55);  // Match ROM command
    write_bytes(rom, 8);
}

void OneWirePIO::skip_rom() {
    write_byte(0xCC);  // Skip ROM command
}

bool OneWirePIO::search(uint8_t* addr) {
    uint8_t id_bit_number = 1;
    uint8_t last_zero = 0;
    uint8_t rom_byte_number = 0;
    bool search_result = false;
    uint8_t rom_bit_mask = 1;
    uint8_t search_direction = 0;
    
    // If the last call was not the last one
    if (!last_device_flag_) {
        // 1-Wire reset
        if (!reset()) {
            // Reset the search
            last_discrepancy_ = 0;
            last_device_flag_ = false;
            return false;
        }
        
        // Issue the search command
        write_byte(0xF0);
        
        // Loop to do the search
        do {
            // Read a bit and its complement
            uint8_t id_bit = read_bit();
            uint8_t cmp_id_bit = read_bit();
            
            // Check for no devices on 1-wire
            if ((id_bit == 1) && (cmp_id_bit == 1)) {
                break;
            } else {
                // All devices coupled have 0 or 1
                if (id_bit != cmp_id_bit) {
                    // Bit write value for search
                    search_direction = id_bit;  // Bit 0 or 1
                } else {
                    // If this discrepancy is before the Last Discrepancy
                    // on a previous next then pick the same as last time
                    if (id_bit_number < last_discrepancy_) {
                        search_direction = ((rom_[rom_byte_number] & rom_bit_mask) > 0);
                    } else {
                        // If equal to last pick 1, if not then pick 0
                        search_direction = (id_bit_number == last_discrepancy_);
                    }
                    
                    // If 0 was picked then record its position in LastZero
                    if (search_direction == 0) {
                        last_zero = id_bit_number;
                    }
                }
                
                // Set or clear the bit in the ROM byte rom_byte_number
                // with mask rom_bit_mask
                if (search_direction == 1) {
                    rom_[rom_byte_number] |= rom_bit_mask;
                } else {
                    rom_[rom_byte_number] &= ~rom_bit_mask;
                }
                
                // Serial number search direction write bit
                write_bit(search_direction);
                
                // Increment the byte counter id_bit_number
                // and shift the mask rom_bit_mask
                id_bit_number++;
                rom_bit_mask <<= 1;
                
                // If the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
                if (rom_bit_mask == 0) {
                    rom_[rom_byte_number] = rom_[rom_byte_number];
                    rom_byte_number++;
                    rom_bit_mask = 1;
                }
            }
        } while (rom_byte_number < 8);  // Loop until through all ROM bytes 0-7
        
        // If the search was successful then
        if (!(id_bit_number < 65)) {
            // Search successful so set LastDiscrepancy,LastDeviceFlag,search_result
            last_discrepancy_ = last_zero;
            
            // Check for last device
            if (last_discrepancy_ == 0) {
                last_device_flag_ = true;
            }
            search_result = true;
        }
    }
    
    // If no device found then reset counters so next 'search' will be like a first
    if (!search_result || !rom_[0]) {
        last_discrepancy_ = 0;
        last_device_flag_ = false;
        search_result = false;
    } else {
        // Copy the ROM to the address
        for (uint8_t i = 0; i < 8; i++) {
            addr[i] = rom_[i];
        }
    }
    
    return search_result;
}

void OneWirePIO::reset_search() {
    last_discrepancy_ = 0;
    last_device_flag_ = false;
    for (uint8_t i = 0; i < 8; i++) {
        rom_[i] = 0;
    }
}

void OneWirePIO::power_on() {
    // Disable PIO state machine
    pio_sm_set_enabled(pio_, sm_, false);
    
    // Configure pin as regular GPIO output
    gpio_init(pin_);
    gpio_set_dir(pin_, GPIO_OUT);
    gpio_put(pin_, 1);  // Drive HIGH for strong pullup
}

void OneWirePIO::power_off() {
    // Re-initialize PIO state machine
    pio_sm_set_enabled(pio_, sm_, false);
    onewire_program_init(pio_, sm_, offset_, pin_);
}

// CRC8 calculation for OneWire
uint8_t OneWirePIO::crc8(const uint8_t* data, uint8_t len) {
    uint8_t crc = 0;
    
    for (uint8_t i = 0; i < len; i++) {
        uint8_t inbyte = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8C;
            }
            inbyte >>= 1;
        }
    }
    
    return crc;
}
