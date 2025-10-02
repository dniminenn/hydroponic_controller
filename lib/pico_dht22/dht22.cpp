#include "dht22.h"
#include "hardware/sync.h"
#include <stdio.h>
#include <string.h>

DHT22::DHT22(uint8_t pin) 
    : pin_(pin), last_error_(0), last_read_time_(0) {
}

bool DHT22::begin() {
    gpio_init(pin_);
    gpio_set_dir(pin_, GPIO_OUT);
    gpio_put(pin_, 1);
    sleep_ms(250);
    printf("DHT22: Sensor initialized on pin %d\n", pin_);
    return true;
}

uint32_t DHT22::waitForPulse(bool level) {
    uint32_t count = 0;
    while (gpio_get(pin_) == level) {
        if (count++ >= DHT_MAX_CYCLES) {
            return 0;
        }
    }
    return count;
}

bool DHT22::readData(uint8_t* data) {
    memset(data, 0, 5);
    
    // Send start signal
    gpio_set_dir(pin_, GPIO_OUT);
    gpio_put(pin_, 1);
    sleep_ms(10);
    
    gpio_put(pin_, 0);
    sleep_ms(2);  // LOW for 1-10ms
    
    gpio_put(pin_, 1);
    gpio_set_dir(pin_, GPIO_IN);
    
    uint32_t prev = save_and_disable_interrupts();
    
    // Wait for sensor response (LOW)
    uint32_t timeout = 0;
    while(gpio_get(pin_) == 1) {
        if (timeout++ > DHT_MAX_CYCLES) {
            restore_interrupts(prev);
            last_error_ = 1;
            return false;
        }
    }
    
    // Sensor pulls LOW for 80µs
    if (waitForPulse(0) == 0) {
        restore_interrupts(prev);
        last_error_ = 2;
        return false;
    }
    
    // Sensor pulls HIGH for 80µs
    if (waitForPulse(1) == 0) {
        restore_interrupts(prev);
        last_error_ = 3;
        return false;
    }
    
    // Read 40 bits
    for (uint8_t i = 0; i < 40; i++) {
        // Wait for LOW period (~50µs)
        if (waitForPulse(0) == 0) {
            restore_interrupts(prev);
            last_error_ = 4;
            return false;
        }
        
        // Wait 40µs then check if still HIGH
        sleep_us(40);
        data[i / 8] <<= 1;
        if (gpio_get(pin_) == 1) {
            data[i / 8] |= 1;
        }
        
        // Wait for HIGH period to end
        timeout = 0;
        while(gpio_get(pin_) == 1) {
            if (timeout++ > DHT_MAX_CYCLES) {
                restore_interrupts(prev);
                last_error_ = 5;
                return false;
            }
        }
    }
    
    restore_interrupts(prev);
    
    sleep_us(10);
    gpio_set_dir(pin_, GPIO_OUT);
    gpio_put(pin_, 1);
    
    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (data[4] != sum) {
        last_error_ = 6;
        return false;
    }
    
    last_error_ = 0;
    return true;
}

bool DHT22::readTemperatureAndHumidity(float* temperature, float* humidity) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if ((now - last_read_time_) < MIN_READ_INTERVAL_MS) {
        last_error_ = 7;
        return false;
    }
    
    uint8_t data[5] = {0};
    
    if (!readData(data)) {
        return false;
    }
    
    last_read_time_ = now;
    
    uint16_t raw_humidity = ((uint16_t)data[0] << 8) | data[1];
    *humidity = raw_humidity * 0.1f;
    
    uint16_t raw_temp = ((uint16_t)(data[2] & 0x7F) << 8) | data[3];
    *temperature = raw_temp * 0.1f;
    if (data[2] & 0x80) {
        *temperature = -*temperature;
    }
    
    return true;
}

float DHT22::readTemperature() {
    float temperature, humidity;
    if (readTemperatureAndHumidity(&temperature, &humidity)) {
        return temperature;
    }
    return -999.0f;
}

float DHT22::readHumidity() {
    float temperature, humidity;
    if (readTemperatureAndHumidity(&temperature, &humidity)) {
        return humidity;
    }
    return -999.0f;
}

